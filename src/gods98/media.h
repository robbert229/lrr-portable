// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Pointer.c entries declared in media.hpp.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_MEDIA_H
#define LEGORRR_GODS98_MEDIA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void media_pointer_cursor_select(int slot);                /* 0045cc20 */
void media_pointer_cursor_override_tick(float delta);      /* 0045cd90 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MEDIA_H */
