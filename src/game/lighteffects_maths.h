// SPDX-License-Identifier: LGPL-3.0-or-later
/* LightEffects.c's inline vector helper (gods98 Maths.h's
 * Maths_Vector3DSubtract) over LightEffects_Vector3F. The original unit
 * expands it in place, so it has no address of its own. Only lighteffects.c
 * includes this file, after LightEffects_Vector3F.
 *
 * Through it light_effects_check_move_limit's difference vector is
 * address-taken, which keeps |x| on the x87 stack past the first test as in
 * the original. It is a static __inline function, not a macro: as a macro
 * the expansion compiles differently (finding 544). In a header it adds no
 * Reccmp lint debt. */
#ifndef LEGORRR_GAME_LIGHTEFFECTS_MATHS_H
#define LEGORRR_GAME_LIGHTEFFECTS_MATHS_H

static __inline LightEffects_Vector3F* light_effects_vector3d_subtract(LightEffects_Vector3F* r,
                                                                       const LightEffects_Vector3F* a,
                                                                       const LightEffects_Vector3F* b)
{
    r->x = (a->x - b->x);
    r->y = (a->y - b->y);
    r->z = (a->z - b->z);
    return r;
}

#endif /* LEGORRR_GAME_LIGHTEFFECTS_MATHS_H */
