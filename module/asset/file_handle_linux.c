/*
 * Files on Linux, with the POSIX functions open, fstat, read, write and
 * rename.
 */
#if defined(__linux__)

/* Needed for O_CLOEXEC, st_mtim and fsync with -std=c99 */
#define _POSIX_C_SOURCE 200809L

#include <asset/file_handle.h>

#include <devkit/log/log.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/* Modification time of a stat result in ns */
LOC s64 f_mtime(struct stat *st);

/* Remember the state of a stat result */
LOC void f_remember(FileHandle *fh, struct stat *st);

/* Write all bytes, retrying short writes */
LOC s32 f_write_all(s32 fd, const u8 *data, s32 size);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 file_init(FileHandle *fh, const char *path)
{
        size_t len;

        if(!fh || !path) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if((len = strlen(path)) == 0 || len >= sizeof(fh->path)) {
                LOG_S(LOG_ERROR, "Invalid file path: %s", (char *)path);
                return -1;
        }

        memcpy(fh->path, path, len + 1);
        fh->known = 0;
        fh->mtime = 0;
        fh->size = 0;

        return 0;
}

PUB s32 file_exists(FileHandle *fh)
{
        struct stat st;

        if(!fh) {
                return 0;
        }

        return stat(fh->path, &st) == 0 && S_ISREG(st.st_mode);
}

PUB s32 file_read(FileHandle *fh, u8 **data_out, s32 *size_out)
{
        struct stat st;
        u8 *data;
        s32 size;
        s32 done = 0;
        s32 fd;

        if(!fh || !data_out || !size_out) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if((fd = open(fh->path, O_RDONLY | O_CLOEXEC)) < 0) {
                LOG_SS(LOG_ERROR, "Failed to open %s: %s", fh->path,
                                strerror(errno));
                return -1;
        }

        if(fstat(fd, &st) < 0) {
                LOG_SS(LOG_ERROR, "Failed to read size of %s: %s",
                                fh->path, strerror(errno));
                goto err_close;
        }

        /* Directories, devices or pipes have no meaningful size */
        if(!S_ISREG(st.st_mode)) {
                LOG_S(LOG_ERROR, "Not a regular file: %s", fh->path);
                goto err_close;
        }

        if(st.st_size > INT32_MAX - 1) {
                LOG_S(LOG_ERROR, "File is too large: %s", fh->path);
                goto err_close;
        }
        size = (s32)st.st_size;

        /* One extra byte, so an empty file still gets a valid buffer */
        if(!(data = malloc(size + 1))) {
                LOG_S(LOG_ERROR, "Failed to allocate memory for %s",
                                fh->path);
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
                                        fh->path, strerror(errno));
                        goto err_free;
                }

                /* The file got shorter while reading */
                if(got == 0) {
                        break;
                }

                done += (s32)got;
        }

        close(fd);

        /*
         * The state from before reading: if the file changes while it is
         * read, the next file_changed reports it and it is read again
         */
        f_remember(fh, &st);

        data[done] = 0;
        *data_out = data;
        *size_out = done;
        return 0;

err_free:
        free(data);

err_close:
        close(fd);
        return -1;
}

PUB s32 file_write(FileHandle *fh, const u8 *data, s32 size)
{
        char tmp[FILE_PATH_LENGTH + 8];
        struct stat st;
        s32 fd;
        s32 n;

        if(!fh || (!data && size > 0) || size < 0) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        n = snprintf(tmp, sizeof(tmp), "%s.tmp", fh->path);
        if(n < 0 || n >= (s32)sizeof(tmp)) {
                LOG_S(LOG_ERROR, "File path is too long: %s", fh->path);
                return -1;
        }

        if((fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
                      0644)) < 0) {
                LOG_SS(LOG_ERROR, "Failed to create %s: %s", tmp,
                                strerror(errno));
                return -1;
        }

        /* fsync makes sure the data is on disk before the rename */
        if(f_write_all(fd, data, size) < 0 || fsync(fd) < 0) {
                LOG_SS(LOG_ERROR, "Failed to write %s: %s", tmp,
                                strerror(errno));
                close(fd);
                unlink(tmp);
                return -1;
        }

        if(close(fd) < 0) {
                LOG_SS(LOG_ERROR, "Failed to write %s: %s", tmp,
                                strerror(errno));
                unlink(tmp);
                return -1;
        }

        /* Replaces the old file in one step */
        if(rename(tmp, fh->path) < 0) {
                LOG_SS(LOG_ERROR, "Failed to replace %s: %s", fh->path,
                                strerror(errno));
                unlink(tmp);
                return -1;
        }

        if(stat(fh->path, &st) == 0) {
                f_remember(fh, &st);
        }

        return 0;
}

PUB s32 file_changed(FileHandle *fh)
{
        struct stat st;

        if(!fh || !fh->known) {
                return -1;
        }

        if(stat(fh->path, &st) < 0 || !S_ISREG(st.st_mode)) {
                return -1;
        }

        return f_mtime(&st) != fh->mtime || (s64)st.st_size != fh->size;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC s64 f_mtime(struct stat *st)
{
        return (s64)st->st_mtim.tv_sec * 1000000000 + st->st_mtim.tv_nsec;
}

LOC void f_remember(FileHandle *fh, struct stat *st)
{
        fh->known = 1;
        fh->mtime = f_mtime(st);
        fh->size = (s64)st->st_size;
}

LOC s32 f_write_all(s32 fd, const u8 *data, s32 size)
{
        s32 done = 0;

        while(done < size) {
                ssize_t put = write(fd, data + done, size - done);

                if(put < 0) {
                        if(errno == EINTR) {
                                continue;
                        }
                        return -1;
                }

                done += (s32)put;
        }

        return 0;
}

#endif /* __linux__ */
