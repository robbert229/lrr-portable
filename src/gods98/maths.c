// SPDX-License-Identifier: LGPL-3.0-or-later
/* Maths.c — original translation unit G98-007 (text 00479500-00479ee0,
 * .rdata 0049e910-0049e93c; finding 505).
 *
 * Rebuilt in the original's shape (issue #395, ADR 0005): one file for the
 * unit, its 20 functions in original order and the C-compatible subset, so
 * the matching build compiles it as C. The native port compiles it as C++.
 * The unit has no .bss or .data.
 *
 * The public API keeps the port's names and float-pointer parameters, which
 * have the original's ABI: a vector is three adjacent floats, a 2D point two
 * and a matrix sixteen, row-major. Inside the unit, Vector3F gives the
 * original's struct copies (integer moves) and the Maths.h inline helpers
 * their shape. */
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "maths.h"
#include "crt/vc5names.h"

typedef struct Vector3F {
    float x, y, z;
} Vector3F;

/* D3DRM.DLL's vector helpers (d3drmdef.h). The Windows builds import them;
 * the native port emulates them in view.cpp. */
#if defined(_WIN32)
#define MATHS_D3DRMAPI __stdcall
#else
#define MATHS_D3DRMAPI
#endif
#ifdef __cplusplus
extern "C" {
#endif
float* MATHS_D3DRMAPI D3DRMVectorRandom(float* d);
float* MATHS_D3DRMAPI D3DRMVectorRotate(float* r, float* v, float* axis,
                                        float theta);
#ifdef __cplusplus
}
#endif

/* The unit's .rdata, in original order. It opens with the 16-byte
 * {1, 2, 4, -1} table at 0049e910 that 19 gods98 objects share (finding
 * 505). The table is d3drmdef.h's (finding 524), but the unit does not
 * include the toolchain's DirectX 3 d3drm.h: it moves the symbol counter so
 * that two exact functions fall (finding 646). Zero and -1.0 stores are immediates in the
 * original, so they use literals. */
// GLOBAL: LEGORR 0x0049e920
static const float s_maths_one = 1.0f;
// GLOBAL: LEGORR 0x0049e924
static const float s_maths_zero = 0.0f;
// GLOBAL: LEGORR 0x0049e928
static const float s_maths_minus_one = -1.0f;
// GLOBAL: LEGORR 0x0049e92c
static const float s_maths_half = 0.5f;
// GLOBAL: LEGORR 0x0049e930
static const float s_maths_rand_scale = 1.0f / 32767.0f;
/* Maths_RaySphereIntersection's 2.0f. MSVC folds each 2.0f * x into
 * fadd st, st, so no instruction reads it; the literal is in that function
 * and this copy keeps the table's layout. */
// GLOBAL: LEGORR 0x0049e934
static const float s_maths_two = 2.0f;
// GLOBAL: LEGORR 0x0049e938
static const float s_maths_four = 4.0f;

/* Maths.h's inline vector helpers (Maths_Vector3DModulus, _Scale and
 * _Normalise). Like the original's, they are always inlined. */
static __inline float maths_vector3d_modulus(const Vector3F* v)
{
    return (float)sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

static __inline Vector3F* maths_vector3d_scale(Vector3F* r, const Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline Vector3F* maths_vector3d_normalise(Vector3F* r)
{
    return maths_vector3d_scale(r, r, s_maths_one / maths_vector3d_modulus(r));
}

// FUNCTION: LEGORR 0x00479500
float* d3drm_vector_random(float* r)
{
    return D3DRMVectorRandom(r);
}

// FUNCTION: LEGORR 0x00479510
float* d3drm_vector_rotate(float* r, float* v, float* axis, float theta)
{
    return D3DRMVectorRotate(r, v, axis, theta);
}

// FUNCTION: LEGORR 0x00479530
float* surface_triangle_normal_calculate(float* n, const float* p1,
                                         const float* p2, const float* p3)
{
    Vector3F v1, v2;
    float x, y, z, len;

    v1.x = p2[0] - p1[0];
    v1.y = p2[1] - p1[1];
    v1.z = p2[2] - p1[2];
    v2.x = p3[0] - p2[0];
    v2.y = p3[1] - p2[1];
    v2.z = p3[2] - p2[2];

    x = v1.y * v2.z - v1.z * v2.y;
    y = v1.z * v2.x - v1.x * v2.z;
    z = v1.x * v2.y - v1.y * v2.x;

    len = s_maths_one / (float)sqrt(x * x + y * y + z * z);
    n[0] = len * x;
    n[1] = len * y;
    n[2] = len * z;
    return n;
}

/* `longest` is only assigned when an edge is longer than the last; with
 * all three lengths NaN it is read uninitialised, as in the original. */
// FUNCTION: LEGORR 0x00479630
float surface_triangle_area_calculate(const float* p1, const float* p2,
                                      const float* p3, int bfc)
{
    Vector3F v[3];
    Vector3F* pts[6];
    Vector3F n;
    Vector3F edge[3];
    float len[3];
    float max = 0.0f;
    float area = 0.0f;
    float angle;
    unsigned int loop, longest, next;

    pts[0] = &v[0];
    pts[1] = &v[1];
    pts[2] = &v[1];
    pts[3] = &v[2];
    pts[4] = &v[2];
    pts[5] = &v[0];

    v[0] = *(const Vector3F*)p1;
    v[1] = *(const Vector3F*)p2;
    v[2] = *(const Vector3F*)p3;
    v[0].z = 0.0f;
    v[1].z = 0.0f;
    v[2].z = 0.0f;

    if (bfc) surface_triangle_normal_calculate(&n.x, &v[0].x, &v[1].x, &v[2].x);
    else n.z = -1.0f;

    if (n.z < s_maths_zero) {
        for (loop = 0; loop < 3; loop++) {
            edge[loop].x = pts[loop * 2 + 1]->x - pts[loop * 2]->x;
            edge[loop].y = pts[loop * 2 + 1]->y - pts[loop * 2]->y;
            edge[loop].z = pts[loop * 2 + 1]->z - pts[loop * 2]->z;
            len[loop] = maths_vector3d_modulus(&edge[loop]);
            maths_vector3d_scale(&edge[loop], &edge[loop], s_maths_one / len[loop]);
            if (len[loop] > max) {
                max = len[loop];
                longest = loop;
            }
        }

        next = (longest + 1) % 3;
        edge[longest].x *= s_maths_minus_one;
        edge[longest].y *= s_maths_minus_one;
        edge[longest].z *= s_maths_minus_one;

        angle = (float)acos(edge[longest].x * edge[next].x +
                            edge[longest].y * edge[next].y +
                            edge[longest].z * edge[next].z);
        if (_finite(angle)) {
            area = len[longest] * s_maths_half * (float)sin(angle) * len[next];
        }
    }

    return area;
}

/* MSVC compiles this to `jmp _rand`; the caller sign-extends the short.
 * Other builds map rand onto the port's VC5 LCG (crt/vc5names.h). */
// FUNCTION: LEGORR 0x004798a0
short rng_thunk_draw15(void)
{
    return rand();
}

// FUNCTION: LEGORR 0x004798b0
float random_float_range(float low, float high)
{
    float r = (float)rng_thunk_draw15();

    r *= s_maths_rand_scale;
    return low + (r * (high - low));
}

// FUNCTION: LEGORR 0x004798e0
int view_ray_intersection_test(float* endPoint, const float* rayOrigin,
                               const float* ray, const float* planePoint,
                               const float* planeNormal)
{
    float t;
    Vector3F r, n;

    r = *(const Vector3F*)ray;
    n = *(const Vector3F*)planeNormal;
    maths_vector3d_normalise(&r);
    maths_vector3d_normalise(&n);

    if (ray_plane_intersection_parameter(&t, rayOrigin, &r.x, planePoint, &n.x)) {
        point_along_ray(endPoint, rayOrigin, &r.x, t);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00479a30
int ray_plane_intersection_parameter(float* t, const float* rayOrigin,
                                     const float* ray,
                                     const float* planePoint,
                                     const float* planeNormal)
{
    float d;
    Vector3F v;

    if ((d = ray[0] * planeNormal[0] + (ray[1] * planeNormal[1] + ray[2] * planeNormal[2])) != s_maths_zero) {
        v.x = planePoint[0] - rayOrigin[0];
        v.y = planePoint[1] - rayOrigin[1];
        v.z = planePoint[2] - rayOrigin[2];
        *t = (v.x * planeNormal[0] + v.y * planeNormal[1] + v.z * planeNormal[2]) / d;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00479ab0
void point_along_ray(float* endPoint, const float* rayOrigin,
                     const float* ray, float t)
{
    Vector3F v;

    v.x = ray[0] * t;
    v.y = ray[1] * t;
    v.z = ray[2] * t;
    endPoint[0] = rayOrigin[0] + v.x;
    endPoint[1] = rayOrigin[1] + v.y;
    endPoint[2] = rayOrigin[2] + v.z;
}

// FUNCTION: LEGORR 0x00479af0
float* line_intersection_2d_compute(float* r, const float* p1,
                                    const float* d1, const float* p2,
                                    const float* d2)
{
    float ax, ay, bx, by, det, t;

    bx = d2[0];
    by = d2[1];
    ax = -d1[0];
    ay = -d1[1];
    det = ay * bx - ax * by;
    if (s_maths_zero != det) {
        t = -(by / det) * (p1[0] - p2[0]) + (bx / det) * (p1[1] - p2[1]);
        r[0] = t * d1[0];
        r[1] = t * d1[1];
        r[0] = p1[0] + r[0];
        r[1] = r[1] + p1[1];
        return r;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00479b80
int point_in_polygon_2d(const float* point, const float* fromList,
                        const float* toList, unsigned int count)
{
    unsigned int loop, crossings = 0;

    for (loop = 0; loop < count; loop++) {
        if (((fromList[loop * 2 + 1] <= point[1] && toList[loop * 2 + 1] >= point[1]) ||
             (fromList[loop * 2 + 1] >= point[1] && toList[loop * 2 + 1] <= point[1])) &&
            (toList[loop * 2] - fromList[loop * 2]) *
                    ((point[1] - fromList[loop * 2 + 1]) /
                     (toList[loop * 2 + 1] - fromList[loop * 2 + 1])) +
                fromList[loop * 2] > point[0]) {
            crossings++;
        }
    }
    return crossings & 1;
}

// FUNCTION: LEGORR 0x00479c10
int sphere_ray_intersection_discriminant_positive(const float* center,
                                                  float radius,
                                                  const float* camera,
                                                  const float* line)
{
    float lx, ly, lz, b, c;
    Vector3F d;

    d = *(const Vector3F*)line;
    lx = camera[0] - center[0];
    ly = camera[1] - center[1];
    lz = camera[2] - center[2];
    b = 2.0f * d.x * lx + 2.0f * d.y * ly + 2.0f * d.z * lz;
    c = (lx * lx + ly * ly + lz * lz) * s_maths_four - radius * radius;
    if (b * b - c > s_maths_zero) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x00479ce0
void matrix4_multiply(float* r, const float* a, const float* b)
{
    int i, j, k;

    matrix4_set_zero(r);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 4; k++) {
                r[i * 4 + j] += b[i * 4 + k] * a[k * 4 + j];
            }
        }
    }
}

// FUNCTION: LEGORR 0x00479d50
void matrix4_set_rotation_x(float* m, float t)
{
    float c = (float)cos(t), s = (float)sin(t);

    matrix4_set_identity(m);
    m[5] = c;
    m[6] = -s;
    m[10] = c;
    m[9] = s;
}

// FUNCTION: LEGORR 0x00479da0
void matrix4_set_rotation_y(float* m, float t)
{
    float c = (float)cos(t), s = (float)sin(t);

    matrix4_set_identity(m);
    m[0] = c;
    m[8] = -s;
    m[10] = c;
    m[2] = s;
}

// FUNCTION: LEGORR 0x00479df0
void matrix4_set_rotation_z(float* m, float t)
{
    float c = (float)cos(t), s = (float)sin(t);

    matrix4_set_identity(m);
    m[0] = c;
    m[1] = -s;
    m[5] = c;
    m[4] = s;
}

// FUNCTION: LEGORR 0x00479e40
void matrix4_identity_translation_set(float* m, const float* t)
{
    matrix4_set_identity(m);
    m[12] = t[0];
    m[13] = t[1];
    m[14] = t[2];
}

// FUNCTION: LEGORR 0x00479e70
void matrix4_set_identity(float* m)
{
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            m[i * 4 + j] = (i == j) ? s_maths_one : s_maths_zero;
        }
    }
}

// FUNCTION: LEGORR 0x00479ea0
void matrix4_set_zero(float* m)
{
    memset(m, 0, sizeof(float) * 16);
}

// FUNCTION: LEGORR 0x00479eb0
void matrix4_copy(float* r, const float* m)
{
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            r[i * 4 + j] = m[i * 4 + j];
        }
    }
}
