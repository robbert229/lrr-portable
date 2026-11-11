// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what lava_erosion.cpp sees from
 * cmd.cpp. A verbatim snapshot of cmd_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_CMD_FOR_LAVA_EROSION_HPP
#define LRR_GAME_CMD_FOR_LAVA_EROSION_HPP

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

#endif /* LRR_GAME_CMD_FOR_LAVA_EROSION_HPP */
