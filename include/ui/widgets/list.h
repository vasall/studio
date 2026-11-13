#ifndef VASALL_STUDIO_UI_WIDGETS_LIST_H
#define VASALL_STUDIO_UI_WIDGETS_LIST_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * A virtualized list: only the rows in view are built. Every row must be
 * exactly row_height tall.
 */
typedef struct UIList {
        UIBox *box;             /* The scroll area */
        s32 count;
        f32 row_height;
        s32 first;              /* Rows to build: first <= i < last */
        s32 last;
} UIList;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Virtualized list of count rows, row_height each:
 *
 *   UIList list = ui_list_begin(ui, "files", count, 22, NULL);
 *   for(i = list.first; i < list.last; i++) {
 *           ui_push_id(ui, (u64)i);
 *           ... build row i ...
 *           ui_pop_id(ui);
 *   }
 *   ui_list_end(ui, &list);
 *
 * Style the size of the list through its class (default "list") or
 * list.box->style.
 */
PUB UIList ui_list_begin(UI *ui, const char *id, s32 count, f32 row_height,
                const char *cls);
PUB void ui_list_end(UI *ui, UIList *list);

/*
 * Scroll so that row index is fully visible (from the next frame on).
 */
PUB void ui_list_scroll_to(UI *ui, UIList *list, s32 index);

#endif /* VASALL_STUDIO_UI_WIDGETS_LIST_H */
