// SPDX-License-Identifier: LGPL-3.0-or-later
/* LightEffects.c (GAME-024) imports: C declarations of the callees the unit
 * uses from other modules (ADR 0005 step 6; finding 594). Included only by
 * lighteffects.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, with the port's
 * parameter types. */
#ifndef LEGORRR_GAME_LIGHTEFFECTS_IMPORTS_H
#define LEGORRR_GAME_LIGHTEFFECTS_IMPORTS_H

#include "gods98/config.h"
#include "gods98/maths.h"
#include "gods98/utils.h"
#include "gods98/view.h"

#ifdef __cplusplus
extern "C" {
#endif

/* G98-002 Container.c (container_unit.cpp). */
void render_object_set_packed_rgba_from_float_channels(
    void* scene_object, float red, float green, float blue,
    float alpha);                                                 /* 004750b0 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);              /* 004754c0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LIGHTEFFECTS_IMPORTS_H */
