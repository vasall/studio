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

typedef struct UiTabResult {
        UiSignal sig;           /* Of the tab itself */
        u64 key;                /* Key of the tab box (for its state/rect) */
        s32 close_clicked;
} UiTabResult;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Empty box that takes space, e.g. ui_spacer(ui, ui_grow(1), ui_px(0)).
 */
PUB UiBox *ui_spacer(Ui *ui, UiSize w, UiSize h);

/*
 * Text without interaction. cls defaults to "label".
 */
PUB UiBox *ui_label(Ui *ui, const char *text, const char *cls);

/*
 * Icon without interaction. cls defaults to "icon".
 */
PUB UiBox *ui_icon(Ui *ui, u16 icon, const char *cls);

/*
 * Text button. label is also the id ("Save##toolbar" shows "Save").
 * cls defaults to "button".
 */
PUB UiSignal ui_button(Ui *ui, const char *label, const char *cls);

/*
 * Icon button with an optional tooltip. cls defaults to "icon-button".
 */
PUB UiSignal ui_icon_button(Ui *ui, const char *id, u16 icon,
                const char *tooltip, const char *cls);

/*
 * Show a tooltip for box after hovering for UI_TOOLTIP_DELAY.
 */
PUB void ui_tooltip(Ui *ui, UiBox *box, const char *text);

/*
 * An editor tab with title, optional icon and close button. The close
 * button shows on the selected tab and on hover. The selected tab gets an
 * accent line on top. cls defaults to "tab".
 */
PUB UiTabResult ui_tab(Ui *ui, const char *id, const char *title, u16 icon,
                s32 selected, s32 closable, const char *cls);

/*
 * A drag handle between two areas. axis UI_AXIS_X resizes horizontally
 * (a vertical bar). It takes 1 px in the layout and a few more pixels of
 * the neighbors for grabbing; a 1 px line ("splitter-line") lights up on
 * hover. Returns how far it was dragged since last frame.
 */
PUB f32 ui_splitter(Ui *ui, const char *id, s32 axis, const char *cls);

/*
 * A box that scrolls its children, with scrollbar thumbs that can be
 * dragged. Style it like any box (size, layout); overflow is set to scroll.
 * The thumbs use the class "<cls>-thumb" if the theme has it, else
 * "scrollbar-thumb"; a fixed width/height in that class sets the thickness.
 */
PUB UiBox *ui_scroll_begin(Ui *ui, const char *id, const char *cls);
PUB void ui_scroll_end(Ui *ui);

/*
 * Menus in the popup layer. Open one with ui_popup_open(ui, key, x, y),
 * then build it every frame:
 *
 *   if(ui_menu_begin(ui, key)) {
 *           if(ui_menu_item(ui, "Close", "Ctrl+W")) ...
 *           ui_menu_end(ui);
 *   }
 */
PUB s32 ui_menu_begin(Ui *ui, u64 key);
PUB s32 ui_menu_item(Ui *ui, const char *label, const char *shortcut);
PUB void ui_menu_separator(Ui *ui);
PUB void ui_menu_end(Ui *ui);

#endif /* VASALL_STUDIO_UI_WIDGETS_H */
