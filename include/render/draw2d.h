#ifndef VASALL_STUDIO_DRAW2D_H
#define VASALL_STUDIO_DRAW2D_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/list.h>

#include <glad/gl.h>

#include <render/atlas.h>
#include <render/font.h>
#include <render/icons.h>
#include <ui/draw.h>
#include <ui/style.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * Renders UI draw lists: every command becomes one or more quads for one
 * shader (rounded-rect SDF, glyph/icon masks, textures). Quads are batched
 * until the texture or the clip rect changes.
 */
typedef struct Draw2D {
        GLuint program;
        GLint u_screen;
        GLint u_tex;
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        s32 ebo_quads;          /* Quads the index buffer has room for */

        List verts;             /* Draw2DVertex */
        List batches;           /* Draw2DBatch */

        Atlas atlas;
        Font fonts[UI_FONT_COUNT];
        Icons icons;

        s32 draw_calls;         /* Of the last render, for the debug overlay */
        s32 quads;
} Draw2D;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Load shaders, fonts and icons from asset_dir. Needs a current GL context.
 *
 * @returns 0 on success, -1 on error
 */
PUB s32 draw2d_init(Draw2D *d, const char *asset_dir);
PUB void draw2d_deinit(Draw2D *d);

/*
 * Draw a list of UIDrawCmd. Commands are in logical pixels and scaled by
 * scale to the framebuffer of fb_w x fb_h pixels.
 */
PUB void draw2d_render(Draw2D *d, List *cmds, f32 scale, s32 fb_w, s32 fb_h);

/*
 * Text metrics in logical pixels, matching what draw2d_render draws.
 */
PUB f32 draw2d_measure(Draw2D *d, u8 font, f32 size, f32 scale,
                const char *text, s32 len);
PUB f32 draw2d_line_height(Draw2D *d, u8 font, f32 size, f32 scale);

#endif /* VASALL_STUDIO_DRAW2D_H */
