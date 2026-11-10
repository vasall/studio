#ifndef VASALL_FILE_HANDLE_H
#define VASALL_FILE_HANDLE_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define FILE_PATH_LENGTH        512     /* Max. length of a full path */

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * A file on disk, remembered by its path and its state when it was last read
 * or written. The file is not kept open, so the handle keeps working when an
 * editor saves by replacing the file.
 */
typedef struct FileHandle {
        char path[FILE_PATH_LENGTH];

        s32 known;      /* 1 once the file was read or written */
        s64 mtime;      /* Last modification then, ns since 1970 */
        s64 size;       /* Size in bytes then */
} FileHandle;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Set up a handle for a path. The file doesn't have to exist yet.
 *
 * @returns 0 on success, -1 if the path is empty or too long
 */
PUB s32 file_init(FileHandle *fh, const char *path);

/*
 * @returns 1 if the path is a regular file, 0 if not
 */
PUB s32 file_exists(FileHandle *fh);

/*
 * Read the whole file and remember its state for file_changed.
 *
 * @param fh Pointer to the handle
 * @param data_out Receives the contents, free it with free(). One byte more
 *                 than size is allocated, so it can be 0-terminated.
 * @param size_out Receives the number of bytes
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 file_read(FileHandle *fh, u8 **data_out, s32 *size_out);

/*
 * Replace the file's contents. The data is written to a temporary file next
 * to it first and then renamed, so the file is never left half written.
 * The new state is remembered, so this write doesn't count as a change.
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 file_write(FileHandle *fh, const u8 *data, s32 size);

/*
 * Check if the file changed on disk since it was last read or written
 * through this handle, by comparing modification time and size.
 *
 * @returns 1 if it changed, 0 if not, -1 if the file can't be checked (e.g.
 *          it was deleted, or it was never read or written)
 */
PUB s32 file_changed(FileHandle *fh);

#endif /* VASALL_FILE_HANDLE_H */
