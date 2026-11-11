// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/cmd.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_CMD_SHARED_HPP
#define LRR_GAME_CMD_SHARED_HPP

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

#endif /* LRR_GAME_CMD_SHARED_HPP */
