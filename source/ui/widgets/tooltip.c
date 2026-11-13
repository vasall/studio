#include <ui/widgets/tooltip.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void ui_tooltip(UI *ui, UIBox *box, const char *text)
{
        UIBox *tip;
        UIBoxState *st;
        f32 x;
        f32 y;

        if(!box || !box->key || ui->hot != box->key || ui->active) return;

        if(ui->time - ui->tooltip_start < UI_TOOLTIP_DELAY) {
                ui_request_wakeup(ui, ui->tooltip_start + UI_TOOLTIP_DELAY);
                return;
        }

        ui_layer_begin(ui, UI_LAYER_TOOLTIP);
        tip = ui_box(ui, "tooltip", "tooltip", UI_BOX_FLOATING);
        ui_box_set_text(ui, tip, text);

        /* Below the mouse, kept inside the window using last frame's size */
        x = ui->input->mouse_x;
        y = ui->input->mouse_y + 20.0f;
        st = tip->state;
        if(st) {
                if(x + st->rect.w > ui->width) x = ui->width - st->rect.w;
                if(y + st->rect.h > ui->height)
                        y = ui->input->mouse_y - st->rect.h - 4.0f;
        }
        tip->float_x = x < 0.0f ? 0.0f : x;
        tip->float_y = y < 0.0f ? 0.0f : y;
        ui_layer_end(ui);
}
