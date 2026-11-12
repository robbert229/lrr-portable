// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for the electric fences (GAME-013,
 * finding 566): Stats.c (GAME-043) getters declared in entity.hpp.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_STATS_H
#define LEGORRR_GAME_STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;

float entity_stats_float_at74(const struct EntitySlot* e);   /* 00469d50 */
uint32_t entity_state_flags_get(const struct EntitySlot* e); /* 00469e80 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_STATS_H */
