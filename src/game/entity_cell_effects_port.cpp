// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-008's port tail (finding 652): the port's models of the unit's
 * functions. Unannotated: the original-shape unit is entity_cell_effects.c
 * (ADR 0005). Natively these models are the live ones; in the matching
 * build the unit defines the port names its callers use
 * (entity_cell_effects_link_names.h), so this file compiles its own out
 * there. Relocated by tools/relocate_unit.py (ADR 0006); body hashes:
 * docs/coverage/relocations/GAME-008.csv. */
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gods98/texture.hpp"
#include "crt/crt.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "crt/crt_for_entity_cell_effects.hpp"
#include <stdint.h>
#include "ai.hpp"
#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/record.hpp"
#include "gods98/message.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "entity_for_entity_cell_effects.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include <limits.h>
#include "game.hpp"
#include "gods98/simtick.hpp"
#include "gods98/surface.hpp"
#include "gods98/camera.hpp"
#include "objective.hpp"
#include "gods98/flic.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "level_for_entity_cell_effects.hpp"
#include "crt/crt_for_entity_cell_effects.hpp"
#include "entity_for_entity_cell_effects.hpp"
#include "level_for_entity_cell_effects.hpp"

/* The port's level switch for the building cost drop (the original's is
 * entity_cell_effects.c's 004b8a54); port_state.cpp reads it. */
uint32_t s_level_stored_word = 0u;

#if !(defined(_MSC_VER) && _MSC_VER < 1200)
/* The port's model of 00409d40. In the matching build
 * entity_cell_effects.c defines this name (entity_cell_effects_link_names.h). */
// PORT-ONLY: the native model of 0x00409d40; entity_cell_effects.c has the original's shape.
void level_state_stored_value_set(uint32_t v)
{
    s_level_stored_word = v;
}
#endif

#if defined(_MSC_VER) && _MSC_VER < 1200
/* The matching build: entity_cell_effects.c defines 00409d50 and 00409ff0
 * under the port's names. 00409f20 keeps the unit's own name there: its
 * port callers (level.cpp) are C++ and pass the port's EntitySlot, whose
 * fields the original reads sit at the original's offsets. */
extern "C" void building_drop_cost(struct Lego_Object* buildingObj); /* 00409f20 */

// PORT-ONLY: C++ forwarder for level.cpp's calls of 0x00409f20 in the matching build.
void entity_placement_commands_build_once(EntitySlot* entity)
{
    building_drop_cost((struct Lego_Object*)entity);
}
#else
namespace unported_ledger_stubs {
/* EntityCellPathEffectsApply (FUN_00409d50) uses one entity argument and the
 * active level/type tables; there is no caller-supplied hook bundle. */
// PORT-ONLY: the native model of 0x00409d50; entity_cell_effects.c has the original's shape.
void entity_cell_path_effects_apply_current(EntitySlot* entity)
{
    GameContext* context = entity != NULL ? entity->owner_ctx : NULL;
    if (context == NULL) context = entity_pool_callback_context_get();
    entity_cell_path_effects_apply_level(
        entity, context != NULL ? context->level : NULL);
}
} // namespace unported_ledger_stubs

/* EntityPlacementCommandsBuildOnce (FUN_00409f20): read the three indexed
 * stats words for this entity, enqueue the corresponding type-6/type-7
 * placement commands at its current terrain cell, and latch the per-entity
 * once bit. Empty/unloaded stats cells read as zero through the live table
 * seam, matching the other stats consumers in this reconstruction. */
// PORT-ONLY: the native model of 0x00409f20; entity_cell_effects.c has the original's shape.
void entity_placement_commands_build_once(EntitySlot* entity)
{
    GameContext* context;
    LevelState* level;
    int cell_xy[2];
    int32_t type6_count;
    int32_t type7_count;
    int index;
    if (entity == NULL || (entity->flags_3ec & 0x800u) != 0u) return;
    context = entity_pool_callback_context_get();
    if (context == NULL) context = entity->owner_ctx;
    if (context == NULL || context->level == NULL) return;
    level = context->level;
    entity_position_to_terrain_cell(entity, level, &cell_xy[0], &cell_xy[1]);
    if (!level_coord_pair_within_bounds(level, cell_xy[0], cell_xy[1])) return;

    /* The reference getters index through entity +0 (stats type), +4
     * (stats row), and +0x330 (strided stats instance). */
    type6_count = (int32_t)stats_table_word_at_b4(
        4, entity->etype, entity->indexed_record_slot_330);
    type7_count = (int32_t)stats_table_word_at_b0(
        4, entity->etype, entity->indexed_record_slot_330);
    (void)stats_table_word_at_b8(
        4, entity->etype, entity->indexed_record_slot_330);

    for (index = type6_count; index != 0; --index)
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 0, 0);
    for (index = type7_count; index != 0; --index)
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 0, 1);
    entity->flags_3ec |= 0x800u;
}
#endif
