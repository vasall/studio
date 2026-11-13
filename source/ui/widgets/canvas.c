#include <ui/widgets/canvas.h>

#include <string.h>

/* One shape: a draw command with its rect in canvas coordinates */
struct UICanvasShape {
        UIDrawCmd cmd;
        UICanvasShape *next;
};

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_draw(UI *ui, UIBox *box, void *data);
LOC UIDrawCmd *s_add(UICanvas *cv, u8 kind, UIRect r);
LOC UIRect s_content(const UIBox *box, const UIRect *rect);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UICanvas *ui_canvas(UI *ui, const char *id, const char *cls)
{
        UICanvas *cv;
        UIBox *box;
        UIRect area;

        box = ui_box(ui, id, cls ? cls : "canvas", UI_BOX_CLICKABLE);
        if(!box) return NULL;
        if(!(cv = arena_alloc(&ui->arena, sizeof(UICanvas)))) return NULL;

        cv->ui = ui;
        cv->box = box;
        cv->sig = ui_signal(ui, box);

        /* Size and mouse from last frame, relative to the content box */
        if(box->state) {
                area = s_content(box, &box->state->rect);
                cv->width = area.w > 0.0f ? area.w : 0.0f;
                cv->height = area.h > 0.0f ? area.h : 0.0f;
                cv->sig.mouse_x -= box->style.padding.left;
                cv->sig.mouse_y -= box->style.padding.top;

                /* Shapes sized from width/height are wrong in the first
                   frame, so draw another one */
                if(box->state->rect.w <= 0.0f) ui->changed = 1;
        }

        box->custom_draw = s_draw;
        box->custom_data = cv;
        return cv;
}

PUB void ui_canvas_rect(UICanvas *cv, UIRect r, UIColor fill)
{
        UIDrawCmd *cmd;

        if(!cv || !(cmd = s_add(cv, UI_DRAW_RECT, r))) return;
        cmd->color = fill;
}

PUB void ui_canvas_box(UICanvas *cv, UIRect r, UIColor fill, UIColor border,
                f32 border_width, f32 radius)
{
        UIDrawCmd *cmd;

        if(!cv || !(cmd = s_add(cv, UI_DRAW_RECT, r))) return;
        cmd->color = fill;
        cmd->border_color = border;
        cmd->border_width = border_width;
        ui_radius(&cmd->corner_radius, radius);
}

PUB void ui_canvas_gl(UICanvas *cv, UIRect r, UIDrawCallbackFn fn,
                void *data)
{
        UIDrawCmd *cmd;

        if(!cv || !fn || !(cmd = s_add(cv, UI_DRAW_CALLBACK, r))) return;
        cmd->callback = fn;
        cmd->callback_data = data;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

/* Move the shapes to where the canvas is this frame and clip them to it */
LOC void s_draw(UI *ui, UIBox *box, void *data)
{
        UICanvas *cv = data;
        UICanvasShape *sh;
        UIDrawCmd cmd;
        UIRect area = s_content(box, &box->rect);
        UIRect clip = ui_rect_intersect(box->clip, area);

        if(clip.w <= 0.0f || clip.h <= 0.0f) return;

        for(sh = cv->first; sh; sh = sh->next) {
                cmd = sh->cmd;
                cmd.rect.x += area.x;
                cmd.rect.y += area.y;
                cmd.clip = clip;
                ui_draw_push(ui, &cmd);
        }
}

LOC UIDrawCmd *s_add(UICanvas *cv, u8 kind, UIRect r)
{
        UICanvasShape *sh;

        if(r.w <= 0.0f || r.h <= 0.0f) return NULL;
        if(!(sh = arena_alloc(&cv->ui->arena, sizeof(UICanvasShape))))
                return NULL;

        memset(sh, 0, sizeof(*sh));
        sh->cmd.kind = kind;
        sh->cmd.rect = r;

        if(cv->last) {
                cv->last->next = sh;
        } else {
                cv->first = sh;
        }
        cv->last = sh;
        return &sh->cmd;
}

/* rect without the box's padding */
LOC UIRect s_content(const UIBox *box, const UIRect *rect)
{
        const UISides *p = &box->style.padding;

        return ui_rect(rect->x + p->left, rect->y + p->top,
                        rect->w - p->left - p->right,
                        rect->h - p->top - p->bottom);
}
