#ifndef VASALL_FILESYSTEM_H
#define VASALL_FILESYSTEM_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/list.h>
#include <devkit/memory/text.h>

#include <stdio.h>
#include <stdlib.h>

#define FS_BASE_PATH_MAX 512
#define FS_FILE_PATH_MAX 512

typedef struct FileSystem FileSystem;

typedef struct ByteBuffer {
        u8 *data;
        s32 size;
} ByteBuffer;

typedef enum FileFormat {
        FILE_BINARY,
        FILE_TEXT
} FileFormat;

typedef struct File {
        FileSystem *fs;

        FileFormat format;

        char path[FS_FILE_PATH_MAX]; /* Absolute path in the filesystem */
        FILE *fd;   /* The file handle */

        time_t last_change;

        union {
                Text text;
                ByteBuffer binary;
        } content;
} File;

struct FileSystem {
        char base_path[FS_BASE_PATH_MAX]; /* Absolute path to the base dir */
        List files;
};

/**
 * Initialize a new file system.
 *
 * @param fs Pointer to the file system to initialize
 *
 * @returns 0 on success or -1 if an error occurred
*/
PUB s32 fs_init(FileSystem *fs, const char *base_path);

/**
 * Shutdown a filesystem and destroy all loaded files.
 *
 * @param fs Pointer to the file system
 */
PUB void fs_deinit(FileSystem *fs);

/**
 * Check if a file exists at the given path.
 *
 * @param fs Pointer to the file system
 * @param path The absolute path to the file to check relative to base dir
 *
 * @returns 1 if the file exists, 0 if not and -1 if an error occurred
 */
PUB s32 fs_exists(FileSystem *fs, const char *path);

/**
 * Open a file on the disk and load its content into a file buffer. If the file
 * has already been opened, just return the the handle for the file.
 * If the file doesn't exist, this function will create it, but the file will
 * only actually be written to the disk when the fs_write() function is called.
 * If fs_close() is without writing the file, no file will be created on the
 * disk.
 *
 * @param fs Pointer to the file system
 * @param format The format to load the file as
 * @param path Absolute path to the file in the base dir
 *
 * @returns A pointer to the file or NULL if an error occurred
 */
PUB File *fs_open(FileSystem *fs, FileFormat format, const char *path);

/**
 * Close a file and close the file handle.
 *
 * @param file Pointer to the file to close
 */
PUB void fs_close(File *file);

/**
 * Get the size of a file in bytes in the content buffer.
 *
 * @param file Pointer to the file
 *
 * @returns Either the size of the file in bytes or -1 if an error occurred
 */
PUB s32 fs_size(File *file);

/**
 * Get the size of the file on the disk in bytes.
 *
 * @param file Pointer to the file
 *
 * @returns Either the size of the file on the disk or -1 if an error occurred
 */
PUB s32 fs_disksize(File *file);

/**
 * Check if the contents of the file on the disk have changed.
 *
 * @param file Pointer to the file
 *
 * @returns 1 if the file on the disk has changed, 0 if not and -1 if an error
 *          occurred
 */
PUB s32 fs_changed(File *file);

/**
 * Reload the contents of the file on the disk. The current file content will be
 * overwritten.
 *
 * @param file Pointer to the file to reload
 *
 * @returns 0 on success or -1 if an error occurred
 */
PUB s32 fs_reload(File *file);

/**
 * Write the contents of the file buffer to the file on the disk. The content of
 * the disk file will be overwritten.
 *
 * @param file Pointer to the file
 *
 * @returns 0 on success or -1 if an error occurred
 */
PUB s32 fs_write(File *file);

#endif /* VASALL_FILESYSTEM_H */
