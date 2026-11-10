#include <ui/ui_screen.h>

#include <devkit/log/log.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Load and compile the UI shader from the asset cache.
 */
LOC ShaderId u_load_shader(AssetCache *cache, Renderer *ren);

/*
 * Clear the canvas and create a texture of the same size for it.
 */
LOC s32 u_create_canvas(UIScreen *screen, s32 w, s32 h);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 ui_screen_init(UIScreen *screen, UIScreenLinks links, s32 w, s32 h)
{
        if(!screen || w < 1 || h < 1) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        screen->links = links;

        screen->shader = u_load_shader(screen->links.assets,
                screen->links.renderer);
        if(!RENDER_ID_VALID(screen->shader)) {
                LOG(LOG_ERROR, "Failed to load the UI shader");
                return -1;
        }

        if(img_init(&screen->canvas, w, h) < 0) {
                LOG(LOG_ERROR, "Failed to create the UI canvas");
                goto err_destroy_shader;
        }

        if(u_create_canvas(screen, w, h) < 0) {
                goto err_deinit_canvas;
        }

        return 0;

err_deinit_canvas:
        img_deinit(&screen->canvas);

err_destroy_shader:
        render_destroy_shader(screen->links.renderer, screen->shader);

        return -1;
}

PUB void ui_screen_deinit(UIScreen *screen)
{
        if(!screen) {
                return;
        }

        render_destroy_texture(screen->links.renderer, screen->texture);
        render_destroy_shader(screen->links.renderer, screen->shader);
        img_deinit(&screen->canvas);
}

PUB s32 ui_screen_resize(UIScreen *screen, s32 w, s32 h)
{
        if(!screen || w < 1 || h < 1) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(w == screen->width && h == screen->height) {
                return 0;
        }

        if(img_resize(&screen->canvas, w, h) < 0) {
                LOG(LOG_ERROR, "Failed to resize the UI canvas");
                return -1;
        }

        /* A texture can't change its size, so make a new one */
        render_destroy_texture(screen->links.renderer, screen->texture);

        return u_create_canvas(screen, w, h);
}

PUB void ui_screen_invalidate(UIScreen *screen, Rectangle rect)
{
        if(!screen || rect.w < 1 || rect.h < 1) {
                return;
        }

        if(screen->has_dirty) {
                screen->dirty = rect_union(screen->dirty, rect);
        }
        else {
                screen->dirty = rect;
                screen->has_dirty = 1;
        }
}

PUB void ui_screen_render(UIScreen *screen)
{
        Renderer *ren;

        if(!screen) {
                return;
        }

        ren = screen->links.renderer;

        if(screen->has_dirty) {
                render_update_texture(ren, screen->texture, &screen->canvas,
                                      screen->dirty);
                screen->has_dirty = 0;
        }

        /* Over everything that is already drawn, in the whole window */
        render_set_area(ren, mkrect(0, 0, screen->width, screen->height),
                        screen->height);
        render_set_depth_test(ren, 0);
        render_set_blend(ren, RENDER_BLEND_PREMULTIPLIED);

        render_use_shader(ren, screen->shader);
        render_set_texture(ren, 0, screen->texture);
        render_set_s32(ren, "u_canvas", 0);
        render_draw_fullscreen(ren);

        render_set_blend(ren, RENDER_BLEND_NONE);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC ShaderId u_load_shader(AssetCache *cache, Renderer *ren)
{
        ShaderId none = { POOL_HANDLE_NONE };
        TextId vertId;
        TextId fragId;
        const char *vertSrc;
        const char *fragSrc;

        vertId = asset_load_text(cache, "shader/shader_ui.vert");
        fragId = asset_load_text(cache, "shader/shader_ui.frag");
        if(!ASSET_ID_VALID(vertId) || !ASSET_ID_VALID(fragId)) {
                return none;
        }

        vertSrc = text_cstr(asset_text(cache, vertId));
        fragSrc = text_cstr(asset_text(cache, fragId));
        return render_create_shader(ren, vertSrc, fragSrc);
}

LOC s32 u_create_canvas(UIScreen *screen, s32 w, s32 h)
{
        /* Transparent, so the scenes below show through */
        img_fill(&screen->canvas, icolor(0, 0, 0, 0));

        screen->texture = render_create_texture(screen->links.renderer, w, h,
                                                screen->canvas.pixels);
        if(!RENDER_ID_VALID(screen->texture)) {
                LOG(LOG_ERROR, "Failed to create the UI texture");
                return -1;
        }

        screen->width = w;
        screen->height = h;
        screen->has_dirty = 0;

        return 0;
}
