// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-001's copies of gods98 Maths.h's inline modulus and scale, which the
 * original expands inside Maths_Vector3DNormalise (vec_normalize). They have
 * no address of their own. Only advisor.c includes this file, just before
 * vec_normalize; in a header they add no annotation-lint debt, as
 * steering_maths.h (finding 544). The out-of-line copies that other units
 * call (vec_len, vec_scale) are advisor.c's own functions. */
#ifndef LEGORRR_GAME_ADVISOR_MATHS_H
#define LEGORRR_GAME_ADVISOR_MATHS_H

static __inline float advisor_vector3d_modulus(const float* v)
{
    return (float)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static __inline float* advisor_vector3d_scale(float* r, const float* a, float f)
{
    r[0] = a[0] * f;
    r[1] = a[1] * f;
    r[2] = a[2] * f;
    return r;
}

#endif /* LEGORRR_GAME_ADVISOR_MATHS_H */
