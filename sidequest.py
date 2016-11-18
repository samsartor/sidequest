width = 1024
height = 1024
samples = 1000;
pix = width * height

print("Importing")

import struct

import random

import pycuda.autoinit
import pycuda.driver as drv
import numpy as np
from pycuda.curandom import rand as curand

from pycuda.compiler import SourceModule

from pycuda.characterize import sizeof as gpu_sizeof

print("Loading CUDA Code")

with open('sidequest-cuda-rt.c', 'r') as myfile:
	cudasrc = SourceModule(myfile.read())

print("Render data")

print("  Building scene data")

object_count = 0

scene = bytes()

for i in range(40):
	scene += struct.pack("12sfi",
		np.random.uniform(-1, 1, 3).astype(np.float32).tobytes(),
		random.uniform(.1, .12),
		random.randint(1, 4))
	object_count += 1

print("  Uploading scene data")

gpu_scene = drv.to_device(scene)

print("  Building material data")

mats = bytes()

mats += struct.pack("12s12sff12s",
	np.array((0, 0, 0), dtype=np.float32).tobytes(),
	np.array((0, 0, 0), dtype=np.float32).tobytes(),
	0,
	0,
	np.array((.24, .24, .3), dtype=np.float32).tobytes())

mats += struct.pack("12s12sff12s",
	np.array((0, 0, 0), dtype=np.float32).tobytes(),
	np.array((0, 0, 0), dtype=np.float32).tobytes(),
	0,
	0,
	np.array((20, 16, 10), dtype=np.float32).tobytes())

mats += struct.pack("12s12sff12s",
	np.array((.8, .2, .2), dtype=np.float32).tobytes(),
	np.array((1, 1, 1), dtype=np.float32).tobytes(),
	.1,
	.05,
	np.array((0, 0, 0), dtype=np.float32).tobytes())

mats += struct.pack("12s12sff12s",
	np.array((.2, .8, .2), dtype=np.float32).tobytes(),
	np.array((1, 1, 1), dtype=np.float32).tobytes(),
	.1,
	.05,
	np.array((0, 0, 0), dtype=np.float32).tobytes())

mats += struct.pack("12s12sff12s",
	np.array((.9, .9, .9), dtype=np.float32).tobytes(),
	np.array((1, 1, 1), dtype=np.float32).tobytes(),
	.02,
	.2,
	np.array((0, 0, 0), dtype=np.float32).tobytes())

print("  Uploading material data")

gpu_mats = drv.to_device(mats)

print("  Building camera data")

cam = np.empty(9, np.float32)
cam[0:3] = np.array([2, 2, 2])
cam[3:6] = np.array([-.577, -.577, -.577])
cam[6:9] = np.array([0, 1, 0])

def ceil(x):
	return int(x + 0.5)

comp_block = (256,1,1)
comp_grid = (ceil(pix/256.0),1)

print("  Uploading camera data")

cam_gpu = drv.In(cam);

print("Rendering")

rand_count = 32235

rt = cudasrc.get_function("rt")
post_rt = cudasrc.get_function("post_rt")
clear_buf = cudasrc.get_function("clear_buf")
init_ortho = cudasrc.get_function("init_ortho")

print("  Allocing buffers")

buf = drv.mem_alloc(4 * 10 * pix)
rays = drv.mem_alloc(4 * 6 * pix)

print("  Building primary rays")

primary_rays = drv.mem_alloc(4 * 6 * pix)

init_ortho(
	primary_rays,
	np.int32(width),
	np.int32(height),
	cam_gpu,
	np.float32(3.2),
	np.float32(3.2),
	block=comp_block,
	grid=comp_grid)

print("  Generating rands")
rands = curand((rand_count,))

print("  Path tracing")

for i in range(samples):
	print("    %d/%d" % (i, samples))
	clear_buf(
		buf,
		np.int32(pix),
		np.float32(10000),
		np.int32(i),
		block=comp_block,
		grid=comp_grid)

	for j in range(6):
		rt(
			buf,
			np.int32(pix),
			primary_rays if j == 0 else rays,
			rays,
			gpu_scene,
			np.int32(object_count),
			gpu_mats,
			np.int32(random.randint(0, rand_count - 1)),
			rands,
			np.int32(rand_count),
			block=comp_block,
			grid=comp_grid)

	post_rt(
		buf,
		np.int32(pix),
		block=comp_block,
		grid=comp_grid)

print("Exporting")

print("  Creating image buffer")

output = np.empty(3 * pix, np.uint8)
output_gpu = drv.Out(output)

print("  Converting luminosity")

to_rgb = cudasrc.get_function("to_rgb")
to_rgb(
	output_gpu,
	buf,
	np.int32(pix),
	np.int32(samples),
	block=comp_block,
	grid=comp_grid)

print("  Saving image")

from PIL import Image

img = Image.frombytes('RGB', (width, height), output)
img.save('image.png')

print("Done")