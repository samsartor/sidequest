#ifndef __HELPER__H__
#define __HELPER__H__

#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include <vector>
#include <string>

#include "shape.h"
#include "freeCamera.h"
#include "pointLight.h"

using namespace std;

float max(float a, float b);
float min(float a, float b);
float getRand();
float getRand(float min, float max);
void drawGrid();
vector<string> tokenizeString(string input, string delimiters);
bool registerOpenGLTexture(unsigned char *imageData, int texWidth, int texHeight, GLuint &texHandle);
void writeToPPM(const char *filename, int width, int height, unsigned char *img);
void raytraceImage(FreeCamera camera, vector<Shape*> shapes, vector<PointLight*> lights, unsigned int imageWidth, unsigned int imageHeight, unsigned char* &raytracedImage);

#endif
