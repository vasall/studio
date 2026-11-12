#include "unity.h"

#include <string.h>

#include <ui/ui.h>
#include <ui/widgets.h>

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
        s_input.mouse_inside = 1;
}

void tearDown(void)
{
        ui_deinit(&s_ui);
        ui_theme_deinit(&s_theme);
}

/* One frame with a 100x40 button at the top left; returns its signal */
LOC UISignal s_frame_with_button(s32 with_button)
{
        UISignal sig;
        UIBox *b;

        memset(&sig, 0, sizeof(sig));
        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        if(with_button) {
                b = ui_box(&s_ui, "button", NULL, UI_BOX_CLICKABLE);
                b->style.width = ui_px(100);
                b->style.height = ui_px(40);
                sig = ui_signal(&s_ui, b);
        }
        ui_end_frame(&s_ui);
        ui_input_next_frame(&s_input);
        return sig;
}

void test_state_survives_frames(void)
{
        UIBox *b;
        u64 key;

        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        b = ui_box(&s_ui, "x", NULL, 0);
        b->state->user_i = 42;
        key = b->key;
        ui_end_frame(&s_ui);

        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        b = ui_box(&s_ui, "x", NULL, 0);
        TEST_ASSERT_EQUAL_UINT64(key, b->key);
        TEST_ASSERT_EQUAL_INT(42, b->state->user_i);
        ui_end_frame(&s_ui);
}

void test_state_freed_when_not_built(void)
{
        UIBox *b;
        u64 key;

        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        b = ui_box(&s_ui, "x", NULL, 0);
        key = b->key;
        ui_end_frame(&s_ui);
        TEST_ASSERT_NOT_NULL(ui_state_of(&s_ui, key));

        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        ui_end_frame(&s_ui);
        TEST_ASSERT_NULL(ui_state_of(&s_ui, key));
}

void test_same_id_different_parent_differs(void)
{
        UIBox *a;
        UIBox *b;

        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        ui_box_begin(&s_ui, "p1", NULL, 0);
        a = ui_box(&s_ui, "child", NULL, 0);
        ui_box_end(&s_ui); /* p1 */
        ui_box_begin(&s_ui, "p2", NULL, 0);
        b = ui_box(&s_ui, "child", NULL, 0);
        ui_box_end(&s_ui); /* p2 */
        ui_end_frame(&s_ui);

        TEST_ASSERT_TRUE(a->key != b->key);
}

void test_click_takes_hover_press_release(void)
{
        UISignal sig;

        /* Frame 1: the button gets its rect; frame 2: it becomes hot */
        s_input.mouse_x = 50.0f;
        s_input.mouse_y = 20.0f;
        s_frame_with_button(1);
        sig = s_frame_with_button(1);
        TEST_ASSERT_TRUE(sig.hovering);

        /* Press */
        s_input.button_down[UI_MOUSE_LEFT] = 1;
        s_input.button_pressed[UI_MOUSE_LEFT] = 1;
        sig = s_frame_with_button(1);
        TEST_ASSERT_TRUE(sig.pressed);
        TEST_ASSERT_FALSE(sig.clicked);

        /* Release */
        s_input.button_down[UI_MOUSE_LEFT] = 0;
        s_input.button_released[UI_MOUSE_LEFT] = 1;
        sig = s_frame_with_button(1);
        TEST_ASSERT_TRUE(sig.clicked);
        TEST_ASSERT_EQUAL_UINT64(0, s_ui.active);
}

void test_drag_is_not_a_click(void)
{
        UISignal sig;

        s_input.mouse_x = 50.0f;
        s_input.mouse_y = 20.0f;
        s_frame_with_button(1);
        s_frame_with_button(1);

        s_input.button_down[UI_MOUSE_LEFT] = 1;
        s_input.button_pressed[UI_MOUSE_LEFT] = 1;
        s_frame_with_button(1);

        s_input.mouse_x = 70.0f;
        sig = s_frame_with_button(1);
        TEST_ASSERT_TRUE(sig.dragging);
        TEST_ASSERT_EQUAL_FLOAT(20.0f, sig.drag_dx);

        s_input.button_down[UI_MOUSE_LEFT] = 0;
        s_input.button_released[UI_MOUSE_LEFT] = 1;
        sig = s_frame_with_button(1);
        TEST_ASSERT_TRUE(sig.released);
        TEST_ASSERT_FALSE(sig.clicked);
}

void test_theme_class_and_hover_state(void)
{
        UIStyle s;

        ui_theme_load_string(&s_theme,
                "$accent = #102030\n"
                "# comment\n"
                "button.height = 22\n"
                "button.padding = 1 2\n"
                "button.infill = $accent\n"
                "button:hover.infill = #ffffff80\n"
                "button.primary.corner-radius = 1 2 3 4\n");

        s = ui_theme_style(&s_theme, "button.primary", 0);
        TEST_ASSERT_EQUAL_INT(UI_SIZE_PIXELS, s.height.kind);
        TEST_ASSERT_EQUAL_FLOAT(22.0f, s.height.value);
        TEST_ASSERT_EQUAL_FLOAT(2.0f, s.padding[UI_LEFT]);
        TEST_ASSERT_EQUAL_FLOAT(1.0f, s.padding[UI_TOP]);
        TEST_ASSERT_EQUAL_UINT8(0x20, s.infill.g);
        TEST_ASSERT_EQUAL_UINT8(255, s.infill.a);
        TEST_ASSERT_EQUAL_FLOAT(3.0f, s.corner_radius[UI_BOTTOM_RIGHT]);

        s = ui_theme_style(&s_theme, "button.primary", 1u << UI_STATE_HOVER);
        TEST_ASSERT_EQUAL_UINT8(0x80, s.infill.a);
}

void test_popup_closes_on_outside_click(void)
{
        ui_popup_open(&s_ui, 77, 10.0f, 10.0f);
        s_frame_with_button(0);
        TEST_ASSERT_TRUE(ui_popup_is_open(&s_ui, 77));

        s_input.mouse_x = 300.0f;
        s_input.mouse_y = 200.0f;
        s_input.button_pressed[UI_MOUSE_LEFT] = 1;
        s_frame_with_button(0);
        TEST_ASSERT_FALSE(ui_popup_is_open(&s_ui, 77));
}

/* A 100x100 scroll area with 400 px of content; returns the y thumb */
LOC UIBox *s_scroll_frame(UIBox **area_out)
{
        UIBox *area;
        UIBox *c;
        UIBox *thumb;

        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, 0.0);
        area = ui_scroll_begin(&s_ui, "area", NULL);
        area->style.width = ui_px(100);
        area->style.height = ui_px(100);
        area->style.layout = UI_LAYOUT_COLUMN;
        c = ui_box(&s_ui, "content", NULL, 0);
        c->style.width = ui_px(100);
        c->style.height = ui_px(400);
        ui_scroll_end(&s_ui);
        ui_end_frame(&s_ui);
        ui_input_next_frame(&s_input);

        thumb = NULL;
        for(c = area->first; c; c = c->next) {
                if(c->flags & UI_BOX_FLOATING) thumb = c;
        }
        if(area_out) *area_out = area;
        return thumb;
}

void test_scroll_thumb_stays_inside_area(void)
{
        UIBox *area;
        UIBox *thumb;

        s_scroll_frame(&area);
        s_scroll_frame(&area);

        /* An offset far past the end, as a wheel or drag could leave it */
        area->state->scroll[1] = 5000.0f;
        thumb = s_scroll_frame(&area);
        TEST_ASSERT_NOT_NULL(thumb);
        TEST_ASSERT_TRUE(thumb->rect.y >= 0.0f);
        TEST_ASSERT_TRUE(thumb->rect.y + thumb->rect.h <= 100.0f + 0.01f);
        TEST_ASSERT_TRUE(thumb->clip.y + thumb->clip.h <= 100.0f + 0.01f);

        area->state->scroll[1] = -500.0f;
        thumb = s_scroll_frame(&area);
        TEST_ASSERT_EQUAL_FLOAT(0.0f, thumb->rect.y);
}

void test_wheel_clamps_at_the_end(void)
{
        UIBox *area;

        s_input.mouse_x = 50.0f;
        s_input.mouse_y = 50.0f;
        s_scroll_frame(&area);
        s_scroll_frame(&area);

        /* Many wheel steps down: the offset ends at content - view */
        s_input.wheel_y = -100.0f;
        s_scroll_frame(&area);
        TEST_ASSERT_EQUAL_FLOAT(300.0f, area->state->scroll[1]);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_state_survives_frames);
        RUN_TEST(test_state_freed_when_not_built);
        RUN_TEST(test_same_id_different_parent_differs);
        RUN_TEST(test_click_takes_hover_press_release);
        RUN_TEST(test_drag_is_not_a_click);
        RUN_TEST(test_theme_class_and_hover_state);
        RUN_TEST(test_popup_closes_on_outside_click);
        RUN_TEST(test_scroll_thumb_stays_inside_area);
        RUN_TEST(test_wheel_clamps_at_the_end);
        return UNITY_END();
}
