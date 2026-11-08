#ifndef VASALL_RENDERER_H
#define VASALL_RENDERER_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/list.h>
#include <devkit/memory/stack.h>

#include <devkit/math/matrix.h>

#include <shape/rectangle.h>

#include <graphic/color.h>
#include <graphic/image.h>
#include <graphic/mesh.h>

#include <render/shader.h>
#include <render/texture.h>
#include <render/model.h>

#include <window/window.h>


typedef enum RenderObjectType {
        RENDER_SHADER,
        RENDER_TEXTURE,
        RENDER_MODEL
} RenderAssetType;

typedef struct RenderObject {
        RenderAssetType type;
        u32 generation;

        union {
                Shader shader;
                Texture texture;
                Model model;
        } data;
} RenderAsset;

typedef struct RenderPool {
        List objects;   /* List of Render Objects */
        Stack open;     /* free slots, for any type*/
} RenderPool;

/* Separate ID types, so the compiler still catches mix-ups */
typedef struct ShaderId { s32 index; u32 generation; } ShaderId;
typedef struct TextureId { s32 index; u32 generation; } TextureId;
typedef struct ModelId { s32 index; u32 generation; } ModelId;

typedef struct Renderer {
        SDL_Window *hidden;
        SDL_GLContext gl_context;

        RenderPool pool;
} Renderer;

/*
 *
*/
PUB s32 render_init(Renderer *ren);

/*
 *
*/
PUB void render_deinit(Renderer *ren);


PUB void render_begin(Renderer *ren, Window *win);
PUB void render_end(Renderer *ren, Window *win);

PUB void render_use_shader(Renderer *ren, ShaderId sh);
PUB void render_set_mat4(Renderer *ren, const char *name, Matrix4 *m);
PUB void render_set_vec3(Renderer *ren, const char *name, Vector3 v);
PUB void render_set_texture(Renderer *ren, s32 slot, TextureId tex);
PUB void render_draw_model(Renderer *ren, ModelId m);

PUB void render_set_area(Renderer *ren, Rectangle area, s32 window_h);
PUB void render_clear(Renderer *ren, FColor col);

/*
 *
*/
PUB ShaderId render_create_shader(Renderer *ren, const char *vert, 
        const char *frag);

PUB void render_destroy_shader(Renderer *ren, ShaderId id);

/*
 *
*/
PUB TextureId render_create_texture(Renderer *ren, s32 w, s32 h, IColor *px);

PUB s32 render_update_texture(Renderer *ren, TextureId id, Image *img, 
        Rectangle rect);

PUB void render_destroy_texture(Renderer *ren, TextureId id);

/*
 *
*/
PUB ModelId render_create_model(Renderer *ren, Mesh *mesh);

/*
 *
*/
PUB s32 render_update_model(Renderer *ren, ModelId id, Mesh *mesh);

/*
 *
*/
PUB void render_destroy_model(Renderer *ren, ModelId id);

#endif /* VASALL_RENDERER_H */