#ifndef VASALL_UI_SCREEN_H
#define VASALL_UI_SCREEN_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <asset/asset_cache.h>
#include <graphic/image/image.h>
#include <render/renderer.h>
#include <shape/rectangle.h>

typedef struct UIScreenLinks {
        AssetCache *assets;
        Renderer *renderer;
} UIScreenLinks;

/*
 * The UI of one window. Everything is drawn on the CPU into the canvas, which
 * is then uploaded and drawn over the window. The canvas is transparent where
 * nothing is drawn, so a 3D scene rendered before shows through.
 */
typedef struct UIScreen {
        UIScreenLinks links;

        s32 width;
        s32 height;

        /* Premultiplied alpha, like all Images */
        Image canvas;
        TextureId texture;
        ShaderId shader;

        /* The part of the canvas that changed since the last upload */
        Rectangle dirty;
        s32 has_dirty;
} UIScreen;

/*
 * Create the canvas and load the UI shader from the assets
 * ("shader/shader_ui.vert" and "shader/shader_ui.frag").
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 ui_screen_init(UIScreen *screen, UIScreenLinks links, s32 w, s32 h);

/*
 * Free the canvas, its texture and the shader.
 */
PUB void ui_screen_deinit(UIScreen *screen);

/*
 * Resize the canvas to the new window size. The canvas is cleared.
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 ui_screen_resize(UIScreen *screen, s32 w, s32 h);

/*
 * Mark a part of the canvas as changed, so it is uploaded before the next
 * draw. Call this after drawing into the canvas.
 */
PUB void ui_screen_invalidate(UIScreen *screen, Rectangle rect);

/*
 * Upload the changed part of the canvas and draw it over the whole window.
 * Call inside render_begin/render_end, after the scenes.
 */
PUB void ui_screen_render(UIScreen *screen);

#endif /* VASALL_UI_SCREEN_H */
