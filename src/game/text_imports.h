// SPDX-License-Identifier: LGPL-3.0-or-later
/* Text.c (GAME-045) imports: C declarations of the callees and globals the
 * unit uses from other modules (ADR 0005 step 6; finding 607). Included only
 * by text.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, by the name it has in
 * the matching build. */
#ifndef LEGORRR_GAME_TEXT_IMPORTS_H
#define LEGORRR_GAME_TEXT_IMPORTS_H

#include <stdint.h>

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LevelState;
struct TextureSurface;

/* Lego.c (lego_unit.c; terrain_grid.cpp natively): legoGlobs.currLevel
 * (005560d4) and legoGlobs.flags1 (00556ecc). */
extern struct LevelState* g_active_level_retail;

/* Interface.c (interface_unit.c): Interface_GetFlashingState. */
uint32_t sim_status_banner_word_get(void);                         /* 0041b9b0 */

/* NERPs (tutorial_script.c): NERPFunc_GetTutorialFlags,
 * NERPs_GetMessagePermit and NERPs_ParseMessage. */
int nerpfunc_get_tutorial_flags(unsigned int* stack);              /* 00456240 */
int nerps_get_message_permit(void);                                /* 00456640 */
void nerps_parse_message(char* text, char* output, int setTimer);  /* 00456830 */

/* Panel.c (panel_unit.c): Panel_SetCurrentPosition, Panel_Update,
 * Panel_AirMeter_DrawJuice and Panel_AirMeter_DrawOxygenLow. */
void indexed_pair_set(int panel, float x, float y);                /* 0045a6e0 */
void simtick_manager_tick(int panel, float elapsed);               /* 0045a730 */
void advisor_progress_bar_fill_draw(int panel, float air);         /* 0045c3f0 */
void panel_resource_position_update(int panel);                    /* 0045c4b0 */

/* SFX.c (sfx.c): SFX_GetType and SFX_Random_SetAndPlayGlobalSample, by the
 * port's names that its definitions take in the matching build
 * (sfx_link_names.h). */
int sound_alias_lookup(const char* name, int* out_index);          /* 00464c70 */
int sound_submit_with_global_cooldown(int alias, int* out_handle); /* 00464ef0 */

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_surface_release Image_Remove /* 0047d470 */
#define image_surface_blit_transformed Image_SetupTrans /* 0047dbf0 */
#endif
/* Images.c (images_unit.cpp): Image_Remove and Image_SetupTrans.
 * Image_LoadBMPScaled (0047d9d0) and Image_DisplayScaled (0047dcb0) are
 * images.h's adapters. */
void image_surface_release(void* record);                          /* 0047d470 */
void image_surface_blit_transformed(struct TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue); /* 0047dbf0 */

/* TextWindow.c (textwindow.c), by the port's names that its definitions
 * take in the matching build (textwindow_link_names.h). */
void float_pair_at_offset4_set_from_ints(void* record, int x, int y); /* 004851b0 */
void float_pair_at_offsetc_set_from_uints(void* record, uint32_t x,
                                          uint32_t y);             /* 004851d0 */
void objective_advisor_text_copy(void* record, int mode,
                                 const char* format, ...);         /* 00485300 */
int text_buffer_draw_wrapped(void* record, uint32_t start_line,
                             float line_scale, int* height_out);   /* 00485390 */
void text_buffer_reset_target(void* record);                       /* 004859b0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TEXT_IMPORTS_H */
