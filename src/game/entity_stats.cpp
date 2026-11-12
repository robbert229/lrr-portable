// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-043 GAME-043: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-043.csv. */
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

static const void* s_stats_cell_ptr[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];
// GLOBAL: LEGORR 0x00502bd8
const void** s_stats_type_table[STATS_TYPE_TABLE_TYPES] = {
    s_stats_cell_ptr[0], s_stats_cell_ptr[1], s_stats_cell_ptr[2],
    s_stats_cell_ptr[3], s_stats_cell_ptr[4], s_stats_cell_ptr[5],
    s_stats_cell_ptr[6], s_stats_cell_ptr[7], s_stats_cell_ptr[8],
    s_stats_cell_ptr[9], s_stats_cell_ptr[10], s_stats_cell_ptr[11],
    s_stats_cell_ptr[12], s_stats_cell_ptr[13], s_stats_cell_ptr[14],
    s_stats_cell_ptr[15], s_stats_cell_ptr[16], s_stats_cell_ptr[17],
    s_stats_cell_ptr[18], s_stats_cell_ptr[19]
};

// GLOBAL: LEGORR 0x00502c28
float s_stats_cell_level_limit[STATS_TYPE_TABLE_TYPES]
                                     [STATS_TYPE_TABLE_ROWS];

// GLOBAL: LEGORR 0x005030d8
EntityIndexedValueGroup
    s_entity_indexed_value_groups[ENTITY_INDEXED_VALUE_GROUPS];

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static uint32_t stats_instance_read(const void* config_block, int off);

/* IndexedValueAppend (FUN_00469a90): append to one of the eleven
 * three-value groups; the count dword occupies the fourth word of each
 * 16-byte group in the reference table. */
// FUNCTION: LEGORR 0x00469A90
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
void entity_indexed_value_append(int group, uint32_t value)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, ecx
        shl eax, 4
        mov edx, dword ptr [eax + s_entity_indexed_value_groups + 0ch]
        lea eax, [eax + s_entity_indexed_value_groups + 0ch]
        lea ecx, [edx + ecx*4]
        mov edx, dword ptr [esp + 8]
        mov dword ptr [ecx*4 + s_entity_indexed_value_groups], edx
        mov ecx, dword ptr [eax]
        inc ecx
        mov dword ptr [eax], ecx
        ret
    }
}
#else
void entity_indexed_value_append(int group, uint32_t value)
{
    uint32_t count;
    if (group < 0 || group >= ENTITY_INDEXED_VALUE_GROUPS) {
        return;
    }
    count = s_entity_indexed_value_groups[group].count;
    if (count < ENTITY_INDEXED_VALUE_CAPACITY) {
        s_entity_indexed_value_groups[group].values[count] = value;
        s_entity_indexed_value_groups[group].count = count + 1u;
    }
}
#endif

/* IndexedRecordWordAtOffsetB0Get (FUN_00469ac0) reads record +0xb0 at a
 * 0x150-byte stride from an installed group/index table. */
// FUNCTION: LEGORR 0x00469AC0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t entity_indexed_record_word_at_b0_get(int group, int index, int slot)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0b0h]
        ret
    }
}
#else
uint32_t entity_indexed_record_word_at_b0_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xb0u);
}
#endif

/* IndexedRecordWordAtOffsetB4Get (FUN_00469af0) is the adjacent dword
 * reader used by EntityKindOneRandomSpawnTrigger. */
// FUNCTION: LEGORR 0x00469AF0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t entity_indexed_record_word_at_b4_get(int group, int index, int slot)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0b4h]
        ret
    }
}
#else
uint32_t entity_indexed_record_word_at_b4_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xb4u);
}
#endif

// FUNCTION: LEGORR 0x00469b20
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t stats_table_word_at_b8(int type, int row, int stride_index)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0b8h]
        ret
    }
}
#else
uint32_t stats_table_word_at_b8(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xb8, stride_index);
}
#endif

// FUNCTION: LEGORR 0x00469b50
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t stats_instance_word_at_bc(const void* config_block)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        mov ecx, dword ptr [eax]
        mov edx, dword ptr [eax + 4]
        mov esi, dword ptr [ecx*4 + s_stats_type_table]
        mov ecx, dword ptr [eax + 330h]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esi + edx*4]
        pop esi
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [ecx + eax + 0bch]
        ret
    }
}
#else
uint32_t stats_instance_word_at_bc(const void* config_block)
{
    return stats_instance_read(config_block, 0xbc);
}
#endif
/* FUN_00469b50 */
// FUNCTION: LEGORR 0x00469b80
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t stats_instance_word_at_e0(const void* config_block)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        mov ecx, dword ptr [eax]
        mov edx, dword ptr [eax + 4]
        mov esi, dword ptr [ecx*4 + s_stats_type_table]
        mov ecx, dword ptr [eax + 330h]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esi + edx*4]
        pop esi
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [ecx + eax + 0e0h]
        ret
    }
}
#else
uint32_t stats_instance_word_at_e0(const void* config_block)
{
    return stats_instance_read(config_block, 0xe0);
}
#endif
/* FUN_00469b80 */
// FUNCTION: LEGORR 0x00469bb0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t stats_instance_word_at_ec(const void* config_block)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        mov ecx, dword ptr [eax]
        mov edx, dword ptr [eax + 4]
        mov esi, dword ptr [ecx*4 + s_stats_type_table]
        mov ecx, dword ptr [eax + 330h]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esi + edx*4]
        pop esi
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [ecx + eax + 0ech]
        ret
    }
}
#else
uint32_t stats_instance_word_at_ec(const void* config_block)
{
    return stats_instance_read(config_block, 0xec);
}
#endif
/* FUN_00469bb0 */
// FUNCTION: LEGORR 0x00469be0
 #if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t stats_instance_word_at_f0(const void* config_block)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        mov ecx, dword ptr [eax]
        mov edx, dword ptr [eax + 4]
        mov esi, dword ptr [ecx*4 + s_stats_type_table]
        mov ecx, dword ptr [eax + 330h]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esi + edx*4]
        pop esi
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [ecx + eax + 0f0h]
        ret
    }
}
#else
uint32_t stats_instance_word_at_f0(const void* config_block)
{
    return stats_instance_read(config_block, 0xf0);
}
#endif
/* EntityBuildingIndexSet (FUN_00469c10): select one of the entity type's
 * 0x150-byte records and update the two transition counters. */
// FUNCTION: LEGORR 0x00469c10
int entity_building_index_set(EntitySlot* entity, uint32_t state)
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
    record = (const uint8_t*)s_stats_type_table[type][row];
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

// FUNCTION: LEGORR 0x00469cb0
float entity_stats_float_at20(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x20); }
// FUNCTION: LEGORR 0x00469cc0
float entity_stats_float_at24_indexed(const EntitySlot* e, int index)
{
    return ENTITY_STATS_FLOAT_READ(e, 0x24 + index * 4);
}
// FUNCTION: LEGORR 0x00469ce0
float entity_stats_float_atd4(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xd4); }
// FUNCTION: LEGORR 0x00469d00
float entity_stats_float_at9c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x9c); }
// FUNCTION: LEGORR 0x00469d20
float entity_stats_float_atd8(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xd8); }
// FUNCTION: LEGORR 0x00469d40
float entity_stats_float_at6c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x6c); }
// FUNCTION: LEGORR 0x00469d50
float entity_stats_float_at74(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x74); }
// FUNCTION: LEGORR 0x00469d60
float entity_stats_float_at78(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x78); }
// FUNCTION: LEGORR 0x00469d70
float entity_stats_float_ate8(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xe8); }

// FUNCTION: LEGORR 0x00469d90
float entity_stats_float_at70(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x70); }
// FUNCTION: LEGORR 0x00469da0
void* entity_stats_field_at_a0_corpus(const EntitySlot* e)
{
    /* the corpus returns the ADDRESS record+0xa0, not its contents */
    return (void*)((const uint8_t*)e->stats_record + 0xa0);
}

// FUNCTION: LEGORR 0x00469db0
float entity_stats_float_at7c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x7c); }
// FUNCTION: LEGORR 0x00469dc0
float entity_stats_float_at80(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x80); }
// FUNCTION: LEGORR 0x00469de0
float entity_energy_decay_rate_get(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x84); }

// FUNCTION: LEGORR 0x00469e00
float stats_table_float_at_dc(int type, int row)
{
    /* FUN_00469e00 */
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const float*)((const char*)s_stats_type_table[type][row] + 0xdc);
#else
    const uint32_t bits = stats_table_read(type, row, 0xdc, 0);
    float f = 0.0f;
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
#endif
}
// FUNCTION: LEGORR 0x00469e20
float entity_stats_float_atdc(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xdc); }
// FUNCTION: LEGORR 0x00469e40
uint32_t entity_stats_word_atac(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0xac); }

// FUNCTION: LEGORR 0x00469e60
uint32_t entity_stats_word_get(const EntitySlot* e)
{
    return ENTITY_STATS_WORD_READ(e, 0x144);
}
// FUNCTION: LEGORR 0x00469e80
uint32_t entity_state_flags_get(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0x148); }

// FUNCTION: LEGORR 0x00469ea0
uint32_t entity_capability_flags_get(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0x14c); }

// FUNCTION: LEGORR 0x00469ec0
uint32_t stats_table_word_at_144(int type, int row)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const uint32_t*)((const char*)s_stats_type_table[type][row] +
                              0x144);
#else
    return stats_table_read(type, row, 0x144, 0);
#endif
}
/* FUN_00469ec0 */
// FUNCTION: LEGORR 0x00469ee0
uint32_t stats_table_word_at_148(int type, int row)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const uint32_t*)((const char*)s_stats_type_table[type][row] +
                              0x148);
#else
    return stats_table_read(type, row, 0x148, 0);
#endif
}
/* FUN_00469ee0 */
// FUNCTION: LEGORR 0x00469f00
uint32_t stats_table_word_at_14c(int type, int row)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const uint32_t*)((const char*)s_stats_type_table[type][row] +
                              0x14c);
#else
    return stats_table_read(type, row, 0x14c, 0);
#endif
}
// FUNCTION: LEGORR 0x00469f20
float entity_stats_float_at88(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x88); }

/* BuildingUpgradeAdmissionCheck (FUN_00469f40) reads the raw word stored
 * beside the stats table. The loader writes the parsed integer as a float;
 * callers then consume that float's bits through an integer table read. */
// FUNCTION: LEGORR 0x00469f40
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t building_upgrade_admission_check(int type, int row)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        lea eax, [eax + eax*2]
        lea edx, [ecx*4]
        lea eax, [eax + eax*4]
        mov eax, dword ptr [edx + eax*4 + s_stats_cell_level_limit]
        ret
    }
}
#else
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
#endif

// FUNCTION: LEGORR 0x00469f60
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t stats_table_word_at_d0(int type, int row, int stride_index)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0d0h]
        ret
    }
}
#else
uint32_t stats_table_word_at_d0(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xd0, stride_index); /* FUN_00469f60 */
}
#endif
// FUNCTION: LEGORR 0x00469f90
uint32_t entity_stats_word_at_c0_or_c4(const EntitySlot* e, int selector)
{
    if (selector != 0) return ENTITY_STATS_WORD_READ(e, 0xc4);
    return ENTITY_STATS_WORD_READ(e, 0xc0);
}
// FUNCTION: LEGORR 0x00469fc0
uint32_t entity_stats_word_atc8(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0xc8); }
// FUNCTION: LEGORR 0x00469fe0
float entity_stats_float_at8c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x8c); }
// FUNCTION: LEGORR 0x0046a000
float entity_stats_float_at90(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x90); }
// FUNCTION: LEGORR 0x0046a020
float entity_stats_float_at98(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x98); }
// FUNCTION: LEGORR 0x0046a040
float entity_stats_float_at94(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x94); }

/* FUN_0046a060 is a raw reader: return the dword at +0xcc in the selected
 * Stats record. The EntitySlot wrapper stores that selected record directly.
 * A null guard is retained for the portable representation, where an entity
 * can exist before its Stats binding. */
// FUNCTION: LEGORR 0x0046a060
int ai_activity_cap(const EntitySlot* e)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const int*)((const uint8_t*)e->stats_record + 0xccu);
#else
    uint32_t record_cap = 0u;
    if (e == NULL || e->stats_record == NULL) return 0;
    memcpy(&record_cap, (const uint8_t*)e->stats_record + 0xccu,
           sizeof(record_cap));
    return (int)record_cap;
#endif
}

// FUNCTION: LEGORR 0x0046a080
float entity_stats_float_atf4(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xf4); }
// FUNCTION: LEGORR 0x0046a0a0
float entity_speed_component_get(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xf8); }
/* MappedIndexedRecordWordAtFCGet (FUN_0046a0c0) is the corresponding
 * +0xfc accessor over the same table. */
// FUNCTION: LEGORR 0x0046A0C0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t entity_mapped_indexed_record_word_at_fc_get(int group, int index,
                                                     int slot)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0fch]
        ret
    }
}
#else
uint32_t entity_mapped_indexed_record_word_at_fc_get(int group, int index,
                                                     int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xfcu);
}
#endif

// FUNCTION: LEGORR 0x0046a0f0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
uint32_t stats_table_word_at_100(int type, int row, int stride_index)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 100h]
        ret
    }
}
#else
uint32_t stats_table_word_at_100(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0x100, stride_index); /* FUN_0046a0f0 */
}
#endif
/* GroupedCodeTableFindValueIndex (FUN_0046a120): scan the count-prefixed
 * fixed groups in order and return the first group containing the value. */
// FUNCTION: LEGORR 0x0046A120
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
int entity_grouped_code_table_find_value_index(uint32_t value,
                                               int* out_group)
{
    __asm {
        push ebx
        push esi
        push edi
        mov edi, dword ptr [esp + 10h]
        xor esi, esi
        mov eax, offset s_entity_indexed_value_groups
    grouped_value_group_loop:
        mov edx, dword ptr [eax + 0ch]
        xor ecx, ecx
        test edx, edx
        jbe grouped_value_next_group
        mov edx, eax
    grouped_value_item_loop:
        cmp edi, dword ptr [edx]
        je grouped_value_found
        mov ebx, dword ptr [eax + 0ch]
        inc ecx
        add edx, 4
        cmp ecx, ebx
        jb grouped_value_item_loop
    grouped_value_next_group:
        add eax, 10h
        inc esi
        cmp eax, offset s_entity_indexed_value_groups + 0b0h
        jb grouped_value_group_loop
        xor eax, eax
        pop edi
        pop esi
        pop ebx
        ret
    grouped_value_found:
        mov eax, dword ptr [esp + 14h]
        mov dword ptr [eax], esi
        mov eax, 1
        pop edi
        pop esi
        pop ebx
        ret
    }
}
#else
int entity_grouped_code_table_find_value_index(uint32_t value,
                                               int* out_group)
{
    int group;
    for (group = 0; group < ENTITY_INDEXED_VALUE_GROUPS; ++group) {
        uint32_t i;
        for (i = 0; i < s_entity_indexed_value_groups[group].count; ++i) {
            if (s_entity_indexed_value_groups[group].values[i] == value) {
                if (out_group != NULL) *out_group = group;
                return 1;
            }
        }
    }
    return 0;
}
#endif

// FUNCTION: LEGORR 0x0046a170
float entity_stats_float_at130(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x130); }
// FUNCTION: LEGORR 0x0046a190
float entity_stats_float_at134(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x134); }
// FUNCTION: LEGORR 0x0046a1b0
float entity_stats_float_at120(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x120); }
// FUNCTION: LEGORR 0x0046a1d0
float entity_stats_float_at128(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x128); }
// FUNCTION: LEGORR 0x0046a1f0
float entity_stats_float_at124(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x124); }
// FUNCTION: LEGORR 0x0046a210
uint32_t entity_stats_word_at13c(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0x13c); }
// FUNCTION: LEGORR 0x0046a230
float entity_stats_float_at12c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x12c); }
// FUNCTION: LEGORR 0x0046a250
float entity_stats_float_at138(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x138); }
// FUNCTION: LEGORR 0x0046a270
float entity_collision_radius_get(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0xe4); }
// FUNCTION: LEGORR 0x0046a290
float entity_stats_float_at114(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x114); }
// FUNCTION: LEGORR 0x0046a2b0
float entity_stats_float_at118(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x118); }
// FUNCTION: LEGORR 0x0046a2d0
float entity_stats_float_at11c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x11c); }
// FUNCTION: LEGORR 0x0046a2f0
float entity_stats_float_at110(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x110); }
// FUNCTION: LEGORR 0x0046a310
float entity_stats_float_at10c(const EntitySlot* e) { return ENTITY_STATS_FLOAT_READ(e, 0x10c); }
/* EntityRecordFlag400ToggleAndRefresh (FUN_0046a330): toggle stats-record
 * flag 0x400 and run the kind-four post-create resolver. The naked body
 * hard-codes EntitySlot.stats_record at +0x334 (asserted above). */
// FUNCTION: LEGORR 0x0046A330
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void entity_record_flag400_toggle_and_refresh(EntitySlot* e)
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [edx + 334h]
        mov eax, dword ptr [ecx + 148h]
        test ah, 4
        je entity_record_flag400_toggle_raise
        and ah, 0fbh
        push edx
        mov dword ptr [ecx + 148h], eax
        call entity_kind_four_post_create_resolve
        add esp, 4
        ret
    entity_record_flag400_toggle_raise:
        or ah, 4
        push edx
        mov dword ptr [ecx + 148h], eax
        call entity_kind_four_post_create_resolve
        add esp, 4
        ret
    }
}
#else
void entity_record_flag400_toggle_and_refresh(EntitySlot* e)
{
    uint32_t* flags;
    if (e == NULL) return;
    flags = (uint32_t*)(stats_rec(e) + 0x148);
    *flags ^= 0x400u;
    entity_kind_four_post_create_resolve(e);
}
#endif

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
