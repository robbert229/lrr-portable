// SPDX-License-Identifier: LGPL-3.0-or-later
/* The -0.5f and 0.5f that the original's Weapons.c creates before
 * weapons_projectile_create_curved's -1.3f, 0.3f, 0.2f and 1/3
 * (0049e83c-0049e840): an unused inline copy standing for a removed helper.
 *
 * The unit's live readers of the two values (weapons_object_quad_intersect,
 * weapons_object_center_get, weapons_mesh_build_segment) come after the
 * projectile functions, so their code creates the literals too late. A
 * helper that halves an object's size about its centre (`size * -0.5f`,
 * then `size * 0.5f`), compiled before the projectile functions, creates
 * them in the original's order. As for maths_normalise_head.h (finding
 * 693), an `extern __inline` function is compiled although nothing calls
 * it; /OPT:REF then removes the code but not the literals. The helper
 * itself is not evidenced, only its literals and their position. A
 * definition costs two symbols of MSVC's counter at every function after
 * it (finding 538). Only the MSVC 5.0 matching build sees it. */
#ifndef LEGORRR_GAME_WEAPONS_EXTENT_HEAD_H
#define LEGORRR_GAME_WEAPONS_EXTENT_HEAD_H

#if defined(_MSC_VER) && _MSC_VER < 1200
extern __inline void weapons_half_extents(float size, float* lo, float* hi)
{
    *lo = size * -0.5f;
    *hi = size * 0.5f;
}
#endif

#endif /* LEGORRR_GAME_WEAPONS_EXTENT_HEAD_H */
