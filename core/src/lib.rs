pub extern crate nalgebra as nalg;
pub extern crate ncollide as ncol;
pub extern crate imgref;
extern crate num_traits;
extern crate serde;
#[macro_use]
extern crate serde_derive;

pub use nalg::{Real, zero, one, Point2, Point3, Unit, Vector3};
pub use imgref::ImgVec;
pub use nalg::geometry::{Isometry3, Perspective3};
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
        let far = Point3::new(from.coords.x, from.coords.y, -1.);
        let far = self.projection.unproject_point(&far);
        let origin = self.position * near;
        let distant = self.position * far;
        let dir = Unit::new_normalize(distant - origin);
        Some(Ray::new(origin, dir.unwrap()))
    }
}

pub struct RasterLayer<'a, P: 'a> {
    pub target: ImgRefMut<'a, P>,
}

impl<'a, P> RasterLayer<'a, P> {
    pub fn new(target: ImgRefMut<'a, P>) -> RasterLayer<'a, P> {
        RasterLayer { target }
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
        self.pixel_size() / 2.
    }

    /// Mutate over pixels in view space.
    pub fn pixels_mut<'s>(&'s mut self)
        -> impl Iterator<Item=(Point2<f64>, &'s mut P)> + 's
    {
        let c = self.centers();
        self.target.rows_mut()
            .enumerate()
            .flat_map(move |(y, r)| r.iter_mut()
            .enumerate()
            .map(move |(x, p)| (c(x, y), p)))
    }

    /// Returns a function that can compute the center of any pixel in view space.
    pub fn centers(&self) -> impl Fn(usize, usize) -> Point2<f64> + Copy {
        let s = self.pixel_size();
        move |x, y| Point2::new(
            (x as f64 + 0.5) * s - 1.,
            (y as f64 + 0.5) * s - 1.,
        )
    }
}

pub fn render_spheres<C, P, PF>(
    spheres: &[Sphere<f64>],
    camera: &C,
    mut raster: RasterLayer<P>,
    pixel: PF,
)
    where C: Camera<Point2<f64>, F=f64>, PF: Fn(Option<Impact<f64>>) -> P
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
