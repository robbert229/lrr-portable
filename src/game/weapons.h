// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for the electric fences (GAME-013,
 * finding 566): Weapons.c (GAME-050) entries. weapon.hpp and weapons.cpp declare
 * them inside namespace lego and namespace unported_ledger_stubs, with C
 * linkage, so they name the same functions.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_WEAPONS_H
#define LEGORRR_GAME_WEAPONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;

uint32_t object_type_name_index_find(const char* name);       /* 0046f0d0 */
float weapon_type_recharge_time_get_target(int type);         /* 0046f110 */
void ledger_stub_0046f270_TypeMotionScaleApplyDispatch(
    struct EntitySlot* entity, int weapon_type, int apply_index, float scale,
    void* finish_argument);                                   /* 0046f270 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_WEAPONS_H */
