// SPDX-License-Identifier: LGPL-3.0-or-later
#include "weapon.hpp"
#include "gods98/heap.hpp"

#include <string.h>

namespace lego {

namespace {

WeaponDefinition s_weapon_definitions[WEAPON_TYPE_CAPACITY];
uint32_t s_weapon_type_count = 0;
void* s_weapon_config = 0;
WeaponDefinitionsParseHook s_weapon_parse_hook = 0;
WeaponIndexedTypeRecord* s_weapon_owned_records = 0;
// GLOBAL: LEGORR 0x00503878
static WeaponIndexedTypeRecord* s_weapon_indexed_records;
static uint32_t s_weapon_indexed_record_count;

void weapon_definition_reset(WeaponDefinition* definition)
{
    unsigned int i;
    memset(definition, 0, sizeof(*definition));
    definition->range = 150.0f;
    for (i = 0; i < WEAPON_WALL_TYPE_COUNT; ++i)
        definition->wall_destroy_time[i] = 5.0f;
}

int ascii_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z') return ch + ('a' - 'A');
    return ch;
}

int ascii_case_equal(const char* a, const char* b)
{
    if (a == 0 || b == 0) return 0;
    while (*a != '\0' && *b != '\0') {
        if (ascii_lower((unsigned char)*a) != ascii_lower((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}

const WeaponDefinition* weapon_definition_at(int type)
{
    if (type < 0 || (uint32_t)type >= s_weapon_type_count) return 0;
    return &s_weapon_definitions[type];
}

int weapon_indexed_entity_state_read(const void* entity_record,
                                     WeaponIndexedEntityState* state)
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(entity_record);
    if (bytes == 0 || state == 0) return 0;
    memcpy(&state->entity_type, bytes, sizeof(state->entity_type));
    memcpy(&state->selected_index, bytes + 4, sizeof(state->selected_index));
    memcpy(&state->level_index, bytes + 0x330, sizeof(state->level_index));
    return 1;
}

int weapon_indexed_cell_index(const WeaponIndexedEntityState* state,
                              uint32_t* index_out)
{
    uint32_t index;
    uint32_t level;
    uint32_t entity_type;
    uint32_t selected_index;
    if (state == 0 || index_out == 0 || state->entity_type < 0 ||
        state->selected_index < 0 || state->level_index < 0) return 0;
    entity_type = (uint32_t)state->entity_type;
    selected_index = (uint32_t)state->selected_index;
    level = (uint32_t)state->level_index;
    if (entity_type > (WEAPON_INDEXED_GRID_FLOAT_COUNT - 1u) / 0xf0u)
        return 0;
    index = entity_type * 0xf0u;
    if (selected_index >
        (WEAPON_INDEXED_GRID_FLOAT_COUNT - 1u - index) / 0x10u)
        return 0;
    index += selected_index * 0x10u;
    if (level > WEAPON_INDEXED_GRID_FLOAT_COUNT - 1u - index) return 0;
    *index_out = index + level;
    return 1;
}

} // namespace

void weapon_definitions_parse_hook_set(WeaponDefinitionsParseHook hook)
{
    s_weapon_parse_hook = hook;
}

/* The original walks the block-text `WeaponTypes` section and fills a much
 * larger type/level grid. This portable owner preserves the directly used
 * footer fields and lets the existing config layer provide parsed entries. */
// FUNCTION: LEGORR 0x0046EB80
int weapon_type_config_load(void* config, void* context)
{
    uint32_t i;
    uint32_t count = 0;
    WeaponIndexedTypeRecord* records;
    s_weapon_config = config;
    s_weapon_type_count = 0;
    for (i = 0; i < WEAPON_TYPE_CAPACITY; ++i)
        weapon_definition_reset(&s_weapon_definitions[i]);
    if (s_weapon_parse_hook == 0) return 0;
    if (!s_weapon_parse_hook(config, context, s_weapon_definitions,
                             WEAPON_TYPE_CAPACITY, &count)) return 0;
    if (count > WEAPON_TYPE_CAPACITY) return 0;

    /* Construct the reference's indexed table from the fields available to
     * this parser. Its omitted per-cell config values start at the reference
     * sentinel (-1.0); parsed default damage and scalar footer values remain
     * live and are therefore available to 0x46f1d0 and the scalar getters. */
    records = count != 0 ? (WeaponIndexedTypeRecord*)engine_heap_alloc(
        (long)((size_t)count * sizeof(WeaponIndexedTypeRecord))) : 0;
    if (count != 0 && records == 0) return 0;
    for (i = 0; i < count; ++i) {
        uint32_t cell;
        WeaponIndexedTypeRecord* record = &records[i];
        for (cell = 0; cell < WEAPON_INDEXED_GRID_FLOAT_COUNT; ++cell)
            record->indexed_values[cell] = -1.0f;
        record->dispatch_mode = 0;
        record->dispatch_scale = 0.0f;
        record->dispatch_index = 0;
        record->recharge_time = s_weapon_definitions[i].recharge_time;
        record->default_damage = s_weapon_definitions[i].default_damage;
        record->discharge_rate = s_weapon_definitions[i].discharge_rate;
        record->unknown_scalar = 0.0f;
        record->range = s_weapon_definitions[i].range;
        for (cell = 0; cell < WEAPON_WALL_TYPE_COUNT; ++cell)
            record->wall_destroy_time[cell] =
                s_weapon_definitions[i].wall_destroy_time[cell];
    }
    if (s_weapon_owned_records != 0)
        engine_heap_free(s_weapon_owned_records);
    s_weapon_owned_records = records;
    s_weapon_indexed_records = records;
    s_weapon_indexed_record_count = count;
    s_weapon_type_count = count;
    return 1;
}

// FUNCTION: LEGORR 0x0046F0D0
uint32_t object_type_name_index_find(const char* name)
{
    uint32_t i;
    if (s_weapon_type_count == 0) return 1;
    for (i = 0; i < s_weapon_type_count; ++i) {
        if (ascii_case_equal(s_weapon_definitions[i].name, name)) return i;
    }
    return s_weapon_type_count + 1;
}

// FUNCTION: LEGORR 0x0046F110
float weapon_type_recharge_time_get(int type)
{
    const WeaponDefinition* definition = weapon_definition_at(type);
    return definition != 0 ? definition->recharge_time : 0.0f;
}

// FUNCTION: LEGORR 0x0046F140
float weapon_type_discharge_rate_get(int type)
{
    const WeaponDefinition* definition = weapon_definition_at(type);
    return definition != 0 ? definition->discharge_rate : 0.0f;
}

// FUNCTION: LEGORR 0x0046F170
float weapon_type_range_get(int type)
{
    const WeaponDefinition* definition = weapon_definition_at(type);
    return definition != 0 ? definition->range : 0.0f;
}

// FUNCTION: LEGORR 0x0046F1A0
float weapon_type_wall_destroy_time_get(int type, int wall_type)
{
    const WeaponDefinition* definition = weapon_definition_at(type);
    if (definition == 0 || wall_type < 0 ||
        wall_type >= WEAPON_WALL_TYPE_COUNT) return 0.0f;
    return definition->wall_destroy_time[wall_type];
}

uint32_t weapon_type_count_get(void)
{
    return s_weapon_type_count;
}

void weapon_indexed_float_table_install(
    const WeaponIndexedTypeRecord* records, uint32_t record_count)
{
    if (records != s_weapon_owned_records && s_weapon_owned_records != 0) {
        engine_heap_free(s_weapon_owned_records);
        s_weapon_owned_records = 0;
    }
    s_weapon_indexed_records = (WeaponIndexedTypeRecord*)records;
    s_weapon_indexed_record_count = records != 0 ? record_count : 0u;
}

float weapon_indexed_float_lookup(
    int weapon_type, const WeaponIndexedEntityState* entity_state)
{
    const WeaponIndexedTypeRecord* record;
    int32_t entity_type;
    int32_t selected_index;
    int32_t level_index;
    int32_t cell_index;
    float value;
    if (entity_state == 0 || weapon_type < 0 ||
        (uint32_t)weapon_type >= WEAPON_TYPE_CAPACITY)
        return -1.0f;
    entity_type = entity_state->entity_type;
    selected_index = entity_state->selected_index;
    level_index = entity_state->level_index;
    if (entity_type < 0 || selected_index < 0 || level_index < 0)
        return -1.0f;
    /* The reference's full type loader owns the 0x4b68-stride table. Until
     * that renderer/config path installs its indexed grid, the parsed port
     * definition still provides the exact +0x4b10 fallback field. */
    if (s_weapon_indexed_records == 0 ||
        (uint32_t)weapon_type >= s_weapon_indexed_record_count) {
        return (uint32_t)weapon_type < s_weapon_type_count
            ? s_weapon_definitions[weapon_type].default_damage : -1.0f;
    }
    cell_index = level_index +
        (entity_type * 3 + selected_index + entity_type * 0xc) * 0x10;
    if ((uint32_t)cell_index >= WEAPON_INDEXED_GRID_FLOAT_COUNT)
        return -1.0f;
    record = &s_weapon_indexed_records[weapon_type];
    value = record->indexed_values[cell_index];
    if (value == -1.0f) return record->default_damage;
    return value;
}

// FUNCTION: LEGORR 0x0046f1d0
float entity_indexed_float_lookup_with_minus_one_fallback(
    int weapon_type, const void* entity_record)
{
    const int32_t* fields = (const int32_t*)entity_record;
    const WeaponIndexedTypeRecord* record;
    int32_t entity_type;
    int32_t selected_index;
    int32_t level_index;
    int32_t cell_index;
    float value;
    if (fields == 0 || weapon_type < 0 ||
        (uint32_t)weapon_type >= s_weapon_indexed_record_count ||
        s_weapon_indexed_records == 0)
        return weapon_type >= 0 &&
               (uint32_t)weapon_type < s_weapon_type_count
            ? s_weapon_definitions[weapon_type].default_damage : -1.0f;
    entity_type = fields[0];
    selected_index = fields[1];
    level_index = fields[0xcc];
    if (entity_type < 0 || selected_index < 0 || level_index < 0)
        return -1.0f;
    cell_index = level_index +
        (entity_type * 3 + selected_index + entity_type * 0xc) * 0x10;
    if ((uint32_t)cell_index >= WEAPON_INDEXED_GRID_FLOAT_COUNT)
        return -1.0f;
    record = &s_weapon_indexed_records[weapon_type];
    value = record->indexed_values[cell_index];
    if (value == -1.0f) return record->default_damage;
    return value;
}

} // namespace lego
