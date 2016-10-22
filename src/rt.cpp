#include "rt.h"

#include <math.h>

bool nextHit(Ray &ray, IntersectionData &hit, float near, float far, Shape** &current, Shape** end) {
    while (current != end) {
        bool isHit = (*current)->intersectWithRay(ray, hit);
        current++;
        if (isHit && hit.depth > near && hit.depth < far) {
            return true;
        }
    }
    return false;
}

void traceRay(Ray &ray, Shape** begin, Shape** end, std::vector<PointLight*> &lights, float near, float far, float weight, Point &color, float &depth, int step_depth) {
    if (step_depth > 10) return;

    IntersectionData hit;
    IntersectionData temp;
    depth = far;
    Shape** current = begin;
    while (nextHit(ray, temp, near, depth, current, end)) {
        hit = temp;
        depth = hit.depth;
    }
    if (hit.wasValidIntersection) {
        color += weight * hit.material.diffuse;
    }
}