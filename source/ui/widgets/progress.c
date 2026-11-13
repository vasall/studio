#include <ui/widgets/progress.h>

#include <devkit/util/helper.h>

#include <math.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void ui_progress(UI *ui, const char *id, f32 value, const char *cls)
{
        UIBox *box;
        UIBox *fill;
        f32 w;
        f32 len;
        f32 phase;

        box = ui_box_begin(ui, id, cls ? cls : "progress", 0);
        box->style.layout = UI_LAYOUT_ROW;
        box->style.overflow = UI_OVERFLOW_CLIP;

        fill = ui_box(ui, NULL, "progress-fill", 0);
        fill->style.height = ui_grow(1.0f);
        if(value >= 0.0f) {
                fill->style.width = ui_pct(CLAMP(value, 0.0f, 1.0f));
        } else {
                /* A third of the bar sweeping across, every 1.5 s */
                w = box->state ? box->state->rect.w : 0.0f;
                len = w * 0.3f;
                phase = (f32)fmod(ui->time, 1.5) / 1.5f;
                fill->style.width = ui_px(len);
                fill->style.margin[UI_LEFT] = phase * (w + len) - len;
                ui->changed = 1;
        }
        ui_box_end(ui); /* progress */
}
