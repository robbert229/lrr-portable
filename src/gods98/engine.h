// SPDX-License-Identifier: LGPL-3.0-or-later
/* Main.c's globals that units rebuilt as C read.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of globals
 * the port keeps in gods98/engine.cpp. The C++ code keeps using
 * gods98/engine.hpp and engine_shared.hpp, whose matching declarations carry
 * extern "C"; src/game/test_c_boundaries.cpp includes them with this
 * header. Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GODS98_ENGINE_H
#define LEGORRR_GODS98_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DAT_00505f88: Main.c's programmer mode. */
extern int32_t s_application_mode;
/* DAT_00556ec8, a common symbol: nonzero once the game is running. */
extern int32_t s_context_mode;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_ENGINE_H */
