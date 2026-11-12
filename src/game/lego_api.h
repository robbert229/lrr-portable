// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Lego.c entries without a C declaration elsewhere.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_LEGO_API_H
#define LEGORRR_GAME_LEGO_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ResetLevelAndExit (004250a0) in the original ABI; unannotated adapter
 * (engine.cpp). */
void lego_reset_level_and_exit(void);
int ledger_stub_00435550_GlobalStateZeroTransitionDetect(void); /* 00435550 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGO_API_H */
