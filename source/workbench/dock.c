#include <workbench/dock.h>

#include <devkit/log/log.h>

#include <render/icon_ids.h>
#include <ui/widgets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_GROUP_SIZE 120.0f   /* Pixels a group keeps when resizing */
#define EDGE_ZONE      0.3f     /* Part of a group that counts as an edge */
#define MAX_ACTIONS    8

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Changes to the tree found while building are queued and applied after
 * the build, so no node is freed while it is being walked.
 */
typedef enum ActionKind {
        ACTION_CLOSE = 0,
        ACTION_CLOSE_OTHERS,
        ACTION_SPLIT,
        ACTION_MOVE
} ActionKind;

typedef struct Action {
        u8 kind;
        ViewHandle view;
        DockNode *node;
        DockNode *target;
        u8 zone;
        s32 index;
} Action;

typedef struct BuildCtx {
        Action actions[MAX_ACTIONS];
        s32 count;
} BuildCtx;

LOC DockNode *s_node_new(Dock *dock, u8 kind);
LOC void s_node_free(DockNode *node);
LOC void s_node_free_tree(Dock *dock, DockNode *node);
LOC s32 s_list_insert(List *lst, s32 index, void *item);
LOC void s_list_remove(List *lst, s32 index);
LOC s32 s_child_index(DockNode *parent, DockNode *child);
LOC s32 s_view_index(DockNode *node, ViewHandle view);
LOC DockNode *s_child(DockNode *node, s32 i);
LOC f32 *s_ratio(DockNode *node, s32 i);
LOC void s_replace(Dock *dock, DockNode *old, DockNode *with);
LOC void s_remove_view(DockNode *node, s32 index);
LOC void s_cleanup(Dock *dock, DockNode *node);
LOC DockNode *s_first_group(DockNode *node);
LOC s32 s_contains(DockNode *tree, DockNode *node);
LOC s32 s_count_groups(DockNode *node);

LOC Action *s_queue(BuildCtx *ctx, u8 kind, ViewHandle view,
                DockNode *node);
LOC void s_apply(Dock *dock, BuildCtx *ctx);
LOC void s_build_node(Dock *dock, UI *ui, BuildCtx *ctx, DockNode *node,
                f32 weight);
LOC void s_build_split(Dock *dock, UI *ui, BuildCtx *ctx, DockNode *node,
                f32 weight);
LOC void s_build_group(Dock *dock, UI *ui, BuildCtx *ctx, DockNode *node,
                f32 weight);
LOC void s_build_empty(UI *ui);
LOC void s_reveal_active_tab(UI *ui, DockNode *node, UIBox *scroll);
LOC void s_update_drag(Dock *dock, UI *ui, BuildCtx *ctx);
LOC DockNode *s_find_target(Dock *dock, DockNode *node, f32 x, f32 y,
                u8 *zone_out, s32 *index_out);
LOC void s_drag_overlay(Dock *dock, UI *ui);
LOC void s_tab_menu(Dock *dock, UI *ui, BuildCtx *ctx);
LOC UIBox *s_floating(UI *ui, const char *cls, UIRect r);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 dock_init(Dock *dock, Views *views, DockCloneFn clone,
                void *clone_user)
{
        memset(dock, 0, sizeof(*dock));
        dock->views = views;
        dock->clone = clone;
        dock->clone_user = clone_user;
        dock->next_id = 1;
        dock->menu_key = hash_str("dock-tab-menu", HASH_SEED);

        dock->root = s_node_new(dock, DOCK_TABS);
        if(!dock->root) return -1;
        dock->active_group = dock->root;
        return 0;
}

PUB void dock_deinit(Dock *dock)
{
        if(dock->root) s_node_free_tree(dock, dock->root);
        dock->root = NULL;
        dock->active_group = NULL;
}

PUB void dock_open(Dock *dock, DockNode *group, ViewHandle view)
{
        if(!group) group = dock->active_group;
        if(!group || group->kind != DOCK_TABS) group = s_first_group(dock->root);
        if(!group) return;

        list_push(&group->views, 1, &view);
        group->active = list_count(&group->views) - 1;
        dock->active_group = group;
}

PUB void dock_close(Dock *dock, DockNode *group, s32 index)
{
        ViewHandle *h;
        ViewHandle view;

        if(!group || group->kind != DOCK_TABS) return;
        h = list_get(&group->views, index);
        if(!h) return;

        view = *h;
        s_remove_view(group, index);
        view_destroy(dock->views, view);
        s_cleanup(dock, group);
}

PUB void dock_close_active(Dock *dock)
{
        DockNode *g = dock->active_group;

        if(g && list_count(&g->views) > 0) dock_close(dock, g, g->active);
}

PUB DockNode *dock_split(Dock *dock, DockNode *group, u8 zone,
                ViewHandle view)
{
        DockNode *fresh;
        DockNode *parent;
        DockNode *split;
        u8 axis;
        s32 before;
        s32 idx;
        f32 half;

        if(!group || zone < DOCK_ZONE_LEFT || zone > DOCK_ZONE_BOTTOM)
                return NULL;

        axis = (zone == DOCK_ZONE_LEFT || zone == DOCK_ZONE_RIGHT) ?
                UI_AXIS_X : UI_AXIS_Y;
        before = zone == DOCK_ZONE_LEFT || zone == DOCK_ZONE_TOP;

        fresh = s_node_new(dock, DOCK_TABS);
        if(!fresh) return NULL;
        list_push(&fresh->views, 1, &view);
        fresh->active = 0;

        parent = group->parent;
        if(parent && parent->axis == axis) {
                /* Same direction: become a sibling, share the group's space */
                idx = s_child_index(parent, group);
                half = *s_ratio(parent, idx) * 0.5f;
                *s_ratio(parent, idx) = half;
                if(!before) idx++;
                s_list_insert(&parent->children, idx, &fresh);
                s_list_insert(&parent->ratios, idx, &half);
                fresh->parent = parent;
        } else {
                /* New split in place of the group */
                split = s_node_new(dock, DOCK_SPLIT);
                if(!split) {
                        s_node_free(fresh);
                        return NULL;
                }
                split->axis = axis;
                half = 0.5f;
                s_replace(dock, group, split);
                if(before) {
                        list_push(&split->children, 1, &fresh);
                        list_push(&split->children, 1, &group);
                } else {
                        list_push(&split->children, 1, &group);
                        list_push(&split->children, 1, &fresh);
                }
                list_push(&split->ratios, 1, &half);
                list_push(&split->ratios, 1, &half);
                group->parent = split;
                fresh->parent = split;
        }

        dock->active_group = fresh;
        return fresh;
}

PUB void dock_split_active(Dock *dock, u8 zone)
{
        ViewHandle v = dock_active_view(dock);
        ViewHandle copy;

        if(!view_get(dock->views, v) || !dock->clone) return;
        copy = dock->clone(dock, v, dock->clone_user);
        if(view_get(dock->views, copy))
                dock_split(dock, dock->active_group, zone, copy);
}

PUB void dock_move(Dock *dock, ViewHandle view, DockNode *from, DockNode *to,
                u8 zone, s32 index)
{
        s32 from_idx;
        s32 count;

        if(!from || !to || zone == DOCK_ZONE_NONE) return;
        from_idx = s_view_index(from, view);
        if(from_idx < 0) return;

        if(from == to && zone == DOCK_ZONE_CENTER) {
                from->active = from_idx;
                dock->active_group = from;
                return;
        }
        /* Splitting a group off its own only tab changes nothing */
        if(from == to && zone != DOCK_ZONE_TABBAR &&
           list_count(&from->views) == 1)
                return;

        s_remove_view(from, from_idx);

        if(zone == DOCK_ZONE_CENTER || zone == DOCK_ZONE_TABBAR) {
                count = list_count(&to->views);
                if(zone == DOCK_ZONE_CENTER) index = count;
                if(from == to && from_idx < index) index--;
                if(index < 0) index = 0;
                if(index > count) index = count;
                s_list_insert(&to->views, index, &view);
                to->active = index;
                dock->active_group = to;
        } else {
                dock_split(dock, to, zone, view);
        }

        s_cleanup(dock, from);
}

PUB void dock_build(Dock *dock, UI *ui)
{
        BuildCtx ctx;

        ctx.count = 0;

        if(!s_contains(dock->root, dock->active_group))
                dock->active_group = s_first_group(dock->root);

        s_build_node(dock, ui, &ctx, dock->root, 1.0f);
        s_update_drag(dock, ui, &ctx);
        s_tab_menu(dock, ui, &ctx);
        s_apply(dock, &ctx);
}

PUB ViewHandle dock_active_view(Dock *dock)
{
        ViewHandle none = { POOL_HANDLE_NONE };
        ViewHandle *h;

        if(!dock->active_group ||
           dock->active_group->active < 0 ||
           dock->active_group->active >= list_count(&dock->active_group->views))
                return none;
        h = list_get(&dock->active_group->views, dock->active_group->active);
        return h ? *h : none;
}

PUB s32 dock_group_count(Dock *dock)
{
        return s_count_groups(dock->root);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC DockNode *s_node_new(Dock *dock, u8 kind)
{
        DockNode *n = calloc(1, sizeof(DockNode));

        if(!n) return NULL;
        n->kind = kind;
        n->id = dock->next_id++;
        list_init(&n->children, sizeof(DockNode *), 2);
        list_init(&n->ratios, sizeof(f32), 2);
        list_init(&n->views, sizeof(ViewHandle), 4);
        list_init(&n->tab_rects, sizeof(UIRect), 4);
        return n;
}

LOC void s_node_free(DockNode *node)
{
        list_deinit(&node->children);
        list_deinit(&node->ratios);
        list_deinit(&node->views);
        list_deinit(&node->tab_rects);
        free(node);
}

LOC void s_node_free_tree(Dock *dock, DockNode *node)
{
        s32 i;

        for(i = 0; i < list_count(&node->children); i++)
                s_node_free_tree(dock, s_child(node, i));
        for(i = 0; i < list_count(&node->views); i++)
                view_destroy(dock->views, *(ViewHandle *)list_get(&node->views,
                                        i));
        s_node_free(node);
}

/* devkit List has no insert/remove at an index; shift the items by hand */
LOC s32 s_list_insert(List *lst, s32 index, void *item)
{
        s32 count = list_count(lst);
        u8 *data;

        if(index < 0 || index > count) return -1;
        if(list_push(lst, 1, item) < 0) return -1;

        data = list_get(lst, 0);
        memmove(data + (size_t)(index + 1) * (size_t)lst->item_size,
                        data + (size_t)index * (size_t)lst->item_size,
                        (size_t)(count - index) * (size_t)lst->item_size);
        memcpy(data + (size_t)index * (size_t)lst->item_size, item,
                        (size_t)lst->item_size);
        return 0;
}

LOC void s_list_remove(List *lst, s32 index)
{
        s32 count = list_count(lst);
        u8 *data;

        if(index < 0 || index >= count) return;
        data = list_get(lst, 0);
        memmove(data + (size_t)index * (size_t)lst->item_size,
                        data + (size_t)(index + 1) * (size_t)lst->item_size,
                        (size_t)(count - index - 1) * (size_t)lst->item_size);
        list_pop(lst, 1, NULL);
}

LOC s32 s_child_index(DockNode *parent, DockNode *child)
{
        s32 i;

        for(i = 0; i < list_count(&parent->children); i++) {
                if(s_child(parent, i) == child) return i;
        }
        return -1;
}

LOC s32 s_view_index(DockNode *node, ViewHandle view)
{
        s32 i;

        for(i = 0; i < list_count(&node->views); i++) {
                if(view_handle_equal(*(ViewHandle *)list_get(&node->views, i),
                                        view))
                        return i;
        }
        return -1;
}

LOC DockNode *s_child(DockNode *node, s32 i)
{
        DockNode **p = list_get(&node->children, i);

        return p ? *p : NULL;
}

LOC f32 *s_ratio(DockNode *node, s32 i)
{
        return list_get(&node->ratios, i);
}

/* Put with where old is in the tree */
LOC void s_replace(Dock *dock, DockNode *old, DockNode *with)
{
        DockNode *parent = old->parent;
        s32 idx;

        if(parent) {
                idx = s_child_index(parent, old);
                *(DockNode **)list_get(&parent->children, idx) = with;
        } else {
                dock->root = with;
        }
        with->parent = parent;
}

/* Remove a tab without destroying the view; keep a sensible active tab */
LOC void s_remove_view(DockNode *node, s32 index)
{
        s_list_remove(&node->views, index);
        if(index < node->active) node->active--;
        if(node->active >= list_count(&node->views))
                node->active = list_count(&node->views) - 1;
        if(node->active < 0) node->active = 0;
}

/*
 * Remove an empty group (not the last one) and fold splits that are left
 * with a single child.
 */
LOC void s_cleanup(Dock *dock, DockNode *node)
{
        DockNode *parent;
        DockNode *only;
        f32 sum = 0.0f;
        s32 idx;
        s32 i;

        if(node->kind != DOCK_TABS || list_count(&node->views) > 0) return;
        if(node == dock->root) return;

        parent = node->parent;
        idx = s_child_index(parent, node);
        s_list_remove(&parent->children, idx);
        s_list_remove(&parent->ratios, idx);
        if(dock->active_group == node) dock->active_group = NULL;
        s_node_free(node);

        /* The others share the freed space */
        for(i = 0; i < list_count(&parent->ratios); i++)
                sum += *s_ratio(parent, i);
        for(i = 0; i < list_count(&parent->ratios) && sum > 0.0f; i++)
                *s_ratio(parent, i) /= sum;

        if(list_count(&parent->children) == 1) {
                only = s_child(parent, 0);
                s_replace(dock, parent, only);
                s_node_free(parent);
        }

        if(!dock->active_group) dock->active_group = s_first_group(dock->root);
}

LOC DockNode *s_first_group(DockNode *node)
{
        DockNode *g;
        s32 i;

        if(!node) return NULL;
        if(node->kind == DOCK_TABS) return node;
        for(i = 0; i < list_count(&node->children); i++) {
                g = s_first_group(s_child(node, i));
                if(g) return g;
        }
        return NULL;
}

LOC s32 s_contains(DockNode *tree, DockNode *node)
{
        s32 i;

        if(!tree || !node) return 0;
        if(tree == node) return 1;
        for(i = 0; i < list_count(&tree->children); i++) {
                if(s_contains(s_child(tree, i), node)) return 1;
        }
        return 0;
}

LOC s32 s_count_groups(DockNode *node)
{
        s32 n = 0;
        s32 i;

        if(!node) return 0;
        if(node->kind == DOCK_TABS) return 1;
        for(i = 0; i < list_count(&node->children); i++)
                n += s_count_groups(s_child(node, i));
        return n;
}

LOC Action *s_queue(BuildCtx *ctx, u8 kind, ViewHandle view,
                DockNode *node)
{
        Action *a;

        if(ctx->count >= MAX_ACTIONS) return NULL;
        a = &ctx->actions[ctx->count++];
        memset(a, 0, sizeof(*a));
        a->kind = kind;
        a->view = view;
        a->node = node;
        return a;
}

LOC void s_apply(Dock *dock, BuildCtx *ctx)
{
        Action *a;
        ViewHandle copy;
        s32 idx;
        s32 i;

        for(i = 0; i < ctx->count; i++) {
                a = &ctx->actions[i];

                /* An earlier action may have removed the node */
                if(!s_contains(dock->root, a->node)) continue;
                idx = s_view_index(a->node, a->view);
                if(idx < 0) continue;

                switch(a->kind) {
                case ACTION_CLOSE:
                        dock_close(dock, a->node, idx);
                        break;

                case ACTION_CLOSE_OTHERS:
                        while(list_count(&a->node->views) > 1) {
                                idx = s_view_index(a->node, a->view);
                                dock_close(dock, a->node, idx == 0 ? 1 : 0);
                        }
                        a->node->active = 0;
                        break;

                case ACTION_SPLIT:
                        if(!dock->clone) break;
                        copy = dock->clone(dock, a->view, dock->clone_user);
                        if(view_get(dock->views, copy))
                                dock_split(dock, a->node, a->zone, copy);
                        break;

                case ACTION_MOVE:
                        if(!s_contains(dock->root, a->target)) break;
                        dock_move(dock, a->view, a->node, a->target, a->zone,
                                        a->index);
                        break;
                }
        }
}

LOC void s_build_node(Dock *dock, UI *ui, BuildCtx *ctx, DockNode *node,
                f32 weight)
{
        if(node->kind == DOCK_SPLIT) {
                s_build_split(dock, ui, ctx, node, weight);
        } else {
                s_build_group(dock, ui, ctx, node, weight);
        }
}

LOC void s_build_split(Dock *dock, UI *ui, BuildCtx *ctx, DockNode *node,
                f32 weight)
{
        UIBox *box;
        char id[32];
        f32 total;
        f32 delta;
        f32 dr;
        f32 min_r;
        s32 n = list_count(&node->children);
        s32 i;

        snprintf(id, sizeof(id), "split-%u", node->id);
        box = ui_box_begin(ui, id, "dock-split", 0);
        box->style.layout = node->axis == UI_AXIS_X ? UI_LAYOUT_ROW :
                UI_LAYOUT_COLUMN;
        box->style.width = ui_grow(weight);
        box->style.height = ui_grow(weight);

        for(i = 0; i < n; i++) {
                s_build_node(dock, ui, ctx, s_child(node, i), *s_ratio(node, i));
                if(i == n - 1) break;

                snprintf(id, sizeof(id), "sash-%d", i);
                delta = ui_splitter(ui, id, node->axis, "dock-sash");
                if(delta == 0.0f || !box->state) continue;

                /* Move space between the two neighbors, both keep a minimum */
                total = node->axis == UI_AXIS_X ? box->state->rect.w :
                        box->state->rect.h;
                if(total <= 0.0f) continue;
                dr = delta / total;
                min_r = MIN_GROUP_SIZE / total;
                if(*s_ratio(node, i) + dr < min_r)
                        dr = min_r - *s_ratio(node, i);
                if(*s_ratio(node, i + 1) - dr < min_r)
                        dr = *s_ratio(node, i + 1) - min_r;
                *s_ratio(node, i) += dr;
                *s_ratio(node, i + 1) -= dr;
        }
        ui_box_end(ui); /* dock-split */
}

LOC void s_build_group(Dock *dock, UI *ui, BuildCtx *ctx, DockNode *node,
                f32 weight)
{
        UIBox *group;
        UIBox *bar;
        UIBox *content;
        UIBox *fill;
        UIBox *scroll;
        UIBoxState *st;
        UITabResult res;
        UISignal sig;
        UIRect zero = { 0.0f, 0.0f, 0.0f, 0.0f };
        ViewHandle h;
        View *v;
        Action *a;
        char id[32];
        s32 is_active = node == dock->active_group;
        s32 i;

        snprintf(id, sizeof(id), "group-%u", node->id);
        group = ui_box_begin(ui, id, "editor-group", 0);
        group->style.layout = UI_LAYOUT_COLUMN;
        group->style.width = ui_grow(weight);
        group->style.height = ui_grow(weight);

        /* Tab bar: scrolling tabs, then the group actions */
        bar = ui_box_begin(ui, "bar", "tab-bar", 0);
        bar->style.width = ui_grow(1.0f);

        scroll = ui_scroll_begin(ui, "tabs", "tab-scroll");
        list_clear(&node->tab_rects);
        for(i = 0; i < list_count(&node->views); i++) {
                h = *(ViewHandle *)list_get(&node->views, i);
                v = view_get(dock->views, h);
                if(!v) continue;

                ui_push_id(ui, v->id);
                res = ui_tab(ui, "tab", v->title,
                                v->cls->icon ? v->cls->icon(v) : ICON_NONE,
                                i == node->active, 1,
                                is_active ? "tab" : "tab.inactive");
                ui_pop_id(ui);

                st = ui_state_of(ui, res.key);
                list_push(&node->tab_rects, 1, st ? &st->rect : &zero);

                if(res.sig.pressed) {
                        node->active = i;
                        dock->active_group = node;
                        dock->drag.pending = 1;
                        dock->drag.active = 0;
                        dock->drag.view = h;
                        dock->drag.source = node;
                }
                if(res.sig.dragging && dock->drag.pending &&
                   view_handle_equal(dock->drag.view, h))
                        dock->drag.active = 1;
                if(res.close_clicked) s_queue(ctx, ACTION_CLOSE, h, node);
                if(res.sig.right_clicked) {
                        node->active = i;
                        dock->active_group = node;
                        dock->menu_view = h;
                        dock->menu_node = node;
                        ui_popup_open(ui, dock->menu_key, ui->input->mouse_x,
                                        ui->input->mouse_y);
                }
        }
        fill = ui_box(ui, "fill", "tab-bar-fill", UI_BOX_CLICKABLE);
        fill->style.width = ui_grow(1.0f);
        if(ui_signal(ui, fill).pressed) dock->active_group = node;
        ui_scroll_end(ui);
        s_reveal_active_tab(ui, node, scroll);

        ui_box_begin(ui, NULL, "tab-actions", 0);
        if(list_count(&node->views) > 0) {
                h = *(ViewHandle *)list_get(&node->views, node->active);
                if(ui_icon_button(ui, "split-right", ICON_SPLIT_HORIZONTAL,
                                        "Split Editor Right", NULL).clicked) {
                        a = s_queue(ctx, ACTION_SPLIT, h, node);
                        if(a) a->zone = DOCK_ZONE_RIGHT;
                }
                if(ui_icon_button(ui, "split-down", ICON_SPLIT_VERTICAL,
                                        "Split Editor Down", NULL).clicked) {
                        a = s_queue(ctx, ACTION_SPLIT, h, node);
                        if(a) a->zone = DOCK_ZONE_BOTTOM;
                }
        }
        ui_box_end(ui); /* tab-actions */

        ui_box_end(ui); /* tab-bar */
        node->tabbar_rect = bar->state ? bar->state->rect : zero;

        /* Content of the shown view */
        content = ui_box_begin(ui, "content", "editor-content",
                        UI_BOX_CLICKABLE);
        content->style.width = ui_grow(1.0f);
        content->style.height = ui_grow(1.0f);
        content->style.layout = UI_LAYOUT_COLUMN;

        v = NULL;
        if(list_count(&node->views) > 0) {
                h = *(ViewHandle *)list_get(&node->views, node->active);
                v = view_get(dock->views, h);
        }
        if(v && v->cls->draw) {
                ui_push_id(ui, v->id);
                v->cls->draw(v, ui, content);
                ui_pop_id(ui);
        } else {
                s_build_empty(ui);
        }

        sig = ui_signal(ui, content);
        if(sig.pressed || sig.right_clicked) dock->active_group = node;
        ui_box_end(ui); /* editor-content */
        node->content_rect = content->state ? content->state->rect : zero;

        ui_box_end(ui); /* editor-group */
}

/*
 * When the shown tab changes, scroll the tab bar so it is fully visible.
 * Uses last frame's rects; a new tab has none yet and waits a frame.
 */
LOC void s_reveal_active_tab(UI *ui, DockNode *node, UIBox *scroll)
{
        ViewHandle *h = list_get(&node->views, node->active);
        UIRect *r = list_get(&node->tab_rects, node->active);
        UIBoxState *st = scroll->state;
        UIRect view;

        if(!h || !r || !st || r->w <= 0.0f) return;
        if(view_handle_equal(*h, node->revealed)) return;

        view = st->rect;
        if(r->x < view.x) {
                st->scroll[0] -= view.x - r->x;
        } else if(r->x + r->w > view.x + view.w) {
                st->scroll[0] += r->x + r->w - (view.x + view.w);
        }
        node->revealed = *h;
        ui->changed = 1;
}

/* Shown when no editor is open */
LOC void s_build_empty(UI *ui)
{
        ui_box_begin(ui, NULL, "empty-editor", 0);
        ui_label(ui, "Vasall Studio", "empty-editor-title");
        ui_label(ui, "New Dummy Tab        Ctrl+N", "empty-editor-hint");
        ui_label(ui, "Toggle Side Bar       Ctrl+B", "empty-editor-hint");
        ui_label(ui, "Toggle Panel          Ctrl+J", "empty-editor-hint");
        ui_label(ui, "Debug Overlay         F12", "empty-editor-hint");
        ui_box_end(ui); /* empty-editor */
}

LOC void s_update_drag(Dock *dock, UI *ui, BuildCtx *ctx)
{
        DockDrag *d = &dock->drag;
        Action *a;

        if(!d->pending) return;

        /* Released: drop */
        if(!ui->input->button_down[UI_MOUSE_LEFT]) {
                a = NULL;
                if(d->active && d->zone != DOCK_ZONE_NONE)
                        a = s_queue(ctx, ACTION_MOVE, d->view, d->source);
                if(a) {
                        a->target = d->target;
                        a->zone = d->zone;
                        a->index = d->index;
                }
                memset(d, 0, sizeof(*d));
                return;
        }

        if(!d->active) return;
        if(!s_contains(dock->root, d->source) ||
           s_view_index(d->source, d->view) < 0) {
                memset(d, 0, sizeof(*d));
                return;
        }

        d->target = s_find_target(dock, dock->root, ui->input->mouse_x,
                        ui->input->mouse_y, &d->zone, &d->index);
        s_drag_overlay(dock, ui);
}

/* The group and zone under (x, y), from last frame's rects */
LOC DockNode *s_find_target(Dock *dock, DockNode *node, f32 x, f32 y,
                u8 *zone_out, s32 *index_out)
{
        DockNode *hit;
        UIRect *r;
        UIRect c;
        f32 rx;
        f32 ry;
        f32 edge;
        s32 i;

        *zone_out = DOCK_ZONE_NONE;
        *index_out = 0;

        if(node->kind == DOCK_SPLIT) {
                for(i = 0; i < list_count(&node->children); i++) {
                        hit = s_find_target(dock, s_child(node, i), x, y,
                                        zone_out, index_out);
                        if(hit) return hit;
                }
                return NULL;
        }

        if(ui_rect_contains(node->tabbar_rect, x, y)) {
                /* Insert before the first tab whose middle is right of x */
                *zone_out = DOCK_ZONE_TABBAR;
                *index_out = list_count(&node->tab_rects);
                for(i = 0; i < list_count(&node->tab_rects); i++) {
                        r = list_get(&node->tab_rects, i);
                        if(x < r->x + r->w * 0.5f) {
                                *index_out = i;
                                break;
                        }
                }
                return node;
        }

        c = node->content_rect;
        if(!ui_rect_contains(c, x, y) || c.w <= 0.0f || c.h <= 0.0f)
                return NULL;

        /* Nearest edge within EDGE_ZONE, else the center */
        rx = (x - c.x) / c.w;
        ry = (y - c.y) / c.h;
        *zone_out = DOCK_ZONE_CENTER;
        edge = EDGE_ZONE;
        if(rx < edge) {
                edge = rx;
                *zone_out = DOCK_ZONE_LEFT;
        }
        if(1.0f - rx < edge) {
                edge = 1.0f - rx;
                *zone_out = DOCK_ZONE_RIGHT;
        }
        if(ry < edge) {
                edge = ry;
                *zone_out = DOCK_ZONE_TOP;
        }
        if(1.0f - ry < edge) {
                *zone_out = DOCK_ZONE_BOTTOM;
        }

        /* A group can't be split off its own only tab */
        if(node == dock->drag.source && list_count(&node->views) == 1 &&
           *zone_out != DOCK_ZONE_CENTER)
                *zone_out = DOCK_ZONE_NONE;
        return node;
}

LOC void s_drag_overlay(Dock *dock, UI *ui)
{
        DockDrag *d = &dock->drag;
        DockNode *t = d->target;
        UIBox *preview;
        UIRect c;
        UIRect *r;
        View *v;
        f32 x;

        ui_layer_begin(ui, UI_LAYER_DRAG);

        if(t && d->zone == DOCK_ZONE_TABBAR) {
                /* Insertion marker between tabs */
                x = t->tabbar_rect.x;
                if(d->index < list_count(&t->tab_rects)) {
                        r = list_get(&t->tab_rects, d->index);
                        x = r->x;
                } else if(list_count(&t->tab_rects) > 0) {
                        r = list_get(&t->tab_rects,
                                        list_count(&t->tab_rects) - 1);
                        x = r->x + r->w;
                }
                s_floating(ui, "drop-zone", t->tabbar_rect);
                s_floating(ui, "drop-insert", ui_rect(x - 1.0f,
                                        t->tabbar_rect.y, 2.0f,
                                        t->tabbar_rect.h));
        } else if(t && d->zone != DOCK_ZONE_NONE) {
                c = t->content_rect;
                switch(d->zone) {
                case DOCK_ZONE_LEFT:
                        c.w *= 0.5f;
                        break;
                case DOCK_ZONE_RIGHT:
                        c.x += c.w * 0.5f;
                        c.w *= 0.5f;
                        break;
                case DOCK_ZONE_TOP:
                        c.h *= 0.5f;
                        break;
                case DOCK_ZONE_BOTTOM:
                        c.y += c.h * 0.5f;
                        c.h *= 0.5f;
                        break;
                default:
                        break;
                }
                s_floating(ui, "drop-zone", c);
        }

        /* The tab title follows the mouse */
        v = view_get(dock->views, d->view);
        if(v) {
                preview = ui_box(ui, NULL, "drag-preview", UI_BOX_FLOATING |
                                UI_BOX_PASSIVE);
                preview->icon = v->cls->icon ? v->cls->icon(v) : ICON_NONE;
                ui_box_set_text(ui, preview, v->title);
                preview->float_x = ui->input->mouse_x + 14.0f;
                preview->float_y = ui->input->mouse_y + 10.0f;
        }

        ui_layer_end(ui);
}

LOC void s_tab_menu(Dock *dock, UI *ui, BuildCtx *ctx)
{
        Action *a;

        if(!ui_menu_begin(ui, dock->menu_key)) return;

        if(ui_menu_item(ui, "Close", "Ctrl+W"))
                s_queue(ctx, ACTION_CLOSE, dock->menu_view, dock->menu_node);
        if(ui_menu_item(ui, "Close Others", NULL))
                s_queue(ctx, ACTION_CLOSE_OTHERS, dock->menu_view,
                                dock->menu_node);
        ui_menu_separator(ui);
        if(ui_menu_item(ui, "Split Right", "Ctrl+\\")) {
                a = s_queue(ctx, ACTION_SPLIT, dock->menu_view,
                                dock->menu_node);
                if(a) a->zone = DOCK_ZONE_RIGHT;
        }
        if(ui_menu_item(ui, "Split Down", "Ctrl+Shift+\\")) {
                a = s_queue(ctx, ACTION_SPLIT, dock->menu_view,
                                dock->menu_node);
                if(a) a->zone = DOCK_ZONE_BOTTOM;
        }
        ui_menu_end(ui);
}

/* A passive floating box at an absolute rect */
LOC UIBox *s_floating(UI *ui, const char *cls, UIRect r)
{
        UIBox *b = ui_box(ui, NULL, cls, UI_BOX_FLOATING | UI_BOX_PASSIVE);

        b->style.width = ui_px(r.w);
        b->style.height = ui_px(r.h);
        b->float_x = r.x;
        b->float_y = r.y;
        return b;
}
