// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_OPERATION_GROUP_HPP
#define LEGORRR_GAME_OPERATION_GROUP_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OPERATION_GROUP_CAPACITY 64
#define OPERATION_GROUP_MEMBER_CAPACITY 64

typedef struct OperationGroupRecord {
    uint32_t key;
    uint32_t expected_count;
    uint32_t current_count;
    uint32_t member_count;
    uint32_t flags;
    float progress;
    void* members[OPERATION_GROUP_MEMBER_CAPACITY];
} OperationGroupRecord;

typedef struct OperationGroupTable {
    OperationGroupRecord records[OPERATION_GROUP_CAPACITY];
    uint32_t count;
    void (*missing_group_feedback)(uint32_t feedback_kind, void* object);
    void (*mark_member_flag)(void* object, uint32_t mask);
} OperationGroupTable;

void operation_group_table_clear(OperationGroupTable* table);
int operation_group_register(OperationGroupTable* table, uint32_t key,
                             uint32_t expected_count);
void operation_group_missing_feedback_set(OperationGroupTable* table,
                                          void (*callback)(uint32_t, void*));
void operation_group_member_flag_hook_set(OperationGroupTable* table,
                                          void (*callback)(void*, uint32_t));
void operation_group_append_object(OperationGroupTable* table, uint32_t key,
                                   void* object, int object_type); /* FUN_00408ce0 */

#ifdef __cplusplus
}
#endif

#endif
