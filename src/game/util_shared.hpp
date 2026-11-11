// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/util.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_UTIL_SHARED_HPP
#define LRR_GAME_UTIL_SHARED_HPP

#define STRING_TABLE_MAX_ENTRIES 256

extern const char* s_string_table_entries[STRING_TABLE_MAX_ENTRIES];
extern unsigned int s_string_table_count;

#define PENDING_RECORD_CAPACITY 256

extern uint32_t s_pending_record_rows[PENDING_RECORD_CAPACITY][5];
extern unsigned int s_pending_record_count;
extern void (*s_pending_record_release_hook)(uint32_t* row);
extern EntityLinkList** s_entity_link_lists;
extern void (*s_entity_link_node_release_hook)(EntityLinkNode*);
extern void (*s_entity_link_list_release_hook)(EntityLinkList*);

extern const char* s_bubble_action_strings[BUBBLE_ACTION_STRING_COUNT];

#endif /* LRR_GAME_UTIL_SHARED_HPP */
