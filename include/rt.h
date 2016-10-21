#ifndef H_RT
#define H_RT

#include "point.h"
#include "ray.h"
#include "shape.h"
#include "pointLight.h"
#include "intersectionData.h"

#include <stdlib.h>
#include <vector>

bool nextHit(Ray &ray, IntersectionData &hit, float near, float far, Shape* &current, Shape** end);
void traceRay(Ray &ray, Shape** begin, Shape** end, std::vector<PointLight*> &lights, float near, float far, float weight, Point &color, float &depth, int step_depth);

#endif