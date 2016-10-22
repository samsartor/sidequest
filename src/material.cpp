#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "material.h"

Material::Material()
{
    diffuse = Point(1,1,1);
    ambient = Point(0.2, 0.2, 0.2);
    specular = Point(0,0,0);
    shininess = 0.0f;
    reflectivity = 0.0f;
    alpha = 1.0f;
    ior = 1.45;
}

Material::Material(Point _diffuse,
                    Point _ambient,
                    Point _specular,
                    float _shininess,
                    float _reflectivity,
                    float _alpha,
                    float _ior)
{
    diffuse = _diffuse;
    ambient = _ambient;
    specular = _specular;
    shininess = _shininess;
    reflectivity = _reflectivity;
    alpha = _alpha;
    ior = _ior;
}

Material::~Material()
{
    //no dynamically allocated variables; nothing to clean up.
}


void Material::setAsCurrent()
{
    float *localDiffuse = new float[4];
    float *localAmbient= new float[4];
    float *localSpecular= new float[4];

    for(int i = 0; i < 3; i++)
    {
        localDiffuse[i] = diffuse.at(i);
        localAmbient[i] = ambient.at(i);
        localSpecular[i] = specular.at(i);
    }

    localDiffuse[3] = localAmbient[3] = localSpecular[3] = 1.0;

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, localDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, localAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, localSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    delete [] localDiffuse;
    delete [] localAmbient;
    delete [] localSpecular;
}
