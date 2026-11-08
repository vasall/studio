#ifndef VASALL_UI_ELEMENT_H
#define VASALL_UI_ELEMENT_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/list.h>
#include <shape/rectangle.h>

typedef struct UIElement UIElement;

struct UIElement {
        char name[128];

        UIElement *parent;
        List children;

        /* The bounding box for the element */
        Rectangle box;
};

/*
 *
*/
PUB s32 ui_elem_init(UIElement *ele, char *name);

/*
 *
*/
PUB void ui_elem_deinit(UIElement *ele);

/*
 * Attach the child element to the parent element.
*/
PUB s32 ui_elem_attach(UIElement *parent, UIElement *child);

/*
 * Detach the element from its parent element.
*/
PUB s32 ui_elem_detach(UIElement *ele);

#endif /* VASALL_UI_ELEMENT_H */