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
#include "game/util.hpp"
#include "gods98/block.hpp"
#include "gods98/message.hpp"
#include "gods98/resource.hpp"

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

/* --------------------------------------------------------------------- */
/* Real-data probe (opt-in `--real <wad-dir>`, meson adds it with          */
/* test_wads_dir): wave-flood pathfinder on shipped .map grids.            */
/*                                                                         */
/* The reference BFS below mirrors the pre-port BFS stand-in exactly       */
/* (b53bc29^, removed when the wave-flood port landed). It exists here     */
/* only as an oracle — the shipping path never runs it. On entity-free     */
/* grids both algorithms gate identically (level_walkable), so the probe   */
/* asserts:                                                                */
/*   - wave-flood paths stay on walkable, unoccupied cells, step           */
/*     4-neighbour-adjacently, and end on the requested target cell;       */
/*   - reachability agrees with the reference BFS;                         */
/*   - when both produce a path they are the same length — both shortest   */
/*     — so agreement survives reroutes around obstacles;                  */
/*   - sampled walkers running the real command path arrive.               */
/* --------------------------------------------------------------------- */

#define REAL_PAIRS_PER_LEVEL 12   /* start/target pairs sampled per level  */
#define REAL_ARRIVALS_PER_LEVEL 2 /* real-grid walkers ticked to arrival   */
#define REAL_LEVELS_EXPECTED 42
#define REAL_PAIRS_EXPECTED 496
#define REAL_PATHS_EXPECTED 350
#define REAL_ARRIVALS_EXPECTED 84

static int s_real_levels;  /* levels loaded and probed        */
static int s_real_pairs;   /* wave-flood / BFS pairs compared */
static int s_real_paths;   /* pairs where wave-flood found one */
static int s_real_arrivals; /* walkers run to arrival          */

/* Same walk order as ai_path_build: start-excluded, target last. */
static int bfs_reference(const LevelState* grid, int sx, int sy, int gx,
                         int gy, int* out, int maxn)
{
    int w = level_width(grid);
    int h = level_height(grid);
    if ((sx == gx && sy == gy) || !level_walkable(grid, sx, sy) ||
        !level_walkable(grid, gx, gy)) {
        return 0;
    }

    int size = w * h;
    int* prev = (int*)malloc((size_t)size * sizeof(int));
    int* queue = (int*)malloc((size_t)size * sizeof(int));
    if (prev == NULL || queue == NULL) {
        free(prev);
        free(queue);
        return 0;
    }
    static const int dx[4] = { 1, -1, 0, 0 };
    static const int dy[4] = { 0, 0, 1, -1 };
    memset(prev, 0xff, (size_t)size * sizeof(int)); /* -1 = unvisited */
    prev[sy * w + sx] = -2; /* start sentinel */
    int head = 0, tail = 0, found = 0;
    queue[tail++] = sy * w + sx;
    while (head < tail && !found) {
        int cell = queue[head++];
        int cx = cell % w, cy = cell / w;
        for (int d = 0; d < 4 && !found; ++d) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                continue;
            }
            int ncell = ny * w + nx;
            if (prev[ncell] != -1 || !level_walkable(grid, nx, ny)) {
                continue;
            }
            prev[ncell] = cell;
            queue[tail++] = ncell;
            if (nx == gx && ny == gy) {
                found = 1;
            }
        }
    }
    free(queue);
    if (!found) {
        free(prev);
        return 0;
    }
    int trace[256];
    int len = 0;
    for (int cell = gy * w + gx; cell != sy * w + sx; cell = prev[cell]) {
        if (len >= (int)(sizeof(trace) / sizeof(trace[0]))) {
            free(prev);
            return 0;
        }
        trace[len++] = cell;
    }
    free(prev);
    if (len > maxn) {
        return 0; /* the port buffer caps at AI_WAYPOINTS_MAX too */
    }
    for (int i = 0; i < len; ++i) {
        out[i] = trace[len - 1 - i];
    }
    return len;
}

/* Wave-flood the same pair through the shipping builder, reading the cell
 * sequence back off the entity's waypoint list. A stack EntitySlot stands
 * in for the mover: ai_path_build touches only x/y and the wp buffer. */
static int wave_probe(GameContext* ctx, int sx, int sy, int gx, int gy,
                      int* out, int maxn)
{
    EntitySlot probe_slot;
    memset(&probe_slot, 0, sizeof(probe_slot));
    probe_slot.x = (float)sx;
    probe_slot.y = (float)sy;
    int n = ai_path_build(ctx, &probe_slot, (float)gx, (float)gy);
    if (n > maxn) {
        return 0;
    }
    for (int i = 0; i < n; ++i) {
        int wx = (int)probe_slot.wp[i].x;
        int wy = (int)probe_slot.wp[i].y;
        out[i] = wy * level_width(ctx->level) + wx;
    }
    return n;
}

static void real_check(int cond, const char* what, const char* level, int sx,
                       int sy, int gx, int gy)
{
    if (cond) {
        return;
    }
    fprintf(stderr, "FAIL --real: %s in \"%s\": (%d,%d) -> (%d,%d)\n", what,
            level, sx, sy, gx, gy);
    ++s_failures;
}

static void probe_level_real(GameContext* ctx, BlockFile* cfg,
                             const char* name)
{
    LevelState* lv = level_load(cfg, name);
    if (lv == NULL) {
        fprintf(stderr, "FAIL --real: \"%s\": level_load failed\n",
                name);
        ++s_failures;
        return;
    }
    int w = level_width(lv);
    int h = level_height(lv);
    if (w < 3 || h < 3) { /* grid_of() treats a sub-margin map as absent */
        fprintf(stderr, "FAIL --real: \"%s\": grid too small (%d x %d)\n",
                name, w, h);
        ++s_failures;
        level_destroy(lv);
        return;
    }
    ++s_real_levels;
    ctx->level = lv;

    int* cells = (int*)malloc((size_t)w * h * sizeof(int));
    if (cells == NULL) {
        fprintf(stderr, "FAIL --real: \"%s\": cell allocation failed\n", name);
        ++s_failures;
        level_destroy(lv);
        ctx->level = NULL;
        return;
    }
    int nc = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (level_walkable(lv, x, y)) {
                cells[nc++] = y * w + x;
            }
        }
    }

    if (nc < 2) {
        fprintf(stderr, "FAIL --real: \"%s\": only %d walkable cell(s)\n",
                name, nc);
        ++s_failures;
        free(cells);
        ctx->level = NULL;
        level_destroy(lv);
        return;
    }
    int pairs = nc < REAL_PAIRS_PER_LEVEL ? nc : REAL_PAIRS_PER_LEVEL;
    int arrivals_left = REAL_ARRIVALS_PER_LEVEL;

    /* First pass: wave-flood / BFS agreement. No walkers run yet, so the
     * only occupancy bits on the grid are whatever the .map carries. */
    int good_s[REAL_ARRIVALS_PER_LEVEL + 1], good_t[REAL_ARRIVALS_PER_LEVEL + 1];
    int good_len[REAL_ARRIVALS_PER_LEVEL + 1];
    int ngood = 0;
    for (int p = 0; p < pairs; ++p) {
        /* p=0 spans the whole walkable set (longest sampled reroute), the
         * rest stride the list with coprime steps: deterministic, and
         * scattered far-apart pairs across every level. */
        unsigned si = p == 0 ? 0u : (unsigned)(((size_t)p * 100003u + 11u) % (unsigned)nc);
        unsigned ti = p == 0 ? (unsigned)(nc - 1) : (unsigned)(((size_t)p * 131071u + 17u) % (unsigned)nc);
        if (si == ti) {
            continue;
        }
        int sx = (int)(cells[si] % w), sy = cells[si] / w;
        int tx = (int)(cells[ti] % w), ty = cells[ti] / w;
        int wave_path[AI_WAYPOINTS_MAX];
        int ref_path[AI_WAYPOINTS_MAX];
        int wlen = wave_probe(ctx, sx, sy, tx, ty, wave_path, AI_WAYPOINTS_MAX);
        int rlen = bfs_reference(lv, sx, sy, tx, ty, ref_path, AI_WAYPOINTS_MAX);
        ++s_real_pairs;
        if (wlen > 0) {
            ++s_real_paths;
        }
        real_check((wlen > 0) == (rlen > 0), "wave-flood and reference BFS "
                                            "disagree on reachability",
                   name, sx, sy, tx, ty);
        real_check(wlen <= 0 || wlen == rlen, "wave-flood path length differs "
                                             "from the reference BFS",
                   name, sx, sy, tx, ty);
        int prev_cell = sy * w + sx;
        for (int i = 0; i < wlen; ++i) {
            int cx = wave_path[i] % w, cy = wave_path[i] / w;
            real_check(level_walkable(lv, cx, cy),
                       "wave-flood path lands on an unwalkable cell", name, sx,
                       sy, tx, ty);
            real_check(!level_tile_occupied(lv, cx, cy),
                       "wave-flood path lands on an occupied cell", name, sx,
                       sy, tx, ty);
            real_check(abs(cx - prev_cell % w) + abs(cy - prev_cell / w) == 1,
                       "wave-flood path steps off-grid or diagonally", name, sx,
                       sy, tx, ty);
            prev_cell = wave_path[i];
        }
        if (wlen > 0) {
            real_check(prev_cell == ty * w + tx,
                       "wave-flood path does not end on the target cell", name,
                       sx, sy, tx, ty);
        }
        if (wlen > 0 && wlen <= 30 && ngood < REAL_ARRIVALS_PER_LEVEL) {
            good_s[ngood] = cells[si]; /* cell ids: the walker pass decodes */
            good_t[ngood] = cells[ti];
            good_len[ngood] = wlen;
            ++ngood;
        }
    }

    /* Second pass: tick real walkers to arrival (cap per level; short paths
     * only, so the whole probe stays inside the suite's tick budget). */
    for (int p = 0; p < ngood && arrivals_left > 0; ++p, --arrivals_left) {
        int sx = good_s[p] % w, sy = good_s[p] / w;
        int tx = good_t[p] % w, ty = good_t[p] / w;
        /* Departure occupancy persists until reused (corpus 0x43cd72 clears
         * only the arrival cell), so the previous walker's trail would
         * legitimately block this walker's flood: reset the reservation
         * bits probe-side between walkers. */
        for (int cy = 0; cy < h; ++cy) {
            for (int cx = 0; cx < w; ++cx) {
                if (level_tile_occupied(lv, cx, cy)) {
                    level_tile_mark(lv, cx, cy, 0);
                }
            }
        }
        EntitySlot* wk = place(ctx, 1, "realwalker", (float)sx, (float)sy);
        if (wk == NULL) {
            fprintf(stderr, "FAIL --real: \"%s\": could not place walker\n",
                    name);
            ++s_failures;
            break;
        }
        cmd_create_move(wk, (float)tx, (float)ty, 0);
        int budget = 8 * (good_len[p] + 4); /* >=1 cell/tick at dt=0.5 */
        int t = 0;
        for (; t < budget && wk->cmd_head != NULL; ++t) {
            ai_tick(ctx, 0.5f);
        }
        ++s_real_arrivals;
        real_check(wk->cmd_head == NULL, "real-grid walker did not arrive in "
                                        "the tick budget",
                   name, sx, sy, tx, ty);
        real_check(wk->wp_index == wk->wp_count, "arrival pops the real path",
                   name, sx, sy, tx, ty);
        real_check((wk->x - tx) * (wk->x - tx) + (wk->y - ty) * (wk->y - ty) <
                       0.25f,
                   "real-grid walker lands on the target cell", name, sx, sy,
                   tx, ty);
    }

    if (arrivals_left != 0) {
        fprintf(stderr, "FAIL --real: \"%s\": ran %d of %d walker arrivals\n",
                name, REAL_ARRIVALS_PER_LEVEL - arrivals_left,
                REAL_ARRIVALS_PER_LEVEL);
        ++s_failures;
    }

    free(cells);
    ctx->level = NULL;
    level_destroy(lv);
}

static void probe_real(const char* wad_dir)
{
    if (resource_mount(wad_dir, "LegoRR") <= 0) {
        fprintf(stderr, "FAIL --real: no wads under %s\n", wad_dir);
        ++s_failures;
        return;
    }
    BlockFile* cfg = block_open("Lego.cfg");
    if (cfg == NULL) {
        fprintf(stderr, "FAIL --real: no Lego.cfg in %s\n",
                wad_dir);
        ++s_failures;
        return;
    }

    GameContext rctx; /* entity-free: no reservations, no stale commands */
    game_context_init(&rctx);
    cmd_context_set(&rctx);
    rctx.stats_speed_sec[4] = 0.0f; /* neutral terrain: the synthetic probe */
    rctx.stats_speed_sec[5] = 0.0f; /* rows above leave the table dirty      */

    size_t nrec = block_record_count(cfg);
    for (size_t i = 0; i < nrec; ++i) {
        BlockRec* g = block_record_at(cfg, i);
        if (g == NULL) {
            continue;
        }
        const char* name = block_name(g);
        char path[512];
        snprintf(path, sizeof(path), "%s Levels %s SurfaceMap",
                 util_root_name(cfg), name);
        int found = block_get_strptr(cfg, path) != NULL;
        if (!found) {
            snprintf(path, sizeof(path), "%s %s SurfaceMap", util_root_name(cfg),
                     name);
            found = block_get_strptr(cfg, path) != NULL;
        }
        if (found) {
            probe_level_real(&rctx, cfg, name);
        }
    }
    fprintf(stderr,
            "ai_test: --real probe: %d levels, %d wave/BFS pairs, %d paths, "
            "%d walker arrivals\n",
            s_real_levels, s_real_pairs, s_real_paths, s_real_arrivals);
    if (s_real_levels != REAL_LEVELS_EXPECTED ||
        s_real_pairs != REAL_PAIRS_EXPECTED ||
        s_real_paths != REAL_PATHS_EXPECTED ||
        s_real_arrivals != REAL_ARRIVALS_EXPECTED) {
        fprintf(stderr,
                "FAIL --real: expected %d levels, %d pairs, %d paths, "
                "%d arrivals\n",
                REAL_LEVELS_EXPECTED, REAL_PAIRS_EXPECTED,
                REAL_PATHS_EXPECTED, REAL_ARRIVALS_EXPECTED);
        ++s_failures;
    }
    block_close(cfg);
}

int main(int argc, char** argv)
{
    const char* real_dir = NULL;
    for (int a = 1; a < argc; ++a) {
        if (strcmp(argv[a], "--real") == 0 && a + 1 < argc) {
            real_dir = argv[a + 1];
        } else if (strcmp(argv[a], "--real") == 0) {
            fprintf(stderr, "FAIL --real: expected a WAD directory\n");
            ++s_failures;
        }
    }

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
    /* Test isolation: the selection list is process state, and later
     * sections' cmd_queue_tick calls dispatch for every entity still
     * selected. Left selected here, digger picks up move records long
     * after its section, and cmd_link_tail then threads later records
     * through digger's still-live chain tail — one record reachable from
     * two chains, popped twice, until ai_tick's chain loop never drains. */
    selection_clear(&ctx);

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

    /* --- minion task-state (spawn-dispatch switch 0x438150) ------------- *
     * The entity spawn dispatcher (FUN_00437d00) assigns the minion types
     * their +0x334 stats block through the selector at 0x438150: type-8
     * bytes at 0x4382ac {00 01 04 04 02 02 02 04 03 03 03 03} jump (0x438298)
     * to the static-template arms DAT_004a98f0 (type 8), DAT_004a9a40
     * (type 9), DAT_004a9b90 (types 0xc/0xd/0xe/0x10/0x11/0x12/0x13); every
     * other type falls back to the instance-stats arm (FUN_00469c10). */
    CHECK(ai_minion_state_for(8) == AI_TASK_STATE_MINION_1 &&
              ai_minion_state_for(9) == AI_TASK_STATE_MINION_2,
          "switch arms pin types 8/9 to their static templates");
    for (int t = 12; t <= 19; ++t) {
        const int expect = (t == 15) ? AI_TASK_STATE_NONE
                                     : AI_TASK_STATE_MINION_3;
        CHECK(ai_minion_state_for(t) == expect,
              "minion template arm covers the 0xc..0xe/0x10..0x13 set");
    }
    CHECK(ai_minion_state_for(1) == AI_TASK_STATE_NONE &&
              ai_minion_state_for(2) == AI_TASK_STATE_NONE &&
              ai_minion_state_for(5) == AI_TASK_STATE_NONE &&
              ai_minion_state_for(6) == AI_TASK_STATE_NONE &&
              ai_minion_state_for(7) == AI_TASK_STATE_NONE &&
              ai_minion_state_for(10) == AI_TASK_STATE_NONE &&
              ai_minion_state_for(11) == AI_TASK_STATE_NONE &&
              ai_minion_state_for(20) == AI_TASK_STATE_NONE,
          "non-template types take the instance-stats arm");

    /* The spawn dispatch sets the minion task state and its per-entity
     * activity capacity (state +0xcc, FUN_0046a060) on the static-template
     * types; the type-2 spawn grant (Activity_Stand) is untouched. */
    EntitySlot* minion8 = place(&ctx, 8, "minion", 0, 0);
    CHECK(minion8->task_state == AI_TASK_STATE_MINION_1 &&
              minion8->activity_cap == AI_ACTIVITY_SLOTS,
          "minion spawn assigns task state and capacity");
    CHECK(minion8->act_count == 0 && ai_activity_probe(minion8) == 0,
          "ungranted minion activity still fails the probe");
    EntitySlot* plain_special = place(&ctx, 15, "plain_special", 0, 0);
    CHECK(plain_special->task_state == AI_TASK_STATE_NONE &&
              plain_special->activity_cap == 0,
          "instance-stats types get no minion task state");
    EntitySlot* rider2 = place(&ctx, 2, "rider2", 0, 0);
    CHECK(rider2->task_state == AI_TASK_STATE_NONE &&
              ai_activity_scan(rider2, AI_ACT_STAND) == 1,
          "type-2 spawn still seeds Stand; no task state");

    /* A granted task answers the can-do probe from the list; the cleared
     * list fails it — on a minion-task-state entity (FUN_0043c440's probe
     * is not type-gated; the FUN_0044c530 type-2 gate wraps only the cheat
     * branch). */
    ai_activity_grant(&ctx, minion8, AI_ACT_DRILL);
    CHECK(ai_activity_scan(minion8, AI_ACT_DRILL) == 1 &&
              ai_activity_probe(minion8) == 1,
          "granted drill task passes the probe");
    ai_activity_grant(&ctx, minion8, AI_ACT_REINFORCE);
    CHECK(ai_activity_probe(minion8) == 3, "reinforce grant answers probe 3");
    minion8->act_count = 0;
    CHECK(ai_activity_probe(minion8) == 0, "cleared list fails the probe");
    ai_activity_grant(&ctx, minion8, AI_ACT_TELEPORT);
    CHECK(ai_activity_probe(minion8) == 2, "teleport grant answers probe 2");
    minion8->act_count = 0;

    /* Per-entity capacity (state +0xcc) sizes the appender: a cap-1 state
     * keeps a one-slot list (append, then FIFO overwrite) — matching
     * FUN_0046a060's per-entity read; capacity 0 = unset (entity uses the
     * game-wide default), and a zero context default makes grants a no-op
     * exactly as the corpus's cap-0 states behave. */
    minion8->activity_cap = 1;
    ai_activity_grant(&ctx, minion8, AI_ACT_STAND);
    CHECK(minion8->act_count == 1 &&
              ai_activity_scan(minion8, AI_ACT_STAND) == 1,
          "cap-1 state holds one grant");
    ai_activity_grant(&ctx, minion8, AI_ACT_ROUTE);
    CHECK(minion8->act_count == 1 &&
              ai_activity_scan(minion8, AI_ACT_STAND) == 0 &&
              ai_activity_scan(minion8, AI_ACT_ROUTE) == 1,
          "full cap-1 list drops the oldest (FIFO)");
    minion8->act_count = 0;
    minion8->activity_cap = 0; /* unset -> ctx default again */
    int default_cap = ctx.stats_activity_cap;
    ctx.stats_activity_cap = 0; /* corpus cap-0 state: grant runs nothing */
    ai_activity_grant(&ctx, minion8, AI_ACT_ROUTE_RUBBLE);
    CHECK(minion8->act_count == 0 &&
              ai_activity_scan(minion8, AI_ACT_ROUTE_RUBBLE) == 0,
          "zero capacity makes grants a no-op");
    ctx.stats_activity_cap = default_cap;

    /* Deferred grant cleanup (0x43da8b): after installing the chain-head
     * grant, the corpus unlinks and frees every type-0x13 record
     * (FUN_00403d10(ent, 0x13, 0, 1)) — the port's cmd_chain_drop. */
    EntitySlot* grantee2 = place(&ctx, 8, "grantee2", 0, 0);
    cmd_create_move(grantee2, 10.0f, 0.0f, 0);
    if (grantee2->cmd_head != NULL) {
        cmd_rec_set_activity(grantee2->cmd_head, AI_ACT_DRILL);
        void* grec = cmd_alloc(0x13); /* type-0x13 grant record */
        cmd_rec_set_next(grantee2->cmd_head, grec);
        cmd_rec_set_next(grec, NULL);
    }
    grantee2->flags_3e4 |= 0x00000040u;
    float x_before_cleanup = grantee2->x;
    run(&ctx, 1, 0.5f);
    CHECK((grantee2->flags_3e4 & 0x00000040u) == 0, "cleanup consumes flag");
    CHECK(ai_activity_scan(grantee2, AI_ACT_DRILL) == 1,
          "head grant lands on the minion list");
    CHECK(grantee2->x == x_before_cleanup, "cleanup branch returns early");
    {
        int has_13 = 0;
        int recs = 0;
        for (const void* r = grantee2->cmd_head; r != NULL;
             r = cmd_rec_next(r)) {
            ++recs;
            if (cmd_rec_type(r) == 0x13) {
                has_13 = 1;
            }
        }
        CHECK(!has_13 && recs == 1,
              "type-0x13 grant record unlinked after the grant");
    }
    /* Purging a chain also unlinks its live records from the global list. */
    cmd_chain_purge(grantee2);

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

    /* Move and target records stay on entity chains; dropping a target
     * leaves the move intact and does not alter the task-only global list. */
    void* global_before_drop = cmd_global_head();
    EntitySlot* dropper = place(&ctx, 1, "dropper", 11, 5);
    cmd_create_move(dropper, 10.0f, 5.0f, 0);
    cmd_create_target(dropper, (void*)dropper);
    CHECK(cmd_chain_drop(dropper, 2) == 1, "drop takes the target record");
    CHECK(cmd_global_head() == global_before_drop,
          "entity-chain drop leaves the task global list intact");
    for (int t = 0; t < 10 && dropper->cmd_head != NULL; ++t) {
        run(&ctx, 1, 0.5f); /* drain before any later link, so nothing else
                             * threads records onto this probe's chains */
    }
    CHECK(dropper->cmd_head == NULL, "dropper chain drains cleanly");
    EntitySlot* linked = place(&ctx, 1, "linked", 12, 5);
    cmd_create_move(linked, 11.0f, 5.0f, 0);
    CHECK(cmd_global_head() == global_before_drop &&
              linked->cmd_head != NULL && cmd_rec_next(linked->cmd_head) == NULL,
          "new move remains on its own chain outside the task list");
    for (int t = 0; t < 10 && linked->cmd_head != NULL; ++t) {
        run(&ctx, 1, 0.5f);
    }
    CHECK(linked->cmd_head == NULL, "linked move drains cleanly");

    if (real_dir != NULL) {
        probe_real(real_dir);
    }

    if (s_failures != 0) {
        fprintf(stderr, "ai_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("ai_test: OK\n");
    return 0;
}
