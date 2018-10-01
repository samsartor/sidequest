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

pub mod shade;
pub mod geo;
pub mod stats;

use failure::Error;
use self::geo::*;
use self::shade::*;
use self::stats::{MulBackPath, ForPath};
use self::palette::{Pixel, LinSrgb, Srgb, named as colors};
use rayon::prelude::*;
use std::f64::consts::{FRAC_PI_4, PI};
use self::rand::Rng;
use self::nalg::Vector2;
use indicatif::{ProgressBar, ProgressStyle};

const FRAME_SIZE: usize = 512;
const FRAME_NUM: usize = 30;
const SAMPLE_NUM: usize = 1024;
const BOUNCE_LIMIT: usize = 4;
const PIXELS_PER_THREAD: usize = 4192;

fn sample_pixel<R: Rng, C: Camera<(Point2<f64>, Vector2<f64>)>>(
    cam: &C,
    world: &World,
    point: Point2<f64>,
    pixel_width: f64,
    rng: &mut R,
    samples: usize,
) -> LinSrgb {
    let mut val = LinSrgb::new(0., 0., 0.);
    for _ in 0..samples {
        let offset = Vector2::new(rng.gen_range(0., pixel_width), rng.gen_range(0., pixel_width));

        let defoc_r = 0.2 * rng.gen_range(0., 1.).sqrt();
        let defoc_theta = rng.gen_range(0., 2. * PI);
        let defoc = Vector2::new(defoc_r * defoc_theta.sin(), defoc_r * defoc_theta.cos());

        let ray = match cam.look((point + offset, defoc)) {
            Some(r) => r,
            None => continue,
        };

        let path = world.sample(ray, MulBackPath::new(), BOUNCE_LIMIT, rng);
        val = val + path.lum();
    }
    val / samples as f32
}

fn main() -> Result<(), Error> {
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

    let sty = ProgressStyle::default_bar().template("[{elapsed_precise}] {wide_bar} PIXEL {pos}/{len} - {msg}");
    let pixels = ProgressBar::new(1);
    pixels.set_style(sty);

    for index in 0..FRAME_NUM {
        pixels.set_message(&format!("FRAME {}/{}", index, FRAME_NUM));

        let mut img = ImgVec::new(vec![Srgb::new(0, 0, 0); FRAME_SIZE * FRAME_SIZE], FRAME_SIZE, FRAME_SIZE);
        pixels.set_position(0);
        pixels.set_length((img.width() * img.height()) as u64);

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

        {
            let mut raster = RasterLayer::new(img.as_mut());
            let px = raster.pixel_size();
            let mut px_buf: Vec<_> = raster.pixels_mut().collect(); // TODO: This is dumb

            px_buf.par_chunks_mut(PIXELS_PER_THREAD).for_each(|buf| {
                let mut rng = rand::thread_rng();
                let inc = buf.len() as u64;
                for &mut (p, ref mut v) in buf {
                    **v = Srgb::from_linear(sample_pixel(
                        &cam,
                        &world,
                        p,
                        px,
                        &mut rng,
                        SAMPLE_NUM
                    )).into_format();
                }
                pixels.inc(inc);
            });
        }

        image::save_buffer(
            format!("demo/frame{:03}.png", index),
            Pixel::into_raw_slice(&img.buf),
            img.width() as u32,
            img.height() as u32,
            image::ColorType::RGB(8),
        )?;
    }

    pixels.finish();

    Ok(())
}
