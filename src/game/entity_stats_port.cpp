// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail of GAME-043 (Stats.c; ADR 0006 decision 4, finding 581): the
 * port's models of the unit's functions, unannotated. The original-shape
 * unit is entity_stats.c.
 *
 * Natively these definitions serve the port's callers: native objects are
 * EntitySlot, whose layout is not the original's. In the matching build the
 * unit's functions take these names (entity_stats_link_names.h), so the
 * port's definitions of them are compiled out there. The stats table they
 * read is the unit's statsGlobs (entity_stats_port_globs()), which the
 * port's loader fills through entity.cpp; the row arrays it points at are
 * this file's storage (entity_stats_port_row_table()). */
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
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
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/string.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "crt/crt.hpp"
#include "frontend.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include "mission.hpp"
#include "entity_for_entity_stats.hpp"
#include "entity_stats.h"
#include "entity_stats_port.hpp"

/* 00469c10: the port's model natively, the unit's function in the matching
 * build (entity_stats_link_names.h). */
extern "C" int entity_building_index_set(EntitySlot* entity, uint32_t state);

/* Row storage for the shared statsGlobs.objectStats[type] (the original
 * allocates the rows in Stats_Initialise; the port's loader installs into
 * these fixed rows instead). */
static StatsData* s_stats_cell_ptr[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];

/* The port's float copy of the level count, read by the port's
 * building_upgrade_admission_check as raw bits (entity.cpp writes it). The
 * original's objectLevels word is statsGlobs.objectLevels. */
float s_stats_cell_level_limit[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];

/* Lego.c (GAME-021): the number of IDs of an object type, the length of the
 * row array Stats_Initialise allocates. */
extern "C" unsigned int lego_stats_level_record_count_for_stat_type(int objType);

StatsData** entity_stats_port_row_table(int type)
{
    Stats_Globs* globs = entity_stats_port_globs();
    StatsData** rows = globs->objectStats[type];
    if (rows != s_stats_cell_ptr[type]) {
        /* In the matching build Lego.c's config init runs the original
         * Stats_Initialise, which allocates a row array of the type's ID
         * count; the port's loader writes all STATS_TYPE_TABLE_ROWS rows,
         * so the loaded rows move to this file's fixed storage. Natively
         * nothing calls Stats_Initialise and rows is NULL or ours. */
        if (rows != NULL) {
            unsigned int count = lego_stats_level_record_count_for_stat_type(type);
            unsigned int i;
            if (count > STATS_TYPE_TABLE_ROWS) count = STATS_TYPE_TABLE_ROWS;
            for (i = 0; i < count; ++i) s_stats_cell_ptr[type][i] = rows[i];
        }
        globs->objectStats[type] = s_stats_cell_ptr[type];
    }
    return globs->objectStats[type];
}

#if !(defined(_MSC_VER) && _MSC_VER < 1200)
/* The port's models of 00469a90-0046a330, native builds only; in the
 * matching build the unit's functions carry these names. */

static uint32_t stats_instance_read(const void* config_block, int off)
{
    if (config_block == NULL) {
        return 0u; /* deviation: the corpus dereferences blindly */
    }
    int type = 0, row = 0, index = 0;
    __builtin_memcpy(&type, (const uint8_t*)config_block + 0x0, sizeof(type));
    __builtin_memcpy(&row, (const uint8_t*)config_block + 0x4, sizeof(row));
    __builtin_memcpy(&index, (const uint8_t*)config_block + 0x330, sizeof(index));
    return stats_table_read(type, row, off, index);
}

/* Port model of 0x00469a90 (Stats_AddToolTaskType): append to one of the
 * eleven three-value groups. */
void entity_indexed_value_append(int group, uint32_t value)
{
    Stats_ToolTaskTypes* tool;
    if (group < 0 || group >= ENTITY_INDEXED_VALUE_GROUPS) {
        return;
    }
    tool = &entity_stats_port_globs()->toolTaskTypes[group];
    if (tool->count < ENTITY_INDEXED_VALUE_CAPACITY) {
        tool->taskTypes[tool->count] = value;
        tool->count = tool->count + 1u;
    }
}

/* Port model of 0x00469ac0 (Stats_GetCostOre). */
uint32_t entity_indexed_record_word_at_b0_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xb0u);
}

/* Port model of 0x00469af0 (Stats_GetCostCrystal). */
uint32_t entity_indexed_record_word_at_b4_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xb4u);
}

/* Port model of 0x00469b20 (Stats_GetCostRefinedOre). */
uint32_t stats_table_word_at_b8(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xb8, stride_index);
}

/* Port models of 0x00469b50-0x00469be0 (by the object's type, ID and
 * level). */
uint32_t stats_instance_word_at_bc(const void* config_block)
{
    return stats_instance_read(config_block, 0xbc);
}
uint32_t stats_instance_word_at_e0(const void* config_block)
{
    return stats_instance_read(config_block, 0xe0);
}
uint32_t stats_instance_word_at_ec(const void* config_block)
{
    return stats_instance_read(config_block, 0xec);
}
uint32_t stats_instance_word_at_f0(const void* config_block)
{
    return stats_instance_read(config_block, 0xf0);
}

/* Port model of 0x00469c10 (StatsObject_SetObjectLevel): select one of the
 * entity type's 0x150-byte records and update the two transition counters. */
extern "C" int entity_building_index_set(EntitySlot* entity, uint32_t state)
{
    int type;
    int row;
    int current;
    int span;
    const uint8_t* record;
    if (entity == NULL) return 0;
    type = entity->type;
    row = entity->etype;
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES || row < 0
        || row >= STATS_TYPE_TABLE_ROWS) {
        return 0;
    }
    span = s_stats_cell_span[type][row];
    record = (const uint8_t*)entity_stats_port_row_table(type)[row];
    if (record == NULL || state >= (uint32_t)span) return 0;
    current = entity->indexed_record_slot_330;
    if (state != (uint32_t)current) {
        if (current >= 0 && current < 16) {
            grid_count_table_slot_poke(type, row, current, 0,
                (uint32_t)(grid_count_table_slot_peek(type, row, current, 0)
                    + 1));
        }
        grid_count_table_slot_poke(type, row, (int)state, 1,
            (uint32_t)(grid_count_table_slot_peek(type, row, (int)state, 1)
                + 1));
    }
    entity->indexed_record_slot_330 = (int32_t)state;
    entity->stats_record = record + (size_t)state * 0x150u;
    return 1;
}

/* Port models of 0x00469cb0-0x0046a0a0: the object's current record. */
float entity_stats_float_at20(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x20); }
float entity_stats_float_at24_indexed(const EntitySlot* e, int index)
{
    return ENTITY_STATS_FLOAT_READ(e, 0x24 + index * 4);
}
float entity_stats_float_atd4(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xd4); }
float entity_stats_float_at9c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x9c); }
float entity_stats_float_atd8(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xd8); }
float entity_stats_float_at6c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x6c); }
float entity_stats_float_at74(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x74); }
float entity_stats_float_at78(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x78); }
float entity_stats_float_ate8(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xe8); }
float entity_stats_float_at70(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x70); }
extern "C" void* entity_stats_field_at_a0_corpus(const EntitySlot* e)
{
    /* the corpus returns the ADDRESS record+0xa0, not its contents */
    return (void*)((const uint8_t*)e->stats_record + 0xa0);
}
float entity_stats_float_at7c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x7c); }
float entity_stats_float_at80(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x80); }
float entity_energy_decay_rate_get(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x84); }

/* Port model of 0x00469e00 (Stats_GetOxygenCoef). */
float stats_table_float_at_dc(int type, int row)
{
    const uint32_t bits = stats_table_read(type, row, 0xdc, 0);
    float f = 0.0f;
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}
float entity_stats_float_atdc(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xdc); }
uint32_t entity_stats_word_atac(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0xac); }
uint32_t entity_stats_word_get(const EntitySlot* e)
{
    return ENTITY_STATS_WORD_READ(e, 0x144);
}
uint32_t entity_state_flags_get(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0x148); }
uint32_t entity_capability_flags_get(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0x14c); }

/* Port models of 0x00469ec0-0x00469f00 (Stats_GetStatsFlags1-3). */
uint32_t stats_table_word_at_144(int type, int row)
{
    return stats_table_read(type, row, 0x144, 0);
}
uint32_t stats_table_word_at_148(int type, int row)
{
    return stats_table_read(type, row, 0x148, 0);
}
uint32_t stats_table_word_at_14c(int type, int row)
{
    return stats_table_read(type, row, 0x14c, 0);
}
float entity_stats_float_at88(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x88); }

/* Port model of 0x00469f40 (Stats_GetLevels). It returns the raw word of
 * the port's float copy of the level count, as the port always has (the
 * original's word is the integer count). */
uint32_t building_upgrade_admission_check(int type, int row)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS) {
        return 0u; /* deviation: the corpus indexes the table blindly */
    }
    uint32_t raw_word = 0u;
    __builtin_memcpy(&raw_word, &s_stats_cell_level_limit[type][row],
                     sizeof(raw_word));
    return raw_word;
}

/* Port model of 0x00469f60 (Stats_GetWaterEntrances). */
uint32_t stats_table_word_at_d0(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xd0, stride_index);
}
uint32_t entity_stats_word_at_c0_or_c4(const EntitySlot* e, int selector)
{
    if (selector != 0) return ENTITY_STATS_WORD_READ(e, 0xc4);
    return ENTITY_STATS_WORD_READ(e, 0xc0);
}
uint32_t entity_stats_word_atc8(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0xc8); }
float entity_stats_float_at8c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x8c); }
float entity_stats_float_at90(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x90); }
float entity_stats_float_at98(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x98); }
float entity_stats_float_at94(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x94); }

/* Port model of 0x0046a060 (StatsObject_GetNumOfToolsCanCarry). A null
 * guard is kept for the portable representation, where an entity can
 * exist before its Stats binding. */
int ai_activity_cap(const EntitySlot* e)
{
    uint32_t record_cap = 0u;
    if (e == NULL || e->stats_record == NULL) return 0;
    memcpy(&record_cap, (const uint8_t*)e->stats_record + 0xccu,
           sizeof(record_cap));
    return (int)record_cap;
}

float entity_stats_float_atf4(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xf4); }
float entity_speed_component_get(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xf8); }

/* Port models of 0x0046a0c0 and 0x0046a0f0 (Stats_GetUpgradeCostOre,
 * Stats_GetUpgradeCostStuds). */
uint32_t entity_mapped_indexed_record_word_at_fc_get(int group, int index,
                                                     int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xfcu);
}
uint32_t stats_table_word_at_100(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0x100, stride_index);
}

/* Port model of 0x0046a120 (Stats_FindToolFromTaskType): the first group
 * containing the value. */
int entity_grouped_code_table_find_value_index(uint32_t value,
                                               int* out_group)
{
    const Stats_ToolTaskTypes* tools = entity_stats_port_globs()->toolTaskTypes;
    int group;
    for (group = 0; group < ENTITY_INDEXED_VALUE_GROUPS; ++group) {
        uint32_t i;
        for (i = 0; i < tools[group].count; ++i) {
            if (tools[group].taskTypes[i] == value) {
                if (out_group != NULL) *out_group = group;
                return 1;
            }
        }
    }
    return 0;
}

/* Port models of 0x0046a170-0x0046a310. */
float entity_stats_float_at130(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x130); }
float entity_stats_float_at134(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x134); }
float entity_stats_float_at120(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x120); }
float entity_stats_float_at128(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x128); }
float entity_stats_float_at124(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x124); }
uint32_t entity_stats_word_at13c(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0x13c); }
float entity_stats_float_at12c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x12c); }
float entity_stats_float_at138(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x138); }
float entity_collision_radius_get(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xe4); }
float entity_stats_float_at114(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x114); }
float entity_stats_float_at118(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x118); }
float entity_stats_float_at11c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x11c); }
float entity_stats_float_at110(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x110); }
float entity_stats_float_at10c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x10c); }

/* Port model of 0x0046a330 (StatsObject_Debug_ToggleSelfPowered): toggle
 * stats-record flag 0x400 and run the kind-four post-create resolver. */
void entity_record_flag400_toggle_and_refresh(EntitySlot* e)
{
    uint32_t* flags;
    if (e == NULL) return;
    flags = (uint32_t*)(stats_rec(e) + 0x148);
    *flags ^= 0x400u;
    entity_kind_four_post_create_resolve(e);
}
#endif /* !MSVC 5.0 */

/* ---- GAME-023 group D adapters ---- */
/* entity_building_index_set is declared only in the frozen
 * entity_from_entity_stats.hpp. An extern "C" pre-declaration before that
 * include in legoobject.cpp and entity.cpp shifts both units' symbol
 * counters (finding 538, rule B), so legoobject_unit.c calls this C entry
 * instead. */
// PORT-ONLY: C-linkage entry to 0x00469c10 for legoobject_unit.c (GAME-023)
extern "C" int legoobject_gd_building_index_set_adapter(EntitySlot* entity,
                                                        uint32_t state)
{
    return entity_building_index_set(entity, state);
}
