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

typedef enum UiDrawKind {
        UI_DRAW_RECT = 0,       /* Rounded rect with fill and border */
        UI_DRAW_TEXT,           /* A run of UTF-8 text */
        UI_DRAW_ICON,           /* An icon mask, tinted */
        UI_DRAW_IMAGE           /* A full RGBA texture (e.g. a viewport) */
} UiDrawKind;

/*
 * One draw command in logical pixels. The UI core only produces these; the
 * renderer turns them into GL draw calls.
 */
typedef struct UiDrawCmd {
        u8 kind;                /* UiDrawKind */
        UiRect rect;            /* Rect, text origin (x, top) or icon rect */
        UiRect clip;

        UiColor color;          /* Fill, text or tint color */
        UiColor border_color;
        f32 border_width;
        f32 corner_radius[4];

        const char *text;       /* Not owned, lives in the frame arena */
        s32 text_len;
        u8 font;
        f32 font_size;

        u16 icon;
        u32 texture;            /* GL texture name for UI_DRAW_IMAGE */
} UiDrawCmd;

#endif /* VASALL_STUDIO_UI_DRAW_H */
