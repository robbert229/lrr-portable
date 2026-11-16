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
/* Matching build: the flic's width and height through the original's
 * getters (#805, finding 674): 004850e0 is Flic_GetHeight under its link
 * name (flic_link_names.h); Flic_GetWidth was folded into 004897d0, which
 * this tree names animclone.c's anim_clone_is_lws (finding 632).
 * Deviation (scoring only): the folded getter's name. Each renames
 * flic.h's declaration and its uses; neither name is otherwise declared
 * here (finding 538). */
#define flic_width anim_clone_is_lws   /* 004897d0 */
#define flic_height object_field10_get /* 004850e0 */
#include "gods98/flic.h"
#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

/* Main.c's flags (00505f84) and Lego.c's legoGlobs.flags1 (00556ecc). */
extern uint32_t s_engine_feature_word;

/* Utils.c (utils.c): Util_Tokenise. */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */

/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_surface_blit_transformed Image_SetupTrans /* 0047dbf0 */
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
