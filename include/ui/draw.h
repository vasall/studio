#ifndef VASALL_STUDIO_UI_DRAW_H
#define VASALL_STUDIO_UI_DRAW_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/style.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef enum UIDrawKind {
        UI_DRAW_RECT = 0,       /* Rounded rect with fill and border */
        UI_DRAW_TEXT,           /* A run of UTF-8 text */
        UI_DRAW_ICON,           /* An icon mask, tinted */
        UI_DRAW_IMAGE,          /* A full RGBA texture (e.g. a viewport) */
        UI_DRAW_CALLBACK        /* Own rendering code, e.g. a live plot */
} UIDrawKind;

/*
 * Where a UI_DRAW_CALLBACK draws, in framebuffer pixels. When the callback
 * runs, the GL viewport is set to the command's rect and the scissor to its
 * clip, so drawing to normalized device coordinates (-1..1) fills the rect.
 */
typedef struct UIDrawCallbackInfo {
        s32 x;                  /* Rect, top left origin */
        s32 y;
        s32 w;
        s32 h;
        s32 fb_w;               /* Framebuffer size */
        s32 fb_h;
        f32 scale;              /* Framebuffer pixels per logical pixel */
} UIDrawCallbackInfo;

/*
 * Draws with any GL calls. It may change GL state freely; the renderer sets
 * its own state again afterwards. It runs during rendering, after the frame
 * was built, so data must live until then (e.g. in the frame arena).
 */
typedef void (*UIDrawCallbackFn)(const UIDrawCallbackInfo *info, void *data);

/*
 * One draw command in logical pixels. The UI core only produces these; the
 * renderer turns them into GL draw calls.
 */
typedef struct UIDrawCmd {
        u8 kind;                /* UIDrawKind */
        UIRect rect;            /* Rect, text origin (x, top) or icon rect */
        UIRect clip;

        UIColor color;          /* Fill, text or tint color */
        UIColor border_color;
        f32 border_width;
        f32 corner_radius[4];

        const char *text;       /* Not owned, lives in the frame arena */
        s32 text_len;
        u8 font;
        f32 font_size;

        u16 icon;
        u32 texture;            /* GL texture name for UI_DRAW_IMAGE */

        UIDrawCallbackFn callback;      /* For UI_DRAW_CALLBACK */
        void *callback_data;
} UIDrawCmd;

#endif /* VASALL_STUDIO_UI_DRAW_H */
