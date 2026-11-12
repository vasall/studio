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
#define UI_MAX_TOASTS     4     /* Notifications shown at once */
#define UI_TOAST_LEN      160
#define UI_TOAST_TIME     6.0   /* Seconds a notification stays */

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * Root trees of a window, drawn and hit-tested in this order (later ones are
 * on top).
 */
typedef enum UILayer {
        UI_LAYER_BASE = 0,
        UI_LAYER_OVERLAY,       /* Notifications */
        UI_LAYER_POPUP,
        UI_LAYER_TOOLTIP,
        UI_LAYER_DRAG,
        UI_LAYER_DEBUG,
        UI_LAYER_COUNT
} UILayer;

typedef enum UIBoxFlag {
        UI_BOX_CLICKABLE = 1 << 0,      /* Takes part in hit testing */
        UI_BOX_FOCUSABLE = 1 << 1,      /* Can take keyboard focus */
        UI_BOX_FLOATING  = 1 << 2,      /* Placed at float_x/y, not in flow */
        UI_BOX_SELECTED  = 1 << 3,      /* Use the :selected style */
        UI_BOX_DISABLED  = 1 << 4,      /* Use the :disabled style, no input */
        UI_BOX_PASSIVE   = 1 << 5       /* Never hot; clicks go through */
} UIBoxFlag;

typedef enum UICursor {
        UI_CURSOR_DEFAULT = 0,
        UI_CURSOR_POINTER,
        UI_CURSOR_TEXT,
        UI_CURSOR_RESIZE_EW,
        UI_CURSOR_RESIZE_NS,
        UI_CURSOR_MOVE,
        UI_CURSOR_COUNT
} UICursor;

typedef enum UIToastKind {
        UI_TOAST_INFO = 0,
        UI_TOAST_WARNING,
        UI_TOAST_ERROR
} UIToastKind;

typedef struct UI UI;
typedef struct UIBox UIBox;

/* Called while building the draw list, to add commands for a box */
typedef void (*UICustomDrawFn)(UI *ui, UIBox *box, void *data);

/* Width of a text run / height of a line, in logical pixels */
typedef f32 (*UIMeasureFn)(void *user, u8 font, f32 size, const char *text,
                s32 len);
typedef f32 (*UILineHeightFn)(void *user, u8 font, f32 size);

/*
 * Clipboard of the platform. get copies at most cap - 1 bytes into buf and
 * terminates it; it returns the length.
 */
typedef s32 (*UIClipboardGetFn)(void *user, char *buf, s32 cap);
typedef void (*UIClipboardSetFn)(void *user, const char *text, s32 len);

/*
 * State of a box that survives between frames, found by the box's key.
 * Freed when the box isn't built for a frame.
 */
typedef struct UIBoxState {
        u64 key;
        u64 last_frame;         /* Frame the box was last built in */
        UIRect rect;            /* Rect from the last layout */
        UIRect clip;            /* Visible part of rect */
        f32 scroll[2];          /* Scroll offset per axis */
        f32 content[2];         /* Size of the content, for scrolling */
        f32 view[2];            /* Size of the visible content box */
        s32 user_i;             /* Free for widgets */
        s32 user_j;
        f32 user_f;
        f64 user_t;
} UIBoxState;

struct UIBox {
        u64 key;                /* 0 for anonymous boxes */
        u64 seed;               /* Children's ids are hashed with this: the
                                   key, or the parent's seed if anonymous */
        u32 flags;              /* UIBoxFlag */
        const char *cls;        /* Theme class, must outlive the frame */
        UIStyle style;

        const char *text;       /* Lives in the frame arena */
        s32 text_len;
        u16 icon;               /* IconId, tinted with text_color */
        u8 cursor;              /* UICursor while hovered or active */

        f32 float_x;            /* Absolute position of floating boxes */
        f32 float_y;

        UICustomDrawFn custom_draw;
        void *custom_data;

        UIBox *parent;
        UIBox *first;
        UIBox *last;
        UIBox *next;
        UIBox *prev;
        s32 child_count;

        UIBoxState *state;      /* NULL for anonymous boxes */

        /* Computed by the layout */
        f32 size[2];            /* Border box */
        f32 pos[2];             /* Relative to the parent's content box */
        f32 content[2];         /* Size of the children incl. margins */
        f32 basis[2];           /* Size the box would have as FIT; GROW
                                   boxes count with it in FIT parents */
        s32 line;               /* Line index when the parent wraps */
        UIRect rect;            /* Absolute, logical pixels */
        UIRect clip;            /* Visible part */
};

/*
 * What happened to a box this frame. Based on the rect from last frame.
 */
typedef struct UISignal {
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
} UISignal;

typedef struct UIToast {
        u32 id;
        u8 kind;                /* UIToastKind */
        f64 start;              /* Time it was first shown, < 0 = not yet */
        char text[UI_TOAST_LEN];
} UIToast;

struct UI {
        Arena arena;            /* Boxes and strings of the current frame */
        Pool states;            /* UIBoxState */
        HashMap state_map;      /* key -> PoolHandle (index << 32 | gen) */

        const UITheme *theme;
        UIInput *input;
        UIMeasureFn measure;
        UILineHeightFn line_height;
        void *measure_user;

        f32 width;              /* Window size, logical pixels */
        f32 height;
        f64 time;               /* Seconds */
        u64 frame;

        UIBox *roots[UI_LAYER_COUNT];
        UIBox *parents[UI_MAX_DEPTH];
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

        /* Notifications, oldest first */
        UIToast toasts[UI_MAX_TOASTS];
        s32 toast_count;
        u32 toast_next_id;

        /* Platform hooks, may be NULL */
        UIClipboardGetFn clipboard_get;
        UIClipboardSetFn clipboard_set;
        void *clipboard_user;

        /* Set by a focused text field while building, read by the platform
           after the frame to turn text input (IME) on or off */
        s32 text_input;
        UIRect text_input_rect; /* Caret, so the IME window sits next to it */

        f64 wakeup;             /* Time a frame is needed, 0 = none */
        s32 changed;            /* Something needs another frame */
        s32 debug;              /* Draw the debug overlay */

        List cmds;              /* UIDrawCmd, built in ui_end_frame */
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
PUB s32 ui_init(UI *ui, const UITheme *theme, UIMeasureFn measure,
                UILineHeightFn line_height, void *measure_user);

PUB void ui_deinit(UI *ui);

/*
 * Start a frame. Creates one root per layer with the window size.
 */
PUB void ui_begin_frame(UI *ui, UIInput *input, f32 width, f32 height,
                f64 time);

/*
 * Finish a frame: layout, hit testing, draw list (ui->cmds), state cleanup.
 *
 * @returns 1 if another frame should follow right away (hover changed,
 *          an animation runs), 0 if the UI is idle
 */
PUB s32 ui_end_frame(UI *ui);

/*
 * Build into another layer until ui_layer_end.
 */
PUB void ui_layer_begin(UI *ui, UILayer layer);
PUB void ui_layer_end(UI *ui);

/*
 * Create a box as the last child of the current parent. id may be NULL for
 * an anonymous box (no state, no input). cls is the theme class (may be
 * NULL). The style starts as the class style, with state overrides from
 * last frame's interaction (hover, pressed, focused) and flags (selected,
 * disabled) applied.
 */
PUB UIBox *ui_box(UI *ui, const char *id, const char *cls, u32 flags);

/*
 * Like ui_box, and make the new box the current parent.
 */
PUB UIBox *ui_box_begin(UI *ui, const char *id, const char *cls, u32 flags);
PUB void ui_box_end(UI *ui);

PUB void ui_push_parent(UI *ui, UIBox *box);
PUB void ui_pop_parent(UI *ui);
PUB UIBox *ui_parent(UI *ui);

/*
 * Mix a number or pointer into the ids of the boxes created until
 * ui_pop_id, so lists can reuse the same id strings.
 */
PUB void ui_push_id(UI *ui, u64 seed);
PUB void ui_pop_id(UI *ui);

/*
 * Set the text of a box. The text is copied into the frame arena; only the
 * part before "##" is shown.
 */
PUB void ui_box_set_text(UI *ui, UIBox *box, const char *text);

/*
 * Get the state of a box by key (from last frame), or NULL.
 */
PUB UIBoxState *ui_state_of(UI *ui, u64 key);

/*
 * Interaction with a box this frame.
 */
PUB UISignal ui_signal(UI *ui, UIBox *box);

/*
 * Keyboard focus. A press on a box that isn't focusable clears it, Tab and
 * Shift+Tab move it between UI_BOX_FOCUSABLE boxes, Esc clears it.
 */
PUB void ui_set_focus(UI *ui, u64 key);

/*
 * Ask for a frame at time t (seconds), e.g. for a delayed tooltip.
 */
PUB void ui_request_wakeup(UI *ui, f64 t);

/*
 * Popups: one is open at a time. It closes on a click outside of the popup
 * layer or with ui_popup_close.
 */
PUB void ui_popup_open(UI *ui, u64 key, f32 x, f32 y);
PUB void ui_popup_close(UI *ui);
PUB s32 ui_popup_is_open(UI *ui, u64 key);

/*
 * Key of a box id under the current parent, without creating the box.
 */
PUB u64 ui_key(UI *ui, const char *id);

/*
 * Add a draw command (used by custom draw callbacks).
 */
PUB void ui_draw_push(UI *ui, const UIDrawCmd *cmd);

/*
 * Text before "##". Returns the length to show.
 */
PUB s32 ui_display_len(const char *text);

/*
 * Lay out a box tree (layout.c). Exposed for tests.
 */
PUB void ui_layout(UIBox *root, UIMeasureFn measure,
                UILineHeightFn line_height, void *user);

#endif /* VASALL_STUDIO_UI_H */
