#include <render/scene.h>

#include <devkit/log/log.h>

#include <render/gl_util.h>
#include <ui/widgets/viewport.h>

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MSAA_SAMPLES 4
#define FLOATS_PER_VERTEX 6     /* Position, normal (or color for lines) */

/* A mesh on the GPU */
typedef struct SceneMeshGPU {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;             /* 0 if not indexed */
        s32 count;              /* Indices, or vertices if not indexed */
        FVector3 center;
        f32 radius;
} SceneMeshGPU;

typedef struct SceneObjectData {
        SceneMesh mesh;
        FMatrix4 transform;
        UIColor color;
        s32 visible;
} SceneObjectData;

/* What one viewport draws, in the frame arena */
typedef struct SceneJob {
        Scene3D *scene;
        SceneView *view;
} SceneJob;

LOC const char *s_vert =
        "#version 330 core\n"
        "layout(location = 0) in vec3 a_pos;\n"
        "layout(location = 1) in vec3 a_normal;\n"
        "uniform mat4 u_model;\n"
        "uniform mat4 u_view;\n"
        "uniform mat4 u_proj;\n"
        "uniform mat3 u_normal; /* View space, inverse transpose */\n"
        "out vec3 v_normal;\n"
        "out vec3 v_attr;\n"
        "void main()\n"
        "{\n"
        "        v_normal = u_normal * a_normal;\n"
        "        v_attr = a_normal;\n"
        "        gl_Position = u_proj * u_view * u_model * vec4(a_pos, 1.0);\n"
        "}\n";

LOC const char *s_frag =
        "#version 330 core\n"
        "in vec3 v_normal;\n"
        "in vec3 v_attr;\n"
        "uniform vec4 u_color;\n"
        "uniform int u_unlit;\n"
        "out vec4 frag;\n"
        "void main()\n"
        "{\n"
        "        if(u_unlit != 0) {\n"
        "                frag = vec4(v_attr, 1.0); /* Line color */\n"
        "                return;\n"
        "        }\n"
        "        vec3 n = normalize(v_normal);\n"
        "        if(!gl_FrontFacing) n = -n;\n"
        "        /* A key light from the upper right of the camera and a\n"
        "           headlight, both in view space */\n"
        "        float key = max(dot(n, normalize(vec3(0.4, 0.6, 0.7))), 0.0);\n"
        "        float head = max(n.z, 0.0);\n"
        "        vec3 c = u_color.rgb * (0.16 + 0.6 * key + 0.3 * head);\n"
        "        frag = vec4(c, 1.0);\n"
        "}\n";

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_render(const UIDrawCallbackInfo *info, void *data);
LOC s32 s_view_targets(SceneView *view, s32 w, s32 h);
LOC void s_view_free(SceneView *view);
LOC GLuint s_vertex_array(GLuint vbo, GLuint ebo);
LOC f32 *s_build_vertices(const SceneMeshData *d, s32 *count_out);
LOC s32 s_build_grid(Scene3D *scene, f32 step, s32 lines);
LOC void s_push_line(f32 *v, s32 *n, f32 x0, f32 y0, f32 x1, f32 y1,
                const f32 rgb[3]);
LOC SceneObjectData *s_object(Scene3D *scene, SceneObject obj);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 scene_init(Scene3D *scene, f32 grid_step, s32 grid_lines)
{
        memset(scene, 0, sizeof(*scene));
        if(pool_init(&scene->meshes, sizeof(SceneMeshGPU), 32) < 0) return -1;
        if(pool_init(&scene->objects, sizeof(SceneObjectData), 64) < 0)
                goto err_objects;

        scene->program = gl_program_from_source(s_vert, s_frag, "scene");
        if(!scene->program) goto err_program;
        scene->u_model = glGetUniformLocation(scene->program, "u_model");
        scene->u_view = glGetUniformLocation(scene->program, "u_view");
        scene->u_proj = glGetUniformLocation(scene->program, "u_proj");
        scene->u_normal = glGetUniformLocation(scene->program, "u_normal");
        scene->u_color = glGetUniformLocation(scene->program, "u_color");
        scene->u_unlit = glGetUniformLocation(scene->program, "u_unlit");

        if(grid_lines > 0 && s_build_grid(scene, grid_step, grid_lines) < 0)
                goto err_grid;
        scene->show_grid = grid_lines > 0;
        scene->background = ui_hex(0x161616ff);
        return 0;

err_grid:
        glDeleteProgram(scene->program);
err_program:
        pool_deinit(&scene->objects);
err_objects:
        pool_deinit(&scene->meshes);
        return -1;
}

PUB void scene_deinit(Scene3D *scene)
{
        SceneMeshGPU *m;
        PoolHandle h;
        s32 i;

        for(i = 0; i < pool_slots(&scene->meshes); i++) {
                if((m = pool_at(&scene->meshes, i, &h)))
                        scene_mesh_remove(scene, h);
        }
        if(scene->grid_vao) glDeleteVertexArrays(1, &scene->grid_vao);
        if(scene->grid_vbo) glDeleteBuffers(1, &scene->grid_vbo);
        if(scene->program) glDeleteProgram(scene->program);
        pool_deinit(&scene->objects);
        pool_deinit(&scene->meshes);
}

PUB SceneMesh scene_mesh_add(Scene3D *scene, const SceneMeshData *data)
{
        SceneMesh none = { POOL_HANDLE_NONE };
        SceneMesh h;
        SceneMeshGPU gpu;
        FVector3 lo;
        FVector3 hi;
        FVector3 p;
        f32 *verts;
        f32 r2 = 0.0f;
        f32 d2;
        s32 count;
        s32 i;

        if(!data || !data->positions || data->vertex_count <= 0) return none;
        if(data->indices) {
                if(data->index_count <= 0 || data->index_count % 3) return none;
                for(i = 0; i < data->index_count; i++) {
                        if(data->indices[i] >= (u32)data->vertex_count) {
                                LOG(LOG_WARN, "scene: mesh index out of range");
                                return none;
                        }
                }
        } else if(data->vertex_count % 3) {
                return none;
        }

        if(!(verts = s_build_vertices(data, &count))) return none;

        memset(&gpu, 0, sizeof(gpu));
        glGenBuffers(1, &gpu.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)count * FLOATS_PER_VERTEX *
                        (GLsizeiptr)sizeof(f32), verts, GL_STATIC_DRAW);
        free(verts);

        /* Smooth meshes keep their indices; flat ones were expanded */
        gpu.count = count;
        if(data->indices && data->normals) {
                glGenBuffers(1, &gpu.ebo);
                gpu.count = data->index_count;
        }
        gpu.vao = s_vertex_array(gpu.vbo, gpu.ebo);
        if(gpu.ebo) {
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)
                                data->index_count * (GLsizeiptr)sizeof(u32),
                                data->indices, GL_STATIC_DRAW);
        }
        glBindVertexArray(0);

        /* Bounding sphere around the box of all vertices */
        lo = hi = fvec3(data->positions[0], data->positions[1],
                        data->positions[2]);
        for(i = 1; i < data->vertex_count; i++) {
                p = fvec3(data->positions[i * 3], data->positions[i * 3 + 1],
                                data->positions[i * 3 + 2]);
                lo = fvec3(fminf(lo.x, p.x), fminf(lo.y, p.y),
                                fminf(lo.z, p.z));
                hi = fvec3(fmaxf(hi.x, p.x), fmaxf(hi.y, p.y),
                                fmaxf(hi.z, p.z));
        }
        gpu.center = fvec3_scale(fvec3_add(lo, hi), 0.5f);
        for(i = 0; i < data->vertex_count; i++) {
                p = fvec3(data->positions[i * 3], data->positions[i * 3 + 1],
                                data->positions[i * 3 + 2]);
                d2 = fvec3_mag_sqr(fvec3_sub(p, gpu.center));
                if(d2 > r2) r2 = d2;
        }
        gpu.radius = sqrtf(r2);

        if(!pool_add(&scene->meshes, &gpu, &h)) {
                glDeleteVertexArrays(1, &gpu.vao);
                glDeleteBuffers(1, &gpu.vbo);
                if(gpu.ebo) glDeleteBuffers(1, &gpu.ebo);
                return none;
        }
        return h;
}

PUB void scene_mesh_remove(Scene3D *scene, SceneMesh mesh)
{
        SceneMeshGPU *m = pool_get(&scene->meshes, mesh);

        if(!m) return;
        glDeleteVertexArrays(1, &m->vao);
        glDeleteBuffers(1, &m->vbo);
        if(m->ebo) glDeleteBuffers(1, &m->ebo);
        pool_remove(&scene->meshes, mesh);
}

PUB s32 scene_mesh_bounds(Scene3D *scene, SceneMesh mesh, FVector3 *center,
                f32 *radius)
{
        SceneMeshGPU *m = pool_get(&scene->meshes, mesh);

        if(!m) return -1;
        if(center) *center = m->center;
        if(radius) *radius = m->radius;
        return 0;
}

PUB SceneObject scene_object_add(Scene3D *scene, SceneMesh mesh,
                FMatrix4 transform, UIColor color)
{
        SceneObject none = { POOL_HANDLE_NONE };
        SceneObjectData o;
        SceneObject h;

        o.mesh = mesh;
        o.transform = transform;
        o.color = color;
        o.visible = 1;
        if(!pool_add(&scene->objects, &o, &h)) return none;
        return h;
}

PUB void scene_object_remove(Scene3D *scene, SceneObject obj)
{
        pool_remove(&scene->objects, obj);
}

PUB void scene_object_set_transform(Scene3D *scene, SceneObject obj,
                FMatrix4 transform)
{
        SceneObjectData *o = s_object(scene, obj);

        if(o) o->transform = transform;
}

PUB void scene_object_set_color(Scene3D *scene, SceneObject obj,
                UIColor color)
{
        SceneObjectData *o = s_object(scene, obj);

        if(o) o->color = color;
}

PUB void scene_object_set_visible(Scene3D *scene, SceneObject obj,
                s32 visible)
{
        SceneObjectData *o = s_object(scene, obj);

        if(o) o->visible = visible;
}

PUB void scene_view_init(SceneView *view)
{
        memset(view, 0, sizeof(*view));
        camera_init(&view->camera);
}

PUB void scene_view_deinit(SceneView *view)
{
        s_view_free(view);
}

PUB UISignal scene_viewport(UI *ui, const char *id, Scene3D *scene,
                SceneView *view, const char *cls)
{
        SceneJob *job = arena_alloc(&ui->arena, sizeof(SceneJob));

        if(job) {
                job->scene = scene;
                job->view = view;
        }
        return ui_viewport(ui, id, &view->camera, job ? s_render : NULL, job,
                        cls);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

/*
 * Draw the scene into the view's multisampled framebuffer, resolve it and
 * copy it into the window at the viewport's place, clipped like the UI.
 */
LOC void s_render(const UIDrawCallbackInfo *info, void *data)
{
        SceneJob *job = data;
        Scene3D *scene = job->scene;
        SceneView *view = job->view;
        SceneMeshGPU *mesh;
        SceneObjectData *o;
        PoolHandle h;
        FMatrix4 proj;
        FMatrix4 cam;
        FMatrix4 nm;
        UIColor bg = scene->background;
        f32 n3[9];
        s32 w = info->w;
        s32 h_px = info->h;
        s32 i;
        s32 c;
        s32 r;

        if(w <= 0 || h_px <= 0 || s_view_targets(view, w, h_px) < 0) return;

        glBindFramebuffer(GL_FRAMEBUFFER, view->fbo_msaa);
        glViewport(0, 0, w, h_px);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glClearColor(bg.r / 255.0f, bg.g / 255.0f, bg.b / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        proj = camera_proj(&view->camera, (f32)w / (f32)h_px);
        cam = camera_view(&view->camera);

        glUseProgram(scene->program);
        glUniformMatrix4fv(scene->u_view, 1, GL_FALSE, cam.data);
        glUniformMatrix4fv(scene->u_proj, 1, GL_FALSE, proj.data);

        if(scene->show_grid && scene->grid_vertices > 0) {
                FMatrix4 id = fmat4_identity();

                glUniformMatrix4fv(scene->u_model, 1, GL_FALSE, id.data);
                glUniform1i(scene->u_unlit, 1);
                glBindVertexArray(scene->grid_vao);
                glDrawArrays(GL_LINES, 0, scene->grid_vertices);
        }

        glUniform1i(scene->u_unlit, 0);
        for(i = 0; i < pool_slots(&scene->objects); i++) {
                o = pool_at(&scene->objects, i, &h);
                if(!o || !o->visible) continue;
                mesh = pool_get(&scene->meshes, o->mesh);
                if(!mesh) continue;

                /* Normals to view space, right for any scaling */
                nm = fmat4_trans(fmat4_inv(fmat4_mul(cam, o->transform)));
                for(c = 0; c < 3; c++) {
                        for(r = 0; r < 3; r++)
                                n3[c * 3 + r] = nm.data[c * 4 + r];
                }

                glUniformMatrix4fv(scene->u_model, 1, GL_FALSE,
                                o->transform.data);
                glUniformMatrix3fv(scene->u_normal, 1, GL_FALSE, n3);
                glUniform4f(scene->u_color, o->color.r / 255.0f,
                                o->color.g / 255.0f, o->color.b / 255.0f,
                                o->color.a / 255.0f);
                glBindVertexArray(mesh->vao);
                if(mesh->ebo) {
                        glDrawElements(GL_TRIANGLES, mesh->count,
                                        GL_UNSIGNED_INT, NULL);
                } else {
                        glDrawArrays(GL_TRIANGLES, 0, mesh->count);
                }
        }
        glBindVertexArray(0);
        glDisable(GL_DEPTH_TEST);

        /* Resolve the samples, then copy into the window (GL's origin is
           the bottom left) */
        glBindFramebuffer(GL_READ_FRAMEBUFFER, view->fbo_msaa);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, view->fbo_resolve);
        glBlitFramebuffer(0, 0, w, h_px, 0, 0, w, h_px, GL_COLOR_BUFFER_BIT,
                        GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, view->fbo_resolve);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glEnable(GL_SCISSOR_TEST);
        glScissor(info->clip_x, info->fb_h - info->clip_y - info->clip_h,
                        info->clip_w, info->clip_h);
        glBlitFramebuffer(0, 0, w, h_px, info->x,
                        info->fb_h - info->y - h_px, info->x + w,
                        info->fb_h - info->y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* (Re)create the view's framebuffers for w x h pixels */
LOC s32 s_view_targets(SceneView *view, s32 w, s32 h)
{
        GLint max_samples = 0;
        GLenum status;

        if(view->fbo_msaa && view->width == w && view->height == h) return 0;
        s_view_free(view);

        glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
        view->samples = max_samples < MSAA_SAMPLES ? max_samples :
                MSAA_SAMPLES;

        glGenRenderbuffers(1, &view->rb_color);
        glBindRenderbuffer(GL_RENDERBUFFER, view->rb_color);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, view->samples,
                        GL_RGBA8, w, h);
        glGenRenderbuffers(1, &view->rb_depth);
        glBindRenderbuffer(GL_RENDERBUFFER, view->rb_depth);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, view->samples,
                        GL_DEPTH_COMPONENT24, w, h);
        glGenFramebuffers(1, &view->fbo_msaa);
        glBindFramebuffer(GL_FRAMEBUFFER, view->fbo_msaa);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                        GL_RENDERBUFFER, view->rb_color);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                        GL_RENDERBUFFER, view->rb_depth);
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        glGenRenderbuffers(1, &view->rb_resolve);
        glBindRenderbuffer(GL_RENDERBUFFER, view->rb_resolve);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
        glGenFramebuffers(1, &view->fbo_resolve);
        glBindFramebuffer(GL_FRAMEBUFFER, view->fbo_resolve);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                        GL_RENDERBUFFER, view->rb_resolve);
        if(status == GL_FRAMEBUFFER_COMPLETE)
                status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if(status != GL_FRAMEBUFFER_COMPLETE) {
                LOG(LOG_ERROR, "scene: can't create the view framebuffer");
                s_view_free(view);
                return -1;
        }
        view->width = w;
        view->height = h;
        return 0;
}

LOC void s_view_free(SceneView *view)
{
        if(view->fbo_msaa) glDeleteFramebuffers(1, &view->fbo_msaa);
        if(view->fbo_resolve) glDeleteFramebuffers(1, &view->fbo_resolve);
        if(view->rb_color) glDeleteRenderbuffers(1, &view->rb_color);
        if(view->rb_depth) glDeleteRenderbuffers(1, &view->rb_depth);
        if(view->rb_resolve) glDeleteRenderbuffers(1, &view->rb_resolve);
        view->fbo_msaa = view->fbo_resolve = 0;
        view->rb_color = view->rb_depth = view->rb_resolve = 0;
        view->width = view->height = 0;
}

/* A VAO for interleaved position + normal; leaves it bound */
LOC GLuint s_vertex_array(GLuint vbo, GLuint ebo)
{
        GLsizei stride = FLOATS_PER_VERTEX * sizeof(f32);
        GLuint vao;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if(ebo) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)(3 * sizeof(f32)));
        return vao;
}

/*
 * Interleave positions and normals. Without normals every triangle gets its
 * own three vertices with the face normal (flat shading).
 */
LOC f32 *s_build_vertices(const SceneMeshData *d, s32 *count_out)
{
        const f32 *p = d->positions;
        f32 *v;
        s32 tris;
        s32 count;
        s32 i;
        s32 k;
        u32 idx[3];
        FVector3 a;
        FVector3 b;
        FVector3 c;
        FVector3 n;

        if(d->normals) {
                count = d->vertex_count;
                if(!(v = malloc((size_t)count * FLOATS_PER_VERTEX *
                                                sizeof(f32))))
                        return NULL;
                for(i = 0; i < count; i++) {
                        memcpy(v + i * 6, p + i * 3, 3 * sizeof(f32));
                        memcpy(v + i * 6 + 3, d->normals + i * 3,
                                        3 * sizeof(f32));
                }
                *count_out = count;
                return v;
        }

        tris = d->indices ? d->index_count / 3 : d->vertex_count / 3;
        count = tris * 3;
        if(!(v = malloc((size_t)count * FLOATS_PER_VERTEX * sizeof(f32))))
                return NULL;

        for(i = 0; i < tris; i++) {
                for(k = 0; k < 3; k++) {
                        idx[k] = d->indices ? d->indices[i * 3 + k] :
                                (u32)(i * 3 + k);
                }
                a = fvec3(p[idx[0] * 3], p[idx[0] * 3 + 1], p[idx[0] * 3 + 2]);
                b = fvec3(p[idx[1] * 3], p[idx[1] * 3 + 1], p[idx[1] * 3 + 2]);
                c = fvec3(p[idx[2] * 3], p[idx[2] * 3 + 1], p[idx[2] * 3 + 2]);
                n = fvec3_cross(fvec3_sub(b, a), fvec3_sub(c, a));
                if(fvec3_mag_sqr(n) > 0.0f) n = fvec3_norm(n);

                for(k = 0; k < 3; k++) {
                        memcpy(v + (i * 3 + k) * 6, p + idx[k] * 3,
                                        3 * sizeof(f32));
                        v[(i * 3 + k) * 6 + 3] = n.x;
                        v[(i * 3 + k) * 6 + 4] = n.y;
                        v[(i * 3 + k) * 6 + 5] = n.z;
                }
        }
        *count_out = count;
        return v;
}

/*
 * Lines on the XY plane every step, lines + 1 each way, centered on the
 * origin. Every fifth is brighter; the X and Y axes are red and green.
 */
LOC s32 s_build_grid(Scene3D *scene, f32 step, s32 lines)
{
        static const f32 minor[3] = {0.20f, 0.20f, 0.20f};
        static const f32 major[3] = {0.28f, 0.28f, 0.28f};
        static const f32 x_axis[3] = {0.78f, 0.28f, 0.26f};
        static const f32 y_axis[3] = {0.33f, 0.62f, 0.25f};
        s32 half = lines / 2;
        f32 ext = (f32)half * step;
        f32 *v;
        f32 t;
        s32 n = 0;
        s32 i;

        if(step <= 0.0f || half <= 0) return 0;
        v = malloc((size_t)(half * 2 + 1) * 4 * FLOATS_PER_VERTEX *
                        sizeof(f32));
        if(!v) return -1;

        for(i = -half; i <= half; i++) {
                t = (f32)i * step;
                s_push_line(v, &n, t, -ext, t, ext,
                                i == 0 ? y_axis : (i % 5 ? minor : major));
                s_push_line(v, &n, -ext, t, ext, t,
                                i == 0 ? x_axis : (i % 5 ? minor : major));
        }

        glGenBuffers(1, &scene->grid_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, scene->grid_vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)n * FLOATS_PER_VERTEX *
                        (GLsizeiptr)sizeof(f32), v, GL_STATIC_DRAW);
        free(v);
        scene->grid_vao = s_vertex_array(scene->grid_vbo, 0);
        glBindVertexArray(0);
        scene->grid_vertices = n;
        return 0;
}

/* Two vertices; the color goes where meshes have their normal */
LOC void s_push_line(f32 *v, s32 *n, f32 x0, f32 y0, f32 x1, f32 y1,
                const f32 rgb[3])
{
        f32 *p = v + *n * FLOATS_PER_VERTEX;

        p[0] = x0;
        p[1] = y0;
        p[2] = 0.0f;
        memcpy(p + 3, rgb, 3 * sizeof(f32));
        p[6] = x1;
        p[7] = y1;
        p[8] = 0.0f;
        memcpy(p + 9, rgb, 3 * sizeof(f32));
        *n += 2;
}

LOC SceneObjectData *s_object(Scene3D *scene, SceneObject obj)
{
        return pool_get(&scene->objects, obj);
}
