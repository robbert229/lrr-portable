// SPDX-License-Identifier: LGPL-3.0-or-later
/* Pending jobs survive loss of their assigned unit and own their payload
 * independently of the temporary copies attached to units. */
#include <cstdlib>
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/legoobject_record.h"
#include "test/lrr_test.h"

int main()
{
    aitask_cmd_registry_init_target();
    Lego_Object first = {}, second = {};
    AITask* a = aitask_cmd_alloc(12);
    AITask* b = aitask_cmd_alloc(12);
    AITask* other = aitask_cmd_alloc(12);
    a->blockX = 2;
    a->blockY = 3;
    b->blockX = 4;
    b->blockY = 1;
    a->entityRef = b->entityRef = &first;
    other->entityRef = &second;
    aitask_cmd_link_head(a, 4);
    aitask_cmd_link_head(b, 4);
    aitask_cmd_link_head(other, 4);

    aitask_pending_record_entity_reference_clear(&first);
    CHECK_EQ(a->entityRef, nullptr);
    CHECK_EQ(b->entityRef, nullptr);
    CHECK_EQ(other->entityRef, &second);
    CHECK_EQ(aiTaskGlobs.globalHead, other);
    CHECK_EQ(other->next, b);
    CHECK_EQ(b->next, a);

    a->buffer = malloc(16);
    CHECK(a->buffer != nullptr, "pending task payload allocated");
    a->entityRef = &second;
    AITask* copy = aitask_cmd_pending_record_copy(a);
    CHECK(copy != a, "entity task is a distinct record");
    CHECK_EQ(copy->blockX, 2);
    CHECK_EQ(copy->blockY, 3);
    CHECK_EQ(copy->entityRef, &second);
    CHECK_EQ(copy->priorityValue, a->priorityValue);
    CHECK_EQ(copy->buffer, a->buffer);
    CHECK_EQ(copy->next, nullptr);
    CHECK_EQ(copy->nextFree, copy);
    CHECK_EQ(copy->flags & 0x200u, 0x200u);
    aitask_cmd_free(copy, 1);
    AITask* reused = aitask_cmd_alloc(3);
    CHECK_EQ(reused, copy);
    CHECK_EQ(reused->buffer, nullptr);
    CHECK_EQ(reused->entityRef, nullptr);
    CHECK_EQ(reused->flags, 0u);

    aiTaskGlobs.globalHead = nullptr;
    for (AITask* task : {a, b, other, reused})
        aitask_cmd_free(task, 1);
    for (unsigned int i = 0; i < aiTaskGlobs.poolBlockCount; ++i)
        free(aiTaskGlobs.poolBlocks[i]);
    return lrr_test_result("task_lifecycle");
}
