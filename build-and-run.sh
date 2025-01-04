#!/bin/bash

docker build -t quake32 .

xhost +local:

docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -e SDL_VIDEODRIVER=x11 \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v $XDG_RUNTIME_DIR/pulse/native:/tmp/pulse-native \
  -e PULSE_SERVER=unix:/tmp/pulse-native \
  -v ~/.config/pulse/cookie:/root/.config/pulse/cookie \
  quake32
