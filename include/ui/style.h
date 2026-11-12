#ifndef VASALL_STUDIO_UI_STYLE_H
#define VASALL_STUDIO_UI_STYLE_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct UIRect {
        f32 x;
        f32 y;
        f32 w;
        f32 h;
} UIRect;

typedef struct UIColor {
        u8 r;
        u8 g;
        u8 b;
        u8 a;
} UIColor;

typedef enum UISizeKind {
        UI_SIZE_FIT = 0,        /* Wrap content: children or text + padding */
        UI_SIZE_PIXELS,         /* Fixed logical pixels */
        UI_SIZE_PERCENT,        /* Of the parent's content box (0..1) */
        UI_SIZE_GROW            /* Share of the leftover space, by weight */
} UISizeKind;

typedef struct UISize {
        u8 kind;
        f32 value;              /* Pixels, percent (0..1) or grow weight */
        f32 min;
        f32 max;                /* <= 0 means no limit */
} UISize;

typedef enum UILayout {
        UI_LAYOUT_ROW = 0,      /* Children left to right */
        UI_LAYOUT_COLUMN,       /* Children top to bottom */
        UI_LAYOUT_STACK         /* Children on top of each other */
} UILayout;

typedef enum UIAlign {
        UI_ALIGN_START = 0,
        UI_ALIGN_CENTER,
        UI_ALIGN_END
} UIAlign;

typedef enum UIOverflow {
        UI_OVERFLOW_VISIBLE = 0,
        UI_OVERFLOW_CLIP,
        UI_OVERFLOW_SCROLL
} UIOverflow;

typedef enum UIFont {
        UI_FONT_UI = 0,
        UI_FONT_UI_MEDIUM,
        UI_FONT_UI_BOLD,
        UI_FONT_CODE,
        UI_FONT_COUNT
} UIFont;

/* Index order of per-side arrays */
enum {
        UI_LEFT = 0,
        UI_TOP,
        UI_RIGHT,
        UI_BOTTOM
};

/* Index order of per-corner arrays */
enum {
        UI_TOP_LEFT = 0,
        UI_TOP_RIGHT,
        UI_BOTTOM_RIGHT,
        UI_BOTTOM_LEFT
};

/* Axis index for size[2] / pos[2] arrays */
enum {
        UI_AXIS_X = 0,
        UI_AXIS_Y = 1
};

typedef struct UIStyle {
        UISize width;
        UISize height;

        UIColor infill;         /* Background fill, a = 0 means none */
        UIColor border_color;
        f32 border_width;       /* Drawn inside the box */
        f32 corner_radius[4];   /* Per corner, see UI_TOP_LEFT... */

        f32 margin[4];          /* Outside, per side, see UI_LEFT... */
        f32 padding[4];         /* Inside, per side */
        f32 gap;                /* Space between children */

        u8 align_x;             /* UIAlign: children horizontally */
        u8 align_y;             /* UIAlign: children vertically */
        u8 layout;              /* UILayout */
        u8 wrap;                /* Row/column: 0 = overflow, 1 = wrap */
        u8 overflow;            /* UIOverflow, when not wrapping */

        u8 font;                /* UIFont */
        f32 font_size;
        UIColor text_color;
} UIStyle;

/*
 * One bit per style field. Theme classes and state overrides only apply the
 * fields whose bit is set.
 */
typedef enum UIStyleField {
        UI_FIELD_WIDTH          = 1 << 0,
        UI_FIELD_HEIGHT         = 1 << 1,
        UI_FIELD_INFILL         = 1 << 2,
        UI_FIELD_BORDER_COLOR   = 1 << 3,
        UI_FIELD_BORDER_WIDTH   = 1 << 4,
        UI_FIELD_CORNER_RADIUS  = 1 << 5,
        UI_FIELD_MARGIN         = 1 << 6,
        UI_FIELD_PADDING        = 1 << 7,
        UI_FIELD_GAP            = 1 << 8,
        UI_FIELD_ALIGN_X        = 1 << 9,
        UI_FIELD_ALIGN_Y        = 1 << 10,
        UI_FIELD_LAYOUT         = 1 << 11,
        UI_FIELD_WRAP           = 1 << 12,
        UI_FIELD_OVERFLOW       = 1 << 13,
        UI_FIELD_FONT           = 1 << 14,
        UI_FIELD_FONT_SIZE      = 1 << 15,
        UI_FIELD_TEXT_COLOR     = 1 << 16
} UIStyleField;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/* Sizes */
PUB UISize ui_fit(void);
PUB UISize ui_px(f32 px);
PUB UISize ui_pct(f32 pct);
PUB UISize ui_grow(f32 weight);

/* Colors */
PUB UIColor ui_rgba(u8 r, u8 g, u8 b, u8 a);
PUB UIColor ui_hex(u32 rrggbbaa);
PUB UIColor ui_color_none(void);

/* Rects */
PUB UIRect ui_rect(f32 x, f32 y, f32 w, f32 h);
PUB UIRect ui_rect_intersect(UIRect a, UIRect b);
PUB s32 ui_rect_contains(UIRect r, f32 x, f32 y);

/* Fill all four sides / corners with one value or four values */
PUB void ui_sides(f32 out[4], f32 all);
PUB void ui_sides4(f32 out[4], f32 l, f32 t, f32 r, f32 b);
PUB void ui_radius(f32 out[4], f32 all);
PUB void ui_radius4(f32 out[4], f32 tl, f32 tr, f32 br, f32 bl);

/*
 * The default style: fit size, row layout, no fill, no border, UI font.
 */
PUB UIStyle ui_style_default(void);

/*
 * Copy the fields selected by mask from src to dst.
 */
PUB void ui_style_apply(UIStyle *dst, const UIStyle *src, u32 mask);

#endif /* VASALL_STUDIO_UI_STYLE_H */
