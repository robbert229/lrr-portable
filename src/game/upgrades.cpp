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
#include "upgrades_for_lego.hpp"
#include "upgrades_for_interface.hpp"

namespace lego {

namespace {

LevelCounterState s_default_counter_state;
} // namespace
LevelCounterState* s_counter_state = &s_default_counter_state;
namespace {
UpgradeAverageCountHook s_average_count_hook = 0;
UpgradeAverageValueHook s_average_value_hook = 0;
} // namespace
UpgradeConfigEntryHook s_upgrade_entry_hook = 0;
uint32_t (*s_upgrade_entry_count_hook)(void*) = 0;
uint32_t s_level_upgrade_names[UPGRADE_NAME_SLOT_COUNT];
char* s_level_upgrade_name_strings[UPGRADE_NAME_SLOT_COUNT];
UpgradeTypeRecord* s_upgrade_types;
char** s_upgrade_type_names;
uint32_t s_upgrade_type_count = 0;
UpgradeTypeLoadHook s_upgrade_type_load_hook = 0;
void (*s_building_upgrade_begin_hook)(void) = 0;
int (*s_building_upgrade_changed_hook)(int, int, int) = 0;
void (*s_building_upgrade_apply_hook)(int, int, int) = 0;
BuildingUpgradeEventHook s_building_upgrade_event_hook = 0;

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

void level_upgrade_config_hooks_set(UpgradeConfigEntryHook entry_hook,
                                    uint32_t (*entry_count_hook)(void*))
{
    s_upgrade_entry_hook = entry_hook;
    s_upgrade_entry_count_hook = entry_count_hook;
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
