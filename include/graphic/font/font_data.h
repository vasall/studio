#ifndef VASALL_FONT_DATA_H
#define VASALL_FONT_DATA_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * A parsed TrueType or OpenType font file. Metrics are in font units and
 * have to be multiplied with font_data_scale to get pixels.
 *
 * Most code should use Font, which adds glyph caches on top of this.
 */
typedef struct FontData {
        u8 *data;       /* The font file, owned by the font data */
        s32 size;       /* The number of bytes in data */

        void *info;     /* Parsed font, only used inside font_data.c */

        s32 ascent;     /* Height above the baseline, in font units */
        s32 descent;    /* Depth below the baseline (negative) */
        s32 line_gap;   /* Extra space between lines */
} FontData;

/*
 * A rasterized glyph: 8-bit coverage, 0 = empty, 255 = fully covered.
 */
typedef struct GlyphBitmap {
        u8 *mask;       /* w * h bytes, row by row from the top, or NULL */
        s32 w;
        s32 h;
        s32 x_off;      /* From the pen position on the baseline to the */
        s32 y_off;      /* top left of the mask, y grows downwards     */
} GlyphBitmap;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Parse the contents of a font file. On success the font data takes
 * ownership of data and frees it in font_data_deinit. On failure data is
 * left untouched and still belongs to the caller.
 *
 * @param fd Pointer to the font data
 * @param data The contents of a .ttf or .otf file, allocated with malloc
 * @param size The number of bytes in data
 *
 * @returns 0 on success, -1 if the data is not a usable font
 */
PUB s32 font_data_init(FontData *fd, u8 *data, s32 size);

/*
 * Free the parsed font and its file data.
 */
PUB void font_data_deinit(FontData *fd);

/*
 * The factor from font units to pixels, so that ascent - descent is px
 * pixels high.
 */
PUB f32 font_data_scale(FontData *fd, f32 px);

/*
 * The glyph index of a unicode codepoint. Returns 0 if the font has no glyph
 * for it; glyph 0 is the font's "missing glyph" symbol.
 */
PUB s32 font_data_glyph_index(FontData *fd, u32 cp);

/*
 * How far the pen moves after the glyph, in font units.
 */
PUB s32 font_data_advance(FontData *fd, s32 glyph);

/*
 * The extra distance between two glyphs that follow each other, in font
 * units, usually 0 or negative. Only the font's "kern" table is read.
 */
PUB s32 font_data_kerning(FontData *fd, s32 glyph1, s32 glyph2);

/*
 * Rasterize a glyph. Glyphs without an outline, like the space, give an
 * empty bitmap with mask NULL.
 *
 * @param fd Pointer to the font data
 * @param glyph The glyph index
 * @param scale The scale from font_data_scale
 * @param bmp Receives the bitmap, free bmp->mask with free()
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 font_data_rasterize(FontData *fd, s32 glyph, f32 scale,
                            GlyphBitmap *bmp);

#endif /* VASALL_FONT_DATA_H */
