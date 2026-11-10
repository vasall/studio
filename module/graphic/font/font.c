#include <graphic/font/font.h>

#include <devkit/log/log.h>
#include <devkit/memory/utf8.h>
#include <devkit/util/helper.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Drawn for bytes that are not valid UTF-8 */
#define FONT_REPLACEMENT_CP     0xFFFD

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Create a cache for a pixel size and add it to the font.
 */
LOC GlyphCache *f_add_size(Font *font, f32 px);

/*
 * Free a cache and all its glyph masks.
 */
LOC void f_free_size(GlyphCache *gc);

/*
 * Rasterize a codepoint into glyph.
 */
LOC s32 f_load_glyph(Font *font, GlyphCache *gc, u32 cp, Glyph *glyph);

/*
 * Walk the text glyph by glyph. If img is set, each glyph is drawn with its
 * top left line corner at (x, y). Returns the size, or lines -1 on error.
 */
LOC TextSize f_layout(Font *font, GlyphCache *gc, Image *img, s32 x, s32 y,
                      const char *str, s32 len, IColor col);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 font_init(Font *font, u8 *data, s32 size)
{
        if(!font) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(font_data_init(&font->data, data, size) < 0) {
                return -1;
        }

        if(list_init(&font->sizes, sizeof(GlyphCache *), 4) < 0) {
                LOG(LOG_ERROR, "Failed to create the glyph cache list");

                /* On failure the caller keeps the file data */
                font->data.data = NULL;
                font_data_deinit(&font->data);
                return -1;
        }

        return 0;
}

PUB void font_deinit(Font *font)
{
        s32 i;

        if(!font) {
                return;
        }

        for(i = 0; i < list_count(&font->sizes); i++) {
                f_free_size(*(GlyphCache **)list_get(&font->sizes, i));
        }

        list_deinit(&font->sizes);
        font_data_deinit(&font->data);
}

PUB GlyphCache *font_size(Font *font, f32 px)
{
        s32 i;

        if(!font || px <= 0.0f) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return NULL;
        }

        /* Only a few sizes are used per font, a linear search is enough */
        for(i = 0; i < list_count(&font->sizes); i++) {
                GlyphCache *gc = *(GlyphCache **)list_get(&font->sizes, i);
                if(gc->px == px) {
                        return gc;
                }
        }

        return f_add_size(font, px);
}

PUB const Glyph *font_glyph(Font *font, GlyphCache *gc, u32 cp)
{
        Glyph glyph;
        s32 i;

        if(!font || !gc) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return NULL;
        }

        if(cp < FONT_FAST_GLYPHS) {
                Glyph *slot = &gc->fast[cp];

                if(!slot->loaded && f_load_glyph(font, gc, cp, slot) < 0) {
                        return NULL;
                }

                return slot;
        }

        for(i = 0; i < list_count(&gc->glyphs); i++) {
                Glyph *g = list_get(&gc->glyphs, i);
                if(g->cp == cp) {
                        return g;
                }
        }

        if(f_load_glyph(font, gc, cp, &glyph) < 0) {
                return NULL;
        }

        if(list_push(&gc->glyphs, 1, &glyph) < 0) {
                LOG(LOG_ERROR, "Failed to cache glyph");
                free(glyph.bitmap.mask);
                return NULL;
        }

        return list_get(&gc->glyphs, list_count(&gc->glyphs) - 1);
}

PUB f32 font_kerning(Font *font, GlyphCache *gc, s32 glyph1, s32 glyph2)
{
        if(!font || !gc) {
                return 0.0f;
        }

        return font_data_kerning(&font->data, glyph1, glyph2) * gc->scale;
}

PUB s32 font_draw_text(Font *font, GlyphCache *gc, Image *img, s32 x, s32 y,
                       const char *str, s32 len, IColor col)
{
        if(!font || !gc || !img || !str) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        return f_layout(font, gc, img, x, y, str, len, col).lines < 0 ? -1 : 0;
}

PUB TextSize font_measure_text(Font *font, GlyphCache *gc, const char *str,
                               s32 len)
{
        TextSize size = { 0.0f, 0.0f, 0 };

        if(!font || !gc || !str) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return size;
        }

        size = f_layout(font, gc, NULL, 0, 0, str, len, icolor(0, 0, 0, 0));
        if(size.lines < 0) {
                size.width = 0.0f;
                size.height = 0.0f;
                size.lines = 0;
        }

        return size;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC GlyphCache *f_add_size(Font *font, f32 px)
{
        FontData *fd = &font->data;
        GlyphCache *gc;

        /*
         * Each cache gets its own allocation, so pointers to it stay valid
         * when the size list grows or the Font is moved
         */
        if(!(gc = malloc(sizeof(GlyphCache)))) {
                LOG(LOG_ERROR, "Failed to allocate glyph cache");
                return NULL;
        }

        memset(gc->fast, 0, sizeof(gc->fast));

        if(list_init(&gc->glyphs, sizeof(Glyph), 32) < 0) {
                LOG(LOG_ERROR, "Failed to create the glyph list");
                free(gc);
                return NULL;
        }

        gc->px = px;
        gc->scale = font_data_scale(fd, px);
        gc->ascent = fd->ascent * gc->scale;
        gc->descent = fd->descent * gc->scale;
        gc->line_height = (fd->ascent - fd->descent + fd->line_gap) *
                          gc->scale;

        if(list_push(&font->sizes, 1, &gc) < 0) {
                LOG(LOG_ERROR, "Failed to add glyph cache");
                f_free_size(gc);
                return NULL;
        }

        return gc;
}

LOC void f_free_size(GlyphCache *gc)
{
        s32 i;

        for(i = 0; i < FONT_FAST_GLYPHS; i++) {
                free(gc->fast[i].bitmap.mask);
        }

        for(i = 0; i < list_count(&gc->glyphs); i++) {
                free(((Glyph *)list_get(&gc->glyphs, i))->bitmap.mask);
        }

        list_deinit(&gc->glyphs);
        free(gc);
}

LOC s32 f_load_glyph(Font *font, GlyphCache *gc, u32 cp, Glyph *glyph)
{
        FontData *fd = &font->data;

        glyph->cp = cp;
        glyph->index = font_data_glyph_index(fd, cp);
        glyph->advance = font_data_advance(fd, glyph->index) * gc->scale;

        if(font_data_rasterize(fd, glyph->index, gc->scale,
                               &glyph->bitmap) < 0) {
                return -1;
        }

        glyph->loaded = 1;

        return 0;
}

LOC TextSize f_layout(Font *font, GlyphCache *gc, Image *img, s32 x, s32 y,
                      const char *str, s32 len, IColor col)
{
        TextSize size = { 0.0f, 0.0f, 1 };
        s32 baseline = y + (s32)floorf(gc->ascent + 0.5f);
        s32 prev = -1;          /* Glyph index before, for kerning */
        f32 pen = 0.0f;         /* Pen position, relative to x */
        s32 off = 0;

        if(len < 0) {
                len = (s32)strlen(str);
        }

        while(off < len) {
                const Glyph *g;
                u32 cp;
                s32 n;

                if((n = utf8_decode(str + off, len - off, &cp)) < 1) {
                        cp = FONT_REPLACEMENT_CP;
                        n = 1;
                }
                off += n;

                if(cp == '\n') {
                        size.width = MAX(size.width, pen);
                        size.lines++;
                        baseline += (s32)floorf(gc->line_height + 0.5f);
                        pen = 0.0f;
                        prev = -1;
                        continue;
                }

                if(!(g = font_glyph(font, gc, cp))) {
                        size.lines = -1;
                        return size;
                }

                if(prev >= 0) {
                        pen += font_kerning(font, gc, prev, g->index);
                }

                /* Round each glyph to whole pixels, but keep the pen exact */
                if(img && g->bitmap.mask) {
                        img_blend_mask(img,
                                       x + (s32)floorf(pen + 0.5f) +
                                       g->bitmap.x_off,
                                       baseline + g->bitmap.y_off,
                                       g->bitmap.mask, g->bitmap.w,
                                       g->bitmap.h, col);
                }

                pen += g->advance;
                prev = g->index;
        }

        size.width = MAX(size.width, pen);
        size.height = size.lines * gc->line_height;

        return size;
}
