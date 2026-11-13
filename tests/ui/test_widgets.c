#include "unity.h"

#include <string.h>

#include <ui/ui.h>
#include <ui/widgets.h>

/**
 * Widgets driven headless: every frame calls the current builder, input is
 * set up between frames. Text is 8 px per byte, lines are 16 px.
 */

LOC UI s_ui;
LOC UITheme s_theme;
LOC UIInput s_input;
LOC f64 s_time;
LOC char s_clipboard[64];

/* State the builders work on */
LOC char s_buf[16];
LOC UITextFieldResult s_tf;
LOC s32 s_check[3];
LOC f32 s_value;
LOC s32 s_selected;
LOC s32 s_expanded;
LOC UIList s_list;
LOC s32 s_list_count;

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

LOC s32 s_clip_get(void *user, char *buf, s32 cap)
{
        (void)user;
        strncpy(buf, s_clipboard, (size_t)cap - 1);
        buf[cap - 1] = '\0';
        return (s32)strlen(buf);
}

LOC void s_clip_set(void *user, const char *text, s32 len)
{
        (void)user;
        memcpy(s_clipboard, text, (size_t)len);
        s_clipboard[len] = '\0';
}

void setUp(void)
{
        ui_theme_init(&s_theme);
        ui_theme_load_string(&s_theme,
                        "text-field.width = 200\n"
                        "text-field.padding = 0 4\n"
                        "slider.width = 114\n"
                        "slider.height = 20\n"
                        "slider-thumb.width = 14\n"
                        "list.width = 200\n"
                        "list.height = 100\n"
                        "tree-row.height = 20\n");
        ui_init(&s_ui, &s_theme, s_measure, s_line_height, NULL);
        s_ui.clipboard_get = s_clip_get;
        s_ui.clipboard_set = s_clip_set;
        memset(&s_input, 0, sizeof(s_input));
        s_input.mouse_inside = 1;
        s_time = 0.0;
        memset(s_buf, 0, sizeof(s_buf));
        memset(s_check, 0, sizeof(s_check));
        s_clipboard[0] = '\0';
}

void tearDown(void)
{
        ui_deinit(&s_ui);
        ui_theme_deinit(&s_theme);
}

LOC void s_frame(void (*build)(void))
{
        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, s_time);
        build();
        ui_end_frame(&s_ui);
        ui_input_next_frame(&s_input);
        s_time += 1.0 / 60.0;
}

/* Hover, press and release at (x, y) */
LOC void s_click(void (*build)(void), f32 x, f32 y)
{
        s_input.mouse_x = x;
        s_input.mouse_y = y;
        s_frame(build);
        s_input.button_down[UI_MOUSE_LEFT] = 1;
        s_input.button_pressed[UI_MOUSE_LEFT] = 1;
        s_input.click_count[UI_MOUSE_LEFT] = 1;
        s_frame(build);
        s_input.button_down[UI_MOUSE_LEFT] = 0;
        s_input.button_released[UI_MOUSE_LEFT] = 1;
        s_frame(build);
}

LOC void s_type(const char *text)
{
        UIEvent ev;

        memset(&ev, 0, sizeof(ev));
        ev.kind = UI_EVENT_TEXT;
        strncpy(ev.text, text, sizeof(ev.text) - 1);
        ui_input_push(&s_input, &ev);
}

LOC void s_key(u32 key, u16 mods)
{
        UIEvent ev;

        memset(&ev, 0, sizeof(ev));
        ev.kind = UI_EVENT_KEY_DOWN;
        ev.key = key;
        ev.mods = mods;
        ui_input_push(&s_input, &ev);
}

/* --- builders ------------------------------------------------------------ */

LOC void s_build_field(void)
{
        s_tf = ui_text_field(&s_ui, "field", s_buf, sizeof(s_buf), "Name",
                        NULL);
}

LOC void s_build_checks(void)
{
        UIBox *col = ui_box_begin(&s_ui, "col", NULL, 0);

        col->style.layout = UI_LAYOUT_COLUMN;
        ui_checkbox(&s_ui, "a", &s_check[0], NULL);
        ui_checkbox(&s_ui, "b", &s_check[1], NULL);
        ui_checkbox(&s_ui, "c", &s_check[2], NULL);
        ui_box_end(&s_ui); /* col */
}

LOC void s_build_slider(void)
{
        ui_slider(&s_ui, "slider", &s_value, 0.0f, 10.0f, NULL);
}

LOC void s_build_dropdown(void)
{
        static const char *const items[] = {"one", "two", "three"};

        ui_dropdown(&s_ui, "drop", &s_selected, items, 3, NULL);
}

LOC void s_build_tree(void)
{
        ui_tree_row(&s_ui, "folder", "folder", 0, 0, 1, &s_expanded, 0, NULL);
}

LOC void s_build_list(void)
{
        s32 i;

        s_list = ui_list_begin(&s_ui, "list", s_list_count, 20.0f, NULL);
        for(i = s_list.first; i < s_list.last; i++) {
                ui_push_id(&s_ui, (u64)i);
                ui_tree_row(&s_ui, "row", "row", 0, 0, 0, NULL, 0, NULL);
                ui_pop_id(&s_ui);
        }
        ui_list_end(&s_ui, &s_list);
}

LOC void s_build_toasts(void)
{
        ui_toasts(&s_ui, 0.0f);
}

/* --- text field ---------------------------------------------------------- */

LOC void s_focus_field(void)
{
        s_frame(s_build_field);
        s_click(s_build_field, 50.0f, 8.0f);
        TEST_ASSERT_TRUE(s_tf.focused);
}

void test_text_field_types_and_edits(void)
{
        s_focus_field();

        s_type("hello");
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("hello", s_buf);
        TEST_ASSERT_TRUE(s_tf.changed);
        TEST_ASSERT_EQUAL_INT(1, s_ui.text_input);

        /* Caret one left, delete the 'l' before it */
        s_key(UI_KEY_LEFT, 0);
        s_key(UI_KEY_BACKSPACE, 0);
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("helo", s_buf);

        /* Select to the start and type over it */
        s_key(UI_KEY_HOME, UI_MOD_SHIFT);
        s_type("X");
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("Xo", s_buf);

        /* Select all, delete */
        s_key('a', UI_MOD_CTRL);
        s_key(UI_KEY_DELETE, 0);
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("", s_buf);
}

void test_text_field_respects_capacity(void)
{
        s_focus_field();

        s_type("0123456789");
        s_type("abcdefghij");
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_INT(15, (s32)strlen(s_buf));
        TEST_ASSERT_EQUAL_STRING("0123456789abcde", s_buf);
}

void test_text_field_utf8_and_words(void)
{
        s_focus_field();

        /* "a ü" then Backspace removes both bytes of the ü */
        s_type("a \xc3\xbc");
        s_key(UI_KEY_BACKSPACE, 0);
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("a ", s_buf);

        /* Ctrl+Backspace removes a whole word */
        s_type("word");
        s_key(UI_KEY_BACKSPACE, UI_MOD_CTRL);
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("a ", s_buf);
}

void test_text_field_clipboard(void)
{
        s_focus_field();

        s_type("copy me");
        s_key('a', UI_MOD_CTRL);
        s_key('x', UI_MOD_CTRL);
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("copy me", s_clipboard);
        TEST_ASSERT_EQUAL_STRING("", s_buf);

        s_key('v', UI_MOD_CTRL);
        s_key('v', UI_MOD_CTRL);
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("copy mecopy me", s_buf);
}

void test_text_field_click_places_caret(void)
{
        strcpy(s_buf, "abcdef");
        s_frame(s_build_field);

        /* Padding 4, 8 px per char: x = 4 + 2.6 * 8 lands after "abc" */
        s_click(s_build_field, 4.0f + 2.6f * 8.0f, 8.0f);
        s_type("_");
        s_frame(s_build_field);
        TEST_ASSERT_EQUAL_STRING("abc_def", s_buf);
}

void test_text_field_enter_and_escape(void)
{
        s_focus_field();

        s_key(UI_KEY_ENTER, 0);
        s_frame(s_build_field);
        TEST_ASSERT_TRUE(s_tf.submitted);

        s_key(UI_KEY_ESCAPE, 0);
        s_frame(s_build_field);
        TEST_ASSERT_TRUE(s_tf.cancelled);
        TEST_ASSERT_EQUAL_UINT64(0, s_ui.focus);
}

/* Plain keys are taken, chords it doesn't know are left for shortcuts */
void test_text_field_events_consumed(void)
{
        UIEvent *ev;

        s_focus_field();
        s_key('q', 0);
        s_key('n', UI_MOD_CTRL);

        ui_begin_frame(&s_ui, &s_input, 400.0f, 300.0f, s_time);
        s_build_field();
        ev = &s_input.events[0];
        TEST_ASSERT_TRUE(ev->consumed);
        ev = &s_input.events[1];
        TEST_ASSERT_FALSE(ev->consumed);
        ui_end_frame(&s_ui);
        ui_input_next_frame(&s_input);
}

void test_press_elsewhere_drops_focus(void)
{
        s_focus_field();
        s_click(s_build_field, 300.0f, 200.0f);
        TEST_ASSERT_EQUAL_UINT64(0, s_ui.focus);
        TEST_ASSERT_EQUAL_INT(0, s_ui.text_input);
}

/* --- toggles and focus --------------------------------------------------- */

void test_checkbox_click_and_space(void)
{
        s_frame(s_build_checks);
        s_click(s_build_checks, 4.0f, 8.0f);
        TEST_ASSERT_EQUAL_INT(1, s_check[0]);

        /* Focused by the click: Space toggles back */
        s_key(' ', 0);
        s_frame(s_build_checks);
        TEST_ASSERT_EQUAL_INT(0, s_check[0]);
}

void test_tab_moves_focus(void)
{
        s_frame(s_build_checks);

        s_key(UI_KEY_TAB, 0);
        s_frame(s_build_checks);
        s_key(UI_KEY_TAB, 0);
        s_frame(s_build_checks);
        s_key(' ', 0);
        s_frame(s_build_checks);
        TEST_ASSERT_EQUAL_INT(0, s_check[0]);
        TEST_ASSERT_EQUAL_INT(1, s_check[1]);

        s_key(UI_KEY_TAB, UI_MOD_SHIFT);
        s_frame(s_build_checks);
        s_key(' ', 0);
        s_frame(s_build_checks);
        TEST_ASSERT_EQUAL_INT(1, s_check[0]);
}

/* --- slider, dropdown, tree, list --------------------------------------- */

void test_slider_drag_and_keys(void)
{
        s_value = 0.0f;
        s_frame(s_build_slider);

        /* Travel is 114 - 14 = 100 px; the thumb center follows the mouse */
        s_click(s_build_slider, 7.0f + 50.0f, 10.0f);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, s_value);

        s_key(UI_KEY_RIGHT, 0);
        s_frame(s_build_slider);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.1f, s_value);

        s_key(UI_KEY_END, 0);
        s_frame(s_build_slider);
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, s_value);
}

void test_dropdown_opens_and_picks_with_keys(void)
{
        UIBoxKey key;

        s_selected = 0;
        s_frame(s_build_dropdown);
        s_click(s_build_dropdown, 5.0f, 5.0f);
        key = s_ui.focus;
        TEST_ASSERT_TRUE(ui_popup_is_open(&s_ui, key));

        s_key(UI_KEY_DOWN, 0);
        s_key(UI_KEY_DOWN, 0);
        s_frame(s_build_dropdown);
        TEST_ASSERT_EQUAL_INT(2, s_selected);

        s_key(UI_KEY_ENTER, 0);
        s_frame(s_build_dropdown);
        TEST_ASSERT_FALSE(ui_popup_is_open(&s_ui, key));
}

void test_tree_row_toggles(void)
{
        s_expanded = 0;
        s_frame(s_build_tree);
        s_click(s_build_tree, 10.0f, 10.0f);
        TEST_ASSERT_EQUAL_INT(1, s_expanded);
        s_click(s_build_tree, 10.0f, 10.0f);
        TEST_ASSERT_EQUAL_INT(0, s_expanded);
}

void test_list_builds_only_visible_rows(void)
{
        s_list_count = 1000;
        s_frame(s_build_list);
        s_frame(s_build_list);
        TEST_ASSERT_EQUAL_INT(0, s_list.first);
        TEST_ASSERT_EQUAL_INT(5, s_list.last);
        TEST_ASSERT_EQUAL_FLOAT(20000.0f, s_list.box->state->content[1]);

        ui_list_scroll_to(&s_ui, &s_list, 500);
        s_frame(s_build_list);
        s_frame(s_build_list);
        TEST_ASSERT_EQUAL_INT(496, s_list.first);
        TEST_ASSERT_EQUAL_INT(501, s_list.last);

        /* Shrinking the list pulls the range back in */
        s_list_count = 3;
        s_frame(s_build_list);
        TEST_ASSERT_EQUAL_INT(0, s_list.first);
        TEST_ASSERT_EQUAL_INT(3, s_list.last);
}

/* --- toasts -------------------------------------------------------------- */

void test_toasts_expire_and_drop_oldest(void)
{
        s32 i;

        for(i = 0; i < UI_MAX_TOASTS + 1; i++)
                ui_toast(&s_ui, UI_TOAST_INFO, i == 0 ? "first" : "more");
        TEST_ASSERT_EQUAL_INT(UI_MAX_TOASTS, s_ui.toast_count);
        TEST_ASSERT_EQUAL_STRING("more", s_ui.toasts[0].text);

        s_frame(s_build_toasts);
        TEST_ASSERT_TRUE(s_ui.wakeup > 0.0);

        s_time += UI_TOAST_TIME + 0.1;
        s_frame(s_build_toasts);
        TEST_ASSERT_EQUAL_INT(0, s_ui.toast_count);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_text_field_types_and_edits);
        RUN_TEST(test_text_field_respects_capacity);
        RUN_TEST(test_text_field_utf8_and_words);
        RUN_TEST(test_text_field_clipboard);
        RUN_TEST(test_text_field_click_places_caret);
        RUN_TEST(test_text_field_enter_and_escape);
        RUN_TEST(test_text_field_events_consumed);
        RUN_TEST(test_press_elsewhere_drops_focus);
        RUN_TEST(test_checkbox_click_and_space);
        RUN_TEST(test_tab_moves_focus);
        RUN_TEST(test_slider_drag_and_keys);
        RUN_TEST(test_dropdown_opens_and_picks_with_keys);
        RUN_TEST(test_tree_row_toggles);
        RUN_TEST(test_list_builds_only_visible_rows);
        RUN_TEST(test_toasts_expire_and_drop_oldest);
        return UNITY_END();
}
