#include "unity.h"

#include <string.h>

#include <ui/ui.h>
#include <workbench/dock.h>
#include <workbench/dummy_view.h>

LOC Views s_views;
LOC Dock s_dock;

LOC ViewHandle s_clone(Dock *dock, ViewHandle view, void *user)
{
        (void)user;
        return dummy_view_create(dock->views, view_get(dock->views,
                                view)->title);
}

void setUp(void)
{
        views_init(&s_views);
        dock_init(&s_dock, &s_views, s_clone, NULL);
}

void tearDown(void)
{
        dock_deinit(&s_dock);
        views_deinit(&s_views);
}

LOC ViewHandle s_open(const char *title)
{
        ViewHandle h = dummy_view_create(&s_views, title);

        dock_open(&s_dock, NULL, h);
        return h;
}

LOC DockNode *s_child(DockNode *n, s32 i)
{
        return *(DockNode **)list_get(&n->children, i);
}

LOC ViewHandle s_view_at(DockNode *n, s32 i)
{
        return *(ViewHandle *)list_get(&n->views, i);
}

void test_open_and_close(void)
{
        ViewHandle a = s_open("a");
        ViewHandle b = s_open("b");

        TEST_ASSERT_EQUAL_INT(2, list_count(&s_dock.root->views));
        TEST_ASSERT_EQUAL_INT(1, s_dock.root->active);

        dock_close_active(&s_dock);
        TEST_ASSERT_EQUAL_INT(1, list_count(&s_dock.root->views));
        TEST_ASSERT_NULL(view_get(&s_views, b));
        TEST_ASSERT_NOT_NULL(view_get(&s_views, a));

        /* The last group stays, even when empty */
        dock_close_active(&s_dock);
        TEST_ASSERT_EQUAL_INT(DOCK_TABS, s_dock.root->kind);
        TEST_ASSERT_EQUAL_INT(0, list_count(&s_dock.root->views));
}

void test_empty_group_has_no_active_view(void)
{
        s_open("a");
        dock_close_active(&s_dock);
        dock_close_active(&s_dock);

        TEST_ASSERT_EQUAL_INT(0, list_count(&s_dock.root->views));
        TEST_ASSERT_NULL(view_get(&s_views, dock_active_view(&s_dock)));
}

void test_split_creates_group_and_cleanup_folds_it(void)
{
        DockNode *left = s_dock.root;
        DockNode *right;
        ViewHandle a;
        ViewHandle b;

        a = s_open("a");
        b = dummy_view_create(&s_views, "b");
        right = dock_split(&s_dock, left, DOCK_ZONE_RIGHT, b);

        TEST_ASSERT_EQUAL_INT(DOCK_SPLIT, s_dock.root->kind);
        TEST_ASSERT_EQUAL_INT(UI_AXIS_X, s_dock.root->axis);
        TEST_ASSERT_EQUAL_PTR(left, s_child(s_dock.root, 0));
        TEST_ASSERT_EQUAL_PTR(right, s_child(s_dock.root, 1));
        TEST_ASSERT_EQUAL_PTR(right, s_dock.active_group);
        TEST_ASSERT_EQUAL_INT(2, dock_group_count(&s_dock));

        /* Moving the only tab of the right group back folds the split */
        dock_move(&s_dock, b, right, left, DOCK_ZONE_CENTER, 0);
        TEST_ASSERT_EQUAL_INT(DOCK_TABS, s_dock.root->kind);
        TEST_ASSERT_EQUAL_PTR(left, s_dock.root);
        TEST_ASSERT_EQUAL_INT(2, list_count(&left->views));
        TEST_ASSERT_TRUE(view_handle_equal(a, s_view_at(left, 0)));
        TEST_ASSERT_TRUE(view_handle_equal(b, s_view_at(left, 1)));
}

void test_split_same_axis_adds_sibling(void)
{
        DockNode *g1 = s_dock.root;
        DockNode *g2;
        DockNode *g3;
        f32 r0;
        f32 r1;
        f32 r2;

        s_open("a");
        g2 = dock_split(&s_dock, g1, DOCK_ZONE_RIGHT,
                        dummy_view_create(&s_views, "b"));
        g3 = dock_split(&s_dock, g2, DOCK_ZONE_RIGHT,
                        dummy_view_create(&s_views, "c"));

        TEST_ASSERT_EQUAL_INT(3, list_count(&s_dock.root->children));
        TEST_ASSERT_EQUAL_PTR(g3, s_child(s_dock.root, 2));
        r0 = *(f32 *)list_get(&s_dock.root->ratios, 0);
        r1 = *(f32 *)list_get(&s_dock.root->ratios, 1);
        r2 = *(f32 *)list_get(&s_dock.root->ratios, 2);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, r0);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, r1);
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, r2);
}

void test_split_other_axis_nests(void)
{
        DockNode *g1 = s_dock.root;
        DockNode *g2;
        DockNode *g3;
        DockNode *inner;

        s_open("a");
        g2 = dock_split(&s_dock, g1, DOCK_ZONE_RIGHT,
                        dummy_view_create(&s_views, "b"));
        g3 = dock_split(&s_dock, g2, DOCK_ZONE_TOP,
                        dummy_view_create(&s_views, "c"));

        inner = s_child(s_dock.root, 1);
        TEST_ASSERT_EQUAL_INT(DOCK_SPLIT, inner->kind);
        TEST_ASSERT_EQUAL_INT(UI_AXIS_Y, inner->axis);
        TEST_ASSERT_EQUAL_PTR(g3, s_child(inner, 0));
        TEST_ASSERT_EQUAL_PTR(g2, s_child(inner, 1));
        TEST_ASSERT_EQUAL_PTR(inner, g2->parent);
}

void test_reorder_in_tab_bar(void)
{
        DockNode *g = s_dock.root;
        ViewHandle a = s_open("a");
        ViewHandle b = s_open("b");
        ViewHandle c = s_open("c");

        /* Drag a behind c: insert index 3 counts a itself, so it ends last */
        dock_move(&s_dock, a, g, g, DOCK_ZONE_TABBAR, 3);
        TEST_ASSERT_TRUE(view_handle_equal(b, s_view_at(g, 0)));
        TEST_ASSERT_TRUE(view_handle_equal(c, s_view_at(g, 1)));
        TEST_ASSERT_TRUE(view_handle_equal(a, s_view_at(g, 2)));
        TEST_ASSERT_EQUAL_INT(2, g->active);
}

void test_own_only_tab_cannot_split_its_group(void)
{
        ViewHandle a = s_open("a");

        dock_move(&s_dock, a, s_dock.root, s_dock.root, DOCK_ZONE_LEFT, 0);
        TEST_ASSERT_EQUAL_INT(DOCK_TABS, s_dock.root->kind);
        TEST_ASSERT_EQUAL_INT(1, dock_group_count(&s_dock));
}

void test_split_active_clones_view(void)
{
        s_open("a");
        dock_split_active(&s_dock, DOCK_ZONE_BOTTOM);

        TEST_ASSERT_EQUAL_INT(2, dock_group_count(&s_dock));
        TEST_ASSERT_EQUAL_INT(UI_AXIS_Y, s_dock.root->axis);
        TEST_ASSERT_EQUAL_STRING("a", view_get(&s_views,
                                dock_active_view(&s_dock))->title);
}

/* --- drag and drop through the UI -------------------------------------- */

LOC f32 s_measure(void *user, u8 font, f32 size, const char *text, s32 len)
{
        (void)user;
        (void)font;
        (void)size;
        (void)text;
        return 7.0f * (f32)len;
}

LOC f32 s_line_height(void *user, u8 font, f32 size)
{
        (void)user;
        (void)font;
        (void)size;
        return 16.0f;
}

LOC void s_frame(UI *ui, UIInput *in)
{
        UIBox *area;

        ui_begin_frame(ui, in, 800.0f, 600.0f, 0.0);
        area = ui_box_begin(ui, "area", NULL, 0);
        area->style.width = ui_grow(1.0f);
        area->style.height = ui_grow(1.0f);
        dock_build(&s_dock, ui);
        ui_box_end(ui); /* area */
        ui_end_frame(ui);
        ui_input_next_frame(in);
}

void test_drag_tab_to_right_edge_splits(void)
{
        UI ui;
        UITheme theme;
        UIInput in;
        UIRect *tab;
        DockNode *g = s_dock.root;
        ViewHandle a;

        a = s_open("a");
        s_open("b");

        ui_theme_init(&theme);
        ui_theme_load_string(&theme, "tab.height = 30\n"
                        "tab-bar.height = 30\n");
        ui_init(&ui, &theme, s_measure, s_line_height, NULL);
        memset(&in, 0, sizeof(in));
        in.mouse_inside = 1;

        /* Two frames so the rects are known */
        s_frame(&ui, &in);
        s_frame(&ui, &in);
        tab = list_get(&g->tab_rects, 0);
        TEST_ASSERT_TRUE(tab->w > 0.0f);

        /* Hover, press on tab "a", drag to the right edge, release */
        in.mouse_x = tab->x + tab->w * 0.5f;
        in.mouse_y = tab->y + tab->h * 0.5f;
        s_frame(&ui, &in);
        in.button_down[UI_MOUSE_LEFT] = 1;
        in.button_pressed[UI_MOUSE_LEFT] = 1;
        s_frame(&ui, &in);
        TEST_ASSERT_TRUE(s_dock.drag.pending);

        in.mouse_x = 780.0f;
        in.mouse_y = 300.0f;
        s_frame(&ui, &in);
        s_frame(&ui, &in);
        TEST_ASSERT_TRUE(s_dock.drag.active);
        TEST_ASSERT_EQUAL_INT(DOCK_ZONE_RIGHT, s_dock.drag.zone);

        in.button_down[UI_MOUSE_LEFT] = 0;
        in.button_released[UI_MOUSE_LEFT] = 1;
        s_frame(&ui, &in);

        TEST_ASSERT_EQUAL_INT(2, dock_group_count(&s_dock));
        TEST_ASSERT_EQUAL_INT(DOCK_SPLIT, s_dock.root->kind);
        TEST_ASSERT_TRUE(view_handle_equal(a,
                                s_view_at(s_child(s_dock.root, 1), 0)));
        TEST_ASSERT_FALSE(s_dock.drag.pending);

        ui_deinit(&ui);
        ui_theme_deinit(&theme);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_open_and_close);
        RUN_TEST(test_empty_group_has_no_active_view);
        RUN_TEST(test_split_creates_group_and_cleanup_folds_it);
        RUN_TEST(test_split_same_axis_adds_sibling);
        RUN_TEST(test_split_other_axis_nests);
        RUN_TEST(test_reorder_in_tab_bar);
        RUN_TEST(test_own_only_tab_cannot_split_its_group);
        RUN_TEST(test_split_active_clones_view);
        RUN_TEST(test_drag_tab_to_right_edge_splits);
        return UNITY_END();
}
