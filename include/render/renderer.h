#ifndef VASALL_RENDERER_H
#define VASALL_RENDERER_H

typedef struct Renderer Renderer;

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/list.h>
#include <devkit/memory/stack.h>

#include <devkit/math/vector.h>
#include <devkit/math/matrix.h>

#include <shape/rectangle.h>

#include <graphic/general/color.h>
#include <graphic/image/image.h>
#include <graphic/mesh/mesh.h>

#include <render/shader.h>
#include <render/texture.h>
#include <render/model.h>

#include <SDL3/SDL.h>

/* Only used through pointers; renderer.h is included by window.h */
struct Window;

typedef enum RenderObjectType {
        RENDER_OBJECT_NONE,
        RENDER_OBJECT_SHADER,
        RENDER_OBJECT_TEXTURE,
        RENDER_OBJECT_MODEL
} RenderObjectType;

typedef struct RenderObject {
        RenderObjectType type;
        u32 generation;

        union {
                Shader shader;
                Texture texture;
                Model model;
        } data;
} RenderObject;

/* How new pixels are combined with the ones already drawn */
typedef enum RenderBlend {
        RENDER_BLEND_NONE,              /* Overwrite */
        RENDER_BLEND_PREMULTIPLIED      /* "Over" with premultiplied alpha */
} RenderBlend;

typedef struct RenderPool {
        List objects;   /* List of Render Objects */
        Stack free_slots;     /* free slots, for any type*/
} RenderPool;

/* Separate ID types, so the compiler still catches mix-ups */
typedef struct ShaderId { s32 index; u32 generation; } ShaderId;
typedef struct TextureId { s32 index; u32 generation; } TextureId;
typedef struct ModelId { s32 index; u32 generation; } ModelId;

struct Renderer {
        SDL_Window *hidden;
        SDL_GLContext gl_context;

        RenderPool pool;

        /* The shader set with render_use_shader, uniforms are sent to it */
        ShaderId shader;

        /* Bound for draws that make their vertices in the shader */
        GLuint empty_vao;
};

/*
 *
*/
PUB s32 render_init(Renderer *ren);

/*
 *
*/
PUB void render_deinit(Renderer *ren);


/*
 * Start a frame in a window: direct all drawing into it, reset the area to
 * the whole window and clear it with the window's color.
 */
PUB void render_begin(Renderer *ren, struct Window *win);

/*
 * Show the finished frame.
 */
PUB void render_end(Renderer *ren, struct Window *win);

/*
 * Limit drawing and clearing to a part of the window. The area uses window
 * pixels with (0, 0) at the top left, like the UI. Drawing is stretched to
 * fill the area, as if it was its own window.
 *
 * @param ren Pointer to the renderer
 * @param area The area in window pixels
 * @param window_h The height of the window in pixels
 */
PUB void render_set_area(Renderer *ren, Rectangle area, s32 window_h);

/*
 * Clear the color and the depth inside the current area.
 */
PUB void render_clear(Renderer *ren, FColor col);

/*
 * Turn the depth test on (3D) or off (2D). Off by default.
 */
PUB void render_set_depth_test(Renderer *ren, s32 on);

/*
 * Set how new pixels are combined with the ones already drawn.
 */
PUB void render_set_blend(Renderer *ren, RenderBlend mode);

/*
 * Draw one triangle that covers the whole area, without a model. The vertex
 * shader makes the corners from gl_VertexID (0, 1, 2).
 */
PUB void render_draw_fullscreen(Renderer *ren);

/*
 * Use a shader for the following draw calls. The render_set_* functions
 * send their uniforms to this shader. An invalid ID unbinds the shader.
 */
PUB void render_use_shader(Renderer *ren, ShaderId sh);

/*
 * Set a uniform of the current shader. Unknown names are logged once and
 * otherwise ignored.
 */
PUB void render_set_mat4(Renderer *ren, const char *name, FMatrix4 *m);
PUB void render_set_vec3(Renderer *ren, const char *name, FVector3 v);
PUB void render_set_s32(Renderer *ren, const char *name, s32 v);

/*
 * Bind a texture to a slot (0, 1, ...). A sampler uniform reads the slot
 * that is set with render_set_s32, slot 0 if it was never set. An invalid ID
 * unbinds the slot.
 */
PUB void render_set_texture(Renderer *ren, s32 slot, TextureId tex);

/*
 * Draw a model with the current shader. Invalid IDs are ignored.
 */
PUB void render_draw_model(Renderer *ren, ModelId m);


/*
 * Compile and link a shader from its GLSL sources.
 *
 * @param ren Pointer to the renderer
 * @param vert The source of the vertex shader
 * @param frag The source of the fragment shader
 *
 * @returns The ID of the shader, with index -1 if an error occurred
 */
PUB ShaderId render_create_shader(Renderer *ren, const char *vert,
                                  const char *frag);

/*
 * Delete a shader. Old IDs of it stop working. Does nothing if the ID is
 * already invalid.
 */
PUB void render_destroy_shader(Renderer *ren, ShaderId id);

/*
 * Create an RGBA texture, see texture_init. px may be NULL.
 *
 * @returns The ID of the texture, with index -1 if an error occurred
 */
PUB TextureId render_create_texture(Renderer *ren, s32 w, s32 h, IColor *px);

/*
 * Copy the pixels inside rect from the image into the texture, see
 * texture_update.
 *
 * @returns 0 on success, -1 if the ID is invalid or an error occurred
 */
PUB s32 render_update_texture(Renderer *ren, TextureId id, Image *img,
                              Rectangle rect);

/*
 * Delete a texture. Old IDs of it stop working.
 */
PUB void render_destroy_texture(Renderer *ren, TextureId id);

/*
 * Upload a mesh as a model, see model_init. The mesh can be freed afterwards.
 *
 * @returns The ID of the model, with index -1 if an error occurred
 */
PUB ModelId render_create_model(Renderer *ren, Mesh *mesh);

/*
 * Replace the model's data with a new mesh, see model_update.
 *
 * @returns 0 on success, -1 if the ID is invalid or an error occurred
 */
PUB s32 render_update_model(Renderer *ren, ModelId id, Mesh *mesh);

/*
 * Delete a model. Old IDs of it stop working.
 */
PUB void render_destroy_model(Renderer *ren, ModelId id);

#endif /* VASALL_RENDERER_H */
