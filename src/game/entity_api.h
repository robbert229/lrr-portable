// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Lego.c and LegoObject.c entries (entity.hpp, entity_for_reward.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_ENTITY_API_H
#define LEGORRR_GAME_ENTITY_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
int entity_type_pair_lookup(const char* name, int* type_out, int* row_out,
                            int* zero_out);                 /* 0042e8b0 */
int entity_pool_find_by_callback_filtered_current(
    int (*callback)(struct EntitySlot*, void*), void* payload); /* 004377b0 */
const char* item_name_by_kind_lookup(uint32_t kind, int index); /* 004396c0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENTITY_API_H */
