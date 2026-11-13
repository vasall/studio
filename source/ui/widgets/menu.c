#include <ui/widgets/menu.h>

#include <ui/widgets/label.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 ui_menu_begin(UI *ui, UIBoxKey key)
{
        UIBox *menu;
        f32 x;
        f32 y;

        if(!ui_popup_is_open(ui, key)) return 0;

        ui_layer_begin(ui, UI_LAYER_POPUP);
        ui_push_id(ui, key);
        menu = ui_box_begin(ui, "menu", "menu", UI_BOX_FLOATING |
                        UI_BOX_CLICKABLE);

        /* Keep it inside the window, using last frame's size */
        x = ui->popup_x;
        y = ui->popup_y;
        if(menu->state && menu->state->rect.w > 0.0f) {
                if(x + menu->state->rect.w > ui->width)
                        x = ui->width - menu->state->rect.w;
                if(y + menu->state->rect.h > ui->height)
                        y = ui->height - menu->state->rect.h;
        }
        menu->float_x = x < 0.0f ? 0.0f : x;
        menu->float_y = y < 0.0f ? 0.0f : y;
        return 1;
}

PUB s32 ui_menu_item(UI *ui, const char *label, const char *shortcut)
{
        return ui_menu_item_ex(ui, label, label, shortcut, 0);
}

PUB s32 ui_menu_item_ex(UI *ui, const char *id, const char *label,
                const char *shortcut, s32 selected)
{
        UIBox *item;
        UIBox *text;
        UISignal sig;

        item = ui_box_begin(ui, id, "menu-item", UI_BOX_CLICKABLE |
                        (selected ? UI_BOX_SELECTED : 0));
        text = ui_label(ui, label, "menu-item-label");
        text->style.width = ui_grow(1.0f);
        if(shortcut) ui_label(ui, shortcut, "menu-item-shortcut");
        ui_box_end(ui); /* menu-item */

        sig = ui_signal(ui, item);
        if(sig.clicked) {
                ui_popup_close(ui);
                return 1;
        }
        return 0;
}

PUB void ui_menu_separator(UI *ui)
{
        ui_box(ui, NULL, "menu-separator", 0);
}

PUB void ui_menu_end(UI *ui)
{
        ui_box_end(ui); /* menu of ui_menu_begin */
        ui_pop_id(ui);
        ui_layer_end(ui);
}
