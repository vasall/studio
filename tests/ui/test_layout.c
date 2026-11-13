#include "unity.h"

#include <string.h>

#include <ui/ui.h>
#include <ui/widgets.h>

/**
 * Layout tests with a fake font: every byte is 8 px wide, lines are 16 px.
 */

LOC UI s_ui;
LOC UITheme s_theme;
LOC UIInput s_input;

LOC f32 s_measure(void *user, u8 font, f32 size, const char *text, s32 len)
{
        (void)user;
        (void)font;
        (void)size;
        (void)text;
        return 8.0f * (f32)len;
}

LOC f32 s_line_height(void *user, u8 font, f32 size)
{
        (void)user;
        (void)font;
        (void)size;
        return 16.0f;
}

void setUp(void)
{
        ui_theme_init(&s_theme);
        ui_init(&s_ui, &s_theme, s_measure, s_line_height, NULL);
        memset(&s_input, 0, sizeof(s_input));
        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
}

void tearDown(void)
{
        ui_deinit(&s_ui);
        ui_theme_deinit(&s_theme);
}

/* A container box filling the window, as base for the tests */
LOC UIBox *s_container(u8 layout)
{
        UIBox *box = ui_box_begin(&s_ui, "container", NULL, 0);

        box->style.width = ui_grow(1.0f);
        box->style.height = ui_grow(1.0f);
        box->style.layout = layout;
        return box;
}

LOC UIBox *s_child(const char *id, UISize w, UISize h)
{
        UIBox *box = ui_box(&s_ui, id, NULL, 0);

        box->style.width = w;
        box->style.height = h;
        return box;
}

LOC void s_finish(void)
{
        while(s_ui.depth > 1) ui_box_end(&s_ui); /* all boxes up to the root */
        ui_end_frame(&s_ui);
}

#define ASSERT_RECT(box, ex, ey, ew, eh) do { \
        TEST_ASSERT_EQUAL_FLOAT((ex), (box)->rect.x); \
        TEST_ASSERT_EQUAL_FLOAT((ey), (box)->rect.y); \
        TEST_ASSERT_EQUAL_FLOAT((ew), (box)->rect.w); \
        TEST_ASSERT_EQUAL_FLOAT((eh), (box)->rect.h); \
} while(0)

void test_fixed_sizes_in_row(void)
{
        UIBox *a;
        UIBox *b;

        s_container(UI_LAYOUT_ROW);
        a = s_child("a", ui_px(100), ui_px(50));
        b = s_child("b", ui_px(60), ui_px(20));
        s_finish();

        ASSERT_RECT(a, 0, 0, 100, 50);
        ASSERT_RECT(b, 100, 0, 60, 20);
}

void test_column_with_gap_and_padding(void)
{
        UIBox *c;
        UIBox *a;
        UIBox *b;

        c = s_container(UI_LAYOUT_COLUMN);
        c->style.gap = 10.0f;
        ui_sides4(&c->style.padding, 5, 7, 5, 7);
        a = s_child("a", ui_px(100), ui_px(50));
        b = s_child("b", ui_px(60), ui_px(20));
        s_finish();

        ASSERT_RECT(a, 5, 7, 100, 50);
        ASSERT_RECT(b, 5, 67, 60, 20);
}

void test_grow_shares_free_space_by_weight(void)
{
        UIBox *a;
        UIBox *b;
        UIBox *c;

        s_container(UI_LAYOUT_ROW);
        a = s_child("a", ui_px(100), ui_px(10));
        b = s_child("b", ui_grow(1), ui_px(10));
        c = s_child("c", ui_grow(3), ui_px(10));
        s_finish();

        ASSERT_RECT(a, 0, 0, 100, 10);
        ASSERT_RECT(b, 100, 0, 75, 10);
        ASSERT_RECT(c, 175, 0, 225, 10);
}

void test_grow_on_cross_axis_fills(void)
{
        UIBox *a;

        s_container(UI_LAYOUT_ROW);
        a = s_child("a", ui_px(10), ui_grow(1));
        s_finish();

        ASSERT_RECT(a, 0, 0, 10, 300);
}

void test_grow_respects_max(void)
{
        UIBox *a;
        UIBox *b;

        s_container(UI_LAYOUT_ROW);
        a = s_child("a", ui_grow(1), ui_px(10));
        a->style.width.max = 50.0f;
        b = s_child("b", ui_grow(1), ui_px(10));
        s_finish();

        TEST_ASSERT_EQUAL_FLOAT(50, a->rect.w);
        TEST_ASSERT_EQUAL_FLOAT(350, b->rect.w);
}

void test_percent_of_content_box(void)
{
        UIBox *c;
        UIBox *a;

        c = s_container(UI_LAYOUT_ROW);
        ui_sides(&c->style.padding, 50);
        a = s_child("a", ui_pct(0.5f), ui_pct(0.25f));
        s_finish();

        ASSERT_RECT(a, 50, 50, 150, 50);
}

void test_fit_wraps_children_and_padding(void)
{
        UIBox *f;
        UIBox *a;

        s_container(UI_LAYOUT_ROW);
        f = ui_box_begin(&s_ui, "fit", NULL, 0);
        f->style.layout = UI_LAYOUT_COLUMN;
        f->style.gap = 4.0f;
        ui_sides(&f->style.padding, 2);
        s_child("a", ui_px(30), ui_px(10));
        a = s_child("b", ui_px(50), ui_px(10));
        ui_box_end(&s_ui); /* fit */
        s_finish();

        ASSERT_RECT(f, 0, 0, 54, 28);
        ASSERT_RECT(a, 2, 16, 50, 10);
}

void test_fit_text(void)
{
        UIBox *l;

        s_container(UI_LAYOUT_ROW);
        l = ui_label(&s_ui, "Hello##id", NULL);
        ui_sides4(&l->style.padding, 4, 2, 4, 2);
        s_finish();

        TEST_ASSERT_EQUAL_INT(5, l->text_len);
        ASSERT_RECT(l, 0, 0, 48, 20);
}

void test_margins_add_space(void)
{
        UIBox *a;
        UIBox *b;

        s_container(UI_LAYOUT_ROW);
        a = s_child("a", ui_px(10), ui_px(10));
        ui_sides4(&a->style.margin, 3, 4, 5, 6);
        b = s_child("b", ui_px(10), ui_px(10));
        s_finish();

        ASSERT_RECT(a, 3, 4, 10, 10);
        ASSERT_RECT(b, 18, 0, 10, 10);
}

void test_align_center_and_end(void)
{
        UIBox *c;
        UIBox *a;
        UIBox *b;

        c = s_container(UI_LAYOUT_ROW);
        c->style.align_x = UI_ALIGN_CENTER;
        c->style.align_y = UI_ALIGN_END;
        a = s_child("a", ui_px(100), ui_px(10));
        b = s_child("b", ui_px(100), ui_px(20));
        s_finish();

        /* The pair is 200 wide, centered in 400; each sits at the bottom */
        ASSERT_RECT(a, 100, 290, 100, 10);
        ASSERT_RECT(b, 200, 280, 100, 20);
}

void test_stack_aligns_each_child(void)
{
        UIBox *c;
        UIBox *a;
        UIBox *b;

        c = s_container(UI_LAYOUT_STACK);
        c->style.align_x = UI_ALIGN_CENTER;
        c->style.align_y = UI_ALIGN_CENTER;
        a = s_child("a", ui_px(100), ui_px(100));
        b = s_child("b", ui_px(20), ui_px(40));
        s_finish();

        ASSERT_RECT(a, 150, 100, 100, 100);
        ASSERT_RECT(b, 190, 130, 20, 40);
}

void test_shrink_on_overflow_keeps_pixels(void)
{
        UIBox *a;
        UIBox *b;
        UIBox *c;

        s_container(UI_LAYOUT_ROW);
        a = s_child("a", ui_px(300), ui_px(10));
        b = ui_label(&s_ui, "0123456789012345678901234", NULL);  /* 200 */
        c = ui_label(&s_ui, "0123456789", NULL);                 /* 80 */
        c->style.width.min = 70.0f;
        s_finish();

        /* 180 too wide: b shrinks by 180 * 200/210, c by 180 * 10/210 */
        TEST_ASSERT_EQUAL_FLOAT(300, a->rect.w);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f - 180.0f * 200.0f / 210.0f,
                        b->rect.w);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 80.0f - 180.0f * 10.0f / 210.0f,
                        c->rect.w);
}

void test_row_wrap_breaks_lines(void)
{
        UIBox *c;
        UIBox *k[5];
        char id[2];
        s32 i;

        c = s_container(UI_LAYOUT_ROW);
        c->style.wrap = 1;
        c->style.gap = 10.0f;
        id[1] = '\0';
        for(i = 0; i < 5; i++) {
                id[0] = (char)('a' + i);
                k[i] = s_child(id, ui_px(120), ui_px(i == 1 ? 30 : 20));
        }
        s_finish();

        /* 3 fit per line (120*3 + 20 = 380 <= 400) */
        ASSERT_RECT(k[0], 0, 0, 120, 20);
        ASSERT_RECT(k[2], 260, 0, 120, 20);
        ASSERT_RECT(k[3], 0, 40, 120, 20);
        ASSERT_RECT(k[4], 130, 40, 120, 20);
        TEST_ASSERT_EQUAL_INT(1, k[4]->line);
}

void test_row_wrap_fit_height_and_grow_per_line(void)
{
        UIBox *w;
        UIBox *a;
        UIBox *g;

        s_container(UI_LAYOUT_COLUMN);
        w = ui_box_begin(&s_ui, "wrap", NULL, 0);
        w->style.width = ui_px(200);
        w->style.wrap = 1;
        s_child("a", ui_px(150), ui_px(20));
        a = s_child("b", ui_px(100), ui_px(30));
        g = s_child("g", ui_grow(1), ui_px(10));
        ui_box_end(&s_ui); /* wrap */
        s_finish();

        /* Lines: [a] [b g]; g grows to the rest of its line */
        TEST_ASSERT_EQUAL_FLOAT(50, w->rect.h);
        ASSERT_RECT(a, 0, 20, 100, 30);
        ASSERT_RECT(g, 100, 20, 100, 10);
}

void test_scroll_offset_and_clamp(void)
{
        UIBox *s;
        UIBox *a;

        s_container(UI_LAYOUT_COLUMN);
        s = ui_scroll_begin(&s_ui, "scroll", NULL);
        s->style.width = ui_px(100);
        s->style.height = ui_px(100);
        s->style.layout = UI_LAYOUT_COLUMN;
        s->state->scroll[1] = 1000.0f;
        a = s_child("a", ui_px(100), ui_px(300));
        ui_scroll_end(&s_ui);
        s_finish();

        /* Scrolled to the end: 300 - 100 */
        TEST_ASSERT_EQUAL_FLOAT(200, s->state->scroll[1]);
        TEST_ASSERT_EQUAL_FLOAT(-200, a->rect.y);
        TEST_ASSERT_EQUAL_FLOAT(0, a->clip.y);
        TEST_ASSERT_EQUAL_FLOAT(100, a->clip.h);
}

void test_floating_child_is_out_of_flow(void)
{
        UIBox *a;
        UIBox *f;
        UIBox *b;

        s_container(UI_LAYOUT_ROW);
        a = s_child("a", ui_px(10), ui_px(10));
        f = ui_box(&s_ui, "float", NULL, UI_BOX_FLOATING);
        f->style.width = ui_px(30);
        f->style.height = ui_px(30);
        f->float_x = 200.0f;
        f->float_y = 150.0f;
        b = s_child("b", ui_px(10), ui_px(10));
        s_finish();

        ASSERT_RECT(a, 0, 0, 10, 10);
        ASSERT_RECT(f, 200, 150, 30, 30);
        ASSERT_RECT(b, 10, 0, 10, 10);
}

void test_fit_parent_counts_grow_children(void)
{
        UIBox *menu;
        UIBox *a;
        UIBox *b;

        s_container(UI_LAYOUT_STACK);
        menu = ui_box_begin(&s_ui, "menu", NULL, UI_BOX_FLOATING);
        menu->style.layout = UI_LAYOUT_COLUMN;
        ui_sides(&menu->style.padding, 4);
        a = ui_label(&s_ui, "Short", NULL);
        a->style.width = ui_grow(1);
        b = ui_label(&s_ui, "A longer item", NULL);
        b->style.width = ui_grow(1);
        ui_box_end(&s_ui); /* menu */
        s_finish();

        /* As wide as the widest item (13 * 8), items fill it */
        TEST_ASSERT_EQUAL_FLOAT(112, menu->rect.w);
        TEST_ASSERT_EQUAL_FLOAT(104, a->rect.w);
        TEST_ASSERT_EQUAL_FLOAT(104, b->rect.w);
}

void test_scroll_children_do_not_shrink(void)
{
        UIBox *s;
        UIBox *a;

        s_container(UI_LAYOUT_COLUMN);
        s = ui_scroll_begin(&s_ui, "tabs", NULL);
        s->style.width = ui_px(100);
        a = ui_label(&s_ui, "0123456789012345678901234", NULL);
        ui_scroll_end(&s_ui);
        s_finish();

        TEST_ASSERT_EQUAL_FLOAT(200, a->rect.w);
        TEST_ASSERT_EQUAL_FLOAT(200, s->state->content[0]);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_fixed_sizes_in_row);
        RUN_TEST(test_column_with_gap_and_padding);
        RUN_TEST(test_grow_shares_free_space_by_weight);
        RUN_TEST(test_grow_on_cross_axis_fills);
        RUN_TEST(test_grow_respects_max);
        RUN_TEST(test_percent_of_content_box);
        RUN_TEST(test_fit_wraps_children_and_padding);
        RUN_TEST(test_fit_text);
        RUN_TEST(test_margins_add_space);
        RUN_TEST(test_align_center_and_end);
        RUN_TEST(test_stack_aligns_each_child);
        RUN_TEST(test_shrink_on_overflow_keeps_pixels);
        RUN_TEST(test_row_wrap_breaks_lines);
        RUN_TEST(test_row_wrap_fit_height_and_grow_per_line);
        RUN_TEST(test_scroll_offset_and_clamp);
        RUN_TEST(test_floating_child_is_out_of_flow);
        RUN_TEST(test_fit_parent_counts_grow_children);
        RUN_TEST(test_scroll_children_do_not_shrink);
        return UNITY_END();
}
