extern crate sidequest;
extern crate gif;
extern crate failure;
extern crate rayon;
extern crate image;

use failure::Error;
use gif::{SetParameter, Encoder, Frame, Repeat};
use sidequest::core::*;
use self::shade::*;
use self::palette::{Pixel, LinSrgb, Srgb, named as colors};
use rayon::prelude::*;
use std::f64::consts::{FRAC_PI_4, PI};
use std::fs::File;

fn main() -> Result<(), Error> {
    let size = 1024;

    let world = World {
        objects: vec![
            Object::new(0., -2., 0., 3., LinSrgb::new(0.05, 0.3, 0.05), 0.75),
            Object::new(0., 3., 0., 1.5, LinSrgb::new(0., 0., 0.), 0.75),
            Object::new(4., 0., 0., 1., LinSrgb::new(0.3, 0.05, 0.05), 0.9),
            Object::new(-4., 0., 0., 1., LinSrgb::new(0.05, 0.05, 0.3), 0.9),
            Object::new(0., 0., 4., 1., LinSrgb::new(0., 0., 0.), 0.1),
            Object::new(0., 0., -4., 1., LinSrgb::new(0., 0., 0.), 0.1),
        ],
        ambient: colors::DARKSLATEGREY.into_format::<f32>().into_linear(),
        margin: 0.00001,
    };

    println!("RENDERING & ENCODING");

    const FRAMENUM: usize = 60;

    let mut frames = Vec::with_capacity(FRAMENUM );
    (0..FRAMENUM).into_par_iter().map(|index| {
        let mut rng = rand::thread_rng();

        let mut img = ImgVec::new(vec![Srgb::new(0, 0, 0); size * size], size, size);
        let angle = 2. * PI * (index as f64 / FRAMENUM  as f64);
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

        {
            let mut raster = RasterLayer::new(img.as_mut());
            for (p, v) in raster.pixels_mut() {
                let mut val = LinSrgb::new(0., 0., 0.);
                let count = 128;
                for _ in 0..count {
                    let ray = match cam.look(p) {
                        Some(r) => r,
                        None => continue,
                    };
                    let (mut f, p) = world.sample(ray, 6, &mut rng);
                    val = val + f / p as f32;
                }
                *v = Srgb::from_linear(val / count as f32).into_format();
            }
        }

        if index == 0 {
            if let Err(e) = image::save_buffer(
                "demo.png",
                Pixel::into_raw_slice(&img.buf),
                img.width() as u32,
                img.height() as u32,
                image::ColorType::RGB(8))
            {
                eprintln!("Could not save PNG: {:?}", e);
            }
        }

        let mut frame = Frame::from_rgb(
            img.width() as u16,
            img.height() as u16,
            Pixel::into_raw_slice(&img.buf),
        );
        frame.delay = 100 / 30;

        println!("\tRENDERED FRAME #{}", index);

        frame
    }).collect_into_vec(&mut frames);

    println!("WRITING");

    let mut gif = Encoder::new(File::create("demo.gif")?, size as u16, size as u16, &[])?;
    gif.set(Repeat::Infinite)?;
    for frame in frames {
        gif.write_frame(&frame)?;
    }

    Ok(())
}
