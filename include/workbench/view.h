#ifndef VASALL_STUDIO_VIEW_H
#define VASALL_STUDIO_VIEW_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/pool.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define VIEW_TITLE_LEN 64

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct View View;

/*
 * What a kind of view can do. Anything shown in an editor tab is a view.
 */
typedef struct ViewClass {
        const char *type;

        /* Build the content into area (the current parent) */
        void (*draw)(View *view, Ui *ui, UiBox *area);

        /* Optional */
        u16 (*icon)(View *view);
        s32 (*is_dirty)(View *view);
        void (*destroy)(View *view);
} ViewClass;

struct View {
        const ViewClass *cls;
        u32 id;                 /* Unique for the session, used for UI ids */
        char title[VIEW_TITLE_LEN];
        void *data;             /* Owned by the class */
};

/* Views are referenced by handle, so closed views never dangle */
typedef PoolHandle ViewHandle;

typedef struct Views {
        Pool pool;              /* View */
        u32 next_id;
} Views;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

PUB s32 views_init(Views *views);

/*
 * Destroy all views that are left.
 */
PUB void views_deinit(Views *views);

PUB ViewHandle view_create(Views *views, const ViewClass *cls,
                const char *title, void *data);

/*
 * The view, or NULL if it was destroyed.
 */
PUB View *view_get(Views *views, ViewHandle h);

PUB void view_destroy(Views *views, ViewHandle h);

PUB s32 view_handle_equal(ViewHandle a, ViewHandle b);

#endif /* VASALL_STUDIO_VIEW_H */
