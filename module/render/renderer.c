
#include <render/renderer.h>
#include <window/window.h>

#include <devkit/log/log.h>

#include <glad/gl.h>
#include <SDL3/SDL.h>


/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * The pool holds every GL object of the renderer. A slot is addressed by its
 * index and generation: freeing a slot bumps its generation, so old IDs that
 * still point to the slot stop working instead of reaching the new object.
 * Generation 0 is never used, so a zeroed ID is always invalid.
 */

LOC s32 r_pool_init(RenderPool *pool);

/*
 * Free the pool's memory. This does not delete the GL objects; they are
 * freed together with the GL context.
 */
LOC void r_pool_deinit(RenderPool *pool);

/*
 * Store an object in a free slot, reusing freed slots first.
 *
 * @param pool Pointer to the pool
 * @param obj The object, with its type set (the generation is ignored)
 * @param gen_out Receives the generation of the slot
 *
 * @returns The index of the slot, or -1 if an error occurred
 */
LOC s32 r_pool_push(RenderPool *pool, RenderObject obj, u32 *gen_out);

/*
 * Free a slot, so the index can be reused. Delete the GL object first.
 *
 * @returns 0 on success, -1 if the slot was already freed
 */
LOC s32 r_pool_remove(RenderPool *pool, s32 index, u32 gen);

/*
 * Get the object in a slot. Returns NULL if the slot was freed since, or
 * holds an object of another type. The pointer is only valid until the next
 * r_pool_push, since the list may be reallocated.
 */
/*
 * Get the shader of an ID, or NULL if the ID is invalid.
 */
LOC Shader *r_get_shader(Renderer *ren, ShaderId id);
LOC Texture *r_get_texture(Renderer *ren, TextureId id);
LOC Model *r_get_model(Renderer *ren, ModelId id);

LOC RenderObject *r_pool_get(RenderPool *pool, s32 index, u32 gen,
                             RenderObjectType type);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/


PUB s32 render_init(Renderer *ren)
{
        SDL_WindowFlags flags;

        /* Request an OpenGL 3.3 core context (must be set before creation) */
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


        /* A hidden 1x1 window, so the context doesn't depend on any of the
         * real windows and survives when they are closed */
        flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
        if(!(ren->hidden = SDL_CreateWindow("", 1, 1, flags))) {
                LOG(LOG_ERROR, "Failed to create hidden window");
                return -1;
        }

        if(!(ren->gl_context = SDL_GL_CreateContext(ren->hidden))) {
                LOG(LOG_ERROR, "Failed to create OpenGL context");
                goto err_destroy_hidden;
        }

        /* Load the OpenGL functions through SDL */
        if(!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
                LOG(LOG_ERROR, "Failed to load OpenGL functions");
                goto err_destroy_gl_context;
        }

        if(r_pool_init(&ren->pool) < 0) {
                LOG(LOG_ERROR, "Failed initialize render pool");
                goto err_destroy_gl_context;
        }

        ren->shader.index = -1;
        ren->shader.generation = 0;

        /* Core profile GL refuses to draw without a bound VAO */
        glGenVertexArrays(1, &ren->empty_vao);

        return 0;

err_destroy_gl_context:
        SDL_GL_DestroyContext(ren->gl_context);

err_destroy_hidden:
        SDL_DestroyWindow(ren->hidden);

        return -1;
}

PUB void render_deinit(Renderer *ren)
{
        s32 i;

        if(!ren) {
                return;
        }

        /* The GL objects die with the context, only free the CPU memory */
        for(i = 0; i < list_count(&ren->pool.objects); i++) {
                RenderObject *obj = list_get(&ren->pool.objects, i);
                if(obj->type == RENDER_OBJECT_SHADER) {
                        uniform_cache_deinit(&obj->data.shader.uniforms);
                }
        }

        r_pool_deinit(&ren->pool);

        /* Destroying the context also deletes all GL objects in it */
        SDL_GL_DestroyContext(ren->gl_context);
        SDL_DestroyWindow(ren->hidden);
}

PUB void render_begin(Renderer *ren, Window *win)
{
        /* Direct all GL calls into this window */
        SDL_GL_MakeCurrent(win->sdl_window, ren->gl_context);

        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, win->width, win->height);

        render_set_depth_test(ren, 0);
        render_set_blend(ren, RENDER_BLEND_NONE);
        render_clear(ren, win->color);
}

PUB void render_end(Renderer *ren, Window *win)
{
        (void)ren;

        SDL_GL_SwapWindow(win->sdl_window);
}

PUB void render_set_area(Renderer *ren, Rectangle area, s32 window_h)
{
        /* GL counts y from the bottom of the window */
        s32 y = window_h - area.y - area.h;

        (void)ren;

        glViewport(area.x, y, area.w, area.h);

        /* The viewport alone doesn't limit glClear, the scissor does */
        glScissor(area.x, y, area.w, area.h);
        glEnable(GL_SCISSOR_TEST);
}

PUB void render_clear(Renderer *ren, FColor col)
{
        (void)ren;

        /* glClear respects the depth mask, so make sure depth is written */
        glDepthMask(GL_TRUE);
        glClearColor(col.r, col.g, col.b, col.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

PUB void render_set_depth_test(Renderer *ren, s32 on)
{
        (void)ren;

        if(on) {
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glDepthMask(GL_TRUE);
        }
        else {
                glDisable(GL_DEPTH_TEST);
        }
}

PUB void render_set_blend(Renderer *ren, RenderBlend mode)
{
        (void)ren;

        switch(mode) {
        case RENDER_BLEND_PREMULTIPLIED:
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                break;

        default:
                glDisable(GL_BLEND);
                break;
        }
}

PUB void render_draw_fullscreen(Renderer *ren)
{
        glBindVertexArray(ren->empty_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
}

PUB void render_use_shader(Renderer *ren, ShaderId sh)
{
        Shader *shader;

        if(!(shader = r_get_shader(ren, sh))) {
                glUseProgram(0);
                ren->shader.index = -1;
                ren->shader.generation = 0;
                return;
        }

        glUseProgram(shader->program);
        ren->shader = sh;
}

PUB void render_set_mat4(Renderer *ren, const char *name, FMatrix4 *m)
{
        Shader *shader;

        if(!m || !(shader = r_get_shader(ren, ren->shader))) {
                LOG(LOG_ERROR, "No shader in use or no matrix given");
                return;
        }

        /* FMatrix4 is column-major like GL, so no transpose is needed */
        glUniformMatrix4fv(shader_uniform(shader, name), 1, GL_FALSE,
                           m->data);
}

PUB void render_set_vec3(Renderer *ren, const char *name, FVector3 v)
{
        Shader *shader;

        if(!(shader = r_get_shader(ren, ren->shader))) {
                LOG(LOG_ERROR, "No shader in use");
                return;
        }

        glUniform3f(shader_uniform(shader, name), v.x, v.y, v.z);
}

PUB void render_set_s32(Renderer *ren, const char *name, s32 v)
{
        Shader *shader;

        if(!(shader = r_get_shader(ren, ren->shader))) {
                LOG(LOG_ERROR, "No shader in use");
                return;
        }

        glUniform1i(shader_uniform(shader, name), v);
}

PUB void render_set_texture(Renderer *ren, s32 slot, TextureId tex)
{
        Texture *texture = r_get_texture(ren, tex);

        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture ? texture->id : 0);
        glActiveTexture(GL_TEXTURE0);
}

PUB void render_draw_model(Renderer *ren, ModelId m)
{
        Model *model;

        if((model = r_get_model(ren, m))) {
                model_draw(model);
        }
}

PUB ShaderId render_create_shader(Renderer *ren, const char *vert,
                                  const char *frag)
{
        ShaderId id = { -1, 0 };
        RenderObject obj;

        if(!ren || !vert || !frag) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return id;
        }

        obj.type = RENDER_OBJECT_SHADER;
        if(shader_init(&obj.data.shader, vert, frag) < 0) {
                return id;
        }

        if((id.index = r_pool_push(&ren->pool, obj, &id.generation)) < 0) {
                shader_deinit(&obj.data.shader);
                id.generation = 0;
        }

        return id;
}

PUB void render_destroy_shader(Renderer *ren, ShaderId id)
{
        ShaderId none = { -1, 0 };
        Shader *shader;

        if(!(shader = r_get_shader(ren, id))) {
                return;
        }

        /* Don't leave a deleted program bound */
        if(ren->shader.index == id.index &&
           ren->shader.generation == id.generation) {
                render_use_shader(ren, none);
        }

        shader_deinit(shader);
        r_pool_remove(&ren->pool, id.index, id.generation);
}


PUB TextureId render_create_texture(Renderer *ren, s32 w, s32 h, IColor *px)
{
        TextureId id = { -1, 0 };
        RenderObject obj;

        if(!ren) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return id;
        }

        obj.type = RENDER_OBJECT_TEXTURE;
        if(texture_init(&obj.data.texture, w, h, px) < 0) {
                return id;
        }

        if((id.index = r_pool_push(&ren->pool, obj, &id.generation)) < 0) {
                texture_deinit(&obj.data.texture);
                id.generation = 0;
        }

        return id;
}

PUB s32 render_update_texture(Renderer *ren, TextureId id, Image *img,
                              Rectangle rect)
{
        Texture *texture;

        if(!(texture = r_get_texture(ren, id))) {
                LOG(LOG_ERROR, "Invalid texture");
                return -1;
        }

        return texture_update(texture, img, rect);
}

PUB void render_destroy_texture(Renderer *ren, TextureId id)
{
        Texture *texture;

        if(!(texture = r_get_texture(ren, id))) {
                return;
        }

        texture_deinit(texture);
        r_pool_remove(&ren->pool, id.index, id.generation);
}

PUB ModelId render_create_model(Renderer *ren, Mesh *mesh)
{
        ModelId id = { -1, 0 };
        RenderObject obj;

        if(!ren) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return id;
        }

        obj.type = RENDER_OBJECT_MODEL;
        if(model_init(&obj.data.model, mesh) < 0) {
                return id;
        }

        if((id.index = r_pool_push(&ren->pool, obj, &id.generation)) < 0) {
                model_deinit(&obj.data.model);
                id.generation = 0;
        }

        return id;
}

PUB s32 render_update_model(Renderer *ren, ModelId id, Mesh *mesh)
{
        Model *model;

        if(!(model = r_get_model(ren, id))) {
                LOG(LOG_ERROR, "Invalid model");
                return -1;
        }

        return model_update(model, mesh);
}

PUB void render_destroy_model(Renderer *ren, ModelId id)
{
        Model *model;

        if(!(model = r_get_model(ren, id))) {
                return;
        }

        model_deinit(model);
        r_pool_remove(&ren->pool, id.index, id.generation);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC s32 r_pool_init(RenderPool *pool)
{
        if(list_init(&pool->objects, sizeof(RenderObject), 32) < 0) {
                LOG(LOG_ERROR, "Failed to create the render object list");
                return -1;
        }

        if(stack_init(&pool->free_slots, sizeof(s32), 12) < 0) {
                LOG(LOG_ERROR, "Failed to create stack for free slots");
                list_deinit(&pool->objects);
                return -1;
        }

        return 0;
}

LOC void r_pool_deinit(RenderPool *pool)
{
        stack_deinit(&pool->free_slots);
        list_deinit(&pool->objects);
}

LOC s32 r_pool_push(RenderPool *pool, RenderObject obj, u32 *gen_out)
{
        RenderObject *slot;
        s32 index;

        /* Reuse a freed slot, it keeps the generation it got when freed */
        if(stack_count(&pool->free_slots) > 0) {
                stack_pop(&pool->free_slots, 1, &index);
                slot = list_get(&pool->objects, index);
                obj.generation = slot->generation;
                *slot = obj;
        }
        else {
                index = list_count(&pool->objects);
                obj.generation = 1;
                if(list_push(&pool->objects, 1, &obj) < 0) {
                        LOG(LOG_ERROR, "Failed to add render object");
                        return -1;
                }
        }

        *gen_out = obj.generation;
        return index;
}

LOC s32 r_pool_remove(RenderPool *pool, s32 index, u32 gen)
{
        RenderObject *obj;

        if(index < 0 || index >= list_count(&pool->objects)) {
                return -1;
        }

        obj = list_get(&pool->objects, index);
        if(obj->generation != gen || obj->type == RENDER_OBJECT_NONE) {
                return -1;
        }

        if(stack_push(&pool->free_slots, 1, &index) < 0) {
                LOG(LOG_ERROR, "Failed to free render object slot");
                return -1;
        }

        obj->type = RENDER_OBJECT_NONE;

        /* Skip 0 on overflow, so a zeroed ID never becomes valid */
        if(++obj->generation == 0) {
                obj->generation = 1;
        }

        return 0;
}

LOC RenderObject *r_pool_get(RenderPool *pool, s32 index, u32 gen,
                             RenderObjectType type)
{
        RenderObject *obj;

        if(index < 0 || index >= list_count(&pool->objects)) {
                return NULL;
        }

        obj = list_get(&pool->objects, index);
        if(obj->generation != gen || obj->type != type) {
                return NULL;
        }

        return obj;
}

LOC Shader *r_get_shader(Renderer *ren, ShaderId id)
{
        RenderObject *obj;

        if(!ren) {
                return NULL;
        }

        obj = r_pool_get(&ren->pool, id.index, id.generation,
                         RENDER_OBJECT_SHADER);

        return obj ? &obj->data.shader : NULL;
}

LOC Texture *r_get_texture(Renderer *ren, TextureId id)
{
        RenderObject *obj;

        if(!ren) {
                return NULL;
        }

        obj = r_pool_get(&ren->pool, id.index, id.generation,
                         RENDER_OBJECT_TEXTURE);

        return obj ? &obj->data.texture : NULL;
}

LOC Model *r_get_model(Renderer *ren, ModelId id)
{
        RenderObject *obj;

        if(!ren) {
                return NULL;
        }

        obj = r_pool_get(&ren->pool, id.index, id.generation,
                         RENDER_OBJECT_MODEL);

        return obj ? &obj->data.model : NULL;
}
