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
#include "game/level.hpp"
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
    CHECK((mover->flags_3e0 & 0x8u) == 0, "arrival clears the +0x3e0 move flag");

    /* --- path list (corpus +0x24 mirror; creator seeding, arrival pop) --- */

    /* A move command seeds a one-entry path and raises the mover flag. */
    EntitySlot* walker = place(&ctx, 1, "walker", 0, 0);
    cmd_create_move(walker, 4.0f, 0.0f, 0);
    CHECK(walker->wp_count == 1 && walker->wp[0].pending == 1,
          "move seeds a one-entry path list");
    CHECK((walker->flags_3e0 & 0x8u) != 0, "move sets the +0x3e0 mover flag");
    run(&ctx, 2, 0.5f); /* 2 of 4 cells */
    CHECK(walker->wp_index == 0 && walker->wp[0].pending == 1,
          "mid-path entry stays pending (pop is arrival-only)");
    run(&ctx, 3, 0.5f);
    CHECK(walker->x >= 3.9f, "walker arrives at the waypoint");
    CHECK(walker->wp[0].pending == 0 && walker->wp_index == 1,
          "arrival pops the pending flag in place (corpus pop semantics)");
    CHECK((walker->flags_3e0 & 0x8u) == 0 && walker->cmd_head == NULL,
          "path exhaustion completes the command and clears the flag");

    /* Multi-waypoint: the tick walks entries in order, popping each. */
    EntitySlot* courier = place(&ctx, 1, "courier", 0, 0);
    cmd_create_move(courier, 4.0f, 0.0f, 0);
    CHECK(ai_path_append(courier, 4.0f, 4.0f) == 1, "waypoint appended");
    run(&ctx, 4, 0.5f); /* exactly the first hop */
    CHECK(courier->x >= 3.9f && courier->y < 1.0f, "first waypoint reached");
    CHECK(courier->wp_index == 1 && courier->wp[0].pending == 0 &&
              courier->wp[1].pending == 1 && courier->cmd_head != NULL,
          "pop advances to the second entry, command stays live");
    CHECK((courier->flags_3e0 & 0x8u) != 0, "mid-path mover flag stays set");
    run(&ctx, 6, 0.5f);
    CHECK(courier->x >= 3.9f && courier->y >= 3.9f, "second waypoint reached");
    CHECK(courier->wp[1].pending == 0 && courier->cmd_head == NULL,
          "final pop completes the command");
    CHECK(ai_path_append(courier, 0.0f, 0.0f) == 0,
          "a fully popped path restarts the buffer");

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

    /* --- verified callback branches (decompile 2026-10-21, finding 10) --- */

    /* Activity table + FUN_004409e0 set semantics. */
    CHECK(ai_activity_name(AI_ACT_COLLECT) != NULL &&
              strcmp(ai_activity_name(AI_ACT_COLLECT),
                     "Activity_Collect") == 0,
          "activity id 15 is Activity_Collect");
    CHECK(ai_activity_name(0) != NULL &&
              strcmp(ai_activity_name(0), "Activity_Stand") == 0,
          "activity id 0 is Activity_Stand");
    CHECK(ai_activity_name(-1) == NULL && ai_activity_name(77) == NULL,
          "out-of-range activity ids have no name");
    EntitySlot* actor = place(&ctx, 3, "actor", 0, 0);
    ai_set_activity(actor, AI_ACT_COLLECT, 7);
    CHECK(actor->activity == AI_ACT_COLLECT && actor->activity_param == 7 &&
              actor->activity_timer == 0.0f,
          "ai_set_activity mirrors +0x2e8/+0x2d8/+0x2d4");
    ai_set_activity(actor, 999, 0);
    CHECK(actor->activity == AI_ACT_COLLECT, "out-of-range id ignored");

    /* Activity completion (0x43c845 + FUN_00447c40 bookkeeping). */
    actor->flags_3ec |= 0x00200000u;
    run(&ctx, 1, 0.5f);
    CHECK((actor->flags_3ec & 0x00200000u) == 0, "activity-done flag cleared");
    CHECK(actor->activity == AI_ACTIVITY_NONE &&
              actor->activity_last == AI_ACTIVITY_NONE,
          "activity + last reset by bookkeeping");
    CHECK(actor->cmd_head != NULL && cmd_rec_type(actor->cmd_head) == 0xe,
          "idle record 0xe attached (FUN_00403f30)");
    int idle_rec = ai_commands_active(&ctx);
    run(&ctx, 1, 0.5f);
    CHECK(ai_commands_active(&ctx) == idle_rec, "idle record is not consumed");
    actor->flags_3ec |= 0x00200000u;
    run(&ctx, 1, 0.5f);
    CHECK(actor->cmd_head != NULL &&
              cmd_rec_type(actor->cmd_head) == 0xe &&
              cmd_rec_next(actor->cmd_head) == NULL,
          "repeat completion: early-return path attaches nothing");

    /* Wake timer (0x43c870): flags tick but movement freezes. */
    EntitySlot* sleeper = place(&ctx, 1, "sleeper", 0, 0);
    cmd_create_move(sleeper, 100.0f, 0.0f, 0);
    sleeper->flags_3e4 |= 0x08000000u;
    sleeper->wake_timer = 0.3f;
    run(&ctx, 1, 0.5f);
    CHECK(sleeper->x == 0.0f, "wake state early-returns (no movement)");
    CHECK(sleeper->wake_timer > -0.3f && sleeper->wake_timer < -0.1f,
          "wake timer still decrements");
    sleeper->flags_3e4 = 0;
    run(&ctx, 1, 0.5f);
    CHECK(sleeper->x > 0.0f, "movement resumes when wake flag clears");

    /* Target acquisition (0x43c8b2). */
    EntitySlot* linker = place(&ctx, 1, "linker", 0, 0);
    EntitySlot* twin = place(&ctx, 2, "twin", 3, 3);
    cmd_create_target(linker, (void*)twin);
    linker->flags_3ec |= 0x00040000u;
    run(&ctx, 1, 0.5f);
    CHECK((linker->flags_3ec & 0x00040000u) == 0, "acquire flag consumed");
    CHECK((linker->flags_3ec & 0x00080000u) != 0, "link flag set");
    CHECK(linker->link_target == (void*)twin, "link handle mirrors payload");

    /* Cheat-word flag (0x43c90b): type-2 can-do probe = activity in
     * {drill, teleport, reinforce} (FUN_0044c530 -> FUN_0043c440). */
    EntitySlot* rider = place(&ctx, 2, "rider", 0, 0);
    rider->activity = AI_ACT_DRILL;
    ctx.cheat = 1;
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) != 0, "cheat + drill activity sets 0x20");
    rider->activity = AI_ACT_WALK;
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) == 0, "non-can-do activity clears 0x20");
    ctx.cheat = 0;
    rider->activity = AI_ACT_REINFORCE;
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) == 0, "flag cleared when cheat is off");
    rider->activity = AI_ACTIVITY_NONE;

    /* Attached wake mirror (FUN_0044c110): an expired wake timer wakes
     * the +0x364 target — +0x340 = 100.0 and +0x3e8 bit 0x800000 — unless
     * the target sits in the attached special state (+0x3e0 bit 0x200000). */
    EntitySlot* waker = place(&ctx, 1, "waker", 0, 0);
    EntitySlot* wakee = place(&ctx, 1, "wakee", 0, 0);
    waker->flags_3e4 |= 0x08000000u;
    waker->wake_timer = -0.1f; /* already expired */
    waker->wake_target = wakee;
    run(&ctx, 1, 0.5f);
    CHECK(wakee->field_340 == 100.0f, "wake mirror writes +0x340 = 100");
    CHECK((wakee->flags_3e8 & 0x00800000u) != 0, "wake raises +0x3e8 bit 0x800000");
    wakee->field_340 = 0.0f;
    wakee->flags_3e8 = 0;
    wakee->flags_3e0 |= 0x00200000u; /* attached special state gates it out */
    run(&ctx, 1, 0.5f);
    CHECK(wakee->field_340 == 0.0f && (wakee->flags_3e8 & 0x00800000u) == 0,
          "attached-state target is not re-woken");
    wakee->flags_3e0 = 0;
    waker->flags_3e4 = 0;

    /* Idle registration (0x43c94d + FUN_0043a960 table/cap). */
    EntitySlot* idler = place(&ctx, 1, "idler", 0, 0);
    int idle0 = ai_idle_count();
    idler->flags_3ec |= 0x40u;
    run(&ctx, 1, 0.5f);
    CHECK(ai_idle_count() == idle0 + 1, "idle table registration");
    CHECK((idler->flags_3ec & 0x40u) == 0, "idle flag cleared");
    idler->flags_3ec |= 0x40u;
    run(&ctx, 1, 0.5f);
    CHECK(ai_idle_count() == idle0 + 1, "no duplicate registration");

    /* --- tile-map wiring (FUN_00432770 / FUN_00432e00 / '\t' probe) ----- */

    {
        static uint8_t voxels[16 * 16 * 0x48];
        LevelState map;
        memset(&map, 0, sizeof(map));
        memset(voxels, 0, sizeof(voxels));
        level_set_int(&map, LEVEL_OFF_WIDTH, 16);
        level_set_int(&map, LEVEL_OFF_HEIGHT, 16);
        level_store_ptr(&map, LEVEL_OFF_VOXELS, voxels);
        ctx.level = &map;

        /* Unwalkable start (no 0x10 layer bit): the move command is live
         * but the walker must not budge; the mover state marks the cell. */
        EntitySlot* stuck = place(&ctx, 1, "stuck", 0, 0);
        cmd_create_move(stuck, 4.0f, 0.0f, 0);
        run(&ctx, 2, 0.5f);
        CHECK(stuck->x == 0.0f && stuck->cmd_head != NULL,
              "unwalkable cell freezes the walker");
        uint32_t* f00 = (uint32_t*)(voxels + 0 * 16 * 0x48 + 0 * 0x48 + 8);
        CHECK((*f00 & 0x8000000u) != 0, "move state marks the cell occupied");

        /* Blocked by type: walkable flags everywhere but a type-7 tile at
         * the walker's cell keeps it put. */
        for (int cy = 0; cy < 3; ++cy) {
            for (int cx = 0; cx < 8; ++cx) {
                uint32_t* f = (uint32_t*)(voxels + (size_t)cy * 16 * 0x48 +
                                          (size_t)cx * 0x48 + 8);
                *f |= 0x10u;
            }
        }
        voxels[0 * 16 * 0x48 + 0 * 0x48 + 2] = 7; /* blocked type */
        run(&ctx, 2, 0.5f);
        CHECK(stuck->x == 0.0f, "type-7 tile keeps the walker on the tile");
        voxels[0 * 16 * 0x48 + 0 * 0x48 + 2] = 0;
        run(&ctx, 5, 0.5f);
        CHECK(stuck->x >= 3.9f && stuck->cmd_head == NULL,
              "unblocked walker finishes the move");
        uint32_t* f40 = (uint32_t*)(voxels + (size_t)(4 * 0x48) + 8);
        CHECK((*f40 & 0x8000000u) == 0, "arrival clears the occupancy flag");
        CHECK(*f00 & 0x8000000u, "the departure cell stays marked (corpus "
                                 "clears only the arrival cell)");

        /* Pathfinder builder: a dispatch on a clean grid reroutes around
         * a blocked column, filling a multi-entry list the walker walks
         * cell by cell without ever landing on the wall. */
        memset(voxels, 0, sizeof(voxels));
        for (int cy = 0; cy <= 13; ++cy) {
            for (int cx = 0; cx <= 13; ++cx) {
                *(uint32_t*)(voxels + ((size_t)cy * 16 + cx) * 0x48 + 8) |= 0x10u;
            }
        }
        for (int cy = 1; cy <= 5; ++cy) {
            *(uint32_t*)(voxels + ((size_t)cy * 16 + 5) * 0x48 + 8) |= 0x40u;
        }
        EntitySlot* detour = place(&ctx, 1, "detour", 1, 3);
        cmd_create_move(detour, 9.0f, 3.0f, 0);
        CHECK(detour->wp_count > 1, "pathfinder fills a multi-entry list");
        int arrived = 0;
        for (int t = 0; t < 40 && !arrived; ++t) {
            run(&ctx, 1, 0.5f);
            int px = (int)(detour->x + 0.5f), py = (int)(detour->y + 0.5f);
            CHECK(!(px == 5 && py >= 1 && py <= 5),
                  "the detour never lands on the blocked column");
            arrived = detour->cmd_head == NULL;
        }
        CHECK(arrived && detour->x == 9.0f && detour->y == 3.0f,
              "the walker reroutes around the blocked column");

        /* Idle probe (FUN_0043a960): the flag registers only on '\t' cells. */
        EntitySlot* idler2 = place(&ctx, 1, "idler2", 8, 0);
        int idle1 = ai_idle_count();
        idler2->flags_3ec |= 0x40u;
        run(&ctx, 1, 0.5f);
        CHECK(ai_idle_count() == idle1,
              "idle registration blocked off the '\\t' tile");
        voxels[(size_t)0 * 16 * 0x48 + (size_t)8 * 0x48 + 2] = '\t';
        idler2->flags_3ec |= 0x40u;
        run(&ctx, 1, 0.5f);
        CHECK(ai_idle_count() == idle1 + 1, "idle registration on '\\t' tile");

        ctx.level = NULL;
        (void)f00;
    }

    if (s_failures != 0) {
        fprintf(stderr, "ai_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("ai_test: OK\n");
    return 0;
}