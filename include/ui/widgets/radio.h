#ifndef VASALL_STUDIO_UI_WIDGETS_RADIO_H
#define VASALL_STUDIO_UI_WIDGETS_RADIO_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * One option of a radio group: sets *value = option when clicked. cls
 * defaults to "radio"; the parts use "radio-dot" and "radio-label".
 *
 * @returns 1 if *value changed
 */
PUB s32 ui_radio(UI *ui, const char *label, s32 *value, s32 option,
                const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_RADIO_H */
