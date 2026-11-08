#ifndef VASALL_WINDOW_H
#define VASALL_WINDOW_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <asset/asset_cache.h>
#include <render/renderer.h>
#include <graphic/color.h>
#include <ui/ui_screen.h>

#include <SDL3/SDL.h>


#define WINDOW_TITLE_LENGTH     128

typedef struct Window {
        char title[WINDOW_TITLE_LENGTH];

        s32 width;
        s32 height;

        /* The color to clear the window with */
        FColor color;

        SDL_Window *sdl_window;

        Renderer *renderer;

        UIScreen ui;
} Window;

/*
 * Initialize a window with an OpenGL 3.3 core context and load the GL
 * functions. SDL must already be initialized with SDL_INIT_VIDEO.
 */
PUB s32 win_init(Window *win, AssetCache *assets, Renderer *ren, 
        const char *title, s32 w, s32 h);

/*
 * Destroy the OpenGL context and the window.
 */
PUB void win_deinit(Window *win);

/*
 * Set the color to clear the window with when a new frame starts.
*/
PUB void win_set_clear_color(Window *win, IColor col);

/*
 * Handle window events, like resizing. Call this for every polled event.
 */
PUB void win_handle_event(Window *win, SDL_Event *event);

PUB void win_update(Window *win);

PUB void win_render(Window *win);


#endif /* VASALL_WINDOW_H */
