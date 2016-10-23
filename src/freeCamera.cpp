#include "freeCamera.h"
#define _USE_MATH_DEFINES
#include "math.h"
#include <stdio.h>

//
//  FreeCamera()
//
//  Default constructor; sets some nice starting values.
//
FreeCamera::FreeCamera()
{
    position = Point(0,0,0);
    directionPhi = M_PI/2.0f;
    directionTheta = 0.0f;
    globalUpVector = Point(0,1,0);
    verticalFieldOfView = 45.0f;
    imageWidth = imageHeight = 512;
    nearClipPlane = 0.1f;
    farClipPlane = 1000.0f;
}

//
//  FreeCamera()
//
//  Default custom constructor; user can set all member variables in one go.
//
FreeCamera::FreeCamera(Point _position, 
                        Point _globalUpVector, 
                        int _imageWidth, 
                        int _imageHeight,
                        float _verticalFieldOfView,
                        float _nearClipPlane,
                        float _farClipPlane,
                        float _startingPhi, 
                        float _startingTheta)
{
    position = _position;
    globalUpVector = _globalUpVector;
    verticalFieldOfView = _verticalFieldOfView;
    imageWidth = _imageWidth;
    imageHeight = _imageHeight;
    nearClipPlane = _nearClipPlane;
    farClipPlane = _farClipPlane;
    directionPhi = _startingPhi;
    directionTheta = _startingTheta;
}

//
//  ~FreeCamera()
//
//  Default destructor.
//
FreeCamera::~FreeCamera()
{
    //no dynamically allocated variables; no cleanup.
}



//
//  getDirectionVector()
//
//  The direction of the camera is stored as (theta/phi), so when we want
//      a vector pointing in the direction of the camera, we need to explicitly 
//      convert. This function does that, and returns the result.
//
Point FreeCamera::getDirectionVector()
{
    Point dir;
    dir.x =  sinf(directionTheta)*sinf(directionPhi);
    dir.y = -cosf(directionPhi);
    dir.z = -cosf(directionTheta)*sinf(directionPhi);
    return dir;
}




//
//  sendLookAtMatrix()
//
//  Computes the necessary parameters for gluLookAt() and passes them.
//
void FreeCamera::sendLookAtMatrix()
{
    Point dir = this->getDirectionVector();

    gluLookAt(position.x, 
                position.y, 
                position.z,
                position.x + dir.x,
                position.y + dir.y,
                position.z + dir.z,
                globalUpVector.x,
                globalUpVector.y,
                globalUpVector.z);
}


//
//  sendPerspectiveProjection()
//
//  Computes the necessary parameters for gluPerspective() and passes them.
//
void FreeCamera::sendPerspectiveProjection()
{
    gluPerspective(verticalFieldOfView,
                    imageWidth / (float)imageHeight,
                    nearClipPlane,
                    farClipPlane);
}


//
//  updateDirection()
//
//  Generalized method for updating the look-at direction of the camera; the user
//      supplies an "x change" and "y change," which could come from mouse or 
//      keyboard or wherever, and the angles get updated appropriately.
//
void FreeCamera::updateDirection(float xChange, float yChange)
{
    directionTheta += xChange;
    directionPhi += yChange;

    if(directionPhi <= 0)
        directionPhi = 0+0.001;
    if(directionPhi >= M_PI)
        directionPhi = M_PI-0.001;
}

//
//  moveAlongDirection()
//
//  Moves the camera's position along its lookat direction.
//
void FreeCamera::moveAlongDirection(float amount)
{
    Point dir = this->getDirectionVector();
    position += dir*amount;
}


//
//  getPrimaryRay()
//
//  Computes a ray whose origin is the camera position and whose
//      direction takes it through a pixel that is (xPercent, yPercent)
//      of the way through the image. (0,0 should get a direction through
//      the upper left corner of the image and 1,1 should get a direction
//      through the lower right of the image.)
//
Ray FreeCamera::getPrimaryRay(float xPercent, float yPercent)
{
    //  Computes a ray whose origin is the camera position and whose
    //      direction takes it through a pixel that is (xPercent, yPercent)
    //      of the way through the image.

    // 0,0 is center of screen
    xPercent -= .5;
    yPercent -= .5;

    float fov = verticalFieldOfView * M_PI / 180.0;
    float v = tan(fov / 2.0) * 2;
    float u = v * ((float) imageWidth / imageHeight);

    u *= xPercent;
    v *= yPercent;

    Point dir = getDirectionVector();

    Point horiz = cross(dir, globalUpVector);
    horiz.normalize();

    Point vert = cross(dir, horiz);

    Point pix = horiz * u + vert * v;

    pix += dir;

    pix.normalize();

	return Ray(position, pix);
}


//
//  getPrimaryRayThroughPixel()
//
//  A helper function to get a ray given pixel coordinates, instead of percentages.
//  Can be extended to return multiple rays for antialiasing!
//
Ray FreeCamera::getPrimaryRayThroughPixel(int pixelCoordX, int pixelCoordY, int imageWidth, int imageHeight, float poffx, float poffy)
{
    //for now... just shoot a regular primary ray through the center of the pixel.
    return this->getPrimaryRay( (pixelCoordX+poffx) / (float)imageWidth,
                                (pixelCoordY+poffy) / (float)imageHeight);
}
