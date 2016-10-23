//header guard - make sure that disaster doesn't 
//strike if someone includes this file multiple times.
#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "point.h"

class Material
{
public:

// CONSTRUCTORS / DESTRUCTORS

    Material();
    Material(Point _diffuse,
    		 Point _specular,
             Point _emission,
             float _gloss);
    ~Material();

// MISCELLANEOUS

    void setAsCurrent();

// MEMBER VARIABLES

    Point diffuse;
    Point specular;
    Point emission;
    float gloss;
};

#endif
