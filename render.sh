#! /bin/bash

set -e

git clean -dXf demo
mkdir demo

./target/release/sidequest

FRAME_SELECT='demo/frame%03d.png'
FRAMES=$(ffprobe -v error -count_frames -select_streams v:0 -show_entries stream=nb_read_frames -of default=nokey=1:noprint_wrappers=1 $FRAME_SELECT)
echo ":: FOUND $FRAMES FRAMES"
FPS=$(($FRAMES / 2))

ffmpeg -framerate $FPS -i $FRAME_SELECT -filter_complex "[0:v] fps=12,scale=w=480:h=-1,split [a][b];[a] palettegen=stats_mode=single [p];[b][p] paletteuse=new=1" -y demo.gif
cp demo/frame000.png demo.png
