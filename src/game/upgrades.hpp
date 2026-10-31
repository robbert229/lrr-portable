// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_UPGRADES_HPP
#define LEGORRR_GAME_UPGRADES_HPP

#include <stdint.h>

namespace lego {

enum { UPGRADE_NAME_SLOT_COUNT = 16, UPGRADE_TYPE_CAPACITY = 64 };

struct UpgradeTypeRecord {
    char name[64];
    void* resource;
};

struct LevelCounterState {
    uint32_t fixed_a[0x4a];
    uint32_t fixed_b[0x4a];
    uint32_t table_a[0x4a];
    uint32_t table_b[0x4a];
    uint32_t dynamic_a[256];
    uint32_t dynamic_b[256];
    uint32_t dynamic_c[256];
    uint32_t dynamic_d[256];
    uint32_t trailing[300];
    uint32_t state_counter;
    uint32_t dynamic_a_count;
    uint32_t dynamic_b_count;
};

typedef int (*UpgradeAverageCountHook)(void);
typedef int (*UpgradeAverageValueHook)(uint32_t index, float* value_out);
void upgrade_average_hooks_set(UpgradeAverageCountHook count_hook,
                               UpgradeAverageValueHook value_hook);
int upgrade_average_is_above_99(void); /* FUN_00418120 */

void level_counter_state_set(LevelCounterState* state,
                             uint32_t dynamic_a_count,
                             uint32_t dynamic_b_count);
void level_state_counters_clear(void); /* FUN_0041a920 */

typedef int (*UpgradeConfigEntryHook)(void* config, uint32_t index,
                                      char* key_out, uint32_t key_capacity,
                                      uint32_t* value_out);
void level_upgrade_config_hooks_set(UpgradeConfigEntryHook entry_hook,
                                    uint32_t (*entry_count_hook)(void*));
void level_upgrade_name_strings_load(void* config); /* FUN_00427f80 */
uint32_t level_upgrade_name_value_get(uint32_t index);

typedef uint32_t (*UpgradeTypeLoadHook)(UpgradeTypeRecord* records,
                                        uint32_t capacity);
void upgrade_type_load_hook_set(UpgradeTypeLoadHook hook);
int upgrade_types_load(void); /* FUN_0042d460 */
uint32_t upgrade_type_count_get(void);
const UpgradeTypeRecord* upgrade_type_record_get(uint32_t index);

typedef void (*BuildingUpgradeEventHook)(int type, int row, int column,
                                         int state);
void building_upgrade_event_hooks_set(
    void (*begin_hook)(void),
    int (*already_changed_hook)(int type, int row, int column),
    void (*apply_hook)(int type, int row, int column),
    BuildingUpgradeEventHook event_hook);
void building_upgrade_state_changed_dispatch(int type, int row, int column,
                                             int state); /* FUN_00418830 */

} // namespace lego

#endif
