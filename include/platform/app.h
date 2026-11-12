#ifndef VASALL_STUDIO_APP_H
#define VASALL_STUDIO_APP_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/list.h>

#include <SDL3/SDL.h>

#include <render/draw2d.h>
#include <ui/input.h>
#include <ui/theme.h>
#include <ui/ui.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct App App;
typedef struct AppWindow AppWindow;

/* Builds the UI of a window for one frame (between ui_begin/end_frame) */
typedef void (*AppFrameFn)(AppWindow *win, void *user);

/*
 * One OS window with its own input and UI context. All windows share the
 * app's GL context and renderer.
 */
struct AppWindow {
        App *app;
        SDL_Window *sdl;
        SDL_WindowID id;

        f32 scale;              /* Logical pixel -> framebuffer pixel */
        f32 mouse_scale;        /* SDL window coordinate -> logical pixel */
        s32 fb_w;               /* Framebuffer size in pixels */
        s32 fb_h;
        f32 width;              /* Size in logical pixels */
        f32 height;

        UIInput input;
        UI ui;

        s32 needs_frame;        /* Draw at the next loop iteration */
        s32 frame_sources;      /* While > 0, draw continuously */
        s32 is_main;
        s32 text_input_on;      /* SDL text input started for this window */
        SDL_Rect text_input_area;

        AppFrameFn frame_fn;
        void *frame_user;

        f64 frame_ms;           /* CPU time of the last frame */
};

struct App {
        SDL_Window *dummy_window; /* Hidden, holds the GL context's surface */
        SDL_GLContext gl;         /* Shared by all windows */

        Draw2D draw;
        UITheme theme;
        char asset_dir[512];

        List windows;           /* AppWindow * */
        u32 wake_event;         /* SDL user event that wakes the loop */
        s32 running;

        SDL_Cursor *cursors[UI_CURSOR_COUNT];
        u8 cursor;

        /* Dev aid: VASALL_SCREENSHOT=file.ppm saves a frame, then quits */
        const char *screenshot_path;
        s32 frames;
};

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Start SDL, find the assets, create the GL context (on a hidden window),
 * the renderer with its fonts and icons, and load the theme. Windows opened
 * afterwards all share this context.
 *
 * @returns 0 on success, -1 on error
 */
PUB s32 app_init(App *app);
PUB void app_deinit(App *app);

/*
 * Open a window. The first one becomes the main window; closing it ends
 * the app.
 *
 * @returns The window, or NULL on error
 */
PUB AppWindow *app_window_open(App *app, const char *title, s32 w, s32 h,
                AppFrameFn frame_fn, void *frame_user);

/*
 * Run until the main window closes.
 */
PUB void app_run(App *app);

/*
 * Draw this window once more as soon as possible.
 */
PUB void app_request_frame(AppWindow *win);

/*
 * While at least one frame source is active, the window draws continuously
 * at display rate (e.g. live simulation, animations).
 */
PUB void app_frame_source_begin(AppWindow *win);
PUB void app_frame_source_end(AppWindow *win);

/*
 * Wake the main loop from any thread; every window draws a frame.
 */
PUB void app_wake(App *app);

/*
 * Translate an SDL event into window input (platform/input.c).
 *
 * @returns 1 if the event changed the input
 */
PUB s32 app_input_event(AppWindow *win, const SDL_Event *ev);

#endif /* VASALL_STUDIO_APP_H */
