//header guard - make sure that disaster doesn't 
//strike if someone includes this file multiple times.
#ifndef __RAY_H__
#define __RAY_H__

#include "point.h"

class Ray
{
public:

// CONSTRUCTORS / DESTRUCTORS

    Ray();
    Ray(Point _origin, Point _direction);
    ~Ray();

    inline void step(float dt) {
    	origin += dt * direction;
    }

// MISCELLANEOUS

// MEMBER VARIABLES

    Point origin;
    Point direction;
};

#endif
