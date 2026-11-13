#include <ui/widgets/tab.h>

#include <render/icon_ids.h>
#include <ui/widgets/icon.h>
#include <ui/widgets/label.h>

#include <string.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UITabResult ui_tab(UI *ui, const char *id, const char *title, u16 icon,
                s32 selected, s32 closable, const char *cls)
{
        UITabResult res;
        UIBox *tab;
        UIBox *row;
        UIBox *close;
        UIBox *accent;
        UISignal close_sig;
        s32 show_close;

        memset(&res, 0, sizeof(res));

        /* A stack: the content row, and the accent line on top of it */
        tab = ui_box_begin(ui, id, cls ? cls : "tab",
                        UI_BOX_CLICKABLE | (selected ? UI_BOX_SELECTED : 0));
        tab->style.layout = UI_LAYOUT_STACK;
        tab->style.align_y = UI_ALIGN_START;

        row = ui_box_begin(ui, NULL, "tab-row", 0);
        row->style.height = ui_grow(1.0f);
        if(icon) ui_icon(ui, icon, "tab-icon");
        ui_label(ui, title, selected ? "tab-label.selected" : "tab-label");

        if(closable) {
                show_close = selected || ui->hot == tab->key;
                close = ui_box(ui, "close", "tab-close", UI_BOX_CLICKABLE);
                close->cursor = UI_CURSOR_POINTER;
                show_close = show_close || ui->hot == close->key;
                close->icon = show_close ? ICON_CLOSE : ICON_NONE;
                close_sig = ui_signal(ui, close);
                res.close_clicked = close_sig.clicked;
        }
        ui_box_end(ui); /* tab-row */

        if(selected) {
                accent = ui_box(ui, NULL, "tab-accent", 0);
                accent->style.width = ui_grow(1.0f);
        }
        ui_box_end(ui); /* tab */

        res.sig = ui_signal(ui, tab);
        res.key = tab->key;
        if(res.sig.middle_clicked && closable) res.close_clicked = 1;
        return res;
}
