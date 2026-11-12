#include <ui/input.h>

#include <string.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void ui_input_next_frame(UIInput *in)
{
        if(!in) return;

        memset(in->button_pressed, 0, sizeof(in->button_pressed));
        memset(in->button_released, 0, sizeof(in->button_released));
        memset(in->click_count, 0, sizeof(in->click_count));
        in->wheel_x = 0.0f;
        in->wheel_y = 0.0f;
        in->event_count = 0;
}

PUB s32 ui_input_push(UIInput *in, const UIEvent *ev)
{
        if(!in || !ev || in->event_count >= UI_INPUT_MAX_EVENTS) return -1;
        in->events[in->event_count++] = *ev;
        return 0;
}

PUB s32 ui_event_is_key(const UIEvent *ev, u32 key, u32 scan, u16 mods)
{
        if(!ev || ev->kind != UI_EVENT_KEY_DOWN || ev->consumed) return 0;
        if(ev->mods != mods) return 0;
        if(ev->key == key) return 1;
        return scan != UI_SCAN_NONE && ev->scan == scan;
}
