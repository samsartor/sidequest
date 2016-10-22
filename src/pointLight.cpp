#include "pointLight.h"
#include "helper.h"
#include <math.h>

PointLight::PointLight()
{
    #ifdef SHADE_PHONG
        diffuse = Point(1,1,1);
        ambient = Point(0.2, 0.2, 0.2);
        specular = Point(0,0,0);
    #else
        emission = Point(1, 1, 1);
    #endif

    lightNumber = 0;

    position = Point(10,10,10);
    radius = 1.0f;
}

PointLight::PointLight(Point _position,
                        Point _emission, 
                        float _radius,
                        int _lightNumber)
{
    position = _position;
    emission = _emission;
    radius = _radius;
    lightNumber = _lightNumber;
}

PointLight::~PointLight()
{
    //no dynamically allocated memory; no cleanup.
}   


void PointLight::enableLightAndUpdateParameters()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0 + lightNumber);

    float *tempDiffuse = new float[4];
    float *tempAmbient = new float[4];
    float *tempSpecular = new float[4];

    for(int i = 0; i < 3; i++)
    {
        tempDiffuse[i] = emission.at(i);
        tempAmbient[i] = 0;
        tempSpecular[i] = 0;
    }

    tempDiffuse[3] = 1.0f;
    tempAmbient[3] = 0.0f;
    tempSpecular[3] = 0.0f;


    glLightfv(GL_LIGHT0 + lightNumber, GL_DIFFUSE, tempDiffuse);
    glLightfv(GL_LIGHT0 + lightNumber, GL_AMBIENT, tempAmbient);
    glLightfv(GL_LIGHT0 + lightNumber, GL_SPECULAR, tempSpecular);

    delete [] tempDiffuse;
    delete [] tempAmbient;
    delete [] tempSpecular;
}


void PointLight::setLightPosition()
{
    float *tempPosition = new float[4];
    
    tempPosition[0] = position.x;
    tempPosition[1] = position.y;
    tempPosition[2] = position.z;
    tempPosition[3] = 1.0f;

    glLightfv(GL_LIGHT0 + lightNumber, GL_POSITION, tempPosition);

    delete [] tempPosition;
}

void PointLight::draw()
{
    float *diffuseColor= new float[4];
    float *ambientColor= new float[4];
    float *specularColor= new float[4];
    float *emissiveColor = new float[4];
    diffuseColor[0] = 0.0f;
    diffuseColor[1] = 0.0f;
    diffuseColor[2] = 0.0f;
    diffuseColor[3] = 1.0f;

    ambientColor[0] = 0.0f;
    ambientColor[1] = 0.0f;
    ambientColor[2] = 0.0f;
    ambientColor[3] = 1.0f;

    specularColor[0] = 0.0f;
    specularColor[1] = 0.0f;
    specularColor[2] = 0.0f;
    specularColor[3] = 1.0f;

    emissiveColor[0] = emission.x;
    emissiveColor[1] = emission.y;
    emissiveColor[2] = emission.z;
    emissiveColor[3] = 1.0f;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissiveColor);
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glutSolidSphere(radius, 16, 16);
    glPopMatrix();

    //reset the emissive color to black, since that's what it is for most objects..
    emissiveColor[0] = emissiveColor[1] = emissiveColor[2] = 0.0f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissiveColor);

    delete [] diffuseColor;
    delete [] ambientColor;
    delete [] specularColor;
    delete [] emissiveColor;
}
