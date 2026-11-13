#include <ui/widgets/toast.h>

#include <render/icon_ids.h>
#include <ui/widgets/button.h>
#include <ui/widgets/icon.h>
#include <ui/widgets/label.h>

#include <stdio.h>
#include <string.h>

#define TOAST_MARGIN       12.0f

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_toast_remove(UI *ui, s32 i);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void ui_toast(UI *ui, s32 kind, const char *text)
{
        UIToast *t;

        if(!text) return;
        if(ui->toast_count == UI_MAX_TOASTS) s_toast_remove(ui, 0);

        t = &ui->toasts[ui->toast_count++];
        memset(t, 0, sizeof(*t));
        t->id = ++ui->toast_next_id;
        t->kind = (u8)kind;
        t->start = -1.0;
        snprintf(t->text, sizeof(t->text), "%s", text);
        ui->changed = 1;
}

PUB void ui_toasts(UI *ui, f32 bottom)
{
        static const u16 icons[3] = {ICON_INFO, ICON_WARNING, ICON_ERROR};
        static const char *icon_cls[3] = {
                "toast-icon.info", "toast-icon.warning", "toast-icon.error"
        };
        UIBox *area;
        UIBox *toast;
        UIToast *t;
        u32 closed[UI_MAX_TOASTS];
        s32 n_closed = 0;
        s32 kind;
        s32 i;
        s32 j;

        /* Expire the old ones */
        for(i = 0; i < ui->toast_count;) {
                t = &ui->toasts[i];
                if(t->start < 0.0) t->start = ui->time;
                if(ui->time - t->start >= UI_TOAST_TIME) {
                        s_toast_remove(ui, i);
                        ui->changed = 1;
                } else {
                        i++;
                }
        }
        if(ui->toast_count == 0) return;

        /* A window-sized column that only aligns; it takes no input */
        ui_layer_begin(ui, UI_LAYER_OVERLAY);
        area = ui_box_begin(ui, "toasts", "toasts", 0);
        area->style.width = ui_grow(1.0f);
        area->style.height = ui_grow(1.0f);
        area->style.layout = UI_LAYOUT_COLUMN;
        area->style.align_x = UI_ALIGN_END;
        area->style.align_y = UI_ALIGN_END;
        ui_sides4(&area->style.padding, TOAST_MARGIN, TOAST_MARGIN,
                        TOAST_MARGIN, TOAST_MARGIN + bottom);

        for(i = 0; i < ui->toast_count; i++) {
                t = &ui->toasts[i];
                kind = t->kind <= UI_TOAST_ERROR ? t->kind : UI_TOAST_INFO;

                ui_push_id(ui, (u64)t->id);
                toast = ui_box_begin(ui, "toast", "toast", UI_BOX_CLICKABLE);
                ui_icon(ui, icons[kind], icon_cls[kind]);
                ui_label(ui, t->text, "toast-text");
                if(ui_icon_button(ui, "close", ICON_CLOSE, NULL,
                                        NULL).clicked)
                        closed[n_closed++] = t->id;
                ui_box_end(ui); /* toast */
                ui_pop_id(ui);

                /* Hovering keeps it */
                if(toast->state && ui->input && ui_rect_contains(
                                        toast->state->clip,
                                        ui->input->mouse_x,
                                        ui->input->mouse_y))
                        t->start = ui->time;
                ui_request_wakeup(ui, t->start + UI_TOAST_TIME);
        }
        ui_box_end(ui); /* toasts */
        ui_layer_end(ui);

        for(j = 0; j < n_closed; j++) {
                for(i = 0; i < ui->toast_count; i++) {
                        if(ui->toasts[i].id == closed[j]) {
                                s_toast_remove(ui, i);
                                ui->changed = 1;
                                break;
                        }
                }
        }
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC void s_toast_remove(UI *ui, s32 i)
{
        if(i < 0 || i >= ui->toast_count) return;
        memmove(&ui->toasts[i], &ui->toasts[i + 1],
                        sizeof(UIToast) * (size_t)(ui->toast_count - i - 1));
        ui->toast_count--;
}
