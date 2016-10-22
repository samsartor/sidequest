#ifndef __POINT_LIGHT_H__
#define __POINT_LIGHT_H__

#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "point.h"

class PointLight
{
public:

    PointLight();
    PointLight(Point _position,
                Point _emission,
                float _radius,
                int _lightNumber);
    ~PointLight();

    
    void enableLightAndUpdateParameters();
    void setLightPosition();

    void draw();

    float radius;
    int lightNumber;

    Point position;
    
    Point emission;
};

#endif
