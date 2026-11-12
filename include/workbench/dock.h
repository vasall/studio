#ifndef VASALL_STUDIO_DOCK_H
#define VASALL_STUDIO_DOCK_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/list.h>

#include <ui/ui.h>
#include <workbench/view.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef enum DockKind {
        DOCK_SPLIT = 0,         /* Children side by side */
        DOCK_TABS               /* A group of tabs */
} DockKind;

/* Where a dragged tab would land */
typedef enum DockZone {
        DOCK_ZONE_NONE = 0,
        DOCK_ZONE_CENTER,       /* Add as tab */
        DOCK_ZONE_LEFT,         /* Split, new group on that side */
        DOCK_ZONE_RIGHT,
        DOCK_ZONE_TOP,
        DOCK_ZONE_BOTTOM,
        DOCK_ZONE_TABBAR        /* Insert at a tab position */
} DockZone;

typedef struct DockNode {
        u8 kind;                /* DockKind */
        u8 axis;                /* Split: UI_AXIS_X = side by side */
        u32 id;                 /* For UI ids */
        struct DockNode *parent;

        List children;          /* Split: DockNode * */
        List ratios;            /* Split: f32 per child, sum = 1 */

        List views;             /* Tabs: ViewHandle */
        s32 active;             /* Tabs: index of the shown tab */

        /* Tabs: geometry from the last frame, for drop targets */
        UIRect tabbar_rect;
        UIRect content_rect;
        List tab_rects;         /* UIRect per tab */
        ViewHandle revealed;    /* Shown tab the tab bar last scrolled to */
} DockNode;

typedef struct DockDrag {
        s32 pending;            /* A tab is pressed */
        s32 active;             /* ... and moved past the threshold */
        ViewHandle view;
        DockNode *source;

        DockNode *target;       /* Drop target under the mouse */
        u8 zone;                /* DockZone */
        s32 index;              /* Tab bar insert position */
} DockDrag;

typedef struct Dock Dock;

/* Makes a copy of a view for "split editor" */
typedef ViewHandle (*DockCloneFn)(Dock *dock, ViewHandle view, void *user);

struct Dock {
        DockNode *root;
        DockNode *active_group;
        Views *views;
        DockDrag drag;
        u32 next_id;

        DockCloneFn clone;
        void *clone_user;

        /* Tab context menu */
        u64 menu_key;
        ViewHandle menu_view;
        DockNode *menu_node;
};

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Start with one empty tab group.
 */
PUB s32 dock_init(Dock *dock, Views *views, DockCloneFn clone,
                void *clone_user);

/*
 * Free the tree and destroy all views in it.
 */
PUB void dock_deinit(Dock *dock);

/*
 * Add a view as a tab to group (NULL = active group) and show it.
 */
PUB void dock_open(Dock *dock, DockNode *group, ViewHandle view);

/*
 * Close the tab at index in group and destroy its view.
 */
PUB void dock_close(Dock *dock, DockNode *group, s32 index);

/*
 * Close the shown tab of the active group.
 */
PUB void dock_close_active(Dock *dock);

/*
 * Put view into a new group on one side (zone LEFT/RIGHT/TOP/BOTTOM) of
 * group and make it active.
 */
PUB DockNode *dock_split(Dock *dock, DockNode *group, u8 zone,
                ViewHandle view);

/*
 * Split the active group, with a copy of its shown view.
 */
PUB void dock_split_active(Dock *dock, u8 zone);

/*
 * Move view from one group to another place (see DockZone). Empty groups
 * are removed afterwards.
 */
PUB void dock_move(Dock *dock, ViewHandle view, DockNode *from, DockNode *to,
                u8 zone, s32 index);

/*
 * Build the dock into the current parent (it fills it) and handle tab
 * clicks, drags, drops and splitters.
 */
PUB void dock_build(Dock *dock, UI *ui);

/*
 * The shown view of the active group (pool handle; invalid if none).
 */
PUB ViewHandle dock_active_view(Dock *dock);

/*
 * Number of tab groups.
 */
PUB s32 dock_group_count(Dock *dock);

#endif /* VASALL_STUDIO_DOCK_H */
