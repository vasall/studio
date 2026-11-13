#ifndef VASALL_STUDIO_UI_WIDGETS_SLIDER_H
#define VASALL_STUDIO_UI_WIDGETS_SLIDER_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Horizontal slider for *value in [min, max]. Drag or click the track; while
 * focused, Left/Right step by 1/100 (Shift: 1/10), Home/End jump. cls
 * defaults to "slider"; the parts use "slider-track", "slider-fill" and
 * "slider-thumb".
 *
 * @returns 1 if *value changed
 */
PUB s32 ui_slider(UI *ui, const char *id, f32 *value, f32 min, f32 max,
                const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_SLIDER_H */
