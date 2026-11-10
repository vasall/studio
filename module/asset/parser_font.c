#include <asset/parser.h>

#include <devkit/log/log.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 parse_font(u8 *data, s32 size, Font *font)
{
        if(!data || !font) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        /* On success the font owns data, on failure the caller still does */
        return font_init(font, data, size);
}
