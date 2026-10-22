#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define ZOOM_MIN  0.05f
#define ZOOM_MAX  50.0f
#define ZOOM_STEP 1.25f

// Structure to hold the texture and dimensions of an image
typedef struct {
    SDL_Texture *texture;
    int w, h;
} Image;

// Structure to track the current viewport state (zoom, pan, fit mode)
typedef struct {
    float zoom;          // 1.0 = 100%
    float pan_x, pan_y;  // image center offset from window center
    bool fit;            // auto-fit image to window
} View;

// Load an image from file path into an SDL texture
static bool load_image(SDL_Renderer *renderer, const char *path, Image *img)
{
    // Load image into CPU surface
    SDL_Surface *surface = IMG_Load(path);
    if (!surface) {
        fprintf(stderr, "Cannot load '%s': %s\n", path, IMG_GetError());
        return false;
    }

    // Create GPU texture from surface
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        fprintf(stderr, "Cannot create texture for '%s': %s\n", path, SDL_GetError());
        SDL_FreeSurface(surface);
        return false;
    }

    img->texture = texture;
    img->w = surface->w;
    img->h = surface->h;
    SDL_FreeSurface(surface); // Free CPU surface once stored in GPU texture
    return true;
}

// Calculate the zoom level required to fit the image inside the window
static float fit_zoom(const Image *img, int win_w, int win_h)
{
    float zw = (float)win_w / img->w;
    float zh = (float)win_h / img->h;
    return (zw < zh) ? zw : zh;
}

// Determine monitor boundaries to handle oversized images and restrict initial window size
static void fit_window_to_image(SDL_Window *window, const Image *img)
{
    SDL_Rect bounds;
    if (SDL_GetDisplayBounds(0, &bounds) != 0) {
        bounds.w = 1280;
        bounds.h = 720;
    }

    // Restrict initial window to maximum 80% of screen size, never upscale
    float scale_w = (bounds.w * 0.8f) / img->w;
    float scale_h = (bounds.h * 0.8f) / img->h;
    float scale = (scale_w < scale_h) ? scale_w : scale_h;
    if (scale > 1.0f) scale = 1.0f;

    int w = (int)(img->w * scale);
    int h = (int)(img->h * scale);
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

// Zoom around (mx, my) so the point under the cursor stays fixed in place
static void zoom_at(View *v, float factor, float mx, float my, int win_w, int win_h)
{
    float new_zoom = v->zoom * factor;
    if (new_zoom < ZOOM_MIN) new_zoom = ZOOM_MIN;
    if (new_zoom > ZOOM_MAX) new_zoom = ZOOM_MAX;
    factor = new_zoom / v->zoom;

    float rx = mx - win_w / 2.0f;
    float ry = my - win_h / 2.0f;

    v->pan_x = rx - (rx - v->pan_x) * factor;
    v->pan_y = ry - (ry - v->pan_y) * factor;
    v->zoom = new_zoom;
    v->fit = false;
}

// Render pass: clear background, compute destination rect with zoom/pan, and draw texture
static void draw(SDL_Renderer *renderer, SDL_Window *window, const Image *img, View *v)
{
    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);

    if (v->fit) {
        v->zoom = fit_zoom(img, win_w, win_h);
        v->pan_x = 0.0f;
        v->pan_y = 0.0f;
    }

    float w = img->w * v->zoom;
    float h = img->h * v->zoom;
    SDL_FRect dst = {
        win_w / 2.0f + v->pan_x - w / 2.0f,
        win_h / 2.0f + v->pan_y - h / 2.0f,
        w,
        h
    };

    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopyF(renderer, img->texture, NULL, &dst);
    SDL_RenderPresent(renderer);
}

// Create dynamic window title including file path, index, total count, and zoom percentage
static void update_title(SDL_Window *window, const char *path, int index, int count, float zoom)
{
    char title[512];
    snprintf(title, sizeof(title), "iver - %s [%d/%d] %d%%",
             path, index + 1, count, (int)(zoom * 100.0f + 0.5f));
    SDL_SetWindowTitle(window, title);
}

// Switch to next (+1) or previous (-1) image, skipping files that fail to load
static void show_other_image(SDL_Renderer *renderer, char **paths, int count,
                             int *index, int step, Image *img, View *view)
{
    int next = *index;

    for (int tries = 0; tries < count; tries++) {
        next = (next + step + count) % count;
        if (next == *index) break;

        Image fresh;
        if (load_image(renderer, paths[next], &fresh)) {
            SDL_DestroyTexture(img->texture);
            *img = fresh;
            *index = next;
            view->fit = true;
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    // Check command line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: iver <image> [image ...]\n");
        return 1;
    }

    char **paths = &argv[1];
    int count = argc - 1;

    int exit_code = 1;
    bool sdl_ok = false;
    bool img_ok = false;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    Image image = { NULL, 0, 0 };
    View view = { 1.0f, 0.0f, 0.0f, true };
    int index = -1;
    bool running = true;
    bool fullscreen = false;
    SDL_Event event;

    // Initialize SDL2 Video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        goto cleanup;
    }
    sdl_ok = true;

    // Initialize SDL_image for PNG, JPG, and WEBP support
    int got = IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP);
    if (!(got & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        fprintf(stderr, "IMG Init Error: %s\n", SDL_GetError());
        goto cleanup;
    }
    img_ok = true;

    // Must be set before textures are created to ensure smooth linear scaling
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    // Create the application window
    window = SDL_CreateWindow("iver", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              800, 600, SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Window Error: %s\n", SDL_GetError());
        goto cleanup;
    }

    // Create SDL renderer with hardware acceleration and VSync
    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        // Fallback to software renderer if hardware acceleration is unavailable
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        fprintf(stderr, "Renderer Error: %s\n", SDL_GetError());
        goto cleanup;
    }

    // Find and load the first valid image from the argument list
    for (int i = 0; i < count; i++) {
        if (load_image(renderer, paths[i], &image)) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        fprintf(stderr, "No image could be loaded.\n");
        goto cleanup;
    }
    fit_window_to_image(window, &image);

    // Main application loop
    while (running) {
        draw(renderer, window, &image, &view);
        update_title(window, paths[index], index, count, view.zoom);

        // Event handling loop
        if (!SDL_WaitEvent(&event)) break;

        int win_w, win_h;
        SDL_GetWindowSize(window, &win_w, &win_h);

        switch (event.type) {
        case SDL_QUIT:
            running = false;
            break;

        case SDL_MOUSEWHEEL:
            // Handle zooming via mouse wheel around the cursor position
            if (event.wheel.y != 0) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                float factor = (event.wheel.y > 0) ? ZOOM_STEP : 1.0f / ZOOM_STEP;
                zoom_at(&view, factor, (float)mx, (float)my, win_w, win_h);
            }
            break;

        case SDL_MOUSEMOTION:
            // Handle panning when the left mouse button is held down
            if (event.motion.state & SDL_BUTTON_LMASK) {
                view.pan_x += event.motion.xrel;
                view.pan_y += event.motion.yrel;
                view.fit = false;
            }
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_q:
                running = false; // Quit on ESC or Q
                break;

            case SDLK_f:
                // Toggle borderless desktop fullscreen on F key
                fullscreen = !fullscreen;
                SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                break;

            case SDLK_RIGHT:
            case SDLK_SPACE:
                // Switch to the next image
                show_other_image(renderer, paths, count, &index, +1, &image, &view);
                break;

            case SDLK_LEFT:
            case SDLK_BACKSPACE:
                // Switch to the previous image
                show_other_image(renderer, paths, count, &index, -1, &image, &view);
                break;

            case SDLK_PLUS:
            case SDLK_KP_PLUS:
            case SDLK_EQUALS:
                // Zoom in
                zoom_at(&view, ZOOM_STEP, win_w / 2.0f, win_h / 2.0f, win_w, win_h);
                break;

            case SDLK_MINUS:
            case SDLK_KP_MINUS:
                // Zoom out
                zoom_at(&view, 1.0f / ZOOM_STEP, win_w / 2.0f, win_h / 2.0f, win_w, win_h);
                break;

            case SDLK_0:
                // Reset view to fit the window
                view.fit = true;
                break;

            case SDLK_1:
                // Reset zoom to 100% (1:1 scale) and center position
                view.zoom = 1.0f;
                view.pan_x = 0.0f;
                view.pan_y = 0.0f;
                view.fit = false;
                break;
            }
            break;
        }
    }

    exit_code = 0;

cleanup:
    // Clean up allocated resources and subsystems
    if (image.texture) SDL_DestroyTexture(image.texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if (img_ok) IMG_Quit();
    if (sdl_ok) SDL_Quit();
    return exit_code;
}