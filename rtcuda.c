#define CUDART_NAN_F            __int_as_float(0x7fffffff)

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

__device__ void v_mul(float *v, float b, float *o) {
	o[0] = v[0] * b;
	o[1] = v[1] * b;
	o[2] = v[2] * b;
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
	v_mul(a, 1 / d, out);
}

struct Ray {
	float origin[3];
	float dir[3];
};

struct Sphere {
	float center[3];
	float radius;
	float color[3];
};

struct BufElem {
	float color[3];
	float pos[3];
	float depth;
	float norm[3];
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

	v_mul(vx, rx, vx);
	v_mul(vy, ry, vy);

	v_add(vx, vy, ray->origin);
	v_cpy(cam + 3, ray->dir);
}

__global__ void rt(BufElem *buf, int buf_size, Ray *rays, Sphere* data, int data_items) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i >= buf_size) return;

	Ray *incoming = rays + i;
	BufElem *elem = buf + i;

	if (incoming->origin[0] == CUDART_NAN_F) return;

	float *depth = &elem->depth;
	*depth = 100000;
	float *norm = elem->norm;
	float *hit = elem->pos;
	float *color = elem->color;

	bool hit_none = true;

	for (int j = 0; j < data_items; j++) {
		Sphere *obj = data + j;

		float t_hit;

		float v_center[3];
		v_sub(obj->center, incoming->origin, v_center);

		float t_closest = v_dot(v_center, incoming->dir);

		float p_closest[3];
		v_cpy(incoming->dir, p_closest);
		v_mul(p_closest, t_closest, p_closest);
		v_add(incoming->origin, p_closest, p_closest);

		float dsq_closest = v_dsq(p_closest, obj->center);

		float radius_sq = obj->radius;
		radius_sq *= radius_sq;

		if (dsq_closest <= radius_sq) {
			hit_none = false;

			float t_back_to_hit = sqrt(radius_sq - dsq_closest);
			t_hit = t_closest - t_back_to_hit;

			if (t_hit < *depth) {
				*depth = t_hit;

				v_mul(incoming->dir, t_hit, hit);
				v_add(incoming->origin, hit, hit);

				v_sub(obj->center, hit, norm);
				v_mul(norm, 1 / obj->radius, norm);

				v_cpy(obj->color, color);
			}
		}
	}

	if (hit_none) {
		incoming->origin[0] = CUDART_NAN_F;
	} else {
		v_cpy(hit, incoming->origin);
		v_cpy(norm, incoming->dir);
	}
}

__device__ unsigned char ftoi8(float val) {
	val *= 255;
	if (val < 0) val = 0;
	if (val > 255) val = 255;
	return (unsigned char) __float2uint_rn(val);
}

__global__ void to_rgb(unsigned char *rgb, BufElem *buf, int size) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i >= size) return;

	int oi = i * 3;

	rgb[oi + 0] = ftoi8(buf[i].color[0]);
	rgb[oi + 1] = ftoi8(buf[i].color[1]);
	rgb[oi + 2] = ftoi8(buf[i].color[2]);
}