#include <ui/widgets/splitter.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB f32 ui_splitter(UI *ui, const char *id, s32 axis, const char *cls)
{
        UIBox *box;
        UIBox *line;
        UISignal sig;
        f32 drag;
        f32 delta = 0.0f;
        s32 lit;

        box = ui_box_begin(ui, id, cls ? cls : "splitter", UI_BOX_CLICKABLE);
        box->style.layout = UI_LAYOUT_STACK;
        box->style.align_x = UI_ALIGN_CENTER;
        box->style.align_y = UI_ALIGN_CENTER;
        if(axis == UI_AXIS_X) {
                box->style.width = ui_px(5.0f);
                box->style.height = ui_grow(1.0f);
                ui_sides4(box->style.margin, -2.0f, 0.0f, -2.0f, 0.0f);
                box->cursor = UI_CURSOR_RESIZE_EW;
        } else {
                box->style.width = ui_grow(1.0f);
                box->style.height = ui_px(5.0f);
                ui_sides4(box->style.margin, 0.0f, -2.0f, 0.0f, -2.0f);
                box->cursor = UI_CURSOR_RESIZE_NS;
        }

        lit = (ui->hot == box->key) || ui->active == box->key;
        line = ui_box(ui, NULL, "splitter-line", lit ? UI_BOX_SELECTED : 0);
        if(axis == UI_AXIS_X) {
                line->style.width = ui_px(lit ? 3.0f : 1.0f);
                line->style.height = ui_grow(1.0f);
        } else {
                line->style.width = ui_grow(1.0f);
                line->style.height = ui_px(lit ? 3.0f : 1.0f);
        }
        ui_box_end(ui); /* splitter */

        sig = ui_signal(ui, box);
        if(!box->state) return 0.0f;

        if(sig.pressed) box->state->user_f = 0.0f;
        if(ui->active == box->key) {
                drag = axis == UI_AXIS_X ? sig.drag_dx : sig.drag_dy;
                delta = drag - box->state->user_f;
                box->state->user_f = drag;
        }
        return delta;
}
