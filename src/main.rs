extern crate sidequest;
extern crate image;
extern crate failure;

use failure::Error;
use sidequest::core::*;
use std::f64::consts::FRAC_PI_4;

fn main() -> Result<(), Error> {
    let size = 512;
    let mut img = ImgVec::new(vec![[0, 0, 0]; size * size], size, size);

    {
        let imgref = RasterLayer::new(img.as_mut());
        let cam = PerspectiveCamera::new(
            Isometry3::new_observer_frame(
                &Point3::new(10., 10., 10.),
                &Point3::new(0., 0., 0.),
                &Vector3::new(0., 1., 0.),
            ),
            FRAC_PI_4,
            0.1,
            100.,
        );

        fn splat(x: f64) -> u8 {
            let x = x / 2. + 0.5;
            let x = x.min(1.).max(0.);
            (x * 255.) as u8
        }

        render_spheres(&[
            Sphere::new(Point3::new(0., -2., 0.), 3.),
            Sphere::new(Point3::new(0., 4., 0.), 2.),
            Sphere::new(Point3::new(4., 0., 0.), 1.),
            Sphere::new(Point3::new(-4., 0., 0.), 1.),
            Sphere::new(Point3::new(0., 0., 4.), 1.),
            Sphere::new(Point3::new(0., 0., -4.), 1.),
        ], &cam, imgref, |b| match b {
            Some(i) => [splat(i.norm[0]), splat(i.norm[1]), splat(i.norm[2])],
            None => [128, 128, 128],
        });
    }

    use std::mem::transmute;
    use std::slice::from_raw_parts;
    let buf: *const [u8; 3] = img.buf.as_ptr();

    image::save_buffer(
        "demo.png",
        unsafe { from_raw_parts(transmute(buf), img.buf.len() * 3) },
        img.width() as u32,
        img.height() as u32,
        image::ColorType::RGB(8))?;

    Ok(())
}
