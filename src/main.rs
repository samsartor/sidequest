extern crate sidequest;
extern crate gif;
extern crate failure;
extern crate rayon;

use failure::Error;
use gif::{SetParameter, Encoder, Frame, Repeat};
use sidequest::core::*;
use self::palette::{Pixel, LinSrgb, Srgb, Xyz, named as colors};
use rayon::prelude::*;
use std::f64::consts::{FRAC_PI_4, PI};
use std::fs::File;

pub fn render_spheres<C, P: Send, PF>(
    spheres: &[Sphere<f64>],
    camera: &C,
    mut raster: RasterLayer<P>,
    pixel: PF,
)
    where
        C: Camera<Point2<f64>, F=f64> + Sync,
        PF: Fn(Option<Impact<f64>>) -> P + Sync,
{

    for (p, v) in raster.pixels_mut() {
        let ray = match camera.look(p) {
            Some(r) => r,
            None => continue,
        };
        let hit = spheres.iter()
            .map(|s| s.cast(ray))
            .fold(None, |a, b| match (a, b) {
                (None, _) => b,
                (_, None) => a,
                (Some(ai), Some(bi)) if ai.t > bi.t => b,
                _ => a,
            });
        *v = pixel(hit);
    }
}

fn smash(norm: f64) -> f32 {
    (norm as f32 + 1.) / 2.
}

fn main() -> Result<(), Error> {
    let size = 512;

    let scene = &[
        Sphere::new(Point3::new(0., -2., 0.), 3.),
        Sphere::new(Point3::new(0., 3., 0.), 1.5),
        Sphere::new(Point3::new(4., 0., 0.), 1.),
        Sphere::new(Point3::new(-4., 0., 0.), 1.),
        Sphere::new(Point3::new(0., 0., 4.), 1.),
        Sphere::new(Point3::new(0., 0., -4.), 1.),
    ];

    println!("RENDERING & ENCODING");


    let red: Xyz = LinSrgb::new(0.95, 0.05, 0.05).into();
    let green: Xyz = LinSrgb::new(0.05, 0.95, 0.05).into();
    let blue: Xyz = LinSrgb::new(0.05, 0.05, 0.95).into();

    const FRAMENUM: usize = 60;

    let mut frames = Vec::with_capacity(FRAMENUM );
    (0..FRAMENUM ).into_par_iter().map(|i| {
        let mut img = ImgVec::new(vec![Srgb::new(0, 0, 0); size * size], size, size);
        let angle = 2. * PI * (i as f64 / FRAMENUM  as f64);
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

        render_spheres(scene, &cam, RasterLayer::new(img.as_mut()), |b| match b {
            Some(i) => {
                let shade = red * smash(i.norm[0]) + green * smash(i.norm[1]) + blue * smash(i.norm[2]);
                let srgb: Srgb = shade.into();
                srgb.into_format()
            },
            None => colors::GRAY,
        });

        let mut frame = Frame::from_rgb(
            img.width() as u16,
            img.height() as u16,
            Pixel::into_raw_slice(&img.buf),
        );
        frame.delay = 100 / 30;

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
