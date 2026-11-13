// SPDX-License-Identifier: LGPL-3.0-or-later
/* Pointer.c (GAME-033) imports: C declarations of the callees and globals
 * the unit uses from other modules (ADR 0005 step 6; finding 612). Included
 * only by pointer.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, by the name it has in
 * the matching build. */
#ifndef LEGORRR_GAME_POINTER_IMPORTS_H
#define LEGORRR_GAME_POINTER_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/flic.h"
#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

/* Main.c's flags (00505f84) and Lego.c's legoGlobs.flags1 (00556ecc). */
extern uint32_t s_engine_feature_word;
extern uint32_t legoGlobs_flags1;

/* Utils.c (utils.c): Util_Tokenise. */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */

/* Images.c (images_unit.cpp): Image_SetupTrans. Image_LoadBMPScaled
 * (0047d9d0) and Image_DisplayScaled (0047dcb0) are images.h's adapters;
 * Flic_Setup (00483c80), Flic_Animate (00484070) and the flic's width and
 * height (004897d0, 004850e0) are flic.h's. */
void image_surface_blit_transformed(struct TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue); /* 0047dbf0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_POINTER_IMPORTS_H */
