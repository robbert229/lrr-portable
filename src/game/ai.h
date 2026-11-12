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

/* DAT_004dee14: the object manager's state word. */
extern uint32_t s_entity_state_flags;
/* DAT_004eabc8: LegoObject.c's tool-tip gate. The port has no owner; this
 * port global stands in for it. */
extern int s_lego_object_tool_tip_gate;

/* FUN_0044c550 (GAME-023). */
void global_scatter_spawn_enable_and_drain(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AI_H */
