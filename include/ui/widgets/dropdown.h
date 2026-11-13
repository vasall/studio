#ifndef VASALL_STUDIO_UI_WIDGETS_DROPDOWN_H
#define VASALL_STUDIO_UI_WIDGETS_DROPDOWN_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * A button that shows items[*selected] and opens a list of the items in the
 * popup layer. Enter, Space or Down open it while focused. cls defaults to
 * "dropdown".
 *
 * @returns 1 if *selected changed
 */
PUB s32 ui_dropdown(UI *ui, const char *id, s32 *selected,
                const char *const *items, s32 count, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_DROPDOWN_H */
