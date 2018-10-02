extern crate failure;
extern crate rayon;
extern crate image;
extern crate indicatif;
extern crate nalgebra as nalg;
extern crate ncollide as ncol;
extern crate imgref;
extern crate palette;
extern crate rand;
extern crate num_traits;
extern crate sdl2;
extern crate dynpool;
extern crate crossbeam_channel as channel;

pub mod sample;
pub mod camera;
pub mod stats;
pub mod pipe;

use failure::Error;

const FRAME_SIZE: usize = 768;
const FRAME_NUM: usize = 1;
const SAMPLE_NUM: usize = 256;
const BOUNCE_LIMIT: usize = 4;
const TILE_SIZE: usize = 32;
const THREADS: usize = 8;

fn main() -> Result<(), Error> {
    use nalg::{Isometry3, Point3, Vector3};
    use camera::{DefocusCamera, PerspectiveCamera};
    use sample::{World, Object};
    use std::f64::consts::{FRAC_PI_4, PI};
    use failure::format_err;
    use palette::{Pixel, LinSrgb, named as colors};
    use indicatif::{ProgressBar, ProgressStyle};
    use sdl2::rect::Rect;
    use sdl2::event::Event;
    use sdl2::keyboard::Keycode;

    // create world object
    let world = World {
        objects: vec![
            Object::new(0., -2., 0., 3., LinSrgb::new(0., 0., 0.), 0.5),
            Object::new(0., 3., 0., 1.5, LinSrgb::new(0.8, 1., 0.8) * 0.9f32, 0.75),
            Object::new(4., 0., 0., 1., LinSrgb::new(1., 0.2, 0.2) * 0.75f32, 0.95),
            Object::new(-4., 0., 0., 1., LinSrgb::new(0.2, 0.2, 1.) * 0.75f32, 0.95),
            Object::new(0., 0., 4., 1., LinSrgb::new(0., 0., 0.), 0.05),
            Object::new(0., 0., -4., 1., LinSrgb::new(0., 0., 0.), 0.05),
        ],
        ambient: colors::DARKSLATEGREY.into_format::<f32>().into_linear() * 0.4,
        margin: 0.00001,
    };

    // create preview window
    let sdl = sdl2::init().map_err(|err| format_err!("Could not initialize SDL: {}", err))?;
    let mut events = sdl.event_pump().map_err(|err| format_err!("Could get SDL events: {}", err))?;
    let video = sdl.video().map_err(|err| format_err!("Could get SDL video: {}", err))?;
    let window = video.window("Sidequest Render Preview", FRAME_SIZE as u32, FRAME_SIZE as u32).build()?;
    let mut canvas = window.into_canvas().build()?;
    canvas.clear();
    canvas.present();
    let texture_create = canvas.texture_creator();
    let mut tile_texture = texture_create.create_texture_static(
        // we could store output as simple RGB, but OpenGL endianess stupidity
        // requires we use an alpha component, and pass it the backwards/wrong
        // texture format. Don't ask me why.
        Some(sdl2::pixels::PixelFormatEnum::ABGR8888),
        TILE_SIZE as u32,
        TILE_SIZE as u32,
    )?;

    // setup progress bar
    let tiles_per_axis = (FRAME_SIZE + TILE_SIZE - 1) / TILE_SIZE;
    let sty = ProgressStyle::default_bar().template("[{eta}] {wide_bar} {pos}/{len}");
    let tiles = ProgressBar::new((tiles_per_axis * tiles_per_axis * FRAME_NUM) as u64);
    tiles.set_style(sty);
    tiles.tick();

    // run render pipeline
    pipe::render_pipeline(
        // create frames to render
        move |index| {
            // check end of animation or close
            if index >= FRAME_NUM { return None }

            // create camera
            let angle = 2. * PI * (index as f64 / FRAME_NUM  as f64 + 0.125);
            let cam = PerspectiveCamera::new(
                Isometry3::new_observer_frame(
                    &Point3::new(angle.sin() * 12., 8., angle.cos() * 12.),
                    &Point3::new(0., 0., 0.),
                    &Vector3::new(0., 1., 0.),
                ),
                FRAC_PI_4,
                0.1,
                100.,
            );
            let cam = DefocusCamera::new(cam, 14.);

            // final world and camera data
            Some(pipe::FrameData {
                world: world.clone(),
                cam,
                params: sample::SampleParams {
                    samples: SAMPLE_NUM,
                    bounce_limit: BOUNCE_LIMIT,
                },
            })
        },
        // use rendered tiles
        |tile| {
            // TODO: save output

            // update progress bar
            tiles.inc(1);

            // calculate buffer information
            let width = tile.buf.width() as u32;
            let height = tile.buf.height() as u32;
            let texrect = Rect::new(0, 0, width, height);
            let canrect = Rect::new(tile.origin.0 as i32, tile.origin.1 as i32, width, height);
            let bytes = Pixel::into_raw_slice(&tile.buf.buf);

            // update texture
            if let Err(err) = tile_texture.update(Some(texrect), bytes, (width * 4) as usize) {
                eprintln!("Can't preview tile: {}", err);
                return
            }

            // copy texture to screen
            if let Err(err) = canvas.copy(
                &tile_texture,
                Some(texrect),
                Some(canrect),
            ) {
                eprintln!("Can't preview tile: {}", err);
                return
            }

            // display to user
            canvas.present();
        },
        // poll window events
        || {
            // check for close
            for event in events.poll_iter() {
            match event {
                    Event::Quit { .. } |
                    Event::KeyDown { keycode: Some(Keycode::Escape), .. } => return false,
                    _ => (),
                }
            }
            true
        },
        // render options
        pipe::RenderParams {
            width: FRAME_SIZE,
            height: FRAME_SIZE,
            tile_size: TILE_SIZE,
            tile_queue: THREADS * 2,
            threads: THREADS,
        },
    );

    // done!
    tiles.finish();
    Ok(())
}
