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

    Point total = Point(0, 0, 0);
    for (PointLight* l : lights) {
        Point to = l->position - hit.intersectionPoint;
        to.normalize();

        float light_dot = dot(hit.surfaceNormal, to);

        // ambient
        total += l->ambient * hit.material.ambient;

        // facing away
        if (light_dot < 0) continue;

        // shadow
        Ray lampray(hit.intersectionPoint, to);
        lampray.step(1e-4);
        IntersectionData blank;
        Shape **current = begin;
        if (nextHit(lampray, blank, 0, 1e10, current, end)) continue;

        // diffuse
        total += light_dot * l->diffuse * hit.material.diffuse;

        // spec
        float spec_dot = dot(to, reflect);
        if (spec_dot < 0) continue;
        total += pow(spec_dot, hit.material.shininess) * l->specular * hit.material.specular;
    }

    // reflection
    float reflectivity = hit.material.reflectivity;
    if (reflectivity > 0) {
        total *= (1 - reflectivity);

        float reflect_depth;
        Ray reflect_ray(hit.intersectionPoint, reflect);
        reflect_ray.step(1e-4);

        Shape **current = begin;
        traceRay(reflect_ray, current, end, lights, 0, 1e10, reflectivity, total, reflect_depth, step_depth + 1);
    }

    float alpha = hit.material.alpha;
    if (alpha < 1) {
        float ior = hit.material.ior;
        if (!hit.backface) ior = 1.0 / ior;

        float cosi = -dot(hit.surfaceNormal, in.direction);
        float k = 1 - ior * ior * (1 - cosi * cosi);
        if (k > 0) {
            Point refract = in.direction * ior + hit.surfaceNormal * (ior *  cosi - sqrt(k)); 
            refract.normalize();

            total *= alpha;

            float refract_depth;
            Ray refract_ray(hit.intersectionPoint, refract);
            refract_ray.step(1e-4);

            Shape **current = begin;
            traceRay(refract_ray, current, end, lights, 0, 1e10, 1 - alpha, total, refract_depth, step_depth + 1);
        }
    }

    color += weight * total;
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