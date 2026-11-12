// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Input.c's key maps (input_shared.hpp, input.cpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_INPUT_KEYS_H
#define LEGORRR_GODS98_INPUT_KEYS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t s_global_current_keys[256];  /* DAT_0076ab80 */
extern uint8_t s_global_previous_keys[256]; /* DAT_0076aa00 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_INPUT_KEYS_H */
