#ifndef H_RT
#define H_RT

#include "point.h"
#include "point.h"
#include "ray.h"
#include "shape.h"
#include "intersectionData.h"

struct RTData {
	Point* dest = 0;
	int samples;
	unsigned int width;
	unsigned int height;
	Shape** shapes = 0;
	int shapeCount;
	Point skyEmission;
};

bool rt_nextHit(Ray &ray, RTData &data, int &firstShape, IntersectionData &hit, float near, float far);
Point rt_sample(Ray &ray, RTData &data, float near, float far, int depth);
inline void rt_samplePath(Ray &ray, RTData &data, int destInd, float near, float far) {
    data.dest[destInd] += rt_sample(ray, data, near, far, 0);
}

#endif