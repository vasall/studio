#include <platform/app.h>

#include <string.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC u32 s_key(SDL_Keycode key);
LOC u16 s_mods(SDL_Keymod mods);
LOC s32 s_button(u8 sdl_button);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 app_input_event(AppWindow *win, const SDL_Event *ev)
{
        UiInput *in = &win->input;
        UiEvent ue;
        s32 b;
        s32 i;

        switch(ev->type) {
        case SDL_EVENT_MOUSE_MOTION:
                in->mouse_x = ev->motion.x * win->mouse_scale;
                in->mouse_y = ev->motion.y * win->mouse_scale;
                in->mouse_inside = 1;
                return 1;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
                b = s_button(ev->button.button);
                if(b < 0) return 0;
                in->mouse_x = ev->button.x * win->mouse_scale;
                in->mouse_y = ev->button.y * win->mouse_scale;
                if(ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        in->button_down[b] = 1;
                        in->button_pressed[b] = 1;
                        in->click_count[b] = ev->button.clicks;
                        SDL_CaptureMouse(true);
                } else {
                        in->button_down[b] = 0;
                        in->button_released[b] = 1;
                        for(i = 0; i < UI_MOUSE_BUTTONS; i++) {
                                if(in->button_down[i]) break;
                        }
                        if(i == UI_MOUSE_BUTTONS) SDL_CaptureMouse(false);
                }
                return 1;

        case SDL_EVENT_MOUSE_WHEEL:
                if(ev->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                        in->wheel_x -= ev->wheel.x;
                        in->wheel_y -= ev->wheel.y;
                } else {
                        in->wheel_x += ev->wheel.x;
                        in->wheel_y += ev->wheel.y;
                }
                return 1;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
                memset(&ue, 0, sizeof(ue));
                ue.kind = (ev->type == SDL_EVENT_KEY_DOWN) ? UI_EVENT_KEY_DOWN :
                        UI_EVENT_KEY_UP;
                ue.repeat = ev->key.repeat ? 1 : 0;
                ue.mods = s_mods(ev->key.mod);
                ue.key = s_key(ev->key.key);
                ue.scan = (ev->key.scancode == SDL_SCANCODE_BACKSLASH) ?
                        UI_SCAN_BACKSLASH : UI_SCAN_NONE;
                in->mods = ue.mods;
                if(ue.key == UI_KEY_NONE && ue.scan == UI_SCAN_NONE) return 1;
                ui_input_push(in, &ue);
                return 1;

        case SDL_EVENT_TEXT_INPUT:
                memset(&ue, 0, sizeof(ue));
                ue.kind = UI_EVENT_TEXT;
                ue.mods = in->mods;
                strncpy(ue.text, ev->text.text, UI_INPUT_TEXT_LEN - 1);
                ui_input_push(in, &ue);
                return 1;

        case SDL_EVENT_WINDOW_MOUSE_ENTER:
                in->mouse_inside = 1;
                return 1;

        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                in->mouse_inside = 0;
                return 1;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
                /* No stuck buttons or modifiers after alt-tab */
                for(i = 0; i < UI_MOUSE_BUTTONS; i++) {
                        if(in->button_down[i]) in->button_released[i] = 1;
                        in->button_down[i] = 0;
                }
                in->mods = 0;
                return 1;

        default:
                return 0;
        }
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC u32 s_key(SDL_Keycode key)
{
        /* Printable ASCII keycodes are the lowercase characters */
        if(key >= 32 && key < 127) return (u32)key;

        switch(key) {
        case SDLK_ESCAPE: return UI_KEY_ESCAPE;
        case SDLK_RETURN: return UI_KEY_ENTER;
        case SDLK_KP_ENTER: return UI_KEY_ENTER;
        case SDLK_TAB: return UI_KEY_TAB;
        case SDLK_BACKSPACE: return UI_KEY_BACKSPACE;
        case SDLK_DELETE: return UI_KEY_DELETE;
        case SDLK_INSERT: return UI_KEY_INSERT;
        case SDLK_LEFT: return UI_KEY_LEFT;
        case SDLK_RIGHT: return UI_KEY_RIGHT;
        case SDLK_UP: return UI_KEY_UP;
        case SDLK_DOWN: return UI_KEY_DOWN;
        case SDLK_HOME: return UI_KEY_HOME;
        case SDLK_END: return UI_KEY_END;
        case SDLK_PAGEUP: return UI_KEY_PAGE_UP;
        case SDLK_PAGEDOWN: return UI_KEY_PAGE_DOWN;
        case SDLK_F1: return UI_KEY_F1;
        case SDLK_F2: return UI_KEY_F2;
        case SDLK_F3: return UI_KEY_F3;
        case SDLK_F4: return UI_KEY_F4;
        case SDLK_F5: return UI_KEY_F5;
        case SDLK_F6: return UI_KEY_F6;
        case SDLK_F7: return UI_KEY_F7;
        case SDLK_F8: return UI_KEY_F8;
        case SDLK_F9: return UI_KEY_F9;
        case SDLK_F10: return UI_KEY_F10;
        case SDLK_F11: return UI_KEY_F11;
        case SDLK_F12: return UI_KEY_F12;
        default: return UI_KEY_NONE;
        }
}

LOC u16 s_mods(SDL_Keymod mods)
{
        u16 m = 0;

        if(mods & SDL_KMOD_SHIFT) m |= UI_MOD_SHIFT;
        if(mods & SDL_KMOD_CTRL) m |= UI_MOD_CTRL;
        if(mods & SDL_KMOD_ALT) m |= UI_MOD_ALT;
        if(mods & SDL_KMOD_GUI) m |= UI_MOD_SUPER;
        return m;
}

LOC s32 s_button(u8 sdl_button)
{
        switch(sdl_button) {
        case SDL_BUTTON_LEFT: return UI_MOUSE_LEFT;
        case SDL_BUTTON_MIDDLE: return UI_MOUSE_MIDDLE;
        case SDL_BUTTON_RIGHT: return UI_MOUSE_RIGHT;
        default: return -1;
        }
}
