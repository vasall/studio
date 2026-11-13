#include "unity.h"

#include <string.h>

#include <ui/ui.h>
#include <ui/widgets/scroll.h>
#include <ui/widgets/viewport.h>

/*
 * A 200 x 100 viewport with padding 1 at the top of a scroll area that is
 * taller than the window, so the wheel could scroll it.
 */

LOC UI s_ui;
LOC UITheme s_theme;
LOC UIInput s_input;
LOC Camera3D s_cam;
LOC UIBox *s_area;
LOC s32 s_data;

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

LOC void s_draw(const UIDrawCallbackInfo *info, void *data)
{
        (void)info;
        (void)data;
}

void setUp(void)
{
        ui_theme_init(&s_theme);
        ui_theme_load_string(&s_theme,
                        "viewport.width = 200\n"
                        "viewport.height = 100\n"
                        "viewport.padding = 1\n"
                        "area.width = 400\n"
                        "area.height = 300\n"
                        "area.layout = column\n");
        ui_init(&s_ui, &s_theme, s_measure, s_line_height, NULL);
        memset(&s_input, 0, sizeof(s_input));
        s_input.mouse_inside = 1;
        s_input.mouse_x = 100.0f;
        s_input.mouse_y = 50.0f;
        camera_init(&s_cam);
}

void tearDown(void)
{
        ui_deinit(&s_ui);
        ui_theme_deinit(&s_theme);
}

LOC void s_frame(void)
{
        UIBox *filler;

        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        s_area = ui_scroll_begin(&s_ui, "area", "area");
        ui_viewport(&s_ui, "vp", &s_cam, s_draw, &s_data, NULL);
        filler = ui_box(&s_ui, NULL, NULL, 0);
        filler->style.height = ui_px(1000.0f);
        ui_scroll_end(&s_ui);
        ui_end_frame(&s_ui);
        ui_input_next_frame(&s_input);
}

void test_left_drag_orbits(void)
{
        f32 yaw = s_cam.yaw;
        f32 pitch = s_cam.pitch;

        s_frame();
        s_frame();
        s_input.button_down[UI_MOUSE_LEFT] = 1;
        s_input.button_pressed[UI_MOUSE_LEFT] = 1;
        s_frame();
        s_input.mouse_x += 30.0f;
        s_input.mouse_y += 10.0f;
        s_frame();
        TEST_ASSERT_TRUE(s_cam.yaw < yaw);
        TEST_ASSERT_TRUE(s_cam.pitch > pitch);

        /* After release, moving does nothing */
        s_input.button_down[UI_MOUSE_LEFT] = 0;
        s_input.button_released[UI_MOUSE_LEFT] = 1;
        s_frame();
        yaw = s_cam.yaw;
        s_input.mouse_x += 30.0f;
        s_frame();
        TEST_ASSERT_EQUAL_FLOAT(yaw, s_cam.yaw);
}

void test_shift_drag_pans(void)
{
        FVector3 t = s_cam.target;
        f32 yaw = s_cam.yaw;

        s_frame();
        s_frame();
        s_input.mods = UI_MOD_SHIFT;
        s_input.button_down[UI_MOUSE_LEFT] = 1;
        s_input.button_pressed[UI_MOUSE_LEFT] = 1;
        s_frame();
        s_input.mouse_x += 20.0f;
        s_frame();
        TEST_ASSERT_EQUAL_FLOAT(yaw, s_cam.yaw);
        TEST_ASSERT_TRUE(s_cam.target.x != t.x || s_cam.target.y != t.y);
}

void test_wheel_zooms_instead_of_scrolling(void)
{
        f32 d = s_cam.distance;

        s_frame();
        s_frame();
        s_input.wheel_y = 2.0f;
        s_frame();
        TEST_ASSERT_TRUE(s_cam.distance < d);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, s_area->state->scroll[1]);
}

void test_draws_one_callback_over_the_content(void)
{
        UIDrawCmd *c;
        s32 found = 0;
        s32 i;

        s_frame();
        for(i = 0; i < list_count(&s_ui.cmds); i++) {
                c = list_get(&s_ui.cmds, i);
                if(c->kind != UI_DRAW_CALLBACK) continue;
                found++;
                TEST_ASSERT_TRUE(c->callback == s_draw);
                TEST_ASSERT_TRUE(c->callback_data == &s_data);
                TEST_ASSERT_EQUAL_FLOAT(1.0f, c->rect.x);
                TEST_ASSERT_EQUAL_FLOAT(198.0f, c->rect.w);
                TEST_ASSERT_EQUAL_FLOAT(98.0f, c->rect.h);
        }
        TEST_ASSERT_EQUAL_INT(1, found);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_left_drag_orbits);
        RUN_TEST(test_shift_drag_pans);
        RUN_TEST(test_wheel_zooms_instead_of_scrolling);
        RUN_TEST(test_draws_one_callback_over_the_content);
        return UNITY_END();
}
