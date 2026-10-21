// SPDX-License-Identifier: LGPL-3.0-or-later
// AI execution receivers (finding 10 open item) — the entity-manager tick.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"
#include "game/select.hpp"
#include "gods98/message.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static EntitySlot* place(GameContext* ctx, int type, const char* name,
                         float x, float y)
{
    int idx = entity_create(ctx, type, name, x, y, 0, 0);
    CHECK(idx >= 0, "entity_create");
    return (EntitySlot*)entity_at(ctx, type, idx);
}

/* Run ai_tick `n` times at dt, returning completions summed. */
static void run(GameContext* ctx, int n, float dt)
{
    for (int i = 0; i < n; ++i) {
        ai_tick(ctx, dt);
    }
}

int main(void)
{
    cmd_init();
    message_clear();
    message_init_defaults(); /* bus gate on (flags 0x4) + Info catalog */
    GameContext ctx;
    game_context_init(&ctx);
    cmd_context_set(&ctx);

    /* Move execution: entity at (0,0) moves to (10,0) at 2 cells/s. */
    EntitySlot* mover = place(&ctx, 1, "mover", 0, 0);
    cmd_create_move(mover, 10.0f, 0.0f, 0);
    int before = ai_commands_active(&ctx);
    CHECK(before >= 1, "move enqueued");
    run(&ctx, 2, 0.5f);
    CHECK(mover->x > 0.0f && mover->x < 10.0f, "movement advances x");
    /* dt=0.5, speed 2 -> 1 cell/tick; 10 cells -> done by tick 11. */
    run(&ctx, 12, 0.5f);
    CHECK(mover->x >= 9.9f, "movement arrives near the target");
    CHECK(mover->cmd_head == NULL, "finished move unlinked from the chain");
    CHECK(ai_commands_active(&ctx) == 0, "no live commands after the move");

    /* Target execution: A chases B. */
    EntitySlot* chaser = place(&ctx, 2, "chaser", 0, 0);
    EntitySlot* target = place(&ctx, 4, "target", 5, 5);
    cmd_create_target(chaser, (void*)target);
    run(&ctx, 20, 0.5f); /* 5 cells * sqrt2 ~ 7.07 -> ~8 ticks + margin */
    CHECK(chaser->x >= 4.9f && chaser->y >= 4.9f, "target chase arrives");
    CHECK(chaser->cmd_head == NULL, "finished target unlinked");

    /* Task execution: a gather task (kind determined by entity type) runs
     * AI_TASK_DURATION seconds then deposits into the mission counters. */
    EntitySlot* miner = place(&ctx, 1, "miner", 0, 0); /* type 1 -> kind 2 */
    cmd_create_task(miner);
    int ore0 = ctx.mission.ore;
    run(&ctx, 1, 1.0f);
    CHECK(ctx.mission.ore == ore0, "task not deposited before its duration");
    run(&ctx, 3, 1.0f); /* 2 s duration, dt=1 -> done by the 3rd tick */
    CHECK(ctx.mission.ore == ore0 + AI_TASK_YIELD, "gather deposited ore yield");
    CHECK(message_active_has(1), "ore deposit fired the OreSeamFound incident");
    CHECK(ai_commands_active(&ctx) == 0, "finished task freed");

    /* A flagged minion (kind 0x12) is not a deposit kind: no counter move. */
    EntitySlot* minion = place(&ctx, 6, "minion", 0, 0);
    minion->flags_3e8 |= 0x80000000u;
    cmd_create_task(minion); /* kind 0x12 -> no deposit effect */
    int ore1 = ctx.mission.ore;
    run(&ctx, 4, 1.0f);
    CHECK(ctx.mission.ore == ore1, "non-deposit task leaves counters alone");

    /* Queue -> executor -> ai: selection move via opcode 5 executes. */
    EntitySlot* digger = place(&ctx, 1, "digger", 0, 0);
    selection_add(&ctx, digger);
    selection_cmd_move(&ctx, 5, 5);
    cmd_queue_tick();
    run(&ctx, 20, 0.5f);
    CHECK(digger->x >= 4.9f && digger->y >= 4.9f, "queue move executes");
    CHECK(digger->cmd_head == NULL, "queue move unlinked after arrival");

    if (s_failures != 0) {
        fprintf(stderr, "ai_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("ai_test: OK\n");
    return 0;
}