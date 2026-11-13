#include <ui/widgets/viewport.h>

#include <string.h>

/* What the box remembers between frames, in its state */
enum {
        DRAG_NONE = 0,
        DRAG_ORBIT,
        DRAG_PAN
};

typedef struct ViewportDraw {
        UIDrawCallbackFn fn;
        void *data;
} ViewportDraw;

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_draw(UI *ui, UIBox *box, void *data);
LOC UIRect s_content(const UIBox *box, UIRect r);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UISignal ui_viewport(UI *ui, const char *id, Camera3D *cam,
                UIDrawCallbackFn draw, void *data, const char *cls)
{
        UIInput *in = ui->input;
        UIBox *box;
        UIBoxState *st;
        UISignal sig;
        ViewportDraw *vd;
        f32 wheel;
        f32 dx;
        f32 dy;

        memset(&sig, 0, sizeof(sig));
        box = ui_box(ui, id, cls ? cls : "viewport", UI_BOX_CLICKABLE);
        if(!box) return sig;
        sig = ui_signal(ui, box);
        st = box->state;

        /**
         * Drags: user_i is the mode, user_f/user_t the mouse position of
         * the last frame, so every frame moves by the difference.
         */
        if(st && in && cam) {
                if(sig.pressed) {
                        st->user_i = (in->mods & UI_MOD_SHIFT) ? DRAG_PAN :
                                DRAG_ORBIT;
                        st->user_f = in->mouse_x;
                        st->user_t = in->mouse_y;
                } else if(sig.hovering && in->button_pressed[UI_MOUSE_MIDDLE]) {
                        st->user_i = DRAG_PAN;
                        st->user_f = in->mouse_x;
                        st->user_t = in->mouse_y;
                }
                if(!in->button_down[UI_MOUSE_LEFT] &&
                   !in->button_down[UI_MOUSE_MIDDLE])
                        st->user_i = DRAG_NONE;

                if(st->user_i != DRAG_NONE) {
                        dx = in->mouse_x - st->user_f;
                        dy = in->mouse_y - (f32)st->user_t;
                        if(st->user_i == DRAG_ORBIT) {
                                camera_orbit(cam, dx, dy);
                        } else {
                                camera_pan(cam, dx, dy, s_content(box,
                                                        st->rect).h);
                        }
                        st->user_f = in->mouse_x;
                        st->user_t = in->mouse_y;
                        if(dx != 0.0f || dy != 0.0f) ui->changed = 1;
                }
                box->cursor = st->user_i != DRAG_NONE ? UI_CURSOR_MOVE :
                        UI_CURSOR_DEFAULT;

                if(ui_take_wheel(ui, box->key, NULL, &wheel)) {
                        camera_zoom(cam, wheel);
                        ui->changed = 1;
                }
        }

        if(draw && (vd = arena_alloc(&ui->arena, sizeof(ViewportDraw)))) {
                vd->fn = draw;
                vd->data = data;
                box->custom_draw = s_draw;
                box->custom_data = vd;
        }
        return sig;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

/* One callback command over the content area */
LOC void s_draw(UI *ui, UIBox *box, void *data)
{
        ViewportDraw *vd = data;
        UIDrawCmd cmd;
        UIRect area = s_content(box, box->rect);

        memset(&cmd, 0, sizeof(cmd));
        cmd.kind = UI_DRAW_CALLBACK;
        cmd.rect = area;
        cmd.clip = ui_rect_intersect(box->clip, area);
        cmd.callback = vd->fn;
        cmd.callback_data = vd->data;
        if(cmd.clip.w > 0.0f && cmd.clip.h > 0.0f) ui_draw_push(ui, &cmd);
}

LOC UIRect s_content(const UIBox *box, UIRect r)
{
        const UISides *p = &box->style.padding;

        return ui_rect(r.x + p->left, r.y + p->top,
                        r.w - p->left - p->right,
                        r.h - p->top - p->bottom);
}
