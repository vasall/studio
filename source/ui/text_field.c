#include <ui/widgets.h>

#include <math.h>
#include <string.h>

#define CARET_BLINK 0.53        /* Seconds per half period */
#define CARET_WIDTH 1.0f

/* What the draw callback needs, copied into the frame arena */
typedef struct TextDraw {
        const char *text;
        s32 len;
        s32 caret;
        s32 sel_lo;
        s32 sel_hi;
        f32 scroll;             /* Text x shown at the left content edge */
        s32 focused;
        s32 show_caret;
        const char *placeholder;
} TextDraw;

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_draw(UI *ui, UIBox *box, void *data);
LOC f32 s_width(UI *ui, const UIStyle *s, const char *text, s32 n);
LOC f32 s_line_height(UI *ui, const UIStyle *s);
LOC s32 s_hit(UI *ui, const UIStyle *s, const char *text, s32 len, f32 x);

LOC s32 s_prev(const char *s, s32 i);
LOC s32 s_next(const char *s, s32 len, s32 i);
LOC s32 s_snap(const char *s, s32 len, s32 i);
LOC s32 s_class(char c);
LOC s32 s_word_left(const char *s, s32 i);
LOC s32 s_word_right(const char *s, s32 len, s32 i);

LOC void s_erase(char *buf, s32 *len, s32 lo, s32 hi);
LOC s32 s_insert(char *buf, s32 *len, s32 cap, s32 at, const char *text,
                s32 n);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UITextFieldResult ui_text_field(UI *ui, const char *id, char *buf,
                s32 cap, const char *placeholder, const char *cls)
{
        UITextFieldResult res;
        UIInput *in = ui->input;
        UIBox *box;
        UIBoxState *st;
        UIStyle *s;
        UIEvent *ev;
        TextDraw *td;
        char *clip;
        s32 len;
        s32 caret;
        s32 anchor;
        s32 lo;
        s32 hi;
        s32 handled;
        s32 ctrl;
        s32 shift;
        s32 n;
        s32 i;
        f32 line_h;
        f32 view_w;
        f32 scroll;
        f32 cx = 0.0f;
        f32 tw;
        f64 phase;

        memset(&res, 0, sizeof(res));
        if(!buf || cap <= 0) return res;

        box = ui_box(ui, id, cls ? cls : "text-field",
                        UI_BOX_CLICKABLE | UI_BOX_FOCUSABLE);
        if(!box) return res;
        s = &box->style;
        box->cursor = UI_CURSOR_TEXT;
        res.key = box->key;

        /* One line of text high, if the class leaves the height open */
        line_h = s_line_height(ui, s);
        if(s->height.kind == UI_SIZE_FIT) {
                s->height = ui_px(line_h + s->padding[UI_TOP] +
                                s->padding[UI_BOTTOM]);
        }

        res.sig = ui_signal(ui, box);
        st = box->state;
        if(!st) return res;

        buf[cap - 1] = '\0';
        len = (s32)strlen(buf);
        caret = s_snap(buf, len, st->user_i);
        anchor = s_snap(buf, len, st->user_j);

        /* Mouse, in text coordinates */
        if(res.sig.pressed || (ui->active == box->key && res.sig.dragging &&
                                in && in->button_down[UI_MOUSE_LEFT])) {
                f32 mx = res.sig.mouse_x - s->padding[UI_LEFT] + st->user_f;

                caret = s_hit(ui, s, buf, len, mx);
                if(res.sig.pressed && !(in->mods & UI_MOD_SHIFT))
                        anchor = caret;
                if(res.sig.double_clicked && len > 0) {
                        /* Select the word (or run of spaces) at the caret */
                        n = caret < len ? caret : len - 1;
                        handled = s_class(buf[n]);
                        anchor = n;
                        while(anchor > 0 && s_class(buf[anchor - 1]) ==
                                        handled)
                                anchor--;
                        caret = n;
                        while(caret < len && s_class(buf[caret]) == handled)
                                caret++;
                }
                st->user_t = ui->time;
        }

        /* Keyboard */
        if(ui->focus == box->key && in) {
                for(i = 0; i < in->event_count; i++) {
                        ev = &in->events[i];
                        if(ev->consumed) continue;
                        lo = caret < anchor ? caret : anchor;
                        hi = caret < anchor ? anchor : caret;

                        if(ev->kind == UI_EVENT_TEXT) {
                                s_erase(buf, &len, lo, hi);
                                n = s_insert(buf, &len, cap, lo, ev->text,
                                                (s32)strlen(ev->text));
                                caret = anchor = lo + n;
                                res.changed |= n > 0 || lo != hi;
                                ev->consumed = 1;
                                st->user_t = ui->time;
                                continue;
                        }
                        if(ev->kind != UI_EVENT_KEY_DOWN) continue;

                        ctrl = (ev->mods & UI_MOD_CTRL) != 0;
                        shift = (ev->mods & UI_MOD_SHIFT) != 0;
                        handled = 1;

                        switch(ev->key) {
                        case UI_KEY_LEFT:
                                if(lo != hi && !shift) {
                                        caret = lo;
                                } else {
                                        caret = ctrl ? s_word_left(buf, caret) :
                                                s_prev(buf, caret);
                                }
                                if(!shift) anchor = caret;
                                break;

                        case UI_KEY_RIGHT:
                                if(lo != hi && !shift) {
                                        caret = hi;
                                } else {
                                        caret = ctrl ? s_word_right(buf, len,
                                                        caret) :
                                                s_next(buf, len, caret);
                                }
                                if(!shift) anchor = caret;
                                break;

                        case UI_KEY_HOME:
                                caret = 0;
                                if(!shift) anchor = caret;
                                break;

                        case UI_KEY_END:
                                caret = len;
                                if(!shift) anchor = caret;
                                break;

                        case UI_KEY_BACKSPACE:
                                if(lo == hi) {
                                        lo = ctrl ? s_word_left(buf, caret) :
                                                s_prev(buf, caret);
                                }
                                res.changed |= lo != hi;
                                s_erase(buf, &len, lo, hi);
                                caret = anchor = lo;
                                break;

                        case UI_KEY_DELETE:
                                if(lo == hi) {
                                        hi = ctrl ? s_word_right(buf, len,
                                                        caret) :
                                                s_next(buf, len, caret);
                                }
                                res.changed |= lo != hi;
                                s_erase(buf, &len, lo, hi);
                                caret = anchor = lo;
                                break;

                        case UI_KEY_ENTER:
                                res.submitted = 1;
                                break;

                        case UI_KEY_ESCAPE:
                                res.cancelled = 1;
                                ui_set_focus(ui, 0);
                                break;

                        case 'a':
                                if(!ctrl) goto plain;
                                anchor = 0;
                                caret = len;
                                break;

                        case 'c':
                        case 'x':
                                if(!ctrl) goto plain;
                                if(lo != hi && ui->clipboard_set) {
                                        ui->clipboard_set(ui->clipboard_user,
                                                        buf + lo, hi - lo);
                                }
                                if(ev->key == 'x' && lo != hi) {
                                        s_erase(buf, &len, lo, hi);
                                        caret = anchor = lo;
                                        res.changed = 1;
                                }
                                break;

                        case 'v':
                                if(!ctrl) goto plain;
                                if(!ui->clipboard_get) break;
                                clip = arena_alloc(&ui->arena, (size_t)cap);
                                if(!clip) break;
                                n = ui->clipboard_get(ui->clipboard_user, clip,
                                                cap);
                                s_erase(buf, &len, lo, hi);
                                n = s_insert(buf, &len, cap, lo, clip, n);
                                caret = anchor = lo + n;
                                res.changed |= n > 0 || lo != hi;
                                break;

                        default:
                        plain:
                                /* Plain keys also come as text; swallow the
                                   key press so it isn't a shortcut, but leave
                                   Tab, F-keys and chords alone */
                                handled = ev->key < UI_KEY_SPECIAL &&
                                        !(ev->mods & (UI_MOD_CTRL |
                                                      UI_MOD_ALT |
                                                      UI_MOD_SUPER));
                                break;
                        }

                        if(handled) {
                                ev->consumed = 1;
                                st->user_t = ui->time;
                        }
                }
        }

        /* Keep the caret in view */
        view_w = st->rect.w - s->padding[UI_LEFT] - s->padding[UI_RIGHT];
        scroll = st->user_f;
        if(view_w > 0.0f) {
                cx = s_width(ui, s, buf, caret);
                tw = s_width(ui, s, buf, len);
                if(tw - scroll < view_w - CARET_WIDTH)
                        scroll = tw - view_w + CARET_WIDTH;
                if(cx - scroll > view_w - CARET_WIDTH)
                        scroll = cx - view_w + CARET_WIDTH;
                if(cx < scroll) scroll = cx;
                if(scroll < 0.0f) scroll = 0.0f;
        }
        st->user_f = scroll;
        st->user_i = caret;
        st->user_j = anchor;
        res.focused = ui->focus == box->key;

        td = arena_alloc(&ui->arena, sizeof(TextDraw));
        if(td) {
                td->text = arena_strdup(&ui->arena, buf, len);
                td->len = len;
                td->caret = caret;
                td->sel_lo = caret < anchor ? caret : anchor;
                td->sel_hi = caret < anchor ? anchor : caret;
                td->scroll = scroll;
                td->focused = res.focused;
                td->placeholder = placeholder;
                box->custom_draw = s_draw;
                box->custom_data = td;
        }

        if(res.focused) {
                /* Blink, restarted by every edit or click */
                phase = (ui->time - st->user_t) / CARET_BLINK;
                if(phase < 0.0) phase = 0.0;
                if(td) td->show_caret = ((s64)phase % 2) == 0;
                ui_request_wakeup(ui, st->user_t +
                                (floor(phase) + 1.0) * CARET_BLINK);

                ui->text_input = 1;
                ui->text_input_rect = ui_rect(st->rect.x +
                                s->padding[UI_LEFT] + cx - scroll,
                                st->rect.y + s->padding[UI_TOP],
                                CARET_WIDTH, line_h);
        }

        if(res.changed) ui->changed = 1;
        return res;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC void s_draw(UI *ui, UIBox *box, void *data)
{
        TextDraw *td = data;
        UIStyle *s = &box->style;
        UIStyle sub;
        UIDrawCmd cmd;
        UIRect content;
        UIRect clip;
        f32 line_h = s_line_height(ui, s);
        f32 x0;
        f32 y;
        f32 a;
        f32 b;

        content = ui_rect(box->rect.x + s->padding[UI_LEFT],
                        box->rect.y + s->padding[UI_TOP],
                        box->rect.w - s->padding[UI_LEFT] -
                        s->padding[UI_RIGHT],
                        box->rect.h - s->padding[UI_TOP] -
                        s->padding[UI_BOTTOM]);
        /* Room for the caret at the very end */
        content.w += CARET_WIDTH;
        clip = ui_rect_intersect(box->clip, content);
        if(clip.w <= 0.0f || clip.h <= 0.0f) return;

        x0 = content.x - td->scroll;
        y = content.y + (content.h - line_h) * 0.5f;

        if(td->focused && td->sel_lo != td->sel_hi) {
                sub = ui_theme_style(ui->theme, "text-field-selection", 0);
                a = s_width(ui, s, td->text, td->sel_lo);
                b = s_width(ui, s, td->text, td->sel_hi);
                memset(&cmd, 0, sizeof(cmd));
                cmd.kind = UI_DRAW_RECT;
                cmd.rect = ui_rect(x0 + a, y, b - a, line_h);
                cmd.clip = clip;
                cmd.color = sub.infill;
                ui_draw_push(ui, &cmd);
        }

        memset(&cmd, 0, sizeof(cmd));
        cmd.kind = UI_DRAW_TEXT;
        cmd.clip = clip;
        cmd.font = s->font;
        cmd.font_size = s->font_size;
        if(td->len > 0) {
                cmd.rect = ui_rect(x0, y, s_width(ui, s, td->text, td->len),
                                line_h);
                cmd.color = s->text_color;
                cmd.text = td->text;
                cmd.text_len = td->len;
                ui_draw_push(ui, &cmd);
        } else if(td->placeholder && *td->placeholder) {
                sub = ui_theme_style(ui->theme, "text-field-placeholder", 0);
                cmd.text_len = ui_display_len(td->placeholder);
                cmd.text = td->placeholder;
                cmd.rect = ui_rect(content.x, y, s_width(ui, s, cmd.text,
                                        cmd.text_len), line_h);
                cmd.color = sub.text_color.a ? sub.text_color : s->text_color;
                ui_draw_push(ui, &cmd);
        }

        if(td->show_caret) {
                memset(&cmd, 0, sizeof(cmd));
                cmd.kind = UI_DRAW_RECT;
                cmd.rect = ui_rect(x0 + s_width(ui, s, td->text, td->caret),
                                y, CARET_WIDTH, line_h);
                cmd.clip = clip;
                cmd.color = s->text_color;
                ui_draw_push(ui, &cmd);
        }
}

LOC f32 s_width(UI *ui, const UIStyle *s, const char *text, s32 n)
{
        if(n <= 0 || !ui->measure) return 0.0f;
        return ui->measure(ui->measure_user, s->font, s->font_size, text, n);
}

LOC f32 s_line_height(UI *ui, const UIStyle *s)
{
        if(!ui->line_height) return s->font_size;
        return ui->line_height(ui->measure_user, s->font, s->font_size);
}

/* Byte offset of the character boundary closest to x */
LOC s32 s_hit(UI *ui, const UIStyle *s, const char *text, s32 len, f32 x)
{
        s32 best = 0;
        s32 i = 0;
        f32 best_d = fabsf(x);
        f32 d;

        while(i < len) {
                i = s_next(text, len, i);
                d = fabsf(s_width(ui, s, text, i) - x);
                if(d < best_d) {
                        best_d = d;
                        best = i;
                }
        }
        return best;
}

/* Start of the code point before i */
LOC s32 s_prev(const char *s, s32 i)
{
        if(i <= 0) return 0;
        i--;
        while(i > 0 && ((u8)s[i] & 0xc0) == 0x80) i--;
        return i;
}

/* Start of the code point after i */
LOC s32 s_next(const char *s, s32 len, s32 i)
{
        if(i >= len) return len;
        i++;
        while(i < len && ((u8)s[i] & 0xc0) == 0x80) i++;
        return i;
}

/* Clamp i into the text and onto a code point boundary */
LOC s32 s_snap(const char *s, s32 len, s32 i)
{
        if(i < 0) return 0;
        if(i > len) return len;
        while(i > 0 && i < len && ((u8)s[i] & 0xc0) == 0x80) i--;
        return i;
}

/* 0 space, 1 word (letters, digits, _ and non-ASCII), 2 punctuation */
LOC s32 s_class(char c)
{
        u8 u = (u8)c;

        if(u == ' ' || u == '\t') return 0;
        if(u >= 0x80 || u == '_' || (u >= '0' && u <= '9') ||
           (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z'))
                return 1;
        return 2;
}

LOC s32 s_word_left(const char *s, s32 i)
{
        s32 cls;

        while(i > 0 && s_class(s[i - 1]) == 0) i--;
        if(i == 0) return 0;
        cls = s_class(s[i - 1]);
        while(i > 0 && s_class(s[i - 1]) == cls) i--;
        return i;
}

LOC s32 s_word_right(const char *s, s32 len, s32 i)
{
        s32 cls;

        if(i >= len) return len;
        cls = s_class(s[i]);
        if(cls != 0) {
                while(i < len && s_class(s[i]) == cls) i++;
        }
        while(i < len && s_class(s[i]) == 0) i++;
        return i;
}

LOC void s_erase(char *buf, s32 *len, s32 lo, s32 hi)
{
        if(hi <= lo) return;
        memmove(buf + lo, buf + hi, (size_t)(*len - hi + 1));
        *len -= hi - lo;
}

/*
 * Insert text at at, dropping control characters (newlines become spaces)
 * and whatever doesn't fit; never splits a code point.
 *
 * @returns Bytes inserted
 */
LOC s32 s_insert(char *buf, s32 *len, s32 cap, s32 at, const char *text,
                s32 n)
{
        s32 room = cap - 1 - *len;
        s32 out = 0;
        s32 i = 0;
        s32 j;
        s32 cp_len;
        u8 c;

        while(i < n && text[i]) {
                c = (u8)text[i];
                cp_len = c < 0x80 ? 1 : c >= 0xf0 ? 4 : c >= 0xe0 ? 3 :
                        c >= 0xc0 ? 2 : 1;
                if(i + cp_len > n) break;
                if(c < 0x20 || c == 0x7f) {
                        if(c != '\n' && c != '\t') {
                                i++;
                                continue;
                        }
                        c = ' ';
                }
                if(out + cp_len > room) break;

                memmove(buf + at + out + cp_len, buf + at + out,
                                (size_t)(*len - at - out + 1));
                if(cp_len == 1) {
                        buf[at + out] = (char)c;
                } else {
                        for(j = 0; j < cp_len; j++)
                                buf[at + out + j] = text[i + j];
                }
                *len += cp_len;
                out += cp_len;
                i += cp_len;
        }
        return out;
}
