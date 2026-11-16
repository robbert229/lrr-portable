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

/* Both words are objectGlobs fields (legoobject_globs.h, finding 662):
 * flags and the count of the buildings among the visited units. */
#include "legoobject_globs.h"
#define s_lego_object_tool_tip_gate objectGlobs.field_c438

/* FUN_0044c550 (GAME-023). */
/* In the matching build 0044c550 is legoobject_unit.c's (GAME-023 part 3,
 * issue #615); one declaration either way. */
#define global_scatter_spawn_enable_and_drain legoobject_scatter_spawn_enable_and_drain
void global_scatter_spawn_enable_and_drain(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AI_H */
