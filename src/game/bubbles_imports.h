// SPDX-License-Identifier: LGPL-3.0-or-later
/* Bubbles.c (GAME-005) imports: C declarations of the callees and globals
 * the unit uses from other modules (ADR 0005 step 6; finding 598). Included
 * only by bubbles.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, by the name it has
 * in the matching build. */
#ifndef LEGORRR_GAME_BUBBLES_IMPORTS_H
#define LEGORRR_GAME_BUBBLES_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/images.h"
#include "gods98/viewports.h"
#include "legoobject_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct TextureSurface;

/* Lego.c (lego_unit.c). legoGlobs.gameName (005560c4) and
 * legoGlobs.viewMain (005560dc), and the check at 00425d80. */
extern const char* s_engine_config_root;   /* 005560c4 */
extern void* legoGlobs_viewMain;           /* 005560dc */
int current_engine_mode_is_zero(void);     /* 00425d80 */

/* Main.c's flags (00505f84). */
extern uint32_t s_engine_feature_word;

/* LegoObject.c (legoobject_unit.c): LegoObject_GetActivityContainer. */
void* legoobject_type_component_get(Lego_Object* obj);          /* 0044a200 */

/* Container.c (container_unit.cpp): Container_GetPosition. */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);            /* 004754c0 */

/* Stats.c (entity_stats.c), by its link names:
 * StatsObject_GetCollHeight and StatsObject_GetStatsFlags3. */
float entity_stats_float_at74(const struct EntitySlot* e);      /* 00469d50 */
uint32_t entity_capability_flags_get(const struct EntitySlot* e); /* 00469ea0 */

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_surface_blit_transformed Image_SetupTrans /* 0047dbf0 */
#endif
/* Images.c (images_unit.cpp): Image_SetupTrans. */
void image_surface_blit_transformed(struct TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue); /* 0047dbf0 */

/* ObjInfo.c (object_overlays.cpp): ObjInfo_DrawHealthBar and
 * ObjInfo_DrawBubbleImage, with the original's arguments. The port's
 * ObjInfo_DrawHungerImage (00459f50) reads a float the caller points at,
 * not the object, so the unit calls the PORT-ONLY adapter at the end of
 * object_overlays.cpp, which passes the object's energy (+0x344). */
void legacy_entity_screen_ring_render(void* entity, int x, int y); /* 00459b00 */
void objinfo_draw_hunger_image_adapter(void* liveObj, int x, int y); /* 00459f50 */
void panel_resource_position_apply_if_enabled(void* resource, int x,
                                              int y);           /* 00459fd0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_BUBBLES_IMPORTS_H */
