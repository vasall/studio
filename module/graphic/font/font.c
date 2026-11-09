#include <graphic/font/font.h>

#include <devkit/log/log.h>

#include <stdlib.h>

/*
 * stb_truetype is compiled here and only here. Its types stay out of font.h,
 * the parsed font is kept behind Font.info.
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

PUB s32 font_init(Font *font, u8 *data, s32 size)
{
        stbtt_fontinfo *info;
        s32 offset;
        int ascent;
        int descent;
        int line_gap;

        if(!font || !data || size < 12) {
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

        font->data = data;
        font->size = size;
        font->info = info;
        font->ascent = ascent;
        font->descent = descent;
        font->line_gap = line_gap;

        return 0;
}

PUB void font_deinit(Font *font)
{
        if(!font) {
                return;
        }

        free(font->info);
        free(font->data);
        font->info = NULL;
        font->data = NULL;
        font->size = 0;
}
