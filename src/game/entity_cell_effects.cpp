// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-008 GAME-008: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-008.csv. */
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
#include "crt/crt_shared.hpp"
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
#include "entity_shared.hpp"
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
#include "level_shared.hpp"
#include "crt/crt_shared.hpp"
#include "entity_shared.hpp"
#include "level_shared.hpp"

// GLOBAL: LEGORR 0x004b8a54
uint32_t s_level_stored_word = 0u;  /* DAT_004b8a54 */

// FUNCTION: LEGORR 0x00409d40
void level_state_stored_value_set(uint32_t v)
{
    s_level_stored_word = v;
}

namespace unported_ledger_stubs {
/* EntityCellPathEffectsApply (FUN_00409d50) uses one entity argument and the
 * active level/type tables; there is no caller-supplied hook bundle. */
// FUNCTION: LEGORR 0x00409d50
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
// FUNCTION: LEGORR 0x00409f20
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
