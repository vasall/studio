#include <window/window.h>

#include <devkit/log/log.h>

#include <glad/gl.h>

#include <stdlib.h>
#include <string.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 win_init(Window *win, AssetCache *assets, Renderer *ren, 
        const char *title, s32 w, s32 h)
{
        SDL_WindowFlags flags = SDL_WINDOW_OPENGL;

        if(!(win->sdl_window = SDL_CreateWindow(title, w, h, flags))) {
                LOG(LOG_ERROR, "Failed to create SDL Window");
                LOG_S(LOG_ERROR, "%s", (char *)SDL_GetError());
                return -1;
        }

        SDL_strlcpy(win->title, title, sizeof(win->title));

        /* Use the pixel size, which can differ from w/h on HiDPI screens */
        SDL_GetWindowSizeInPixels(win->sdl_window, &win->width, &win->height);
        glViewport(0, 0, win->width, win->height);

        /* Initialize the UI */
        if(ui_screen_init(&win->ui, assets, win->width, win->height) < 0) {
                LOG(LOG_ERROR, "Failed to initialize the UI");
                goto err_destroy_window;
        }

        /**/
        win->color = fcolor(1, 1, 1, 1);
        win->renderer = ren;

        return 0;

err_destroy_window:
        SDL_DestroyWindow(win->sdl_window);

        return -1;
}

PUB void win_deinit(Window *win)
{
        if(!win) {
                return;
        }

        SDL_DestroyWindow(win->sdl_window);
}

PUB void win_handle_event(Window *win, SDL_Event *event)
{
        if(event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED &&
           event->window.windowID == SDL_GetWindowID(win->sdl_window)) {
                /* The viewport is updated in win_begin_frame */
                win->width = event->window.data1;
                win->height = event->window.data2;
        }
}

PUB void win_set_clear_color(Window *win, IColor col)
{
        win->color = fcolor_from_u8(col);
}


PUB void win_render(Window *win)
{
        render_begin(win->renderer, win);

        /* Render scenes */

        /* Render UI */

        render_end(win->renderer, win);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

