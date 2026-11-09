#ifndef VASALL_SHADER_H
#define VASALL_SHADER_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <render/uniform_cache.h>

#include <glad/gl.h>

typedef struct Shader {
        GLuint program;
        UniformCache uniforms; /* name → location, filled on first use */
} Shader;

/*
 * Compile and link a shader program. Errors from the GLSL compiler and the
 * linker are written to the log. Needs a current GL context.
 *
 * @param sh Pointer to the shader
 * @param vert The source of the vertex shader
 * @param frag The source of the fragment shader
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 shader_init(Shader *sh, const char *vert, const char *frag);

/*
 * Delete the program and free the uniform cache.
 */
PUB void shader_deinit(Shader *sh);

/*
 * Get the location of a uniform, see uniform_cache_get.
 */
PUB GLint shader_uniform(Shader *sh, const char *name);

#endif /* VASALL_SHADER_H */
