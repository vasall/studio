#include <core/path.h>

#include <devkit/log/log.h>

#include <string.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_clear(Path *p);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB Path mkpath(const char *src)
{
        Path pth;
        s32 length = strlen(src);

        if(!src) {
                LOG(LOG_WARN, "Parameter(s) invalid");
                goto return_empty;
        }

        if(length + 1 > PATH_LENGTH_MAX) {
                LOG(LOG_WARN, "Path to long for buffer");
                goto return_empty;
        }

        strncpy(pth.cstr, src, length);
        pth.cstr[length] = '\0';

        return pth;

return_empty:
        s_clear(&pth);
        return pth;
}

PUB char *path_cstr(Path *p)
{
        return p ? p->cstr : "";
}

PUB Path path_join(Path p1, Path p2)
{
        Path pth;
        s32 len1 = strlen(p1.cstr);
        s32 len2 = strlen(p2.cstr);
        s32 off = len1;

        if(len1 + len2 + 1 > PATH_LENGTH_MAX) {
                LOG(LOG_WARN, "Combined path to long for buffer");
                s_clear(&pth);
                return pth;
        }

        strncpy(pth.cstr, p1.cstr, len1);

        if(p2.cstr[0] != '/') {
                pth.cstr[len1] = '/';
                off++;
        }

        strncpy(&pth.cstr[off], p2.cstr, len2);
        pth.cstr[len1 + len2] = '\0';

        return pth;
}

PUB Path path_resolve(Path p)
{
        Path out;
        s32 starts[PATH_LENGTH_MAX / 2 + 1];    /* Output length before each
                                                   kept part, to cut it off */
        s32 depth = 0;                          /* Kept parts */
        s32 ups = 0;                            /* Of those, leading ".." */
        s32 absolute = p.cstr[0] == '/';
        s32 len = 0;
        const char *s = p.cstr;
        const char *part;
        s32 n;

        if(absolute) out.cstr[len++] = '/';

        while(*s) {
                /* Next part between slashes */
                while(*s == '/') s++;
                part = s;
                while(*s && *s != '/') s++;
                n = (s32)(s - part);

                if(n == 0 || (n == 1 && part[0] == '.')) continue;

                if(n == 2 && part[0] == '.' && part[1] == '.') {
                        if(depth > ups) {
                                /* Go up: drop the last real part */
                                depth--;
                                len = starts[depth];
                                continue;
                        }
                        if(absolute) continue;
                        ups++;
                }

                /* Keep the part, with a separator unless right after "/" */
                starts[depth++] = len;
                if(len > 0 && out.cstr[len - 1] != '/') out.cstr[len++] = '/';
                memcpy(out.cstr + len, part, (size_t)n);
                len += n;
        }

        /* Never longer than the input (or "."), so it always fits */
        if(len == 0) out.cstr[len++] = '.';
        out.cstr[len] = '\0';
        return out;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC void s_clear(Path *p)
{
        *p->cstr = '\0';
}
