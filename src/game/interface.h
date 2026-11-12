// SPDX-License-Identifier: LGPL-3.0-or-later
/* Interface.c (original unit GAME-020) entry points that units rebuilt as C
 * call, with the port's contracts.
 *
 * C boundary header (ADR 0005). The C++ declarations (game/frontend.hpp,
 * game/cmd.hpp) carry extern "C"; src/game/test_c_boundaries.cpp includes
 * them with this header. Shared C++ headers do not include this file
 * (finding 506). */
#ifndef LEGORRR_GAME_INTERFACE_H
#define LEGORRR_GAME_INTERFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_0041b9b0: the blinking phase of flashing buttons and meters. */
uint32_t sim_status_banner_word_get(void);
/* FUN_0041b2d0. */
void command_context_post_to_hud(int kind, const uint32_t coord[2]);
/* FUN_0041e750. */
void selection_clear_request_and_hud_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_INTERFACE_H */
