
print("Importing")

import pycuda.autoinit
import pycuda.driver as drv
import numpy as np

from pycuda.compiler import SourceModule

print("Loading CUDA Code")

with open('rtcuda.c', 'r') as myfile:
	cudasrc = SourceModule(myfile.read())

print("Starting")

width = 1024
height = 1024
pix = width * height

print("Building scene data")

data_count = 75
data_size = 7 * data_count

#data = np.array([0, 0, 0, 1], np.float32)
data = np.empty(data_size, np.float32)

for i in range(0,data_size,7):
	data[i:(i+3)] = np.random.uniform(-1, 1, 3)
	data[i+3] = np.random.uniform(.05, .1)
	data[(i+4):(i+7)] = np.random.uniform(0, 1, 3)

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
	np.float32(3.5),
	np.float32(3.5),
	block=comp_block,
	grid=comp_grid)

print("Rendering")

buf = drv.mem_alloc(4 * 7 * pix)
rt = cudasrc.get_function("rt")
rt(
	buf,
	rays,
	drv.In(data),
	np.int32(data_size),
	block=comp_block,
	grid=comp_grid)

print("Render done")
print("Converting to image")

output = np.empty(3 * pix, np.uint8)

to_rgb = cudasrc.get_function("to_rgb")
to_rgb(
	drv.Out(output),
	buf,
	block=comp_block,
	grid=comp_grid)

from PIL import Image

print("Saving image")

img = Image.new('RGB', (width, height))
img.putdata(map(tuple, np.split(output, pix)))
img.save('image.png')

print("Done");