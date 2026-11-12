// SPDX-License-Identifier: LGPL-3.0-or-later
/* Message.c (original unit GAME-027) entry points that units rebuilt as C
 * call, with the port's contracts.
 *
 * C boundary header (ADR 0005). The C++ declaration (game/cmd.hpp) carries
 * extern "C"; src/game/test_c_boundaries.cpp includes it with this header.
 * Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GAME_MESSAGE_H
#define LEGORRR_GAME_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00452060 Message_PostEvent. */
void cmd_produce(int op, const void* a1, int a2, const int* pair);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_MESSAGE_H */
