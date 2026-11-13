#include <ui/widgets/scroll.h>

#define SCROLLBAR_SIZE     10.0f
#define SCROLLBAR_MIN_LEN  24.0f

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_scrollbar(UI *ui, UIBox *area, s32 a);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UIBox *ui_scroll_begin(UI *ui, const char *id, const char *cls)
{
        UIBox *box = ui_box_begin(ui, id, cls, 0);

        box->style.overflow = UI_OVERFLOW_SCROLL;
        return box;
}

PUB void ui_scroll_end(UI *ui)
{
        UIBox *area = ui_parent(ui);

        if(area && area->state) {
                s_scrollbar(ui, area, UI_AXIS_X);
                s_scrollbar(ui, area, UI_AXIS_Y);
        }
        ui_box_end(ui); /* scroll area of ui_scroll_begin */
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC void s_scrollbar(UI *ui, UIBox *area, s32 a)
{
        UIBoxState *st = area->state;
        UIBox *thumb;
        UISignal sig;
        f32 view = st->view[a];
        f32 content = st->content[a];
        f32 track;
        f32 len;
        f32 range;
        f32 ratio;
        f32 pos;
        f32 drag;
        f32 size = SCROLLBAR_SIZE;
        const char *cls = "scrollbar-thumb";
        const char *own;
        const UISize *thick;
        UIStyle style;

        if(content <= view + 0.5f || view <= 0.0f) return;

        /* The area's own thumb class, if the theme has one */
        if(area->cls) {
                own = arena_printf(&ui->arena, "%s-thumb", area->cls);
                if(own && ui_theme_class(ui->theme, own)) cls = own;
        }
        style = ui_theme_style(ui->theme, cls, 0);
        thick = a == UI_AXIS_X ? &style.height : &style.width;
        if(thick->kind == UI_SIZE_PIXELS) size = thick->value;

        track = a == UI_AXIS_X ? st->rect.w : st->rect.h;
        len = track * view / content;
        if(len < SCROLLBAR_MIN_LEN) len = SCROLLBAR_MIN_LEN;
        if(len > track) len = track;
        range = content - view;

        /* The offset may be past the end until the next layout clamps it */
        ratio = st->scroll[a] / range;
        if(ratio < 0.0f) ratio = 0.0f;
        if(ratio > 1.0f) ratio = 1.0f;
        pos = (track - len) * ratio;

        thumb = ui_box(ui, a == UI_AXIS_X ? "scroll-x" : "scroll-y",
                        cls, UI_BOX_FLOATING | UI_BOX_CLICKABLE);
        if(a == UI_AXIS_X) {
                thumb->style.width = ui_px(len);
                thumb->style.height = ui_px(size);
                thumb->float_x = st->rect.x + pos;
                thumb->float_y = st->rect.y + st->rect.h - size;
        } else {
                thumb->style.width = ui_px(size);
                thumb->style.height = ui_px(len);
                thumb->float_x = st->rect.x + st->rect.w - size;
                thumb->float_y = st->rect.y + pos;
        }

        sig = ui_signal(ui, thumb);
        if(!thumb->state) return;

        /* Remember the scroll offset at the press, then follow the mouse */
        if(sig.pressed) thumb->state->user_f = st->scroll[a];
        if(ui->active == thumb->key && track > len) {
                drag = a == UI_AXIS_X ? sig.drag_dx : sig.drag_dy;
                st->scroll[a] = thumb->state->user_f + drag * range /
                        (track - len);
                if(st->scroll[a] < 0.0f) st->scroll[a] = 0.0f;
                if(st->scroll[a] > range) st->scroll[a] = range;
                ui->changed = 1;
        }
}
