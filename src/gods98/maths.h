// SPDX-License-Identifier: LGPL-3.0-or-later
/* Maths.c (original translation unit G98-007, finding 505): the gods98
 * vector, ray and 4x4 matrix helpers, and the D3DRMVector wrappers.
 *
 * C-compatible header (ADR 0005). Maths.c is compiled as C in the matching
 * build and as C++ in the native port. Vectors are three adjacent floats
 * (x, y, z), 2D points two, and matrices sixteen in row-major order. The
 * C++ declarations in view.hpp and level.hpp mirror these;
 * test_c_boundaries.cpp checks that they agree. */
#ifndef LEGORRR_GODS98_MATHS_H
#define LEGORRR_GODS98_MATHS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Maths_Vector3DRandom (FUN_00479500): D3DRMVectorRandom(r). Returns r. */
float* d3drm_vector_random(float* r);

/* Maths_Vector3DRotate (FUN_00479510): D3DRMVectorRotate(r, v, axis,
 * theta). The import normalises axis in place; r may alias v or axis. */
float* d3drm_vector_rotate(float* r, float* v, float* axis, float theta);

/* Maths_PlaneNormal (FUN_00479530): the unit normal of the plane through
 * p1, p2, p3, cross(p2 - p1, p3 - p2), with no zero-length guard. */
float* surface_triangle_normal_calculate(float* n, const float* p1,
                                         const float* p2, const float* p3);

/* Maths_TriangleAreaZ (FUN_00479630): the area of the triangle projected
 * onto z = 0. With bfc set, a triangle whose normal faces +z has area 0. */
float surface_triangle_area_calculate(const float* p1, const float* p2,
                                      const float* p3, int bfc);

/* Maths_Rand (FUN_004798a0): the CRT's rand(), 0..0x7fff. */
short rng_thunk_draw15(void);

/* Maths_RandRange (FUN_004798b0): a random value in [low, high]. */
float random_float_range(float low, float high);

/* Maths_RayPlaneIntersection (FUN_004798e0): normalise copies of ray and
 * planeNormal, then intersect. Writes endPoint and returns 1 on a hit. */
int view_ray_intersection_test(float* endPoint, const float* rayOrigin,
                               const float* ray, const float* planePoint,
                               const float* planeNormal);

/* Maths_RayPlaneDistance (FUN_00479a30): writes the ray parameter t of the
 * hit to *t and returns 1, or returns 0 when the ray is parallel. */
int ray_plane_intersection_parameter(float* t, const float* rayOrigin,
                                     const float* ray,
                                     const float* planePoint,
                                     const float* planeNormal);

/* Maths_RayEndPoint (FUN_00479ab0): endPoint = rayOrigin + t * ray. */
void point_along_ray(float* endPoint, const float* rayOrigin,
                     const float* ray, float t);

/* Maths_Vector2DIntersection (FUN_00479af0): intersect the 2D lines
 * (p1, d1) and (p2, d2). Writes and returns r, or NULL when parallel. */
float* line_intersection_2d_compute(float* r, const float* p1,
                                    const float* d1, const float* p2,
                                    const float* d2);

/* Maths_PointInsidePoly (FUN_00479b80): even-odd test of the 2D point
 * against the edges fromList[i] -> toList[i]. */
int point_in_polygon_2d(const float* point, const float* fromList,
                        const float* toList, unsigned int count);

/* Maths_RaySphereIntersection (FUN_00479c10): 1 when the line from camera
 * along `line` meets the sphere (center, radius). */
int sphere_ray_intersection_discriminant_positive(const float* center,
                                                  float radius,
                                                  const float* camera,
                                                  const float* line);

/* Matrix_Mult (FUN_00479ce0): r = b x a. */
void matrix4_multiply(float* r, const float* a, const float* b);

/* Matrix_RotX, Matrix_RotY, Matrix_RotZ (FUN_00479d50, FUN_00479da0,
 * FUN_00479df0): a rotation by t radians about one axis. */
void matrix4_set_rotation_x(float* m, float t);
void matrix4_set_rotation_y(float* m, float t);
void matrix4_set_rotation_z(float* m, float t);

/* Matrix_Translation (FUN_00479e40): identity with row 3 = t. */
void matrix4_identity_translation_set(float* m, const float* t);

/* Matrix_Identity (FUN_00479e70). */
void matrix4_set_identity(float* m);

/* Matrix_Zero (FUN_00479ea0). */
void matrix4_set_zero(float* m);

/* Matrix_Copy (FUN_00479eb0): r = m. */
void matrix4_copy(float* r, const float* m);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MATHS_H */
