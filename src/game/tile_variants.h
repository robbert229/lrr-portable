// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-011 (tile_variants.cpp): the texture-set lookup for units rebuilt as
 * C.
 *
 * C boundary header (ADR 0005). Shared C++ headers do not include this file
 * (finding 506); test_c_boundaries.cpp checks it against gods98/record.hpp,
 * which also declares the function. */
#ifndef LEGORRR_GAME_TILE_VARIANTS_H
#define LEGORRR_GAME_TILE_VARIANTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Detail_GetTexture (FUN_0040b850): the texture of `packed_cell` in the
 * texture set `grid`. */
unsigned int packed_grid_cell_value_get(const void* grid,
                                        unsigned int packed_cell);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TILE_VARIANTS_H */
