# iver

A small, fast image viewer written in C with SDL2.

## Features

- Open one or many images and flip through them
- Zoom with the mouse wheel (around the cursor) and pan by dragging
- Fit to window, 100% view and fullscreen
- Smart initial window size (max. 80% of your screen, small images are not upscaled)
- Low CPU usage: the window only redraws when something happens
- Files that fail to load are skipped, the current image stays on screen
- Supports PNG, JPG and WebP (other formats depend on your SDL2_image build)

## Requirements

- A C compiler (`gcc`)
- `make` and `pkg-config`
- SDL2 and SDL2_image development packages

```bash
# Debian / Ubuntu
sudo apt install build-essential pkg-config libsdl2-dev libsdl2-image-dev

# Arch
sudo pacman -S base-devel sdl2 sdl2_image

# Fedora
sudo dnf install gcc make pkgconf-pkg-config SDL2-devel SDL2_image-devel
```

## Build

```bash
git clone https://github.com/stev0505/iver.git
cd iver
make
```

## Usage

```bash
./iver image.png
./iver photo1.jpg photo2.png photo3.webp
./iver *.jpg
```

## Controls

| Key / Mouse                | Action                         |
|:---------------------------|:-------------------------------|
| **Right** / **Space**      | Next image                     |
| **Left** / **Backspace**   | Previous image                 |
| **Mouse wheel**            | Zoom at cursor                 |
| **Left mouse button drag** | Pan                            |
| **+** / **-**              | Zoom in / out                  |
| **0**                      | Fit to window                  |
| **1**                      | 100% zoom                      |
| **F**                      | Toggle fullscreen              |
| **Q** / **Esc**            | Quit                           |

The window title shows the file name, the position in the list (e.g. `[2/5]`) and the current zoom level.

## Notes

- After the last image, **Right** wraps around to the first one (and the other way round).
- On HiDPI displays with scaling, the zoom and drag behaviour may feel slightly off.
