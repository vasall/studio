#include <scene/camera.h>

#include <devkit/log/log.h>

#include <math.h>

#define CAMERA_PI 3.14159265358979323846

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void camera_init(Camera *cam)
{
        if(!cam) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return;
        }

        /* At +Z with no rotation, the camera looks along -Z at the origin */
        cam->position = vec3(0.0, 0.0, 5.0);
        cam->orientation = quat_identity();

        cam->fov = 60.0 * CAMERA_PI / 180.0;
        cam->near = 0.1;
        cam->far = 1000.0;
}

PUB void camera_look_at(Camera *cam, Vector3 target, Vector3 up)
{
        Vector3 forward;
        Vector3 right;
        Vector3 cam_up;

        if(!cam) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return;
        }

        forward = vec3_sub(target, cam->position);
        if(vec3_mag_sqr(forward) == 0.0) {
                return;
        }
        forward = vec3_norm(forward);

        right = vec3_cross(forward, up);
        if(vec3_mag_sqr(right) == 0.0) {
                return;
        }
        right = vec3_norm(right);
        cam_up = vec3_cross(right, forward);

        /* The columns are the camera's axes in world space; it looks along -Z */
        cam->orientation = quat_norm(quat_from_mat3(
                mat3(right, cam_up, vec3_scale(forward, -1.0))));
}

PUB Matrix4 camera_view_rotation(Camera *cam)
{
        /* World to camera is the inverse, which for a rotation is conj */
        return quat_to_mat4(quat_conj(quat_norm(cam->orientation)));
}

PUB Matrix4 camera_projection(Camera *cam, f64 aspect)
{
        Matrix4 m = mat4_zero();
        f64 f = 1.0 / tan(cam->fov / 2.0);
        f64 depth = cam->near - cam->far;

        if(aspect <= 0.0) {
                aspect = 1.0;
        }

        m.data[0] = f / aspect;
        m.data[5] = f;
        m.data[10] = (cam->far + cam->near) / depth;
        m.data[11] = -1.0;
        m.data[14] = 2.0 * cam->far * cam->near / depth;

        return m;
}
