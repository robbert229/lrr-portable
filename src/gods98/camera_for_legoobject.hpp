// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from camera.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_CAMERA_FOR_LEGOOBJECT_HPP
#define LRR_GODS98_CAMERA_FOR_LEGOOBJECT_HPP

extern int s_predug_spawn_cells[256][2];
extern CameraSpawnRandomIndexHook s_spawn_random_index_hook;
extern CameraSpawnRandomAngleHook s_spawn_random_angle_hook;
extern CameraSpawnObjectCellHook s_spawn_object_cell_hook;
extern CameraSpawnCoordinateHook s_spawn_coordinate_hook;
extern CameraSpawnCreateHook s_spawn_create_hook;
extern CameraSpawnInitializeHook s_spawn_initialize_hook;
extern void* s_spawn_hook_context;
extern void* s_spawn_coordinate_context;

#endif /* LRR_GODS98_CAMERA_FOR_LEGOOBJECT_HPP */
