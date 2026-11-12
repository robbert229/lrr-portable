// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from simtick.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SIMTICK_FOR_LEGOOBJECT_HPP
#define LRR_GODS98_SIMTICK_FOR_LEGOOBJECT_HPP

typedef struct SimtickCellCoord { int x, y; } SimtickCellCoord;

extern SimtickSceneEntityHook s_scene_entity_hook;
extern void* s_scene_entity_user;
extern void (*s_scene_broadcast)(int);
extern int s_scene_bcasts;
extern SimtickCellCoord* s_scene_tracked;
extern uint32_t s_scene_tracked_count;
extern uint32_t s_scene_pending_count;
extern void (*s_scene_cell_init)(EngineState*, int, int);
int scene_cell_list_append(SimtickCellCoord** list, uint32_t* count, uint32_t* capacity, int x, int y);
extern SimtickRenderEntityHook s_render_entity_hook;
extern void* s_render_entity_user;

#endif /* LRR_GODS98_SIMTICK_FOR_LEGOOBJECT_HPP */
