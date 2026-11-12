#ifndef VASALL_STUDIO_FILE_H
#define VASALL_STUDIO_FILE_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Read a whole file into a '\0'-terminated buffer. Free it with free().
 *
 * @param size_out Receives the size in bytes (may be NULL)
 *
 * @returns The data, or NULL on error
 */
PUB char *file_read(const char *path, s64 *size_out);

#endif /* VASALL_STUDIO_FILE_H */
