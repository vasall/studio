#include <ui/widgets/dropdown.h>

#include <render/icon_ids.h>
#include <ui/widgets/icon.h>
#include <ui/widgets/label.h>
#include <ui/widgets/menu.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 ui_dropdown(UI *ui, const char *id, s32 *selected,
                const char *const *items, s32 count, const char *cls)
{
        UIBox *box;
        UIBox *label;
        UIBox *menu;
        UISignal sig;
        UIBoxKey key = ui_key(ui, id);
        s32 open = ui_popup_is_open(ui, key);
        s32 cur = *selected;
        s32 toggle = 0;
        s32 i;

        box = ui_box_begin(ui, id, cls ? cls : "dropdown",
                        UI_BOX_CLICKABLE | UI_BOX_FOCUSABLE |
                        (open ? UI_BOX_SELECTED : 0));
        box->cursor = UI_CURSOR_POINTER;
        label = ui_label(ui, cur >= 0 && cur < count ? items[cur] : "",
                        "dropdown-label");
        label->style.width = ui_grow(1.0f);
        ui_icon(ui, ICON_CHEVRON_DOWN, "dropdown-chevron");
        ui_box_end(ui); /* dropdown */

        sig = ui_signal(ui, box);
        if(sig.pressed) toggle = 1;

        if(open) {
                /* Arrows pick right away, like a native select */
                while(ui_take_key(ui, box->key, UI_KEY_UP, 0)) {
                        if(*selected > 0) (*selected)--;
                }
                while(ui_take_key(ui, box->key, UI_KEY_DOWN, 0)) {
                        if(*selected < count - 1) (*selected)++;
                }
                if(ui_take_key(ui, box->key, UI_KEY_ENTER, 0) ||
                   ui_take_key(ui, box->key, ' ', 0))
                        toggle = 1;
        } else if(ui_take_key(ui, box->key, UI_KEY_ENTER, 0) ||
                  ui_take_key(ui, box->key, ' ', 0) ||
                  ui_take_key(ui, box->key, UI_KEY_DOWN, 0)) {
                toggle = 1;
        }

        if(toggle && box->state) {
                if(open) {
                        ui_popup_close(ui);
                } else {
                        ui_popup_open(ui, key, box->state->rect.x,
                                        box->state->rect.y +
                                        box->state->rect.h + 2.0f);
                }
        }

        if(ui_menu_begin(ui, key)) {
                menu = ui_parent(ui);
                if(box->state) menu->style.width.min = box->state->rect.w;
                for(i = 0; i < count; i++) {
                        ui_push_id(ui, (u64)i);
                        if(ui_menu_item_ex(ui, "item", items[i], NULL,
                                                i == *selected)) {
                                *selected = i;
                        }
                        ui_pop_id(ui);
                }
                ui_menu_end(ui);
        }

        if(*selected != cur) {
                ui->changed = 1;
                return 1;
        }
        return 0;
}
