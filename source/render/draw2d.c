#include <render/draw2d.h>

#include <devkit/log/log.h>
#include <devkit/memory/utf8.h>

#include <render/gl_util.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ATLAS_SIZE 2048

enum {
        MODE_RECT = 0,
        MODE_MASK = 1,
        MODE_IMAGE = 2
};

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

typedef struct Draw2DVertex {
        f32 x;
        f32 y;
        f32 u;
        f32 v;
        f32 lx; /* Relative to the rect center */
        f32 ly;
        f32 hw; /* Half size */
        f32 hh;
        f32 radius[4];
        f32 border;
        f32 mode;
        u8 color[4];
        u8 border_color[4];
} Draw2DVertex;

/* Quads with one texture and clip, or one callback */
typedef struct Draw2DBatch {
        GLuint texture;
        s32 clip[4]; /* Scissor x, y, w, h in framebuffer pixels */
        s32 first;   /* First quad */
        s32 count;   /* Number of quads */

        UIDrawCallbackFn callback; /* Set for a callback batch */
        void *data;
        s32 rect[4]; /* Viewport of the callback, framebuffer pixels */
} Draw2DBatch;

typedef struct Quad {
        f32 x0, y0, x1, y1; /* Framebuffer pixels */
        f32 u0, v0, u1, v1;
        f32 radius[4];
        f32 border;
        s32 mode;
        UIColor color;
        UIColor border_color;
} Quad;

LOC void s_push_quad(Draw2D *d, const Quad *q, GLuint tex, const s32 clip[4]);
LOC void s_rect_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s, const s32 clip[4]);
LOC void s_text_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s, const s32 clip[4]);
LOC void s_icon_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s, const s32 clip[4]);
LOC void s_image_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s, const s32 clip[4]);
LOC void s_callback_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s,
                const s32 clip[4]);
LOC void s_bind_state(Draw2D *d, s32 fb_w, s32 fb_h);
LOC void s_ensure_indices(Draw2D *d, s32 quads);
LOC void s_setup_vao(Draw2D *d);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 draw2d_init(Draw2D *d, const char *asset_dir)
{
        char vs[512];
        char fs[512];
        char path[512];
        static const char *font_files[UI_FONT_COUNT] = {
                "fonts/inter/Inter-Regular.ttf",
                "fonts/inter/Inter-Medium.ttf",
                "fonts/inter/Inter-SemiBold.ttf",
                "fonts/jetbrains-mono/JetBrainsMono-Regular.ttf"};
        s32 i;

        memset(d, 0, sizeof(*d));

        snprintf(vs, sizeof(vs), "%s/shaders/ui.vert", asset_dir);
        snprintf(fs, sizeof(fs), "%s/shaders/ui.frag", asset_dir);
        d->program = gl_program_from_files(vs, fs);
        if(!d->program) return -1;
        d->u_screen = glGetUniformLocation(d->program, "u_screen");
        d->u_tex = glGetUniformLocation(d->program, "u_tex");

        for(i = 0; i < UI_FONT_COUNT; i++) {
                snprintf(path, sizeof(path), "%s/%s", asset_dir, font_files[i]);
                if(font_load(&d->fonts[i], path, (u8)i) < 0) {
                        LOG_S(LOG_ERROR, "can't load font %s", path);
                        return -1;
                }
        }

        snprintf(path, sizeof(path), "%s/icons", asset_dir);
        icons_load(&d->icons, path);

        if(atlas_init(&d->atlas, ATLAS_SIZE) < 0) return -1;
        if(list_init(&d->verts, sizeof(Draw2DVertex), 4096) < 0) return -1;
        if(list_init(&d->batches, sizeof(Draw2DBatch), 64) < 0) return -1;

        s_setup_vao(d);
        s_ensure_indices(d, 4096);
        return 0;
}

PUB void draw2d_deinit(Draw2D *d)
{
        s32 i;

        if(d->vbo) glDeleteBuffers(1, &d->vbo);
        if(d->ebo) glDeleteBuffers(1, &d->ebo);
        if(d->vao) glDeleteVertexArrays(1, &d->vao);
        if(d->program) glDeleteProgram(d->program);

        atlas_deinit(&d->atlas);
        for(i = 0; i < UI_FONT_COUNT; i++) {
                font_unload(&d->fonts[i]);
        }

        icons_unload(&d->icons);
        list_deinit(&d->verts);
        list_deinit(&d->batches);
}

PUB void draw2d_render(Draw2D *d, List *cmds, f32 scale, s32 fb_w, s32 fb_h)
{
        Draw2DBatch *b;
        UIDrawCmd *cmd;
        s32 clip[4];
        s32 count = list_count(cmds);
        s32 i;

        list_clear(&d->verts);
        list_clear(&d->batches);
        d->atlas.flushed = 0;

        for(i = 0; i < count; i++) {
                cmd = list_get(cmds, i);

                /* Clip rect in framebuffer pixels, same rounding as rects */
                clip[0] = (s32)roundf(cmd->clip.x * scale);
                clip[1] = (s32)roundf(cmd->clip.y * scale);
                clip[2] = (s32)roundf((cmd->clip.x + cmd->clip.w) * scale) -
                          clip[0];
                clip[3] = (s32)roundf((cmd->clip.y + cmd->clip.h) * scale) -
                          clip[1];
                if(clip[2] <= 0 || clip[3] <= 0) continue;

                switch(cmd->kind) {
                case UI_DRAW_RECT:
                        s_rect_cmd(d, cmd, scale, clip);
                        break;
                case UI_DRAW_TEXT:
                        s_text_cmd(d, cmd, scale, clip);
                        break;
                case UI_DRAW_ICON:
                        s_icon_cmd(d, cmd, scale, clip);
                        break;
                case UI_DRAW_IMAGE:
                        s_image_cmd(d, cmd, scale, clip);
                        break;
                case UI_DRAW_CALLBACK:
                        s_callback_cmd(d, cmd, scale, clip);
                        break;
                }
        }

        d->quads = list_count(&d->verts) / 4;
        d->draw_calls = 0;
        if(list_count(&d->batches) == 0) return;

        if(d->quads > 0) {
                s_ensure_indices(d, d->quads);
                glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
                /* Orphan the old buffer so the driver doesn't wait for the
                   GPU */
                glBufferData(GL_ARRAY_BUFFER,
                             (GLsizeiptr)list_count(&d->verts) *
                                     (GLsizeiptr)sizeof(Draw2DVertex),
                             NULL, GL_STREAM_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0,
                                (GLsizeiptr)list_count(&d->verts) *
                                        (GLsizeiptr)sizeof(Draw2DVertex),
                                list_get(&d->verts, 0));
        }
        s_bind_state(d, fb_w, fb_h);

        for(i = 0; i < list_count(&d->batches); i++) {
                b = list_get(&d->batches, i);

                if(b->callback) {
                        UIDrawCallbackInfo info;

                        info.x = b->rect[0];
                        info.y = b->rect[1];
                        info.w = b->rect[2];
                        info.h = b->rect[3];
                        info.fb_w = fb_w;
                        info.fb_h = fb_h;
                        info.scale = scale;

                        /* GL has its origin at the bottom left */
                        glViewport(b->rect[0], fb_h - b->rect[1] - b->rect[3],
                                   b->rect[2], b->rect[3]);
                        glScissor(b->clip[0], fb_h - b->clip[1] - b->clip[3],
                                  b->clip[2], b->clip[3]);
                        b->callback(&info, b->data);
                        d->draw_calls++;

                        /* It may have changed anything */
                        s_bind_state(d, fb_w, fb_h);
                        continue;
                }

                glBindTexture(GL_TEXTURE_2D, b->texture);
                /* GL scissor has its origin at the bottom left */
                glScissor(b->clip[0], fb_h - b->clip[1] - b->clip[3],
                          b->clip[2], b->clip[3]);
                glDrawElements(GL_TRIANGLES, b->count * 6, GL_UNSIGNED_INT,
                               (void *)((size_t)b->first * 6 * sizeof(u32)));
                d->draw_calls++;
        }

        glDisable(GL_SCISSOR_TEST);
        glBindVertexArray(0);
}

PUB f32 draw2d_measure(Draw2D *d, u8 font, f32 size, f32 scale,
                       const char *text, s32 len)
{
        if(font >= UI_FONT_COUNT || scale <= 0.0f) return 0.0f;
        return font_measure(&d->fonts[font], roundf(size * scale), text, len) /
               scale;
}

PUB f32 draw2d_line_height(Draw2D *d, u8 font, f32 size, f32 scale)
{
        if(font >= UI_FONT_COUNT || scale <= 0.0f) return 0.0f;
        return font_line_height(&d->fonts[font], roundf(size * scale)) / scale;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC void s_push_quad(Draw2D *d, const Quad *q, GLuint tex, const s32 clip[4])
{
        Draw2DVertex v[4];
        Draw2DBatch *last;
        Draw2DBatch nb;
        f32 cx = (q->x0 + q->x1) * 0.5f;
        f32 cy = (q->y0 + q->y1) * 0.5f;
        f32 hw = (q->x1 - q->x0) * 0.5f;
        f32 hh = (q->y1 - q->y0) * 0.5f;
        s32 i;

        for(i = 0; i < 4; i++) {
                f32 x = (i == 1 || i == 2) ? q->x1 : q->x0;
                f32 y = (i >= 2) ? q->y1 : q->y0;

                v[i].x = x;
                v[i].y = y;
                v[i].u = (i == 1 || i == 2) ? q->u1 : q->u0;
                v[i].v = (i >= 2) ? q->v1 : q->v0;
                v[i].lx = x - cx;
                v[i].ly = y - cy;
                v[i].hw = hw;
                v[i].hh = hh;
                memcpy(v[i].radius, q->radius, sizeof(v[i].radius));
                v[i].border = q->border;
                v[i].mode = (f32)q->mode;
                v[i].color[0] = q->color.r;
                v[i].color[1] = q->color.g;
                v[i].color[2] = q->color.b;
                v[i].color[3] = q->color.a;
                v[i].border_color[0] = q->border_color.r;
                v[i].border_color[1] = q->border_color.g;
                v[i].border_color[2] = q->border_color.b;
                v[i].border_color[3] = q->border_color.a;
        }

        /* Same texture and clip as the last batch: extend it */
        last = list_count(&d->batches) > 0 ?
                       list_get(&d->batches, list_count(&d->batches) - 1) :
                       NULL;
        if(last && !last->callback && last->texture == tex &&
           memcmp(last->clip, clip, sizeof(last->clip)) == 0) {
                last->count++;
        } else {
                memset(&nb, 0, sizeof(nb));
                nb.texture = tex;
                memcpy(nb.clip, clip, sizeof(nb.clip));
                nb.first = list_count(&d->verts) / 4;
                nb.count = 1;
                list_push(&d->batches, 1, &nb);
        }
        list_push(&d->verts, 4, v);
}

LOC void s_rect_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s, const s32 clip[4])
{
        Quad q;
        f32 max_r;
        s32 i;

        memset(&q, 0, sizeof(q));

        /* Snap edges to pixels so 1px borders stay sharp */
        q.x0 = roundf(cmd->rect.x * s);
        q.y0 = roundf(cmd->rect.y * s);
        q.x1 = roundf((cmd->rect.x + cmd->rect.w) * s);
        q.y1 = roundf((cmd->rect.y + cmd->rect.h) * s);
        if(q.x1 <= q.x0 || q.y1 <= q.y0) return;

        max_r = (q.x1 - q.x0 < q.y1 - q.y0 ? q.x1 - q.x0 : q.y1 - q.y0) * 0.5f;
        for(i = 0; i < 4; i++) {
                q.radius[i] = cmd->corner_radius[i] * s;
                if(q.radius[i] > max_r) q.radius[i] = max_r;
        }
        q.border = cmd->border_width > 0.0f ?
                           (cmd->border_width * s < 1.0f ?
                                    1.0f :
                                    roundf(cmd->border_width * s)) :
                           0.0f;
        q.mode = MODE_RECT;
        q.color = cmd->color;
        q.border_color = cmd->border_color;
        s_push_quad(d, &q, d->atlas.texture, clip);
}

LOC void s_text_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s, const s32 clip[4])
{
        Font *font;
        const AtlasEntry *g;
        Quad q;
        f32 px;
        f32 pen;
        f32 baseline;
        f32 inv = 1.0f / (f32)d->atlas.size;
        u32 cp;
        u32 prev = 0;
        s32 off = 0;
        s32 n;

        if(cmd->font >= UI_FONT_COUNT) return;
        font = &d->fonts[cmd->font];
        px = roundf(cmd->font_size * s);

        pen = roundf(cmd->rect.x * s);
        baseline = roundf(cmd->rect.y * s) + font_ascent(font, px);

        memset(&q, 0, sizeof(q));
        q.mode = MODE_MASK;
        q.color = cmd->color;

        while(off < cmd->text_len) {
                n = utf8_decode(cmd->text + off, cmd->text_len - off, &cp);
                if(n <= 0) {
                        cp = 0xfffd;
                        n = 1;
                }
                off += n;

                if(prev) pen += font_kern(font, px, prev, cp);
                prev = cp;

                g = font_glyph(font, &d->atlas, px, cp);
                if(!g) continue;

                if(g->w > 0 && g->h > 0) {
                        q.x0 = roundf(pen + g->xoff);
                        q.y0 = baseline + g->yoff;
                        q.x1 = q.x0 + (f32)g->w;
                        q.y1 = q.y0 + (f32)g->h;
                        q.u0 = (f32)g->x * inv;
                        q.v0 = (f32)g->y * inv;
                        q.u1 = (f32)(g->x + g->w) * inv;
                        q.v1 = (f32)(g->y + g->h) * inv;
                        s_push_quad(d, &q, d->atlas.texture, clip);
                }
                pen += g->advance;
        }
}

LOC void s_icon_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s, const s32 clip[4])
{
        const AtlasEntry *e;
        Quad q;
        f32 inv = 1.0f / (f32)d->atlas.size;
        s32 px = (s32)roundf(cmd->rect.w * s);

        e = icons_get(&d->icons, &d->atlas, cmd->icon, px);
        if(!e) return;

        memset(&q, 0, sizeof(q));
        q.x0 = roundf(cmd->rect.x * s);
        q.y0 = roundf(cmd->rect.y * s);
        q.x1 = q.x0 + (f32)e->w;
        q.y1 = q.y0 + (f32)e->h;
        q.u0 = (f32)e->x * inv;
        q.v0 = (f32)e->y * inv;
        q.u1 = (f32)(e->x + e->w) * inv;
        q.v1 = (f32)(e->y + e->h) * inv;
        q.mode = MODE_MASK;
        q.color = cmd->color;
        s_push_quad(d, &q, d->atlas.texture, clip);
}

LOC void s_image_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s, const s32 clip[4])
{
        Quad q;

        memset(&q, 0, sizeof(q));
        q.x0 = roundf(cmd->rect.x * s);
        q.y0 = roundf(cmd->rect.y * s);
        q.x1 = roundf((cmd->rect.x + cmd->rect.w) * s);
        q.y1 = roundf((cmd->rect.y + cmd->rect.h) * s);
        /* GL textures are bottom-up */
        q.u0 = 0.0f;
        q.v0 = 1.0f;
        q.u1 = 1.0f;
        q.v1 = 0.0f;
        q.mode = MODE_IMAGE;
        q.color = cmd->color.a ? cmd->color : ui_rgba(255, 255, 255, 255);
        s_push_quad(d, &q, cmd->texture, clip);
}

/* A batch of its own, so quads before and after it keep their order */
LOC void s_callback_cmd(Draw2D *d, const UIDrawCmd *cmd, f32 s,
                const s32 clip[4])
{
        Draw2DBatch nb;

        if(!cmd->callback) return;

        memset(&nb, 0, sizeof(nb));
        nb.callback = cmd->callback;
        nb.data = cmd->callback_data;
        memcpy(nb.clip, clip, sizeof(nb.clip));
        nb.rect[0] = (s32)roundf(cmd->rect.x * s);
        nb.rect[1] = (s32)roundf(cmd->rect.y * s);
        nb.rect[2] = (s32)roundf((cmd->rect.x + cmd->rect.w) * s) - nb.rect[0];
        nb.rect[3] = (s32)roundf((cmd->rect.y + cmd->rect.h) * s) - nb.rect[1];
        nb.first = list_count(&d->verts) / 4;
        if(nb.rect[2] <= 0 || nb.rect[3] <= 0) return;
        list_push(&d->batches, 1, &nb);
}

/* GL state for drawing quads; set again after every callback */
LOC void s_bind_state(Draw2D *d, s32 fb_w, s32 fb_h)
{
        glViewport(0, 0, fb_w, fb_h);
        glUseProgram(d->program);
        glUniform2f(d->u_screen, (f32)fb_w, (f32)fb_h);
        glUniform1i(d->u_tex, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(d->vao);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_SCISSOR_TEST);
}

LOC void s_ensure_indices(Draw2D *d, s32 quads)
{
        u32 *idx;
        s32 cap;
        s32 i;

        if(quads <= d->ebo_quads) return;

        cap = d->ebo_quads > 0 ? d->ebo_quads : 1024;
        while(cap < quads)
                cap *= 2;

        idx = malloc((size_t)cap * 6 * sizeof(u32));
        if(!idx) return;
        for(i = 0; i < cap; i++) {
                idx[i * 6 + 0] = (u32)(i * 4 + 0);
                idx[i * 6 + 1] = (u32)(i * 4 + 1);
                idx[i * 6 + 2] = (u32)(i * 4 + 2);
                idx[i * 6 + 3] = (u32)(i * 4 + 0);
                idx[i * 6 + 4] = (u32)(i * 4 + 2);
                idx[i * 6 + 5] = (u32)(i * 4 + 3);
        }

        glBindVertexArray(d->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)cap * 6 * (GLsizeiptr)sizeof(u32), idx,
                     GL_STATIC_DRAW);
        glBindVertexArray(0);
        free(idx);
        d->ebo_quads = cap;
}

LOC void s_setup_vao(Draw2D *d)
{
        GLsizei stride = sizeof(Draw2DVertex);
        void *offset;

        glGenVertexArrays(1, &d->vao);
        glGenBuffers(1, &d->vbo);
        glGenBuffers(1, &d->ebo);

        glBindVertexArray(d->vao);
        glBindBuffer(GL_ARRAY_BUFFER, d->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d->ebo);

        glEnableVertexAttribArray(0);
        offset = (void *)offsetof(Draw2DVertex, x);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, offset);

        glEnableVertexAttribArray(1);
        offset = (void *)offsetof(Draw2DVertex, u);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,offset);

        glEnableVertexAttribArray(2);
        offset = (void *)offsetof(Draw2DVertex, lx);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, offset);

        glEnableVertexAttribArray(3);
        offset = (void *)offsetof(Draw2DVertex, hw);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, offset);

        glEnableVertexAttribArray(4);
        offset = (void *)offsetof(Draw2DVertex, radius);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, offset);

        glEnableVertexAttribArray(5);
        offset =(void *)offsetof(Draw2DVertex, border);
        glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, stride, offset);

        glEnableVertexAttribArray(6);
        offset = (void *)offsetof(Draw2DVertex, color);
        glVertexAttribPointer(6, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, offset);

        glEnableVertexAttribArray(7);
        offset = (void *)offsetof(Draw2DVertex, border_color);
        glVertexAttribPointer(7, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, offset);

        glBindVertexArray(0);
}
