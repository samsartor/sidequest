use ::{Ray, Sphere, Castable, Vector3, Unit, Impact};
use rand::Rng;
use palette::{LinSrgb};

pub struct Object {
    pub geo: Sphere,
    pub emission: LinSrgb,
    pub reflectivity: f32,
}

impl Object {
    pub fn new(x: f64, y: f64, z: f64, r: f64, emission: LinSrgb, reflectivity: f32) -> Object {
        use ::Point3;

        Object {
            geo: Sphere::new(Point3::new(x, y, z), r),
            emission,
            reflectivity,
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

pub fn cosine_weighted_hemi<R: Rng>(rng: &mut R) -> (f64, Unit<Vector3<f64>>) {
    use std::f64::consts::PI;

    let u: f64 = rng.gen_range(0., 1.);
    let r = u.sqrt();
    let phi = rng.gen_range(0., 2. * PI);

    let x = r * phi.cos();
    let y = r * phi.sin();

    (r, Unit::new_unchecked(Vector3::new(x, y, (1. - u).sqrt())))
}

impl World {
    pub fn sample<R: Rng>(&self, ray: Ray, limit: usize, rng: &mut R) -> (LinSrgb, f32) {
        use std::f32::consts::PI;

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
                let x;
                let p_x;
                let f_prob;

                let refl = i.data.reflectivity;
                let not_refl = 1. - refl;
                if refl > rng.gen_range(0., 1.) {
                    x = reflect(ray.dir, i.halfway());
                    p_x = refl;
                    f_prob = refl;
                } else {
                    let (cos_theta, cwh) = cosine_weighted_hemi(rng);
                    x = Unit::new_unchecked(i.surface() * cwh.unwrap());
                    let cos_theta_over_pi = cos_theta as f32 / PI;
                    p_x = not_refl * cos_theta_over_pi;
                    f_prob = not_refl * cos_theta_over_pi;
                }

                let (prior_f, prior_p) = self.sample(
                    Ray::new(ray.origin + i.t * ray.dir.unwrap(), x),
                    limit - 1,
                    rng,
                );

                let f = (prior_f + i.data.emission) * f_prob;
                let p = p_x * prior_p;
                (f, p)
            }
        }
    }
}
