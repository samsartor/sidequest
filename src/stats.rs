use palette::LinSrgb;

pub trait BackPath {
    type Forward: ForPath;

    fn decide(&mut self, prob: f32);
    fn decide_not(&mut self, prob: f32) { self.decide(1. - prob) }
    fn source(self, color: <Self::Forward as ForPath>::Color) -> Self::Forward;
}

pub trait ForPath {
    type Color;
    type Filter;

    fn filter(&mut self, filter: Self::Filter);
    fn lum(&self) -> Self::Color;
}

pub struct MulBackPath {
    prob: f32,
}

impl MulBackPath {
    pub fn new() -> MulBackPath {
        MulBackPath { prob: 1. }
    }
}

impl BackPath for MulBackPath {
    type Forward = MulForPath;

    fn decide(&mut self, prob: f32) {
        self.prob *= prob;
    }

    fn source(self, color: LinSrgb) -> MulForPath {
        MulForPath {
            lum: color / self.prob,
        }
    }
}

pub struct MulForPath {
    lum: LinSrgb,
}

impl ForPath for MulForPath {
    type Color = LinSrgb;
    type Filter = LinSrgb;

    fn filter(&mut self, filter: LinSrgb) {
        self.lum = filter * self.lum;
    }

    fn lum(&self) -> LinSrgb {
        self.lum
    }
}
