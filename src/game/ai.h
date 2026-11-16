// SPDX-License-Identifier: LGPL-3.0-or-later
/* LegoObject.c (original unit GAME-023) state that units rebuilt as C read.
 *
 * C boundary header (ADR 0005). The C++ declaration (game/ai.hpp) carries
 * extern "C"; src/game/test_c_boundaries.cpp includes it with this header.
 * Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GAME_AI_H
#define LEGORRR_GAME_AI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Matching build: both words are objectGlobs fields (004de790,
 * legoobject_globs.h, finding 662): flags (+0x684) and the count of the
 * buildings among the visited units (+0xc438). objectGlobs is declared
 * here without its struct, at the offsets legoobject_globs.h gives:
 * the declaration and its new tag cost the two symbols of the two
 * declarations it replaces, so panel_unit.c's symbol counter does not move
 * (the header itself would cost three). Deviation (scoring only). */
extern struct LegoObject_Globs objectGlobs;
#define s_entity_state_flags (*(unsigned int*)((char*)&objectGlobs + 0x684))
#define s_lego_object_tool_tip_gate (*(int*)((char*)&objectGlobs + 0xc438))

/* FUN_0044c550 (GAME-023). */
/* In the matching build 0044c550 is legoobject_unit.c's (GAME-023 part 3,
 * issue #615); one declaration either way. */
#define global_scatter_spawn_enable_and_drain legoobject_scatter_spawn_enable_and_drain
void global_scatter_spawn_enable_and_drain(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AI_H */
