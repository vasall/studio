#ifndef VASALL_STUDIO_UI_WIDGETS_SPACER_H
#define VASALL_STUDIO_UI_WIDGETS_SPACER_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Empty box that takes space, e.g. ui_spacer(ui, ui_grow(1), ui_px(0)).
 */
PUB UIBox *ui_spacer(UI *ui, UISize w, UISize h);

#endif /* VASALL_STUDIO_UI_WIDGETS_SPACER_H */
