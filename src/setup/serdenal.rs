use serde::de::{self, Deserialize, Deserializer};
#[allow(unused_imports)]
use serde::ser::{self, Serialize, Serializer, SerializeSeq};
use std::vec::IntoIter;
use std::iter::FromIterator;
use nalgebra;
use num_traits::identities::Zero;
use std::ops::{Deref, DerefMut};

enum SizeMode {
    Any,
    Exact(usize),
    Max(usize),
}

use self::SizeMode::*;
impl SizeMode {
    pub fn allowed(&self, size: usize) -> bool {
        match *self {
            Any => true,
            Exact(v) => size == v,
            Max(v) => size <= v,
        }
    }
}

struct VectorVisitor<N, C> {
    size: SizeMode,
    from: fn(IntoIter<N>) -> C,
}

impl<N, C> VectorVisitor<N, C> {
    fn new(size: SizeMode, from: fn(IntoIter<N>) -> C) -> Self { VectorVisitor {
        size: size,
        from: from,
    }}
}

impl<N, C> de::Visitor for VectorVisitor<N, C>
    where N: Deserialize
{
    type Value = Sd<C>;

    fn visit_seq<S: de::SeqVisitor>(self, mut visitor: S) -> Result<Self::Value, S::Error>
    {
        let mut values = Vec::with_capacity(visitor.size_hint().0);

        loop { match visitor.visit() {
            Ok(Some(val)) => values.push(val),
            Ok(None) => break,
            Err(err) => return Err(err),
        }}

        if !self.size.allowed(values.len()) {
            return Err(de::Error::invalid_length(values.len()));
        }

        Ok(Sd::new((self.from)(values.into_iter())))
    }
}

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

macro_rules! for_vector {
    ( $($i:tt)::+, $size:expr ) => {
        impl<N> Deserialize for Sd<$($i)::+<N>>
            where N: Deserialize + Zero
        {
            fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
                where D: Deserializer
            {
                deserializer.deserialize_seq(VectorVisitor::new($size, $($i)::+::from_iter))
            }
        }

        /*
        impl<N: Serialize> Serialize for Sd<$($i)::+<N>>
        {
            fn serialize<S: Serializer>(&self, s: S) -> Result<S::Ok, S::Error> 
            {
                let seq = s.serialize_seq(Some(self.len()))?;
                for i in self.as_ref() {
                    seq.serialize_element(i)?;
                }
                seq.end()?;
                S::Ok()
            }
        }
        */
    };
}

for_vector!(nalgebra::Vector1, Exact(1));
for_vector!(nalgebra::Vector2, Exact(2));
for_vector!(nalgebra::Vector3, Exact(3));
for_vector!(nalgebra::Vector4, Exact(4));
for_vector!(nalgebra::Vector5, Exact(5));
for_vector!(nalgebra::Vector6, Exact(6));
for_vector!(nalgebra::Point1, Exact(1));
for_vector!(nalgebra::Point2, Exact(2));
for_vector!(nalgebra::Point3, Exact(3));
for_vector!(nalgebra::Point4, Exact(4));
for_vector!(nalgebra::Point5, Exact(5));
for_vector!(nalgebra::Point6, Exact(6));
for_vector!(nalgebra::DVector1, Max(1));
for_vector!(nalgebra::DVector2, Max(2));
for_vector!(nalgebra::DVector3, Max(3));
for_vector!(nalgebra::DVector4, Max(4));
for_vector!(nalgebra::DVector5, Max(5));
for_vector!(nalgebra::DVector6, Max(6));
for_vector!(nalgebra::DVector, Any);
for_vector!(nalgebra::Quaternion, Exact(4));