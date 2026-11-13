// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-049 Water.c's inline vector helper (finding 614), as the other units'
 * copies of gods98 Maths.h's helpers (finding 609's
 * screen_effect_quads_maths.h, weapons_maths.h). The original expands it in
 * place in Water_UpdateFlowBlocks (0046e610), so it has no address of its
 * own: its pointer arguments are why the rebuild keeps the result in memory
 * and the distance on the x87 stack, as the original does. Only water.c
 * includes this file, inside its extern "C" block, after terrain_map.h.
 *
 * A static __inline function, not a macro: as a macro the expansion
 * compiles differently (finding 544). In a header it adds no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_WATER_MATHS_H
#define LEGORRR_GAME_WATER_MATHS_H

/* The point dist past the middle of the block edge from a to b, along dir. */
static __inline TerrainMapVector3* water_flow_point(TerrainMapVector3* r,
                                                    const TerrainMapVector3* a,
                                                    const TerrainMapVector3* b,
                                                    const TerrainMapVector3* dir,
                                                    float dist)
{
    r->x = ((a->x + b->x) * 0.5f) + (dir->x * dist);
    r->y = ((a->y + b->y) * 0.5f) + (dir->y * dist);
    r->z = ((a->z + b->z) * 0.5f) + (dir->z * dist);
    return r;
}

#endif /* LEGORRR_GAME_WATER_MATHS_H */
