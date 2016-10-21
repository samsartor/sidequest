#include "ray.h"
#include "point.h"
#include <math.h>

Ray::Ray()
{
    origin = Point(0,0,0);
    direction = Point(0,0,0);
}

Ray::Ray(Point _origin, Point _direction)
{
    origin = _origin;
    direction = _direction;
}

Ray::~Ray()
{
    //no dynamically allocated member variables; nothing to clean up.
}
