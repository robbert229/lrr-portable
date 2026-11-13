// SPDX-License-Identifier: LGPL-3.0-or-later
/* Message.c (GAME-027) imports: C declarations of the callees and globals
 * the unit uses from other modules (ADR 0005 step 6). Included only by
 * message_unit.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names the definition the matching build links, with C
 * linkage; message_unit_link_names.h maps the callees' original-shape names
 * to it there. */
#ifndef LEGORRR_GAME_MESSAGE_UNIT_IMPORTS_H
#define LEGORRR_GAME_MESSAGE_UNIT_IMPORTS_H

#include <stdint.h>

#include "lego_unit.h"
#include "legoobject_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct ElectricFenceObject;
struct Container;
struct StatsObject;

/* gods98 Input.c's key map (input). */
extern uint8_t s_global_current_keys[256];  /* 0076ab80 Input_KeyMap */

/* GAME-021 Lego.c's legoGlobs fields (lego_unit.c stand-ins and the
 * port's globals). */
extern void* s_zoom_step_context;           /* 005560e4 cameraMain */
/* lego_unit.h has legoGlobs_cameraTrack (005560e8), legoGlobs_selectedObject
 * (00556160 objectFP), legoGlobs_f_5563f8 and legoGlobs_flags1 (00556ecc). */

/* GAME-002 AITask.c (aitask.c). */
void aitask_cmd_create_task(struct EntitySlot* object, float timeIn); /* 00402720 */
void aitask_selection_drag_area_command_create(struct EntitySlot** objects,
                                               unsigned int count,
                                               const int* blockPos,
                                               int param);        /* 00403270 */
void aitask_cmd_create_move(struct EntitySlot* object, const int* blockPos,
                            int param);                           /* 004035c0 */
void aitask_cmd_create_gated(struct EntitySlot* object,
                             struct EntitySlot* target);          /* 00403610 */
void aitask_cmd_create_target(struct EntitySlot* object,
                              struct EntitySlot* target);         /* 004036c0 */
void aitask_entity_activity_records_refresh(struct EntitySlot* object); /* 00403b60 */
/* GAME-008 (entity.cpp). */
void entity_destroy_tracked_pointer_cleanup(void* entity);        /* 00409ff0 */
/* GAME-013 (terrain_cell_objects.c). */
void electric_fence_remove(struct ElectricFenceObject* object);   /* 0040d0a0 */
/* GAME-020 Interface.c (interface_unit.c). */
void command_context_post_to_hud(int menu, const uint32_t blockPos[2]); /* 0041b2d0 */
/* GAME-021 Lego.c (lego_unit.c). */
void lego_render_record_set_from_input(struct EntitySlot* object); /* 00425d50 */
void lego_sound_minimum_distance_update_mode(int viewMode, void* liveObj,
                                             int cameraFrame);    /* 004295f0 */
void* level_current_terrain_get(int cell_x, int cell_y);          /* 00431570 */
void lego_terrain_stamp_impact_handle(unsigned int bx, unsigned int by,
                                      int direction, int vertexMode); /* 00431780 */
uint32_t terrain_cell_flag_8_bounded_current(uint32_t cx, uint32_t cy); /* 00432b50 */
/* GAME-023 LegoObject.c (legoobject_unit.c); legoobject_unit.h, included
 * above, declares the others. */
void* legoobject_type_component_get(Lego_Object* obj);            /* 0044a200 */
int legoobject_conditional_type_five_spawn(Lego_Object* obj);     /* 004470d0 */
int legoobject_adjacent_placement_cell_search(Lego_Object* obj,
                                              Lego_Object* target); /* 00447210 */
void legoobject_selection_clear_marker_and_reset_hud(Lego_Object* obj); /* 0044c4a0 */
int legoobject_selection_clear_callback(Lego_Object* obj, void* data); /* 0044c500 */
/* GAME-035 (level_id_remap.cpp). */
void command_entity_reference_remap(uint32_t* entity_reference);  /* 0045d870 */
/* GAME-041 SFX.c (sfx.c). */
int sfx_random_play_sound_normal(int sfxID, int loop);            /* 00464fa0 */
int sfx_random_play_sound3d_on_container(void* cont, int sfxID, int loop,
                                         int onCont, float* wPos); /* 00465050 */
/* GAME-043 Stats.c (entity_stats.c). */
unsigned int stats_object_get_stats_flags1(const struct StatsObject* obj); /* 00469e60 */

/* gods98 Container.c (container.c). */
void Container_GetOrientation(struct Container* cont, struct Container* ref, float* dir,
                              float* up);                         /* 00475500 */
/* gods98 Maths.c (maths.c). */
short rng_thunk_draw15(void);                                     /* 004798a0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_MESSAGE_UNIT_IMPORTS_H */
