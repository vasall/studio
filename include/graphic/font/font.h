#ifndef VASALL_FONT_H
#define VASALL_FONT_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/list.h>

#include <graphic/font/font_data.h>
#include <graphic/general/color.h>
#include <graphic/image/image.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

/* Codepoints below this are cached in a plain array for fast lookup */
#define FONT_FAST_GLYPHS        128

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * A glyph rasterized at one pixel size.
 */
typedef struct Glyph {
        u32 cp;                 /* The unicode codepoint */
        s32 index;              /* The glyph index in the font, for kerning */
        f32 advance;            /* How far the pen moves, in pixels */
        GlyphBitmap bitmap;     /* The coverage mask, mask NULL if empty */
        s32 loaded;             /* 0 for unused slots of the fast array */
} Glyph;

/*
 * All glyphs of a font that were used at one pixel size. Line metrics are
 * already scaled to pixels.
 */
typedef struct GlyphCache {
        f32 px;                 /* The pixel height this cache is for */
        f32 scale;              /* Font units to pixels */

        f32 ascent;             /* Above the baseline */
        f32 descent;            /* Below the baseline (negative) */
        f32 line_height;        /* From one baseline to the next */

        Glyph fast[FONT_FAST_GLYPHS];
        List glyphs;            /* List of Glyph, all other codepoints */
} GlyphCache;

/*
 * The size of a piece of text, in pixels.
 */
typedef struct TextSize {
        f32 width;              /* Of the widest line */
        f32 height;             /* lines * line_height */
        s32 lines;              /* Lines separated by '\n' */
} TextSize;

/*
 * A font with its glyph caches. Glyphs are rasterized the first time they
 * are used at a size and kept until the font is freed.
 */
typedef struct Font {
        FontData data;
        List sizes;             /* List of GlyphCache *, one per size */
} Font;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Create a font from the contents of a font file. On success the font takes
 * ownership of data and frees it in font_deinit. On failure data is left
 * untouched and still belongs to the caller.
 *
 * @param font Pointer to the font
 * @param data The contents of a .ttf or .otf file, allocated with malloc
 * @param size The number of bytes in data
 *
 * @returns 0 on success, -1 if the data is not a usable font
 */
PUB s32 font_init(Font *font, u8 *data, s32 size);

/*
 * Free the font, all glyph caches and the file data.
 */
PUB void font_deinit(Font *font);

/*
 * Get the glyph cache for a pixel size, creating it on first use. The
 * pointer stays valid until font_deinit, also when the Font itself is moved
 * (e.g. by the asset cache).
 *
 * @param font Pointer to the font
 * @param px The pixel height, e.g. 14
 *
 * @returns The cache, or NULL if an error occurred
 */
PUB GlyphCache *font_size(Font *font, f32 px);

/*
 * Get a glyph at the cache's size, rasterizing it on first use. Codepoints
 * the font doesn't have give the font's "missing glyph" symbol.
 *
 * The pointer is only valid until the next font_glyph call with the same
 * cache, since adding a glyph may move the others.
 *
 * @returns The glyph, or NULL if an error occurred
 */
PUB const Glyph *font_glyph(Font *font, GlyphCache *gc, u32 cp);

/*
 * The extra distance between two glyphs that follow each other, in pixels.
 * Pass the glyph indices (Glyph.index).
 */
PUB f32 font_kerning(Font *font, GlyphCache *gc, s32 glyph1, s32 glyph2);

/*
 * Draw UTF-8 text into an image. Lines are separated by '\n' and start at x.
 * Invalid UTF-8 bytes are drawn as U+FFFD. There is no wrapping or shaping,
 * glyphs are placed one after another with kerning.
 *
 * @param font Pointer to the font
 * @param gc The glyph cache for the size to draw with
 * @param img The image to draw into
 * @param x The left edge of the text
 * @param y The top of the first line (the baseline is ascent below it)
 * @param str The text, UTF-8
 * @param len The number of bytes in str, or -1 if it ends with a 0 byte
 * @param col The color, with straight alpha
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 font_draw_text(Font *font, GlyphCache *gc, Image *img, s32 x, s32 y,
                       const char *str, s32 len, IColor col);

/*
 * Measure UTF-8 text the same way font_draw_text places it. The width is
 * the distance the pen moves; slanted glyphs can reach a bit past it.
 *
 * @param font Pointer to the font
 * @param gc The glyph cache for the size to measure with
 * @param str The text, UTF-8
 * @param len The number of bytes in str, or -1 if it ends with a 0 byte
 *
 * @returns The size; all 0 if an error occurred
 */
PUB TextSize font_measure_text(Font *font, GlyphCache *gc, const char *str,
                               s32 len);

#endif /* VASALL_FONT_H */
