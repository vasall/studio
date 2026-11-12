#ifndef VASALL_STUDIO_UI_INPUT_H
#define VASALL_STUDIO_UI_INPUT_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define UI_INPUT_MAX_EVENTS 64
#define UI_INPUT_TEXT_LEN   32

/*
 * Keys. Printable ASCII keys use their lowercase character ('a', '1', '\\'),
 * everything else uses the codes below. The platform layer translates SDL
 * keycodes, so the UI core doesn't depend on SDL.
 */
enum {
        UI_KEY_NONE = 0,
        UI_KEY_SPECIAL = 0x10000,
        UI_KEY_ESCAPE,
        UI_KEY_ENTER,
        UI_KEY_TAB,
        UI_KEY_BACKSPACE,
        UI_KEY_DELETE,
        UI_KEY_INSERT,
        UI_KEY_LEFT,
        UI_KEY_RIGHT,
        UI_KEY_UP,
        UI_KEY_DOWN,
        UI_KEY_HOME,
        UI_KEY_END,
        UI_KEY_PAGE_UP,
        UI_KEY_PAGE_DOWN,
        UI_KEY_F1,
        UI_KEY_F2,
        UI_KEY_F3,
        UI_KEY_F4,
        UI_KEY_F5,
        UI_KEY_F6,
        UI_KEY_F7,
        UI_KEY_F8,
        UI_KEY_F9,
        UI_KEY_F10,
        UI_KEY_F11,
        UI_KEY_F12
};

/* Modifier bits */
enum {
        UI_MOD_SHIFT = 1 << 0,
        UI_MOD_CTRL  = 1 << 1,
        UI_MOD_ALT   = 1 << 2,
        UI_MOD_SUPER = 1 << 3
};

/* Physical key positions the keybindings may fall back to */
enum {
        UI_SCAN_NONE = 0,
        UI_SCAN_BACKSLASH
};

enum {
        UI_MOUSE_LEFT = 0,
        UI_MOUSE_MIDDLE,
        UI_MOUSE_RIGHT,
        UI_MOUSE_BUTTONS
};

typedef enum UIEventKind {
        UI_EVENT_KEY_DOWN = 0,
        UI_EVENT_KEY_UP,
        UI_EVENT_TEXT
} UIEventKind;

/*
 * One keyboard event. Keys and text are kept in one queue, so they can be
 * handled in the order they were typed.
 */
typedef struct UIEvent {
        u8 kind;                /* UIEventKind */
        u8 repeat;              /* Key held down */
        u8 consumed;            /* Set by whoever handled it */
        u16 mods;
        u32 key;                /* UI_KEY_* or ASCII */
        u32 scan;               /* UI_SCAN_* */
        char text[UI_INPUT_TEXT_LEN]; /* UTF-8, for UI_EVENT_TEXT */
} UIEvent;

/*
 * Input of one window for one frame, in logical pixels.
 */
typedef struct UIInput {
        f32 mouse_x;
        f32 mouse_y;
        s32 mouse_inside;       /* The mouse is over the window */

        u8 button_down[UI_MOUSE_BUTTONS];
        u8 button_pressed[UI_MOUSE_BUTTONS];   /* Went down this frame */
        u8 button_released[UI_MOUSE_BUTTONS];  /* Went up this frame */
        u8 click_count[UI_MOUSE_BUTTONS];      /* 2 = double click */

        f32 wheel_x;
        f32 wheel_y;

        u16 mods;

        UIEvent events[UI_INPUT_MAX_EVENTS];
        s32 event_count;
} UIInput;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Clear the per-frame parts (pressed/released, wheel, events). Held buttons,
 * mouse position and modifiers stay.
 */
PUB void ui_input_next_frame(UIInput *in);

/*
 * Append an event. Returns 0, or -1 if the queue is full.
 */
PUB s32 ui_input_push(UIInput *in, const UIEvent *ev);

/*
 * Check if a key event matches a key and an exact set of modifiers.
 * Matches the keycode, or the physical key if scan isn't UI_SCAN_NONE.
 */
PUB s32 ui_event_is_key(const UIEvent *ev, u32 key, u32 scan, u16 mods);

#endif /* VASALL_STUDIO_UI_INPUT_H */
