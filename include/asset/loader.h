#ifndef VASALL_LOADER_H
#define VASALL_LOADER_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <asset/file_handle.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define LOADER_PATH_LENGTH      128     /* Max. length of a relative path */
#define LOADER_BASE_DIR_LENGTH  256     /* Max. length of the base dir    */

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * Reads raw files from one folder. Paths are relative to the base dir and
 * can't leave it. The loader doesn't know what the files contain, turning
 * the bytes into images, fonts or text is the parser's job.
 */
typedef struct Loader {
        char base_dir[LOADER_BASE_DIR_LENGTH];
} Loader;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Set up a loader for a folder. The folder isn't checked until a file is
 * read.
 *
 * @returns 0 on success, -1 if the base dir is too long
 */
PUB s32 loader_init(Loader *ld, const char *base_dir);

/*
 * Set up a file handle for a path inside the base dir. The path uses '/',
 * must not start with '/' and must not contain "..".
 *
 * @param ld Pointer to the loader
 * @param path The relative path, e.g. "image/arrow.png"
 * @param fh Receives the handle
 *
 * @returns 0 on success, -1 if the path is not allowed or too long
 */
PUB s32 loader_open(Loader *ld, const char *path, FileHandle *fh);

/*
 * Read a file that was opened with loader_open, see file_read. Also used
 * to read it again after it changed.
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 loader_read(Loader *ld, FileHandle *fh, u8 **data_out, s32 *size_out);

#endif /* VASALL_LOADER_H */
