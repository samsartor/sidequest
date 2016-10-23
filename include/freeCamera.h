#ifndef __FREE_CAMERA__H__
#define __FREE_CAMERA__H__

#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "point.h"
#include "ray.h"


class FreeCamera
{
public:

    FreeCamera();
    FreeCamera(Point _position, 
                Point _globalUpVector, 
                int _imageWidth,
                int _imageHeight,
                float _verticalFieldOfView,
                float _nearClipPlane,
                float _farClipPlane,
                float _startingPhi, 
                float _startingTheta);
    ~FreeCamera();

    
    Point getDirectionVector();
    void sendPerspectiveProjection();
    void sendLookAtMatrix();
    void updateDirection(float xChange, float yChange);
    void moveAlongDirection(float amount);

    Ray getPrimaryRay(float xPercent, float yPercent);
    Ray getPrimaryRayThroughPixel(int pixelCoordX, int pixelCoordY, int imageWidth, int imageHeight, float poffx, float poffy);

    Point position;
    Point globalUpVector;
    int imageWidth;
    int imageHeight;
    float verticalFieldOfView;
    float nearClipPlane, farClipPlane;
    float directionTheta, directionPhi;
};


#endif
