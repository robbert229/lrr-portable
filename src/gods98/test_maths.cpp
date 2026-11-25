// SPDX-License-Identifier: LGPL-3.0-or-later
/* maths.c — triangle projection and winding, intersections and matrix
 * layout and composition. Use a tolerance for operations involving trig. */
#include <limits>
#include "gods98/maths.h"
#include "test/lrr_test.h"

static void check_triangles()
{
    const float a[] = {0, 0, 0}, b[] = {4, 0, 0}, c[] = {0, 3, 0};
    const float lifted[] = {0, 3, 8};
    float normal[3] = {};
    CHECK_EQ(surface_triangle_normal_calculate(normal, a, b, c), normal + 0);
    CHECK_NEAR(normal[0], 0.0f, 1e-6);
    CHECK_NEAR(normal[1], 0.0f, 1e-6);
    CHECK_NEAR(normal[2], 1.0f, 1e-6);
    surface_triangle_normal_calculate(normal, a, c, b);
    CHECK_NEAR(normal[2], -1.0f, 1e-6);
    CHECK_NEAR(surface_triangle_area_calculate(a, b, lifted, 0), 6.0f, 1e-5);
    CHECK_EQ(surface_triangle_area_calculate(a, b, c, 1), 0.0f);
    CHECK_NEAR(surface_triangle_area_calculate(a, c, b, 1), 6.0f, 1e-5);
    const float nonfinite[] = {std::numeric_limits<float>::infinity(), 0, 0};
    const float invalid[] = {std::numeric_limits<float>::quiet_NaN(), 0, 0};
    const int culls[] = {0, 1};
    for (int cull : culls) {
        CHECK_EQ(surface_triangle_area_calculate(a, a, a, cull), 0.0f);
        CHECK_EQ(surface_triangle_area_calculate(a, a, b, cull), 0.0f);
        CHECK_EQ(surface_triangle_area_calculate(a, nonfinite, b, cull), 0.0f);
        CHECK_EQ(surface_triangle_area_calculate(invalid, invalid, invalid, cull), 0.0f);
    }
}

static void check_intersections()
{
    const float origin[] = {1, 2, 3}, ray[] = {2, 0, -4};
    const float plane[] = {0, 0, -1}, normal[] = {0, 0, 5};
    const float parallel[] = {1, 0, 0};
    float parameter = -1.0f, hit[3] = {};
    CHECK_EQ(ray_plane_intersection_parameter(&parameter, origin, ray, plane, normal), 1);
    CHECK_NEAR(parameter, 1.0f, 1e-6);
    CHECK_EQ(view_ray_intersection_test(hit, origin, ray, plane, normal), 1);
    CHECK_NEAR(hit[0], 3.0f, 1e-6);
    CHECK_NEAR(hit[1], 2.0f, 1e-6);
    CHECK_NEAR(hit[2], -1.0f, 1e-6);
    CHECK_EQ(ray_plane_intersection_parameter(&parameter, origin, parallel, plane, normal), 0);
    CHECK_NEAR(parameter, 1.0f, 1e-6);

    const float p1[] = {1, 2}, d1[] = {1, 1};
    const float p2[] = {3, 8}, d2[] = {0, 1};
    CHECK_EQ(line_intersection_2d_compute(hit, p1, d1, p2, d2), hit + 0);
    CHECK_NEAR(hit[0], 3.0f, 1e-6);
    CHECK_NEAR(hit[1], 4.0f, 1e-6);
    CHECK_EQ(line_intersection_2d_compute(hit, p1, d1, p2, d1), nullptr);

    const float from[] = {0, 0, 4, 0, 4, 4, 0, 4};
    const float to[] = {4, 0, 4, 4, 0, 4, 0, 0};
    const float inside[] = {2, 2}, outside[] = {5, 2};
    CHECK_EQ(point_in_polygon_2d(inside, from, to, 4), 1);
    CHECK_EQ(point_in_polygon_2d(outside, from, to, 4), 0);

    const float centre[] = {0, 0, 0}, camera[] = {0, 0, -5};
    const float forward[] = {0, 0, 1};
    CHECK_EQ(sphere_ray_intersection_discriminant_positive(centre, 1, camera, forward), 1);
    CHECK_EQ(sphere_ray_intersection_discriminant_positive(centre, 1, camera, parallel), 0);
}

static void check_matrices()
{
    const float shift[] = {2, 3, 4};
    constexpr float quarterTurn = 1.57079633f;
    float translation[16], rotation[16], result[16], copy[16];
    matrix4_identity_translation_set(translation, shift);
    const float expected[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 3, 4, 1};
    for (int i = 0; i < 16; ++i) CHECK_EQ(translation[i], expected[i]);

    matrix4_set_rotation_x(rotation, quarterTurn);
    CHECK_NEAR(rotation[6], -1.0f, 1e-6);
    CHECK_NEAR(rotation[9], 1.0f, 1e-6);
    matrix4_set_rotation_y(rotation, quarterTurn);
    CHECK_NEAR(rotation[2], 1.0f, 1e-6);
    CHECK_NEAR(rotation[8], -1.0f, 1e-6);
    matrix4_set_rotation_z(rotation, quarterTurn);
    CHECK_NEAR(rotation[1], -1.0f, 1e-6);
    CHECK_NEAR(rotation[4], 1.0f, 1e-6);

    matrix4_multiply(result, rotation, translation);
    CHECK_NEAR(result[12], 3.0f, 1e-6);
    CHECK_NEAR(result[13], -2.0f, 1e-6);
    CHECK_NEAR(result[14], 4.0f, 1e-6);
    CHECK_EQ(result[15], 1.0f);
    matrix4_copy(copy, result);
    for (int i = 0; i < 16; ++i) CHECK_EQ(copy[i], result[i]);
    matrix4_set_zero(copy);
    for (float value : copy) CHECK_EQ(value, 0.0f);
}

int main()
{
    check_triangles();
    check_intersections();
    check_matrices();
    return lrr_test_result();
}
