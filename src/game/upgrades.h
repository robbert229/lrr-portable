// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * FrontEnd.c's reward average (upgrades.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_UPGRADES_H
#define LEGORRR_GAME_UPGRADES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int upgrade_average_is_above_99(void);                      /* 00418120 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_UPGRADES_H */
