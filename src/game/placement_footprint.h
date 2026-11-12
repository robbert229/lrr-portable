// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for the electric fences (GAME-013,
 * finding 566): the GAME-040 shape rotation declared in gods98/view.hpp.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_PLACEMENT_FOOTPRINT_H
#define LEGORRR_GAME_PLACEMENT_FOOTPRINT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation); /* 00464110 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_PLACEMENT_FOOTPRINT_H */
