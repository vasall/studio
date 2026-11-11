#ifndef VASALL_STUDIO_FONT_H
#define VASALL_STUDIO_FONT_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <stb_truetype.h>

#include <render/atlas.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct Font {
        stbtt_fontinfo info;
        u8 *data;               /* The TTF file, must outlive info */
        u8 id;                  /* Part of the atlas key */
        s32 ascent;             /* Font units */
        s32 descent;
        s32 line_gap;
} Font;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Load a TTF file.
 *
 * @returns 0 on success, -1 on error
 */
PUB s32 font_load(Font *font, const char *path, u8 id);
PUB void font_unload(Font *font);

/*
 * Metrics for a font size in pixels (the em size, like CSS font-size).
 * The line height is rounded to whole pixels.
 */
PUB f32 font_line_height(Font *font, f32 px);
PUB f32 font_ascent(Font *font, f32 px);

/*
 * Width of a UTF-8 run in pixels (sum of advances and kerning).
 */
PUB f32 font_measure(Font *font, f32 px, const char *text, s32 len);

/*
 * The glyph for a codepoint, rasterized into the atlas on first use.
 */
PUB const AtlasEntry *font_glyph(Font *font, Atlas *atlas, f32 px, u32 cp);

/*
 * Kerning between two codepoints in pixels.
 */
PUB f32 font_kern(Font *font, f32 px, u32 cp1, u32 cp2);

#endif /* VASALL_STUDIO_FONT_H */
