pub mod serdenal;

use std::ops::{Deref, DerefMut};
use std::collections::HashMap;
use std::fmt::{self, Debug};

use serde::de::Deserialize;
use serde::ser::Serialize;

use nalgebra::{Point3, Vector3};

pub struct Sd<V> {
    v: V
}

impl<V> Sd<V> {
    pub fn new(v: V) -> Sd<V> {
        Sd {
            v: v
        }
    }
}

impl<V> Deref for Sd<V> {
    type Target = V;

    fn deref(&self) -> &Self::Target  {
        &self.v
    }
}

impl<V> DerefMut for Sd<V> {
    fn deref_mut(&mut self) -> &mut Self::Target  {
        &mut self.v
    }
}

impl<V: Debug> Debug for Sd<V> {
	fn fmt(&self, fmt: &mut fmt::Formatter) -> Result<(), fmt::Error> {
		self.deref().fmt(fmt)
	}
}

#[derive(Deserialize, Debug)]
pub struct Setup {
	pub geo: HashMap<String, Mesh>,
	pub mats: HashMap<String, Material>,
	pub draw: Vec<Draw>,
	pub sky: Sd<Vector3<f32>>,
}

#[derive(Deserialize, Debug)]
pub struct Mesh {
	pub verts: Vec<Sd<Point3<f32>>>,
	pub tris: Vec<(usize, usize, usize)>,
}

#[derive(Deserialize, Debug)]
pub struct Material {
	pub diffuse: Sd<Vector3<f32>>,
	pub specular: Sd<Vector3<f32>>,
	pub reflectivity: f32,
	pub iqr: Option<f32>,
	pub emission: Option<Sd<Vector3<f32>>>,
}

#[derive(Deserialize, Debug)]
pub enum Draw {
	Mesh { geo: String, mat: String },
	Plane{ on: Sd<Point3<f32>>, norm: Sd<Vector3<f32>>, mat: String },
	Sphere{ center: Sd<Point3<f32>>, radius: f32, mat: String },
}