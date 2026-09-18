# iver - Minimalist SDL2 Image Viewer

**iver** is a lightweight, hardware-accelerated command-line image viewer written in C using **SDL2** and **SDL2_image**.

## Features

- Supports PNG and JPG image formats.
- Smart initial window scaling (prevents images from exceeding screen size).
- Aspect ratio preservation on resize and fullscreen.
- Low CPU footprint using an event-driven wait loop.
- Keyboard navigation controls.

## Keyboard Controls

| Key | Action |
| :--- | :--- |
| **F** | Toggle Fullscreen |
| **Q** / **ESC** | Quit `iver` |

## Prerequisites

Ensure you have a C compiler and the SDL2 development headers installed.

### Linux (Debian/Ubuntu)
```bash
sudo apt update
sudo apt install build-essential libsdl2-dev libsdl2-image-dev