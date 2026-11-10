#ifndef VASALL_WINDOW_H
#define VASALL_WINDOW_H

typedef struct Window Window;

#include <devkit/scope.h>
#include <devkit/types.h>

#include <asset/asset_cache.h>
#include <render/renderer.h>
#include <graphic/general/color.h>
#include <ui/ui_screen.h>
#include <scene/scene.h>
#include <scene/camera.h>

#include <SDL3/SDL.h>


#define WINDOW_TITLE_LENGTH     128

typedef struct WindowLinks {
        AssetCache *assets;
        Renderer *renderer;
} WindowLinks;

struct Window {
        char title[WINDOW_TITLE_LENGTH];

        s32 width;
        s32 height;

        WindowLinks links;

        /* The color to clear the window with */
        FColor color;

        SDL_Window *sdl_window;

        UIScreen ui;
};

/*
 * Create a resizable window and its UI. The renderer must already be
 * initialized, all windows share its GL context.
 */
PUB s32 win_init(Window *win, WindowLinks lnk, const char *title, s32 w, s32 h);

/*
 * Free the UI and destroy the window.
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

/*
 * Show a scene in the whole window, below the UI. Pass NULL to show none.
 * The scene and camera are not owned by the window.
 */
PUB void win_set_scene(Window *win, Scene *scn, Camera *cam);

/*
 * Draw a frame: the clear color, the scene and the UI on top.
 */
PUB void win_render(Window *win);


#endif /* VASALL_WINDOW_H */
