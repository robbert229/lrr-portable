// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-009: the credits loop and the damage font (finding 609).
 *
 * C-compatible header (ADR 0005). screen_effect_quads.c is compiled as C in
 * the matching build and as C++ in the native port. Shared C++ headers do
 * not include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the same functions.
 *
 * In the matching build screen_effect_quads_link_names.h gives five of
 * these the port's names. */
#ifndef LEGORRR_GAME_SCREEN_EFFECT_QUADS_H
#define LEGORRR_GAME_SCREEN_EFFECT_QUADS_H

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct TextureSurface;
struct DamageFont_Data;

/* Credits_Play (0040a0c0). */
void credits_play(const char* textFile, struct TextureSurface* font,
                  const char* aviFile);

/* DamageFont_Cleanup (0040a3d0). */
void damage_font_cleanup(void);

/* DamageFont_LoadFrames (0040a400). */
void damage_font_load_frames(const char* dirName, const char* fileName);

/* DamageFont_DisplayDamage_OverLiveObject (0040a4b0). */
void damage_font_display_damage_over_live_object(struct EntitySlot* liveObj,
                                                 unsigned int damage);

/* DamageFont_GetNextFree (0040a5c0). */
struct DamageFont_Data* damage_font_get_next_free(void);

/* DamageFont_SetDamageNumber (0040a5e0). */
void damage_font_set_damage_number(struct DamageFont_Data* data,
                                   unsigned int damage);

/* DamageFont_UpdateAll (0040aa10). */
void damage_font_update_all(float elapsed);

/* DamageFont_Update (0040aa40). */
void damage_font_update(struct DamageFont_Data* data, float elapsed);

/* DamageFont_LiveObject_CheckCanShowDamage (0040aae0). */
int damage_font_live_object_check_can_show_damage(struct EntitySlot* liveObj);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SCREEN_EFFECT_QUADS_H */
