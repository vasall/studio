#ifndef VASALL_STUDIO_UI_WIDGETS_MENU_H
#define VASALL_STUDIO_UI_WIDGETS_MENU_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Menus in the popup layer. Open one with ui_popup_open(ui, key, x, y),
 * then build it every frame:
 *
 *   if(ui_menu_begin(ui, key)) {
 *           if(ui_menu_item(ui, "Close", "Ctrl+W")) ...
 *           ui_menu_end(ui);
 *   }
 */
PUB s32 ui_menu_begin(UI *ui, UIBoxKey key);
PUB s32 ui_menu_item(UI *ui, const char *label, const char *shortcut);
PUB void ui_menu_separator(UI *ui);
PUB void ui_menu_end(UI *ui);

/*
 * A menu item that can show as selected (e.g. the current choice of a
 * dropdown). id is hashed, label is shown; ui_menu_item(ui, l, s) is
 * ui_menu_item_ex(ui, l, l, s, 0).
 *
 * @returns 1 if it was clicked; the menu is closed then
 */
PUB s32 ui_menu_item_ex(UI *ui, const char *id, const char *label,
                const char *shortcut, s32 selected);

#endif /* VASALL_STUDIO_UI_WIDGETS_MENU_H */
