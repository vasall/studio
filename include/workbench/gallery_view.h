#ifndef VASALL_STUDIO_GALLERY_VIEW_H
#define VASALL_STUDIO_GALLERY_VIEW_H

#include <workbench/view.h>

/**
 * A development view that shows every widget with some state to play with:
 * buttons, toggles, text fields, sliders, dropdowns, badges, a tree and a
 * virtualized list of 10 000 rows.
 */
extern const ViewClass gallery_view_class;

/**
 * @returns The view, or an invalid handle on error
 */
PUB ViewHandle gallery_view_create(Views *views);

#endif /* VASALL_STUDIO_GALLERY_VIEW_H */
