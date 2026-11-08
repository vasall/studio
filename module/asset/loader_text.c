#include "loader.h"

#include <devkit/log/log.h>

#include <stdlib.h>
#include <string.h>

/******************************************************************************
 *
 * 				PRIVATE FUNCTIONS
 *
 ******************************************************************************/

PRV s32 load_text(const char *path, Text *txt)
{
        static const char bom[] = "\xEF\xBB\xBF";
        u8 *file;
        s32 size;
        s32 skip = 0;
        s32 ret;

        if(!path || !txt) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(load_file(path, &file, &size) < 0) {
                return -1;
        }

        /* Some editors put a byte order mark at the start of UTF-8 files */
        if(size >= 3 && memcmp(file, bom, 3) == 0) {
                skip = 3;
        }

        /* text_init copies the data and checks that it is valid UTF-8 */
        if((ret = text_init(txt, size - skip, (char *)file + skip)) < 0) {
                LOG_S(LOG_ERROR, "Failed to load text %s", (char *)path);
        }

        free(file);
        return ret;
}
