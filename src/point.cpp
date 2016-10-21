/*
 *  CSE 40166 / 60166, Computer Graphics, Fall 2011
 *
 *  Project: doubleCamerasWoah 
 *  File: point.cpp
 *
 *  Description:
 *      Contains the implementation of a simple point class, to make 
 *      code more compact and increase code reuse. This is not intended as a
 *      serious example of competent object-oriented programming.
 *
 *  Author:
 *      Alexandri Zavodny, University of Notre Dame.
 *  
 *  Notes:
 *      Originally written for CSE 40166/60166 in Fall 2010 and modified for 2011.
 *
 */

#include "gl.h"

#include "point.h"
#include <math.h>


// OPERATOR OVERLOADS

Point operator*(Point a, float f)
{
    return Point(a.x*f,a.y*f,a.z*f);
}

Point operator/(Point a, float f)
{
    return Point(a.x/f,a.y/f,a.z/f);
}

Point operator/(float f, Point a)
{
    return Point(a.x/f,a.y/f,a.z/f);
}

Point operator*(float f, Point a)
{
    return Point(a.x*f,a.y*f,a.z*f);
}

Point operator+(Point a, Point b)
{
    return Point(a.x+b.x, a.y+b.y, a.z+b.z);
}

Point operator-(Point a, Point b)
{
    return Point(a.x-b.x, a.y-b.y, a.z-b.z);
}

Point operator*(Point a, Point b)
{
    return Point(a.x*b.x, a.y*b.y, a.z*b.z);
}

Point& Point::operator+=(Point rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}


Point& Point::operator-=(Point rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

Point& Point::operator*=(float rhs)
{
    x *= rhs;
    y *= rhs;
    z *= rhs;
    return *this;
}

Point& Point::operator/=(float rhs)
{
    x /= rhs;
    y /= rhs;
    z /= rhs;
    return *this;
}




Point cross(Point a, Point b)
{
    Point c;
    c.x = a.y*b.z - a.z*b.y;
    c.y = b.x*a.z - a.x*b.z;
    c.z = a.x*b.y - a.y*b.x;
    return c;
}

double dot(Point a, Point b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}


// MEMBER FUNCTIONS

Point Point::clamp(float min, float max)
{
    if(x < min) x = min;
    if(x > max) x = max;
    if(y < min) y = min;
    if(y > max) y = max;
    if(z < min) z = min;
    if(z > max) z = max;
    return *this;
}

double Point::magSq()
{
    return x*x + y*y + z*z;
}

double Point::mag()
{
    double t = magSq();
    if(t <= 0.0)
        return 0;
    return sqrt(t);
}

Point& Point::normalize()
{
    double m = mag();
    x /= m;
    y /= m;
    z /= m;
    return *this;
}

double Point::at(int i)
{
    if(i == 0)  return x;
    if(i == 1)  return y;
    if(i == 2)  return z;
    return -1;
}

void Point::glVertex()
{
    glVertex3f(x, y, z);
}

void Point::glNormal()
{
    glNormal3f(x, y, z);
}

void Point::glTexCoord()
{
    glTexCoord2f(x, y);
}
