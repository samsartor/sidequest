
print("Importing")

import struct

import pycuda.autoinit
import pycuda.driver as drv
import numpy as np

from pycuda.compiler import SourceModule

from pycuda.characterize import sizeof as gpu_sizeof;

print("Loading CUDA Code")

with open('rtcuda.c', 'r') as myfile:
	cudasrc = SourceModule(myfile.read())

print("Starting")

width = 1024
height = 1024
pix = width * height

print("Building scene data")

object_count = 0

scene = bytes();

for i in range(0, 75):
	scene += struct.pack("12sf12s",
		np.random.uniform(-1, 1, 3).astype(np.float32).tobytes(),
		np.random.uniform(.05, .1),
		np.random.uniform(0, 1, 3).astype(np.float32).tobytes())
	object_count += 1

print("Uploading scene data")

gpu_scene = drv.to_device(scene)

print("Building camera data")

cam = np.empty(9, np.float32)
cam[0:3] = np.array([2, 2, 2])
cam[3:6] = np.array([-.577, -.577, -.577])
cam[6:9] = np.array([0, 1, 0])

def ceil(x):
	return int(x + 0.5)

comp_block = (256,1,1)
comp_grid = (ceil(pix/256.0),1)

print("Building initial rays")

rays = drv.mem_alloc(4 * 6 * pix)
init_ortho = cudasrc.get_function("init_ortho")
init_ortho(
	rays,
	np.int32(width),
	np.int32(height),
	drv.In(cam),
	np.float32(3.2),
	np.float32(3.2),
	block=comp_block,
	grid=comp_grid)

print("Rendering")

buf = drv.mem_alloc(4 * 7 * pix)
rt = cudasrc.get_function("rt")
rt(
	buf,
	np.int32(pix),
	rays,
	gpu_scene,
	np.int32(object_count),
	block=comp_block,
	grid=comp_grid)
rt(
	buf,
	np.int32(pix),
	rays,
	gpu_scene,
	np.int32(object_count),
	block=comp_block,
	grid=comp_grid)

print("Render done")
print("Converting to image")

output = np.empty(3 * pix, np.uint8)

to_rgb = cudasrc.get_function("to_rgb")
to_rgb(
	drv.Out(output),
	buf,
	np.int32(pix),
	block=comp_block,
	grid=comp_grid)

print("Saving image")

from PIL import Image

img = Image.frombytes('RGB', (width, height), output)
img.save('image.png')

print("Done");