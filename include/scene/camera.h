#ifndef VASALL_CAMERA_H
#define VASALL_CAMERA_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/math/vector.h>
#include <devkit/math/matrix.h>
#include <devkit/math/quaternion.h>

/*
 * A perspective camera in world space. Like in OpenGL, the camera looks
 * along its -Z axis, +Y is up and +X is right.
 */
typedef struct Camera {
        Vector3 position;
        Quaternion orientation; /* Rotation from camera to world space */

        f64 fov;                /* Vertical field of view in radians */
        f64 near;               /* Distance of the near plane, > 0 */
        f64 far;                /* Distance of the far plane, > near */
} Camera;

/*
 * Initialize a camera at (0, 0, 5) looking at the origin, with a 60 degree
 * field of view.
 */
PUB void camera_init(Camera *cam);

/*
 * Turn the camera so it looks at the target. up is the direction that should
 * point up on screen, e.g. (0, 1, 0). Does nothing if the target is at the
 * camera's position or up points along the view direction.
 */
PUB void camera_look_at(Camera *cam, Vector3 target, Vector3 up);

/*
 * The rotation part of the view matrix (world to camera), without the
 * translation. The renderer subtracts the camera position from every object
 * itself, so the f32 matrices sent to the GPU only hold small numbers.
 */
PUB Matrix4 camera_view_rotation(Camera *cam);

/*
 * The perspective projection matrix for an area with the given aspect ratio
 * (width / height). Maps depth to -1 .. 1 like OpenGL expects.
 */
PUB Matrix4 camera_projection(Camera *cam, f64 aspect);

#endif /* VASALL_CAMERA_H */
