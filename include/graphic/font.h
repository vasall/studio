#ifndef VASALL_FONT_H
#define VASALL_FONT_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * A TrueType or OpenType font. The font can be drawn at any size; metrics
 * are in font units and have to be scaled to a pixel size.
 */
typedef struct Font {
        u8 *data;       /* The font file, owned by the font */
        s32 size;       /* The number of bytes in data */

        void *info;     /* Parsed font, only used inside font.c */

        s32 ascent;     /* Height above the baseline, in font units */
        s32 descent;    /* Depth below the baseline (negative) */
        s32 line_gap;   /* Extra space between lines */
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
 * Free the font and its file data.
 */
PUB void font_deinit(Font *font);

#endif /* VASALL_FONT_H */
