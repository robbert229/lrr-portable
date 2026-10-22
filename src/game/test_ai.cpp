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

    /* Cheat-word flag (0x43c90b): the type-2 "can do" probe FUN_0044c530 =
     * FUN_0043c440 over the entity's permitted-activity list (+0x370 slots,
     * count +0x384, scanned by FUN_0044b480) for drill/teleport/reinforce —
     * not the single current-activity field. Type-2 spawns seed the list
     * with Activity_Stand (spawn-dispatch grant, 0x437eb6). */
    EntitySlot* rider = place(&ctx, 2, "rider", 0, 0);
    CHECK(ai_activity_scan(rider, AI_ACT_STAND) == 1 &&
              ai_activity_probe(rider) == 0,
          "type-2 spawn seeds Stand; Stand is not a can-do");
    ctx.cheat = 1;
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) == 0, "cheat + Stand-only list clears 0x20");
    ai_activity_grant(&ctx, rider, AI_ACT_DRILL);
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) != 0, "cheat + drill grant sets 0x20");
    /* A special (4/5/7) grant replaces the first special entry in place
     * (write at 0x44b463): no append, count unchanged, still can-do. */
    ai_activity_grant(&ctx, rider, AI_ACT_TELEPORT);
    CHECK(ai_activity_scan(rider, AI_ACT_DRILL) == 0 &&
              ai_activity_scan(rider, AI_ACT_TELEPORT) == 1,
          "special grant replaces the special entry in place");
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) != 0, "cheat + teleport grant sets 0x20");
    /* Clearing the list ends the cheat: FUN_0044b480's scan answers 0 and
     * the flag drops on the next tick. */
    rider->act_count = 0;
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) == 0, "list clear drops the cheat flag");
    /* A reinforce-only list still answers the probe (result 3). */
    ai_activity_grant(&ctx, rider, AI_ACT_REINFORCE);
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) != 0, "reinforce grant sets 0x20");
    /* Cheat off: the branch clears the bit even though the list permits. */
    ctx.cheat = 0;
    run(&ctx, 1, 0.5f);
    CHECK((rider->flags_3ec & 0x20u) == 0, "flag cleared when cheat is off");
    rider->act_count = 0;

    /* FIFO install at capacity (FUN_0044b370's append/overwrite, corpus
     * 0x44b44a / 0x44b417): a full list shifts left and the new grant lands
     * in the last slot; non-special appends never replace in place. */
    EntitySlot* fifo = place(&ctx, 1, "fifo", 0, 0);
    const int plain[] = { AI_ACT_STAND, AI_ACT_ROUTE, AI_ACT_ROUTE_RUBBLE,
                          AI_ACT_RUN_PANIC, AI_ACT_TURN_LEFT };
    for (unsigned i = 0; i < sizeof plain / sizeof plain[0]; ++i) {
        ai_activity_grant(&ctx, fifo, plain[i]);
    }
    CHECK(fifo->act_count == AI_ACTIVITY_SLOTS, "append fills to cap");
    ai_activity_grant(&ctx, fifo, AI_ACT_EMERGE);
    CHECK(fifo->act_count == AI_ACTIVITY_SLOTS &&
              ai_activity_scan(fifo, AI_ACT_STAND) == 0 &&
              ai_activity_scan(fifo, AI_ACT_ROUTE) == 1 &&
              ai_activity_scan(fifo, AI_ACT_EMERGE) == 1,
          "full-list grant drops the oldest and appends");

    /* Deferred install branch (0x43da62): +0x3e4 bit 0x40 takes the grant
     * from the chain-head record's +0x38 field, installs it through the
     * setter, and returns early — the chain head does not run that tick. */
    EntitySlot* grantee = place(&ctx, 1, "grantee", 0, 0);
    cmd_create_move(grantee, 10.0f, 0.0f, 0);
    CHECK(grantee->cmd_head != NULL, "move record at the chain head");
    if (grantee->cmd_head != NULL) {
        cmd_rec_set_activity(grantee->cmd_head, AI_ACT_TELEPORT);
    }
    grantee->flags_3e4 |= 0x00000040u;
    float x_before_grant = grantee->x;
    run(&ctx, 1, 0.5f);
    CHECK((grantee->flags_3e4 & 0x00000040u) == 0, "grant flag consumed");
    CHECK(ai_activity_scan(grantee, AI_ACT_TELEPORT) == 1,
          "record grant lands on the entity's list");
    CHECK(grantee->x == x_before_grant, "grant branch returns early that tick");
    grantee->act_count = 0;

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

        /* --- wave-flood pathfinder core (FUN_00430ee0/FUN_00430530 port) - *
         * The seed spreads the wave from the target, marks walked cells
         * with flag 0x40000, and the traceback walks back to the start
         * through the flags & 8 gate — issue #70. */

        /* Multi-cell path on a clean grid: a straight 4-cell flood to a
         * target one margin-row away, installed in walk order. */
        memset(voxels, 0, sizeof(voxels));
        for (int cy = 0; cy <= 13; ++cy) {
            for (int cx = 0; cx <= 13; ++cx) {
                *(uint32_t*)(voxels + ((size_t)cy * 16 + cx) * 0x48 + 8) |= 0x10u;
            }
        }
        EntitySlot* runner = place(&ctx, 1, "runner", 1, 1);
        cmd_create_move(runner, 5.0f, 1.0f, 0);
        CHECK(runner->wp_count == 4,
              "wave-flood builds the multi-cell path on a clean grid");
        CHECK(runner->wp[0].x == 2.0f && runner->wp[3].x == 5.0f &&
                  runner->wp[3].y == 1.0f,
              "the traceback installs the flooded cells in walk order");
        CHECK(ai_wave_flood_count() > runner->wp_count,
              "the wave spread marks more cells with flag 0x40000 than the path uses");
        for (int t = 0; t < 12 && runner->cmd_head != NULL; ++t) {
            run(&ctx, 1, 0.5f);
        }
        CHECK(runner->x == 5.0f && runner->y == 1.0f &&
                  runner->cmd_head == NULL,
              "the walker walks every flooded cell and arrives");

        /* Retargeting mid-move (PR #87 feedback): the walker's own cell
         * carries its occupancy reservation every tick (0x43deed), so a
         * rebuild while the command is live must exempt it — otherwise
         * the wave can never root at the start and every mid-move
         * retarget silently drops to the direct fallback. */
        memset(voxels, 0, sizeof(voxels));
        for (int cy = 0; cy <= 13; ++cy) {
            for (int cx = 0; cx <= 13; ++cx) {
                *(uint32_t*)(voxels + ((size_t)cy * 16 + cx) * 0x48 + 8) |= 0x10u;
            }
        }
        EntitySlot* retarget = place(&ctx, 1, "retarget", 1, 5);
        cmd_create_move(retarget, 8.0f, 5.0f, 0);
        CHECK(retarget->wp_count == 7,
              "the first build routes the seven-cell move");
        run(&ctx, 1, 0.1f); /* mid-move: the walker marks cell (1,5) */
        CHECK(retarget->cmd_head != NULL, "the retarget move is still live");
        CHECK(ai_path_build(&ctx, retarget, 8.0f, 5.0f) == 7,
              "the rebuild exempts the mover's own reservation at the start");
        for (int t = 0; t < 15 && retarget->cmd_head != NULL; ++t) {
            run(&ctx, 1, 0.5f);
        }
        CHECK(retarget->cmd_head == NULL && retarget->x == 8.0f &&
                  retarget->y == 5.0f,
              "the rebuilt route walks to the target");

        /* Seed flag footprint (PR #87 feedback): record 12 raises the
         * flag over the northwest 2x2 block including the seed cell,
         * (-1,-1), (0,-1), (0,0), (-1,0) — not the cardinal neighbours.
         * A target one step east of the start pins the count: footprint
         * {(2,11),(3,11),(3,12),(2,12)} plus the wave's only spread entry
         * (4,12) gives exactly five flagged cells; cardinal-neighbour
         * marking gives four and misses the seed cell itself. */
        EntitySlot* seedcell = place(&ctx, 1, "seedcell", 2, 12);
        CHECK(ai_path_build(&ctx, seedcell, 3.0f, 12.0f) == 1,
              "a one-step target roots a one-cell traceback");
        CHECK(ai_wave_flood_count() == 5,
              "the seed raises record 12's northwest 2x2 footprint flag");

        /* Move-command path through the executor (T2): the demo's own
         * route — a selection opcode-5 move dispatches through the queue
         * executor into CmdCreateMove, which builds the path with the
         * wave-flood core. On a bound grid with a blocked column in the
         * way, the command installs a multi-cell wave-built list that
         * walks around the column, popping waypoints on arrival. */
        memset(voxels, 0, sizeof(voxels));
        for (int cy = 0; cy <= 13; ++cy) {
            for (int cx = 0; cx <= 13; ++cx) {
                *(uint32_t*)(voxels + ((size_t)cy * 16 + cx) * 0x48 + 8) |= 0x10u;
            }
        }
        for (int cy = 1; cy <= 5; ++cy) {
            *(uint32_t*)(voxels + ((size_t)cy * 16 + 5) * 0x48 + 8) |= 0x40u;
        }
        EntitySlot* mailrun = place(&ctx, 1, "mailrun", 1, 3);
        selection_add(&ctx, mailrun);
        selection_cmd_move(&ctx, 9, 3); /* demo 'm': opcode 5 -> CmdCreateMove */
        cmd_queue_tick(); /* the per-frame executor (FUN_004520d0) */
        CHECK(mailrun->cmd_head != NULL && cmd_rec_type(mailrun->cmd_head) == 0,
              "queue move dispatches a move command onto the chain");
        CHECK(mailrun->wp_count > 1,
              "the executor move installs a multi-cell wave-built path");
        CHECK(ai_wave_flood_count() > mailrun->wp_count,
              "the wave spread marked more cells than the path used");
        int mail_arrived = 0;
        for (int t = 0; t < 40 && !mail_arrived; ++t) {
            run(&ctx, 1, 0.5f);
            int px = (int)(mailrun->x + 0.5f);
            int py = (int)(mailrun->y + 0.5f);
            CHECK(!(px == 5 && py >= 1 && py <= 5),
                  "the executor move never lands on the blocked column");
            mail_arrived = mailrun->cmd_head == NULL;
        }
        CHECK(mail_arrived && mailrun->x == 9.0f && mailrun->y == 3.0f,
              "the move command's wave route reroutes around the column");
        CHECK((mailrun->flags_3e0 & 0x8u) == 0,
              "arrival clears the +0x3e0 move flag");
        selection_clear(&ctx);

        /* Unreachable target: the blocked column now spans the whole grid,
         * so the wave floods the target side, exhausts, and the traceback
         * never roots at the start. */
        for (int cy = 0; cy <= 13; ++cy) {
            *(uint32_t*)(voxels + ((size_t)cy * 16 + 5) * 0x48 + 8) |= 0x40u;
        }
        EntitySlot* stranded = place(&ctx, 1, "stranded", 1, 5);
        CHECK(ai_path_build(&ctx, stranded, 9.0f, 5.0f) == 0,
              "unreachable target across the full wall yields no path");
        CHECK(stranded->wp_count == 0,
              "a failed build installs nothing (the creator keeps its fallback)");
        CHECK(ai_wave_flood_count() > 0,
              "the wave still spread across the target side before exhausting");

        /* Equal cells: the seed validates the target (it is the start), so
         * the traceback is a zero-cell walk and yields no path — the
         * creator's one-hop fallback seeds, and the walker arrives in the
         * same tick. */
        EntitySlot* parked = place(&ctx, 1, "parked", 1, 3);
        CHECK(ai_path_build(&ctx, parked, 1.0f, 3.0f) == 0,
              "start-equal-target builds no path");
        cmd_create_move(parked, 1.0f, 3.0f, 0);
        CHECK(parked->wp_count == 1,
              "the equal-cell move keeps the one-hop fallback seed");
        run(&ctx, 1, 0.5f);
        CHECK(parked->cmd_head == NULL && parked->x == 1.0f,
              "the zero-distance fallback completes on the first tick");

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
        /* No-grid case: the wave-flood core has no handle, builds no
         * state, and the caller's one-hop fallback runs unchanged. */
        EntitySlot* gridless = place(&ctx, 1, "gridless", 0, 0);
        CHECK(ai_path_build(&ctx, gridless, 6.0f, 6.0f) == 0,
              "no grid handle: no path and no flood state");
        CHECK(ai_wave_flood_count() == 0,
              "a no-grid build owns no wave work state");
        cmd_create_move(gridless, 6.0f, 6.0f, 0);
        CHECK(gridless->wp_count == 1, "no-grid move seeds the one-hop fallback");
        run(&ctx, 12, 0.5f);
        CHECK(gridless->x >= 5.9f && gridless->cmd_head == NULL,
              "the no-grid fallback path walks unchanged");

        (void)f00;
    }

    /* --- per-terrain movement speed (FUN_00469cc0 speed probe) ----------- */

    {
        static uint8_t voxels2[16 * 16 * 0x48];
        LevelState map2;
        memset(&map2, 0, sizeof(map2));
        memset(voxels2, 0, sizeof(voxels2));
        level_set_int(&map2, LEVEL_OFF_WIDTH, 16);
        level_set_int(&map2, LEVEL_OFF_HEIGHT, 16);
        level_store_ptr(&map2, LEVEL_OFF_VOXELS, voxels2);
        for (int cy = 0; cy < 14; ++cy) {
            for (int cx = 0; cx < 14; ++cx) {
                *(uint32_t*)(voxels2 + ((size_t)cy * 16 + cx) * 0x48 + 8) |= 0x10u;
            }
        }
        /* Row 5 crosses columns 4..7 of *slow* terrain (type 4), row 7 the
         * same span of *fast* terrain (type 5), row 6 stays plain. */
        for (int cx = 4; cx <= 7; ++cx) {
            voxels2[(size_t)5 * 16 * 0x48 + (size_t)cx * 0x48 + 2] = 4;
            voxels2[(size_t)7 * 16 * 0x48 + (size_t)cx * 0x48 + 2] = 5;
        }
        ctx.stats_speed_sec[4] = 2.0f;  /* corpus units: sec/cell -> 0.5 cps */
        ctx.stats_speed_sec[5] = 0.25f; /* -> 4 cps */

        /* The probe reads the table by terrain index; unset entries and
         * out-of-range types fall back to the default (1/AI_MOVE_SPEED_CPS). */
        CHECK(ai_stats_speed(&ctx, 4) == 2.0f, "speed table entry for type 4");
        CHECK(ai_stats_speed(&ctx, 5) == 0.25f, "speed table entry for type 5");
        CHECK(ai_stats_speed(&ctx, 0) * AI_MOVE_SPEED_CPS == 1.0f,
              "unset entry defaults to the port speed");
        CHECK(ai_stats_speed(&ctx, -1) * AI_MOVE_SPEED_CPS == 1.0f &&
                  ai_stats_speed(&ctx, AI_STATS_TERRAIN_TYPES) *
                      AI_MOVE_SPEED_CPS == 1.0f,
              "out-of-range terrain types are guarded to the default");

        ctx.level = &map2;
        EntitySlot* slow = place(&ctx, 1, "slow", 3, 5);
        EntitySlot* plain = place(&ctx, 1, "plain", 3, 6);
        EntitySlot* fast = place(&ctx, 1, "fast", 3, 7);
        cmd_create_move(slow, 11.0f, 5.0f, 0);
        cmd_create_move(plain, 11.0f, 6.0f, 0);
        cmd_create_move(fast, 11.0f, 7.0f, 0);
        CHECK(slow->wp_count > 1, "the speed rows walk straight-through paths");

        int t_slow = 0, t_plain = 0, t_fast = 0;
        float lag = 0.0f;
        for (int t = 1; t <= 64 && !(t_slow && t_plain && t_fast); ++t) {
            ai_tick(&ctx, 0.5f);
            if (t_plain == 0 && plain->cmd_head == NULL) {
                t_plain = t;
                lag = plain->x - slow->x;
            }
            if (t_slow == 0 && slow->cmd_head == NULL) {
                t_slow = t;
            }
            if (t_fast == 0 && fast->cmd_head == NULL) {
                t_fast = t;
            }
        }
        CHECK(t_slow > 0 && t_plain > 0 && t_fast > 0, "all walkers arrive in "
                                                      "the tick budget");
        CHECK(t_slow > t_plain, "slow terrain visibly delays arrival");
        CHECK(t_plain > t_fast, "fast terrain visibly quickens arrival");
        CHECK(lag > 1.0f, "the slow walker is behind when the plain row lands");

        /* Arrival still pops the path list as before the probe landed. */
        CHECK(slow->wp_index == slow->wp_count && slow->cmd_head == NULL &&
                  slow->wp[slow->wp_count - 1].pending == 0,
              "slow walker arrival pops the path like today");
        CHECK(fast->x == 11.0f && fast->y == 7.0f, "fast walker lands exactly");
        const uint32_t* f_slow = (const uint32_t*)(voxels2 + ((size_t)5 * 16 +
                                                              11) * 0x48 + 8);
        CHECK((*f_slow & 0x8000000u) == 0,
              "slow-row arrival clears the occupancy flag");

        ctx.level = NULL;
        ctx.stats_speed_sec[4] = 0.0f;
        ctx.stats_speed_sec[5] = 0.0f;
    }

    if (s_failures != 0) {
        fprintf(stderr, "ai_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("ai_test: OK\n");
    return 0;
}