#ifndef VASALL_SCENE_H
#define VASALL_SCENE_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/list.h>
#include <devkit/math/vector.h>
#include <devkit/math/quaternion.h>

#include <shape/rectangle.h>
#include <graphic/general/color.h>
#include <render/renderer.h>
#include <scene/camera.h>

/*
 * A model placed in the world. Everything is f64; the conversion to f32
 * happens in scene_render, relative to the camera.
 */
typedef struct SceneObject {
        ModelId model;

        Vector3 position;
        Quaternion rotation;
        Vector3 scale;
} SceneObject;

/*
 * A list of models, drawn with one shader. The shader gets:
 *
 *   uniform mat4 u_mvp;         model -> clip space
 *   uniform mat4 u_model_view;  model -> camera space, for lighting
 */
typedef struct Scene {
        List objects;           /* List of SceneObject */
        ShaderId shader;
        FColor background;
} Scene;

/*
 * Create an empty scene that draws with the given shader.
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 scene_init(Scene *scn, ShaderId shader);

/*
 * Free the scene. The models and the shader are not destroyed, they belong
 * to the renderer.
 */
PUB void scene_deinit(Scene *scn);

/*
 * Add a model at a position, without rotation and with scale 1.
 *
 * @returns The index of the object, or -1 if an error occurred
 */
PUB s32 scene_add(Scene *scn, ModelId model, Vector3 position);

/*
 * Get an object to move, rotate or scale it. The pointer is only valid until
 * the next scene_add.
 *
 * @returns The object, or NULL if the index is out of range
 */
PUB SceneObject *scene_object(Scene *scn, s32 index);

/*
 * Draw the scene into an area of the window, as seen by the camera. The area
 * is cleared with the scene's background first.
 *
 * @param scn Pointer to the scene
 * @param ren Pointer to the renderer, inside render_begin/render_end
 * @param cam The camera to look through
 * @param area The area in window pixels, (0, 0) at the top left
 * @param window_h The height of the window in pixels
 */
PUB void scene_render(Scene *scn, Renderer *ren, Camera *cam, Rectangle area,
                      s32 window_h);

#endif /* VASALL_SCENE_H */
