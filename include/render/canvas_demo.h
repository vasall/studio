#ifndef VASALL_STUDIO_CANVAS_DEMO_H
#define VASALL_STUDIO_CANVAS_DEMO_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/draw.h>

/**
 * The smallest example of drawing into a canvas with own GL code
 * (ui_canvas_gl): a box drawn by its own shader, without vertex buffers.
 * Live plots will work the same way, with their samples in GPU buffers.
 */

typedef struct CanvasDemoBox {
        f32 x;                  /* Part of the callback's area, 0..1, */
        f32 y;                  /* from the top left */
        f32 w;
        f32 h;
        UIColor color;
} CanvasDemoBox;

/**
 * A UIDrawCallbackFn; data is a CanvasDemoBox. The shader is built on first
 * use and lives as long as the GL context.
 */
PUB void canvas_demo_draw(const UIDrawCallbackInfo *info, void *data);

#endif /* VASALL_STUDIO_CANVAS_DEMO_H */
