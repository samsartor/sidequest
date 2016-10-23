#include "rt.h"
#include "helper.h"

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
    if (depth > 4) return Point(0, 0, 0);
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
        Point sample;
        // if (getRand() > .5) { // russian roulette
            Point rdir = Point(getRand(-1, 1), getRand(-1, 1), getRand(-1, 1));
            rdir.normalize();

            float dot_sn = dot(rdir, hit.surfaceNormal);
            if (dot_sn < 0) {
                rdir *= -1;
                dot_sn *= -1;
            }

            //Point reflect = ray.direction - 2 * hit.surfaceNormal * dot(ray.direction, hit.surfaceNormal); 
            //float dot_rn = dot(rdir, reflect);

            Ray next_path(hit.intersectionPoint, rdir);
            next_path.step(.0001);

            sample += rt_sample(next_path, data, 0, 1e10, depth + 1) * hit.material.diffuse;
            sample *= dot_sn * 2; //pow(dot_rn, hit.material.gloss) * (hit.material.gloss + 2); // BRDF
        // }
        sample += hit.material.emission;
        return sample;
    } else {
        return data.skyEmission;
    }
}