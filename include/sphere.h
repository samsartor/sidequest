#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "shape.h"
#include "point.h"
#include "ray.h"
#include "intersectionData.h"


class Sphere : public Shape
{
public:
    Sphere();
    Sphere(Point _center, float _radius);
    ~Sphere();

    void draw();
    bool intersectWithRay(Ray &r, IntersectionData &id);

    Point center;
    float radius;
};

#endif
