extern crate serde;
extern crate serde_json;
#[macro_use]
extern crate serde_derive;

extern crate num_traits;
extern crate nalgebra;

use std::io::prelude::*;
use std::fs::File;

mod setup;

pub fn main() {
	let mut f = File::open("example_scene.json").unwrap();
    let mut s = String::new();
    f.read_to_string(&mut s).unwrap();

    let setup: setup::Setup = serde_json::from_str(&s).unwrap();
    println!("{:?}", setup);

}