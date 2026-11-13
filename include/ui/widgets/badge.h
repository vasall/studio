#ifndef VASALL_STUDIO_UI_WIDGETS_BADGE_H
#define VASALL_STUDIO_UI_WIDGETS_BADGE_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * A small pill with text, e.g. a count. cls defaults to "badge".
 */
PUB UIBox *ui_badge(UI *ui, const char *text, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_BADGE_H */
