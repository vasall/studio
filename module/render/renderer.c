
#include <render/renderer.h>

#include <devkit/log/log.h>

#include <glad/gl.h>
#include <SDL3/SDL.h>

PUB s32 render_init(Renderer *ren)
{
        SDL_WindowFlags flags;

        /* Request an OpenGL 3.3 core context (must be set before creation) */
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        
        /* A hidden 1x1 window, so the context doesn't depend on any of the
         * real windows and survives when they are closed */
        flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
        if(!(ren->hidden = SDL_CreateWindow("", 1, 1, flags))) {
                LOG(LOG_ERROR, "Failed to create hidden window");
                return -1;
        }
        
        if(!(ren->gl_context = SDL_GL_CreateContext(ren->hidden))) {
                LOG(LOG_ERROR, "Failed to create OpenGL context");
                goto err_destroy_hidden;
        }

        /* Load the OpenGL functions through SDL */
        if(!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
                LOG(LOG_ERROR, "Failed to load OpenGL functions");
                goto err_destroy_gl_context;
        }

        return 0;

err_destroy_gl_context:
        SDL_GL_DestroyContext(ren->gl_context);

err_destroy_hidden:
        SDL_DestroyWindow(ren->hidden);

        return -1;
}

PUB void render_deinit(Renderer *ren)
{
        (void)ren;
}

PUB void render_begin(Renderer *ren, Window *win)
{
        /* Direct all GL calls into this window */
        SDL_GL_MakeCurrent(win->sdl_window, ren->gl_context);
        glViewport(0, 0, win->width, win->height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

PUB void render_end(Renderer *ren, Window *win)
{
        (void)ren;

        SDL_GL_SwapWindow(win->sdl_window);
}