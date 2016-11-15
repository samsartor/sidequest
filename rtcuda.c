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

__global__ void init_ortho(float* rays, int width, int height, float* cam, float hsize, float vsize) {
	// cam + 0 = pos
	// cam + 3 = dir
	// cam + 6 = up

	// ray + 0 = origin
	// ray + 3 = dir


	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	int x = i % width;
	int y = i / width;
	if (y >= height) return;

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

	v_add(vx, vy, rays + i * 6);
	v_cpy(cam + 3, rays + i * 6 + 3);
}

__global__ void rt(float *buf, float *rays, float* data, int data_size, int buf_size) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i >= buf_size) return;

	float *p_origin = &rays[i * 6];
	float *v_ray = p_origin + 3;

	float *obj = data;
	float *end = data + data_size;

	float clip_back = 100000;

	float depth = clip_back;
	float norm[3];
	float hit[3];
	float color[3] = {0, 0, 0};

	while (obj < end) {
		float t_hit;


		float *p_center = obj + 0;
		float radius = obj[3];
		float *obj_color = obj + 4;

		float v_center[3];
		v_sub(p_center, p_origin, v_center);

		float t_closest = v_dot(v_center, v_ray);

		float p_closest[3];
		v_cpy(v_ray, p_closest);
		v_mul(p_closest, t_closest, p_closest);
		v_add(p_origin, p_closest, p_closest);

		float dsq_closest = v_dsq(p_closest, p_center);
		float radius_sq = radius * radius;
		if (dsq_closest > radius_sq) {
			t_hit = clip_back;
		} else {
			float t_back_to_hit = sqrt(radius_sq - dsq_closest);
			t_hit = t_closest - t_back_to_hit;

			if (t_hit < depth) {
				depth = t_hit;

				v_mul(v_ray, t_hit, hit);
				v_add(p_origin, hit, hit);

				v_sub(hit, p_center, norm);
				v_mul(norm, 1 / radius, norm);

				v_cpy(obj_color, color);
			}
		}

		obj += 7;
	}

	float *opix = buf + i * 7;

	v_cpy(color, opix);
	v_cpy(hit, opix + 3);
	opix[6] = depth;
}

__device__ unsigned char ftoi8(float val) {
	val *= 255;
	if (val < 0) val = 0;
	if (val > 255) val = 255;
	return (unsigned char) __float2uint_rn(val);
}

__global__ void to_rgb(unsigned char *rgb, float* buf, int size) {
	const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
	if (i >= size) return;

	int oi = i * 3;
	int ii = i * 7;

	rgb[oi + 0] = ftoi8(buf[ii + 0]);
	rgb[oi + 1] = ftoi8(buf[ii + 1]);
	rgb[oi + 2] = ftoi8(buf[ii + 2]);
}