#include <ui/widgets.h>

#include <render/icon_ids.h>

#include <string.h>

#define SCROLLBAR_SIZE     10.0f
#define SCROLLBAR_MIN_LEN  24.0f

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_scrollbar(Ui *ui, UiBox *area, s32 a);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UiBox *ui_spacer(Ui *ui, UiSize w, UiSize h)
{
        UiBox *box = ui_box(ui, NULL, NULL, 0);

        box->style.width = w;
        box->style.height = h;
        return box;
}

PUB UiBox *ui_label(Ui *ui, const char *text, const char *cls)
{
        UiBox *box = ui_box(ui, NULL, cls ? cls : "label", 0);

        ui_box_set_text(ui, box, text);
        return box;
}

PUB UiBox *ui_icon(Ui *ui, u16 icon, const char *cls)
{
        UiBox *box = ui_box(ui, NULL, cls ? cls : "icon", 0);

        box->icon = icon;
        return box;
}

PUB UiSignal ui_button(Ui *ui, const char *label, const char *cls)
{
        UiBox *box = ui_box(ui, label, cls ? cls : "button", UI_BOX_CLICKABLE);

        box->cursor = UI_CURSOR_POINTER;
        ui_box_set_text(ui, box, label);
        return ui_signal(ui, box);
}

PUB UiSignal ui_icon_button(Ui *ui, const char *id, u16 icon,
                const char *tooltip, const char *cls)
{
        UiBox *box = ui_box(ui, id, cls ? cls : "icon-button",
                        UI_BOX_CLICKABLE);

        box->icon = icon;
        box->cursor = UI_CURSOR_POINTER;
        if(tooltip) ui_tooltip(ui, box, tooltip);
        return ui_signal(ui, box);
}

PUB void ui_tooltip(Ui *ui, UiBox *box, const char *text)
{
        UiBox *tip;
        UiBoxState *st;
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

PUB UiTabResult ui_tab(Ui *ui, const char *id, const char *title, u16 icon,
                s32 selected, s32 closable, const char *cls)
{
        UiTabResult res;
        UiBox *tab;
        UiBox *row;
        UiBox *close;
        UiBox *accent;
        UiSignal close_sig;
        s32 show_close;

        memset(&res, 0, sizeof(res));

        /* A stack: the content row, and the accent line on top of it */
        tab = ui_box_begin(ui, id, cls ? cls : "tab",
                        UI_BOX_CLICKABLE | (selected ? UI_BOX_SELECTED : 0));
        tab->style.layout = UI_LAYOUT_STACK;
        tab->style.align_y = UI_ALIGN_START;

        row = ui_box_begin(ui, NULL, "tab-row", 0);
        row->style.height = ui_grow(1.0f);
        if(icon) ui_icon(ui, icon, "tab-icon");
        ui_label(ui, title, selected ? "tab-label.selected" : "tab-label");

        if(closable) {
                show_close = selected || ui->hot == tab->key;
                close = ui_box(ui, "close", "tab-close", UI_BOX_CLICKABLE);
                close->cursor = UI_CURSOR_POINTER;
                show_close = show_close || ui->hot == close->key;
                close->icon = show_close ? ICON_CLOSE : ICON_NONE;
                close_sig = ui_signal(ui, close);
                res.close_clicked = close_sig.clicked;
        }
        ui_box_end(ui);

        if(selected) {
                accent = ui_box(ui, NULL, "tab-accent", 0);
                accent->style.width = ui_grow(1.0f);
        }
        ui_box_end(ui);

        res.sig = ui_signal(ui, tab);
        res.key = tab->key;
        if(res.sig.middle_clicked && closable) res.close_clicked = 1;
        return res;
}

PUB f32 ui_splitter(Ui *ui, const char *id, s32 axis, const char *cls)
{
        UiBox *box;
        UiBox *line;
        UiSignal sig;
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
        ui_box_end(ui);

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

PUB UiBox *ui_scroll_begin(Ui *ui, const char *id, const char *cls)
{
        UiBox *box = ui_box_begin(ui, id, cls, 0);

        box->style.overflow = UI_OVERFLOW_SCROLL;
        return box;
}

PUB void ui_scroll_end(Ui *ui)
{
        UiBox *area = ui_parent(ui);

        if(area && area->state) {
                s_scrollbar(ui, area, UI_AXIS_X);
                s_scrollbar(ui, area, UI_AXIS_Y);
        }
        ui_box_end(ui);
}

PUB s32 ui_menu_begin(Ui *ui, u64 key)
{
        UiBox *menu;
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

PUB s32 ui_menu_item(Ui *ui, const char *label, const char *shortcut)
{
        UiBox *item;
        UiBox *text;
        UiSignal sig;

        item = ui_box_begin(ui, label, "menu-item", UI_BOX_CLICKABLE);
        text = ui_label(ui, label, "menu-item-label");
        text->style.width = ui_grow(1.0f);
        if(shortcut) ui_label(ui, shortcut, "menu-item-shortcut");
        ui_box_end(ui);

        sig = ui_signal(ui, item);
        if(sig.clicked) {
                ui_popup_close(ui);
                return 1;
        }
        return 0;
}

PUB void ui_menu_separator(Ui *ui)
{
        ui_box(ui, NULL, "menu-separator", 0);
}

PUB void ui_menu_end(Ui *ui)
{
        ui_box_end(ui);
        ui_pop_id(ui);
        ui_layer_end(ui);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

/*
 * A draggable thumb along axis a, from last frame's scroll numbers. It is a
 * floating child of the area, so scrolling doesn't move it.
 */
LOC void s_scrollbar(Ui *ui, UiBox *area, s32 a)
{
        UiBoxState *st = area->state;
        UiBox *thumb;
        UiSignal sig;
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
        const UiSize *thick;
        UiStyle style;

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
