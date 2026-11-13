#include <render/camera.h>

#include <math.h>

#define PI            3.14159265358979f
#define ORBIT_SPEED   0.008f    /* Radians per pixel */
#define PITCH_LIMIT   1.55f     /* Just under 90 degrees, so up stays valid */
#define ZOOM_STEP     0.85f     /* Distance factor per step */
#define MIN_DISTANCE  0.01f

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void camera_init(Camera3D *cam)
{
        cam->target = fvec3(0.0f, 0.0f, 0.0f);
        cam->yaw = -PI * 0.25f;         /* Eye at +X -Y: front right */
        cam->pitch = PI / 6.0f;
        cam->distance = 300.0f;
        cam->fov = PI / 4.0f;
}

PUB FVector3 camera_eye(const Camera3D *cam)
{
        f32 c = cosf(cam->pitch);

        return fvec3_add(cam->target, fvec3_scale(fvec3(c * cosf(cam->yaw),
                                        c * sinf(cam->yaw),
                                        sinf(cam->pitch)), cam->distance));
}

PUB FMatrix4 camera_view(const Camera3D *cam)
{
        FMatrix4 m = fmat4_identity();
        FVector3 eye = camera_eye(cam);
        FVector3 f = fvec3_norm(fvec3_sub(cam->target, eye));
        FVector3 s = fvec3_norm(fvec3_cross(f, fvec3(0.0f, 0.0f, 1.0f)));
        FVector3 u = fvec3_cross(s, f);

        /* Rows are the camera axes; column-major storage */
        m.data[0] = s.x;
        m.data[4] = s.y;
        m.data[8] = s.z;
        m.data[1] = u.x;
        m.data[5] = u.y;
        m.data[9] = u.z;
        m.data[2] = -f.x;
        m.data[6] = -f.y;
        m.data[10] = -f.z;
        m.data[12] = -fvec3_dot(s, eye);
        m.data[13] = -fvec3_dot(u, eye);
        m.data[14] = fvec3_dot(f, eye);
        return m;
}

PUB FMatrix4 camera_proj(const Camera3D *cam, f32 aspect)
{
        FMatrix4 m = fmat4_zero();
        f32 near_z = cam->distance * 0.01f;
        f32 far_z = cam->distance * 100.0f;
        f32 f = 1.0f / tanf(cam->fov * 0.5f);

        if(aspect <= 0.0f) aspect = 1.0f;
        m.data[0] = f / aspect;
        m.data[5] = f;
        m.data[10] = (far_z + near_z) / (near_z - far_z);
        m.data[11] = -1.0f;
        m.data[14] = 2.0f * far_z * near_z / (near_z - far_z);
        return m;
}

PUB void camera_orbit(Camera3D *cam, f32 dx, f32 dy)
{
        /* Dragging right turns the scene right: the eye goes left */
        cam->yaw -= dx * ORBIT_SPEED;
        cam->pitch += dy * ORBIT_SPEED;
        if(cam->pitch > PITCH_LIMIT) cam->pitch = PITCH_LIMIT;
        if(cam->pitch < -PITCH_LIMIT) cam->pitch = -PITCH_LIMIT;
        cam->yaw = fmodf(cam->yaw, 2.0f * PI);
}

PUB void camera_pan(Camera3D *cam, f32 dx, f32 dy, f32 view_h)
{
        FVector3 eye = camera_eye(cam);
        FVector3 f = fvec3_norm(fvec3_sub(cam->target, eye));
        FVector3 s = fvec3_norm(fvec3_cross(f, fvec3(0.0f, 0.0f, 1.0f)));
        FVector3 u = fvec3_cross(s, f);
        f32 per_px;

        if(view_h <= 0.0f) return;

        /* World units per pixel at the target's depth */
        per_px = 2.0f * cam->distance * tanf(cam->fov * 0.5f) / view_h;
        cam->target = fvec3_sub(cam->target, fvec3_scale(s, dx * per_px));
        cam->target = fvec3_add(cam->target, fvec3_scale(u, dy * per_px));
}

PUB void camera_zoom(Camera3D *cam, f32 steps)
{
        cam->distance *= powf(ZOOM_STEP, steps);
        if(cam->distance < MIN_DISTANCE) cam->distance = MIN_DISTANCE;
}

PUB void camera_frame(Camera3D *cam, FVector3 center, f32 radius)
{
        cam->target = center;
        if(radius <= 0.0f) radius = 1.0f;
        cam->distance = radius / sinf(cam->fov * 0.5f) * 1.1f;
}
