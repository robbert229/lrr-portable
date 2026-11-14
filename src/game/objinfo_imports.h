// SPDX-License-Identifier: LGPL-3.0-or-later
/* ObjInfo.c (GAME-031) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6). Included only by objinfo.c and
 * test_c_boundaries.cpp, so the other C units' symbol numbering does not
 * change (finding 568, "Unit-private imports header"). Each declaration
 * names a definition with C linkage, by the name it has in the matching
 * build. */
#ifndef LEGORRR_GAME_OBJINFO_IMPORTS_H
#define LEGORRR_GAME_OBJINFO_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/draw.h"
#include "gods98/images.h"
#include "legoobject_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct TextureSurface;

/* GAME-009 DamageFont (screen_effect_quads.c):
 * DamageFont_LiveObject_CheckCanShowDamage. */
int damage_font_live_object_check_can_show_damage(struct EntitySlot* liveObj); /* 0040aae0 */

/* Config's "" (004b8a14), which config.c defines (finding 581). */
extern const char s_config_angle_empty[4];

/* gods98 Utils.c (utils.c): Util_Tokenise. */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632). */
#define image_surface_blit_transformed Image_SetupTrans /* 0047dbf0 */
#endif
/* Images.c (images_unit.c): Image_SetupTrans. */
void image_surface_blit_transformed(struct TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue); /* 0047dbf0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OBJINFO_IMPORTS_H */
