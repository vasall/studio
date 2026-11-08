/*
 * Reading files on Linux, with the POSIX functions open, fstat and read.
 */
#if defined(__linux__)

/* Needed for O_CLOEXEC and fstat with -std=c99 */
#define _POSIX_C_SOURCE 200809L

#include "loader.h"

#include <devkit/log/log.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/******************************************************************************
 *
 * 				PRIVATE FUNCTIONS
 *
 ******************************************************************************/

PRV s32 load_file(const char *path, u8 **data_out, s32 *size_out)
{
        struct stat st;
        u8 *data;
        s32 size;
        s32 done = 0;
        s32 fd;

        if(!path || !data_out || !size_out) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if((fd = open(path, O_RDONLY | O_CLOEXEC)) < 0) {
                LOG_SS(LOG_ERROR, "Failed to open %s: %s", (char *)path,
                                strerror(errno));
                return -1;
        }

        if(fstat(fd, &st) < 0) {
                LOG_SS(LOG_ERROR, "Failed to read size of %s: %s",
                                (char *)path, strerror(errno));
                goto err_close;
        }

        /* Directories, devices or pipes have no meaningful size */
        if(!S_ISREG(st.st_mode)) {
                LOG_S(LOG_ERROR, "Not a regular file: %s", (char *)path);
                goto err_close;
        }

        if(st.st_size > INT32_MAX - 1) {
                LOG_S(LOG_ERROR, "File is too large: %s", (char *)path);
                goto err_close;
        }
        size = (s32)st.st_size;

        /* One extra byte, so an empty file still gets a valid buffer */
        if(!(data = malloc(size + 1))) {
                LOG_S(LOG_ERROR, "Failed to allocate memory for %s",
                                (char *)path);
                goto err_close;
        }

        /* read can return fewer bytes than asked for, so loop */
        while(done < size) {
                ssize_t got = read(fd, data + done, size - done);

                if(got < 0) {
                        if(errno == EINTR) {
                                continue;
                        }
                        LOG_SS(LOG_ERROR, "Failed to read %s: %s",
                                        (char *)path, strerror(errno));
                        goto err_free;
                }

                /* The file got shorter while reading */
                if(got == 0) {
                        break;
                }

                done += (s32)got;
        }

        close(fd);

        *data_out = data;
        *size_out = done;
        return 0;

err_free:
        free(data);

err_close:
        close(fd);
        return -1;
}

#endif /* __linux__ */
