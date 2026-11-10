#include <asset/loader.h>

#include <devkit/log/log.h>

#include <stdio.h>
#include <string.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Check that the path is relative, not empty, short enough and doesn't leave
 * the base dir with "..".
 *
 * @returns 0 if the path can be used, -1 if not
 */
LOC s32 l_check_path(const char *path);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 loader_init(Loader *ld, const char *base_dir)
{
        size_t len;

        if(!ld || !base_dir) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if((len = strlen(base_dir)) == 0 || len >= sizeof(ld->base_dir)) {
                LOG(LOG_ERROR, "Invalid loader base dir");
                return -1;
        }

        memcpy(ld->base_dir, base_dir, len + 1);

        /* Without a trailing '/', so joining always adds exactly one */
        while(len > 1 && ld->base_dir[len - 1] == '/') {
                ld->base_dir[--len] = '\0';
        }

        return 0;
}

PUB s32 loader_open(Loader *ld, const char *path, FileHandle *fh)
{
        char full[FILE_PATH_LENGTH];
        s32 n;

        if(!ld || !path || !fh) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(l_check_path(path) < 0) {
                LOG_S(LOG_ERROR, "Invalid asset path: %s", (char *)path);
                return -1;
        }

        n = snprintf(full, sizeof(full), "%s/%s", ld->base_dir, path);
        if(n < 0 || n >= (s32)sizeof(full)) {
                LOG_S(LOG_ERROR, "Asset path is too long: %s", (char *)path);
                return -1;
        }

        return file_init(fh, full);
}

PUB s32 loader_read(Loader *ld, FileHandle *fh, u8 **data_out, s32 *size_out)
{
        if(!ld || !fh) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        return file_read(fh, data_out, size_out);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC s32 l_check_path(const char *path)
{
        const char *seg = path;
        size_t      len = strlen(path);

        if(len == 0 || len >= LOADER_PATH_LENGTH) {
                return -1;
        }

        if(path[0] == '/') {
                return -1;
        }

        /* Check every segment between the '/' for ".." */
        while(*seg) {
                const char *end = strchr(seg, '/');
                size_t      seg_len = end ? (size_t)(end - seg) : strlen(seg);

                if(seg_len == 2 && seg[0] == '.' && seg[1] == '.') {
                        return -1;
                }

                if(!end) {
                        break;
                }
                seg = end + 1;
        }

        return 0;
}
