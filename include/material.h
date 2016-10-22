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
                Point _ambient,
                Point _specular,
                float _shininess,
                float _reflectivity,
                float _alpha,
                float _ior);
    ~Material();

// MISCELLANEOUS

    void setAsCurrent();

// MEMBER VARIABLES

    Point diffuse;
    Point ambient;
    Point specular;
    float shininess;
    float reflectivity;
    float alpha;
    float ior;
};

#endif
