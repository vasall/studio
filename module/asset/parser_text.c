#include <asset/parser.h>

#include <devkit/log/log.h>

#include <string.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 parse_text(const u8 *data, s32 size, Text *txt)
{
        static const char bom[] = "\xEF\xBB\xBF";
        s32 skip = 0;

        if(!data || size < 0 || !txt) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        /* Some editors put a byte order mark at the start of UTF-8 files */
        if(size >= 3 && memcmp(data, bom, 3) == 0) {
                skip = 3;
        }

        /* text_init copies the data and checks that it is valid UTF-8 */
        return text_init(txt, size - skip, (const char *)data + skip);
}
