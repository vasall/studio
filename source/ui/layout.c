#include <ui/ui.h>

#include <render/icon_ids.h>

#include <string.h>

/*
 * Layout in three steps, see the plan "Layout solver":
 *
 *   for x, then y:
 *     s_fit    bottom-up: fixed sizes, text, and FIT containers
 *     s_solve  top-down: percent, grow, shrink, line breaking (wrap)
 *   s_place    top-down: positions, alignment, scrolling, clipping
 *
 * Wrapping rows break into lines during the x pass, so the y pass already
 * knows the lines. Wrapping columns would need the y sizes during the x pass;
 * for now their width is computed as if they didn't wrap.
 */

#define MAX_LINES 256

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

typedef struct LayoutCtx {
        UiMeasureFn measure;
        UiLineHeightFn line_height;
        void *user;
} LayoutCtx;

LOC void s_fit(LayoutCtx *ctx, UiBox *box, s32 a);
LOC void s_solve(UiBox *box, s32 a);
LOC void s_place(UiBox *box);

LOC s32 s_main_axis(const UiBox *box);
LOC f32 s_pad_lo(const UiBox *box, s32 a);
LOC f32 s_pad_hi(const UiBox *box, s32 a);
LOC f32 s_margin_lo(const UiBox *box, s32 a);
LOC f32 s_margin_hi(const UiBox *box, s32 a);
LOC f32 s_outer(const UiBox *box, s32 a);
LOC f32 s_fit_outer(const UiBox *box, s32 a);
LOC f32 s_avail(const UiBox *box, s32 a);
LOC const UiSize *s_size_of(const UiBox *box, s32 a);
LOC f32 s_clamp(const UiSize *s, f32 v);
LOC s32 s_in_flow(const UiBox *box);
LOC f32 s_align(f32 avail, f32 used, u8 align);
LOC f32 s_intrinsic_own(LayoutCtx *ctx, const UiBox *box, s32 a);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void ui_layout(UiBox *root, UiMeasureFn measure,
                UiLineHeightFn line_height, void *user)
{
        LayoutCtx ctx;
        s32 a;

        if(!root) return;

        ctx.measure = measure;
        ctx.line_height = line_height;
        ctx.user = user;

        for(a = 0; a < 2; a++) {
                s_fit(&ctx, root, a);

                /* The root has no parent to grow in; percent of nothing */
                if(s_size_of(root, a)->kind == UI_SIZE_PERCENT ||
                                s_size_of(root, a)->kind == UI_SIZE_GROW)
                        root->size[a] = s_clamp(s_size_of(root, a), 0.0f);

                s_solve(root, a);
        }

        root->pos[0] = 0.0f;
        root->pos[1] = 0.0f;
        root->rect = ui_rect(root->float_x, root->float_y, root->size[0],
                        root->size[1]);
        root->clip = root->rect;
        s_place(root);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

/*
 * Bottom-up: set size[a] of fixed and FIT boxes. PERCENT and GROW boxes get
 * their min here and their real size from the parent in s_solve.
 */
LOC void s_fit(LayoutCtx *ctx, UiBox *box, s32 a)
{
        const UiSize *sz = s_size_of(box, a);
        UiBox *c;
        s32 main = s_main_axis(box);
        f32 content = 0.0f;
        f32 v;
        s32 n = 0;

        for(c = box->first; c; c = c->next)
                s_fit(ctx, c, a);

        /* Content size from the children */
        if(main != a && box->style.wrap && main >= 0) {
                /* Cross axis of a wrapping box: sum of line heights */
                f32 line_size[MAX_LINES];
                s32 lines = 0;
                s32 i;

                memset(line_size, 0, sizeof(line_size));
                for(c = box->first; c; c = c->next) {
                        if(!s_in_flow(c)) continue;
                        i = c->line < MAX_LINES ? c->line : MAX_LINES - 1;
                        v = s_fit_outer(c, a);
                        if(v > line_size[i]) line_size[i] = v;
                        if(i + 1 > lines) lines = i + 1;
                }
                for(i = 0; i < lines; i++)
                        content += line_size[i];
                if(lines > 1) content += box->style.gap * (f32)(lines - 1);
        } else {
                for(c = box->first; c; c = c->next) {
                        if(!s_in_flow(c)) continue;
                        v = s_fit_outer(c, a);
                        if(main == a) {
                                content += v;
                        } else if(v > content) {
                                content = v;
                        }
                        n++;
                }
                if(main == a && n > 1) content += box->style.gap * (f32)(n - 1);
        }

        /* Own text and icon */
        v = s_intrinsic_own(ctx, box, a);
        if(v > content) content = v;

        box->basis[a] = s_clamp(sz, content + s_pad_lo(box, a) +
                        s_pad_hi(box, a));

        switch(sz->kind) {
        case UI_SIZE_PIXELS:
                box->size[a] = s_clamp(sz, sz->value);
                break;

        case UI_SIZE_PERCENT:
        case UI_SIZE_GROW:
                box->size[a] = s_clamp(sz, 0.0f);
                break;

        default:
                box->size[a] = box->basis[a];
                break;
        }
}

/*
 * Top-down: box->size[a] is final, set the sizes of its children.
 */
LOC void s_solve(UiBox *box, s32 a)
{
        UiBox *c;
        const UiSize *sz;
        s32 main = s_main_axis(box);
        f32 avail = s_avail(box, a);
        f32 used;
        f32 free_space;
        f32 weights;
        f32 shrinkable;
        s32 n;
        s32 iter;

        /* Percent children and floating children, any axis */
        for(c = box->first; c; c = c->next) {
                sz = s_size_of(c, a);
                if(sz->kind == UI_SIZE_PERCENT) {
                        c->size[a] = s_clamp(sz, sz->value * avail);
                } else if(!s_in_flow(c) && sz->kind == UI_SIZE_GROW) {
                        c->size[a] = s_clamp(sz, avail - s_margin_lo(c, a) -
                                        s_margin_hi(c, a));
                }
        }

        if(main == a && box->style.wrap) {
                /* Break into lines, then grow per line */
                f32 line_used = 0.0f;
                s32 line = 0;
                s32 in_line = 0;
                UiBox *line_first = NULL;
                UiBox *d;

                for(c = box->first;; c = c->next) {
                        s32 line_ends = !c;

                        if(c && s_in_flow(c)) {
                                f32 v = s_outer(c, a);
                                if(in_line > 0 &&
                                   line_used + box->style.gap + v > avail) {
                                        line_ends = 1;
                                }
                        } else if(c) {
                                continue;
                        }

                        if(line_ends && in_line > 0) {
                                /* Grow the children of the finished line */
                                free_space = avail - line_used;
                                weights = 0.0f;
                                for(d = line_first; d != c; d = d->next) {
                                        if(s_in_flow(d) &&
                                           s_size_of(d, a)->kind == UI_SIZE_GROW)
                                                weights += s_size_of(d, a)->value;
                                }
                                if(free_space > 0.0f && weights > 0.0f) {
                                        for(d = line_first; d != c; d = d->next) {
                                                sz = s_size_of(d, a);
                                                if(!s_in_flow(d) ||
                                                   sz->kind != UI_SIZE_GROW)
                                                        continue;
                                                d->size[a] = s_clamp(sz,
                                                        d->size[a] + free_space *
                                                        sz->value / weights);
                                        }
                                }
                                line++;
                                line_used = 0.0f;
                                in_line = 0;
                        }
                        if(!c) break;

                        if(in_line == 0) {
                                line_first = c;
                                line_used = s_outer(c, a);
                        } else {
                                line_used += box->style.gap + s_outer(c, a);
                        }
                        c->line = line;
                        in_line++;
                }
        } else if(main == a) {
                /* One line: grow into free space or shrink on overflow */
                used = 0.0f;
                n = 0;
                for(c = box->first; c; c = c->next) {
                        if(!s_in_flow(c)) continue;
                        c->line = 0;
                        used += s_outer(c, a);
                        n++;
                }
                if(n > 1) used += box->style.gap * (f32)(n - 1);
                free_space = avail - used;

                /* Grow, a few rounds so children capped at max pass on */
                for(iter = 0; iter < 4 && free_space > 0.01f; iter++) {
                        f32 given = 0.0f;

                        weights = 0.0f;
                        for(c = box->first; c; c = c->next) {
                                sz = s_size_of(c, a);
                                if(!s_in_flow(c) || sz->kind != UI_SIZE_GROW)
                                        continue;
                                if(sz->max > 0.0f && c->size[a] >= sz->max)
                                        continue;
                                weights += sz->value;
                        }
                        if(weights <= 0.0f) break;

                        for(c = box->first; c; c = c->next) {
                                f32 old;

                                sz = s_size_of(c, a);
                                if(!s_in_flow(c) || sz->kind != UI_SIZE_GROW)
                                        continue;
                                if(sz->max > 0.0f && c->size[a] >= sz->max)
                                        continue;
                                old = c->size[a];
                                c->size[a] = s_clamp(sz, old + free_space *
                                                sz->value / weights);
                                given += c->size[a] - old;
                        }
                        free_space -= given;
                }

                /*
                 * Shrink FIT and GROW children towards their min. Children
                 * of a scrolling box keep their size and scroll instead.
                 */
                if(free_space < 0.0f &&
                   box->style.overflow != UI_OVERFLOW_SCROLL) {
                        f32 overflow = -free_space;

                        shrinkable = 0.0f;
                        for(c = box->first; c; c = c->next) {
                                sz = s_size_of(c, a);
                                if(!s_in_flow(c) || sz->kind == UI_SIZE_PIXELS ||
                                   sz->kind == UI_SIZE_PERCENT)
                                        continue;
                                if(c->size[a] > sz->min)
                                        shrinkable += c->size[a] - sz->min;
                        }
                        if(shrinkable > 0.0f) {
                                for(c = box->first; c; c = c->next) {
                                        f32 range;

                                        sz = s_size_of(c, a);
                                        if(!s_in_flow(c) ||
                                           sz->kind == UI_SIZE_PIXELS ||
                                           sz->kind == UI_SIZE_PERCENT)
                                                continue;
                                        range = c->size[a] - sz->min;
                                        if(range <= 0.0f) continue;
                                        if(overflow >= shrinkable) {
                                                c->size[a] = sz->min;
                                        } else {
                                                c->size[a] -= overflow * range /
                                                        shrinkable;
                                        }
                                }
                        }
                }
        } else {
                /* Cross axis or stack: GROW fills the content box or line */
                f32 line_size[MAX_LINES];
                s32 wraps = box->style.wrap && main >= 0;
                s32 i;

                if(wraps) {
                        memset(line_size, 0, sizeof(line_size));
                        for(c = box->first; c; c = c->next) {
                                if(!s_in_flow(c)) continue;
                                if(s_size_of(c, a)->kind == UI_SIZE_GROW)
                                        continue;
                                i = c->line < MAX_LINES ? c->line : MAX_LINES - 1;
                                if(s_outer(c, a) > line_size[i])
                                        line_size[i] = s_outer(c, a);
                        }
                }

                for(c = box->first; c; c = c->next) {
                        f32 room;

                        sz = s_size_of(c, a);
                        if(!s_in_flow(c) || sz->kind != UI_SIZE_GROW) continue;

                        room = avail;
                        if(wraps) {
                                i = c->line < MAX_LINES ? c->line : MAX_LINES - 1;
                                room = line_size[i];
                        }
                        c->size[a] = s_clamp(sz, room - s_margin_lo(c, a) -
                                        s_margin_hi(c, a));
                }
        }

        for(c = box->first; c; c = c->next) {
                if(c->size[a] < 0.0f) c->size[a] = 0.0f;
                s_solve(c, a);
        }
}

/*
 * Top-down: positions and clip rects. box->rect and box->clip are set.
 */
LOC void s_place(UiBox *box)
{
        UiBox *c;
        s32 main = s_main_axis(box);
        s32 cross = main == UI_AXIS_X ? UI_AXIS_Y : UI_AXIS_X;
        f32 avail[2];
        f32 origin[2];
        f32 scroll[2];
        f32 content[2];
        f32 used;
        f32 cursor;
        f32 bw;
        UiRect inner;
        UiRect child_clip;
        s32 n;
        s32 a;

        avail[0] = s_avail(box, UI_AXIS_X);
        avail[1] = s_avail(box, UI_AXIS_Y);
        content[0] = 0.0f;
        content[1] = 0.0f;

        if(main < 0) {
                /* STACK: every child on its own */
                for(c = box->first; c; c = c->next) {
                        if(!s_in_flow(c)) continue;
                        for(a = 0; a < 2; a++) {
                                u8 al = a == 0 ? box->style.align_x :
                                        box->style.align_y;
                                c->pos[a] = s_align(avail[a], s_outer(c, a), al) +
                                        s_margin_lo(c, a);
                                if(s_outer(c, a) > content[a])
                                        content[a] = s_outer(c, a);
                        }
                }
        } else if(!box->style.wrap) {
                u8 al_main = main == 0 ? box->style.align_x : box->style.align_y;
                u8 al_cross = main == 0 ? box->style.align_y : box->style.align_x;

                used = 0.0f;
                n = 0;
                for(c = box->first; c; c = c->next) {
                        if(!s_in_flow(c)) continue;
                        used += s_outer(c, main);
                        if(s_outer(c, cross) > content[cross])
                                content[cross] = s_outer(c, cross);
                        n++;
                }
                if(n > 1) used += box->style.gap * (f32)(n - 1);
                content[main] = used;

                cursor = s_align(avail[main], used, al_main);
                for(c = box->first; c; c = c->next) {
                        if(!s_in_flow(c)) continue;
                        c->pos[main] = cursor + s_margin_lo(c, main);
                        cursor += s_outer(c, main) + box->style.gap;
                        c->pos[cross] = s_align(avail[cross],
                                        s_outer(c, cross), al_cross) +
                                s_margin_lo(c, cross);
                }
        } else {
                /* Wrapped lines */
                u8 al_main = main == 0 ? box->style.align_x : box->style.align_y;
                u8 al_cross = main == 0 ? box->style.align_y : box->style.align_x;
                f32 line_used[MAX_LINES];
                f32 line_size[MAX_LINES];
                f32 line_cursor[MAX_LINES];
                f32 line_start[MAX_LINES];
                f32 total = 0.0f;
                s32 lines = 0;
                s32 i;

                memset(line_used, 0, sizeof(line_used));
                memset(line_size, 0, sizeof(line_size));
                for(c = box->first; c; c = c->next) {
                        if(!s_in_flow(c)) continue;
                        i = c->line < MAX_LINES ? c->line : MAX_LINES - 1;
                        if(line_used[i] > 0.0f) line_used[i] += box->style.gap;
                        line_used[i] += s_outer(c, main);
                        if(s_outer(c, cross) > line_size[i])
                                line_size[i] = s_outer(c, cross);
                        if(i + 1 > lines) lines = i + 1;
                }

                for(i = 0; i < lines; i++) {
                        total += line_size[i];
                        if(line_used[i] > content[main])
                                content[main] = line_used[i];
                }
                if(lines > 1) total += box->style.gap * (f32)(lines - 1);
                content[cross] = total;

                cursor = s_align(avail[cross], total, al_cross);
                for(i = 0; i < lines; i++) {
                        line_start[i] = cursor;
                        cursor += line_size[i] + box->style.gap;
                        line_cursor[i] = s_align(avail[main], line_used[i],
                                        al_main);
                }

                for(c = box->first; c; c = c->next) {
                        if(!s_in_flow(c)) continue;
                        i = c->line < MAX_LINES ? c->line : MAX_LINES - 1;
                        c->pos[main] = line_cursor[i] + s_margin_lo(c, main);
                        line_cursor[i] += s_outer(c, main) + box->style.gap;
                        c->pos[cross] = line_start[i] + s_align(line_size[i],
                                        s_outer(c, cross), al_cross) +
                                s_margin_lo(c, cross);
                }
        }

        box->content[0] = content[0];
        box->content[1] = content[1];

        /* Scrolling: clamp the offset to the content */
        scroll[0] = 0.0f;
        scroll[1] = 0.0f;
        if(box->state) {
                box->state->content[0] = content[0];
                box->state->content[1] = content[1];
                box->state->view[0] = avail[0];
                box->state->view[1] = avail[1];

                if(box->style.overflow == UI_OVERFLOW_SCROLL) {
                        for(a = 0; a < 2; a++) {
                                f32 max = content[a] - avail[a];
                                if(max < 0.0f) max = 0.0f;
                                if(box->state->scroll[a] > max)
                                        box->state->scroll[a] = max;
                                if(box->state->scroll[a] < 0.0f)
                                        box->state->scroll[a] = 0.0f;
                                scroll[a] = box->state->scroll[a];
                        }
                } else {
                        box->state->scroll[0] = 0.0f;
                        box->state->scroll[1] = 0.0f;
                }
        }

        origin[0] = box->rect.x + s_pad_lo(box, UI_AXIS_X) - scroll[0];
        origin[1] = box->rect.y + s_pad_lo(box, UI_AXIS_Y) - scroll[1];

        /* Children are clipped to the inside of the border */
        child_clip = box->clip;
        if(box->style.overflow != UI_OVERFLOW_VISIBLE) {
                bw = box->style.border_width;
                inner = ui_rect(box->rect.x + bw, box->rect.y + bw,
                                box->rect.w - 2.0f * bw, box->rect.h - 2.0f * bw);
                child_clip = ui_rect_intersect(child_clip, inner);
        }

        for(c = box->first; c; c = c->next) {
                if(s_in_flow(c)) {
                        c->rect = ui_rect(origin[0] + c->pos[0],
                                        origin[1] + c->pos[1],
                                        c->size[0], c->size[1]);
                        c->clip = ui_rect_intersect(child_clip, c->rect);
                } else {
                        c->rect = ui_rect(c->float_x, c->float_y,
                                        c->size[0], c->size[1]);
                        c->pos[0] = c->float_x - origin[0];
                        c->pos[1] = c->float_y - origin[1];
                        /*
                         * Floating boxes are not moved by scrolling, but
                         * stay inside the parent's visible area (popups sit
                         * on a layer root, which is the whole window)
                         */
                        c->clip = ui_rect_intersect(box->clip, c->rect);
                }
                s_place(c);
        }
}

/* Axis children are placed along, -1 for STACK */
LOC s32 s_main_axis(const UiBox *box)
{
        if(box->style.layout == UI_LAYOUT_ROW) return UI_AXIS_X;
        if(box->style.layout == UI_LAYOUT_COLUMN) return UI_AXIS_Y;
        return -1;
}

LOC f32 s_pad_lo(const UiBox *box, s32 a)
{
        return box->style.padding[a == UI_AXIS_X ? UI_LEFT : UI_TOP];
}

LOC f32 s_pad_hi(const UiBox *box, s32 a)
{
        return box->style.padding[a == UI_AXIS_X ? UI_RIGHT : UI_BOTTOM];
}

LOC f32 s_margin_lo(const UiBox *box, s32 a)
{
        return box->style.margin[a == UI_AXIS_X ? UI_LEFT : UI_TOP];
}

LOC f32 s_margin_hi(const UiBox *box, s32 a)
{
        return box->style.margin[a == UI_AXIS_X ? UI_RIGHT : UI_BOTTOM];
}

/* Size including margins */
LOC f32 s_outer(const UiBox *box, s32 a)
{
        return box->size[a] + s_margin_lo(box, a) + s_margin_hi(box, a);
}

/*
 * Size including margins, as seen by a FIT parent: GROW children count with
 * their content size, so a fitting menu is as wide as its widest item.
 */
LOC f32 s_fit_outer(const UiBox *box, s32 a)
{
        f32 v = s_size_of(box, a)->kind == UI_SIZE_GROW ? box->basis[a] :
                box->size[a];

        return v + s_margin_lo(box, a) + s_margin_hi(box, a);
}

/* Size of the content box */
LOC f32 s_avail(const UiBox *box, s32 a)
{
        f32 v = box->size[a] - s_pad_lo(box, a) - s_pad_hi(box, a);
        return v > 0.0f ? v : 0.0f;
}

LOC const UiSize *s_size_of(const UiBox *box, s32 a)
{
        return a == UI_AXIS_X ? &box->style.width : &box->style.height;
}

LOC f32 s_clamp(const UiSize *s, f32 v)
{
        if(s->max > 0.0f && v > s->max) v = s->max;
        if(v < s->min) v = s->min;
        return v;
}

LOC s32 s_in_flow(const UiBox *box)
{
        return !(box->flags & UI_BOX_FLOATING);
}

/* Offset of a block of size used inside avail */
LOC f32 s_align(f32 avail, f32 used, u8 align)
{
        f32 free_space = avail - used;

        if(free_space <= 0.0f) return 0.0f;
        if(align == UI_ALIGN_CENTER) return free_space * 0.5f;
        if(align == UI_ALIGN_END) return free_space;
        return 0.0f;
}

/* Size of the box's own icon and text along an axis */
LOC f32 s_intrinsic_own(LayoutCtx *ctx, const UiBox *box, s32 a)
{
        f32 text = 0.0f;
        f32 icon = box->icon ? (f32)ICON_SIZE : 0.0f;
        s32 has_text = box->text && box->text_len > 0;

        if(a == UI_AXIS_X) {
                if(has_text && ctx->measure)
                        text = ctx->measure(ctx->user, box->style.font,
                                        box->style.font_size, box->text,
                                        box->text_len);
                if(has_text && box->icon)
                        return icon + box->style.gap + text;
                return icon > text ? icon : text;
        }

        if(has_text)
                text = ctx->line_height ? ctx->line_height(ctx->user,
                                box->style.font, box->style.font_size) :
                        box->style.font_size;
        return icon > text ? icon : text;
}
