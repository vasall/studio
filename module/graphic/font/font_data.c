#include <graphic/font/font_data.h>

#include <devkit/log/log.h>

#include <stdlib.h>
#include <string.h>

/*
 * stb_truetype is compiled here and only here. Its types stay out of
 * font_data.h, the parsed font is kept behind FontData.info.
 */
#define STB_TRUETYPE_IMPLEMENTATION

/* stb_truetype is third-party code, don't drown our warnings in its own */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <stb_truetype.h>
#pragma GCC diagnostic pop

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 font_data_init(FontData *fd, u8 *data, s32 size)
{
        stbtt_fontinfo *info;
        s32 offset;
        int ascent;
        int descent;
        int line_gap;

        if(!fd || !data || size < 12) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        /* Font collections can hold several fonts, use the first one */
        if((offset = stbtt_GetFontOffsetForIndex(data, 0)) < 0 ||
                        offset >= size) {
                LOG(LOG_ERROR, "Not a TrueType or OpenType font");
                return -1;
        }

        if(!(info = malloc(sizeof(stbtt_fontinfo)))) {
                LOG(LOG_ERROR, "Failed to allocate memory for font");
                return -1;
        }

        if(!stbtt_InitFont(info, data, offset)) {
                LOG(LOG_ERROR, "Failed to parse font");
                free(info);
                return -1;
        }

        stbtt_GetFontVMetrics(info, &ascent, &descent, &line_gap);

        fd->data = data;
        fd->size = size;
        fd->info = info;
        fd->ascent = ascent;
        fd->descent = descent;
        fd->line_gap = line_gap;

        return 0;
}

PUB void font_data_deinit(FontData *fd)
{
        if(!fd) {
                return;
        }

        free(fd->info);
        free(fd->data);
        fd->info = NULL;
        fd->data = NULL;
        fd->size = 0;
}

PUB f32 font_data_scale(FontData *fd, f32 px)
{
        return stbtt_ScaleForPixelHeight(fd->info, px);
}

PUB s32 font_data_glyph_index(FontData *fd, u32 cp)
{
        return stbtt_FindGlyphIndex(fd->info, (int)cp);
}

PUB s32 font_data_advance(FontData *fd, s32 glyph)
{
        int advance;
        int left_bearing;

        stbtt_GetGlyphHMetrics(fd->info, glyph, &advance, &left_bearing);

        return advance;
}

PUB s32 font_data_kerning(FontData *fd, s32 glyph1, s32 glyph2)
{
        return stbtt_GetGlyphKernAdvance(fd->info, glyph1, glyph2);
}

PUB s32 font_data_rasterize(FontData *fd, s32 glyph, f32 scale,
                            GlyphBitmap *bmp)
{
        int x0;
        int y0;
        int x1;
        int y1;

        if(!fd || !bmp) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        memset(bmp, 0, sizeof(*bmp));

        stbtt_GetGlyphBitmapBox(fd->info, glyph, scale, scale,
                                &x0, &y0, &x1, &y1);

        bmp->x_off = x0;
        bmp->y_off = y0;

        /* Nothing to draw, e.g. a space */
        if(x1 <= x0 || y1 <= y0 || stbtt_IsGlyphEmpty(fd->info, glyph)) {
                return 0;
        }

        bmp->w = x1 - x0;
        bmp->h = y1 - y0;

        if(!(bmp->mask = malloc(bmp->w * bmp->h))) {
                LOG(LOG_ERROR, "Failed to allocate memory for glyph");
                bmp->w = 0;
                bmp->h = 0;
                return -1;
        }

        stbtt_MakeGlyphBitmap(fd->info, bmp->mask, bmp->w, bmp->h, bmp->w,
                              scale, scale, glyph);

        return 0;
}
