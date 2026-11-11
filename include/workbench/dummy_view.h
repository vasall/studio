#ifndef VASALL_STUDIO_DUMMY_VIEW_H
#define VASALL_STUDIO_DUMMY_VIEW_H

#include <workbench/view.h>

/*
 * A placeholder editor: an empty box with its name and a colored stripe,
 * so tabs can be told apart while moving them around.
 */
extern const ViewClass dummy_view_class;

PUB ViewHandle dummy_view_create(Views *views, const char *title);

#endif /* VASALL_STUDIO_DUMMY_VIEW_H */
