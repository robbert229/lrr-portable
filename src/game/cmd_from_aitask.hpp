// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what cmd.cpp uses from aitask.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by cmd.cpp only. */
#ifndef LRR_GAME_CMD_FROM_AITASK_HPP
#define LRR_GAME_CMD_FROM_AITASK_HPP

#include "cmd_for_aitask.hpp"

namespace unported_ledger_stubs {
int ledger_stub_00404fc0_PlaceCommandEligibilityCheck( const ObjectTaskEligibilityRecord* command, EntitySlot* entity, float screen_xy[2], float world_xy[2], uint32_t* out_flag, int require_command_gate, int relaxed_entity_gate);
} // namespace unported_ledger_stubs
void cmd_registry_init_target(void);
extern CmdRec* s_blocks[CMD_POOL_MAX_BLOCKS];
extern void* s_free;
extern int s_pool_level;
extern CmdRec* s_global_head;
extern unsigned s_pending_timer_count;
extern int s_priority_state[PRIORITY_ENTRY_COUNT];
extern const char* s_task_names[31];
extern const char* s_priority_names[27];
extern int32_t s_behaviour[27];
void cmd_link_head(CmdRec* rec, int kind);

#endif /* LRR_GAME_CMD_FROM_AITASK_HPP */
