#include "unity.h"

#include <string.h>

#include <ui/widgets/canvas.h>
#include <ui/ui.h>

/*
 * A 200 x 100 canvas with padding 2, placed at (50, 20) by an outer box
 * with padding 50 20. Its drawing area starts at (52, 22) and is 196 x 96.
 */

LOC UI s_ui;
LOC UITheme s_theme;
LOC UIInput s_input;
LOC UICanvas *s_cv;
LOC s32 s_calls;

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

LOC void s_callback(const UIDrawCallbackInfo *info, void *data)
{
        (void)info;
        (void)data;
        s_calls++;
}

void setUp(void)
{
        ui_theme_init(&s_theme);
        ui_theme_load_string(&s_theme,
                        "outer.padding = 20 50\n"
                        "canvas.width = 200\n"
                        "canvas.height = 100\n"
                        "canvas.padding = 2\n");
        ui_init(&s_ui, &s_theme, s_measure, s_line_height, NULL);
        memset(&s_input, 0, sizeof(s_input));
        s_input.mouse_inside = 1;
        s_calls = 0;
}

void tearDown(void)
{
        ui_deinit(&s_ui);
        ui_theme_deinit(&s_theme);
}

/* One frame; shapes are added by add() */
LOC void s_frame(void (*add)(void))
{
        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        ui_box_begin(&s_ui, "outer", "outer", 0);
        s_cv = ui_canvas(&s_ui, "cv", NULL);
        if(add) add();
        ui_box_end(&s_ui); /* outer */
        ui_end_frame(&s_ui);
        ui_input_next_frame(&s_input);
}

/* The draw commands after the canvas' own background (none here) */
LOC UIDrawCmd *s_cmd(s32 i)
{
        return list_get(&s_ui.cmds, i);
}

LOC void s_add_shapes(void)
{
        ui_canvas_rect(s_cv, ui_rect(10, 10, 20, 30), ui_hex(0xff0000ff));
        ui_canvas_gl(s_cv, ui_rect(0, 0, 196, 96), s_callback, &s_calls);
        ui_canvas_box(s_cv, ui_rect(-5, 90, 300, 20), ui_color_none(),
                        ui_hex(0x00ff00ff), 1.0f, 4.0f);
}

void test_shapes_move_with_the_canvas_and_keep_order(void)
{
        UIDrawCmd *c;

        s_frame(s_add_shapes);
        TEST_ASSERT_EQUAL_INT(3, list_count(&s_ui.cmds));

        c = s_cmd(0);
        TEST_ASSERT_EQUAL_INT(UI_DRAW_RECT, c->kind);
        TEST_ASSERT_EQUAL_FLOAT(62.0f, c->rect.x);
        TEST_ASSERT_EQUAL_FLOAT(32.0f, c->rect.y);
        TEST_ASSERT_EQUAL_FLOAT(20.0f, c->rect.w);
        TEST_ASSERT_EQUAL_FLOAT(30.0f, c->rect.h);

        c = s_cmd(1);
        TEST_ASSERT_EQUAL_INT(UI_DRAW_CALLBACK, c->kind);
        TEST_ASSERT_TRUE(c->callback == s_callback);
        TEST_ASSERT_TRUE(c->callback_data == &s_calls);
        TEST_ASSERT_EQUAL_FLOAT(52.0f, c->rect.x);
        TEST_ASSERT_EQUAL_FLOAT(22.0f, c->rect.y);

        c = s_cmd(2);
        TEST_ASSERT_EQUAL_FLOAT(4.0f, c->corner_radius[UI_BOTTOM_LEFT]);
        TEST_ASSERT_EQUAL_FLOAT(1.0f, c->border_width);
}

void test_shapes_are_clipped_to_the_drawing_area(void)
{
        UIDrawCmd *c;
        s32 i;

        s_frame(s_add_shapes);
        for(i = 0; i < list_count(&s_ui.cmds); i++) {
                c = s_cmd(i);
                TEST_ASSERT_EQUAL_FLOAT(52.0f, c->clip.x);
                TEST_ASSERT_EQUAL_FLOAT(22.0f, c->clip.y);
                TEST_ASSERT_EQUAL_FLOAT(196.0f, c->clip.w);
                TEST_ASSERT_EQUAL_FLOAT(96.0f, c->clip.h);
        }
}

void test_size_and_mouse_in_canvas_coordinates(void)
{
        s_frame(NULL);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, s_cv->width);
        TEST_ASSERT_EQUAL_INT(1, s_ui.changed);

        s_input.mouse_x = 60.0f;
        s_input.mouse_y = 30.0f;
        s_frame(NULL);
        TEST_ASSERT_EQUAL_FLOAT(196.0f, s_cv->width);
        TEST_ASSERT_EQUAL_FLOAT(96.0f, s_cv->height);
        TEST_ASSERT_EQUAL_FLOAT(8.0f, s_cv->sig.mouse_x);
        TEST_ASSERT_EQUAL_FLOAT(8.0f, s_cv->sig.mouse_y);

        /* Hot from the frame before */
        s_frame(NULL);
        TEST_ASSERT_TRUE(s_cv->sig.hovering);
}

void test_empty_shapes_are_skipped(void)
{
        s_frame(NULL);
        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        s_cv = ui_canvas(&s_ui, "cv", NULL);
        ui_canvas_rect(s_cv, ui_rect(0, 0, 0, 10), ui_hex(0xffffffff));
        ui_canvas_gl(s_cv, ui_rect(0, 0, 10, 10), NULL, NULL);
        TEST_ASSERT_NULL(s_cv->first);
        ui_end_frame(&s_ui);
        TEST_ASSERT_EQUAL_INT(0, s_calls);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_shapes_move_with_the_canvas_and_keep_order);
        RUN_TEST(test_shapes_are_clipped_to_the_drawing_area);
        RUN_TEST(test_size_and_mouse_in_canvas_coordinates);
        RUN_TEST(test_empty_shapes_are_skipped);
        return UNITY_END();
}
