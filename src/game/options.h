// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * FrontEnd.c's option entries (options.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_OPTIONS_H
#define LEGORRR_GAME_OPTIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void options_write_to_slot_target(void);                    /* 00416910 */
void options_context_six_field_set_target(const uint32_t* src); /* 00417f20 */
void options_context_copy_and_replace_resource_target(const uint32_t* source); /* 004180c0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OPTIONS_H */
