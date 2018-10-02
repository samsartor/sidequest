extern crate failure;
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
#[macro_use]
extern crate structopt;
extern crate num_cpus;

pub mod sample;
pub mod camera;
pub mod stats;
pub mod pipe;

use failure::Error;

#[derive(StructOpt, Debug)]
#[structopt(name="sidequest")]
struct Params {
    #[structopt(short="t", long="threads")]
    threads: Option<usize>,
    #[structopt(short="w", long="width", default_value="512")]
    width: u32,
    #[structopt(short="h", long="height", default_value="512")]
    height: u32,
    #[structopt(short="s", long="samples", default_value="1000")]
    samples: usize,
    #[structopt(short="f", long="frames", default_value="30")]
    frames: u32,
    #[structopt(long="tilesize", default_value="64")]
    tile_size: u32,
    #[structopt(long="bounces", default_value="12")]
    bounce_limit: usize,
}

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
    use structopt::StructOpt;

    // parse args
    let params = Params::from_args();
    let threads = params.threads.unwrap_or(num_cpus::get());
    let frame_num = params.frames;
    let sample_params = sample::SampleParams {
        samples: params.samples,
        bounce_limit: params.bounce_limit,
    };
    let render_params = pipe::RenderParams {
        width: params.width as usize,
        height: params.height as usize,
        tile_size: params.tile_size as usize,
        tile_queue: threads * 2,
        threads: threads,
    };

    // create preview window
    let sdl = sdl2::init().map_err(|err| format_err!("Could not initialize SDL: {}", err))?;
    let mut events = sdl.event_pump().map_err(|err| format_err!("Could get SDL events: {}", err))?;
    let video = sdl.video().map_err(|err| format_err!("Could get SDL video: {}", err))?;
    let window = video.window("Sidequest Render Preview", params.width, params.height).build()?;
    let mut canvas = window.into_canvas().build()?;
    canvas.clear();
    canvas.present();
    let texture_create = canvas.texture_creator();
    let mut tile_texture = texture_create.create_texture_static(
        // we could store output as simple RGB, but OpenGL endianess stupidity
        // requires we use an alpha component, and pass it the backwards/wrong
        // texture format. Don't ask me why.
        Some(sdl2::pixels::PixelFormatEnum::ABGR8888),
        params.tile_size,
        params.tile_size,
    )?;

    // setup progress bar
    let tile_num_x = (params.width + params.tile_size - 1) / params.tile_size;
    let tile_num_y = (params.height + params.tile_size - 1) / params.tile_size;
    let sty = ProgressStyle::default_bar().template("[{eta}] {wide_bar} {pos}/{len}");
    let tiles = ProgressBar::new((tile_num_x * tile_num_y * params.frames) as u64);
    tiles.set_style(sty);
    tiles.tick();

    // run render pipeline
    pipe::render_pipeline(
        // create frames to render
        move |index| {
            // check end of animation or close
            if index >= frame_num { return None }

            // create camera
            let angle = 2. * PI * (index as f64 / frame_num  as f64);
            let cam = PerspectiveCamera::new(
                Isometry3::new_observer_frame(
                    &Point3::new(12., 8., 12.),
                    &Point3::new(0., 0., 0.),
                    &Vector3::new(0., 1., 0.),
                ),
                FRAC_PI_4,
                0.1,
                100.,
            );
            let cam = DefocusCamera::new(cam, 14.);

            // create world
            let world = World {
                objects: vec![
                    //Format: (x, y, z, radius, emmisivity(r,g,b), reflectivity)
                    Object::new(0., -2., 0., 3., LinSrgb::new(0.894, 0.345, 0.925) * 0.25f32, 0.5),
                    Object::new(0., 3., 0., 1.5, LinSrgb::new(0.8, 1., 0.8) * 0.9f32, 0.75),
                    Object::new(4., -2.25 * angle.sin(), 0., 1., LinSrgb::new(1.0, 0.2, 0.2) * 0.75f32 * (((angle.cos() + 1.) as f32) / 2f32), 0.95),
                    Object::new(-4., 2.25 * angle.sin(), 0., 1., LinSrgb::new(0.2, 0.2, 1.) * 0.75f32 * (((angle.sin() + 1.) as f32) / 2f32), 0.95),
                    Object::new(4. * angle.sin(), 0., 4. * angle.cos(), 1., LinSrgb::new(0., 0., 0.), 0.95),
                    Object::new(-4. * angle.sin(), 0., -4. * angle.cos(), 1., LinSrgb::new(0., 0., 0.), 0.05),
                ],
                ambient: colors::DARKSLATEGREY.into_format::<f32>().into_linear() * 0.4,
                margin: 0.00001,
            };


            // final world and camera data
            Some(pipe::FrameData {
                world,
                cam,
                params: sample_params,
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
        render_params,
    );

    // done!
    tiles.finish();
    Ok(())
}
