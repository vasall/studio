#ifndef VASALL_STUDIO_UI_WIDGETS_TREE_H
#define VASALL_STUDIO_UI_WIDGETS_TREE_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct UITreeRowResult {
        UISignal sig;           /* Of the whole row */
        s32 toggled;            /* *expanded was flipped */
} UITreeRowResult;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * A row of a tree: indent by depth, a chevron if it has children, an icon
 * and a label. A click on a row with children flips *expanded (expanded may
 * be NULL for leaves). The caller keeps the tree state, so rows can be
 * virtualized. cls defaults to "tree-row"; the parts use "tree-chevron",
 * "tree-icon" and "tree-label".
 */
PUB UITreeRowResult ui_tree_row(UI *ui, const char *id, const char *label,
                u16 icon, s32 depth, s32 has_children, s32 *expanded,
                s32 selected, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_TREE_H */
