#ifndef VASALL_STUDIO_UI_WIDGETS_ICON_H
#define VASALL_STUDIO_UI_WIDGETS_ICON_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Icon without interaction. cls defaults to "icon".
 */
PUB UIBox *ui_icon(UI *ui, u16 icon, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_ICON_H */
