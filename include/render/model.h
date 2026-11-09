#ifndef VASALL_MODEL_H
#define VASALL_MODEL_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <graphic/mesh/mesh.h>

#include <glad/gl.h>

/*
 * The vertex attributes as the shaders see them:
 *
 *   layout(location = 0) in vec3 a_position;
 *   layout(location = 1) in vec3 a_normal;
 *   layout(location = 2) in vec2 a_uv;
 *   layout(location = 3) in vec4 a_color;    (0..1, from IColor)
 */
#define MODEL_ATTR_POSITION     0
#define MODEL_ATTR_NORMAL       1
#define MODEL_ATTR_UV           2
#define MODEL_ATTR_COLOR        3

/*
 * A mesh uploaded to the GPU, drawn as triangles.
 */
typedef struct Model {
        GLuint vao;             /* vertex layout */
        GLuint vbo;             /* vertex data */
        GLuint ibo;             /* indices */
        s32 index_count;
} Model;

/*
 * Upload a mesh. The mesh can be freed afterwards. Needs a current GL
 * context.
 *
 * @returns 0 on success, -1 if an error occurred or a face points to a
 *          vertex that doesn't exist
 */
PUB s32 model_init(Model *mdl, Mesh *mesh);

/*
 * Delete the buffers.
 */
PUB void model_deinit(Model *mdl);

/*
 * Replace the model's data with a new mesh. The vertex and face count may
 * change. On failure the old data stays.
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 model_update(Model *mdl, Mesh *mesh);

/*
 * Draw the model with the shader that is currently in use.
 */
PUB void model_draw(Model *mdl);

#endif /* VASALL_MODEL_H */
