use ::{Ray, Sphere, Castable, Vector3, Unit, Impact};
use rand::Rng;
use palette::{LinSrgb};

pub struct Object {
    pub geo: Sphere,
    pub emission: LinSrgb,
}

impl Object {
    pub fn new(x: f64, y: f64, z: f64, r: f64, emission: LinSrgb) -> Object {
        use ::Point3;

        Object {
            geo: Sphere::new(Point3::new(x, y, z), r),
            emission,
        }
    }
}

impl<'o> Impact<&'o Object> {
    pub fn halfway(&self) -> Unit<Vector3<f64>>  {
        self.norm
    }
}

pub struct World {
    pub objects: Vec<Object>,
    pub ambient: LinSrgb,
    pub margin: f64,
}

pub fn reflect(input: Unit<Vector3<f64>>, half: Unit<Vector3<f64>>) -> Unit<Vector3<f64>> {
    Unit::new_unchecked(input.unwrap() - 2. * (input.dot(half.as_ref())) * half.as_ref())
}

impl World {
    pub fn sample(&self, ray: Ray, limit: usize) -> (LinSrgb, f64) {
        let hit = self.objects.iter()
            .map(|o| o.geo.cast(ray, o))
            .filter_map(|i| i.filter(|i| i.t > self.margin))
            .fold(None, |a, bi| match a {
                None => Some(bi),
                Some(ai) if ai.t > bi.t => Some(bi),
                _ => a,
            });
        match (hit, limit) {
            (None, _) => (self.ambient, 1.),
            (Some(i), 0) => (i.data.emission, 1.),
            (Some(i), _) => {
                let x = reflect(ray.dir, i.halfway());
                let p_x = 1.;

                let (prior_f, prior_p) = self.sample(Ray::new(ray.origin + i.t * ray.dir.unwrap(), x), limit - 1);

                let f = prior_f + i.data.emission;
                let p = p_x * prior_p;
                (f, p)
            }
        }
    }
}
