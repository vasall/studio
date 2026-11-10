#include <graphic/font/font_family.h>

#include <devkit/log/log.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void font_family_init(FontFamily *fam)
{
        s32 i;

        if(!fam) {
                return;
        }

        for(i = 0; i < FONT_STYLE_COUNT; i++) {
                fam->fonts[i] = NULL;
        }
}

PUB s32 font_family_set(FontFamily *fam, FontStyle style, Font *font)
{
        if(!fam || style < 0 || style >= FONT_STYLE_COUNT) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        fam->fonts[style] = font;

        return 0;
}

PUB Font *font_family_get(FontFamily *fam, FontStyle style, FontStyle *got)
{
        /* The styles to try for each wanted style, best first */
        static const FontStyle order[FONT_STYLE_COUNT][4] = {
                { FONT_STYLE_REGULAR, FONT_STYLE_REGULAR,
                  FONT_STYLE_REGULAR, FONT_STYLE_REGULAR },
                { FONT_STYLE_BOLD, FONT_STYLE_REGULAR,
                  FONT_STYLE_REGULAR, FONT_STYLE_REGULAR },
                { FONT_STYLE_ITALIC, FONT_STYLE_REGULAR,
                  FONT_STYLE_REGULAR, FONT_STYLE_REGULAR },
                { FONT_STYLE_BOLD_ITALIC, FONT_STYLE_BOLD,
                  FONT_STYLE_ITALIC, FONT_STYLE_REGULAR }
        };
        s32 i;

        if(!fam) {
                return NULL;
        }

        if(style < 0 || style >= FONT_STYLE_COUNT) {
                style = FONT_STYLE_REGULAR;
        }

        for(i = 0; i < 4; i++) {
                FontStyle s = order[style][i];

                if(fam->fonts[s]) {
                        if(got) *got = s;
                        return fam->fonts[s];
                }
        }

        /* Not even regular: better any font than no text */
        for(i = 0; i < FONT_STYLE_COUNT; i++) {
                if(fam->fonts[i]) {
                        if(got) *got = (FontStyle)i;
                        return fam->fonts[i];
                }
        }

        return NULL;
}
