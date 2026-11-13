#ifndef VASALL_STUDIO_UI_WIDGETS_VIEWPORT_H
#define VASALL_STUDIO_UI_WIDGETS_VIEWPORT_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <render/camera.h>
#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * A 3D view: a box that moves cam with the mouse and leaves the drawing to
 * draw (see UIDrawCallbackFn), which gets the box's content area as its
 * viewport. Usually called through scene_viewport (render/scene.h), which
 * draws a Scene3D.
 *
 * Mouse: left drag orbits, Shift + left drag or middle drag pans, the wheel
 * zooms. Size and background come from the class (default "viewport").
 *
 * @returns The signal of the box
 */
PUB UISignal ui_viewport(UI *ui, const char *id, Camera3D *cam,
                UIDrawCallbackFn draw, void *data, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_VIEWPORT_H */
