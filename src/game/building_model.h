// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for the electric fences (GAME-013,
 * finding 566): the GAME-006 shape getter declared in gods98/record.hpp.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_BUILDING_MODEL_H
#define LEGORRR_GAME_BUILDING_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t task_field84_get_and_field88_out(const void* task, uint32_t* out88); /* 00408360 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_BUILDING_MODEL_H */
