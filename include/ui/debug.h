#ifndef VASALL_STUDIO_UI_DEBUG_H
#define VASALL_STUDIO_UI_DEBUG_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/*
 * Build the debug info panel (bottom right) in the debug layer. info is
 * extra text from the app, e.g. frame time and draw calls. Only does
 * something while ui->debug is set.
 */
PUB void ui_debug_panel(UI *ui, const char *info);

/*
 * Add outline draw commands for every box (called by ui_end_frame while
 * ui->debug is set).
 */
PUB void ui_debug_outlines(UI *ui);

#endif /* VASALL_STUDIO_UI_DEBUG_H */
