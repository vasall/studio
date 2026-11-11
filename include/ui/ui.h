#ifndef VASALL_STUDIO_UI_H
#define VASALL_STUDIO_UI_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/list.h>
#include <devkit/memory/pool.h>
#include <devkit/memory/hashmap.h>

#include <core/arena.h>
#include <devkit/util/hash.h>
#include <ui/draw.h>
#include <ui/input.h>
#include <ui/style.h>
#include <ui/theme.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define UI_MAX_DEPTH      64    /* Nesting of boxes */
#define UI_MAX_ID_SEEDS   32    /* Nesting of ui_push_id */
#define UI_DRAG_THRESHOLD 4.0f  /* Pixels before a press becomes a drag */
#define UI_TOOLTIP_DELAY  0.6   /* Seconds of hovering before a tooltip */

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * Root trees of a window, drawn and hit-tested in this order (later ones are
 * on top).
 */
typedef enum UiLayer {
        UI_LAYER_BASE = 0,
        UI_LAYER_POPUP,
        UI_LAYER_TOOLTIP,
        UI_LAYER_DRAG,
        UI_LAYER_DEBUG,
        UI_LAYER_COUNT
} UiLayer;

typedef enum UiBoxFlag {
        UI_BOX_CLICKABLE = 1 << 0,      /* Takes part in hit testing */
        UI_BOX_FOCUSABLE = 1 << 1,      /* Can take keyboard focus */
        UI_BOX_FLOATING  = 1 << 2,      /* Placed at float_x/y, not in flow */
        UI_BOX_SELECTED  = 1 << 3,      /* Use the :selected style */
        UI_BOX_DISABLED  = 1 << 4,      /* Use the :disabled style, no input */
        UI_BOX_PASSIVE   = 1 << 5       /* Never hot; clicks go through */
} UiBoxFlag;

typedef enum UiCursor {
        UI_CURSOR_DEFAULT = 0,
        UI_CURSOR_POINTER,
        UI_CURSOR_TEXT,
        UI_CURSOR_RESIZE_EW,
        UI_CURSOR_RESIZE_NS,
        UI_CURSOR_MOVE,
        UI_CURSOR_COUNT
} UiCursor;

typedef struct Ui Ui;
typedef struct UiBox UiBox;

/* Called while building the draw list, to add commands for a box */
typedef void (*UiCustomDrawFn)(Ui *ui, UiBox *box, void *data);

/* Width of a text run / height of a line, in logical pixels */
typedef f32 (*UiMeasureFn)(void *user, u8 font, f32 size, const char *text,
                s32 len);
typedef f32 (*UiLineHeightFn)(void *user, u8 font, f32 size);

/*
 * State of a box that survives between frames, found by the box's key.
 * Freed when the box isn't built for a frame.
 */
typedef struct UiBoxState {
        u64 key;
        u64 last_frame;         /* Frame the box was last built in */
        UiRect rect;            /* Rect from the last layout */
        UiRect clip;            /* Visible part of rect */
        f32 scroll[2];          /* Scroll offset per axis */
        f32 content[2];         /* Size of the content, for scrolling */
        f32 view[2];            /* Size of the visible content box */
        s32 user_i;             /* Free for widgets */
        f32 user_f;
        f64 user_t;
} UiBoxState;

struct UiBox {
        u64 key;                /* 0 for anonymous boxes */
        u64 seed;               /* Children's ids are hashed with this: the
                                   key, or the parent's seed if anonymous */
        u32 flags;              /* UiBoxFlag */
        const char *cls;        /* Theme class, must outlive the frame */
        UiStyle style;

        const char *text;       /* Lives in the frame arena */
        s32 text_len;
        u16 icon;               /* IconId, tinted with text_color */
        u8 cursor;              /* UiCursor while hovered or active */

        f32 float_x;            /* Absolute position of floating boxes */
        f32 float_y;

        UiCustomDrawFn custom_draw;
        void *custom_data;

        UiBox *parent;
        UiBox *first;
        UiBox *last;
        UiBox *next;
        UiBox *prev;
        s32 child_count;

        UiBoxState *state;      /* NULL for anonymous boxes */

        /* Computed by the layout */
        f32 size[2];            /* Border box */
        f32 pos[2];             /* Relative to the parent's content box */
        f32 content[2];         /* Size of the children incl. margins */
        f32 basis[2];           /* Size the box would have as FIT; GROW
                                   boxes count with it in FIT parents */
        s32 line;               /* Line index when the parent wraps */
        UiRect rect;            /* Absolute, logical pixels */
        UiRect clip;            /* Visible part */
};

/*
 * What happened to a box this frame. Based on the rect from last frame.
 */
typedef struct UiSignal {
        u8 hovering;
        u8 pressed;             /* Left button went down on it */
        u8 released;            /* Left button went up while it was active */
        u8 clicked;             /* Pressed and released on it */
        u8 double_clicked;
        u8 right_clicked;
        u8 middle_clicked;
        u8 dragging;            /* Active and moved past the threshold */
        f32 drag_dx;            /* Mouse movement since the press */
        f32 drag_dy;
        f32 mouse_x;            /* Mouse relative to the box */
        f32 mouse_y;
} UiSignal;

struct Ui {
        Arena arena;            /* Boxes and strings of the current frame */
        Pool states;            /* UiBoxState */
        HashMap state_map;      /* key -> PoolHandle (index << 32 | gen) */

        const UiTheme *theme;
        UiInput *input;
        UiMeasureFn measure;
        UiLineHeightFn line_height;
        void *measure_user;

        f32 width;              /* Window size, logical pixels */
        f32 height;
        f64 time;               /* Seconds */
        u64 frame;

        UiBox *roots[UI_LAYER_COUNT];
        UiBox *parents[UI_MAX_DEPTH];
        s32 depth;
        u64 seeds[UI_MAX_ID_SEEDS];
        s32 seed_depth;
        s32 box_count;

        /* Interaction, keys of boxes */
        u64 hot;                /* Under the mouse (from last frame) */
        s32 hot_layer;
        u64 active;             /* Got the press, owns the mouse */
        u64 focus;              /* Keyboard owner */
        u8 dragging;            /* Active box moved past the threshold */
        f32 press_x;
        f32 press_y;
        u8 cursor;

        /* Popup (context menu or dropdown), one at a time */
        u64 popup_key;
        f32 popup_x;
        f32 popup_y;
        u64 popup_frame;        /* Frame it was opened in */

        /* Tooltip */
        u64 tooltip_key;
        f64 tooltip_start;

        f64 wakeup;             /* Time a frame is needed, 0 = none */
        s32 changed;            /* Something needs another frame */
        s32 debug;              /* Draw the debug overlay */

        List cmds;              /* UiDrawCmd, built in ui_end_frame */
};

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Initialize a UI context. measure and line_height are used to size text.
 *
 * @returns 0 on success, -1 on error
 */
PUB s32 ui_init(Ui *ui, const UiTheme *theme, UiMeasureFn measure,
                UiLineHeightFn line_height, void *measure_user);

PUB void ui_deinit(Ui *ui);

/*
 * Start a frame. Creates one root per layer with the window size.
 */
PUB void ui_begin_frame(Ui *ui, UiInput *input, f32 width, f32 height,
                f64 time);

/*
 * Finish a frame: layout, hit testing, draw list (ui->cmds), state cleanup.
 *
 * @returns 1 if another frame should follow right away (hover changed,
 *          an animation runs), 0 if the UI is idle
 */
PUB s32 ui_end_frame(Ui *ui);

/*
 * Build into another layer until ui_layer_end.
 */
PUB void ui_layer_begin(Ui *ui, UiLayer layer);
PUB void ui_layer_end(Ui *ui);

/*
 * Create a box as the last child of the current parent. id may be NULL for
 * an anonymous box (no state, no input). cls is the theme class (may be
 * NULL). The style starts as the class style, with state overrides from
 * last frame's interaction (hover, pressed, focused) and flags (selected,
 * disabled) applied.
 */
PUB UiBox *ui_box(Ui *ui, const char *id, const char *cls, u32 flags);

/*
 * Like ui_box, and make the new box the current parent.
 */
PUB UiBox *ui_box_begin(Ui *ui, const char *id, const char *cls, u32 flags);
PUB void ui_box_end(Ui *ui);

PUB void ui_push_parent(Ui *ui, UiBox *box);
PUB void ui_pop_parent(Ui *ui);
PUB UiBox *ui_parent(Ui *ui);

/*
 * Mix a number or pointer into the ids of the boxes created until
 * ui_pop_id, so lists can reuse the same id strings.
 */
PUB void ui_push_id(Ui *ui, u64 seed);
PUB void ui_pop_id(Ui *ui);

/*
 * Set the text of a box. The text is copied into the frame arena; only the
 * part before "##" is shown.
 */
PUB void ui_box_set_text(Ui *ui, UiBox *box, const char *text);

/*
 * Get the state of a box by key (from last frame), or NULL.
 */
PUB UiBoxState *ui_state_of(Ui *ui, u64 key);

/*
 * Interaction with a box this frame.
 */
PUB UiSignal ui_signal(Ui *ui, UiBox *box);

PUB void ui_set_focus(Ui *ui, u64 key);

/*
 * Ask for a frame at time t (seconds), e.g. for a delayed tooltip.
 */
PUB void ui_request_wakeup(Ui *ui, f64 t);

/*
 * Popups: one is open at a time. It closes on a click outside of the popup
 * layer or with ui_popup_close.
 */
PUB void ui_popup_open(Ui *ui, u64 key, f32 x, f32 y);
PUB void ui_popup_close(Ui *ui);
PUB s32 ui_popup_is_open(Ui *ui, u64 key);

/*
 * Key of a box id under the current parent, without creating the box.
 */
PUB u64 ui_key(Ui *ui, const char *id);

/*
 * Add a draw command (used by custom draw callbacks).
 */
PUB void ui_draw_push(Ui *ui, const UiDrawCmd *cmd);

/*
 * Text before "##". Returns the length to show.
 */
PUB s32 ui_display_len(const char *text);

/*
 * Lay out a box tree (layout.c). Exposed for tests.
 */
PUB void ui_layout(UiBox *root, UiMeasureFn measure,
                UiLineHeightFn line_height, void *user);

#endif /* VASALL_STUDIO_UI_H */
