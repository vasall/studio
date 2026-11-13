#ifndef VASALL_STUDIO_UI_WIDGETS_SCROLL_H
#define VASALL_STUDIO_UI_WIDGETS_SCROLL_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * A box that scrolls its children, with scrollbar thumbs that can be
 * dragged. Style it like any box (size, layout); overflow is set to scroll.
 * The thumbs use the class "<cls>-thumb" if the theme has it, else
 * "scrollbar-thumb"; a fixed width/height in that class sets the thickness.
 */
PUB UIBox *ui_scroll_begin(UI *ui, const char *id, const char *cls);
PUB void ui_scroll_end(UI *ui);

#endif /* VASALL_STUDIO_UI_WIDGETS_SCROLL_H */
