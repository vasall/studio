#include <ui/widgets/tree.h>

#include <render/icon_ids.h>
#include <ui/widgets/icon.h>
#include <ui/widgets/label.h>

#include <string.h>

#define TREE_INDENT        12.0f

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UITreeRowResult ui_tree_row(UI *ui, const char *id, const char *label,
                u16 icon, s32 depth, s32 has_children, s32 *expanded,
                s32 selected, const char *cls)
{
        UITreeRowResult res;
        UIBox *row;
        UIBox *chevron;
        s32 open = expanded && *expanded;

        memset(&res, 0, sizeof(res));

        row = ui_box_begin(ui, id, cls ? cls : "tree-row", UI_BOX_CLICKABLE |
                        (selected ? UI_BOX_SELECTED : 0));
        row->style.width = ui_grow(1.0f);
        row->style.padding[UI_LEFT] += (f32)(depth > 0 ? depth : 0) *
                TREE_INDENT;
        row->cursor = UI_CURSOR_POINTER;

        /* Leaves keep the space, so labels line up */
        chevron = ui_box(ui, NULL, "tree-chevron", 0);
        if(has_children)
                chevron->icon = open ? ICON_CHEVRON_DOWN : ICON_CHEVRON_RIGHT;
        if(icon) ui_icon(ui, icon, "tree-icon");
        ui_label(ui, label, selected ? "tree-label.selected" : "tree-label");
        ui_box_end(ui); /* tree-row */

        res.sig = ui_signal(ui, row);
        if(has_children && expanded && res.sig.clicked) {
                *expanded = !open;
                res.toggled = 1;
                ui->changed = 1;
        }
        return res;
}
