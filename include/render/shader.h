#ifndef VASALL_SHADER_H
#define VASALL_SHADER_H

#include <devkit/scope.h>
#include <devkit/types.h>

typedef struct Shader {
        GLuint program;
        UniformCache uniforms;  /* name → location, filled on first use */
} Shader;

#endif /* VASALL_SHADER_H */