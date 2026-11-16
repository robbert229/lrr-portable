// SPDX-License-Identifier: LGPL-3.0-or-later
/* Maths.h's inline vector functions, under GAME-001's names, as the camera
 * unit (camera_control.c) sees them in the MSVC 5.0 matching build
 * (finding 692). Only camera_control.c includes this file; in a header they
 * add no annotation-lint debt, as advisor_maths.h (finding 544).
 *
 * The unit is compiled without optimisation (/Od, which implies /Ob0), so
 * MSVC 5.0 expands none of them: the camera's code calls each one, and the
 * object carries its own copy as a pick-any COMDAT (`inline`). LINK
 * keeps the first copy it meets, Advisor.c's (004013a0-00401750, defined
 * there the same way), and discards these. The copies call `sqrt` out of
 * line, so the object references `_sqrt` ahead of camera_update's `_fabs`.
 * That reference is what pulls LIBC's sqrt.obj (`__CIsqrt`, `_sqrt`,
 * 0048e5c0-0048e680) between crt0dat.obj and fabs.obj in the original,
 * although nothing in its image calls it. */
#ifndef LEGORRR_GAME_CAMERA_MATHS_H
#define LEGORRR_GAME_CAMERA_MATHS_H

inline float vec_len(const float* a)
{
    return (float)sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

inline float vec_dist(const float* a, const float* b)
{
    return (float)sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

inline float* vec_add(float* dst, const float* a, const float* b)
{
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
    return dst;
}

inline float* vec_sub(float* dst, const float* a, const float* b)
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
    return dst;
}

inline float vec_dot(const float* a, const float* b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline float* vec_scale(float* dst, const float* src, float s)
{
    dst[0] = src[0] * s;
    dst[1] = src[1] * s;
    dst[2] = src[2] * s;
    return dst;
}

inline void vec_normalize(float* v)
{
    vec_scale(v, v, 1.0f / vec_len(v));
}

#endif /* LEGORRR_GAME_CAMERA_MATHS_H */
