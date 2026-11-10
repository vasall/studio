#include <scene/scene.h>

#include <devkit/log/log.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * The model matrix of an object with the camera at the origin. Subtracting
 * the camera position in f64 keeps the numbers small, so they survive the
 * conversion to f32 even far away from the world origin.
 */
LOC Matrix4 s_model_matrix(SceneObject *obj, Vector3 cam_pos);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 scene_init(Scene *scn, ShaderId shader)
{
        if(!scn) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(list_init(&scn->objects, sizeof(SceneObject), 16) < 0) {
                LOG(LOG_ERROR, "Failed to create the scene object list");
                return -1;
        }

        scn->shader = shader;
        scn->background = fcolor(0.15f, 0.15f, 0.18f, 1.0f);

        return 0;
}

PUB void scene_deinit(Scene *scn)
{
        if(!scn) {
                return;
        }

        list_deinit(&scn->objects);
}

PUB s32 scene_add(Scene *scn, ModelId model, Vector3 position)
{
        SceneObject obj;
        s32 index;

        if(!scn) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        obj.model = model;
        obj.texture.index = -1;
        obj.texture.generation = 0;
        obj.position = position;
        obj.rotation = quat_identity();
        obj.scale = vec3(1.0, 1.0, 1.0);

        index = list_count(&scn->objects);
        if(list_push(&scn->objects, 1, &obj) < 0) {
                LOG(LOG_ERROR, "Failed to add object to scene");
                return -1;
        }

        return index;
}

PUB SceneObject *scene_object(Scene *scn, s32 index)
{
        if(!scn || index < 0 || index >= list_count(&scn->objects)) {
                return NULL;
        }

        return list_get(&scn->objects, index);
}

PUB void scene_render(Scene *scn, Renderer *ren, Camera *cam, Rectangle area,
                      s32 window_h)
{
        Matrix4 view;
        Matrix4 proj;
        s32 i;

        if(!scn || !ren || !cam || area.w < 1 || area.h < 1) {
                return;
        }

        render_set_area(ren, area, window_h);
        render_clear(ren, scn->background);

        render_set_depth_test(ren, 1);
        render_set_blend(ren, RENDER_BLEND_NONE);
        render_use_shader(ren, scn->shader);
        render_set_s32(ren, "u_texture", 0);

        view = camera_view_rotation(cam);
        proj = camera_projection(cam, (f64)area.w / (f64)area.h);

        for(i = 0; i < list_count(&scn->objects); i++) {
                SceneObject *obj = list_get(&scn->objects, i);
                FMatrix4 mv_f;
                FMatrix4 mvp_f;
                Matrix4 mv;

                /* Everything is combined in f64, only the result is f32 */
                mv = mat4_mul(view, s_model_matrix(obj, cam->position));
                mv_f = fmat4_from_mat4(mv);
                mvp_f = fmat4_from_mat4(mat4_mul(proj, mv));

                render_set_mat4(ren, "u_model_view", &mv_f);
                render_set_mat4(ren, "u_mvp", &mvp_f);
                render_set_texture(ren, 0, RENDER_ID_VALID(obj->texture) ?
                                   obj->texture : ren->white);
                render_draw_model(ren, obj->model);
        }

        render_set_depth_test(ren, 0);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC Matrix4 s_model_matrix(SceneObject *obj, Vector3 cam_pos)
{
        Vector3 rel = vec3_sub(obj->position, cam_pos);
        Matrix4 m = quat_to_mat4(quat_norm(obj->rotation));
        s32 row;

        /* Scale the columns: m = R * S */
        for(row = 0; row < 3; row++) {
                m.data[0 + row] *= obj->scale.x;
                m.data[4 + row] *= obj->scale.y;
                m.data[8 + row] *= obj->scale.z;
        }

        /* Then translate: m = T * R * S */
        m.data[12] = rel.x;
        m.data[13] = rel.y;
        m.data[14] = rel.z;

        return m;
}
