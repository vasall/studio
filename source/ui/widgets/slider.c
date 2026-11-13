#include <ui/widgets/slider.h>

#include <devkit/util/helper.h>

#include <stddef.h>

#define SLIDER_THUMB       14.0f

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 ui_slider(UI *ui, const char *id, f32 *value, f32 min, f32 max,
                const char *cls)
{
        UIBox *box;
        UIBox *track;
        UIBox *fill;
        UIBox *thumb;
        UISignal sig;
        UIStyle ts;
        f32 thumb_w = SLIDER_THUMB;
        f32 range = max - min;
        f32 old = *value;
        f32 step;
        f32 w = 0.0f;
        f32 t;
        s32 lit;
        s32 shift;

        box = ui_box_begin(ui, id, cls ? cls : "slider",
                        UI_BOX_CLICKABLE | UI_BOX_FOCUSABLE);
        box->style.layout = UI_LAYOUT_STACK;
        box->style.align_x = UI_ALIGN_START;
        box->style.align_y = UI_ALIGN_CENTER;
        box->cursor = UI_CURSOR_POINTER;
        sig = ui_signal(ui, box);

        ts = ui_theme_style(ui->theme, "slider-thumb", 0);
        if(ts.width.kind == UI_SIZE_PIXELS) thumb_w = ts.width.value;

        /* Travel of the thumb's left edge, from last frame's rect */
        if(box->state) {
                w = box->state->rect.w - box->style.padding[UI_LEFT] -
                        box->style.padding[UI_RIGHT] - thumb_w;
                if(box->state->rect.w <= 0.0f) ui->changed = 1;
        }
        if(w < 0.0f) w = 0.0f;

        /* Mouse: the thumb's center follows the pointer */
        if(ui->active == box->key && ui->input->button_down[UI_MOUSE_LEFT] &&
           w > 0.0f) {
                t = (sig.mouse_x - box->style.padding[UI_LEFT] -
                     thumb_w * 0.5f) / w;
                *value = min + CLAMP(t, 0.0f, 1.0f) * range;
        }

        /* Keys */
        for(shift = 0; shift < 2; shift++) {
                u16 mods = shift ? UI_MOD_SHIFT : 0;

                step = range / (shift ? 10.0f : 100.0f);
                while(ui_take_key(ui, box->key, UI_KEY_LEFT, mods) ||
                      ui_take_key(ui, box->key, UI_KEY_DOWN, mods))
                        *value -= step;
                while(ui_take_key(ui, box->key, UI_KEY_RIGHT, mods) ||
                      ui_take_key(ui, box->key, UI_KEY_UP, mods))
                        *value += step;
        }
        if(ui_take_key(ui, box->key, UI_KEY_HOME, 0)) *value = min;
        if(ui_take_key(ui, box->key, UI_KEY_END, 0)) *value = max;

        if(range > 0.0f) {
                *value = CLAMP(*value, min, max);
        } else {
                *value = min;
        }
        t = range > 0.0f ? (*value - min) / range : 0.0f;

        track = ui_box(ui, NULL, "slider-track", 0);
        track->style.width = ui_grow(1.0f);
        ui_sides4(track->style.margin, thumb_w * 0.5f, 0.0f,
                        thumb_w * 0.5f, 0.0f);

        fill = ui_box(ui, NULL, "slider-fill", 0);
        fill->style.width = ui_px(t * w);
        fill->style.margin[UI_LEFT] = thumb_w * 0.5f;

        lit = ui->hot == box->key || ui->active == box->key ||
                ui->focus == box->key;
        thumb = ui_box(ui, NULL, "slider-thumb", lit ? UI_BOX_SELECTED : 0);
        thumb->style.width = ui_px(thumb_w);
        thumb->style.margin[UI_LEFT] = t * w;
        ui_box_end(ui); /* slider */

        if(*value != old) {
                ui->changed = 1;
                return 1;
        }
        return 0;
}
