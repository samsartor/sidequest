#define CUDART_NAN_F            __int_as_float(0x7fffffff)

#include "stdio.h"

__device__ void v_add(float *a, float *b, float *o) {
	o[0] = a[0] + b[0];
	o[1] = a[1] + b[1];
	o[2] = a[2] + b[2];
}

__device__ void v_sub(float *a, float *b, float *o) {
	o[0] = a[0] - b[0];
	o[1] = a[1] - b[1];
	o[2] = a[2] - b[2];
}

__device__ void v_cml(float *v, float b, float *o) {
	o[0] = v[0] * b;
	o[1] = v[1] * b;
	o[2] = v[2] * b;
}

__device__ void v_mul(float *a, float *b, float *o) {
	o[0] = a[0] * b[0];
	o[1] = a[1] * b[1];
	o[2] = a[2] * b[2];
}

__device__ void v_cpy(float *from, float *to) {
	to[0] = from[0];
	to[1] = from[1];
	to[2] = from[2];
}

__device__ float v_dot(float *a, float *b) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

__device__ void v_cross(float *u, float *v, float *o) {
	o[0] = u[1] * v[2] - u[2] * v[1];
	o[1] = u[2] * v[0] - u[0] * v[2];
	o[2] = u[0] * v[1] - u[1] * v[0];
}

__device__ float v_msq(float *a) {
	float d1 = a[0];
	float d2 = a[1];
	float d3 = a[2];
	return d1 * d1 + d2 * d2 + d3 * d3;
}

__device__ float v_dsq(float *a, float *b) {
	float d1 = a[0] - b[0];
	float d2 = a[1] - b[1];
	float d3 = a[2] - b[2];
	return d1 * d1 + d2 * d2 + d3 * d3;
}

__device__ void v_norm(float *a, float *out) {
	float d = sqrtf(v_msq(a));
	v_cml(a, 1 / d, out);
}

struct Ray {
	float origin[3];
	float dir[3];
};

struct Sphere {
	float center[3];
	float radius;
	int mat;
};

struct BufElem {
	float val[3];
	float filter[3];
	float depth;

	float out[3];
};

struct Material {
	float diff[3];
	float spec[3];
	float refl;
	float rough;
	float emiss[3];
};

__global__ void init_ortho(Ray *rays, int width, int height, float* cam, float hsize, float vsize) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	int x = i % width;
	int y = i / width;
	if (y >= height) return;

	Ray *ray = rays + i;

	float rx = (float) x / width;
	rx = (rx - .5) * hsize;
	float ry = (float) y / height;
	ry = (ry - .5) * vsize;

	float vx[3];
	float vy[3];

	v_cross(cam + 3, cam + 6, vx);
	v_norm(vx, vx);
	v_cross(vx, cam + 3, vy);
	v_norm(vy, vy);

	v_cml(vx, rx, vx);
	v_cml(vy, ry, vy);

	v_add(vx, vy, ray->origin);
	v_add(ray->origin, cam, ray->origin);
	v_cpy(cam + 3, ray->dir);
}

__global__ void clear_buf(BufElem *buf, int buf_size, float far, int sample_num) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i >= buf_size) return;

	buf += i;

	if (sample_num == 0) {
		buf->out[0] = 0;
		buf->out[1] = 0;
		buf->out[2] = 0;
	}

	buf->val[0] = 0;
	buf->val[1] = 0;
	buf->val[2] = 0;

	buf->filter[0] = 1;
	buf->filter[1] = 1;
	buf->filter[2] = 1;

	buf->depth = far;
}

__global__ void rt(BufElem *buf, int buf_size, Ray *rays, unsigned char* data, int data_items, Material* mats, int seed, float* randsrc, int randsrc_size) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i >= buf_size) return;

	Ray *incoming = rays + i;
	BufElem *elem = buf + i;

	if (incoming->origin[0] == CUDART_NAN_F) return;

	float *depth = &elem->depth;
	float norm[3];
	float hit[3];
	Material *mat = mats + 0;

	bool hit_none = true;

	for (int j = 0; j < data_items; j++) {
		Sphere *obj = (struct Sphere*) data;

		float t_hit;

		float v_center[3];
		v_sub(obj->center, incoming->origin, v_center);

		float t_closest = v_dot(v_center, incoming->dir);

		float p_closest[3];
		v_cpy(incoming->dir, p_closest);
		v_cml(p_closest, t_closest, p_closest);
		v_add(incoming->origin, p_closest, p_closest);

		float dsq_closest = v_dsq(p_closest, obj->center);

		float radius_sq = obj->radius;
		radius_sq *= radius_sq;

		if (dsq_closest <= radius_sq) {

			float t_back_to_hit = sqrt(radius_sq - dsq_closest);
			t_hit = t_closest - t_back_to_hit;

			if (t_hit > 0 && t_hit < *depth) {
				hit_none = false;

				*depth = t_hit;

				v_cml(incoming->dir, t_hit, hit);
				v_add(incoming->origin, hit, hit);

				v_sub(obj->center, hit, norm);
				v_cml(norm, 1 / obj->radius, norm);

				mat = mats + obj->mat;
			}
		}

		data += sizeof(Sphere);
	}

	float prob = 1;
	float *color;

	if (hit_none) {
		incoming->origin[0] = CUDART_NAN_F;
	} else {
		v_cpy(hit, incoming->origin);

		float* rand = randsrc + (i * 4 + seed) % (randsrc_size - 4);
		if (*rand < mat->refl) {
			float micro[3];
			float reflection[3];

			v_cpy(rand + 1, micro);
			v_cml(micro, mat->rough, micro);
			v_add(norm, micro, micro);
			v_norm(micro, micro);

			v_cpy(micro, reflection);
			v_cml(reflection, -2 * v_dot(incoming->dir, micro), reflection);
			v_add(reflection, incoming->dir, reflection);
			v_norm(reflection, reflection);

			v_cpy(reflection, incoming->dir);

			color = mat->spec;
		} else {
			v_cpy(rand + 1, incoming->dir);
			v_norm(incoming->dir, incoming->dir);

			prob = -v_dot(norm, incoming->dir);
			if (prob < 0) {
				v_cml(incoming->dir, -1, incoming->dir);
				prob *= -1;
			}
			prob *= 2;

			color = mat->diff;
		}
	}

	float filtered[3];
	v_mul(mat->emiss, elem->filter, filtered);
	v_add(filtered, elem->val, elem->val);
	v_mul(color, elem->filter, elem->filter);
	v_cml(elem->filter, prob, elem->filter);
}

__global__ void post_rt(BufElem *buf, int size) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i >= size) return;

	buf += i;

	v_add(buf->val, buf->out, buf->out);
}

__device__ unsigned char ftoi8(float val) {
	val *= 255;
	if (val < 0) val = 0;
	if (val > 255) val = 255;
	return (unsigned char) __float2uint_rn(val);
}

__global__ void to_rgb(unsigned char *rgb, BufElem *buf, int size, int samples) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i >= size) return;

	buf += i;
	float *fin = buf->out;
	v_cml(fin, 1 / (float) samples, fin);

	rgb[i * 3 + 0] = ftoi8(fin[0]);
	rgb[i * 3 + 1] = ftoi8(fin[1]);
	rgb[i * 3 + 2] = ftoi8(fin[2]);
}