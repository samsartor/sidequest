#ifndef __PLANE_H__
#define __PLANE_H__

#include "shape.h"
#include "point.h"
#include "ray.h"
#include "intersectionData.h"


class Plane : public Shape
{
public:
    Plane();
    Plane(Point _normal, Point _onPlane);
    ~Plane();

    void draw();
    bool intersectWithRay(Ray &r, IntersectionData &id);

    Point onPlane;
    Point normal;
};

#endif
