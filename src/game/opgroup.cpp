// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "opgroup.hpp"

void operation_group_table_clear(OperationGroupTable* table)
{
    if (table != NULL) memset(table, 0, sizeof(*table));
}

int operation_group_register(OperationGroupTable* table, uint32_t key,
                             uint32_t expected_count)
{
    OperationGroupRecord* record;
    if (table == NULL || table->count >= OPERATION_GROUP_CAPACITY) return 0;
    record = &table->records[table->count++];
    memset(record, 0, sizeof(*record));
    record->key = key;
    record->expected_count = expected_count;
    return 1;
}

void operation_group_missing_feedback_set(OperationGroupTable* table,
                                          void (*callback)(uint32_t, void*))
{
    if (table != NULL) table->missing_group_feedback = callback;
}

void operation_group_member_flag_hook_set(OperationGroupTable* table,
                                          void (*callback)(void*, uint32_t))
{
    if (table != NULL) table->mark_member_flag = callback;
}

void operation_group_append_object(OperationGroupTable* table, uint32_t key,
                                   void* object, int object_type)
{
    uint32_t i;
    OperationGroupRecord* record = NULL;
    if (table == NULL) return;
    for (i = 0; i < table->count; ++i) {
        if (table->records[i].key == key) {
            record = &table->records[i];
            break;
        }
    }
    if (record == NULL) {
        if (object != NULL && table->missing_group_feedback != NULL)
            table->missing_group_feedback(object_type == 6 ? 0x16u : 0x1cu,
                                          object);
        return;
    }
    if (object != NULL && record->member_count < OPERATION_GROUP_MEMBER_CAPACITY) {
        if (table->mark_member_flag != NULL)
            table->mark_member_flag(object, 0x100u);
        record->members[record->member_count++] = object;
        record->current_count++;
    }
    if (record->expected_count == record->current_count) {
        record->progress = 125.0f;
        record->flags |= 1u;
    }
}
