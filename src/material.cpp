#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "material.h"

Material::Material()
{
    diffuse = Point(1,1,1);
    emission = Point(0,0,0);
}

Material::Material(Point _diffuse,
                    Point _emission)
{
    diffuse = _diffuse;
    emission = _emission;
}

Material::~Material()
{
    //no dynamically allocated variables; nothing to clean up.
}


void Material::setAsCurrent()
{
    float *localDiffuse = new float[4];
    float *localAmbient= new float[4];

    for(int i = 0; i < 3; i++)
    {
        localDiffuse[i] = diffuse.at(i);
        localAmbient[i] = diffuse.at(i) * .2 + emission.at(i);
    }

    localDiffuse[3] = localAmbient[3] = 1.0;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, localDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, localAmbient);

    delete [] localDiffuse;
    delete [] localAmbient;
}
