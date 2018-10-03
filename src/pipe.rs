use imgref::{ImgVec};
use nalg::Point2;
use camera::DefocusCamera;
use sample::{World, SampleParams};
use dynpool::{Worker, Context, Pool, Scale, Decision};
use channel::{Receiver, Sender};
use std::sync::{Arc, atomic::{AtomicBool, Ordering::SeqCst}};
use palette::{Srgba, Alpha};
use rand::{self, ThreadRng};
use failure::Error;

pub struct FrameData {
    pub world: World,
    pub cam: DefocusCamera,
    pub params: SampleParams,
}

pub struct Tile {
    pub frame_num: u32,
    pub top: usize,
    pub left: usize,
    pub buf: ImgVec<Srgba<u8>>,
    pub frame: Arc<FrameData>,
}

pub struct RenderCtx {
    pub input: Receiver<Tile>,
    pub output: Sender<Tile>,
    pub size: (usize, usize),
    pub threads: usize,
    pub running: AtomicBool,
}

impl Context for RenderCtx {
    type Worker = Renderer;

    fn init(&self, _: usize) -> Renderer {
        Renderer {
            rng: rand::thread_rng(),
        }
    }

    fn scale(&self) -> Scale {
        match self.running.load(SeqCst) {
            true => Scale::active(self.threads),
            false => Scale::shutdown(),
        }
    }
}

pub struct Renderer {
    pub rng: ThreadRng,
}

impl Worker<RenderCtx> for Renderer {
    fn work(&mut self, ctx: &RenderCtx) -> Decision {
        use crate::sample::sample_pixel;

        let mut tile = match ctx.input.recv() {
            Some(tile) => tile,
            None => return Decision::Again,
        };

        let pixel_width = 1. / ctx.size.1.max(ctx.size.0) as f64;
        for (y, row) in tile.buf.rows_mut().enumerate() {
            let y = (y + tile.top) as f64 * pixel_width;
            for (x, px) in row.iter_mut().enumerate() {
                let x = (x + tile.left) as f64 * pixel_width;

                *px = Srgba::from_linear(Alpha {
                    color: sample_pixel(
                        &tile.frame.cam,
                        &tile.frame.world,
                        Point2::new(x * 2. - 1., y * 2. - 1.),
                        pixel_width,
                        &mut self.rng,
                        &tile.frame.params,
                    ),
                    alpha: 1.,
                }).into_format()
            }
        }

        ctx.output.send(tile);

        Decision::Incomplete
    }
}

#[derive(Copy, Clone, Debug)]
pub struct RenderParams {
    pub width: usize,
    pub height: usize,
    pub tile_size: usize,
    pub tile_queue: usize,
    pub threads: usize,
}

impl RenderParams {
    pub fn tiles_per_frame(&self) -> usize {
        let tile_num_x = (self.width + self.tile_size - 1) / self.tile_size;
        let tile_num_y = (self.height + self.tile_size - 1) / self.tile_size;

        tile_num_x * tile_num_y
    }

    pub fn uninitialized_frame(&self) -> FullFrame {
        use std::mem::uninitialized;

        FullFrame {
            buf: unsafe { ImgVec::new(vec![uninitialized(); self.width * self.height], self.width, self.height) },
            todo_tiles: self.tiles_per_frame(),
        }
    }
}

#[derive(Clone, Debug)]
pub struct FullFrame {
    pub buf: ImgVec<Srgba<u8>>,
    todo_tiles: usize,
}

impl FullFrame {
    pub fn tile_ready(&mut self, tile: &Tile) {
        self.todo_tiles -= 1;
        for (to, from) in self.buf.sub_image_mut(
            tile.left,
            tile.top,
            tile.buf.width(),
            tile.buf.height()
        ).rows_mut()
        .zip(tile.buf.rows()) {
            to.copy_from_slice(from);
        }
    }

    pub fn is_done(&self) -> bool { self.todo_tiles == 0 }
}

impl RenderParams {
    fn tiles(self, frame_num: u32, frame: Arc<FrameData>) -> impl Iterator<Item=Tile> {
        struct Tiles {
            left: usize,
            top: usize,
            params: RenderParams,
            frame: Arc<FrameData>,
            frame_num: u32,
        }

        impl Iterator for Tiles {
            type Item = Tile;

            fn next(&mut self) -> Option<Tile> {
                if self.top >= self.params.height { return None }

                let w = (self.params.width - self.left).min(self.params.tile_size);
                let h = (self.params.height - self.top).min(self.params.tile_size);
                let tile = Tile {
                    buf: ImgVec::new(vec![Srgba::new(0, 0, 0, 255); w * h], w, h),
                    top: self.top,
                    left: self.left,
                    frame: self.frame.clone(),
                    frame_num: self.frame_num,
                };

                self.left += self.params.tile_size;
                if self.left >= self.params.width {
                    self.left = 0;
                    self.top += self.params.tile_size;
                }

                Some(tile)
            }
        }

        Tiles {
            left: 0,
            top: 0,
            params: self,
            frame,
            frame_num,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TickResult {
    Run,
    Exit,
}

pub fn render_pipeline(
    mut frames: impl FnMut(u32) -> Result<Option<FrameData>, Error>,
    mut rendered: impl FnMut(Tile) -> Result<(), Error>,
    mut tick: impl FnMut() -> TickResult,
    tick_ms: u64,
    params: RenderParams
) -> Result<(), Error> {
    use std::time::Duration;
    use channel::{bounded, unbounded, tick as tickrecv, Select};
    use std::mem::drop;

    let (is, ir) = bounded(params.tile_queue);
    let (cs, cr) = unbounded();
    let ticker = tickrecv(Duration::from_millis(tick_ms));

    let pool = Pool::start_bg(RenderCtx {
        input: ir,
        output: cs,
        size: (params.width, params.height),
        threads: params.threads,
        running: AtomicBool::new(true),
    });

    let mut running = true;
    'frames: for frame_num in 0.. {
        let frame = match frames(frame_num)? {
            Some(f) => Arc::new(f),
            None => break,
        };
        let mut tiles = params.tiles(frame_num, frame);

        let mut tile = tiles.next();
        loop {
            if !running { break 'frames }
            if tile.is_none() { break }

            Select::<Result<_, Error>>::new()
                .recv(&cr, |tile_done| {
                    if let Some(tile_done) = tile_done { rendered(tile_done)?; }
                    Ok(())
                })
                .recv(&ticker, |_| {
                    if tick() == TickResult::Exit { running = false }
                    Ok(())
                })
                .send(&is, || tile.take().unwrap(), || Ok(()))
                .wait()?;

            if tile.is_none() { tile = tiles.next() }
        }
    }

    drop(is);
    pool.ctx().running.store(false, SeqCst);

    while running && (pool.thread_count() > 0 || !cr.is_empty()) {
        Select::<Result<_, Error>>::new()
            .recv(&cr, |tile_done| {
                if let Some(tile_done) = tile_done { rendered(tile_done)?; }
                Ok(())
            })
            .recv(&ticker, |_| {
                if tick() == TickResult::Exit { running = false }
                Ok(())
            })
            .wait()?;
    }

    Ok(())
}
