#ifndef VASALL_STUDIO_SCENE_H
#define VASALL_STUDIO_SCENE_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/math/matrix.h>
#include <devkit/math/vector.h>
#include <devkit/memory/pool.h>

#include <glad/gl.h>

#include <render/camera.h>
#include <ui/ui.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/**
 * Meshes and objects are referenced by handle: a removed one is simply
 * skipped, so nothing dangles (e.g. an object whose mesh was replaced).
 */
typedef PoolHandle SceneMesh;
typedef PoolHandle SceneObject;

/**
 * Triangle mesh data for scene_mesh_add. It is copied to the GPU, so the
 * arrays can be freed afterwards.
 */
typedef struct SceneMeshData {
        const f32 *positions;   /* x, y, z per vertex */
        const f32 *normals;     /* x, y, z per vertex, or NULL: flat shading
                                   with normals from the triangles */
        const u32 *indices;     /* 3 per triangle, or NULL: every 3 vertices
                                   are a triangle */
        s32 vertex_count;
        s32 index_count;
} SceneMeshData;

/**
 * A set of meshes on the GPU and objects that place them in the world.
 * Upload a mesh once (a part), then add objects (instances of the part)
 * and move them every frame with scene_object_set_transform, e.g. from
 * simulation state; that costs one matrix per object.
 */
typedef struct Scene3D {
        Pool meshes;            /* SceneMeshGPU, see scene.c */
        Pool objects;           /* SceneObjectData, see scene.c */

        GLuint program;
        GLint u_model;
        GLint u_view;
        GLint u_proj;
        GLint u_normal;
        GLint u_color;
        GLint u_unlit;

        GLuint grid_vao;
        GLuint grid_vbo;
        s32 grid_vertices;
        s32 show_grid;

        UIColor background;
} Scene3D;

/**
 * One view into a scene: a camera and its own offscreen framebuffer
 * (multisampled, so edges are smooth). Several views can show one scene,
 * e.g. in split editors. Owned by whoever shows the view.
 */
typedef struct SceneView {
        Camera3D camera;

        GLuint fbo_msaa;        /* Rendered into */
        GLuint rb_color;
        GLuint rb_depth;
        GLuint fbo_resolve;     /* Single sample, copied to the window */
        GLuint rb_resolve;
        s32 width;              /* Framebuffer pixels, 0 = not created */
        s32 height;
        s32 samples;
} SceneView;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/**
 * Needs a current GL context. grid_step/grid_lines set the ground grid on
 * the XY plane (e.g. 10 mm, 20 lines each way); 0 lines means no grid.
 *
 * @returns 0 on success, -1 on error
 */
PUB s32 scene_init(Scene3D *scene, f32 grid_step, s32 grid_lines);
PUB void scene_deinit(Scene3D *scene);

/**
 * Upload a triangle mesh.
 *
 * @returns The mesh, or { POOL_HANDLE_NONE } on error
 */
PUB SceneMesh scene_mesh_add(Scene3D *scene, const SceneMeshData *data);

/**
 * Free a mesh. Objects that use it are no longer drawn.
 */
PUB void scene_mesh_remove(Scene3D *scene, SceneMesh mesh);

/**
 * Center and radius of a sphere around the mesh, in mesh coordinates.
 *
 * @returns 0, or -1 if the mesh doesn't exist
 */
PUB s32 scene_mesh_bounds(Scene3D *scene, SceneMesh mesh, FVector3 *center,
                f32 *radius);

/**
 * Place a mesh in the world with a transform (mesh to world) and a color.
 *
 * @returns The object, or { POOL_HANDLE_NONE } on error
 */
PUB SceneObject scene_object_add(Scene3D *scene, SceneMesh mesh,
                FMatrix4 transform, UIColor color);
PUB void scene_object_remove(Scene3D *scene, SceneObject obj);

PUB void scene_object_set_transform(Scene3D *scene, SceneObject obj,
                FMatrix4 transform);
PUB void scene_object_set_color(Scene3D *scene, SceneObject obj,
                UIColor color);
PUB void scene_object_set_visible(Scene3D *scene, SceneObject obj,
                s32 visible);

/**
 * A view with a camera from camera_init and no framebuffer yet; that is
 * made on first draw. deinit needs the GL context.
 */
PUB void scene_view_init(SceneView *view);
PUB void scene_view_deinit(SceneView *view);

/**
 * The viewport widget for a scene: ui_viewport with view's camera, drawing
 * scene. Scene and view must live until the frame is rendered.
 */
PUB UISignal scene_viewport(UI *ui, const char *id, Scene3D *scene,
                SceneView *view, const char *cls);

#endif /* VASALL_STUDIO_SCENE_H */
