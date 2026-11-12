#include <ui/widgets.h>

#include <render/icon_ids.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SCROLLBAR_SIZE     10.0f
#define SCROLLBAR_MIN_LEN  24.0f
#define TREE_INDENT        12.0f
#define SLIDER_THUMB       14.0f
#define TOAST_MARGIN       12.0f

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_scrollbar(UI *ui, UIBox *area, s32 a);
LOC s32 s_menu_item(UI *ui, const char *id, const char *label,
                const char *shortcut, s32 selected);
LOC s32 s_take_key(UI *ui, u64 key, u32 k, u16 mods);
LOC void s_toast_remove(UI *ui, s32 i);
LOC f32 s_clampf(f32 v, f32 lo, f32 hi);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UIBox *ui_spacer(UI *ui, UISize w, UISize h)
{
        UIBox *box = ui_box(ui, NULL, NULL, 0);

        box->style.width = w;
        box->style.height = h;
        return box;
}

PUB UIBox *ui_label(UI *ui, const char *text, const char *cls)
{
        UIBox *box = ui_box(ui, NULL, cls ? cls : "label", 0);

        ui_box_set_text(ui, box, text);
        return box;
}

PUB UIBox *ui_icon(UI *ui, u16 icon, const char *cls)
{
        UIBox *box = ui_box(ui, NULL, cls ? cls : "icon", 0);

        box->icon = icon;
        return box;
}

PUB UISignal ui_button(UI *ui, const char *label, const char *cls)
{
        UIBox *box = ui_box(ui, label, cls ? cls : "button", UI_BOX_CLICKABLE);

        box->cursor = UI_CURSOR_POINTER;
        ui_box_set_text(ui, box, label);
        return ui_signal(ui, box);
}

PUB UISignal ui_icon_button(UI *ui, const char *id, u16 icon,
                const char *tooltip, const char *cls)
{
        UIBox *box = ui_box(ui, id, cls ? cls : "icon-button",
                        UI_BOX_CLICKABLE);

        box->icon = icon;
        box->cursor = UI_CURSOR_POINTER;
        if(tooltip) ui_tooltip(ui, box, tooltip);
        return ui_signal(ui, box);
}

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

PUB UITabResult ui_tab(UI *ui, const char *id, const char *title, u16 icon,
                s32 selected, s32 closable, const char *cls)
{
        UITabResult res;
        UIBox *tab;
        UIBox *row;
        UIBox *close;
        UIBox *accent;
        UISignal close_sig;
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
        ui_box_end(ui); /* tab-row */

        if(selected) {
                accent = ui_box(ui, NULL, "tab-accent", 0);
                accent->style.width = ui_grow(1.0f);
        }
        ui_box_end(ui); /* tab */

        res.sig = ui_signal(ui, tab);
        res.key = tab->key;
        if(res.sig.middle_clicked && closable) res.close_clicked = 1;
        return res;
}

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

PUB s32 ui_menu_begin(UI *ui, u64 key)
{
        UIBox *menu;
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

PUB s32 ui_menu_item(UI *ui, const char *label, const char *shortcut)
{
        return s_menu_item(ui, label, label, shortcut, 0);
}

PUB void ui_menu_separator(UI *ui)
{
        ui_box(ui, NULL, "menu-separator", 0);
}

PUB void ui_menu_end(UI *ui)
{
        ui_box_end(ui); /* menu of ui_menu_begin */
        ui_pop_id(ui);
        ui_layer_end(ui);
}

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
        if(sig.clicked || s_take_key(ui, row->key, ' ', 0)) {
                *value = !on;
                ui->changed = 1;
                return 1;
        }
        return 0;
}

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
        if(sig.clicked || s_take_key(ui, row->key, ' ', 0)) {
                *value = option;
                ui->changed = 1;
                return 1;
        }
        return 0;
}

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
                *value = min + s_clampf(t, 0.0f, 1.0f) * range;
        }

        /* Keys */
        for(shift = 0; shift < 2; shift++) {
                u16 mods = shift ? UI_MOD_SHIFT : 0;

                step = range / (shift ? 10.0f : 100.0f);
                while(s_take_key(ui, box->key, UI_KEY_LEFT, mods) ||
                      s_take_key(ui, box->key, UI_KEY_DOWN, mods))
                        *value -= step;
                while(s_take_key(ui, box->key, UI_KEY_RIGHT, mods) ||
                      s_take_key(ui, box->key, UI_KEY_UP, mods))
                        *value += step;
        }
        if(s_take_key(ui, box->key, UI_KEY_HOME, 0)) *value = min;
        if(s_take_key(ui, box->key, UI_KEY_END, 0)) *value = max;

        if(range > 0.0f) {
                *value = s_clampf(*value, min, max);
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
                fill->style.width = ui_pct(s_clampf(value, 0.0f, 1.0f));
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

PUB s32 ui_dropdown(UI *ui, const char *id, s32 *selected,
                const char *const *items, s32 count, const char *cls)
{
        UIBox *box;
        UIBox *label;
        UIBox *menu;
        UISignal sig;
        u64 key = ui_key(ui, id);
        s32 open = ui_popup_is_open(ui, key);
        s32 cur = *selected;
        s32 toggle = 0;
        s32 i;

        box = ui_box_begin(ui, id, cls ? cls : "dropdown",
                        UI_BOX_CLICKABLE | UI_BOX_FOCUSABLE |
                        (open ? UI_BOX_SELECTED : 0));
        box->cursor = UI_CURSOR_POINTER;
        label = ui_label(ui, cur >= 0 && cur < count ? items[cur] : "",
                        "dropdown-label");
        label->style.width = ui_grow(1.0f);
        ui_icon(ui, ICON_CHEVRON_DOWN, "dropdown-chevron");
        ui_box_end(ui); /* dropdown */

        sig = ui_signal(ui, box);
        if(sig.pressed) toggle = 1;

        if(open) {
                /* Arrows pick right away, like a native select */
                while(s_take_key(ui, box->key, UI_KEY_UP, 0)) {
                        if(*selected > 0) (*selected)--;
                }
                while(s_take_key(ui, box->key, UI_KEY_DOWN, 0)) {
                        if(*selected < count - 1) (*selected)++;
                }
                if(s_take_key(ui, box->key, UI_KEY_ENTER, 0) ||
                   s_take_key(ui, box->key, ' ', 0))
                        toggle = 1;
        } else if(s_take_key(ui, box->key, UI_KEY_ENTER, 0) ||
                  s_take_key(ui, box->key, ' ', 0) ||
                  s_take_key(ui, box->key, UI_KEY_DOWN, 0)) {
                toggle = 1;
        }

        if(toggle && box->state) {
                if(open) {
                        ui_popup_close(ui);
                } else {
                        ui_popup_open(ui, key, box->state->rect.x,
                                        box->state->rect.y +
                                        box->state->rect.h + 2.0f);
                }
        }

        if(ui_menu_begin(ui, key)) {
                menu = ui_parent(ui);
                if(box->state) menu->style.width.min = box->state->rect.w;
                for(i = 0; i < count; i++) {
                        ui_push_id(ui, (u64)i);
                        if(s_menu_item(ui, "item", items[i], NULL,
                                                i == *selected)) {
                                *selected = i;
                        }
                        ui_pop_id(ui);
                }
                ui_menu_end(ui);
        }

        if(*selected != cur) {
                ui->changed = 1;
                return 1;
        }
        return 0;
}

PUB UIBox *ui_badge(UI *ui, const char *text, const char *cls)
{
        return ui_label(ui, text, cls ? cls : "badge");
}

PUB UITreeRowResult ui_tree_row(UI *ui, const char *id, const char *label,
                u16 icon, s32 depth, s32 has_children, s32 *expanded,
                s32 selected, const char *cls)
{
        UITreeRowResult res;
        UIBox *row;
        UIBox *chevron;
        s32 open = expanded && *expanded;

        memset(&res, 0, sizeof(res));

        row = ui_box_begin(ui, id, cls ? cls : "tree-row", UI_BOX_CLICKABLE |
                        (selected ? UI_BOX_SELECTED : 0));
        row->style.width = ui_grow(1.0f);
        row->style.padding[UI_LEFT] += (f32)(depth > 0 ? depth : 0) *
                TREE_INDENT;
        row->cursor = UI_CURSOR_POINTER;

        /* Leaves keep the space, so labels line up */
        chevron = ui_box(ui, NULL, "tree-chevron", 0);
        if(has_children)
                chevron->icon = open ? ICON_CHEVRON_DOWN : ICON_CHEVRON_RIGHT;
        if(icon) ui_icon(ui, icon, "tree-icon");
        ui_label(ui, label, selected ? "tree-label.selected" : "tree-label");
        ui_box_end(ui); /* tree-row */

        res.sig = ui_signal(ui, row);
        if(has_children && expanded && res.sig.clicked) {
                *expanded = !open;
                res.toggled = 1;
                ui->changed = 1;
        }
        return res;
}

PUB UIList ui_list_begin(UI *ui, const char *id, s32 count, f32 row_height,
                const char *cls)
{
        UIList list;
        UIBoxState *st;
        f32 scroll = 0.0f;
        f32 view;
        f32 max;

        memset(&list, 0, sizeof(list));
        if(count < 0) count = 0;
        if(row_height < 1.0f) row_height = 1.0f;

        list.box = ui_scroll_begin(ui, id, cls ? cls : "list");
        list.box->style.layout = UI_LAYOUT_COLUMN;
        list.box->style.gap = 0.0f;
        list.count = count;
        list.row_height = row_height;

        /* Visible range from last frame; the first frame assumes a full
           window so it doesn't come up empty */
        st = list.box->state;
        view = st && st->view[1] > 0.0f ? st->view[1] : ui->height;
        if(st) scroll = st->scroll[1];
        max = (f32)count * row_height - view;
        if(scroll > max) scroll = max;
        if(scroll < 0.0f) scroll = 0.0f;

        list.first = (s32)(scroll / row_height);
        list.last = (s32)ceilf((scroll + view) / row_height);
        if(list.first > count) list.first = count;
        if(list.last > count) list.last = count;

        if(list.first > 0)
                ui_spacer(ui, ui_px(0.0f), ui_px((f32)list.first * row_height));
        return list;
}

PUB void ui_list_end(UI *ui, UIList *list)
{
        if(list->last < list->count) {
                ui_spacer(ui, ui_px(0.0f), ui_px((f32)(list->count -
                                                list->last) *
                                        list->row_height));
        }
        ui_scroll_end(ui);
}

PUB void ui_list_scroll_to(UI *ui, UIList *list, s32 index)
{
        UIBoxState *st = list->box ? list->box->state : NULL;
        f32 top;
        f32 bottom;

        if(!st || index < 0 || index >= list->count) return;

        top = (f32)index * list->row_height;
        bottom = top + list->row_height;
        if(top < st->scroll[1]) {
                st->scroll[1] = top;
        } else if(bottom > st->scroll[1] + st->view[1]) {
                st->scroll[1] = bottom - st->view[1];
        } else {
                return;
        }
        ui->changed = 1;
}

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
        ui_sides4(area->style.padding, TOAST_MARGIN, TOAST_MARGIN,
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

/*
 * A draggable thumb along axis a, from last frame's scroll numbers. It is a
 * floating child of the area, so scrolling doesn't move it.
 */
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

LOC s32 s_menu_item(UI *ui, const char *id, const char *label,
                const char *shortcut, s32 selected)
{
        UIBox *item;
        UIBox *text;
        UISignal sig;

        item = ui_box_begin(ui, id, "menu-item", UI_BOX_CLICKABLE |
                        (selected ? UI_BOX_SELECTED : 0));
        text = ui_label(ui, label, "menu-item-label");
        text->style.width = ui_grow(1.0f);
        if(shortcut) ui_label(ui, shortcut, "menu-item-shortcut");
        ui_box_end(ui); /* menu-item */

        sig = ui_signal(ui, item);
        if(sig.clicked) {
                ui_popup_close(ui);
                return 1;
        }
        return 0;
}

/* Take one unhandled key press if the box with key has focus */
LOC s32 s_take_key(UI *ui, u64 key, u32 k, u16 mods)
{
        UIInput *in = ui->input;
        s32 i;

        if(!in || !key || ui->focus != key) return 0;
        for(i = 0; i < in->event_count; i++) {
                if(ui_event_is_key(&in->events[i], k, UI_SCAN_NONE, mods)) {
                        in->events[i].consumed = 1;
                        return 1;
                }
        }
        return 0;
}

LOC void s_toast_remove(UI *ui, s32 i)
{
        if(i < 0 || i >= ui->toast_count) return;
        memmove(&ui->toasts[i], &ui->toasts[i + 1],
                        sizeof(UIToast) * (size_t)(ui->toast_count - i - 1));
        ui->toast_count--;
}

LOC f32 s_clampf(f32 v, f32 lo, f32 hi)
{
        if(v < lo) return lo;
        if(v > hi) return hi;
        return v;
}
