#ifndef VASALL_UNIFORM_CACHE_H
#define VASALL_UNIFORM_CACHE_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/list.h>

#include <glad/gl.h>

#define UNIFORM_NAME_LENGTH     64      /* Max. length of a uniform name */

typedef struct UniformEntry {
        char name[UNIFORM_NAME_LENGTH];
        GLint location;         /* -1 if the program has no such uniform */
} UniformEntry;

/*
 * Remembers the locations of a program's uniforms, so glGetUniformLocation
 * is only called the first time a name is used.
 */
typedef struct UniformCache {
        GLuint program;
        List entries;           /* List of UniformEntry */
} UniformCache;

/*
 * Create an empty cache for a linked program.
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 uniform_cache_init(UniformCache *cache, GLuint program);

/*
 * Free the cache. The program is not deleted.
 */
PUB void uniform_cache_deinit(UniformCache *cache);

/*
 * Get the location of a uniform, asking GL only on the first call for a name.
 * Unknown names are remembered as -1 and logged once. GL ignores uploads to
 * location -1, so the result can always be passed to glUniform*.
 *
 * @param cache Pointer to the cache
 * @param name The name of the uniform in the shader, e.g. "u_mvp"
 *
 * @returns The location, or -1 if the program has no such uniform
 */
PUB GLint uniform_cache_get(UniformCache *cache, const char *name);

#endif /* VASALL_UNIFORM_CACHE_H */
