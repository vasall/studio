#include "unity.h"

#include <math.h>

#include <render/camera.h>

LOC Camera3D s_cam;

void setUp(void)
{
        camera_init(&s_cam);
        s_cam.target = fvec3(10.0f, 20.0f, 5.0f);
        s_cam.yaw = 0.0f;
        s_cam.pitch = 0.0f;
        s_cam.distance = 100.0f;
}

void tearDown(void)
{
}

void test_eye_orbits_the_target(void)
{
        FVector3 e = camera_eye(&s_cam);

        /* yaw 0, pitch 0: on +X of the target */
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, 110.0f, e.x);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, 20.0f, e.y);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, 5.0f, e.z);

        s_cam.pitch = 1.5707963f * 0.999f;
        e = camera_eye(&s_cam);
        TEST_ASSERT_FLOAT_WITHIN(0.5f, 105.0f, e.z);
}

void test_view_puts_target_in_front(void)
{
        FMatrix4 v;
        FVector4 p;

        s_cam.yaw = 0.7f;
        s_cam.pitch = 0.4f;
        v = camera_view(&s_cam);
        p = fmat4_xfm(v, fvec4(10.0f, 20.0f, 5.0f, 1.0f));
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, p.x);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, p.y);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, -100.0f, p.z);

        /* World up stays up on screen */
        p = fmat4_xfm(v, fvec4(10.0f, 20.0f, 15.0f, 1.0f));
        TEST_ASSERT_TRUE(p.y > 0.0f);
}

void test_projection_maps_target_depth_inside(void)
{
        FMatrix4 m = fmat4_mul(camera_proj(&s_cam, 2.0f), camera_view(&s_cam));
        FVector4 p = fmat4_xfm(m, fvec4(10.0f, 20.0f, 5.0f, 1.0f));

        TEST_ASSERT_TRUE(p.w > 0.0f);
        TEST_ASSERT_TRUE(p.z / p.w > -1.0f && p.z / p.w < 1.0f);
}

void test_orbit_clamps_pitch(void)
{
        camera_orbit(&s_cam, 0.0f, 10000.0f);
        TEST_ASSERT_TRUE(s_cam.pitch < 1.5708f);
        camera_orbit(&s_cam, 0.0f, -20000.0f);
        TEST_ASSERT_TRUE(s_cam.pitch > -1.5708f);
}

void test_zoom_and_frame(void)
{
        camera_zoom(&s_cam, 1.0f);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, 85.0f, s_cam.distance);
        camera_zoom(&s_cam, -1.0f);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, 100.0f, s_cam.distance);

        camera_frame(&s_cam, fvec3(1.0f, 2.0f, 3.0f), 50.0f);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, 3.0f, s_cam.target.z);
        TEST_ASSERT_FLOAT_WITHIN(0.01f,
                        50.0f / sinf(s_cam.fov * 0.5f) * 1.1f, s_cam.distance);
}

void test_pan_moves_target_sideways(void)
{
        FVector3 before = s_cam.target;

        /* Looking along -X: dragging right moves the target to -Y... */
        camera_pan(&s_cam, 100.0f, 0.0f, 500.0f);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, before.x, s_cam.target.x);
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, before.z, s_cam.target.z);
        TEST_ASSERT_TRUE(fabsf(s_cam.target.y - before.y) > 1.0f);
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_eye_orbits_the_target);
        RUN_TEST(test_view_puts_target_in_front);
        RUN_TEST(test_projection_maps_target_depth_inside);
        RUN_TEST(test_orbit_clamps_pitch);
        RUN_TEST(test_zoom_and_frame);
        RUN_TEST(test_pan_moves_target_sideways);
        return UNITY_END();
}
