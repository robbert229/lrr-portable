// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for the electric fences (GAME-013,
 * finding 566): the port's name of Building_GetShapePoints (GAME-006),
 * declared in gods98/record.hpp. In the matching build Building.c defines
 * it (building_model_link_names.h, finding 592); natively the port tail
 * building_model_port.cpp does. Only C units and
 * src/game/test_c_boundaries.cpp include it; the C++ declaration it pairs
 * with has C linkage in place. The unit's own API is building_model.h. */
#ifndef LEGORRR_GAME_BUILDING_MODEL_PORTS_H
#define LEGORRR_GAME_BUILDING_MODEL_PORTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t task_field84_get_and_field88_out(const void* task, uint32_t* out88); /* 00408360 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_BUILDING_MODEL_PORTS_H */
