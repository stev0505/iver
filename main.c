#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc < 2) {
        printf("Usage: iver <image_path>\n");
        return 1;
    }

    const char *image_path = argv[1];

    // Basic file existence check before initializing heavy libraries
    FILE *file = fopen(image_path, "r");
    if (!file) {
        printf("Error: Could not open file '%s'\n", image_path);
        return 1;
    }
    fclose(file);

    // Initialize SDL2 Video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize SDL_image for PNG and JPG support
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        printf("IMG Init Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    // Load image into surface
    SDL_Surface *surface = IMG_Load(image_path);
    if (!surface) {
        printf("Image Load Error: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Determine monitor boundaries to handle oversized images
    SDL_Rect display_bounds;
    if (SDL_GetDisplayBounds(0, &display_bounds) != 0) {
        display_bounds.w = 1280;
        display_bounds.h = 720;
    }

    // Restrict initial window to maximum 80% of screen size
    int max_w = (int)(display_bounds.w * 0.8f);
    int max_h = (int)(display_bounds.h * 0.8f);

    int win_w = surface->w;
    int win_h = surface->h;

    float scale_w = (float)max_w / surface->w;
    float scale_h = (float)max_h / surface->h;
    float scale = (scale_w < scale_h) ? scale_w : scale_h;

    if (scale < 1.0f) {
        win_w = (int)(surface->w * scale);
        win_h = (int)(surface->h * scale);
    }

    // Create dynamic window title including the image path
    char title[256];
    snprintf(title, sizeof(title), "iver - %s", image_path);

    SDL_Window *window = SDL_CreateWindow(
        title,
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

    // Preserve aspect ratio and handle scaling automatically on window resize
    SDL_RenderSetLogicalSize(renderer, surface->w, surface->h);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface); // Free CPU surface once stored in GPU texture

    if (!texture) {
        printf("Texture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    bool running = true;
    bool fullscreen = false;
    SDL_Event event;

    while (running) {
        // Event handling loop
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        running = false; // Quit on ESC or Q
                        break;
                    case SDLK_f:
                        // Toggle borderless fullscreen on F key
                        fullscreen = !fullscreen;
                        SDL_SetWindowFullscreen(
                            window, 
                            fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0
                        );
                        break;
                }
            }
        }

        // Render pass
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    // Clean up allocated resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}