// SPDX-License-Identifier: LGPL-3.0-or-later
#include "upgrades.hpp"
#include "cmd.hpp"
#include "frontend.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "port_loop_ports.hpp"
#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/input.hpp"
#include "gods98/string.hpp"
#include "util.hpp"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "upgrades_for_helpwindow.hpp"

namespace lego {

namespace {

LevelCounterState s_default_counter_state;
LevelCounterState* s_counter_state = &s_default_counter_state;
UpgradeAverageCountHook s_average_count_hook = 0;
UpgradeAverageValueHook s_average_value_hook = 0;
UpgradeConfigEntryHook s_upgrade_entry_hook = 0;
uint32_t (*s_upgrade_entry_count_hook)(void*) = 0;
uint32_t s_level_upgrade_names[UPGRADE_NAME_SLOT_COUNT];
char* s_level_upgrade_name_strings[UPGRADE_NAME_SLOT_COUNT];
UpgradeTypeRecord* s_upgrade_types;
char** s_upgrade_type_names;
uint32_t s_upgrade_type_count = 0;
UpgradeTypeLoadHook s_upgrade_type_load_hook = 0;
} // namespace
void (*s_building_upgrade_begin_hook)(void) = 0;
int (*s_building_upgrade_changed_hook)(int, int, int) = 0;
void (*s_building_upgrade_apply_hook)(int, int, int) = 0;
BuildingUpgradeEventHook s_building_upgrade_event_hook = 0;
namespace {

int ascii_lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

void upgrade_type_tables_release(void)
{
    uint32_t i;
    for (i = 0u; i < s_upgrade_type_count; ++i)
        engine_heap_free(s_upgrade_type_names != 0
            ? s_upgrade_type_names[i] : 0);
    engine_heap_free(s_upgrade_type_names);
    engine_heap_free(s_upgrade_types);
    s_upgrade_type_names = 0;
    s_upgrade_types = 0;
    s_upgrade_type_count = 0u;
    upgrade_name_table_install(0, 0, 0);
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
    EventTimerState frontend_state;
    uint32_t i;
    LevelCounterState* state = s_counter_state;
    frontend_event_timer_state_get(&frontend_state);
    frontend_state.flags = 0u;
    frontend_event_timer_state_set(&frontend_state);
    input_action_level_state_clear();
    for (i = 0u; i < 300u; ++i)
        cell_action_permission_table_set((int)i, 0u);
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
    objective_selection_state_reset();
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
    if (s_upgrade_entry_hook != 0 && s_upgrade_entry_count_hook != 0) {
        count = s_upgrade_entry_count_hook(config);
        if (count > 4096) count = 4096;
        for (i = 0; i < count; ++i) {
            const char* suffix;
            uint32_t index;
            key[0] = '\0';
            value = 0;
            if (!s_upgrade_entry_hook(config, i, key, sizeof(key), &value))
                continue;
            key[sizeof(key) - 1] = '\0';
            if (!prefix_equal(key, "Level")) continue;
            suffix = key + 5;
            /* FUN_00427f80 passes the suffix straight to FUN_0048db70/atoi.
             * Keep atoi's prefix parsing and empty/non-numeric result of 0. */
            index = (uint32_t)atoi(suffix);
            if (index < UPGRADE_NAME_SLOT_COUNT) {
                if (s_level_upgrade_name_strings[index] != 0) {
                    engine_heap_free(s_level_upgrade_name_strings[index]);
                    s_level_upgrade_name_strings[index] = 0;
                }
                s_level_upgrade_names[index] = value;
            }
        }
        return;
    }

    /* GameConfigInit passes the live Lego.cfg block to FUN_00427f80. The
     * reference resolves UpgradeNames from its config root, then duplicates
     * each matching value string into the global 16-entry pointer table. */
    {
        BlockRec* group = config != 0
            ? block_subgroup((BlockFile*)config,
                  string_path_join(util_root_name((BlockFile*)config),
                                   "UpgradeNames", (const char*)NULL)) : 0;
        BlockRec* entry;
        for (entry = group; entry != 0;
             entry = block_next_equal_key(entry)) {
            const char* suffix;
            const char* entry_name = entry->key;
            const char* entry_value = entry->value;
            uint32_t index;
            char* copy;
            if (strnicmp_lite("Level", entry_name, 5) != 0) continue;
            suffix = entry_name + 5;
            index = (uint32_t)atoi(suffix);
            if (index >= UPGRADE_NAME_SLOT_COUNT) continue;
            copy = string_duplicate(entry_value);
            if (s_level_upgrade_name_strings[index] != 0) {
                engine_heap_free(s_level_upgrade_name_strings[index]);
                s_level_upgrade_name_strings[index] = 0;
            }
            s_level_upgrade_name_strings[index] = copy;
            /* The retail global is a 32-bit pointer table. Keep that image
             * on x86; on wider hosts callers use the pointer sidecar getter. */
            s_level_upgrade_names[index] = sizeof(void*) == sizeof(uint32_t)
                ? (uint32_t)(uintptr_t)copy : 0u;
        }
    }
}

uint32_t level_upgrade_name_value_get(uint32_t index)
{
    if (index >= UPGRADE_NAME_SLOT_COUNT) return 0;
    return s_level_upgrade_names[index];
}

const char* level_upgrade_name_string_get(uint32_t index)
{
    if (index >= UPGRADE_NAME_SLOT_COUNT) return 0;
    return s_level_upgrade_name_strings[index];
}

void upgrade_type_load_hook_set(UpgradeTypeLoadHook hook)
{
    s_upgrade_type_load_hook = hook;
}

// FUNCTION: LEGORR 0x0042D460
int upgrade_types_load(void* config)
{
    BlockRec* group;
    BlockRec* entry;
    uint32_t count;
    uint32_t i;
    upgrade_type_tables_release();

    /* Retain the explicit adapter for hosts that provide complete retail
     * records. The production config path below owns the table and names. */
    if (s_upgrade_type_load_hook != 0) {
        UpgradeTypeRecord loaded[UPGRADE_TYPE_CAPACITY];
        memset(loaded, 0, sizeof(loaded));
        count = s_upgrade_type_load_hook(loaded, UPGRADE_TYPE_CAPACITY);
        if (count == 0u || count > UPGRADE_TYPE_CAPACITY) return 0;
        s_upgrade_types = (UpgradeTypeRecord*)engine_heap_alloc(
            (long)(count * sizeof(UpgradeTypeRecord)));
        s_upgrade_type_names = (char**)engine_heap_alloc(
            (long)(count * sizeof(char*)));
        if (s_upgrade_types == 0 || s_upgrade_type_names == 0) {
            engine_heap_free(s_upgrade_types);
            engine_heap_free(s_upgrade_type_names);
            s_upgrade_types = 0;
            s_upgrade_type_names = 0;
            return 0;
        }
        memset(s_upgrade_type_names, 0, count * sizeof(char*));
        memcpy(s_upgrade_types, loaded, count * sizeof(UpgradeTypeRecord));
        s_upgrade_type_count = count;
        upgrade_name_table_install(s_upgrade_types,
            (const char* const*)s_upgrade_type_names, (int)count);
        return 1;
    }

    group = config != 0
        ? block_subgroup((BlockFile*)config,
              string_path_join(util_root_name((BlockFile*)config),
                               "UpgradeTypes", (const char*)NULL)) : 0;
    count = 0u;
    for (entry = group; entry != 0;
         entry = block_next_equal_key(entry)) {
        if (count == 0xffffffffu) return 0;
        ++count;
    }
    if (count == 0u) return 0;
    if (count > (uint32_t)(LONG_MAX / (long)sizeof(UpgradeTypeRecord)) ||
        count > (uint32_t)(LONG_MAX / (long)sizeof(char*))) return 0;

    s_upgrade_types = (UpgradeTypeRecord*)engine_heap_alloc(
        (long)(count * sizeof(UpgradeTypeRecord)));
    if (s_upgrade_types == 0) return 0;
    memset(s_upgrade_types, 0, count * sizeof(UpgradeTypeRecord));
    s_upgrade_type_names = (char**)engine_heap_alloc(
        (long)(count * sizeof(char*)));
    if (s_upgrade_type_names == 0) {
        engine_heap_free(s_upgrade_types);
        s_upgrade_types = 0;
        return 0;
    }
    memset(s_upgrade_type_names, 0, count * sizeof(char*));

    i = 0u;
    for (entry = group; entry != 0;
         entry = block_next_equal_key(entry), ++i) {
        const char* entry_name = block_name(entry);
        char* name_copy;
        if (entry_name == 0) goto load_failed;
        name_copy = string_duplicate(entry_name);
        if (name_copy == 0) goto load_failed;
        /* FUN_0046c030 zeroes the 0x10-byte row, writes the zero-based
         * ordinal at +0 and attempts LWO, ACT, then MESH resource creation
         * into +4. The renderer factory has no portable scene-object
         * equivalent, so preserve the row and name while leaving +4 null. */
        s_upgrade_types[i].index = i;
        s_upgrade_type_names[i] = name_copy;
        s_upgrade_type_count = i + 1u;
    }
    upgrade_name_table_install(s_upgrade_types,
        (const char* const*)s_upgrade_type_names, (int)s_upgrade_type_count);
    return 1;

load_failed:
    upgrade_type_tables_release();
    return 0;
}

uint32_t upgrade_type_count_get(void)
{
    return s_upgrade_type_count;
}

const UpgradeTypeRecord* upgrade_type_record_get(uint32_t index)
{
    return index < s_upgrade_type_count ? &s_upgrade_types[index] : 0;
}

const char* upgrade_type_name_get(uint32_t index)
{
    return index < s_upgrade_type_count && s_upgrade_type_names != 0
        ? s_upgrade_type_names[index] : 0;
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

} // namespace lego

#include "port_loop_ports.hpp"

/* UpgradeLevelTableParse (FUN_0046c160): enumerate the sixteen level groups
 * in order, append each parsed row to the fixed 1000-record table, and OR
 * each present group's numeric index into the destination mask word. */
void upgrade_level_table_parse(UpgradeLevelTable* table, void* context,
                               UpgradeLevelEntryReadHook read_entry)
{
    uint32_t level;
    if (table == NULL) return;
    memset(table, 0, sizeof(*table));
    if (read_entry == NULL) return;
    for (level = 0; level < 16u; ++level) {
        uint32_t ordinal = 0;
        for (;;) {
            UpgradeLevelEntry entry;
            memset(&entry, 0, sizeof(entry));
            if (!read_entry(context, level, ordinal, &entry)) break;
            if (table->count >= 1000u) return;
            entry.level = level;
            table->entries[table->count++] = entry;
            table->level_mask |= level;
            ++ordinal;
        }
    }
}

namespace unported_ledger_stubs {

/* Adapter only: UpgradeLevelTableParse (FUN_0046c160) receives the live
 * table record and two config owners, then parses the sixteen Upgrades level
 * groups. This hook-driven port table does not match that record/config ABI. */
void ledger_stub_0046c160_UpgradeLevelTableParse(
    UpgradeLevelTable* table, void* context,
    UpgradeLevelEntryReadHook read_entry)
{
    upgrade_level_table_parse(table, context, read_entry);
}



} // namespace unported_ledger_stubs
