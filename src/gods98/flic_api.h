// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Flic.c's Flic_Close (flic.hpp). gods98/flic.h holds Panel.c's adapters.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declaration it pairs with has C linkage in place. */
#ifndef LEGORRR_GODS98_FLIC_API_H
#define LEGORRR_GODS98_FLIC_API_H

#ifdef __cplusplus
extern "C" {
#endif

struct Flic;
int flic_close(struct Flic* f);                             /* 00483f00 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FLIC_API_H */
