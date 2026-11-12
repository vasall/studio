#include <ui/ui.h>

#include <math.h>
#include <string.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UISignal ui_signal(UI *ui, UIBox *box)
{
        UISignal sig;
        UIInput *in = ui->input;
        UIBoxState *st;
        f32 dx;
        f32 dy;

        memset(&sig, 0, sizeof(sig));
        if(!box || !box->state || !in || (box->flags & UI_BOX_DISABLED))
                return sig;

        st = box->state;
        sig.mouse_x = in->mouse_x - st->rect.x;
        sig.mouse_y = in->mouse_y - st->rect.y;
        sig.hovering = ui->hot == box->key;

        if(sig.hovering) {
                if(in->button_pressed[UI_MOUSE_LEFT] && ui->active == 0) {
                        ui->active = box->key;
                        ui->dragging = 0;
                        ui->press_x = in->mouse_x;
                        ui->press_y = in->mouse_y;
                        sig.pressed = 1;
                        sig.double_clicked =
                                in->click_count[UI_MOUSE_LEFT] >= 2;
                        if(box->flags & UI_BOX_FOCUSABLE)
                                ui_set_focus(ui, box->key);
                        ui->changed = 1;
                }
                if(in->button_pressed[UI_MOUSE_RIGHT]) sig.right_clicked = 1;
                if(in->button_pressed[UI_MOUSE_MIDDLE]) sig.middle_clicked = 1;
        }

        if(ui->active == box->key) {
                dx = in->mouse_x - ui->press_x;
                dy = in->mouse_y - ui->press_y;
                if(!ui->dragging && (fabsf(dx) > UI_DRAG_THRESHOLD ||
                                     fabsf(dy) > UI_DRAG_THRESHOLD))
                        ui->dragging = 1;

                sig.dragging = ui->dragging;
                sig.drag_dx = dx;
                sig.drag_dy = dy;

                if(in->button_released[UI_MOUSE_LEFT]) {
                        sig.released = 1;
                        sig.clicked = !ui->dragging &&
                                ui_rect_contains(st->clip, in->mouse_x,
                                                in->mouse_y);
                }
        }
        return sig;
}
