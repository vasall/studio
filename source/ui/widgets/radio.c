#include <ui/widgets/radio.h>

#include <render/icon_ids.h>
#include <ui/widgets/label.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 ui_radio(UI *ui, const char *label, s32 *value, s32 option,
                const char *cls)
{
        UIBox *row;
        UIBox *dot;
        UISignal sig;
        s32 on = value && *value == option;

        row = ui_box_begin(ui, label, cls ? cls : "radio",
                        UI_BOX_CLICKABLE | UI_BOX_FOCUSABLE |
                        (on ? UI_BOX_SELECTED : 0));
        row->cursor = UI_CURSOR_POINTER;
        dot = ui_box(ui, NULL, "radio-dot", on ? UI_BOX_SELECTED : 0);
        dot->icon = on ? ICON_DOT : ICON_NONE;
        ui_label(ui, label, "radio-label");
        ui_box_end(ui); /* radio */

        sig = ui_signal(ui, row);
        if(!value || on) return 0;
        if(sig.clicked || ui_take_key(ui, row->key, ' ', 0)) {
                *value = option;
                ui->changed = 1;
                return 1;
        }
        return 0;
}
