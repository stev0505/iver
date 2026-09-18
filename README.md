# Simple SDL2 Image Viewer

A lightweight, hardware-accelerated image viewer written in C using **SDL2** and **SDL2_image**.

## Features
- Supports PNG and JPG image formats.
- Dynamic window sizing based on image resolution.
- Resizable window support.
- Low CPU overhead (uses event-driven blocking loop).

## Prerequisites

To build and run this application, you need `gcc` (or another C compiler) and the SDL2 development libraries installed.

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential libsdl2-dev libsdl2-image-dev