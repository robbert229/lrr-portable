// SPDX-License-Identifier: LGPL-3.0-or-later
/* The tutorial script (original unit GAME-028, game/tutorial_script.cpp)
 * entry points that units rebuilt as C call, with the port's contracts.
 *
 * C boundary header (ADR 0005). The C++ declaration (gods98/message.hpp)
 * carries extern "C"; src/game/test_c_boundaries.cpp includes it with this
 * header. Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GAME_TUTORIAL_SCRIPT_H
#define LEGORRR_GAME_TUTORIAL_SCRIPT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00456240: the tutorial's interface-block flags. The original takes
 * one argument; the port's entry takes none. */
uint32_t system_state_flags_get(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TUTORIAL_SCRIPT_H */
