// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what terrain_cell_objects.cpp sees from
 * simtick.cpp. A verbatim snapshot of simtick_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_SIMTICK_FOR_TERRAIN_CELL_OBJECTS_HPP
#define LRR_GODS98_SIMTICK_FOR_TERRAIN_CELL_OBJECTS_HPP

extern SimtickEffect s_effects[16];
extern int s_effect_count;
extern int s_effect_fired;
extern void (*s_effect_opacity_hook)(uint32_t, int, float, int);
extern void (*s_effect_release_hook)(uint32_t, int);

/* LightEffects.c's scene-transform word (DAT_004eaec8, GAME-024). It is one
 * original global: every reader and writer in the original is a LightEffects
 * function (0044c790..0044da50). game/util.cpp defines it with the rest of
 * the unit's state; simtick.cpp holds two of the unit's functions. */
extern uint32_t s_motion_jitter_flags;

extern SimtickBobbin* s_bobbin_head;
extern int s_bobbin_advance;
extern SimtickBobbinMarkHook s_bobbin_mark_hook;
extern void* s_bobbin_mark_context;
extern SimtickBobbinSpawnHook s_bobbin_spawn_hook;
extern void* s_bobbin_action_context;
void bobbin_clear_delay(SimtickBobbin* node);

extern int s_post_steps;
extern void** s_post_record_tables;
extern uint32_t s_post_record_table_count;
extern SimtickPostClusterRecordHook s_post_active_hook;
extern SimtickPostClusterRecordHook s_post_entity_first_hook;
extern SimtickPostClusterRecordHook s_post_entity_second_hook;
extern void* s_post_cluster_user;

extern float s_xform_current[3];
extern float s_xform_config[3];

extern int s_marker_fired;

extern int s_post_n;
extern SimtickPostStepHook s_post_step_hook;
extern void* s_post_step_user;

extern SimtickFxEntry* s_fx_entries;
extern uint32_t s_fx_count;
extern SimtickFxResourceUpdateFn s_fx_update;
extern void* s_fx_user;

#endif /* LRR_GODS98_SIMTICK_FOR_TERRAIN_CELL_OBJECTS_HPP */
