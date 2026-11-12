#ifndef VASALL_STUDIO_GL_UTIL_H
#define VASALL_STUDIO_GL_UTIL_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <glad/gl.h>

/*
 * Compile and link a program from two GLSL files. Errors are logged.
 *
 * @returns The program, or 0 on error
 */
PUB GLuint gl_program_from_files(const char *vert_path, const char *frag_path);

#endif /* VASALL_STUDIO_GL_UTIL_H */
