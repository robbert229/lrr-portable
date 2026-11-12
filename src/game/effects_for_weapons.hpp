// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what weapons uses from effects.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_EFFECTS_FOR_WEAPONS_HPP
#define LRR_GAME_EFFECTS_FOR_WEAPONS_HPP

extern uint32_t s_active_effect_occupied[10];
extern uint32_t s_active_segment_visual_occupied[10];
extern ActiveEffectRecord s_active_effect_records[10];
extern ActiveEffectBuildHooks s_active_effect_build_hooks;
extern EntityImpulseApplyHook s_entity_impulse_hook;
extern EntityDirectionApplyHook s_entity_direction_hook;
extern TimedSegmentVisualHooks s_timed_segment_visual_hooks;
extern void* s_timed_segment_visuals[10][2];
extern void* s_timed_segment_visual_owners[10];
extern SimtickSfxTimedVisualRecord s_timed_segment_tick_records[10];
extern EffectApplyHooks s_effect_apply_hooks;
BezierCurve* active_effect_path(const ActiveEffectRecord* record);

#endif /* LRR_GAME_EFFECTS_FOR_WEAPONS_HPP */
