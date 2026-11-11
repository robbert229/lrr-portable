// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what placement.cpp sees from
 * cmd.cpp. A verbatim snapshot of cmd_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_CMD_FOR_PLACEMENT_HPP
#define LRR_GAME_CMD_FOR_PLACEMENT_HPP

extern GameContext* s_ctx;
extern float s_sweep_timer;

void cmd_operation_group_append_object(uint32_t key, EntitySlot* entity);

#define IDLE_TABLE_MAX 20

typedef struct IdleQueueRecord {
    EntitySlot* entity;
    float timer;
} IdleQueueRecord;

void idle_register(EntitySlot* e);
extern IdleQueueRecord s_idle[IDLE_TABLE_MAX];

/* --- command sample stats -----------------------------------------------
 * CommandSampleStatsAccumulate (FUN_0046b660) operates on one 0x27c-byte
 * record per type. The record's twenty sample pointers begin at +0x20c;
 * its count/head/window-total/cumulative-total/high-water words live at
 * +0x25c..+0x26c. These words overlap the timed-effect table owned by the
 * frontend module, so update the shared record returned by its storage API.
 * command_sample_stats_peek() exposes the compact 0..24 inspection view.
 * The reference does not clamp the pointer-log index at twenty. The port
 * keeps the log within its known twenty-word allocation because the
 * reference's later writes can corrupt other timed-effect records and the
 * decompile provides no bound for writes beyond the 40-record table. */
#define CMD_STATS_KINDS 40u

#define CMD_STATS_LOG_DWORD 131u /* +0x20c */

#define CMD_STATS_LOG_DWORDS 20u

#define CMD_STATS_COUNT_DWORD 151u /* +0x25c */

#define CMD_STATS_HEAD_DWORD 152u /* +0x260 */

#define CMD_STATS_WINDOW_DWORD 153u /* +0x264 */

#define CMD_STATS_TOTAL_DWORD 154u /* +0x268 */

#define CMD_STATS_HIGH_DWORD 155u /* +0x26c */

#endif /* LRR_GAME_CMD_FOR_PLACEMENT_HPP */
