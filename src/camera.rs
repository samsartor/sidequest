pub use nalg::{Real, zero, one, Point2, Point3, Unit, Vector3, Matrix3, Vector2};
pub use imgref::ImgVec;
pub use nalg::geometry::{Isometry3, Perspective3};
use ncol::shape::Ball;
use ncol::query::{RayCast, Ray as NcolRay};

#[derive(Copy, Clone, Debug)]
pub struct Ray {
    pub origin: Point3<f64>,
    pub dir: Unit<Vector3<f64>>,
}

impl Ray {
    pub fn new(origin: Point3<f64>, dir: Unit<Vector3<f64>>) -> Ray {
        Ray { origin, dir }
    }

    pub fn col(self) -> NcolRay<Point3<f64>> {
        NcolRay::new(self.origin, self.dir.unwrap())
    }
}

#[derive(Copy, Clone, Debug)]
pub struct Impact<T> {
    pub t: f64,
    pub norm: Unit<Vector3<f64>>,
    pub data: T,
}

impl<T> Impact<T> {
    pub fn surface(&self) -> Matrix3<f64> {
        let x_axis = Vector3::new(1., 0., 0.);
        let y_axis = Vector3::new(0., 1., 0.);

        // TODO: Use proper tangent and bitangent

        let est = if self.norm.dot(&x_axis).abs() > 0.8 {
            // norm is too close to parallel with x axis, use different axis
            y_axis
        } else { x_axis };

        let tan = self.norm.cross(&est);
        let bitan = self.norm.cross(&tan);

        Matrix3::from_columns(&[tan, bitan, self.norm.unwrap()])
    }
}

pub trait Castable {
    fn cast<T>(&self, ray: Ray, data: T) -> Option<Impact<T>>;
}

#[derive(Clone, Debug)]
pub struct Sphere {
    pub center: Point3<f64>,
    ball: Ball<f64>,
}

impl Sphere {
    pub fn new(center: Point3<f64>, radius: f64) -> Sphere {
        Sphere {
            center,
            ball: Ball::new(radius),
        }
    }
}

impl Castable for Sphere {
    fn cast<T>(&self, ray: Ray, data: T) -> Option<Impact<T>> {
        use nalg::Translation;

        self.ball.toi_and_normal_with_ray(
            &Translation { vector: self.center.coords },
            &ray.col(),
            false,
        ).and_then(|r| {
            if r.toi == 0. { None }
            else { Some(Impact {
                t: r.toi,
                norm: Unit::new_unchecked(r.normal),
                data,
            }) }
        })
    }
}

pub trait Camera<I> {
    fn look(&self, from: I) -> Option<Ray>;
}

#[derive(Clone, Debug)]
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
    fn look(&self, from: Point2<f64>) -> Option<Ray> {
        let near = Point3::new(from.coords.x, from.coords.y, 0.);
        let near = self.projection.unproject_point(&near);
        let far = Point3::new(from.coords.x, from.coords.y, -1.);
        let far = self.projection.unproject_point(&far);
        let origin = self.position * near;
        let distant = self.position * far;
        let dir = Unit::new_normalize(distant - origin);
        Some(Ray::new(origin, dir))
    }
}

pub struct DefocusCamera {
    pub base: PerspectiveCamera,
    pub focal_distance: f64,
}

impl DefocusCamera {
    pub fn new(base: PerspectiveCamera, focal_distance: f64) -> DefocusCamera {
        DefocusCamera { base, focal_distance }
    }
}

impl Camera<(Point2<f64>, Vector2<f64>)> for DefocusCamera {
    fn look(&self, (from, offset): (Point2<f64>, Vector2<f64>)) -> Option<Ray> {
        let base = match self.base.look(from) {
            Some(b) => b,
            None => return None,
        };
        let offset = self.base.position * Vector3::new(offset[0], offset[1], 0.);
        let origin = base.origin + offset;
        let dir = base.dir.unwrap() - offset / self.focal_distance;
        Some(Ray::new(origin, Unit::new_normalize(dir)))
    }
}
