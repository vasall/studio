#ifndef VASALL_FONT_FAMILY_H
#define VASALL_FONT_FAMILY_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <graphic/font/font.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef enum FontStyle {
        FONT_STYLE_REGULAR,
        FONT_STYLE_BOLD,
        FONT_STYLE_ITALIC,
        FONT_STYLE_BOLD_ITALIC,
        FONT_STYLE_COUNT
} FontStyle;

/*
 * The fonts of one typeface, one per style. The family doesn't own the
 * fonts, they have to stay valid while the family is used (the asset cache
 * takes care of that).
 */
typedef struct FontFamily {
        Font *fonts[FONT_STYLE_COUNT];  /* NULL where the style is missing */
} FontFamily;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Initialize a family without any fonts.
 */
PUB void font_family_init(FontFamily *fam);

/*
 * Set the font of a style, or NULL to remove it.
 *
 * @returns 0 on success, -1 if the style is invalid
 */
PUB s32 font_family_set(FontFamily *fam, FontStyle style, Font *font);

/*
 * Get the font to draw a style with. If the family doesn't have the style,
 * the closest one is used instead:
 *
 *   bold italic -> bold -> italic -> regular
 *   bold        -> regular
 *   italic      -> regular
 *
 * and if even regular is missing, any font the family has.
 *
 * @param fam Pointer to the family
 * @param style The style that is wanted
 * @param got Receives the style of the returned font (may be NULL). If it
 *            differs from style, the text can be made bold or italic in
 *            another way, e.g. by drawing glyphs twice or slanted.
 *
 * @returns The font, or NULL if the family has no fonts at all
 */
PUB Font *font_family_get(FontFamily *fam, FontStyle style, FontStyle *got);

#endif /* VASALL_FONT_FAMILY_H */
