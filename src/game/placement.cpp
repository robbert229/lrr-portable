// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-007 GAME-007: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-007.csv. */
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
#include "level_for_placement.hpp"
/* C linkage for AITask.c (aitask.c, ADR 0005 step 6, finding 570): the
 * frozen header below redeclares idle_register and
 * cmd_operation_group_append_object and inherits it. */
extern "C" void idle_register(EntitySlot* e);
extern "C" void cmd_operation_group_append_object(uint32_t key, EntitySlot* entity);
#include "cmd_for_placement.hpp"
#include <float.h>
#include <ctype.h>
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "weapon.hpp"
#include "gods98/model.hpp"
#include "entity_for_placement.hpp"
#include <errno.h>
#include <locale.h>
#include "texture.hpp"
#include "record_for_placement.hpp"
#include "game/render.hpp"
#include "simtick_for_placement.hpp"
#include <stdarg.h>
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "view_for_placement.hpp"
#include "level_for_placement.hpp"
/* C linkage for AITask.c (aitask.c, ADR 0005 step 6, finding 570): the
 * frozen header below redeclares idle_register and
 * cmd_operation_group_append_object and inherits it. */
extern "C" void idle_register(EntitySlot* e);
extern "C" void cmd_operation_group_append_object(uint32_t key, EntitySlot* entity);
#include "cmd_for_placement.hpp"
#include "entity_for_placement.hpp"
#include "util_for_placement.hpp"
#include "gods98/record_for_placement.hpp"
#include "gods98/simtick_for_placement.hpp"
#include "gods98/view_for_placement.hpp"

/* DAT_004b8a20 role */
// GLOBAL: LEGORR 0x004b8a20
uint32_t s_path_record_count;
// GLOBAL: LEGORR 0x004b8a24
void* s_path_record_last;     /* DAT_004b8a24 role */

// GLOBAL: LEGORR 0x004b8a50
unsigned int s_string_table_count;

void path_record_remove_and_free(void* record);

namespace unported_ledger_stubs {
int ledger_stub_00409470_NeighborCellEntitySearchRecurse(
    const int cell_xy[2], int resource_amount);
} // namespace unported_ledger_stubs

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void* cmd_path_descriptor_find(uint32_t key);
static void bobbin_delayed(SimtickBobbin* node);
static void bobbin_mark(SimtickBobbin* node);
static void bobbin_record_remove_and_free(SimtickBobbin* node);
static int bobbin_spawn(SimtickBobbin* node);
static void descriptor_transition_apply(SimtickBobbin* node);
static void path_record_storage_release(void* record);

// FUNCTION: LEGORR 0x004089d0
float segment_2d_point_distance(const float* point, const float* start,
                                const float* end)
{
    float segment_x = end[0] - start[0];
    float segment_y = end[1] - start[1];
    float point_to_end_x = point[0] - end[0];
    float point_to_end_y = point[1] - end[1];
    float point_to_end_distance = sqrtf(
        point_to_end_y * point_to_end_y +
        point_to_end_x * point_to_end_x);
    float inverse_segment_length = 1.0f / sqrtf(
        segment_y * segment_y + segment_x * segment_x);
    float unit_x = inverse_segment_length * segment_x;
    float unit_y = inverse_segment_length * segment_y;

    if ((1.0f / point_to_end_distance) * point_to_end_y * unit_y +
        (1.0f / point_to_end_distance) * point_to_end_x * unit_x < 0.0f) {
        float point_to_start_x = point[0] - start[0];
        float point_to_start_y = point[1] - start[1];
        float point_to_start_distance = sqrtf(
            point_to_start_y * point_to_start_y +
            point_to_start_x * point_to_start_x);
        if ((1.0f / point_to_start_distance) * point_to_start_y * unit_y +
            (1.0f / point_to_start_distance) * point_to_start_x * unit_x <=
                0.0f) {
            return point_to_start_distance;
        }
        {
            float perpendicular = point_to_start_x * unit_y -
                                 point_to_start_y * unit_x;
            if (perpendicular < 0.0f) perpendicular = -perpendicular;
            {
                const double angle = asin((double)perpendicular /
                                           (double)point_to_start_distance);
                return (float)(sin(angle) *
                               (double)point_to_start_distance);
            }
        }
    }
    return point_to_end_distance;
}

// FUNCTION: LEGORR 0x00408b00
float point_to_polyline_minimum_segment_distance(const float* point,
                                                 const float* first_point,
                                                 const float* last_point,
                                                 unsigned int segment_count)
{
    float minimum = segment_2d_point_distance(point, first_point, last_point);
    if (segment_count > 1u) {
        const ptrdiff_t segment_bytes =
            (const char*)last_point - (const char*)first_point;
        uint32_t remaining = segment_count - 1u;
        do {
            float distance;
            first_point = (const float*)((const char*)first_point + 8);
            last_point = (const float*)((const char*)first_point +
                                        segment_bytes);
            distance = segment_2d_point_distance(point, first_point,
                                                  last_point);
            if (distance < minimum) {
                minimum = distance;
            }
            remaining = remaining - 1u;
        }
        while (remaining != 0u);
    }
    return minimum;
}

// FUNCTION: LEGORR 0x00408b60
void point_project_onto_line2d(const float* line_start, const float* line_end,
                               const float* point, float* out)
{
    float dx = line_end[0] - line_start[0];
    float dy = line_end[1] - line_start[1];
    float inverse_length = 1.0f / sqrt(dy * dy + dx * dx);
    float unit_x = inverse_length * dx;
    float unit_y = inverse_length * dy;
    float projection = (point[1] - line_start[1]) * unit_y +
                       (point[0] - line_start[0]) * unit_x;
    out[0] = projection * unit_x + line_start[0];
    out[1] = projection * unit_y + line_start[1];
}

// FUNCTION: LEGORR 0x00408bf0
void point2d_project_onto_direction(const float* origin, const float* direction,
                                    const float* point, float* out)
{
    float direction_x = direction[0];
    float direction_y = direction[1];
    float inverse_length = 1.0f / sqrt(direction_y * direction_y +
                                       direction_x * direction_x);
    float unit_x = inverse_length * direction_x;
    float unit_y = inverse_length * direction_y;
    float projection = (point[1] - origin[1]) * unit_y +
                       (point[0] - origin[0]) * unit_x;
    out[0] = projection * unit_x + origin[0];
    out[1] = projection * unit_y + origin[1];
}

/* StringTableIndexLookup (FUN_00408c80): scan the configured name-pointer
 * table in order and return the first exact byte-string match.  The retail
 * comparator loads two bytes per iteration, but still checks both bytes and
 * stops at the same NUL terminator as an ordinary byte walk.  A miss returns
 * the table count. */
// FUNCTION: LEGORR 0x00408c80
unsigned int string_table_index_lookup(const char* value)
{
    unsigned int i;
    if (s_string_table_count == 0 || value == NULL) return 0;
    for (i = 0; i < s_string_table_count; ++i) {
        const unsigned char* left = (const unsigned char*)
            s_string_table_entries[i];
        const unsigned char* right = (const unsigned char*)value;
        int equal = 1;
        if (left == NULL) continue;
        for (;;) {
            unsigned char first = *left;
            unsigned char second = *right;
            if (first != second) {
                equal = 0;
                break;
            }
            if (first == 0) break;
            first = left[1];
            second = right[1];
            if (first != second) {
                equal = 0;
                break;
            }
            left += 2;
            right += 2;
            if (second == 0) break;
        }
        if (equal) return i;
    }
    return s_string_table_count;
}

// FUNCTION: LEGORR 0x00408CE0
void cmd_operation_group_append_object(uint32_t key, EntitySlot* entity)
{
    void* descriptor = cmd_path_descriptor_find(key);
    uint32_t expected_count = 0u;
    uint32_t current_count = 0u;
    uint32_t flags = 0u;
    if (descriptor == NULL) {
        if (entity != NULL)
            cmd_produce(entity->type == 6 ? 0x16 : 0x1c,
                        entity, 0, NULL);
        return;
    }

    if (entity != NULL) {
        /* Retail sets +0x3ec bit 0x100 before appending the entity pointer.
         * The sidecar append mirrors the descriptor's +0xcc and +0x20 counts. */
        entity->flags_3ec |= 0x100u;
        if (!path_record_entity_append(descriptor, entity)) return;
    }

    memcpy(&expected_count, (char*)descriptor + 0x1c,
           sizeof(expected_count));
    memcpy(&current_count, (char*)descriptor + 0x20,
           sizeof(current_count));
    if (expected_count == current_count) {
        const float completed_progress = 1.3125f;
        memcpy((char*)descriptor + 0x28, &completed_progress,
               sizeof(completed_progress));
        memcpy(&flags, (char*)descriptor + 0xd0, sizeof(flags));
        flags |= 1u;
        memcpy((char*)descriptor + 0xd0, &flags, sizeof(flags));
    }
}

// FUNCTION: LEGORR 0x00408d70
int terrain_descriptor_coordinate_count_limit_check(
    uint32_t descriptor_key, int type, int row)
{
    uint8_t* record = (uint8_t*)path_record_last_peek();
    uint32_t descriptor_flags;
    uint32_t descriptor_value;
    uint32_t matching_count;
    uint32_t limit;
    int selector;

    /* TerrainDescriptorFindExact(0, &key) walks the path-record chain and
     * compares the key against record +8.  The port stores that same chain
     * through the widened +0x24 link. */
    while (record != NULL) {
        uint32_t key;
        memcpy(&key, record + 0x08, sizeof(key));
        if (key == descriptor_key) break;
        record = (uint8_t*)*(void**)(record + 0x24);
    }
    if (record == NULL) return 0;

    /* Retail's 0x408e10 walks the +0x2c pointer list for +0x20 entries;
     * each pointed-to entity begins with the type/row pair. The port keeps
     * the live pointer list in its entity sidecar on non-32-bit builds, but
     * maintains the retail +0x20 count alongside +0xcc in append(). */
    matching_count = path_record_count_matching_coordinates(record, type,
                                                             row);
    memcpy(&descriptor_value, record, sizeof(descriptor_value));
    memcpy(&descriptor_flags, record + 0xd0, sizeof(descriptor_flags));
    /* Retail's masked-negation expression yields 15 when bit 1 is set. */
    selector = (descriptor_flags & 2u) != 0u ? 15 : 4;
    if (type == 6) {
        limit = entity_indexed_record_word_at_b4_get(
            selector, (int)descriptor_value, 0);
    }
    else if (type == 7) {
        if (row == 0) {
            limit = entity_indexed_record_word_at_b0_get(
                selector, (int)descriptor_value, 0);
        }
        else {
            limit = stats_table_word_at_b8(selector,
                                           (int)descriptor_value, 0);
        }
    }
    else {
        return 1;
    }
    return matching_count < limit;
}

// FUNCTION: LEGORR 0x00408e10
int count_matching_coordinates_in_list(const void* manager,
                                       int32_t coord_x, int32_t coord_y)
{
    const int32_t count = (int32_t)rec_dword(manager, 0x20);
    if (count == 0) {
        return 0;
    }
    int found = 0;
    for (int32_t i = 0; i < count; ++i) {
        void* entry = rec_ptr(manager, 0x2cu + (size_t)i * sizeof(void*));
        if (entry != NULL
            && (int32_t)rec_dword(entry, 0) == coord_x
            && (int32_t)rec_dword(entry, 4) == coord_y) {
            ++found;
        }
    }
    return found;
}

/* CreateRepeatedResourceInstances (FUN_00408e50): the parameter is a cell
 * coordinate pair. The descriptor owns the sequence counter and supplies the
 * zone written into each newly linked kind-7 command. */
// FUNCTION: LEGORR 0x00408E50
void entity_repeated_resource_instances_create(
    const int cell_xy[2], int kind, int variant, uint32_t value, int count)
{
    uint8_t* record;
    uint32_t zone;
    if (cell_xy == NULL || count <= 0) return;
    record = (uint8_t*)terrain_descriptor_find_exact_current(
        (const uint32_t*)cell_xy, NULL);
    if (record == NULL) return;
    if (kind == 7 && variant == 1) {
        uint32_t flags;
        memcpy(&flags, record + 0xd0u, sizeof(flags));
        flags |= 8u;
        memcpy(record + 0xd0u, &flags, sizeof(flags));
    }
    memcpy(&zone, record + 8u, sizeof(zone));
    while (count-- > 0) {
        uint32_t sequence;
        memcpy(&sequence, record + 0x1cu, sizeof(sequence));
        ++sequence;
        memcpy(record + 0x1cu, &sequence, sizeof(sequence));
        cmd_command_kind_record_issue((uint32_t)kind, (uint32_t)variant,
                                      value, cell_xy, zone, NULL, 0);
    }
}

// FUNCTION: LEGORR 0x00408ec0
void terrain_kind_nine_neighbor_emit_for_footprint_cells_current(
    const int cell_xy[2], const int* footprint_xy, uint32_t footprint_count)
{
    (void)terrain_kind_nine_neighbor_emit_for_footprint_cells(
        cell_xy, footprint_xy, footprint_count);
}
// FUNCTION: LEGORR 0x004090a0
int terrain_cell_predicate_check_current(const uint32_t pair_key[2])
{
    return terrain_descriptor_find_exact_current(pair_key, NULL) != NULL;
}
/* Retail's descriptor chain consists of the raw path-record windows. */
// FUNCTION: LEGORR 0x004090c0
void* terrain_descriptor_find_exact_current(const uint32_t* pair_key,
                                            const uint32_t* single_key)
{
    uint8_t* record = (uint8_t*)path_record_last_peek();
    while (record != NULL) {
        uint32_t first, second;
        void* previous;
        if (pair_key != NULL) {
            memcpy(&first, record + 0x0c, sizeof first);
            memcpy(&second, record + 0x10, sizeof second);
            if (first == pair_key[0] && second == pair_key[1]) return record;
        }
        else if (single_key != NULL) {
            memcpy(&first, record + 0x08, sizeof first);
            if (first == *single_key) return record;
        }
        memcpy(&previous, record + 0x24, sizeof previous);
        record = (uint8_t*)previous;
    }
    return NULL;
}

/* The two UI group operations resolve the shared DAT_004b8a24 path-record
 * list. Their entity arrays live at +0x2c/+0xcc in the 32-bit record; native
 * builds use level.cpp's widened-pointer sidecar accessors for that window. */
// FUNCTION: LEGORR 0x00409110
void simtick_selection_group_release(const uint32_t key_pair[2])
{
    void* record;
    record = terrain_descriptor_find_exact_current(key_pair, NULL);
    if (record != NULL) {
        cancel_action_for_matching_coordinates(key_pair);
#if defined(_WIN32)
        bobbin_delayed((SimtickBobbin*)record);
#else
        {
            uint32_t count = path_record_entity_count_get(record);
            uint32_t i;
            for (i = 0u; i < count; ++i) {
                EntitySlot* entity = path_record_entity_get(record, i);
                if (entity != NULL) {
                    const float no_timeout = -1.0f;
                    entity->field_340 = no_timeout;
                    entity->flags_3e8 |= 0x800000u;
                }
            }
        }
#endif
        path_record_remove_and_free(record);
    }
}

// FUNCTION: LEGORR 0x00409150
void simtick_selection_group_clear(const uint32_t key_pair[2])
{
    void* record;
    uint32_t count;
    uint32_t i;
    uint32_t group_key;
    int cell_xy[2];
    record = terrain_descriptor_find_exact_current(key_pair, NULL);
    if (record == NULL) return;
    cell_xy[0] = (int)key_pair[0];
    cell_xy[1] = (int)key_pair[1];
    terrain_cell_flag40000000_clear_or_set_current(cell_xy, 0);
#if defined(_WIN32)
    count = *(uint32_t*)((char*)record + 0xcc);
    for (i = 0u; i < count; ++i) {
        EntitySlot* entity = (EntitySlot*)
            *(uint32_t*)((char*)record + 0x2c + i * 4u);
        if (entity == NULL) continue;
        entity->flags_3ec &= ~0x100u;
        cmd_create_task(entity, 0); /* FUN_00402720(entity, 0) */
    }
#else
    count = path_record_entity_count_get(record);
    for (i = 0u; i < count; ++i) {
        EntitySlot* entity = path_record_entity_get(record, i);
        if (entity == NULL) continue;
        entity->flags_3ec &= ~0x100u;
        cmd_create_task(entity, 0); /* FUN_00402720(entity, 0) */
    }
#endif
    cancel_action_for_matching_coordinates(key_pair);
    memcpy(&group_key, (const char*)record + 0x08, sizeof(group_key));
    cmd_cancel_by_coords((int)group_key, 0, 0, 0);
    path_record_remove_and_free(record);
}

// FUNCTION: LEGORR 0x004091e0
void simtick_bobbins(EngineContext* c)
{
    (void)c;
    SimtickBobbin* node = s_bobbin_head;
    while (node != NULL) {
        uint32_t f = node->flags;
        SimtickBobbin* next = node->next;
        ++s_bobbin_advance;
        if (((f & 2u) == 0) || ((f & 4u) != 0)) {
            if ((f & 1u) != 0) {
                if ((f & 2u) == 0) {
                    if (bobbin_spawn(node)) {
                        bobbin_delayed(node);
                        bobbin_mark(node);
                        bobbin_record_remove_and_free(node);
                    }
                } else {
                    bobbin_clear_delay(node);
                    node->flags = node->flags & ~1u;
                }
            }
        } else {
            descriptor_transition_apply(node);
        }
        node = next;
    }
}
// FUNCTION: LEGORR 0x00409270
static int bobbin_spawn(SimtickBobbin* node)
{
    return node != NULL && s_bobbin_spawn_hook != NULL
        ? s_bobbin_spawn_hook(node, s_bobbin_action_context) : 0;
} /* FUN_00409270 */

// FUNCTION: LEGORR 0x00409350
void* path_record_allocate_and_link(const uint32_t* coord_pair)
{
    const size_t bytes =
        PATH_RECORD_WINDOW + sizeof(void*) - sizeof(uint32_t);
    char* rec = (char*)engine_heap_alloc((long)bytes);
    if (rec == NULL) {
        return NULL; /* deviation: the corpus strides the allocator's word */
    }
    __builtin_memset(rec, 0, bytes); /* the corpus's rep stos zero-fill  */
    /* The corpus writes these two belt words after the sweep and MSVC5
     * keeps the stores (the disassembly shows them landing blind). */
    *(uint32_t*)(rec + 0x20) = 0u;
    *(uint32_t*)(rec + 0x1c) = 0u;
    /* Keep the corpus's two counter reads separate: the record receives the
     * old word at 0x40936e, then 0x409379 reloads and increments the global. */
    *(uint32_t*)(rec + 0x08) = s_path_record_count;
    s_path_record_count = s_path_record_count + 1u;
    if (coord_pair != NULL) {
        *(uint32_t*)(rec + 0x0c) = coord_pair[0];
        *(uint32_t*)(rec + 0x10) = coord_pair[1];
    }
    *(void**)(rec + 0x24) = s_path_record_last;         /* prev link */
    s_path_record_last = rec;
    return rec;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x004093b0
void placement_cell_search_expand(const int cell_xy[2])
{
    static const int offset[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = context != NULL ? context->level : NULL;
    int direction;
    if (level == NULL) level = level_current_owner_get();
    if (level == NULL || cell_xy == NULL) return;
    for (direction = 0; direction < 4; ++direction) {
        int next_xy[2];
        int nx = cell_xy[0] + offset[direction][0];
        int ny = cell_xy[1] + offset[direction][1];
        if (!level_cell_occupied_at(level, nx, ny) &&
            !level_cell_occupied_or_flag(level, nx, ny, 1)) continue;
        if (level_cell_secondary_flag_100(level, nx, ny) != 0u) continue;
        terrain_cell_flag100_set_and_track(level, nx, ny);
        next_xy[0] = nx;
        next_xy[1] = ny;
        placement_cell_search_expand(next_xy);
    }
}

// FUNCTION: LEGORR 0x00409450
int ledger_stub_00409450_FindNearbyPlacementCandidate(
    const int start_xy[2], int resource_amount)
{
    int found = ledger_stub_00409470_NeighborCellEntitySearchRecurse(
        start_xy, resource_amount);
    terrain_cell_visited_flags_clear_all_current();
    return found;
}

// NeighborCellEntitySearchRecurse
// FUNCTION: LEGORR 0x00409470
int ledger_stub_00409470_NeighborCellEntitySearchRecurse(
    const int cell_xy[2], int resource_amount)
{
    static const int offset[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = context != NULL ? context->level : NULL;
    int direction;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL || cell_xy == NULL) return 0;
    for (direction = 0; direction < 4; ++direction) {
        int cell[2];
        EntitySlot* found;
        cell[0] = cell_xy[0] + offset[direction][0];
        cell[1] = cell_xy[1] + offset[direction][1];
        if (level_cell_secondary_bit0(level, cell[0], cell[1]) != 0u) {
            found = entity_at_grid_position_find(cell);
            if (found != NULL && entity_available_resource_spend(
                    level, resource_amount, found)) return 1;
        }
        if (!level_cell_occupied_at(level, cell[0], cell[1]) &&
            !level_cell_occupied_or_flag(level, cell[0], cell[1], 1)) continue;
        if (level_cell_visited_flag(level, cell[0], cell[1]) != 0u) continue;
        terrain_cell_visited_flag_set_and_track(level, cell[0], cell[1]);
        if (ledger_stub_00409470_NeighborCellEntitySearchRecurse(
                cell, resource_amount)) return 1;
    }
    return 0;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00409550
static void descriptor_transition_apply(SimtickBobbin* node)
{
    int* cell_xy;
    void* matching_entity;
    uint32_t kind6_count;
    uint32_t kind7_default_count;
    uint32_t kind7_variant_count;
    int kind7_variant;
    if (node == NULL || !always_accept_place_command_cell()) return;
    cell_xy = (int*)node->key_pair;
    matching_entity = unported_ledger_stubs::
        ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
        NULL, cell_xy, 7u, 0u);
    kind6_count = entity_indexed_record_word_at_b4_get(15, 0, 0);
    kind7_default_count = entity_indexed_record_word_at_b0_get(15, 0, 0);
    kind7_variant_count = entity_indexed_record_word_at_b8_get(15, 0, 0);

    entity_repeated_resource_instances_create(cell_xy, 6, 0, 0u,
                                               (int)kind6_count);
    kind7_variant = matching_entity != NULL && kind7_variant_count != 0u;
    entity_repeated_resource_instances_create(cell_xy, 7, kind7_variant,
        0u, (int)(kind7_variant ? kind7_variant_count : kind7_default_count));
    node->flags |= 4u;
} /* FUN_00409550 */

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00409600
void ledger_stub_00409600_TerrainDescriptorTilesResetAndFree(
    const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    uint8_t* descriptor;
    int* coordinates;
    uint32_t count;
    uint32_t entity_count;
    uint32_t entity_index;
    uint32_t progress;
    uint32_t descriptor_key;
    if (cell_xy == NULL || level == NULL) return;

    /* This is the target's exact pair-key descriptor lookup. */
    descriptor = (uint8_t*)terrain_descriptor_find_exact_current(
        (const uint32_t*)cell_xy, NULL);
    if (descriptor == NULL) return;
    memcpy(&progress, descriptor + 0x1c, sizeof(progress));
    if (progress == *(uint32_t*)(descriptor + 0x20)) return;
    coordinates = (int*)path_record_payload_get(descriptor);
    count = path_record_payload_count_get(descriptor);
    memcpy(&descriptor_key, descriptor + 0x08, sizeof(descriptor_key));
    if (count != 0u && coordinates == NULL) return;

    progress = 0u;
    while (progress < count) {
        int x = coordinates[progress * 2u];
        int y = coordinates[progress * 2u + 1u];
        int duplicate = progress + 1u < count &&
            x == coordinates[(progress + 1u) * 2u] &&
            y == coordinates[(progress + 1u) * 2u + 1u];
        if (!level_cell_path_record_state_apply(
                level, x, y, NULL, 0, duplicate))
            level_cell_init_pass(level, x, y);
        /* FUN_0041cfb0 follows CellInitPass for every listed pair, even if
         * the portable cell-link adapter could not represent that cell. */
        cancel_action_for_matching_coordinates(
            (const uint32_t*)coordinates + progress * 2u);
        ++progress;
    }

    entity_count = path_record_entity_count_get(descriptor);
    for (entity_index = 0u; entity_index < entity_count; ++entity_index) {
        EntitySlot* entity = path_record_entity_get(descriptor, entity_index);
        if (entity == NULL) continue;
        if (entity->type == 9) {
            entity->flags_3e8 |= 0x800000u;
            entity->field_340 = -1.0f;
        }
        else {
            entity->flags_3ec &= ~0x100u;
            cmd_create_task(entity);
        }
    }

    cmd_cancel_by_coords((int)descriptor_key, 0, 0, 0);
    path_record_release(descriptor);
}

// CellPathRecordRegisterApply
// FUNCTION: LEGORR 0x00409790
uint32_t ledger_stub_00409790_CellPathRecordRegisterApply(
    uint32_t record_key, const int cell_xy[2], uint32_t direction,
    const int* footprint_xy, uint32_t footprint_count)
{
    LevelState* level = level_current_owner_get();
    void* record;
    int* coordinate_copy = NULL;
    uint32_t index = 0u;
    if (cell_xy == NULL ||
        (footprint_count != 0u && footprint_xy == NULL) ||
        (size_t)footprint_count > ((size_t)-1) / (2u * sizeof(int)))
        return 0u;
    if (level == NULL) return 0u;

    record = path_record_allocate_and_link((const uint32_t*)cell_xy);
    if (record == NULL) return 0u;
    memcpy((char*)record + 0x00, &record_key, sizeof(record_key));
    memcpy((char*)record + 0x04, &direction, sizeof(direction));
    if (footprint_count != 0u) {
        coordinate_copy = (int*)engine_heap_alloc((long)(
            (size_t)footprint_count * 2u * sizeof(int)));
        if (coordinate_copy == NULL) {
            path_record_release(record);
            return 0u;
        }
        memcpy(coordinate_copy, footprint_xy,
               (size_t)footprint_count * 2u * sizeof(int));
    }
    if (!path_record_payload_set(record, coordinate_copy, footprint_count)) {
        engine_heap_free(coordinate_copy);
        path_record_release(record);
        return 0u;
    }
    if (footprint_count != 0u) {
        /* FUN_00409790 passes a four-unit clamp width. */
        terrain_cell_surface_height_clamp(
            level_surface_grid_get(level),
            coordinate_copy, footprint_count, 4.0f);
    }

    while (index < footprint_count) {
        int x = coordinate_copy[index * 2u];
        int y = coordinate_copy[index * 2u + 1u];
        int duplicate = index + 1u < footprint_count &&
            x == coordinate_copy[(index + 1u) * 2u] &&
            y == coordinate_copy[(index + 1u) * 2u + 1u];
        if (level != NULL) {
            if (!level_cell_path_record_state_apply(
                    level, x, y, record, 1, duplicate))
                level_cell_init_pass(level, x, y);
            terrain_cell_refresh_eight_neighbors_current(
                coordinate_copy + index * 2u);
            cancel_action_for_matching_coordinates(
                (const uint32_t*)coordinate_copy + index * 2u);
        }
        /* CellPathRecordRegisterApply advances over a repeated adjacent
         * coordinate as one logical pair even when the cell could not be
         * updated. The decompile increments its list cursor in the duplicate
         * branch before the common increment below. */
        if (duplicate) ++index;
        ++index;
    }
    return *(uint32_t*)((char*)record + 0x08);
}

// FUNCTION: LEGORR 0x00409940
void terrain_cell_refresh_eight_neighbors_current(const int cell_xy[2])
{
    /* The retail local_40 table is ordered NW, N, NE, W, E, SW, S, SE.
     * Keep the walk in the mapped body: the reference calls the mutation
     * worker once for every offset and lets that worker apply edge checks. */
    static const int offset[8][2] = {
        { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 },
        { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
    };
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    int index;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL || cell_xy == NULL) return;
    for (index = 0; index < 8; ++index) {
        const int64_t x = (int64_t)cell_xy[0] + offset[index][0];
        const int64_t y = (int64_t)cell_xy[1] + offset[index][1];
        if (x < 0 || y < 0 || x > 0x7fffffff || y > 0x7fffffff)
            continue;
        terrain_cell_mutation_progress(level, (uint32_t)x, (uint32_t)y);
    }
}
} // namespace unported_ledger_stubs

/* ReleaseAllGroupRecords (FUN_004099d0) saves each successor before the
 * unlinking/freeing leaf, so the full list can be released head-first. */
// FUNCTION: LEGORR 0x004099d0
void path_record_release_all(void)
{
    void* current = s_path_record_last;
    while (current != NULL) {
        void* next = *(void**)((char*)current + 0x24);
        path_record_remove_and_free(current);
        current = next;
    }
}
/* PathRecordRelease (FUN_004099f0): unlink the selected record from the
 * path-record chain, then release its optional owned payload and record. */
// FUNCTION: LEGORR 0x004099f0
void path_record_remove_and_free(void* record)
{
    void* previous = NULL;
    void* current = s_path_record_last;

    /* The corpus enters a do/while and tests the link before comparing the
     * requested record.  Keep that shape: an empty list goes straight to the
     * common payload/record release path. */
    do {
        if (current == NULL) goto release_record;
        if (current == record) {
            if (previous != NULL) {
                *(void**)((char*)previous + 0x24) =
                    *(void**)((char*)current + 0x24);
            }
            else {
                s_path_record_last = *(void**)((char*)current + 0x24);
            }
            goto release_record;
        }
        previous = current;
        current = *(void**)((char*)current + 0x24);
    } while (1);

release_record:
    path_record_storage_release(record);
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x00409a40
static void bobbin_delayed(SimtickBobbin* node)
{
    if (node == NULL) return;
    uint32_t count = node->member_count;
    if (count > 20u) count = 20u;
    for (uint32_t i = 0; i < count; ++i) {
        void* record = node->members[i];
        if (record == NULL) continue;
        const float no_timeout = -1.0f;
        uint32_t flags = 0;
        __builtin_memcpy((char*)record + 0x340u, &no_timeout,
                         sizeof(no_timeout));
        __builtin_memcpy(&flags, (char*)record + 0x3e8u, sizeof(flags));
        flags |= 0x800000u;
        __builtin_memcpy((char*)record + 0x3e8u, &flags, sizeof(flags));
    }
}

// FUNCTION: LEGORR 0x00409a90
void world_object_create_with_transform(const WorldObjectCreateHooks* hooks,
                                        uint32_t object_kind,
                                        const int32_t position[2],
                                        uint32_t subtype,
                                        const int32_t* coordinate_pairs,
                                        uint32_t coordinate_pair_count,
                                        int notify)
{
    WorldObjectSpawnRequest request;
    if (hooks == NULL || position == NULL) return;
    memset(&request, 0, sizeof(request));
    request.object_kind = object_kind;
    request.orientation_quadrant = subtype;
    request.origin_x = position[0];
    request.origin_y = position[1];
    request.coordinate_pairs = coordinate_pairs;
    request.coordinate_pair_count = coordinate_pair_count;
    if (hooks->terrain_pairs_prepare != NULL)
        hooks->terrain_pairs_prepare(hooks->context, coordinate_pairs,
                                     coordinate_pair_count, 4.0f);
    if (hooks->neighbor_cells_refresh != NULL)
        hooks->neighbor_cells_refresh(hooks->context, position);
    else
        unported_ledger_stubs::terrain_cell_refresh_eight_neighbors_current(
            (const int*)position);
    world_object_spawn_request_create(hooks, &request, notify);
}
// FUNCTION: LEGORR 0x00409b30
EntitySlot* world_object_spawn_request_create(
    const WorldObjectCreateHooks* hooks,
    const WorldObjectSpawnRequest* request,
    int notify)
{
    EntitySlot* entity;
    uint32_t i;
    float orientation;
    if (hooks == NULL || request == NULL ||
        hooks->entity_create_type4 == NULL) return NULL;
    orientation = (float)request->orientation_quadrant * 1.5707964f;
    entity = hooks->entity_create_type4(hooks->context, request, orientation);
    if (entity == NULL) return NULL;
    if (notify != 0 && hooks->event8_dispatch != NULL)
        hooks->event8_dispatch(hooks->context, entity);
    for (i = 0; request->coordinate_pairs != NULL &&
         i < request->coordinate_pair_count; ++i) {
        const int32_t* pair = request->coordinate_pairs + i * 2u;
        const int is_matching_pair =
            (i + 1u < request->coordinate_pair_count &&
             pair[0] == pair[2] && pair[1] == pair[3]);
        if (is_matching_pair) {
            if (hooks->matching_pair_prepare != NULL)
                hooks->matching_pair_prepare(hooks->context, pair[0], pair[1]);
            if (hooks->matching_pair_attach != NULL)
                hooks->matching_pair_attach(hooks->context, entity, pair);
            ++i;
        }
        else if ((entity_stats_word_get(entity) & 0x100000u) != 0u) {
            if (hooks->boundary_pair_special != NULL)
                hooks->boundary_pair_special(hooks->context, pair);
            else
                /* FUN_00409b30 calls FUN_004323f0 for this stats-gated
                 * branch. Use the mapped current-level entry when no host
                 * override supplies that operation. */
                terrain_cell_flags_set_and_refresh_current(
                    (const int*)pair);
        }
        else {
            if (hooks->boundary_pair_unlink != NULL)
                hooks->boundary_pair_unlink(hooks->context, pair);
            if (hooks->boundary_pair_apply != NULL)
                hooks->boundary_pair_apply(hooks->context, pair[0], pair[1]);
            if ((entity_state_flags_get(entity) & 0x200u) != 0u &&
                hooks->boundary_pair_secondary != NULL) {
                hooks->boundary_pair_secondary(hooks->context, pair);
            }
            if (hooks->cardinal_neighbors_apply != NULL)
                hooks->cardinal_neighbors_apply(hooks->context,
                                                pair[0], pair[1]);
            else
                /* FUN_00409cd0 runs the four cardinal cell refreshes here,
                 * immediately before FUN_0040cea0 refreshes this pair. The
                 * current-level entrypoint resolves the live LevelState
                 * through the same engine binding used by other retail ABI
                 * callers. */
                cell_cardinal_neighbor_apply_current((const int*)pair);
        }
        if (hooks->pair_refresh != NULL)
            hooks->pair_refresh(hooks->context, pair[0], pair[1]);
    }
    return entity;
}

// FUNCTION: LEGORR 0x00409cd0
void cell_cardinal_neighbor_apply_current(const int coord_pair[2])
{
    static const int offsets[8] = { 0,-1, 1,0, 0,1, -1,0 };
    int i;
    if (coord_pair == NULL) return;
    for (i = 0; i < 4; ++i) {
        const int pair[2] = { coord_pair[0] + offsets[i * 2],
                              coord_pair[1] + offsets[i * 2 + 1] };
        terrain_cell_refresh_dispatch_current(pair, 0);
    }
}

static void path_record_storage_release(void* record)
{
    PathRecordPayloadPort* previous = NULL;
    PathRecordPayloadPort* port;
    PathRecordEntityPort* entity_previous = NULL;
    PathRecordEntityPort* entity_port;
    if (record == NULL) return;
    path_record_cell_links_release(NULL, record);
    for (port = s_path_record_payload_ports; port != NULL; port = port->next) {
        if (port->record == record) {
            if (previous != NULL) previous->next = port->next;
            else s_path_record_payload_ports = port->next;
            engine_heap_free(port->payload);
            engine_heap_free(port);
            break;
        }
        previous = port;
    }
    for (entity_port = s_path_record_entity_ports; entity_port != NULL;
         entity_port = entity_port->next) {
        if (entity_port->record == record) {
            if (entity_previous != NULL)
                entity_previous->next = entity_port->next;
            else
                s_path_record_entity_ports = entity_port->next;
            engine_heap_free(entity_port);
            break;
        }
        entity_previous = entity_port;
    }
    if (port == NULL) engine_heap_free(path_record_payload_get(record));
}

/* The 0x408d70/0x408ce0 helpers share the live path-descriptor list owned by
 * level.cpp. The descriptor key is its serial dword at +0x08; its +0x2c list
 * contains EntitySlot pointers, mirrored by the native sidecar accessors. */
static void* cmd_path_descriptor_find(uint32_t key)
{
    void* record;
    for (record = path_record_last_peek(); record != NULL;
         record = *(void**)((char*)record + 0x24)) {
        uint32_t record_key;
        memcpy(&record_key, (char*)record + 0x08, sizeof(record_key));
        if (record_key == key) return record;
    }
    return NULL;
}

static void bobbin_mark(SimtickBobbin* node)
{
    if (node != NULL && s_bobbin_mark_hook != NULL)
        s_bobbin_mark_hook(node, s_bobbin_mark_context);
}
static void bobbin_record_remove_and_free(SimtickBobbin* node)
{
    SimtickBobbin* previous = NULL;
    SimtickBobbin* current = s_bobbin_head;
    while (current != NULL && current != node) {
        previous = current;
        current = current->next;
    }
    if (current == NULL) return;
    if (previous == NULL) s_bobbin_head = current->next;
    else previous->next = current->next;
    engine_heap_free_wrapper(current->owned_buffer);
    engine_heap_free_wrapper(current);
}
