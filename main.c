#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: viewer <image_path>\n");
        return 1;
    }

    const char *image_path = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        printf("IMG Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *surface = IMG_Load(image_path);
    if (!surface) {
        printf("Image Load Error: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // --- SCREEN BOUNDS & SCALING LOGIC ---
    SDL_Rect display_bounds;
    // Get the size of the primary display (index 0)
    if (SDL_GetDisplayBounds(0, &display_bounds) != 0) {
        // Fallback if display query fails
        display_bounds.w = 1280;
        display_bounds.h = 720;
    }

    // Set maximum allowed window dimensions (80% of screen size)
    int max_w = (int)(display_bounds.w * 0.8f);
    int max_h = (int)(display_bounds.h * 0.8f);

    int win_w = surface->w;
    int win_h = surface->h;

    // Calculate scaling factor to fit large images while maintaining aspect ratio
    float scale_w = (float)max_w / surface->w;
    float scale_h = (float)max_h / surface->h;
    float scale = (scale_w < scale_h) ? scale_w : scale_h;

    // Only scale down if the image is larger than the maximum allowed dimensions
    if (scale < 1.0f) {
        win_w = (int)(surface->w * scale);
        win_h = (int)(surface->h * scale);
    }
    // -------------------------------------

    SDL_Window *window = SDL_CreateWindow(
        "SDL2 Image Viewer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        printf("Window Error: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_FreeSurface(surface);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Keep correct aspect ratio and automatic scaling when manually resizing the window
    SDL_RenderSetLogicalSize(renderer, surface->w, surface->h);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        printf("Texture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}