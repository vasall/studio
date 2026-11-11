#include <platform/app.h>

#include <devkit/log/log.h>

#include <glad/gl.h>

#include <stdio.h>
#include <string.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC s32 s_sdl_setup(void);
LOC s32 s_opengl_setup(void);

LOC s32 s_create_gl_context(App *app);
LOC void s_destroy_gl_context(App *app);

LOC void s_find_assets(App *app);
LOC AppWindow *s_window_of(App *app, const SDL_Event *ev);
LOC void s_handle_event(App *app, const SDL_Event *ev);
LOC void s_update_metrics(AppWindow *win);
LOC void s_render(App *app, AppWindow *win);
LOC s32 s_wait_timeout_ms(App *app);
LOC f64 s_now(void);
LOC void s_save_ppm(const char *path, s32 w, s32 h);
LOC f32 s_measure(void *user, u8 font, f32 size, const char *text, s32 len);
LOC f32 s_line_height(void *user, u8 font, f32 size);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 app_init(App *app)
{
        const char *theme;
        char path[600];
        static const SDL_SystemCursor cursor_ids[UI_CURSOR_COUNT] = {
                SDL_SYSTEM_CURSOR_DEFAULT,   SDL_SYSTEM_CURSOR_POINTER,
                SDL_SYSTEM_CURSOR_TEXT,      SDL_SYSTEM_CURSOR_EW_RESIZE,
                SDL_SYSTEM_CURSOR_NS_RESIZE, SDL_SYSTEM_CURSOR_MOVE};
        s32 i;

        memset(app, 0, sizeof(*app));

        if(s_sdl_setup() < 0) {
                LOG(LOG_ERROR, "Failed to initialize SDL");
                return -1;
        }

        LOG_S(LOG_INFO, "video driver: %s",
              (char *)SDL_GetCurrentVideoDriver());

        /* The renderer loads shaders and fonts from here */
        s_find_assets(app);
        LOG_S(LOG_INFO, "assets: %s", app->asset_dir);

        if(s_opengl_setup() < 0) {
                LOG(LOG_ERROR, "Failed to initialize OpenGL");
                goto err_quit_sdl;
        }

        if(s_create_gl_context(app) < 0) {
                LOG(LOG_ERROR, "Failed to create OpenGL context");
                goto err_quit_sdl;
        }

        app->wake_event = SDL_RegisterEvents(1);
        if(list_init(&app->windows, sizeof(AppWindow *), 4) < 0) {
                LOG(LOG_ERROR, "Failed to initialize window list");
                goto err_destroy_gl_context;
        }

        for(i = 0; i < UI_CURSOR_COUNT; i++) {
                app->cursors[i] = SDL_CreateSystemCursor(cursor_ids[i]);
        }

        /* VASALL_THEME=<name> loads assets/theme/<name>.theme instead */
        if(ui_theme_init(&app->theme) < 0) {
                LOG(LOG_ERROR, "Failed to initialize Theme System");
                goto err_deinit_win_list;
        }

        theme = SDL_getenv("VASALL_THEME");
        snprintf(path, sizeof(path), "%s/theme/%s.theme", app->asset_dir,
                 theme && *theme ? theme : "carbon-g100");
        if(ui_theme_load_file(&app->theme, path) < 0 && theme) {
                snprintf(path, sizeof(path), "%s/theme/carbon-g100.theme",
                         app->asset_dir);
                ui_theme_load_file(&app->theme, path);
        }
        LOG_S(LOG_INFO, "theme: %s", path);

        app->screenshot_path = SDL_getenv("VASALL_SCREENSHOT");
        app->running = 1;
        return 0;

err_deinit_win_list:
        list_deinit(&app->windows);
        for(i = 0; i < UI_CURSOR_COUNT; i++) {
                if(app->cursors[i]) SDL_DestroyCursor(app->cursors[i]);
        }

err_destroy_gl_context:
        s_destroy_gl_context(app);

err_quit_sdl:
        SDL_Quit();

        return -1;
}

PUB void app_deinit(App *app)
{
        AppWindow **pw;
        s32 i;

        s_destroy_gl_context(app);

        for(i = 0; i < list_count(&app->windows); i++) {
                pw = list_get(&app->windows, i);
                ui_deinit(&(*pw)->ui);
                SDL_DestroyWindow((*pw)->sdl);
                SDL_free(*pw);
        }
        list_deinit(&app->windows);

        for(i = 0; i < UI_CURSOR_COUNT; i++) {
                if(app->cursors[i]) SDL_DestroyCursor(app->cursors[i]);
        }
        ui_theme_deinit(&app->theme);
        SDL_Quit();
}

PUB AppWindow *app_window_open(App *app, const char *title, s32 w, s32 h,
                               AppFrameFn frame_fn, void *frame_user)
{
        SDL_WindowFlags wflags = 0;
        AppWindow *win;

        if(!(win = SDL_calloc(1, sizeof(AppWindow)))) {
                LOG(LOG_ERROR, "Failed to allocate memory for app window");
                return NULL;
        }

        win->app = app;
        win->frame_fn = frame_fn;
        win->frame_user = frame_user;
        win->is_main = list_count(&app->windows) == 0;

        wflags |= SDL_WINDOW_OPENGL;
        wflags |= SDL_WINDOW_RESIZABLE;
        wflags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
        wflags |= SDL_WINDOW_HIDDEN; /* Hide window until loading is finished */
        if(!(win->sdl = SDL_CreateWindow(title, w, h, wflags))) {
                LOG_S(LOG_ERROR, "can't create window: %s",
                      (char *)SDL_GetError());
                SDL_free(win);
                return NULL;
        }
        win->id = SDL_GetWindowID(win->sdl);
        SDL_SetWindowMinimumSize(win->sdl, 480, 320);

        SDL_GL_MakeCurrent(win->sdl, app->gl);

        /* Only the main window waits for vsync */
        SDL_GL_SetSwapInterval(win->is_main ? 1 : 0);

        if(ui_init(&win->ui, &app->theme, s_measure, s_line_height, win) < 0) {
                goto err;
        }

        s_update_metrics(win);
        list_push(&app->windows, 1, &win);
        win->needs_frame = 1;
        SDL_ShowWindow(win->sdl);
        return win;

err:
        SDL_DestroyWindow(win->sdl);
        SDL_free(win);
        return NULL;
}

PUB void app_run(App *app)
{
        SDL_Event ev;
        AppWindow **pw;
        s32 timeout;
        f64 now;
        s32 i;

        while(app->running) {
                timeout = s_wait_timeout_ms(app);
                if(SDL_WaitEventTimeout(&ev, timeout)) {
                        s_handle_event(app, &ev);
                        while(SDL_PollEvent(&ev))
                                s_handle_event(app, &ev);
                }
                if(!app->running) break;

                now = s_now();
                for(i = 0; i < list_count(&app->windows); i++) {
                        pw = list_get(&app->windows, i);
                        if((*pw)->ui.wakeup > 0.0 && (*pw)->ui.wakeup <= now)
                                (*pw)->needs_frame = 1;
                        if((*pw)->needs_frame || (*pw)->frame_sources > 0)
                                s_render(app, *pw);
                }
        }
}

PUB void app_request_frame(AppWindow *win)
{
        if(win) win->needs_frame = 1;
}

PUB void app_frame_source_begin(AppWindow *win)
{
        if(win) win->frame_sources++;
}

PUB void app_frame_source_end(AppWindow *win)
{
        if(win && win->frame_sources > 0) win->frame_sources--;
        if(win) win->needs_frame = 1;
}

PUB void app_wake(App *app)
{
        SDL_Event ev;

        /* SDL_PushEvent is thread-safe */
        SDL_zero(ev);
        ev.type = app->wake_event;
        SDL_PushEvent(&ev);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC s32 s_sdl_setup(void)
{
        if(!SDL_Init(SDL_INIT_VIDEO)) {
                LOG_S(LOG_ERROR, "SDL_Init failed: %s", (char *)SDL_GetError());
                return -1;
        }

        return 0;
}

LOC s32 s_opengl_setup(void)
{
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        /* Later windows share textures and buffers with the first context */
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

        return 0;
}

LOC s32 s_create_gl_context(App *app)
{
        SDL_WindowFlags wflags;

        /* Never shown; it only gives the context a surface to start on */
        wflags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
        if(!(app->dummy_window = SDL_CreateWindow("vs_dummy", 1, 1, wflags))) {
                LOG_S(LOG_ERROR, "can't create dummy window: %s",
                      (char *)SDL_GetError());

                return -1;
        }

        /* First window: create the context and the renderer */
        if(!(app->gl = SDL_GL_CreateContext(app->dummy_window))) {
                LOG_S(LOG_ERROR, "can't create GL context: %s",
                        (char *)SDL_GetError());
                goto err_destroy_dummy;
        }
        SDL_GL_MakeCurrent(app->dummy_window, app->gl);

        if(!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
                LOG(LOG_ERROR, "can't load GL functions");
                goto err_destroy_gl_context;
        }
        LOG_S(LOG_INFO, "GL: %s", (char *)glGetString(GL_VERSION));
        LOG_S(LOG_INFO, "GPU: %s", (char *)glGetString(GL_RENDERER));

        if(draw2d_init(&app->draw, app->asset_dir) < 0) {
                LOG(LOG_ERROR, "can't init renderer");
                goto err_destroy_gl_context;
        }

        return 0;

err_destroy_gl_context:
        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DestroyContext(app->gl);
        app->gl = NULL;

err_destroy_dummy:
        SDL_DestroyWindow(app->dummy_window);
        app->dummy_window = NULL;

        return -1;
}

LOC void s_destroy_gl_context(App *app)
{
        if(app->gl) {
                SDL_GL_MakeCurrent(app->dummy_window, app->gl);
                draw2d_deinit(&app->draw);
                SDL_GL_MakeCurrent(NULL, NULL);
                SDL_GL_DestroyContext(app->gl);
                app->gl = NULL;
        }
        if(app->dummy_window) {
                SDL_DestroyWindow(app->dummy_window);
                app->dummy_window = NULL;
        }
}

/* <exe dir>/../assets when run from build/, else ./assets */
LOC void s_find_assets(App *app)
{
        const char *base = SDL_GetBasePath();
        char probe[600];
        SDL_PathInfo info;

        if(base) {
                snprintf(app->asset_dir, sizeof(app->asset_dir), "%s../assets",
                         base);
                snprintf(probe, sizeof(probe), "%s/shaders", app->asset_dir);
                if(SDL_GetPathInfo(probe, &info)) return;
        }
        snprintf(app->asset_dir, sizeof(app->asset_dir), "assets");
}

LOC AppWindow *s_window_of(App *app, const SDL_Event *ev)
{
        SDL_Window *sdl = SDL_GetWindowFromEvent(ev);
        AppWindow **pw;
        s32 i;

        if(!sdl) return NULL;
        for(i = 0; i < list_count(&app->windows); i++) {
                pw = list_get(&app->windows, i);
                if((*pw)->sdl == sdl) return *pw;
        }
        return NULL;
}

LOC void s_handle_event(App *app, const SDL_Event *ev)
{
        AppWindow *win;
        AppWindow **pw;
        s32 i;

        if(ev->type == SDL_EVENT_QUIT) {
                app->running = 0;
                return;
        }
        if(ev->type == app->wake_event) {
                for(i = 0; i < list_count(&app->windows); i++) {
                        pw = list_get(&app->windows, i);
                        (*pw)->needs_frame = 1;
                }
                return;
        }

        if(!(win = s_window_of(app, ev))) return;

        switch(ev->type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if(win->is_main) app->running = 0;
                break;

        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_RESTORED:
                s_update_metrics(win);
                win->needs_frame = 1;
                break;

        default:
                if(app_input_event(win, ev)) win->needs_frame = 1;
                break;
        }
}

LOC void s_update_metrics(AppWindow *win)
{
        f32 density;

        SDL_GetWindowSizeInPixels(win->sdl, &win->fb_w, &win->fb_h);
        win->scale = SDL_GetWindowDisplayScale(win->sdl);
        if(win->scale <= 0.0f) win->scale = 1.0f;
        density = SDL_GetWindowPixelDensity(win->sdl);
        if(density <= 0.0f) density = 1.0f;

        win->width = (f32)win->fb_w / win->scale;
        win->height = (f32)win->fb_h / win->scale;
        win->mouse_scale = density / win->scale;
}

LOC void s_render(App *app, AppWindow *win)
{
        UiColor bg;
        f64 t0;
        s32 more;

        win->needs_frame = 0;
        if(SDL_GetWindowFlags(win->sdl) & SDL_WINDOW_MINIMIZED) return;

        SDL_GL_MakeCurrent(win->sdl, app->gl);
        s_update_metrics(win);
        t0 = s_now();

        ui_begin_frame(&win->ui, &win->input, win->width, win->height, t0);
        if(win->frame_fn) win->frame_fn(win, win->frame_user);
        more = ui_end_frame(&win->ui);
        ui_input_next_frame(&win->input);

        if(win->ui.cursor != app->cursor && app->cursors[win->ui.cursor]) {
                app->cursor = win->ui.cursor;
                SDL_SetCursor(app->cursors[app->cursor]);
        }

        bg = ui_theme_color(&app->theme, "background", ui_hex(0x161616ff));
        glViewport(0, 0, win->fb_w, win->fb_h);
        glClearColor(bg.r / 255.0f, bg.g / 255.0f, bg.b / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw2d_render(&app->draw, &win->ui.cmds, win->scale, win->fb_w,
                      win->fb_h);
        win->frame_ms = (s_now() - t0) * 1000.0;

        /* A few frames in, so hover state and glyphs have settled */
        if(app->screenshot_path && win->is_main && ++app->frames == 5) {
                s_save_ppm(app->screenshot_path, win->fb_w, win->fb_h);
                app->running = 0;
        }

        SDL_GL_SwapWindow(win->sdl);

        /* Hover changed, an animation runs, or glyphs were lost in a flush */
        if(more || app->draw.atlas.flushed || app->screenshot_path) {
                win->needs_frame = 1;
        }
}

/* How long the loop may sleep: 0 if a frame is due, -1 for forever */
LOC s32 s_wait_timeout_ms(App *app)
{
        AppWindow **pw;
        f64 now = s_now();
        f64 next = 0.0;
        f64 ms;
        s32 i;

        for(i = 0; i < list_count(&app->windows); i++) {
                pw = list_get(&app->windows, i);
                if((*pw)->needs_frame || (*pw)->frame_sources > 0) return 0;
                if((*pw)->ui.wakeup > 0.0 &&
                   (next == 0.0 || (*pw)->ui.wakeup < next))
                        next = (*pw)->ui.wakeup;
        }
        if(next == 0.0) return -1;

        ms = (next - now) * 1000.0;
        return ms <= 0.0 ? 0 : (s32)ms + 1;
}

/* Write the current framebuffer as a binary PPM (top row first) */
LOC void s_save_ppm(const char *path, s32 w, s32 h)
{
        FILE *fp;
        u8 *px;
        s32 y;

        if(!(px = SDL_malloc((size_t)w * (size_t)h * 3))) return;
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px);

        if((fp = fopen(path, "wb"))) {
                fprintf(fp, "P6\n%d %d\n255\n", w, h);
                for(y = h - 1; y >= 0; y--) {
                        fwrite(px + (size_t)y * (size_t)w * 3, 1, (size_t)w * 3,
                               fp);
                }
                fclose(fp);
                LOG_S(LOG_INFO, "screenshot saved to %s", (char *)path);
        }
        SDL_free(px);
}

LOC f64 s_now(void)
{
        return (f64)SDL_GetTicksNS() / 1e9;
}

LOC f32 s_measure(void *user, u8 font, f32 size, const char *text, s32 len)
{
        AppWindow *win = user;

        return draw2d_measure(&win->app->draw, font, size, win->scale, text,
                              len);
}

LOC f32 s_line_height(void *user, u8 font, f32 size)
{
        AppWindow *win = user;

        return draw2d_line_height(&win->app->draw, font, size, win->scale);
}
