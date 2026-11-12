#ifndef VASALL_STUDIO_UI_WIDGETS_H
#define VASALL_STUDIO_UI_WIDGETS_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct UITabResult {
        UISignal sig;           /* Of the tab itself */
        u64 key;                /* Key of the tab box (for its state/rect) */
        s32 close_clicked;
} UITabResult;

typedef struct UITextFieldResult {
        UISignal sig;
        u64 key;                /* Key of the field, e.g. for ui_set_focus */
        s32 changed;            /* The text was edited this frame */
        s32 submitted;          /* Enter was pressed */
        s32 cancelled;          /* Esc was pressed; focus is dropped */
        s32 focused;
} UITextFieldResult;

typedef struct UITreeRowResult {
        UISignal sig;           /* Of the whole row */
        s32 toggled;            /* *expanded was flipped */
} UITreeRowResult;

/*
 * A virtualized list: only the rows in view are built. Every row must be
 * exactly row_height tall.
 */
typedef struct UIList {
        UIBox *box;             /* The scroll area */
        s32 count;
        f32 row_height;
        s32 first;              /* Rows to build: first <= i < last */
        s32 last;
} UIList;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Empty box that takes space, e.g. ui_spacer(ui, ui_grow(1), ui_px(0)).
 */
PUB UIBox *ui_spacer(UI *ui, UISize w, UISize h);

/*
 * Text without interaction. cls defaults to "label".
 */
PUB UIBox *ui_label(UI *ui, const char *text, const char *cls);

/*
 * Icon without interaction. cls defaults to "icon".
 */
PUB UIBox *ui_icon(UI *ui, u16 icon, const char *cls);

/*
 * Text button. label is also the id ("Save##toolbar" shows "Save").
 * cls defaults to "button".
 */
PUB UISignal ui_button(UI *ui, const char *label, const char *cls);

/*
 * Icon button with an optional tooltip. cls defaults to "icon-button".
 */
PUB UISignal ui_icon_button(UI *ui, const char *id, u16 icon,
                const char *tooltip, const char *cls);

/*
 * Show a tooltip for box after hovering for UI_TOOLTIP_DELAY.
 */
PUB void ui_tooltip(UI *ui, UIBox *box, const char *text);

/*
 * An editor tab with title, optional icon and close button. The close
 * button shows on the selected tab and on hover. The selected tab gets an
 * accent line on top. cls defaults to "tab".
 */
PUB UITabResult ui_tab(UI *ui, const char *id, const char *title, u16 icon,
                s32 selected, s32 closable, const char *cls);

/*
 * A drag handle between two areas. axis UI_AXIS_X resizes horizontally
 * (a vertical bar). It takes 1 px in the layout and a few more pixels of
 * the neighbors for grabbing; a 1 px line ("splitter-line") lights up on
 * hover. Returns how far it was dragged since last frame.
 */
PUB f32 ui_splitter(UI *ui, const char *id, s32 axis, const char *cls);

/*
 * A box that scrolls its children, with scrollbar thumbs that can be
 * dragged. Style it like any box (size, layout); overflow is set to scroll.
 * The thumbs use the class "<cls>-thumb" if the theme has it, else
 * "scrollbar-thumb"; a fixed width/height in that class sets the thickness.
 */
PUB UIBox *ui_scroll_begin(UI *ui, const char *id, const char *cls);
PUB void ui_scroll_end(UI *ui);

/*
 * Menus in the popup layer. Open one with ui_popup_open(ui, key, x, y),
 * then build it every frame:
 *
 *   if(ui_menu_begin(ui, key)) {
 *           if(ui_menu_item(ui, "Close", "Ctrl+W")) ...
 *           ui_menu_end(ui);
 *   }
 */
PUB s32 ui_menu_begin(UI *ui, u64 key);
PUB s32 ui_menu_item(UI *ui, const char *label, const char *shortcut);
PUB void ui_menu_separator(UI *ui);
PUB void ui_menu_end(UI *ui);

/*
 * A box with a check mark and a label. Toggles *value on click, or with
 * Space while focused. cls defaults to "checkbox"; the parts use
 * "checkbox-box" and "checkbox-label".
 *
 * @returns 1 if *value changed
 */
PUB s32 ui_checkbox(UI *ui, const char *label, s32 *value, const char *cls);

/*
 * One option of a radio group: sets *value = option when clicked. cls
 * defaults to "radio"; the parts use "radio-dot" and "radio-label".
 *
 * @returns 1 if *value changed
 */
PUB s32 ui_radio(UI *ui, const char *label, s32 *value, s32 option,
                const char *cls);

/*
 * Single-line text field that edits buf (UTF-8, cap bytes incl. the
 * terminator) in place. Mouse: click, drag and double-click to select.
 * Keys: arrows, Home/End, Backspace/Delete (with Ctrl by word), Shift to
 * select, Ctrl+A/C/X/V, Enter submits, Esc cancels. placeholder is shown
 * while buf is empty. cls defaults to "text-field"; the selection uses
 * "text-field-selection" and the placeholder "text-field-placeholder".
 */
PUB UITextFieldResult ui_text_field(UI *ui, const char *id, char *buf,
                s32 cap, const char *placeholder, const char *cls);

/*
 * Horizontal slider for *value in [min, max]. Drag or click the track; while
 * focused, Left/Right step by 1/100 (Shift: 1/10), Home/End jump. cls
 * defaults to "slider"; the parts use "slider-track", "slider-fill" and
 * "slider-thumb".
 *
 * @returns 1 if *value changed
 */
PUB s32 ui_slider(UI *ui, const char *id, f32 *value, f32 min, f32 max,
                const char *cls);

/*
 * Progress bar. value is 0..1; a negative value shows a moving bar for work
 * of unknown length (it draws continuously while shown). cls defaults to
 * "progress"; the bar uses "progress-fill".
 */
PUB void ui_progress(UI *ui, const char *id, f32 value, const char *cls);

/*
 * A button that shows items[*selected] and opens a list of the items in the
 * popup layer. Enter, Space or Down open it while focused. cls defaults to
 * "dropdown".
 *
 * @returns 1 if *selected changed
 */
PUB s32 ui_dropdown(UI *ui, const char *id, s32 *selected,
                const char *const *items, s32 count, const char *cls);

/*
 * A small pill with text, e.g. a count. cls defaults to "badge".
 */
PUB UIBox *ui_badge(UI *ui, const char *text, const char *cls);

/*
 * A row of a tree: indent by depth, a chevron if it has children, an icon
 * and a label. A click on a row with children flips *expanded (expanded may
 * be NULL for leaves). The caller keeps the tree state, so rows can be
 * virtualized. cls defaults to "tree-row"; the parts use "tree-chevron",
 * "tree-icon" and "tree-label".
 */
PUB UITreeRowResult ui_tree_row(UI *ui, const char *id, const char *label,
                u16 icon, s32 depth, s32 has_children, s32 *expanded,
                s32 selected, const char *cls);

/*
 * Virtualized list of count rows, row_height each:
 *
 *   UIList list = ui_list_begin(ui, "files", count, 22, NULL);
 *   for(i = list.first; i < list.last; i++) {
 *           ui_push_id(ui, (u64)i);
 *           ... build row i ...
 *           ui_pop_id(ui);
 *   }
 *   ui_list_end(ui, &list);
 *
 * Style the size of the list through its class (default "list") or
 * list.box->style.
 */
PUB UIList ui_list_begin(UI *ui, const char *id, s32 count, f32 row_height,
                const char *cls);
PUB void ui_list_end(UI *ui, UIList *list);

/*
 * Scroll so that row index is fully visible (from the next frame on).
 */
PUB void ui_list_scroll_to(UI *ui, UIList *list, s32 index);

/*
 * Queue a notification. It shows in the bottom right corner for
 * UI_TOAST_TIME seconds (longer while hovered) or until closed. The oldest
 * one is dropped when the queue is full.
 */
PUB void ui_toast(UI *ui, s32 kind, const char *text);

/*
 * Build the queued notifications. Call once per frame, anywhere. bottom is
 * the space to keep free at the bottom of the window (e.g. a status bar).
 */
PUB void ui_toasts(UI *ui, f32 bottom);

#endif /* VASALL_STUDIO_UI_WIDGETS_H */
