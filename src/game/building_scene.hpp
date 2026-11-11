// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_BUILDING_SCENE_HPP
#define LEGORRR_BUILDING_SCENE_HPP
#include <stdint.h>
struct EntitySlot;
struct GameContext;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
/* Copy the actual configured locator's world pose immediately. Status is
 * 1 found, 0 authored locator absent, -1 owner/pose unavailable. */
int building_scene_deposit_approach_get(EntitySlot* entity, int tool_null,
                                        float position[3], float facing[3]);
/* Dedicated type4 owner; never installs the shared visual_load callback.
 * Config paths are copied, templates open lazily for actual created buildings.
 * Consumers retire before instances/templates/context and the global root. */
int building_scene_initialize(void);
int building_scene_config_path_set(uint32_t row, const char* path, uint32_t feature);
int building_scene_types_clear(void);
int building_scene_entity_bind(EntitySlot* entity);
void building_scene_entity_release(EntitySlot* entity);
int building_scene_pose_set(EntitySlot* entity);
int building_scene_activity_set(EntitySlot* entity, const char* state);
void building_scene_tick(GameContext* game, float seconds);
int building_scene_shutdown(void);
/* A failed native owner must not continue global/CRT teardown. Emits a
 * bounded error and terminates with _exit(2), bypassing later destructors. */
void building_scene_require_stopped(void);
#endif
#endif
