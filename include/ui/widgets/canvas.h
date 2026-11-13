#ifndef VASALL_STUDIO_UI_WIDGETS_CANVAS_H
#define VASALL_STUDIO_UI_WIDGETS_CANVAS_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct UICanvasShape UICanvasShape;

/**
 * A box to draw 2D shapes into, e.g. graphs. Shapes use canvas coordinates:
 * logical pixels from the top left corner of the canvas' content box (inside
 * its padding), y pointing down. Everything is clipped to the content box
 * and drawn in the order it was added.
 *
 * Simple shapes go through the normal UI renderer. Heavy content (a live
 * plot with many samples) can draw with its own GL code through
 * ui_canvas_gl, in between the other shapes.
 *
 * The canvas lives in the frame arena: use it only in the frame it was
 * created in.
 */
typedef struct UICanvas {
        UI *ui;
        UIBox *box;
        UISignal sig;           /* Input on the canvas; mouse_x/y are in
                                   canvas coordinates */
        f32 width;              /* Size of the drawing area from last frame, */
        f32 height;             /* 0 in the first frame */

        UICanvasShape *first;   /* Shapes in drawing order */
        UICanvasShape *last;
} UICanvas;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Create a canvas. Size and background come from the class (default
 * "canvas") or canvas->box->style.
 *
 *   UICanvas *cv = ui_canvas(ui, "plot", NULL);
 *   ui_canvas_rect(cv, ui_rect(10, 10, cv->width - 20, 40), color);
 *
 * @returns The canvas, or NULL if the frame arena is out of memory
 */
PUB UICanvas *ui_canvas(UI *ui, const char *id, const char *cls);

/**
 * A filled rectangle.
 */
PUB void ui_canvas_rect(UICanvas *cv, UIRect r, UIColor fill);

/**
 * A rectangle with fill, border (drawn inside r) and rounded corners. A fill
 * with alpha 0 draws only the outline.
 */
PUB void ui_canvas_box(UICanvas *cv, UIRect r, UIColor fill, UIColor border,
                f32 border_width, f32 radius);

/**
 * Draw r with own GL code: fn runs while the frame is rendered, with the
 * viewport set to r and the scissor to the visible part of the canvas (see
 * UIDrawCallbackFn). data must live until then, e.g. in the frame arena or
 * in the caller's state.
 */
PUB void ui_canvas_gl(UICanvas *cv, UIRect r, UIDrawCallbackFn fn,
                void *data);

#endif /* VASALL_STUDIO_UI_WIDGETS_CANVAS_H */
