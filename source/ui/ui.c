#include <ui/ui.h>
#include <ui/debug.h>

#include <devkit/log/log.h>

#include <render/icon_ids.h>

#include <string.h>

#define FRAME_ARENA_BLOCK (256 * 1024)
#define WHEEL_STEP        48.0f

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC UiBoxState *s_state_get_or_add(Ui *ui, u64 key);
LOC void s_store_rects(Ui *ui, UiBox *box, UiBox **active_box);
LOC UiBox *s_hit_test(UiBox *box, f32 x, f32 y, u32 need_flags);
LOC UiBox *s_scroll_target(UiBox *box, f32 x, f32 y, f32 wx, f32 wy);
LOC void s_emit(Ui *ui, UiBox *box);
LOC void s_prune(Ui *ui);
LOC UiBox *s_new_root(Ui *ui, s32 layer);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 ui_init(Ui *ui, const UiTheme *theme, UiMeasureFn measure,
                UiLineHeightFn line_height, void *measure_user)
{
        if(!ui) return -1;

        memset(ui, 0, sizeof(*ui));
        if(arena_init(&ui->arena, FRAME_ARENA_BLOCK) < 0) return -1;
        if(pool_init(&ui->states, sizeof(UiBoxState), 256) < 0) goto err_pool;
        if(hashmap_init(&ui->state_map, 512) < 0) goto err_map;
        if(list_init(&ui->cmds, sizeof(UiDrawCmd), 1024) < 0) goto err_cmds;

        ui->theme = theme;
        ui->measure = measure;
        ui->line_height = line_height;
        ui->measure_user = measure_user;
        ui->frame = 1;
        return 0;

err_cmds:
        hashmap_deinit(&ui->state_map);
err_map:
        pool_deinit(&ui->states);
err_pool:
        arena_deinit(&ui->arena);
        return -1;
}

PUB void ui_deinit(Ui *ui)
{
        if(!ui) return;
        list_deinit(&ui->cmds);
        hashmap_deinit(&ui->state_map);
        pool_deinit(&ui->states);
        arena_deinit(&ui->arena);
}

PUB void ui_begin_frame(Ui *ui, UiInput *input, f32 width, f32 height,
                f64 time)
{
        s32 i;

        arena_reset(&ui->arena);

        ui->input = input;
        ui->width = width;
        ui->height = height;
        ui->time = time;
        ui->box_count = 0;
        ui->seed_depth = 0;
        ui->changed = 0;

        if(ui->wakeup > 0.0 && ui->wakeup <= time) ui->wakeup = 0.0;

        for(i = 0; i < UI_LAYER_COUNT; i++)
                ui->roots[i] = s_new_root(ui, i);

        ui->parents[0] = ui->roots[UI_LAYER_BASE];
        ui->depth = 1;
}

PUB s32 ui_end_frame(Ui *ui)
{
        UiInput *in = ui->input;
        UiBox *active_box = NULL;
        UiBox *hit = NULL;
        u64 old_hot = ui->hot;
        u64 old_active = ui->active;
        s32 hit_layer = -1;
        s32 i;

        if(ui->depth != 1) {
                LOG_I(LOG_WARN, "ui: %d unclosed boxes at end of frame",
                                ui->depth - 1);
        }

        /* Layout every layer with the window size */
        for(i = 0; i < UI_LAYER_COUNT; i++) {
                ui_layout(ui->roots[i], ui->measure, ui->line_height,
                                ui->measure_user);
                s_store_rects(ui, ui->roots[i], &active_box);
        }

        /* Hit test from the top layer down; the debug layer never takes input */
        if(in && in->mouse_inside) {
                for(i = UI_LAYER_DEBUG - 1; i >= 0 && !hit; i--) {
                        hit = s_hit_test(ui->roots[i], in->mouse_x,
                                        in->mouse_y, UI_BOX_CLICKABLE);
                        if(hit) hit_layer = i;
                }
        }
        ui->hot_layer = hit_layer;
        if(ui->active && (!hit || hit->key != ui->active)) {
                /* While a box owns the mouse nothing else is hovered */
                ui->hot = 0;
        } else {
                ui->hot = hit ? hit->key : 0;
        }

        if(in) {
                /* Wheel scrolling */
                if(in->wheel_x != 0.0f || in->wheel_y != 0.0f) {
                        UiBox *sb = NULL;
                        for(i = UI_LAYER_DEBUG - 1; i >= 0 && !sb; i--) {
                                sb = s_scroll_target(ui->roots[i], in->mouse_x,
                                                in->mouse_y, in->wheel_x,
                                                in->wheel_y);
                        }
                        if(sb && sb->state) {
                                f32 wx = in->wheel_x;
                                f32 wy = in->wheel_y;
                                UiBoxState *st = sb->state;

                                /* Only horizontal room: the wheel scrolls x */
                                if(st->content[1] <= st->view[1] && wx == 0.0f) {
                                        wx = -wy;
                                        wy = 0.0f;
                                }
                                st->scroll[0] += wx * WHEEL_STEP;
                                st->scroll[1] -= wy * WHEEL_STEP;

                                /* Stay inside the content */
                                for(i = 0; i < 2; i++) {
                                        f32 max = st->content[i] - st->view[i];
                                        if(max < 0.0f) max = 0.0f;
                                        if(st->scroll[i] > max)
                                                st->scroll[i] = max;
                                        if(st->scroll[i] < 0.0f)
                                                st->scroll[i] = 0.0f;
                                }
                                ui->changed = 1;
                        }
                }

                /* Close the popup on a click outside of it */
                if(ui->popup_key && ui->popup_frame != ui->frame &&
                   (in->button_pressed[UI_MOUSE_LEFT] ||
                    in->button_pressed[UI_MOUSE_RIGHT]) &&
                   hit_layer != UI_LAYER_POPUP) {
                        ui_popup_close(ui);
                }

                /* Release ends the active box */
                if(in->button_released[UI_MOUSE_LEFT] ||
                   !in->button_down[UI_MOUSE_LEFT]) {
                        ui->active = 0;
                        ui->dragging = 0;
                }

                /* Esc drops keyboard focus if nobody used it */
                for(i = 0; i < in->event_count; i++) {
                        if(ui_event_is_key(&in->events[i], UI_KEY_ESCAPE,
                                                UI_SCAN_NONE, 0)) {
                                if(ui->popup_key) {
                                        ui_popup_close(ui);
                                } else {
                                        ui->focus = 0;
                                }
                                in->events[i].consumed = 1;
                                ui->changed = 1;
                        }
                }
        }

        /* Cursor of the active box, else of the hovered one */
        ui->cursor = UI_CURSOR_DEFAULT;
        if(active_box) {
                ui->cursor = active_box->cursor;
        } else if(hit) {
                ui->cursor = hit->cursor;
        }

        /* Tooltip timer restarts when the hovered box changes */
        if(ui->hot != ui->tooltip_key) {
                ui->tooltip_key = ui->hot;
                ui->tooltip_start = ui->time;
        }

        /* Draw list */
        list_clear(&ui->cmds);
        for(i = 0; i < UI_LAYER_DEBUG; i++)
                s_emit(ui, ui->roots[i]);
        if(ui->debug) ui_debug_outlines(ui);
        s_emit(ui, ui->roots[UI_LAYER_DEBUG]);

        s_prune(ui);

        if(ui->hot != old_hot || ui->active != old_active) ui->changed = 1;
        ui->frame++;
        return ui->changed;
}

PUB void ui_layer_begin(Ui *ui, UiLayer layer)
{
        ui_push_parent(ui, ui->roots[layer]);
}

PUB void ui_layer_end(Ui *ui)
{
        ui_pop_parent(ui);
}

PUB UiBox *ui_box(Ui *ui, const char *id, const char *cls, u32 flags)
{
        UiBox *parent = ui_parent(ui);
        UiBox *box;
        u32 states = 0;

        box = arena_alloc(&ui->arena, sizeof(UiBox));
        if(!box) return NULL;
        ui->box_count++;

        box->flags = flags;
        box->line = 0;

        box->seed = parent ? parent->seed : HASH_SEED;
        if(id) {
                u64 seed = box->seed;
                if(ui->seed_depth > 0) seed ^= ui->seeds[ui->seed_depth - 1];
                box->key = hash_str(id, seed);
                if(box->key == 0) box->key = 1;
                box->seed = box->key;

                box->state = s_state_get_or_add(ui, box->key);
                if(box->state) {
                        if(box->state->last_frame == ui->frame) {
                                LOG_S(LOG_WARN, "ui: id '%s' used twice "
                                                "under one parent", (char *)id);
                        }
                        box->state->last_frame = ui->frame;
                }

                if(!(flags & UI_BOX_DISABLED)) {
                        if(ui->hot == box->key) states |= 1u << UI_STATE_HOVER;
                        if(ui->active == box->key && ui->hot == box->key)
                                states |= 1u << UI_STATE_PRESSED;
                        if(ui->focus == box->key)
                                states |= 1u << UI_STATE_FOCUSED;
                }
        }
        if(flags & UI_BOX_SELECTED) states |= 1u << UI_STATE_SELECTED;
        if(flags & UI_BOX_DISABLED) states |= 1u << UI_STATE_DISABLED;

        box->cls = cls;
        box->style = ui_theme_style(ui->theme, cls, states);

        /* Link as last child */
        if(parent) {
                box->parent = parent;
                box->prev = parent->last;
                if(parent->last) {
                        parent->last->next = box;
                } else {
                        parent->first = box;
                }
                parent->last = box;
                parent->child_count++;
        }
        return box;
}

PUB UiBox *ui_box_begin(Ui *ui, const char *id, const char *cls, u32 flags)
{
        UiBox *box = ui_box(ui, id, cls, flags);

        ui_push_parent(ui, box);
        return box;
}

PUB void ui_box_end(Ui *ui)
{
        ui_pop_parent(ui);
}

PUB void ui_push_parent(Ui *ui, UiBox *box)
{
        if(ui->depth >= UI_MAX_DEPTH) {
                LOG(LOG_ERROR, "ui: boxes nested too deep");
                return;
        }
        ui->parents[ui->depth++] = box;
}

PUB void ui_pop_parent(Ui *ui)
{
        if(ui->depth <= 1) {
                LOG(LOG_ERROR, "ui: ui_box_end without ui_box_begin");
                return;
        }
        ui->depth--;
}

PUB UiBox *ui_parent(Ui *ui)
{
        return ui->depth > 0 ? ui->parents[ui->depth - 1] : NULL;
}

PUB void ui_push_id(Ui *ui, u64 seed)
{
        u64 base = ui->seed_depth > 0 ? ui->seeds[ui->seed_depth - 1] : 0;

        if(ui->seed_depth >= UI_MAX_ID_SEEDS) {
                LOG(LOG_ERROR, "ui: ui_push_id nested too deep");
                return;
        }
        ui->seeds[ui->seed_depth++] = hash_bytes(&seed, sizeof(seed),
                        base ^ HASH_SEED);
}

PUB void ui_pop_id(Ui *ui)
{
        if(ui->seed_depth > 0) ui->seed_depth--;
}

PUB void ui_box_set_text(Ui *ui, UiBox *box, const char *text)
{
        s32 len;

        if(!box || !text) return;
        len = ui_display_len(text);
        box->text = arena_strdup(&ui->arena, text, len);
        box->text_len = len;
}

PUB UiBoxState *ui_state_of(Ui *ui, u64 key)
{
        u64 packed;
        PoolHandle h;

        if(!hashmap_get(&ui->state_map, key, &packed)) return NULL;
        h = pool_handle((s32)(packed >> 32), (u32)packed);
        return pool_get(&ui->states, h);
}

PUB void ui_set_focus(Ui *ui, u64 key)
{
        if(ui->focus != key) ui->changed = 1;
        ui->focus = key;
}

PUB void ui_request_wakeup(Ui *ui, f64 t)
{
        if(ui->wakeup == 0.0 || t < ui->wakeup) ui->wakeup = t;
}

PUB void ui_popup_open(Ui *ui, u64 key, f32 x, f32 y)
{
        ui->popup_key = key;
        ui->popup_x = x;
        ui->popup_y = y;
        ui->popup_frame = ui->frame;
        ui->changed = 1;
}

PUB void ui_popup_close(Ui *ui)
{
        if(ui->popup_key) ui->changed = 1;
        ui->popup_key = 0;
}

PUB s32 ui_popup_is_open(Ui *ui, u64 key)
{
        return key != 0 && ui->popup_key == key;
}

PUB u64 ui_key(Ui *ui, const char *id)
{
        UiBox *parent = ui_parent(ui);
        u64 seed = parent ? parent->seed : HASH_SEED;
        u64 key;

        if(ui->seed_depth > 0) seed ^= ui->seeds[ui->seed_depth - 1];
        key = hash_str(id, seed);
        return key ? key : 1;
}

PUB void ui_draw_push(Ui *ui, const UiDrawCmd *cmd)
{
        list_push(&ui->cmds, 1, (void *)cmd);
}

PUB s32 ui_display_len(const char *text)
{
        const char *hash;

        if(!text) return 0;
        hash = strstr(text, "##");
        return hash ? (s32)(hash - text) : (s32)strlen(text);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC UiBoxState *s_state_get_or_add(Ui *ui, u64 key)
{
        UiBoxState *st;
        PoolHandle h;

        st = ui_state_of(ui, key);
        if(st) return st;

        st = pool_add(&ui->states, NULL, &h);
        if(!st) return NULL;
        st->key = key;
        hashmap_set(&ui->state_map, key,
                        ((u64)(u32)h.index << 32) | (u64)h.generation);
        return st;
}

LOC UiBox *s_new_root(Ui *ui, s32 layer)
{
        UiBox *root;
        char id[16];

        root = arena_alloc(&ui->arena, sizeof(UiBox));
        root->style = ui_style_default();
        root->style.width = ui_px(ui->width);
        root->style.height = ui_px(ui->height);
        root->style.layout = UI_LAYOUT_STACK;

        id[0] = 'L';
        id[1] = (char)('0' + layer);
        id[2] = '\0';
        root->key = hash_str(id, HASH_SEED);
        root->seed = root->key;
        return root;
}

/* Remember rects for next frame's hit testing; find the active box */
LOC void s_store_rects(Ui *ui, UiBox *box, UiBox **active_box)
{
        UiBox *c;

        if(box->state) {
                box->state->rect = box->rect;
                box->state->clip = box->clip;
        }
        if(box->key && box->key == ui->active) *active_box = box;

        for(c = box->first; c; c = c->next)
                s_store_rects(ui, c, active_box);
}

/* Topmost box with need_flags under (x, y): later children are on top */
LOC UiBox *s_hit_test(UiBox *box, f32 x, f32 y, u32 need_flags)
{
        UiBox *c;
        UiBox *hit;

        /* Floating children can stick out of their parent, so always look */
        for(c = box->last; c; c = c->prev) {
                hit = s_hit_test(c, x, y, need_flags);
                if(hit) return hit;
        }

        if((box->flags & need_flags) == need_flags &&
           !(box->flags & (UI_BOX_DISABLED | UI_BOX_PASSIVE)) &&
           box->key && ui_rect_contains(box->clip, x, y))
                return box;
        return NULL;
}

/* Innermost scrollable box under (x, y) that can move in the wheel's way */
LOC UiBox *s_scroll_target(UiBox *box, f32 x, f32 y, f32 wx, f32 wy)
{
        UiBox *c;
        UiBox *hit;
        UiBoxState *st;

        if(!ui_rect_contains(box->clip, x, y) && !(box->flags & UI_BOX_FLOATING))
                return NULL;

        for(c = box->last; c; c = c->prev) {
                hit = s_scroll_target(c, x, y, wx, wy);
                if(hit) return hit;
        }

        st = box->state;
        if(box->style.overflow == UI_OVERFLOW_SCROLL && st &&
           ui_rect_contains(box->clip, x, y)) {
                s32 can_x = st->content[0] > st->view[0] + 0.5f;
                s32 can_y = st->content[1] > st->view[1] + 0.5f;

                if((wy != 0.0f && (can_y || can_x)) || (wx != 0.0f && can_x))
                        return box;
        }
        return NULL;
}

LOC void s_emit(Ui *ui, UiBox *box)
{
        UiDrawCmd cmd;
        UiBox *c;
        UiStyle *s = &box->style;
        s32 visible = box->clip.w > 0.0f && box->clip.h > 0.0f;
        f32 cx;
        f32 cy;
        f32 cw;
        f32 ch;
        f32 text_w = 0.0f;
        f32 line_h = 0.0f;
        f32 block_w;
        f32 x;

        if(visible && (s->infill.a > 0 ||
                       (s->border_width > 0.0f && s->border_color.a > 0))) {
                memset(&cmd, 0, sizeof(cmd));
                cmd.kind = UI_DRAW_RECT;
                cmd.rect = box->rect;
                cmd.clip = box->clip;
                cmd.color = s->infill;
                cmd.border_color = s->border_color;
                cmd.border_width = s->border_width;
                memcpy(cmd.corner_radius, s->corner_radius,
                                sizeof(cmd.corner_radius));
                ui_draw_push(ui, &cmd);
        }

        if(visible && (box->icon || (box->text && box->text_len > 0))) {
                /* Icon and text sit in the content box, aligned as a block */
                cx = box->rect.x + s->padding[UI_LEFT];
                cy = box->rect.y + s->padding[UI_TOP];
                cw = box->rect.w - s->padding[UI_LEFT] - s->padding[UI_RIGHT];
                ch = box->rect.h - s->padding[UI_TOP] - s->padding[UI_BOTTOM];

                if(box->text && box->text_len > 0) {
                        text_w = ui->measure ? ui->measure(ui->measure_user,
                                        s->font, s->font_size, box->text,
                                        box->text_len) : 0.0f;
                        line_h = ui->line_height ? ui->line_height(
                                        ui->measure_user, s->font,
                                        s->font_size) : s->font_size;
                }
                block_w = text_w;
                if(box->icon) {
                        block_w += (f32)ICON_SIZE;
                        if(text_w > 0.0f) block_w += s->gap;
                }

                x = cx;
                if(s->align_x == UI_ALIGN_CENTER && cw > block_w)
                        x += (cw - block_w) * 0.5f;
                else if(s->align_x == UI_ALIGN_END && cw > block_w)
                        x += cw - block_w;

                if(box->icon) {
                        f32 iy = cy;

                        if(s->align_y == UI_ALIGN_CENTER)
                                iy += (ch - (f32)ICON_SIZE) * 0.5f;
                        else if(s->align_y == UI_ALIGN_END)
                                iy += ch - (f32)ICON_SIZE;

                        memset(&cmd, 0, sizeof(cmd));
                        cmd.kind = UI_DRAW_ICON;
                        cmd.rect = ui_rect(x, iy, (f32)ICON_SIZE,
                                        (f32)ICON_SIZE);
                        cmd.clip = box->clip;
                        cmd.color = s->text_color;
                        cmd.icon = box->icon;
                        ui_draw_push(ui, &cmd);
                        x += (f32)ICON_SIZE + s->gap;
                }

                if(text_w > 0.0f) {
                        f32 ty = cy;

                        if(s->align_y == UI_ALIGN_CENTER)
                                ty += (ch - line_h) * 0.5f;
                        else if(s->align_y == UI_ALIGN_END)
                                ty += ch - line_h;

                        memset(&cmd, 0, sizeof(cmd));
                        cmd.kind = UI_DRAW_TEXT;
                        cmd.rect = ui_rect(x, ty, text_w, line_h);
                        cmd.clip = box->clip;
                        cmd.color = s->text_color;
                        cmd.text = box->text;
                        cmd.text_len = box->text_len;
                        cmd.font = s->font;
                        cmd.font_size = s->font_size;
                        ui_draw_push(ui, &cmd);
                }
        }

        if(box->custom_draw && visible)
                box->custom_draw(ui, box, box->custom_data);

        for(c = box->first; c; c = c->next)
                s_emit(ui, c);
}

/* Free the state of boxes that weren't built this frame */
LOC void s_prune(Ui *ui)
{
        PoolHandle h;
        UiBoxState *st;
        s32 i;

        for(i = 0; i < pool_slots(&ui->states); i++) {
                st = pool_at(&ui->states, i, &h);
                if(!st || st->last_frame == ui->frame) continue;

                if(st->key == ui->focus) ui->focus = 0;
                if(st->key == ui->active) ui->active = 0;
                if(st->key == ui->hot) ui->hot = 0;

                hashmap_remove(&ui->state_map, st->key);
                pool_remove(&ui->states, h);
        }
}
