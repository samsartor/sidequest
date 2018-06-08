pub extern crate nalgebra as nalg;
pub extern crate ncollide as ncol;
pub extern crate imgref;
extern crate num_traits;
extern crate serde;
#[macro_use]
extern crate serde_derive;

pub use nalg::{Real, zero, one, Point2, Point3, Unit, Vector3};
use nalg::geometry::{Isometry3, Perspective3};
use ncol::shape::Ball;
use ncol::query::{RayCast, Ray as NcolRay};
use imgref::ImgRefMut;

pub type Ray<F> = NcolRay<Point3<F>>;

#[derive(Copy, Clone, Debug, Serialize, Deserialize)]
pub struct Impact<F: Real> {
    pub t: F,
    pub norm: Unit<Vector3<F>>,
}

pub trait Castable<F: Real> {
    fn cast(&self, ray: Ray<F>) -> Option<Impact<F>>;
}

#[derive(Clone, Debug)]
pub struct Sphere<F: Real> {
    pub center: Point3<F>,
    ball: Ball<F>,
}

impl<F: Real> Sphere<F> {
    pub fn new(center: Point3<F>, radius: F) -> Sphere<F> {
        Sphere {
            center,
            ball: Ball::new(radius),
        }
    }
}

impl<F: Real> Castable<F> for Sphere<F> {
    fn cast(&self, ray: Ray<F>) -> Option<Impact<F>> {
        use nalg::Translation;

        self.ball.toi_and_normal_with_ray(
            &Translation { vector: self.center.coords },
            &ray,
            false,
        ).and_then(|r| {
            if r.toi == F::zero() { None }
            else { Some(Impact {
                t: r.toi,
                norm: Unit::new_unchecked(r.normal)
            }) }
        })
    }
}

pub trait Camera<I> {
    type F: Real;

    fn look(&self, from: I) -> Option<Ray<Self::F>>;
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct PerspectiveCamera {
    pub position: Isometry3<f64>,
    pub projection: Perspective3<f64>,
}

impl PerspectiveCamera {
    pub fn new(position: Isometry3<f64>, fov: f64, near: f64, far: f64) -> PerspectiveCamera {
        PerspectiveCamera { position, projection: Perspective3::new(1., fov, near, far) }
    }
}

impl Camera<Point2<f64>> for PerspectiveCamera {
    type F = f64;

    fn look(&self, from: Point2<f64>) -> Option<Ray<f64>> {
        let near = Point3::new(from.coords.x, from.coords.y, 0.);
        let near = self.projection.unproject_point(&near);
        let far = Point3::new(from.coords.x, from.coords.y, 1.);
        let far = self.projection.unproject_point(&far);
        let origin = self.position * near;
        let distant = self.position * far;
        let dir = (distant - origin).normalize();
        Some(Ray::new(origin, dir))
    }
}

pub struct RasterLayer<'a, C: Camera<Point2<f64>>, P: 'a> {
    pub target: ImgRefMut<'a, P>,
    pub camera: C,
}

impl<'a, C: Camera<Point2<f64>>, P> RasterLayer<'a, C, P> {
    pub fn new(target: ImgRefMut<'a, P>, camera: C) -> RasterLayer<'a, C, P> {
        RasterLayer { target, camera }
    }

    /// Maximum of width and height.
    pub fn dim(&self) -> usize {
        self.target.width().max(self.target.height())
    }

    /// The width/height of a single pixel in view space.
    pub fn pixel_size(&self) -> f64 {
        2. / self.dim() as f64
    }

    /// Half the width/height of a single pixel in view space.
    pub fn pixel_radius(&self) -> f64 {
        1. / self.dim() as f64
    }

    /// Iterate over pixel coordinates in screen space.
    pub fn pixels(&self) -> impl Iterator<Item=(usize, usize)> + '_ {
        let w = self.target.width();
        let h = self.target.height();
        (0..w).flat_map(move |x| (0..h).map(move |y| (x, y)))
    }

    /// Get the center of a pixel in view space.
    pub fn center(&self, x: usize, y: usize) -> Point2<f64> {
        let s = self.pixel_size();
        Point2::new(
            (x as f64 + 0.5) * s - 1.,
            (y as f64 + 0.5) * s - 1.,
        )
    }
}
