#ifndef __SHAPE_H__
#define __SHAPE_H__

#include "point.h"
#include "ray.h"
#include "intersectionData.h"
#include "material.h"

class Shape
{
public:

    //abstract class needs a virtual destructor.
    virtual ~Shape() {}

    //make the draw() function PURE virtual: child classes MUST implement it;
    //there is no "base class" implementation.
    virtual void draw() = 0;

    virtual bool intersectWithRay(Ray &r, IntersectionData &id) = 0;

    Material material;
};


#endif
