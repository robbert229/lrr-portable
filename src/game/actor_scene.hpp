// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_ACTOR_SCENE_HPP
#define LEGORRR_ACTOR_SCENE_HPP
#include <stdint.h>
struct EntitySlot;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
/* Type-2/3 authored ACT owner. Paths are copied from the actual type rows;
 * templates, Models and real retained frames outlive their borrowed DDi
 * consumers. This supplies pose/timing capability, not mesh rendering. */
int actor_scene_initialize(void);
int actor_scene_config_path_set(int type, uint32_t row, const char* path,
                                 uint32_t feature);
int actor_scene_types_clear(int type);
int actor_scene_entity_bind(EntitySlot* entity);
void actor_scene_entity_release(EntitySlot* entity);
int actor_scene_pose_set(EntitySlot* entity);
/* Borrow only this live instance's actual root for ordered pose queries. */
void* actor_scene_motion_component(EntitySlot* entity);
int actor_scene_activity_set(EntitySlot* entity, const char* state,
                              float initial_time);
int actor_scene_clock_reset(EntitySlot* entity);
/* Delta is already in reference frame units. A false return never supplies
 * a completion value; callers must not substitute elapsed time. */
int actor_scene_advance(EntitySlot* entity, float delta, uint32_t count,
                         float* overflow);
void* actor_scene_named_node(EntitySlot* entity, const char* configured_key,
                              uint32_t ordinal);
/* Must precede building/global scene shutdown. Refuses destruction with
 * surviving frames; no global scene pool or foreign context is released. */
void actor_scene_require_stopped(void);
#endif
#endif
