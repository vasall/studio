#include <render/font.h>

#include <devkit/log/log.h>
#include <devkit/memory/utf8.h>

#include <render/gl_util.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC f32 s_scale(Font *font, f32 px);
LOC u64 s_key(Font *font, f32 px, u32 cp);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 font_load(Font *font, const char *path, u8 id)
{
        memset(font, 0, sizeof(*font));

        font->data = (u8 *)gl_read_file(path, NULL);
        if(!font->data) return -1;

        if(!stbtt_InitFont(&font->info, font->data,
                                stbtt_GetFontOffsetForIndex(font->data, 0))) {
                LOG_S(LOG_ERROR, "font: can't parse %s", (char *)path);
                free(font->data);
                font->data = NULL;
                return -1;
        }

        font->id = id;
        stbtt_GetFontVMetrics(&font->info, &font->ascent, &font->descent,
                        &font->line_gap);
        return 0;
}

PUB void font_unload(Font *font)
{
        free(font->data);
        font->data = NULL;
}

PUB f32 font_line_height(Font *font, f32 px)
{
        f32 s = s_scale(font, px);

        return ceilf((f32)(font->ascent - font->descent + font->line_gap) * s);
}

PUB f32 font_ascent(Font *font, f32 px)
{
        f32 s = s_scale(font, px);
        f32 gap = (f32)font->line_gap * s;

        /* Half of the line gap goes above, so text is centered in the line */
        return roundf((f32)font->ascent * s + gap * 0.5f);
}

PUB f32 font_measure(Font *font, f32 px, const char *text, s32 len)
{
        f32 s = s_scale(font, px);
        f32 w = 0.0f;
        u32 cp;
        u32 prev = 0;
        s32 adv;
        s32 lsb;
        s32 n;
        s32 off = 0;

        if(!font->data) return 0.0f;

        while(off < len) {
                n = utf8_decode(text + off, len - off, &cp);
                if(n <= 0) {
                        cp = 0xfffd;
                        n = 1;
                }
                off += n;

                if(prev) w += font_kern(font, px, prev, cp);
                stbtt_GetCodepointHMetrics(&font->info, (int)cp, &adv, &lsb);
                w += (f32)adv * s;
                prev = cp;
        }
        return w;
}

PUB const AtlasEntry *font_glyph(Font *font, Atlas *atlas, f32 px, u32 cp)
{
        const AtlasEntry *e;
        u64 key = s_key(font, px, cp);
        f32 s = s_scale(font, px);
        s32 x0;
        s32 y0;
        s32 x1;
        s32 y1;
        s32 adv;
        s32 lsb;
        s32 w;
        s32 h;
        u8 *bmp = NULL;

        e = atlas_find(atlas, key);
        if(e) return e;
        if(!font->data) return NULL;

        stbtt_GetCodepointHMetrics(&font->info, (int)cp, &adv, &lsb);
        stbtt_GetCodepointBitmapBox(&font->info, (int)cp, s, s,
                        &x0, &y0, &x1, &y1);
        w = x1 - x0;
        h = y1 - y0;

        if(w > 0 && h > 0) {
                bmp = malloc((size_t)w * (size_t)h);
                if(!bmp) return NULL;
                stbtt_MakeCodepointBitmap(&font->info, bmp, w, h, w, s, s,
                                (int)cp);
        }

        e = atlas_add(atlas, key, w, h, bmp, (f32)x0, (f32)y0, (f32)adv * s);
        free(bmp);
        return e;
}

PUB f32 font_kern(Font *font, f32 px, u32 cp1, u32 cp2)
{
        return (f32)stbtt_GetCodepointKernAdvance(&font->info, (int)cp1,
                        (int)cp2) * s_scale(font, px);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC f32 s_scale(Font *font, f32 px)
{
        return stbtt_ScaleForMappingEmToPixels(&font->info, px);
}

/* font id | size in quarter pixels | codepoint */
LOC u64 s_key(Font *font, f32 px, u32 cp)
{
        u64 q = (u64)(px * 4.0f + 0.5f) & 0xffff;

        return ((u64)font->id << 56) | (q << 32) | (u64)cp;
}
