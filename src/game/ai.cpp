// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <string.h>

#include "ai.hpp"
#include "cmd.hpp"
#include "mission.hpp"

/* The command record's mutable `now` doubles as the task countdown for
 * task commands (the link primitives seed it with the 1500.0 "now"
 * timestamp; the executor resets it to AI_TASK_DURATION on first run). */
#define AI_TOMBSTONE 1000.0f

static const void* rec_payload(const void* rec) { return cmd_rec_payload(rec); }

/* Yield a task's deposit into the mission counters by priority kind. */
static void task_deposit(GameContext* ctx, int kind)
{
    switch (kind) {
    case 0: /* Crystal */
        mission_set_crystals(ctx, ctx->mission.crystals + AI_TASK_YIELD);
        break;
    case 1: /* Ore */
    case 2: /* DefaultCollect */
        mission_set_ore(ctx, ctx->mission.ore + AI_TASK_YIELD);
        break;
    case 4: /* Construction */
        ctx->mission.constructions += AI_TASK_YIELD;
        break;
    default:
        break; /* other priorities have no counter effect in this subset */
    }
}

/* Finish the chain-head command on `e`: unlink from both structures, free,
 * advance the chain. */
static int finish_head(GameContext* ctx, EntitySlot* e)
{
    void* rec = e->cmd_head;
    e->cmd_head = cmd_rec_next(rec);
    cmd_global_unlink(rec);
    cmd_free(rec, 0);
    (void)ctx;
    return 1;
}

/* Move/target step: move `e` toward (tx,ty) by speed*dt; 1 when arrived. */
static int step_toward(EntitySlot* e, float tx, float ty, float dt)
{
    float dx = tx - e->x;
    float dy = ty - e->y;
    float len = (float)__builtin_sqrtf(dx * dx + dy * dy);
    if (len <= 0.001f) {
        e->x = tx;
        e->y = ty;
        return 1;
    }
    float step = AI_MOVE_SPEED_CPS * dt;
    if (step >= len) {
        e->x = tx;
        e->y = ty;
        return 1;
    }
    e->x += (dx / len) * step;
    e->y += (dy / len) * step;
    return 0;
}

int ai_tick(GameContext* ctx, float dt)
{
    if (ctx == NULL || dt <= 0.0f) {
        return 0;
    }
    int done = 0;
    /* 1. Entity chains: move + target heads are the entity-local view. */
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            EntitySlot* e = &ctx->entity[t][i];
            while (e->cmd_head != NULL) {
                void* head = e->cmd_head;
                int type = cmd_rec_type(head);
                if (type == 0) {
                    /* Move: step toward the record coordinates. */
                    if (!step_toward(e, cmd_rec_move_x(head),
                                     cmd_rec_move_y(head), dt)) {
                        break; /* still moving */
                    }
                    done += finish_head(ctx, e);
                } else if (type == 2) {
                    /* Target: step toward the payload entity. */
                    const EntitySlot* tgt = (const EntitySlot*)rec_payload(head);
                    if (tgt == NULL || step_toward(e, tgt->x, tgt->y, dt)) {
                        done += finish_head(ctx, e);
                    } else {
                        break;
                    }
                } else if (type == 3) {
                    /* Task on an entity chain: run as a timer-only command
                     * (the deposit half happens via the global pass below
                     * when the record came through CmdLinkHead). */
                    done += finish_head(ctx, e);
                } else {
                    /* Unknown command types: drop them. */
                    done += finish_head(ctx, e);
                }
            }
        }
    }
    /* 2. Global command list: task records (CmdLinkHead) run their yield.
     *    Move/target records in the global list are the same objects as the
     *    chain heads handled above — already unlinked when completed. */
    {
        void* rec = cmd_global_head();
        while (rec != NULL) {
            void* next = cmd_rec_next(rec);
            int type = cmd_rec_type(rec);
            if (type == 3) {
                float now = cmd_rec_now(rec);
                if (now <= 0.0f || now >= AI_TOMBSTONE) {
                    now = AI_TASK_DURATION; /* first run: seed the timer */
                }
                now -= dt;
                if (now <= 0.0f) {
                    task_deposit(ctx, cmd_rec_kind(rec));
                    cmd_global_unlink(rec);
                    cmd_free(rec, 0);
                    ++done;
                } else {
                    cmd_rec_set_now(rec, now);
                }
            }
            rec = next;
        }
    }
    return done;
}

int ai_commands_active(const GameContext* ctx)
{
    int n = 0;
    if (ctx == NULL) {
        return 0;
    }
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            const EntitySlot* e = &ctx->entity[t][i];
            for (void* rec = e->cmd_head; rec != NULL; rec = cmd_rec_next(rec)) {
                ++n;
            }
        }
    }
    return n;
}
