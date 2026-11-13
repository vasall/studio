#ifndef VASALL_STUDIO_UI_WIDGETS_TOOLTIP_H
#define VASALL_STUDIO_UI_WIDGETS_TOOLTIP_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Show a tooltip for box after hovering for UI_TOOLTIP_DELAY.
 */
PUB void ui_tooltip(UI *ui, UIBox *box, const char *text);

#endif /* VASALL_STUDIO_UI_WIDGETS_TOOLTIP_H */
