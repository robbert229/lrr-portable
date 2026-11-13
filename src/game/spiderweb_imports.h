// SPDX-License-Identifier: LGPL-3.0-or-later
/* SpiderWeb.c (GAME-042) imports: C declarations of the callees the unit
 * uses from other modules (ADR 0005 step 6; finding 620). Included only by
 * spiderweb.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, with the parameter
 * types the other C units' headers give it. LegoObject.c's callees are in
 * legoobject_unit.h, and Map3D_BlockToWorldPos in terrain_map.h. */
#ifndef LEGORRR_GAME_SPIDERWEB_IMPORTS_H
#define LEGORRR_GAME_SPIDERWEB_IMPORTS_H

#include <stdint.h>

#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GAME-050 Weapons.c (weapons.c). */
uint32_t object_type_name_index_find(const char* name);           /* 0046f0d0 */
float weapons_get_damage_for_object(int weapon_type,
                                    const void* entity_record);   /* 0046f1d0 */
int weapons_object_collision_check(void* obj, float* fromPos,
                                   float* toPos, int checkSide);  /* 00470260 */
/* G98-002 Container.c: Container_SetActivity and
 * Container_SetAnimationTime (gods98/container.h, gods98/view.h). */
unsigned int scene_action_name_set(void* scene_object,
                                   const char* action_name);      /* 00473370 */
float ddiscene_frame_update_and_callbacks(void* scene_object,
                                          float time);            /* 00475140 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SPIDERWEB_IMPORTS_H */
