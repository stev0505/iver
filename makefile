CC      = gcc
CFLAGS  = -Wall -Wextra -O2 $(shell pkg-config --cflags sdl2 SDL2_image)
LDFLAGS = $(shell pkg-config --libs sdl2 SDL2_image)

iver: main.c
	$(CC) $(CFLAGS) main.c -o iver $(LDFLAGS)

clean:
	rm -f iver

.PHONY: clean
