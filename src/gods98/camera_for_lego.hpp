// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from camera.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_CAMERA_FOR_LEGO_HPP
#define LRR_GODS98_CAMERA_FOR_LEGO_HPP

extern uint32_t s_predug_spawn_count;
extern int s_predug_spawn_enabled;
extern const uint8_t* s_spawn_level_block;
extern CameraGridSpawnHook s_grid_spawn_hook;
extern void* s_grid_spawn_context;
extern CameraSpawnRegistryProbeHook s_spawn_registry_probe_hook;
extern void* s_spawn_registry_probe_context;
extern CameraControlRegistryPredicate s_control_registry_predicate;
extern void* s_control_registry_user;

#endif /* LRR_GODS98_CAMERA_FOR_LEGO_HPP */
