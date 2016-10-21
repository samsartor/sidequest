//header guard - make sure that disaster doesn't 
//strike if someone includes this file multiple times.
#ifndef __INTERSECTION_DATA_H__
#define __INTERSECTION_DATA_H__

#include "point.h"
#include "material.h"

class IntersectionData
{
public:

// CONSTRUCTORS / DESTRUCTORS

    IntersectionData();
    ~IntersectionData();

// MISCELLANEOUS

// MEMBER VARIABLES

    bool wasValidIntersection;
    float depth;
    Point intersectionPoint;
    Point surfaceNormal;
    Material material;
};

#endif
