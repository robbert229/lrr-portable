// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what advisor.cpp sees from
 * objective.cpp. A verbatim snapshot of objective_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_OBJECTIVE_FOR_ADVISOR_HPP
#define LRR_GAME_OBJECTIVE_FOR_ADVISOR_HPP

#define ADV_ACTION_ROWS 21

typedef struct AdvisorAnimationSlot {
    uint32_t handle;
    float start_time;
    float end_time;
} AdvisorAnimationSlot;

extern const char* s_advisor_animation_names[11];
extern uint32_t s_adv_actions[ADV_ACTION_ROWS][9];
extern float s_adv_scene_scale;
extern uint32_t s_adv_scene_state_word;
extern AdvisorSceneActionHook s_adv_scene_action_hook;
extern void* s_adv_scene_action_user;
extern AdvisorAnimationSlot s_adv_animation_slots[11];
extern AdvisorAnimationHooks s_adv_animation_hooks;
extern AdvisorAnimationTargetConfigLookupHook s_advisor_config_target_lookup;
extern void* s_advisor_config_target_lookup_user;
extern AdvisorTransformApplyHook s_adv_transform_apply_hook;
extern void* s_adv_transform_apply_user;

extern ObjectiveDeferredCallback s_objective_deferred_callback;
extern LevelState* s_objective_active_level;
extern int s_advisor_sound_slot;
extern const char** s_adv_text_tokens;
extern uint32_t s_adv_text_token_count;
extern const uint32_t* s_adv_tokens;
extern uint32_t s_adv_token_count;

extern char s_objective_voice_alias[256];
extern int s_objective_voice_alias_pending;
extern uint32_t s_objective_result_word;
extern int s_objective_voice_stream_handle;
extern int s_objective_voice_result_active;
extern int s_objective_tick_resolved;
void objective_advisor_clear_active_state(void);
extern int s_objective_announce_target_gate;

#endif /* LRR_GAME_OBJECTIVE_FOR_ADVISOR_HPP */
