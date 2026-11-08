#ifndef VASALL_MODEL_H
#define VASALL_MODEL_H

#include <devkit/scope.h>
#include <devkit/types.h>

typedef struct Model {
        GLuint vao;             /* vertex layout */
        GLuint vbo;             /* vertex data */
        GLuint ibo;             /* indices */
        s32 index_count;
} Model;

#endif /* VASALL_MODEL_H */