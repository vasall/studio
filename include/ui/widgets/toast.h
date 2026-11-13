#ifndef VASALL_STUDIO_UI_WIDGETS_TOAST_H
#define VASALL_STUDIO_UI_WIDGETS_TOAST_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Queue a notification. It shows in the bottom right corner for
 * UI_TOAST_TIME seconds (longer while hovered) or until closed. The oldest
 * one is dropped when the queue is full.
 */
PUB void ui_toast(UI *ui, s32 kind, const char *text);

/*
 * Build the queued notifications. Call once per frame, anywhere. bottom is
 * the space to keep free at the bottom of the window (e.g. a status bar).
 */
PUB void ui_toasts(UI *ui, f32 bottom);

#endif /* VASALL_STUDIO_UI_WIDGETS_TOAST_H */
