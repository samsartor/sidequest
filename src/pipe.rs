use imgref::ImgVec;
use nalg::Point2;
use camera::DefocusCamera;
use sample::{World, SampleParams};
use dynpool::{Worker, Context, Pool, Scale, Decision};
use channel::{Receiver, Sender};
use std::sync::{Arc, atomic::{AtomicBool, Ordering::Relaxed}};
use palette::{Srgb};
use rand::{self, ThreadRng};

pub struct FrameData {
    pub world: World,
    pub cam: DefocusCamera,
    pub params: SampleParams,
}

pub struct Tile {
    pub frame_num: usize,
    pub buf: ImgVec<Srgb<u8>>,
    pub origin: (usize, usize),
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
        match self.running.load(Relaxed) {
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
            let y = (y + tile.origin.1) as f64 * pixel_width;
            for (x, px) in row.iter_mut().enumerate() {
                let x = (x + tile.origin.0) as f64 * pixel_width;

                *px = Srgb::from_linear(sample_pixel(
                    &tile.frame.cam,
                    &tile.frame.world,
                    Point2::new(x, y),
                    pixel_width,
                    &mut self.rng,
                    &tile.frame.params,
                )).into_format()
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
    fn tiles(self, frame_num: usize, frame: Arc<FrameData>) -> impl Iterator<Item=Tile> {
        struct Tiles {
            left: usize,
            top: usize,
            params: RenderParams,
            frame: Arc<FrameData>,
            frame_num: usize,
        }

        impl Iterator for Tiles {
            type Item = Tile;

            fn next(&mut self) -> Option<Tile> {
                if self.top >= self.params.height { return None }

                let w = (self.params.width - self.left).max(self.params.tile_size);
                let h = (self.params.height - self.top).max(self.params.tile_size);
                let tile = Tile {
                    buf: ImgVec::new(vec![Srgb::new(0, 0, 0); w * h], w, h),
                    origin: (self.left, self.top),
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

pub fn render_pipeline(
    mut frames: impl FnMut(usize) -> Option<FrameData>,
    mut rendered: impl FnMut(Tile),
    params: RenderParams
) {
    use channel::{bounded, Select};

    let (is, ir) = bounded(params.tile_queue);
    let (cs, cr) = bounded(params.tile_queue);
    let pool = Pool::start_bg(RenderCtx {
        input: ir,
        output: cs,
        size: (params.width, params.height),
        threads: params.threads,
        running: AtomicBool::new(true),
    });

    for frame_num in 0.. {
        let frame = match frames(frame_num) {
            Some(f) => Arc::new(f),
            None => break,
        };
        let mut tiles = params.tiles(frame_num, frame);

        let mut tile = tiles.next();
        loop {
            if tile.is_none() { break }

            Select::new()
                .recv(&cr, |tile| if let Some(tile) = tile { rendered(tile); })
                .send(&is, || tile.take().unwrap(), || ())
                .wait();

            if tile.is_none() { tile = tiles.next() }
        }
    }

    pool.ctx().running.store(false, Relaxed);
}
