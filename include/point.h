/*
 *  CSE 40166 / 60166, Computer Graphics, Fall 2011
 *
 *  Project: doubleCamerasWoah 
 *  File: point.h
 *
 *  Description:
 *      Contains the definition of a simple point class, to make 
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


//header guard - make sure that disaster doesn't 
//strike if someone includes this file multiple times.
#ifndef __POINT_H__
#define __POINT_H__

class Point
{
public:

// CONSTRUCTORS / DESTRUCTORS

    Point()
    {x=y=z=0.0;}

    Point(double a, double b, double c)
    { x = a; y = b; z = c; }

// MISCELLANEOUS

    Point clamp(float min, float max);
    double magSq();
    double mag();
    Point& normalize();
    double at(int i);

    void glVertex();
    void glNormal();
    void glTexCoord();

Point& operator+=(Point rhs);
Point& operator-=(Point rhs);
Point& operator*=(float rhs);
Point& operator/=(float rhs);

// MEMBER VARIABLES

    double x,y,z;
};

// RELATED OPERATORS

Point operator*(Point a, float f);
Point operator/(Point a, float f);
Point operator/(float f, Point p);
Point operator*(float f, Point a);
Point operator+(Point a, Point b);
Point operator-(Point a, Point b);
Point operator*(Point a, Point b);


Point cross(Point a, Point b);
double dot(Point a, Point b);

#endif
