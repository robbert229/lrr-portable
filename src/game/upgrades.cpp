// SPDX-License-Identifier: LGPL-3.0-or-later
#include "upgrades.hpp"

#include <string.h>

namespace lego {

namespace {

LevelCounterState s_default_counter_state;
LevelCounterState* s_counter_state = &s_default_counter_state;
UpgradeAverageCountHook s_average_count_hook = 0;
UpgradeAverageValueHook s_average_value_hook = 0;
UpgradeConfigEntryHook s_upgrade_entry_hook = 0;
uint32_t (*s_upgrade_entry_count_hook)(void*) = 0;
uint32_t s_level_upgrade_names[UPGRADE_NAME_SLOT_COUNT];
UpgradeTypeRecord s_upgrade_types[UPGRADE_TYPE_CAPACITY];
uint32_t s_upgrade_type_count = 0;
UpgradeTypeLoadHook s_upgrade_type_load_hook = 0;
void (*s_building_upgrade_begin_hook)(void) = 0;
int (*s_building_upgrade_changed_hook)(int, int, int) = 0;
void (*s_building_upgrade_apply_hook)(int, int, int) = 0;
BuildingUpgradeEventHook s_building_upgrade_event_hook = 0;

int ascii_lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

int prefix_equal(const char* value, const char* prefix)
{
    if (value == 0 || prefix == 0) return 0;
    while (*prefix != '\0') {
        if (*value == '\0' || ascii_lower((unsigned char)*value) !=
                               ascii_lower((unsigned char)*prefix)) return 0;
        ++value;
        ++prefix;
    }
    return 1;
}

int parse_nonnegative_decimal(const char* text, uint32_t* value_out)
{
    uint32_t value = 0;
    if (text == 0 || *text == '\0') return 0;
    while (*text != '\0') {
        if (*text < '0' || *text > '9') return 0;
        value = value * 10 + (uint32_t)(*text - '0');
        if (value > 1000000U) return 0;
        ++text;
    }
    *value_out = value;
    return 1;
}

} // namespace

void upgrade_average_hooks_set(UpgradeAverageCountHook count_hook,
                               UpgradeAverageValueHook value_hook)
{
    s_average_count_hook = count_hook;
    s_average_value_hook = value_hook;
}

// FUNCTION: LEGORR 0x00418120
int upgrade_average_is_above_99(void)
{
    uint32_t count;
    uint32_t i;
    uint32_t total = 0;
    float value;
    if (s_average_count_hook == 0 || s_average_value_hook == 0) return 0;
    count = (uint32_t)s_average_count_hook();
    if (count <= 8) return 0;
    for (i = 8; i < count; ++i) {
        if (!s_average_value_hook(i, &value)) return 0;
        if (value > 0.0f) total += (uint32_t)value;
        else if (value < 0.0f) total -= (uint32_t)(-value);
    }
    return total / (count - 8) > 99;
}

void level_counter_state_set(LevelCounterState* state,
                             uint32_t dynamic_a_count,
                             uint32_t dynamic_b_count)
{
    if (state == 0) {
        s_counter_state = &s_default_counter_state;
        s_counter_state->dynamic_a_count = 0;
        s_counter_state->dynamic_b_count = 0;
        return;
    }
    s_counter_state = state;
    s_counter_state->dynamic_a_count = dynamic_a_count > 256 ? 256 : dynamic_a_count;
    s_counter_state->dynamic_b_count = dynamic_b_count > 256 ? 256 : dynamic_b_count;
}

// FUNCTION: LEGORR 0x0041A920
void level_state_counters_clear(void)
{
    LevelCounterState* state = s_counter_state;
    if (state == 0) return;
    memset(state->fixed_a, 0, sizeof(state->fixed_a));
    memset(state->fixed_b, 0, sizeof(state->fixed_b));
    memset(state->table_a, 0, sizeof(state->table_a));
    memset(state->table_b, 0, sizeof(state->table_b));
    state->state_counter = 0;
    if (state->dynamic_a_count > 256) state->dynamic_a_count = 256;
    if (state->dynamic_b_count > 256) state->dynamic_b_count = 256;
    memset(state->dynamic_a, 0, state->dynamic_a_count * sizeof(uint32_t));
    memset(state->dynamic_b, 0, state->dynamic_a_count * sizeof(uint32_t));
    memset(state->dynamic_c, 0, state->dynamic_b_count * sizeof(uint32_t));
    memset(state->dynamic_d, 0, state->dynamic_b_count * sizeof(uint32_t));
    memset(state->trailing, 0, sizeof(state->trailing));
}

void level_upgrade_config_hooks_set(UpgradeConfigEntryHook entry_hook,
                                    uint32_t (*entry_count_hook)(void*))
{
    s_upgrade_entry_hook = entry_hook;
    s_upgrade_entry_count_hook = entry_count_hook;
}

// FUNCTION: LEGORR 0x00427F80
void level_upgrade_name_strings_load(void* config)
{
    uint32_t i;
    uint32_t count;
    char key[128];
    uint32_t value;
    for (i = 0; i < UPGRADE_NAME_SLOT_COUNT; ++i) s_level_upgrade_names[i] = 0;
    if (s_upgrade_entry_hook == 0 || s_upgrade_entry_count_hook == 0) return;
    count = s_upgrade_entry_count_hook(config);
    if (count > 4096) count = 4096;
    for (i = 0; i < count; ++i) {
        const char* suffix;
        uint32_t index;
        key[0] = '\0';
        value = 0;
        if (!s_upgrade_entry_hook(config, i, key, sizeof(key), &value)) continue;
        key[sizeof(key) - 1] = '\0';
        if (!prefix_equal(key, "Level")) continue;
        suffix = key + 5;
        if (!parse_nonnegative_decimal(suffix, &index)) continue;
        if (index < UPGRADE_NAME_SLOT_COUNT) s_level_upgrade_names[index] = value;
    }
}

uint32_t level_upgrade_name_value_get(uint32_t index)
{
    if (index >= UPGRADE_NAME_SLOT_COUNT) return 0;
    return s_level_upgrade_names[index];
}

void upgrade_type_load_hook_set(UpgradeTypeLoadHook hook)
{
    s_upgrade_type_load_hook = hook;
}

// FUNCTION: LEGORR 0x0042D460
int upgrade_types_load(void)
{
    uint32_t count;
    uint32_t i;
    s_upgrade_type_count = 0;
    memset(s_upgrade_types, 0, sizeof(s_upgrade_types));
    if (s_upgrade_type_load_hook == 0) return 0;
    count = s_upgrade_type_load_hook(s_upgrade_types, UPGRADE_TYPE_CAPACITY);
    if (count > UPGRADE_TYPE_CAPACITY) {
        memset(s_upgrade_types, 0, sizeof(s_upgrade_types));
        return 0;
    }
    for (i = 0; i < count; ++i)
        s_upgrade_types[i].name[sizeof(s_upgrade_types[i].name) - 1] = '\0';
    s_upgrade_type_count = count;
    return count != 0;
}

uint32_t upgrade_type_count_get(void)
{
    return s_upgrade_type_count;
}

const UpgradeTypeRecord* upgrade_type_record_get(uint32_t index)
{
    return index < s_upgrade_type_count ? &s_upgrade_types[index] : 0;
}

void building_upgrade_event_hooks_set(
    void (*begin_hook)(void),
    int (*already_changed_hook)(int type, int row, int column),
    void (*apply_hook)(int type, int row, int column),
    BuildingUpgradeEventHook event_hook)
{
    s_building_upgrade_begin_hook = begin_hook;
    s_building_upgrade_changed_hook = already_changed_hook;
    s_building_upgrade_apply_hook = apply_hook;
    s_building_upgrade_event_hook = event_hook;
}

// FUNCTION: LEGORR 0x00418830
void building_upgrade_state_changed_dispatch(int type, int row, int column,
                                             int state)
{
    if (s_building_upgrade_begin_hook != 0) s_building_upgrade_begin_hook();
    if (s_building_upgrade_changed_hook != 0 &&
        s_building_upgrade_changed_hook(type, row, column) != 0) return;
    if (s_building_upgrade_apply_hook != 0)
        s_building_upgrade_apply_hook(type, row, column);
    if (s_building_upgrade_event_hook != 0)
        s_building_upgrade_event_hook(type, row, column, state);
}

} // namespace lego
