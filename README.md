## Build using Docker

```bash
docker build -t quake32 .
```

## Run in Docker on Arch / Wayland

### Prerequisites

Install `xorg-xhost`:

```bash
sudo pacman -Sy xorg-xhost
```

### Run the Game

Allow local Docker containers to access XWayland:

```bash
xhost +local:
```

Run the container:

```bash
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -e SDL_VIDEODRIVER=x11 \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v $XDG_RUNTIME_DIR/pulse/native:/tmp/pulse-native \
  -e PULSE_SERVER=unix:/tmp/pulse-native \
  -v ~/.config/pulse/cookie:/root/.config/pulse/cookie \
  quake32
```

## Music

The music is not included in this repository. You can copy the music files from the original Quake CD to the `id1/music` directory. Name them `2.mp3`, `3.mp3`, ... `11.mp3`.
