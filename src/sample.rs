use camera::{Camera, Ray, Sphere, Castable, Impact};
use nalg::{Vector3, Vector2, Point2, Unit};
use rand::Rng;
use palette::{LinSrgb};
use stats::{ForPath, BackPath};

/// an object in the scene (always a sphere right now)
#[derive(Clone, Debug)]
pub struct Object {
    pub geo: Sphere,
    pub emission: LinSrgb,
    pub reflectivity: f32,
}

impl Object {
    pub fn new(x: f64, y: f64, z: f64, r: f64, emission: LinSrgb, reflectivity: f32) -> Object {
        use nalg::Point3;

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

/// a collection of objects and global world properties
#[derive(Clone, Debug)]
pub struct World {
    pub objects: Vec<Object>,
    pub ambient: LinSrgb,
    pub margin: f64,
}

/// calculate reflection vector
pub fn reflect(input: Unit<Vector3<f64>>, half: Unit<Vector3<f64>>) -> Unit<Vector3<f64>> {
    Unit::new_unchecked(input.unwrap() - 2. * (input.dot(half.as_ref())) * half.as_ref())
}

// sample a hemisphere, with cos(theta) weighting
pub fn cosine_weighted_hemi<R: Rng>(rng: &mut R) -> Unit<Vector3<f64>> {
    use std::f64::consts::PI;

    let u: f64 = rng.gen_range(0., 1.);
    let r = u.sqrt();
    let phi = rng.gen_range(0., 2. * PI);

    let x = r * phi.cos();
    let y = r * phi.sin();

    Unit::new_unchecked(Vector3::new(x, y, (1. - u).sqrt()))
}

impl World {
    /// extend light transport path through world
    pub fn sample<R: Rng, P: BackPath>(&self, ray: Ray, mut bpath: P, limit: usize, rng: &mut R) -> P::Forward
        where P::Forward: ForPath<Color=LinSrgb, Filter=LinSrgb>
    {
        // find place that light must have come from, if any
        let hit = self.objects.iter()
            // test light direction against all objects in scene (slow)
            .map(|o| o.geo.cast(ray, o))
            // include only surfaces that could have interacted, and that are at least `margin` units away (excludes current surface)
            .filter_map(|i| i.filter(|i| i.t > self.margin))
            // find closest surface (all other surfaces must be behind)
            .fold(None, |a, bi| match a {
                None => Some(bi),
                Some(ai) if ai.t > bi.t => Some(bi),
                _ => a,
            });

        match (hit, limit) {
            (None, _) => bpath.source(self.ambient), // light came from sky
            (Some(i), 0) => bpath.source(i.data.emission), // reached limit, assume light came from surface
            (Some(i), _) => { // light came from surface (maybe reflected?)
                let x;
                let filter;

                // assume light came from surface 10% of the time
                const EMISSION_P: f32 = 0.1;

                let mut fpath = if rng.gen_range(0., 1.) < EMISSION_P {
                    // assume light energy came from surface
                    bpath.decide(EMISSION_P);

                    filter = 1.;
                    bpath.source(i.data.emission)
                } else {
                    // assume light reflected off surface
                    bpath.decide_not(EMISSION_P);

                    let refl = i.data.reflectivity;
                    let not_refl = 1. - refl;
                    if refl > rng.gen_range(0., 1.) {
                        // assume specular reflection
                        bpath.decide(refl);

                        x = reflect(ray.dir, i.halfway());
                        filter = refl;
                    } else {
                        // assume diffuse reflection
                        bpath.decide_not(refl);

                        let cwh = cosine_weighted_hemi(rng);
                        x = Unit::new_unchecked(i.surface() * cwh.unwrap());
                        filter = not_refl;
                    }

                    // extend transport path again
                    self.sample(
                        Ray::new(ray.origin + i.t * ray.dir.unwrap(), x),
                        bpath,
                        limit - 1,
                        rng,
                    )
                };

                fpath.filter(LinSrgb::new(filter, filter, filter));
                fpath
            }
        }
    }
}

#[derive(Copy, Clone, Debug)]
pub struct SampleParams {
    /// number of samples per pixel
    pub samples: usize,
    /// maximum number of bounces
    pub bounce_limit: usize,
}

/// Get value of a single pixel
pub fn sample_pixel<R: Rng, C: Camera<(Point2<f64>, Vector2<f64>)>>(
    cam: &C, // camera ray calculator
    world: &World, // world object
    point: Point2<f64>, // upper-left corner of pixel on film
    pixel_width: f64, // width of a single pixel on film
    rng: &mut R, // random number generator
    params: &SampleParams, // parameters for pixel sampling
) -> LinSrgb {
    use stats::MulBackPath;
    use std::f64::consts::PI;

    // initialize the sum
    let mut val = LinSrgb::new(0., 0., 0.);

    // sample many times
    for _ in 0..params.samples {
        // light doesn't strike the exact corner of the pixel
        // offset by random amount (cartesian since pixel is square)
        let offset = Vector2::new(rng.gen_range(0., pixel_width), rng.gen_range(0., pixel_width));

        // light doesn't pass through the exact center of aperture
        // offset by random amount (polar since aperture is round)
        let defoc_r = 0.2 * rng.gen_range(0f64, 1f64).sqrt();
        let defoc_theta = rng.gen_range(0., 2. * PI);
        let defoc = Vector2::new(defoc_r * defoc_theta.sin(), defoc_r * defoc_theta.cos());

        // create ray to trace
        let ray = match cam.look((point + offset, defoc)) {
            Some(r) => r,
            None => continue,
        };

        // get transport path of light through world
        let path = world.sample(ray, MulBackPath::new(), params.bounce_limit, rng);
        val = val + path.lum(); // add luminance to sum
    }

    // each path additionally has a 1/n probability of occurring relative to other paths
    val / params.samples as f32 // return sum
}
