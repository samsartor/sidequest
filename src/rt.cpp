#include "rt.h"

#include <math.h>

bool rt_nextHit(Ray &ray, RTData &data, int &firstShape, IntersectionData &hit, float near, float far) {
    while (firstShape < data.shapeCount) {
        bool isHit = data.shapes[firstShape]->intersectWithRay(ray, hit);
        firstShape++;
        if (isHit && hit.depth > near && hit.depth < far) {
            return true;
        }
    }
    return false;
}

Point rt_sample(Ray &ray, RTData &data, float near, float far, int depth) {
    IntersectionData hit;
    IntersectionData temp;
    int first = 0;
    float nearest = far;
    bool valid = false;
    while(rt_nextHit(ray, data, first, temp, near, nearest)) {
        valid = true;
        hit = temp;
        nearest = hit.depth;
    }

    if (valid) {
        return hit.material.diffuse;
    } else {
        return data.skyEmission;
    }
}

void rt_samplePath(Ray &ray, RTData &data, int destInd, float near, float far, int count) {
    for (int i = 0; i < count; i++) {
        data.dest[destInd] += rt_sample(ray, data, near, far, 0);
    }
}