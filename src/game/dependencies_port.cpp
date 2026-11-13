// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-010 Dependencies.c's port tail (finding 612). dependencies.c is the
 * unit, rebuilt in original shape; this file keeps the port's model of it,
 * unannotated: the level-owned dependency grid entries (dependency_grid.cpp)
 * and the fallback table accessors that level.cpp wraps. In the matching
 * build dependencies.c serves the two C-linkage port names defined here
 * (dependencies_link_names.h), so they are compiled out there
 * (LRR_DEPENDENCIES_PORT_NAMES). Relocated from dependencies.cpp
 * (tools/relocate_unit.py, ADR 0006); body hashes:
 * docs/coverage/relocations/GAME-010.csv. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
#include "legoobject_port_aliases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/record.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/simtick.hpp"
#include "gods98/string.hpp"
#include "gods98/surface.hpp"
#include "crt/crt.hpp"
#include "gods98/view.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "entity.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"
#include "gods98/flic.hpp"
#include "select.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "level_for_dependencies.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include "crt/crt_for_dependencies.hpp"
#include "level_for_dependencies.hpp"
#include "crt/crt_for_dependencies.hpp"
#include "dependency_grid_for_dependencies.hpp"

/* Cell-entry readiness bits occupy bytes at DAT_004b8c4c. DependencyGridConfigLoad
 * produces the parallel record/count lists in the active LevelState sidecar;
 * its flags seed the retail ready-bit word here. */
#define CELL_ENTRY_SLOT_TYPES 20
#define CELL_ENTRY_SLOT_ROWS 15
#define CELL_ENTRY_SLOT_VARIANTS 16
#define CELL_ENTRY_SLOT_STRIDE 49

/* Port state of 0x004c7178 */
uint32_t s_mode_zero_flag = 0u;     /* DAT_004c7178 */

namespace unported_ledger_stubs {
/* The retail entry receives its level config block and the global config
 * block.  Its records are owned by the active LevelState in this port, so
 * the same direct entry is called at level initialization after the active
 * level has been installed in GameContext. */
/* Port model of 0x0040ab70 */
void dependency_grid_config_load(BlockRec* level_block,
                                 BlockFile* global_config)
{
    LevelState* level = dependency_grid_current_level();
    dependency_grid_level_load(level, level_block, global_config);
}
} // namespace unported_ledger_stubs

/* dependencies.c serves the port's names of 0040aea0 and 0040af40 in the
 * matching build (dependencies_link_names.h): every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_DEPENDENCIES_PORT_NAMES 0
#else
#define LRR_DEPENDENCIES_PORT_NAMES 1
#endif

#if LRR_DEPENDENCIES_PORT_NAMES /* dependencies.c serves these names in the matching build */
/* Port model of 0x0040aea0 */
int ensure_cell_entries_loaded(uint32_t kind, int row, int slot)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    if (unported_ledger_stubs::dependency_grid_mode_zero_input_get(level)
        != 0u) {
        return 1;
    }
    if (kind >= CELL_ENTRY_SLOT_TYPES || row < 0
        || row >= CELL_ENTRY_SLOT_ROWS || slot < 0
        || slot >= CELL_ENTRY_SLOT_VARIANTS) {
        return 0;
    }
    const size_t index = ((size_t)kind * CELL_ENTRY_SLOT_ROWS
                          + (size_t)row) * CELL_ENTRY_SLOT_STRIDE
                         + (size_t)slot;
    uint8_t* const ready = (uint8_t*)s_grid_rec + 0x84u + index;
    *ready |= (uint8_t)(unported_ledger_stubs::
        dependency_grid_cell_flags_get(level, kind, (uint32_t)row,
                                       (uint32_t)slot) & 1u);
    if ((*ready & 2u) != 0u) {
        return 1;
    }
    if (!unported_ledger_stubs::dependency_grid_cell_entries_validate(
            level, kind, (uint32_t)row, (uint32_t)slot)) {
        return 0;
    }
    if ((*ready & 1u) != 0u) {
        *ready |= 2u;
    }
    return 1;
}
/* CellResourceEntryMatches (FUN_0040af40): entries backed by a matching
 * live task are accepted directly. Otherwise the first two words must
 * match, and a nonzero bounded-mode flag applies the unsigned cell limit
 * from entry word two to the cell record's word index 0xcc (+0x330 bytes). */
/* Port model of 0x0040af40 */
int terrain_cell_resource_entry_matches(const uint32_t* cell_record,
                                        const uint32_t* resource_entry)
{
    if (cell_record == NULL || resource_entry == NULL) {
        return 0;
    }
    if (task_kind_filtered_search((int)resource_entry[0]) != 0) {
        return 1;
    }
    if (resource_entry[0] != cell_record[0]
        || resource_entry[1] != cell_record[1]) {
        return 0;
    }
    if (resource_entry[3] != 0u
        && cell_record[0xcc] < resource_entry[2]) {
        return 0;
    }
    return 1;
}

#endif /* LRR_DEPENDENCIES_PORT_NAMES */

/* The port's fallback table accessors (level.cpp's wrappers call them). */
/* Port model of 0x0040af90 */
void terrain_cell_record_get_target(int x, int y, int field,
                                           uint32_t* value_a,
                                           uint32_t* value_b)
{
#if defined(_MSC_VER) && _MSC_VER < 1200
    /* No port table in the matching build (#700): an empty one's words. */
    (void)x; (void)y; (void)field;
    *value_a = 0u;
    *value_b = 0u;
#else
    const uint32_t cell = (uint32_t)(y + x * GRID_REC_COLS);
    const uint32_t offset = cell * GRID_REC_STRIDE;
    const uint32_t base = offset +
        (s_grid_rec[offset + 32u] != 0u ? (uint32_t)field : 0u);
    *value_a = s_grid_rec[base + 16u];
    *value_b = s_grid_rec[base];
#endif
}

namespace unported_ledger_stubs {
/* Port model of 0x0040b000 */
void dependency_grid_matches_collect(uint32_t first, uint32_t second,
                                     uint32_t selector,
                                     DependencyGridMatches* output)
{
    DependencyGridLevelStorage* storage = dependency_grid_current_storage();
    DependencyGridState* state = storage != NULL ? &storage->state : NULL;
    uint32_t row, column, variant;
    if (output == NULL) return;
    output->count = 0u;
    if (state == NULL) return;
    for (row = 0u; row < DEPENDENCY_GRID_ROWS; ++row) {
        for (column = 0u; column < DEPENDENCY_GRID_COLUMNS; ++column) {
            DependencyGridCell* cell = &state->cells[row][column];
            const uint32_t variant_limit =
                cell->variant_records_present != 0u
                    ? DEPENDENCY_GRID_VARIANTS : 1u;
            for (variant = 0u; variant < variant_limit; ++variant) {
                const DependencyGridRecordList* list =
                    cell->variant_records_present != 0u
                        ? &cell->variant_records[variant]
                        : &cell->base_records;
                uint32_t entry;
                if (list->records == NULL) continue;
                for (entry = 0u; entry < list->count; ++entry) {
                    const DependencyGridTuple* tuple = &list->records[entry];
                    uint32_t out_index;
                    if (tuple->first != first || tuple->second != second ||
                        (tuple->wildcard != 0u &&
                         tuple->selector != selector)) continue;
                    out_index = output->count;
                    /* The retail caller provides four 32-dword arrays, but
                     * the loop has no overflow branch. Keep this port's
                     * count and all four outputs within that concrete ABI
                     * capacity if malformed/expanded config exceeds it. */
                    if (out_index >= DEPENDENCY_GRID_MATCH_CAPACITY) return;
                    output->x[out_index] = row;
                    output->y[out_index] = column;
                    output->variant[out_index] = variant;
                    output->variant_present[out_index] =
                        cell->variant_records_present != 0u ? 1u : 0u;
                    ++output->count;
                }
            }
        }
    }
}

/* Port model of 0x0040b1b0 */
void dependency_grid_occupancy_pass(void)
{
    DependencyGridLevelStorage* storage = dependency_grid_current_storage();
    DependencyGridState* state;
    uint32_t row, column, variant;
    if (storage == NULL) return;
    state = &storage->state;
    for (row = 0u; row < DEPENDENCY_GRID_ROWS; ++row) {
        for (column = 0u; column < DEPENDENCY_GRID_COLUMNS; ++column) {
            DependencyGridCell* cell = &state->cells[row][column];
            for (variant = 0u; variant < DEPENDENCY_GRID_VARIANTS;
                 ++variant) {
                uint32_t flags;
                int occupied;
                /* The retail slot-one gate skips the remaining variants for
                 * a cell whose per-variant record table is populated. */
                if (variant == 1u && cell->variant_records_present != 0u)
                    break;
                flags = cell->flags[variant];
                if ((flags & 8u) != 0u) continue;
                occupied = dependency_grid_cell_entries_ensure_loaded(
                    state, row, column, variant);
                /* Validation can set bit 1 for HitOnceStay. The reference
                 * reloads this dword after EnsureCellEntriesLoaded before
                 * changing the occupancy bits; using the earlier snapshot
                 * would discard the validation cache. */
                flags = cell->flags[variant];
                if (occupied == 0) {
                    if ((flags & 4u) == 0u) continue;
                    flags = (flags & ~4u) | 8u;
                }
                else if ((flags & 4u) == 0u) {
                    flags |= 4u;
                }
                else {
                    flags = (flags & ~4u) | 8u;
                }
                cell->flags[variant] = flags;
            }
        }
    }
}
} // namespace unported_ledger_stubs

/* Port model of 0x0040b250 */
uint32_t cell_flag4_is_set_target(int x, int y, int field)
{
#if defined(_MSC_VER) && _MSC_VER < 1200
    (void)x; (void)y; (void)field;
    return 0u; /* no port table in the matching build (#700) */
#else
    const uint32_t cell = (uint32_t)(y + x * GRID_REC_COLS);
    const uint32_t offset = cell * GRID_REC_STRIDE;
    if (s_grid_rec[offset + 32u] != 0u)
        return s_grid_rec[offset + 33u + (uint32_t)field] & 4u;
    return s_grid_rec[offset + 33u] & 4u;
#endif
}

/* Port model of 0x0040b2a0 */
void cell_flag100_set_target(int x, int y, int field)
{
#if defined(_MSC_VER) && _MSC_VER < 1200
    (void)x; (void)y; (void)field; /* no port table in the matching build (#700) */
#else
    const uint32_t cell = (uint32_t)(y + x * GRID_REC_COLS);
    const uint32_t word = cell * GRID_REC_STRIDE + (uint32_t)field + 33u;
    s_grid_rec[word] |= 0x100u;
#endif
}

/* Port model of 0x0040b2e0 */
uint32_t cell_flag100_is_set_target(int x, int y, int field)
{
#if defined(_MSC_VER) && _MSC_VER < 1200
    (void)x; (void)y; (void)field;
    return 0u; /* no port table in the matching build (#700) */
#else
    const uint32_t cell = (uint32_t)(y + x * GRID_REC_COLS);
    const uint32_t word = cell * GRID_REC_STRIDE + (uint32_t)field + 33u;
    return s_grid_rec[word] & 0x100u;
#endif
}
