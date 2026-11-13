#ifndef VASALL_STUDIO_UI_WIDGETS_PROGRESS_H
#define VASALL_STUDIO_UI_WIDGETS_PROGRESS_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Progress bar. value is 0..1; a negative value shows a moving bar for work
 * of unknown length (it draws continuously while shown). cls defaults to
 * "progress"; the bar uses "progress-fill".
 */
PUB void ui_progress(UI *ui, const char *id, f32 value, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_PROGRESS_H */
