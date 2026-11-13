#include <ui/widgets/checkbox.h>

#include <render/icon_ids.h>
#include <ui/widgets/label.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 ui_checkbox(UI *ui, const char *label, s32 *value, const char *cls)
{
        UIBox *row;
        UIBox *mark;
        UISignal sig;
        s32 on = value && *value;

        row = ui_box_begin(ui, label, cls ? cls : "checkbox",
                        UI_BOX_CLICKABLE | UI_BOX_FOCUSABLE |
                        (on ? UI_BOX_SELECTED : 0));
        row->cursor = UI_CURSOR_POINTER;
        mark = ui_box(ui, NULL, "checkbox-box", on ? UI_BOX_SELECTED : 0);
        mark->icon = on ? ICON_CHECK : ICON_NONE;
        ui_label(ui, label, "checkbox-label");
        ui_box_end(ui); /* checkbox */

        sig = ui_signal(ui, row);
        if(!value) return 0;
        if(sig.clicked || ui_take_key(ui, row->key, ' ', 0)) {
                *value = !on;
                ui->changed = 1;
                return 1;
        }
        return 0;
}
