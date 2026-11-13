#ifndef VASALL_STUDIO_UI_WIDGETS_CHECKBOX_H
#define VASALL_STUDIO_UI_WIDGETS_CHECKBOX_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * A box with a check mark and a label. Toggles *value on click, or with
 * Space while focused. cls defaults to "checkbox"; the parts use
 * "checkbox-box" and "checkbox-label".
 *
 * @returns 1 if *value changed
 */
PUB s32 ui_checkbox(UI *ui, const char *label, s32 *value, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_CHECKBOX_H */
