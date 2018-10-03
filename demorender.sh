#! /bin/bash

set -e

if [ -v FINAL ]
then
    SIZE=${SIZE:-768}
    FRAMES=${FRAMES:-60}
    FPS=${FPS:-30}
    SAMPLES=${SAMPLES:-1024}
else
    echo "Using fast settings. Set FINAL=1 for higher quality."
    SIZE=${SIZE:-256}
    FRAMES=${FRAMES:-30}
    FPS=${FPS:-15}
    SAMPLES=${SAMPLES:-48}
fi

echo
echo "This will destroy the existing demo animation."
read -p "Are you sure? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 1
fi
echo

git clean -dXf demo
mkdir demo

echo
echo "Rendering $FRAMES ${SIZE}x$SIZE frames with $SAMPLES samples per pixel."
./target/release/sidequest -w $SIZE -h $SIZE -s $SAMPLES -f $FRAMES 'demo/frame%n.png'

echo
echo "Using ffmpeg to produce video."
FRAME_SELECT='demo/frame%d.png'
ffmpeg -framerate $FPS -i $FRAME_SELECT -filter_complex "[0:v] fps=12,scale=w=480:h=-1,split [a][b];[a] palettegen=stats_mode=single [p];[b][p] paletteuse=new=1" -y demo.gif
cp demo/frame0.png demo.png
