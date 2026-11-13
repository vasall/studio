#ifndef VASALL_STUDIO_CAMERA_H
#define VASALL_STUDIO_CAMERA_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/math/matrix.h>
#include <devkit/math/vector.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * An orbit camera around a target point. The world is Z-up (as usual in CAD
 * and robotics), units are millimeters. Only math, no GL, so it can be used
 * and tested anywhere.
 */
typedef struct Camera3D {
        FVector3 target;        /* Point the camera orbits around */
        f32 yaw;                /* Around world Z, radians, 0 looks along -X */
        f32 pitch;              /* Above the XY plane, radians */
        f32 distance;           /* From the target */
        f32 fov;                /* Vertical field of view, radians */
} Camera3D;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * A view from the front right, slightly above, 300 mm away.
 */
PUB void camera_init(Camera3D *cam);

/*
 * Position of the camera.
 */
PUB FVector3 camera_eye(const Camera3D *cam);

/*
 * World to view space (right-handed, looking down -Z in view space).
 */
PUB FMatrix4 camera_view(const Camera3D *cam);

/*
 * View to clip space for a viewport of the given aspect ratio (w / h). The
 * near and far planes follow the distance, so zooming keeps depth precise.
 */
PUB FMatrix4 camera_proj(const Camera3D *cam, f32 aspect);

/*
 * Rotate around the target by a mouse movement in pixels.
 */
PUB void camera_orbit(Camera3D *cam, f32 dx, f32 dy);

/*
 * Move the target sideways and up/down so the scene follows the mouse;
 * view_h is the viewport height in the same pixels as dx/dy.
 */
PUB void camera_pan(Camera3D *cam, f32 dx, f32 dy, f32 view_h);

/*
 * Move closer (steps > 0) or farther away, e.g. by wheel steps.
 */
PUB void camera_zoom(Camera3D *cam, f32 steps);

/*
 * Look at a sphere so that it fills the view.
 */
PUB void camera_frame(Camera3D *cam, FVector3 center, f32 radius);

#endif /* VASALL_STUDIO_CAMERA_H */
