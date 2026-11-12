#ifndef VASALL_STUDIO_PATH_H
#define VASALL_STUDIO_PATH_H

#include <devkit/scope.h>
#include <devkit/types.h>

#define PATH_LENGTH_MAX 512

typedef struct Path {
        char cstr[PATH_LENGTH_MAX];
} Path;

PUB Path mkpath(const char *pth);

/* Get the '\0'-terminated c-string for the path */
PUB char *path_cstr(Path *p);

/* Append part to the end of path */
PUB Path path_append(Path p1, const char *p);

/* ret_path = p1 + p2 */
PUB Path path_join(Path p1, Path p2);

/*
 * Normalize a path without touching the file system:
 *
 *   "home/test/.///folder"  ->  "home/test/folder"   "." and empty parts go
 *   "a/b/../c"              ->  "a/c"                ".." removes a part
 *   "/../x"                 ->  "/x"                 nothing is above "/"
 *   "../a/../.."            ->  "../.."              relative paths keep
 *                                                    leading ".."
 *   "a/.." or ""            ->  "."
 *
 * A trailing '/' is removed (except for the root "/"). Symlinks are not
 * followed, so "link/.." becomes "." even if link points elsewhere.
 */
PUB Path path_resolve(Path p);


#endif /* VASALL_STDUIO_PATH_H */
