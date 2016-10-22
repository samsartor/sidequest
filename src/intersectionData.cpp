#include "intersectionData.h"

IntersectionData::IntersectionData()
{
    wasValidIntersection = false;
    intersectionPoint = Point(0,0,0);
    surfaceNormal = Point(0,0,0);
    material = Material();
    depth = 1.0 / 0;
    backface = false;
}

IntersectionData::~IntersectionData()
{
    //no dynamically allocated variables; nothing to clean up.
}
