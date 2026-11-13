#include <ui/widgets/list.h>

#include <ui/widgets/scroll.h>
#include <ui/widgets/spacer.h>

#include <math.h>
#include <string.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UIList ui_list_begin(UI *ui, const char *id, s32 count, f32 row_height,
                const char *cls)
{
        UIList list;
        UIBoxState *st;
        f32 scroll = 0.0f;
        f32 view;
        f32 max;

        memset(&list, 0, sizeof(list));
        if(count < 0) count = 0;
        if(row_height < 1.0f) row_height = 1.0f;

        list.box = ui_scroll_begin(ui, id, cls ? cls : "list");
        list.box->style.layout = UI_LAYOUT_COLUMN;
        list.box->style.gap = 0.0f;
        list.count = count;
        list.row_height = row_height;

        /* Visible range from last frame; the first frame assumes a full
           window so it doesn't come up empty */
        st = list.box->state;
        view = st && st->view[1] > 0.0f ? st->view[1] : ui->height;
        if(st) scroll = st->scroll[1];
        max = (f32)count * row_height - view;
        if(scroll > max) scroll = max;
        if(scroll < 0.0f) scroll = 0.0f;

        list.first = (s32)(scroll / row_height);
        list.last = (s32)ceilf((scroll + view) / row_height);
        if(list.first > count) list.first = count;
        if(list.last > count) list.last = count;

        if(list.first > 0)
                ui_spacer(ui, ui_px(0.0f), ui_px((f32)list.first * row_height));
        return list;
}

PUB void ui_list_end(UI *ui, UIList *list)
{
        if(list->last < list->count) {
                ui_spacer(ui, ui_px(0.0f), ui_px((f32)(list->count -
                                                list->last) *
                                        list->row_height));
        }
        ui_scroll_end(ui);
}

PUB void ui_list_scroll_to(UI *ui, UIList *list, s32 index)
{
        UIBoxState *st = list->box ? list->box->state : NULL;
        f32 top;
        f32 bottom;

        if(!st || index < 0 || index >= list->count) return;

        top = (f32)index * list->row_height;
        bottom = top + list->row_height;
        if(top < st->scroll[1]) {
                st->scroll[1] = top;
        } else if(bottom > st->scroll[1] + st->view[1]) {
                st->scroll[1] = bottom - st->view[1];
        } else {
                return;
        }
        ui->changed = 1;
}
