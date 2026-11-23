// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_aitask — registry defaults, priority overrides, pending task
 * matching, and timer updates for ordinary and protected tasks. */
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/legoobject_record.h"
#include "test/lrr_test.h"

int main(void)
{
    aitask_cmd_registry_init_target();
    int priority = -1;
    CHECK_EQ(aitask_cmd_kind_by_name("ai_priority_construction", &priority), 1);
    CHECK_EQ(priority, 4);
    CHECK_EQ(aitask_cmd_kind_by_name("missing", &priority), 0);
    CHECK_EQ(priority, 4);
    priority_entry_state_set(priority, 1);
    CHECK_EQ(priority_entry_state_get(priority), 1);
    CHECK_EQ(priority_entry_state_get(3), 0);

    Lego_Object target = {};
    AITask byObject = {};
    byObject.type = 3;
    byObject.object = &target;
    AITask byCell = {};
    byCell.type = 3;
    byCell.blockX = 2;
    byCell.blockY = 4;
    AITask wrongType = byCell;
    wrongType.type = 5;
    wrongType.object = &target;

    aitask_cmd_link_head(&byObject, priority);
    CHECK_EQ(byObject.priorityValue, 60);
    priority_entry_order_value_set(priority, 24);
    aitask_cmd_link_head(&byCell, priority);
    aitask_cmd_link_head(&wrongType, priority);
    CHECK_EQ(aiTaskGlobs.globalHead, &wrongType);
    CHECK_EQ(wrongType.next, &byCell);
    CHECK_EQ(byCell.next, &byObject);
    CHECK_EQ(byCell.priorityValue, 24);

    const int cell[] = { 2, 4 };
    aitask_cmd_mark_matching(3, &target, cell);
    aitask_cmd_set_kind(&byObject, priority);
    aitask_cmd_set_kind(&byCell, priority);
    aitask_cmd_set_kind(&wrongType, priority);
    CHECK_EQ(byObject.priorityValue, 99);
    CHECK_EQ(byCell.priorityValue, 99);
    CHECK_EQ(wrongType.priorityValue, 24);

    byObject.timeIn = 8.0f;
    byCell.timeIn = 8.0f;
    byCell.flags |= 0x80; // Keep the task's lifetime timer.
    float elapsed = 2.0f;
    CHECK_EQ(aitask_cmd_pool_timer_delta_apply(&byObject, &elapsed), 0);
    CHECK_EQ(aitask_cmd_pool_timer_delta_apply(&byCell, &elapsed), 0);
    CHECK_EQ(byObject.time, 1498.0f);
    CHECK_EQ(byCell.time, 1500.0f);
    CHECK_EQ(byObject.timeIn, 6.0f);
    CHECK_EQ(byCell.timeIn, 6.0f);
    aiTaskGlobs.globalHead = nullptr;
    return lrr_test_result("aitask");
}
