// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_priorities — a configured AI priority list, swapping and moving
 * entries to the front, toggling a row and restoring the configured order. */
#include "game/aitask.h"
#include "game/priorities.h"
#include "test/lrr_test.h"

extern "C" void __wrap_media_action_feedback_sound_dispatch(int) {}

static void check_order(int first, int second, int third)
{
    AITask task = {};
    aitask_cmd_set_kind(&task, first);
    CHECK_EQ(task.priorityValue, 80);
    aitask_cmd_set_kind(&task, second);
    CHECK_EQ(task.priorityValue, 75);
    aitask_cmd_set_kind(&task, third);
    CHECK_EQ(task.priorityValue, 70);
}

int main(void)
{
    struct ConfigEntry {
        char key[25];
        char value[6];
        unsigned int depth;
    } entries[] = {
        { "LegoRR", "{", 0 },
        { "TestLevel", "{", 1 },
        { "Priorities", "{", 2 },
        { "AI_Priority_Construction", "FALSE", 3 },
        { "AI_Priority_Repair", "TRUE", 3 },
        { "AI_Priority_Reinforce", "FALSE", 3 },
        { "End", "", 0 },
    };
    constexpr size_t count = sizeof(entries) / sizeof(entries[0]);
    BlockRec config[count] = {};
    for (size_t i = 0; i < count; ++i) {
        config[i].key = entries[i].key;
        config[i].value = entries[i].value;
        config[i].depth = entries[i].depth;
        if (i != 0) config[i].prev = &config[i - 1];
        if (i + 1 < count) config[i].next = &config[i + 1];
    }

    aitask_cmd_registry_init_target();
    int construction = 0, repair = 0, reinforce = 0;
    CHECK(aitask_cmd_kind_by_name(entries[3].key, &construction), "construction is registered");
    CHECK(aitask_cmd_kind_by_name(entries[4].key, &repair), "repair is registered");
    CHECK(aitask_cmd_kind_by_name(entries[5].key, &reinforce), "reinforce is registered");
    CHECK_EQ(priorities_load(config, "LegoRR", "TestLevel"), 1);
    check_order(construction, repair, reinforce);
    CHECK_EQ(priority_entry_state_get(construction), 1);
    CHECK_EQ(priority_entry_state_get(repair), 0);
    CHECK_EQ(priority_entry_state_get(reinforce), 1);

    priorities_move_to_top(2);
    check_order(reinforce, construction, repair);
    priorities_move_up(1);
    check_order(construction, reinforce, repair);
    priorities_move_up(0);
    priorities_move_to_top(3);
    check_order(construction, reinforce, repair);

    priorities_toggle(2);
    CHECK_EQ(priority_entry_state_get(repair), 1);
    priorities_toggle(0);
    CHECK_EQ(priority_entry_state_get(construction), 0);
    priorities_reset();
    check_order(construction, repair, reinforce);
    CHECK_EQ(priority_entry_state_get(construction), 1);
    CHECK_EQ(priority_entry_state_get(repair), 0);
    return lrr_test_result();
}
