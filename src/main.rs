extern crate sidequest;
extern crate failure;
extern crate rayon;
extern crate image;

use failure::Error;
use sidequest::core::*;
use self::shade::*;
use self::stats::{MulBackPath, ForPath};
use self::palette::{Pixel, LinSrgb, Srgb, named as colors};
use rayon::prelude::*;
use std::f64::consts::{FRAC_PI_4, PI};
use self::rand::Rng;
use self::nalg::Vector2;


fn main() -> Result<(), Error> {
    const FRAME_SIZE: usize = 512;
    const FRAME_NUM: usize = 60;
    const SAMPLE_NUM: usize = 256;
    const BOUNCE_LIMIT: usize = 6;

    let world = World {
        objects: vec![
            Object::new(0., -2., 0., 3., LinSrgb::new(0., 0., 0.), 0.75),
            Object::new(0., 3., 0., 1.5, LinSrgb::new(0.05, 0.3, 0.05), 0.75),
            Object::new(4., 0., 0., 1., LinSrgb::new(0.3, 0.05, 0.05), 0.9),
            Object::new(-4., 0., 0., 1., LinSrgb::new(0.05, 0.05, 0.3), 0.9),
            Object::new(0., 0., 4., 1., LinSrgb::new(0., 0., 0.), 0.1),
            Object::new(0., 0., -4., 1., LinSrgb::new(0., 0., 0.), 0.1),
        ],
        ambient: colors::DARKSLATEGREY.into_format::<f32>().into_linear(),
        margin: 0.00001,
    };

    println!("RENDERING & ENCODING");

    let mut frames = Vec::with_capacity(FRAME_NUM );
    (0..FRAME_NUM).into_par_iter().map(|index| {
        let mut rng = rand::thread_rng();

        let mut img = ImgVec::new(vec![Srgb::new(0, 0, 0); FRAME_SIZE * FRAME_SIZE], FRAME_SIZE, FRAME_SIZE);
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
            for (p, v) in raster.pixels_mut() {
                let mut val = LinSrgb::new(0., 0., 0.);
                for _ in 0..SAMPLE_NUM {
                    let offset = Vector2::new(rng.gen_range(0., px), rng.gen_range(0., px));

                    let defoc_r = 0.2 * rng.gen_range(0., 1.).sqrt();
                    let defoc_theta = rng.gen_range(0., 2. * PI);
                    let defoc = Vector2::new(defoc_r * defoc_theta.sin(), defoc_r * defoc_theta.cos());

                    let ray = match cam.look((p + offset, defoc)) {
                        Some(r) => r,
                        None => continue,
                    };

                    let path = world.sample(ray, MulBackPath::new(), BOUNCE_LIMIT, &mut rng);
                    val = val + path.lum();
                }
                *v = Srgb::from_linear(val / SAMPLE_NUM as f32).into_format();
            }
        }

        if let Err(e) = image::save_buffer(
            format!("demo/frame{:03}.png", index),
            Pixel::into_raw_slice(&img.buf),
            img.width() as u32,
            img.height() as u32,
            image::ColorType::RGB(8))
        {
            eprintln!("\tERROR SAVING FRAME #{}: {:?}", index, e);
        } else {
            println!("\tRENDERED FRAME #{}", index);
        }
    }).collect_into_vec(&mut frames);

    Ok(())
}
