#include "loader.h"

#include <devkit/log/log.h>

#include <stdlib.h>

/******************************************************************************
 *
 * 				PRIVATE FUNCTIONS
 *
 ******************************************************************************/

PRV s32 load_font(const char *path, Font *font)
{
        u8 *file;
        s32 size;

        if(!path || !font) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(load_file(path, &file, &size) < 0) {
                return -1;
        }

        /* The font keeps using the file data, so it takes ownership of it */
        if(font_init(font, file, size) < 0) {
                LOG_S(LOG_ERROR, "Failed to load font %s", (char *)path);
                free(file);
                return -1;
        }

        return 0;
}
