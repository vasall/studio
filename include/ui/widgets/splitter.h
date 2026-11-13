#ifndef VASALL_STUDIO_UI_WIDGETS_SPLITTER_H
#define VASALL_STUDIO_UI_WIDGETS_SPLITTER_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * A drag handle between two areas. axis UI_AXIS_X resizes horizontally
 * (a vertical bar). It takes 1 px in the layout and a few more pixels of
 * the neighbors for grabbing; a 1 px line ("splitter-line") lights up on
 * hover. Returns how far it was dragged since last frame.
 */
PUB f32 ui_splitter(UI *ui, const char *id, s32 axis, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_SPLITTER_H */
