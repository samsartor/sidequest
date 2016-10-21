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

inline void shadeHit(std::vector<PointLight*> &lights, Ray& in, IntersectionData &hit, float weight, Point &color, Shape** begin, Shape** end, int step_depth) {
    Point reflect = in.direction - 2 * hit.surfaceNormal * dot(in.direction, hit.surfaceNormal);

    Point phong = Point(0, 0, 0);
    for (PointLight* l : lights) {
        Point to = l->position - hit.intersectionPoint;
        to.normalize();

        float light_dot = dot(hit.surfaceNormal, to);

        // ambient
        phong += l->ambient * hit.material.ambient;

        // facing away
        if (light_dot < 0) continue;

        // shadow
        Ray lampray(hit.intersectionPoint, to);
        IntersectionData blank;
        Shape **current = begin;
        if (nextHit(lampray, blank, .01, 1e10, current, end)) continue;

        // diffuse
        phong += light_dot * l->diffuse * hit.material.diffuse;

        // spec
        float spec_dot = dot(to, reflect);
        if (spec_dot < 0) continue;
        phong += pow(spec_dot, hit.material.shininess) * l->specular * hit.material.specular;
    }

    // reflection
    float reflectivity = hit.material.reflectivity;
    if (reflectivity > 0) {
        float reflect_depth;
        Ray reflect_ray(hit.intersectionPoint, reflect);

        Shape **current = begin;
        traceRay(reflect_ray, current, end, lights, .01, 1e10, weight * reflectivity, color, reflect_depth, step_depth + 1);

        color += weight * phong * (1 - reflectivity);
    } else {
        color += weight * phong;
    }
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
        shadeHit(lights, ray, hit, weight, color, begin, end, step_depth);
    }
}