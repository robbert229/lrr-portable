// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "port_loop_ports.hpp"
#include "entity.hpp"
#include "cmd.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "../crt/crt.hpp"
#include "../gods98/block.hpp"

namespace unported_ledger_stubs {

typedef struct DependencyGridLevelStorage {
    DependencyGridState state;
    DependencyGridCategoryCandidates candidates;
    DependencyGridTuple* tuples[DEPENDENCY_GRID_ROWS]
        [DEPENDENCY_GRID_COLUMNS][DEPENDENCY_GRID_VARIANTS + 1u];
    uint32_t tuple_capacity[DEPENDENCY_GRID_ROWS]
        [DEPENDENCY_GRID_COLUMNS][DEPENDENCY_GRID_VARIANTS + 1u];
} DependencyGridLevelStorage;

static void dependency_grid_owned_records_clear(
    DependencyGridLevelStorage* storage)
{
    uint32_t row, column, list;
    if (storage == NULL) return;
    for (row = 0u; row < DEPENDENCY_GRID_ROWS; ++row) {
        for (column = 0u; column < DEPENDENCY_GRID_COLUMNS; ++column) {
            for (list = 0u; list <= DEPENDENCY_GRID_VARIANTS; ++list) {
                free(storage->tuples[row][column][list]);
                storage->tuples[row][column][list] = NULL;
                storage->tuple_capacity[row][column][list] = 0u;
            }
        }
    }
}

static DependencyGridLevelStorage* dependency_grid_storage_get(
    const LevelState* level)
{
    if (level == NULL) return NULL;
    return (DependencyGridLevelStorage*)level->ptr_dependency_grid_state;
}

static DependencyGridLevelStorage* dependency_grid_storage_ensure(
    LevelState* level)
{
    DependencyGridLevelStorage* storage;
    if (level == NULL) return NULL;
    storage = dependency_grid_storage_get(level);
    if (storage == NULL) {
        storage = (DependencyGridLevelStorage*)calloc(1, sizeof(*storage));
        if (storage == NULL) return NULL;
        level->ptr_dependency_grid_state = storage;
    }
    return storage;
}

static LevelState* dependency_grid_current_level(void)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    return level;
}

static DependencyGridLevelStorage* dependency_grid_current_storage(void)
{
    return dependency_grid_storage_get(dependency_grid_current_level());
}

static int dependency_grid_name_equal(const char* left, const char* right)
{
    if (left == NULL || right == NULL) return 0;
    while (*left != '\0' && *right != '\0') {
        int a = (unsigned char)*left++;
        int b = (unsigned char)*right++;
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return 0;
    }
    return *left == '\0' && *right == '\0';
}

static BlockRec* dependency_grid_child_find(BlockRec* parent,
                                             const char* name)
{
    BlockRec* child;
    if (parent == NULL) return NULL;
    if (dependency_grid_name_equal(block_name(parent), name) &&
        block_has_children(parent)) return parent;
    for (child = block_first_child(parent); child != NULL;
         child = block_next_sibling(child)) {
        if (dependency_grid_name_equal(block_name(child), name) &&
            block_has_children(child)) return child;
    }
    return NULL;
}

static void dependency_grid_occupancy_pass_state(DependencyGridState* state);

void dependency_grid_state_bind(DependencyGridState* state)
{
    DependencyGridLevelStorage* storage;
    LevelState* level = dependency_grid_current_level();
    if (level == NULL) return;
    if (state == NULL) {
        dependency_grid_level_release(level);
        return;
    }
    storage = dependency_grid_storage_ensure(level);
    if (storage != NULL && state != &storage->state)
        memcpy(&storage->state, state, sizeof(storage->state));
}

static int dependency_grid_parse(LevelState* level, BlockRec* level_block,
                                 BlockFile* global_config)
{
    DependencyGridLevelStorage* storage =
        dependency_grid_storage_ensure(level);
    DependencyGridState* state;
    BlockRec* section;
    BlockRec* entry;
    int processed = 0;
    uint32_t row, column;
    if (storage == NULL) return 0;
    if (level_block == NULL && global_config == NULL) return 0;
    state = &storage->state;
    memset(state->cells, 0, sizeof(state->cells));
    dependency_grid_owned_records_clear(storage);
    memset(&storage->candidates, 0, sizeof(storage->candidates));
    section = dependency_grid_child_find(level_block, "Dependencies");
    if (section == NULL && global_config != NULL)
        section = block_subgroup(global_config, "Dependencies");
    if (section == NULL) {
        dependency_grid_occupancy_pass_state(state);
        return 0;
    }
    for (entry = block_first_child(section); entry != NULL;
         entry = block_next_equal_key(entry)) {
        const char* raw_name = block_name(entry);
        const char* raw_value = block_value(entry);
        char name_copy[512];
        char value_copy[512];
        char* name_part[3];
        char* value_part[10];
        uint32_t name_count = 1u;
        uint32_t value_count = 1u;
        uint32_t i;
        uint32_t variant = 0u;
        uint32_t type_index;
        uint32_t record_index;
        int type_code, type_row;
        int hit_once;
        DependencyGridCell* cell;
        DependencyGridRecordList* list;
        if (raw_name == NULL || raw_value == NULL) continue;
        if (strlen(raw_name) >= sizeof(name_copy) ||
            strlen(raw_value) >= sizeof(value_copy)) continue;
        strcpy(name_copy, raw_name);
        strcpy(value_copy, raw_value);
        name_part[0] = name_copy;
        for (i = 0u; name_copy[i] != '\0'; ++i) {
            if (name_copy[i] == ':') {
                name_copy[i] = '\0';
                if (name_count < 3u) name_part[name_count] = name_copy + i + 1u;
                ++name_count;
            }
        }
        if (name_count < 2u) continue;
        hit_once = 1;
        {
            const char* key = name_part[0];
            const char* expected = "HitOnceStay";
            while (*key != '\0' && *expected != '\0') {
                int a = (unsigned char)*key++;
                int b = (unsigned char)*expected++;
                if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
                if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
                if (a != b) { hit_once = 0; break; }
            }
            if (*key != '\0' || *expected != '\0') hit_once = 0;
        }
        type_code = 0;
        type_row = 0;
        if (!entity_type_row_for_name(name_part[1], &type_code, &type_row) ||
            type_row < 0 || type_row >= (int)DEPENDENCY_GRID_COLUMNS)
            continue;
        if (type_code < 0 || type_code >= (int)DEPENDENCY_GRID_ROWS) continue;
        type_index = (uint32_t)type_code;
        if (name_count == 3u) {
            variant = (uint32_t)atoi(name_part[2]);
            if (atoi(name_part[2]) < 0 ||
                variant >= DEPENDENCY_GRID_VARIANTS) continue;
        }
        cell = &state->cells[type_index][(uint32_t)type_row];
        if (name_count == 3u) {
            cell->variant_records_present = 1u;
            if (hit_once) cell->flags[variant] |= 1u;
            list = &cell->variant_records[variant];
        }
        else {
            /* Retail writes the per-cell selector for every parsed entry.
             * A non-variant key clears it, even if an earlier key for this
             * cell had installed variant-specific lists. */
            cell->variant_records_present = 0u;
            if (hit_once) {
                for (i = 0u; i < DEPENDENCY_GRID_VARIANTS; ++i)
                    cell->flags[i] |= 1u;
            }
            list = &cell->base_records;
        }
        value_part[0] = value_copy;
        for (i = 0u; value_copy[i] != '\0'; ++i) {
            if (value_copy[i] == ',') {
                value_copy[i] = '\0';
                if (value_count < 10u) value_part[value_count] = value_copy + i + 1u;
                ++value_count;
            }
        }
        if (list->records == NULL) {
            uint32_t storage_variant = name_count == 3u ? variant + 1u : 0u;
            list->records = storage->tuples[type_index][(uint32_t)type_row]
                [storage_variant];
        }
        for (i = 0u; i < value_count && i < 10u; ++i) {
            char* dependency_part[2];
            uint32_t part_count = 1u;
            int dependency_type, dependency_row;
            DependencyGridTuple* tuple;
            uint32_t storage_variant = name_count == 3u ? variant + 1u : 0u;
            char* token = value_part[i];
            char* comma = strchr(token, ':');
            if (comma != NULL) {
                *comma = '\0';
                dependency_part[1] = comma + 1;
                part_count = 2u;
            }
            dependency_part[0] = token;
            if (!entity_type_row_for_name(dependency_part[0],
                                          &dependency_type,
                                          &dependency_row)) continue;
            record_index = list->count;
            if (record_index >= storage->tuple_capacity[type_index]
                    [(uint32_t)type_row][storage_variant]) {
                uint32_t capacity = storage->tuple_capacity[type_index]
                    [(uint32_t)type_row][storage_variant];
                uint32_t next_capacity = capacity == 0u ? 4u : capacity * 2u;
                DependencyGridTuple* grown;
                if (next_capacity < capacity ||
                    next_capacity > 0xffffffffu / sizeof(*grown))
                    continue;
                grown = (DependencyGridTuple*)realloc(
                    storage->tuples[type_index][(uint32_t)type_row]
                        [storage_variant],
                    (size_t)next_capacity * sizeof(*grown));
                if (grown == NULL) continue;
                storage->tuples[type_index][(uint32_t)type_row]
                    [storage_variant] = grown;
                storage->tuple_capacity[type_index][(uint32_t)type_row]
                    [storage_variant] = next_capacity;
                list->records = grown;
            }
            tuple = &storage->tuples[type_index][(uint32_t)type_row]
                [storage_variant][record_index];
            tuple->first = (uint32_t)dependency_type;
            tuple->second = (uint32_t)dependency_row;
            if (part_count == 2u) {
                tuple->selector = (uint32_t)atoi(dependency_part[1]);
                tuple->wildcard = 1u;
            }
            ++list->count;
        }
        ++processed;
    }
    dependency_grid_occupancy_pass_state(state);
    return processed;
}

/* The retail entry receives its level config block and the global config
 * block.  Its records are owned by the active LevelState in this port, so
 * the same direct entry is called at level initialization after the active
 * level has been installed in GameContext. */
// FUNCTION: LEGORR 0x0040ab70
void dependency_grid_config_load(BlockRec* level_block,
                                 BlockFile* global_config)
{
    LevelState* level = dependency_grid_current_level();
    dependency_grid_level_load(level, level_block, global_config);
}

void dependency_grid_level_load(LevelState* level, BlockRec* level_block,
                                BlockFile* global_config)
{
    if (level == NULL) return;
    (void)dependency_grid_parse(level, level_block, global_config);
}

void dependency_grid_level_release(LevelState* level)
{
    DependencyGridLevelStorage* storage =
        dependency_grid_storage_get(level);
    if (storage == NULL) return;
    level->ptr_dependency_grid_state = NULL;
    dependency_grid_owned_records_clear(storage);
    free(storage);
}

/* GlobalModeZeroInputSet (FUN_0040ab10) stores (value == 0) in a process
 * global. Explicit level callers can retain a sidecar snapshot, but the
 * retail bypass remains global because EnsureCellEntriesLoaded tests
 * DAT_004c7178 directly. */
void dependency_grid_mode_zero_input_set(LevelState* level, int value)
{
    DependencyGridLevelStorage* storage;
    global_mode_zero_input_set(value);
    if (level == NULL) level = dependency_grid_current_level();
    storage = dependency_grid_storage_ensure(level);
    if (storage == NULL) return;
    storage->state.mode_zero_input = (value == 0) ? 1u : 0u;
    storage->state.mode_zero_input_bound = 1u;
}

uint32_t dependency_grid_mode_zero_input_get(const LevelState* level)
{
    (void)level;
    /* The retail bypass checks DAT_004c7178, even if a prior level has a
     * different sidecar snapshot. Never let that stale snapshot mask a
     * later write through the exact global setter at 0x40ab10. */
    return global_mode_zero_input_peek();
}

typedef struct DependencyGridEntityMatch {
    const DependencyGridTuple* tuple;
} DependencyGridEntityMatch;

static int dependency_grid_entity_matches(EntitySlot* entity, void* payload)
{
    DependencyGridEntityMatch* match = (DependencyGridEntityMatch*)payload;
    const DependencyGridTuple* tuple;
    if (entity == NULL || match == NULL || match->tuple == NULL) return 0;
    tuple = match->tuple;

    /* FUN_00439200 searches the entire live pool for any entity whose stats
     * bit 8 and action-kind predicate match this entry. This is a global
     * query, independent of the entity currently visited by FUN_004377b0. */
    if (tuple->first != 1u && tuple->first != 2u && tuple->first != 4u &&
        task_kind_filtered_search((int)tuple->first) != 0) {
        return 1;
    }

    /* FUN_0040af40 compares the two leading entity words, then applies the
     * configured threshold to the stats word at +0xcc.  EntitySlot exposes
     * those as type/etype and activity_cap.  The loader's type-name resolver
     * can produce rows that have no runtime representation; those simply do
     * not match unless the live pair agrees. */
    if ((uint32_t)entity->type != tuple->first ||
        (uint32_t)entity->etype != tuple->second) return 0;
    if (tuple->wildcard != 0u &&
        (uint32_t)entity->activity_cap < tuple->selector) return 0;
    return 1;
}

static int dependency_grid_cell_entries_ensure_loaded(
    DependencyGridState* state, uint32_t row, uint32_t column,
    uint32_t variant)
{
    DependencyGridCell* cell;
    const DependencyGridRecordList* list;
    uint32_t* flags;
    uint32_t entry;
    if (state == NULL || row >= DEPENDENCY_GRID_ROWS ||
        column >= DEPENDENCY_GRID_COLUMNS ||
        variant >= DEPENDENCY_GRID_VARIANTS) return 1;

    /* Retail performs the dependency walk while DAT_004c7178 is zero and
     * bypasses it when the flag is set. GlobalModeZeroInputSet stores
     * (input == 0), so read the shared global rather than a stale per-level
     * snapshot. */
    if (global_mode_zero_input_peek() != 0u) return 1;

    cell = &state->cells[row][column];
    flags = &cell->flags[variant];
    if ((*flags & 2u) != 0u) return 1;

    list = cell->variant_records_present != 0u
        ? &cell->variant_records[variant] : &cell->base_records;
    /* Retail EnsureCellEntriesLoaded resolves this cell's configured tuples,
     * then searches the active entity pool with FUN_004377b0 for every one.
     * Keep that live query as the production path; an optional occupancy
     * callback would replace the tuple semantics and has no retail analogue. */
    if (list->count != 0u && list->records == NULL) return 0;
    if (entity_pool_callback_context_get() == NULL && list->count != 0u)
        return 0;
    for (entry = 0u; entry < list->count; ++entry) {
        DependencyGridEntityMatch match;
        match.tuple = &list->records[entry];
        if (!entity_pool_find_by_callback_filtered_current(
                dependency_grid_entity_matches, &match)) return 0;
    }

    /* HitOnceStay marks this selector for one successful validation. Retail
     * caches only that success; ordinary dependency rows are checked again
     * on the next occupancy pass so entity changes can invalidate them. */
    if ((*flags & 1u) != 0u) *flags |= 2u;
    return 1;
}

/* The config parser is the source producer for TerrainCellRecordGet's
 * parallel lists. Keep their native pointer/count representation in the
 * LevelState sidecar, then perform EnsureCellEntriesLoaded's callback walk
 * here instead of truncating host pointers into the retail dword table. */
int dependency_grid_cell_entries_validate(const LevelState* level,
    uint32_t kind, uint32_t row, uint32_t variant)
{
    DependencyGridLevelStorage* storage = dependency_grid_storage_get(level);
    if (storage == NULL || kind >= DEPENDENCY_GRID_ROWS ||
        row >= DEPENDENCY_GRID_COLUMNS ||
        variant >= DEPENDENCY_GRID_VARIANTS) return 1;
    return dependency_grid_cell_entries_ensure_loaded(
        &storage->state, kind, row, variant);
}

uint32_t dependency_grid_cell_flags_get(const LevelState* level,
    uint32_t kind, uint32_t row, uint32_t variant)
{
    DependencyGridLevelStorage* storage = dependency_grid_storage_get(level);
    DependencyGridCell* cell;
    if (storage == NULL || kind >= DEPENDENCY_GRID_ROWS ||
        row >= DEPENDENCY_GRID_COLUMNS ||
        variant >= DEPENDENCY_GRID_VARIANTS) return 0u;
    cell = &storage->state.cells[kind][row];
    if (cell->variant_records_present == 0u) variant = 0u;
    return cell->flags[variant];
}

int dependency_grid_cell_record_get_native(const LevelState* level,
    int x, int y, int field, const DependencyGridTuple** records_out,
    uint32_t* count_out)
{
    DependencyGridLevelStorage* storage = dependency_grid_storage_get(level);
    const DependencyGridCell* cell;
    const DependencyGridRecordList* list;
    if (storage == NULL || x < 0 || x >= (int)DEPENDENCY_GRID_ROWS ||
        y < 0 || y >= (int)DEPENDENCY_GRID_COLUMNS) return 0;
    if (records_out != NULL) *records_out = NULL;
    if (count_out != NULL) *count_out = 0u;
    cell = &storage->state.cells[x][y];
    if (cell->variant_records_present != 0u) {
        if (field < 0 || field >= (int)DEPENDENCY_GRID_VARIANTS) return 1;
        list = &cell->variant_records[field];
    }
    else {
        /* Retail does not inspect param_3 at all when the marker byte is
         * clear; it always selects the base pointer/count pair. */
        list = &cell->base_records;
    }
    if (records_out != NULL) *records_out = list->records;
    if (count_out != NULL) *count_out = list->count;
    return 1;
}

// FUNCTION: LEGORR 0x0040b000
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

/* CellFlag4IsSet (FUN_0040b250) reads the selected dependency-grid bit.
 * 0xffffffff means there is no bound portable state, allowing the legacy
 * table image in level.cpp to remain the fallback. */
uint32_t dependency_grid_flag4_get(int x, int y, int field)
{
    DependencyGridLevelStorage* storage = dependency_grid_current_storage();
    DependencyGridState* state = storage != NULL ? &storage->state : NULL;
    DependencyGridCell* cell;
    if (state == NULL || x < 0 || x >= (int)DEPENDENCY_GRID_ROWS ||
        y < 0 || y >= (int)DEPENDENCY_GRID_COLUMNS) return 0xffffffffu;
    cell = &state->cells[x][y];
    if (cell->variant_records_present == 0u) field = 0;
    else if (field < 0 || field >= (int)DEPENDENCY_GRID_CELL_SLOTS) return 0u;
    return cell->flags[field] & 4u;
}

int dependency_grid_flag100_set(int x, int y, int field)
{
    DependencyGridLevelStorage* storage = dependency_grid_current_storage();
    DependencyGridState* state = storage != NULL ? &storage->state : NULL;
    if (state == NULL || x < 0 || x >= (int)DEPENDENCY_GRID_ROWS ||
        y < 0 || y >= (int)DEPENDENCY_GRID_COLUMNS || field < 0 ||
        field >= (int)DEPENDENCY_GRID_CELL_SLOTS) return 0;
    state->cells[x][y].flags[field] |= 0x100u;
    return 1;
}

uint32_t dependency_grid_flag100_get(int x, int y, int field)
{
    DependencyGridLevelStorage* storage = dependency_grid_current_storage();
    DependencyGridState* state = storage != NULL ? &storage->state : NULL;
    if (state == NULL || x < 0 || x >= (int)DEPENDENCY_GRID_ROWS ||
        y < 0 || y >= (int)DEPENDENCY_GRID_COLUMNS || field < 0 ||
        field >= (int)DEPENDENCY_GRID_CELL_SLOTS) return 0xffffffffu;
    return state->cells[x][y].flags[field] & 0x100u;
}

// FUNCTION: LEGORR 0x00418920
void dependency_grid_category_candidates_build(uint32_t category,
                                               uint32_t row,
                                               uint32_t selector)
{
    DependencyGridMatches matches;
    DependencyGridLevelStorage* storage = dependency_grid_current_storage();
    DependencyGridCategoryCandidates* candidates;
    uint32_t index;
    if (storage == NULL) return;
    candidates = &storage->candidates;
    candidates->count = 0u;
    candidates->category = category;
    candidates->row = row;
    candidates->selector = selector;
    dependency_grid_matches_collect(category, row, selector, &matches);
    for (index = 0u; index < matches.count &&
         index < DEPENDENCY_GRID_MATCH_CAPACITY; ++index) {
        const uint32_t x = matches.x[index];
        const uint32_t y = matches.y[index];
        const uint32_t variant = matches.variant[index];
        uint32_t output_index;
        if (cell_flag4_is_set((int)x, (int)y, (int)variant) == 0u) continue;
        output_index = candidates->count;
        if (output_index < DEPENDENCY_GRID_CATEGORY_CANDIDATE_CAPACITY) {
            candidates->x[output_index] = x;
            candidates->y[output_index] = y;
            candidates->variant[output_index] = variant;
        }
        ++candidates->count;
    }
}

const DependencyGridCategoryCandidates*
dependency_grid_category_candidates_get(void)
{
    DependencyGridLevelStorage* storage = dependency_grid_current_storage();
    return storage != NULL ? &storage->candidates : NULL;
}

static void dependency_grid_occupancy_pass_state(DependencyGridState* state)
{
    uint32_t row, column, variant;
    if (state == NULL) return;
    for (row = 0u; row < DEPENDENCY_GRID_ROWS; ++row) {
        for (column = 0u; column < DEPENDENCY_GRID_COLUMNS; ++column) {
            DependencyGridCell* cell = &state->cells[row][column];
            for (variant = 0u; variant < DEPENDENCY_GRID_VARIANTS; ++variant) {
                uint32_t flags;
                int occupied;
                if (variant == 1u && cell->variant_records_present != 0u)
                    break;
                flags = cell->flags[variant];
                if ((flags & 8u) != 0u) continue;
                occupied = dependency_grid_cell_entries_ensure_loaded(
                    state, row, column, variant);
                /* EnsureCellEntriesLoaded may set the cached-success bit
                 * (bit 1) for a HitOnceStay record. Retail reads the rule
                 * word after that call, so preserve its mutation while
                 * applying the occupancy transition below. */
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

// FUNCTION: LEGORR 0x0040b1b0
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

void ledger_stub_0040b1b0_DependencyGridOccupancyPass(void)
{
    (void)dependency_grid_occupancy_pass();
}


}
