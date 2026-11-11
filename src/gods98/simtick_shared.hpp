// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/simtick.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_SIMTICK_SHARED_HPP
#define LRR_GODS98_SIMTICK_SHARED_HPP

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

#endif /* LRR_GODS98_SIMTICK_SHARED_HPP */
