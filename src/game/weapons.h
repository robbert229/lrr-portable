// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) of Weapons.c (GAME-050, weapons.c;
 * finding 582) for the C units that call it: the electric fences (GAME-013,
 * finding 566). The first two are weapons.c's and serve native callers too;
 * weapon.hpp declares them in namespace lego, with C linkage. The third is
 * the port's model of 0046f270 in the port tail weapons_port.cpp, also with
 * C linkage; in the matching build the macro below points it at weapons.c's
 * weapons_generic_damage_object (as weapons_port_aliases.h does for other
 * units). The macro is here rather than in an included alias header because
 * a new #include alone moves terrain_cell_objects.c's symbol counter
 * (finding 538) and costs 0040cdc0 its exact match; weapons.c defines
 * LEGORRR_GAME_WEAPONS_UNIT to keep its own name.
 * Its declarations are kept as they were: terrain_cell_objects.c includes it,
 * and each added declaration would shift that unit's symbol numbering
 * (finding 538). weapons.c declares its own records and functions.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_WEAPONS_H
#define LEGORRR_GAME_WEAPONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(LEGORRR_GAME_WEAPONS_UNIT)
#define ledger_stub_0046f270_TypeMotionScaleApplyDispatch weapons_generic_damage_object
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
