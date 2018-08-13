#!/usr/bin/bash

git clean -dXf demo
mkdir demo
./target/release/sidequest
ffmpeg -i demo/frame%03d.png -filter_complex "[0:v] palettegen" -y /tmp/palette.png
ffmpeg -i demo/frame%03d.png -i /tmp/palette.png -filter_complex "[0:v][1:v] paletteuse" -y demo.gif
cp demo/frame000.png demo.png
