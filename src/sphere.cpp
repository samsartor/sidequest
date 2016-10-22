#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "sphere.h"
#include "intersectionData.h"
#include "ray.h" 
#include <stdio.h>

#include <math.h>

Sphere::Sphere()
{
    center = Point(0,0,0);
    radius = 1.0f;
    material = Material();
}

Sphere::Sphere(Point _center, float _radius)
{
    center = _center;
    radius = _radius;
    material = Material();
}

Sphere::~Sphere()
{
    //no dynamically allocated memory; nothing to do.
}

void Sphere::draw()
{
    material.setAsCurrent();

    glPushMatrix();
        glTranslatef(center.x, center.y, center.z);

        glutSolidSphere(radius, 16, 16);
    glPopMatrix();
}

bool Sphere::intersectWithRay(Ray &r, IntersectionData &id)
{    
    // Compute the intersection between a ray and the sphere.
    // Store the result in IntersectionData

    id.wasValidIntersection = false;

    Point v = r.direction;

    Point v_center = center - r.origin;

    float t_closest = dot(v_center, v); // cos(angle between v and v_center) * distance to center = distance to closest approach
    Point p_closest = r.origin + v * t_closest;

    float dsq_closest = (p_closest - center).magSq();
    if (dsq_closest > (radius * radius)) return false;

    float t_to_hit = sqrt(radius * radius - dsq_closest);
    float t_hit = t_closest - t_to_hit;

    id.backface = false;
    if (t_hit < 0 && (t_closest + t_to_hit) > 0) {
        t_hit = t_closest + t_to_hit;
        id.backface = true;
    }

    id.intersectionPoint = r.origin + v * t_hit;
    id.surfaceNormal = (id.intersectionPoint - center) / radius;
    if (id.backface) id.surfaceNormal *= -1;
    id.material = material;
    id.depth = t_hit;
    id.wasValidIntersection = true;

    return true;
}
