// SPDX-License-Identifier: LGPL-3.0-or-later
/* Erode.c (GAME-015) imports: C declarations of the callees and globals the
 * unit uses from other modules (ADR 0005 step 6; finding 599). Included only
 * by erode.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, with the parameter
 * types the other C units' imports headers give it. */
#ifndef LEGORRR_GAME_ERODE_IMPORTS_H
#define LEGORRR_GAME_ERODE_IMPORTS_H

#include "gods98/maths.h"
#include "info_unit.h"
#include "lego_unit.h"
#include "terrain_grid.h"
#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GAME-012 the effects unit: Effect_Spawn_Particle (0040cce0), by the
 * port's name, which rock_fall_styles.c's definition takes in the matching
 * build. */
int lava_effect_select_and_place(int slot, const float* position,
                                 const float* rotation);           /* 0040cce0 */
/* GAME-021 Lego.c (lego_unit.c; lego.cpp's models natively). */
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);     /* 00433090 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ERODE_IMPORTS_H */
