#ifndef VASALL_STUDIO_UI_WIDGETS_LABEL_H
#define VASALL_STUDIO_UI_WIDGETS_LABEL_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Text without interaction. cls defaults to "label".
 */
PUB UIBox *ui_label(UI *ui, const char *text, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_LABEL_H */
