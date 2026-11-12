// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what weapons uses from entity.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_ENTITY_FOR_WEAPONS_HPP
#define LRR_GAME_ENTITY_FOR_WEAPONS_HPP

extern EntitySegmentRadiusGetHook s_entity_segment_radius_hook;
extern EntityRayHitSweepOps s_entity_ray_hit_sweep_ops;
extern EntitySteerVelocityApplyFn s_entity_steer_velocity_apply;
extern void* s_entity_steer_velocity_context;
extern EntityVelocitySlotDispatchOps s_entity_velocity_slot_dispatch_ops;

#endif /* LRR_GAME_ENTITY_FOR_WEAPONS_HPP */
