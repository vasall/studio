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

PUB s32 win_init(Window *win, WindowLinks lnk, const char *title, s32 w, s32 h)
{
        SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

        if(!(win->sdl_window = SDL_CreateWindow(title, w, h, flags))) {
                LOG(LOG_ERROR, "Failed to create SDL Window");
                LOG_S(LOG_ERROR, "%s", (char *)SDL_GetError());
                return -1;
        }

        SDL_strlcpy(win->title, title, sizeof(win->title));
        win->links = lnk;

        /* Use the pixel size, which can differ from w/h on HiDPI screens */
        SDL_GetWindowSizeInPixels(win->sdl_window, &win->width, &win->height);

        if(ui_screen_init(&win->ui, lnk.assets, lnk.renderer, win->width,
                          win->height) < 0) {
                LOG(LOG_ERROR, "Failed to initialize the UI");
                goto err_destroy_window;
        }

        win->color = fcolor(1, 1, 1, 1);
        win->scene = NULL;
        win->camera = NULL;

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

        ui_screen_deinit(&win->ui);
        SDL_DestroyWindow(win->sdl_window);
}

PUB void win_handle_event(Window *win, SDL_Event *event)
{
        if(event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED &&
           event->window.windowID == SDL_GetWindowID(win->sdl_window)) {
                /* The viewport is updated in render_begin */
                win->width = event->window.data1;
                win->height = event->window.data2;

                if(ui_screen_resize(&win->ui, win->width, win->height) < 0) {
                        LOG(LOG_ERROR, "Failed to resize the UI");
                }
        }
}

PUB void win_set_clear_color(Window *win, IColor col)
{
        win->color = fcolor_from_u8(col);
}


PUB void win_set_scene(Window *win, Scene *scn, Camera *cam)
{
        win->scene = scn;
        win->camera = cam;
}

PUB void win_render(Window *win)
{
        Renderer *ren = win->links.renderer;

        render_begin(ren, win);

        if(win->scene && win->camera) {
                scene_render(win->scene, ren, win->camera,
                             mkrect(0, 0, win->width, win->height),
                             win->height);
        }

        ui_screen_render(&win->ui);

        render_end(ren, win);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/
