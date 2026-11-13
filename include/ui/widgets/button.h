#ifndef VASALL_STUDIO_UI_WIDGETS_BUTTON_H
#define VASALL_STUDIO_UI_WIDGETS_BUTTON_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Text button. label is also the id ("Save##toolbar" shows "Save").
 * cls defaults to "button".
 */
PUB UISignal ui_button(UI *ui, const char *label, const char *cls);

/**
 * Icon button with an optional tooltip. cls defaults to "icon-button".
 */
PUB UISignal ui_icon_button(UI *ui, const char *id, u16 icon,
                const char *tooltip, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_BUTTON_H */
