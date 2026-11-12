// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * ToolTip.c entries (effects.hpp, tooltip.cpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_EFFECTS_H
#define LEGORRR_GAME_EFFECTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void timed_effect_runtime_update(uint32_t render_x, uint32_t render_y,
                                 float delta);              /* 0046b7c0 */
#include "gods98/images.h"

/* gods98's ColourRGBF. */
typedef struct ColourRGBF {
    float red;
    float green;
    float blue;
} ColourRGBF;

/* ToolTip_DrawBox (0046b8b0) in the original ABI; unannotated adapter. */
void tooltip_draw_box(Area2F rect, ColourRGBF fill, ColourRGBF highlight,
                      ColourRGBF shadow, int dark);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_EFFECTS_H */
