#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "plane.h"
#include <stdio.h>

Plane::Plane()
{
    onPlane = Point(0,0,0);
    normal = Point(0,1,0);
    material = Material();
}   

Plane::Plane(Point _onPlane, Point _normal)
{
    onPlane = _onPlane;
    normal = _normal;
    material = Material();
}

Plane::~Plane()
{
}

void Plane::draw()
{
    //an ugly situation, but: get two vectors in-plane using 
    //the normal dotted with some vector that's not the normal.
    Point localX, localZ;
    if(normal.y > 0.99)
    {
        //special case: normal is (0,1,0) or close; cross it with (1,0,0) (arbitrary choice)
        localX = cross(normal, Point(1,0,0)).normalize();
        localZ = cross(localX, normal).normalize();
    } else {
        localX = cross(normal, Point(0,1,0)).normalize();
        localZ = cross(localX, normal).normalize();
    }

    float width = 100.0f;
    Point v1 = onPlane + localX*width + localZ*width;
    Point v2 = onPlane - localX*width + localZ*width;
    Point v3 = onPlane - localX*width - localZ*width;
    Point v4 = onPlane + localX*width - localZ*width;

    //to preserve GL_CULL_FACE to whatever it was before this got called, 
    //use glPushAttrib / glPopAttrib to store that value in OpenGL.
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_CULL_FACE);
    glBegin(GL_QUADS);
        normal.glNormal();
        v1.glVertex();
        v2.glVertex();
        v3.glVertex();
        v4.glVertex();
    glEnd();
    glPopAttrib();
}

bool Plane::intersectWithRay(Ray &r, IntersectionData &id)
{
    // Computes the intersection between a ray and the plane.
    // Store the result in IntersectionData

    float t = dot(onPlane - r.origin, normal) / dot(r.direction, normal);
    id.depth = t;
    id.intersectionPoint = r.origin + t * r.direction;
    id.surfaceNormal = normal;
    id.material = material;
    id.wasValidIntersection = true;

    return true;
}
