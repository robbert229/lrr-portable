// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for the electric fences (GAME-013,
 * finding 566): the GAME-012 effect entry declared in gods98/resource.hpp.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_ROCK_FALL_STYLES_H
#define LEGORRR_GAME_ROCK_FALL_STYLES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int transient_effect_render_slot_configure(int group, float x, float y,
                                           float size, float heading,
                                           float dx, float dy); /* 0040c2f0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ROCK_FALL_STYLES_H */
