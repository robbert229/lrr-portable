// SPDX-License-Identifier: LGPL-3.0-or-later
// AI execution receivers (finding 10 open item) — the entity-manager tick.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

#define FIX_DIM 128

/* A FIX_DIM x FIX_DIM all-zero surface map for the scratch terrain. */
static void write_surface_map(const char* path)
{
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        return;
    }
    const char magic[4] = { 'M', 'A', 'P', ' ' };
    const uint32_t len = FIX_DIM * FIX_DIM * 2, w = FIX_DIM, h = FIX_DIM;
    static const uint16_t cells[FIX_DIM * FIX_DIM] = { 0 };
    fwrite(magic, 1, 4, fp);
    fwrite(&len, 1, 4, fp);
    fwrite(&w, 1, 4, fp);
    fwrite(&h, 1, 4, fp);
    fwrite(cells, 1, sizeof(cells), fp);
    fclose(fp);
}

/* Scratch terrain the faithful route code resolves cells through. Entity
 * positions in this test are WORLD coordinates (blocksize 1: cell centres sit
 * on integer world coordinates, y flipped); move targets are cells. */
static void* s_fix_terrain;
static LevelState fix_level;
static uint8_t fix_grid[FIX_DIM * FIX_DIM * LEVEL_VOXEL_STRIDE];
/* A minimal Stats record: only RouteSpeed (+0x20) set, so the faithful
 * 0x446610 / entity_route_speed path yields 0.08 * 25 = 2 world units/s. */
static uint8_t s_fix_stats[ENTITY_STATS_RECORD_SIZE];

static EntitySlot* place(GameContext* ctx, int type, const char* name,
                         float x, float y)
{
    int idx = entity_create(ctx, type, name, x, y, 0, 0);
    CHECK(idx >= 0, "entity_create");
    EntitySlot* e = (EntitySlot*)entity_at(ctx, type, idx);
    if (e != NULL && e->stats_record == NULL) {
        e->stats_record = s_fix_stats;
    }
    return e;
}

static void cell_of(float wx, float wy, int* cx, int* cy)
{
    *cx = 0;
    *cy = 0;
    CHECK(terrain_world_xy_to_cell(s_fix_terrain, wx, wy, cx, cy) != 0,
          "world position resolves to a cell");
}

/* Fixture voxel flag word of the cell under a WORLD position. */
static uint32_t* fix_flags(float wx, float wy)
{
    int cx, cy;
    cell_of(wx, wy, &cx, &cy);
    return (uint32_t*)(fix_grid + ((size_t)cy * FIX_DIM + (size_t)cx) *
                                      LEVEL_VOXEL_STRIDE + 8);
}

/* Stand an entity at the centre of grid cell (cx, cy). */
static void set_cell(EntitySlot* e, int cx, int cy)
{
    float w[2] = { 0.0f, 0.0f };
    CHECK(terrain_cell_world_center_xy_transform(s_fix_terrain, cx, cy, w) != 0,
          "cell centre resolves");
    e->x = w[0];
    e->y = w[1];
}

/* cmd_create_move toward a WORLD position: the faithful record carries the
 * target CELL, resolved through the scratch terrain. */
static void move_to(EntitySlot* e, float wx, float wy, int flag)
{
    int cx = 0, cy = 0;
    CHECK(terrain_world_xy_to_cell(s_fix_terrain, wx, wy, &cx, &cy) != 0,
          "move target resolves to a cell");
    cmd_create_move(e, (float)cx, (float)cy, flag);
}

/* One application frame, `n` times at dt: the game-layer move/target/task
 * pass (ai_tick), then the per-entity callback receiver. The faithful
 * ai_tick no longer runs the flag/callback branches itself (the engine's
 * EntityFrameUpdateDispatch walker owns them and their liveness filter), so
 * the harness plays that walker: every live slot gets ai_entity_frame_update
 * after the pass. */
static void run(GameContext* ctx, int n, float dt)
{
    for (int i = 0; i < n; ++i) {
        ai_tick(ctx, dt);
        for (int t = 0; t < ENTITY_TYPES; ++t) {
            for (int k = 0; k < ctx->entity_count[t]; ++k) {
                EntitySlot* e = &ctx->entity[t][k];
                if (e->type != 0) {
                    ai_entity_frame_update(e, dt);
                }
            }
        }
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
        snprintf(path, sizeof(path), "%s::Levels::%s::SurfaceMap",
                 util_root_name(cfg), name);
        int found = block_get_strptr(cfg, path) != NULL;
        if (!found) {
            snprintf(path, sizeof(path), "%s::%s::SurfaceMap", util_root_name(cfg),
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

    /* The faithful move route (cmd_move_route_prepare / step_path) resolves
     * cells through the level's terrain, so the context needs a level backed
     * by a real terrain (file opens resolve under the CD root: the fixture
     * directory). */
    system("rm -rf ai_fixture");
    mkdir("ai_fixture", 0755);
    write_surface_map("ai_fixture/L01_surface.map");
    if (chdir("ai_fixture") != 0) {
        fprintf(stderr, "ai_test: chdir failed\n");
        return 1;
    }
    resource_set_cd_root(".");
    resource_set_cd_data_dir(".");
    s_fix_terrain = terrain_create("L01_surface.map", 1.0f);
    void* const fix_terrain = s_fix_terrain;
    {
        const float route_speed = 0.08f;
        const uint32_t walk_relief = 0x2000000u;
        const float surface_scale = 1.0f; /* terrain never slows the route */
        memcpy(s_fix_stats + 0x20, &route_speed, sizeof(route_speed));
        memcpy(s_fix_stats + 0xd4, &surface_scale, sizeof(surface_scale));
        memcpy(s_fix_stats + 0x144, &walk_relief, sizeof(walk_relief));
        /* A bound Stats record owns the activity capacity (+0xcc, 0x46a060);
         * without it the grant setter sees a cap-0 entity and does nothing. */
        const uint32_t activity_cap = AI_ACTIVITY_SLOTS;
        memcpy(s_fix_stats + 0xcc, &activity_cap, sizeof(activity_cap));
    }
    CHECK(fix_terrain != NULL, "scratch terrain");
    memset(&fix_level, 0, sizeof(fix_level));
    level_set_int(&fix_level, LEVEL_OFF_WIDTH, FIX_DIM);
    level_set_int(&fix_level, LEVEL_OFF_HEIGHT, FIX_DIM);
    fix_level.ptr_voxels = fix_grid;
    fix_level.ptr_terrain = fix_terrain;
    /* Every cell reads walkable to the faithful route gate (flag 0x8). */
    for (int c = 0; c < FIX_DIM * FIX_DIM; ++c) {
        const uint32_t walkable = 0x8u;
        memcpy(fix_grid + c * LEVEL_VOXEL_STRIDE + 8, &walkable,
               sizeof(walkable));
    }
    ctx.level = &fix_level;

    /* Move execution: entity at (0,0) moves to (10,0) at 2 cells/s. */
    EntitySlot* mover = place(&ctx, 1, "mover", 0, 0);
    move_to(mover, 10.0f, 0.0f, 0);
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

    /* --- path list (corpus +0x24 mirror; lazy route build, arrival pop) --- */

    /* The faithful creator (0x4035c0) only links the record; the route is
     * built lazily on the first tick by cmd_move_route_prepare, from the
     * entity's start CELL through to the target cell (both endpoints), so
     * four cells of travel is a five-entry list. */
    EntitySlot* walker = place(&ctx, 1, "walker", 0, 0);
    move_to(walker, 4.0f, 0.0f, 0);
    CHECK(walker->wp_count == 0 && (walker->flags_3e0 & 0x1u) == 0,
          "the creator seeds no path and raises no mover flag");
    run(&ctx, 1, 0.5f); /* 1 of 4 cells */
    CHECK(walker->wp_count == 5 && (walker->flags_3e0 & 0x1u) != 0,
          "the first tick routes start..target and raises the mover flag");
    run(&ctx, 1, 0.5f); /* 2 of 4 cells */
    CHECK(walker->wp_index == 3,
          "mid-path entry stays pending (pop is arrival-only)");
    run(&ctx, 3, 0.5f);
    CHECK(walker->x >= 3.9f, "walker arrives at the waypoint");
    CHECK((walker->flags_3e0 & 0x1u) == 0 && walker->cmd_head == NULL &&
              walker->wp_count == 0,
          "path exhaustion completes the command, clears the flag and path");

    /* Chained moves: the tick walks the first record to arrival, then the
     * next queued record routes from where the first ended. */
    EntitySlot* courier = place(&ctx, 1, "courier", 0, 0);
    move_to(courier, 4.0f, 0.0f, 0);
    move_to(courier, 4.0f, 4.0f, 0);
    run(&ctx, 4, 0.5f); /* exactly the first leg (1 cell per tick) */
    CHECK(courier->x >= 3.9f && courier->y < 1.0f, "first waypoint reached");
    CHECK(courier->cmd_head != NULL && courier->wp_count == 0,
          "the first leg retires, the second command stays live");
    run(&ctx, 1, 0.5f);
    CHECK((courier->flags_3e0 & 0x1u) != 0 && courier->wp_count == 5,
          "mid-chain mover flag is up on the second leg's route");
    run(&ctx, 6, 0.5f);
    CHECK(courier->x >= 3.9f && courier->y >= 3.9f, "second waypoint reached");
    CHECK(courier->cmd_head == NULL, "final leg completes the command");

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
    {
        /* The pointer command takes a grid CELL: resolve world (5,5). */
        int cx5 = 0, cy5 = 0;
        CHECK(terrain_world_xy_to_cell(s_fix_terrain, 5.0f, 5.0f, &cx5, &cy5),
              "world (5,5) resolves to a cell");
        selection_cmd_move(&ctx, cx5, cy5);
    }
    cmd_queue_tick();
    run(&ctx, 20, 0.5f);
    CHECK(digger->x >= 4.9f && digger->y >= 4.9f, "queue move executes");
    CHECK(digger->cmd_head == NULL, "queue move unlinked after arrival");
    /* Test isolation: the selection list is process state, and later
     * sections' cmd_queue_tick calls dispatch for every entity still
     * selected. */
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
    CHECK(ai_activity_name(AI_ACT_CRUMBLE) != NULL &&
              strcmp(ai_activity_name(AI_ACT_CRUMBLE),
                     "Activity_Crumble") == 0 &&
              ai_activity_name(AI_ACT_TELEPORT_IN) != NULL &&
              strcmp(ai_activity_name(AI_ACT_TELEPORT_IN),
                     "Activity_TeleportIn") == 0 &&
              ai_activity_name(-1) == NULL &&
              ai_activity_name(AI_ACTIVITY_COUNT) == NULL,
          "activity name lookup covers all labels and rejects invalid ids");
    EntitySlot* actor = place(&ctx, 3, "actor", 0, 0);
    ai_set_activity(actor, AI_ACT_COLLECT, 7);
    /* FUN_004409e0 leaves +0x2ec alone; the paired FUN_00447c40 bookkeeping
     * observes the change and commits activity_last (faithful split). */
    activity_register_bookkeeping(actor);
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

    /* Wake timer (0x43c870): the callback ticks the timer and returns early
     * (no further callback behaviour). The faithful ai_tick no longer gates
     * its ordinary kind-0 mover on this flag (movement is the engine's
     * separate command pass), so the old "movement freezes" assertions are
     * replaced by the callback's own return value and activity-timer hold. */
    EntitySlot* sleeper = place(&ctx, 1, "sleeper", 0, 0);
    move_to(sleeper, 60.0f, 0.0f, 0);
    sleeper->flags_3e4 |= 0x08000000u;
    sleeper->wake_timer = 0.3f;
    sleeper->activity_timer = 0.0f;
    CHECK(ai_entity_frame_update(sleeper, 0.5f) == 1,
          "wake state early-returns from the callback");
    CHECK(sleeper->activity_timer == 0.0f,
          "the early return skips the activity-timer refresh");
    CHECK(sleeper->wake_timer > -0.3f && sleeper->wake_timer < -0.1f,
          "wake timer still decrements");
    sleeper->flags_3e4 = 0;
    CHECK(ai_entity_frame_update(sleeper, 0.5f) == 0 &&
              sleeper->activity_timer == 0.5f,
          "callback resumes (and refreshes the activity timer) once the wake flag clears");

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
    move_to(grantee, 10.0f, 0.0f, 0);
    CHECK(grantee->cmd_head != NULL, "move record at the chain head");
    if (grantee->cmd_head != NULL) {
        cmd_rec_set_activity(grantee->cmd_head, AI_ACT_TELEPORT);
    }
    grantee->flags_3e4 |= 0x00000040u;
    /* The branch lives in the callback receiver; ai_tick's separate mover
     * pass is not gated by it, so drive the receiver directly. */
    CHECK(ai_entity_frame_update(grantee, 0.5f) == 1,
          "grant branch returns early from the callback");
    CHECK((grantee->flags_3e4 & 0x00000040u) == 0, "grant flag consumed");
    CHECK(ai_activity_scan(grantee, AI_ACT_TELEPORT) == 1,
          "record grant lands on the entity's list");
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
    /* The spawn sets the task state; its capacity is bound from a same-name
     * Stats row (entity_task_state_capacity_bind) and stays 0 when no row
     * exists, as here. Bind it by hand for the grant tests below. */
    CHECK(minion8->task_state == AI_TASK_STATE_MINION_1 &&
              minion8->activity_cap == 0,
          "minion spawn assigns the task state; capacity needs a Stats row");
    minion8->activity_cap = AI_ACTIVITY_SLOTS;
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
    /* A task-state entity reads ONLY its own capacity (the context default
     * is just the fallback for entities with neither task state nor Stats),
     * so an unbound cap-0 state makes grants a no-op like the corpus. */
    minion8->activity_cap = 0;
    ai_activity_grant(&ctx, minion8, AI_ACT_ROUTE_RUBBLE);
    CHECK(minion8->act_count == 0 &&
              ai_activity_scan(minion8, AI_ACT_ROUTE_RUBBLE) == 0,
          "zero capacity makes grants a no-op");

    /* Deferred grant cleanup (0x43da8b): after installing the chain-head
     * grant, the corpus unlinks and frees every type-0x13 record
     * (FUN_00403d10(ent, 0x13, 0, 1)) — the port's cmd_chain_drop. */
    EntitySlot* grantee2 = place(&ctx, 8, "grantee2", 0, 0);
    grantee2->activity_cap = AI_ACTIVITY_SLOTS; /* no Stats row to bind it */
    move_to(grantee2, 10.0f, 0.0f, 0);
    if (grantee2->cmd_head != NULL) {
        cmd_rec_set_activity(grantee2->cmd_head, AI_ACT_DRILL);
        void* grec = cmd_alloc(0x13); /* type-0x13 grant record */
        cmd_rec_set_next(grantee2->cmd_head, grec);
        cmd_rec_set_next(grec, NULL);
    }
    grantee2->flags_3e4 |= 0x00000040u;
    CHECK(ai_entity_frame_update(grantee2, 0.5f) == 1,
          "cleanup branch returns early from the callback");
    CHECK((grantee2->flags_3e4 & 0x00000040u) == 0, "cleanup consumes flag");
    CHECK(ai_activity_scan(grantee2, AI_ACT_DRILL) == 1,
          "head grant lands on the minion list");
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
    /* The faithful FUN_0043a960 skips entities whose stats word carries the
     * walk-relief bit 0x02000000 (every fixture mover), and resolves the
     * tile through the level terrain: the idler gets no Stats record and
     * stands on an idle-type cell of the fixture level. */
    EntitySlot* idler = place(&ctx, 1, "idler", 0, 0);
    idler->stats_record = NULL;
    int idle0 = ai_idle_count();
    int idle_cx = 0, idle_cy = 0;
    CHECK(terrain_world_xy_to_cell(s_fix_terrain, 0.0f, 0.0f, &idle_cx,
                                   &idle_cy),
          "idler cell resolves");
    uint8_t* const idle_voxel =
        fix_grid + (idle_cy * FIX_DIM + idle_cx) * LEVEL_VOXEL_STRIDE;
    idle_voxel[2] = (uint8_t)TILE_TYPE_IDLE;
    idler->flags_3ec |= 0x40u;
    run(&ctx, 1, 0.5f);
    CHECK(ai_idle_count() == idle0 + 1, "idle table registration");
    CHECK((idler->flags_3ec & 0x40u) == 0, "idle flag cleared");
    idler->flags_3ec |= 0x40u;
    run(&ctx, 1, 0.5f);
    /* The faithful table append has no dedupe (it only caps at 100), so a
     * re-raised flag registers the entity a second time. */
    CHECK(ai_idle_count() == idle0 + 2, "re-registration appends (no dedupe)");
    idle_voxel[2] = 0;

    /* --- tile-map wiring (faithful route gate over the fixture grid) ---- *
     * The faithful mover routes through terrain_neighbor_path_check, whose
     * per-cell gate (0x432770 family) lets a plain entity enter a cell only
     * when the voxel flag word carries 0x8. The legacy 0x10-layer/0x40-wall/
     * 0x8000000-occupancy model of the pre-#372 stand-in is gone from the
     * mover, so these scenarios drive the same intents (blocked, detour,
     * executor) through that flag. */

    {
        /* A target cell without the walk flag cannot be routed: the command
         * stays live and the walker does not budge (no straight-line
         * fallback); once the cell opens the walker finishes the move. */
        EntitySlot* stuck = place(&ctx, 1, "stuck", 0, 0);
        uint32_t* const goal_flags = fix_flags(4.0f, 0.0f);
        const uint32_t goal_saved = *goal_flags;
        *goal_flags = 0u;
        move_to(stuck, 4.0f, 0.0f, 0);
        run(&ctx, 2, 0.5f);
        CHECK(stuck->x == 0.0f && stuck->cmd_head != NULL &&
                  stuck->wp_count == 0,
              "an unroutable target keeps the walker put with its command live");
        *goal_flags = goal_saved;
        run(&ctx, 6, 0.5f);
        CHECK(stuck->x >= 3.9f && stuck->cmd_head == NULL,
              "the walker finishes the move once the target opens");

        /* Detour: a wall column (cells without the walk flag) forces a
         * multi-entry route around it; the walker never stands on the wall. */
        uint32_t wall_saved[5];
        for (int wy = 1; wy <= 5; ++wy) {
            wall_saved[wy - 1] = *fix_flags(5.0f, (float)wy);
            *fix_flags(5.0f, (float)wy) = 0u;
        }
        EntitySlot* detour = place(&ctx, 1, "detour", 1, 3);
        move_to(detour, 9.0f, 3.0f, 0);
        run(&ctx, 1, 0.5f);
        CHECK(detour->wp_count > 9,
              "the pathfinder routes around the wall (longer than straight)");
        int arrived = 0;
        for (int t = 0; t < 40 && !arrived; ++t) {
            run(&ctx, 1, 0.5f);
            int px = (int)(detour->x + 0.5f), py = (int)(detour->y + 0.5f);
            CHECK(!(px == 5 && py >= 1 && py <= 5),
                  "the detour never lands on the blocked column");
            arrived = detour->cmd_head == NULL;
        }
        CHECK(arrived && fabsf(detour->x - 9.0f) < 0.01f &&
                  fabsf(detour->y - 3.0f) < 0.01f,
              "the walker reroutes around the blocked column");

        /* Executor path: a selection opcode-5 move dispatches through the
         * queue executor into CmdCreateMove and walks around the same wall. */
        EntitySlot* mailrun = place(&ctx, 1, "mailrun", 1, 3);
        selection_add(&ctx, mailrun);
        {
            int mx, my;
            cell_of(9.0f, 3.0f, &mx, &my);
            selection_cmd_move(&ctx, mx, my); /* demo 'm': opcode 5 */
        }
        cmd_queue_tick(); /* the per-frame executor (FUN_004520d0) */
        CHECK(mailrun->cmd_head != NULL && cmd_rec_type(mailrun->cmd_head) == 0,
              "queue move dispatches a move command onto the chain");
        int mail_arrived = 0;
        for (int t = 0; t < 40 && !mail_arrived; ++t) {
            run(&ctx, 1, 0.5f);
            int px = (int)(mailrun->x + 0.5f);
            int py = (int)(mailrun->y + 0.5f);
            CHECK(!(px == 5 && py >= 1 && py <= 5),
                  "the executor move never lands on the blocked column");
            mail_arrived = mailrun->cmd_head == NULL;
        }
        CHECK(mail_arrived && fabsf(mailrun->x - 9.0f) < 0.01f &&
                  fabsf(mailrun->y - 3.0f) < 0.01f,
              "the move command's route reroutes around the column");
        CHECK((mailrun->flags_3e0 & 0x1u) == 0,
              "arrival clears the +0x3e0 mover flag");
        selection_clear(&ctx);

        /* Equal cells: the route is the one start cell and finishes on the
         * first tick. */
        EntitySlot* parked = place(&ctx, 1, "parked", 1, 3);
        move_to(parked, 1.0f, 3.0f, 0);
        run(&ctx, 1, 0.5f);
        CHECK(parked->cmd_head == NULL && parked->x == 1.0f,
              "the zero-distance move completes on the first tick");

        /* Unreachable target: the wall spans the whole column, so the route
         * search exhausts and the command is left live and unrouted. */
        uint32_t* col_saved = (uint32_t*)malloc(FIX_DIM * sizeof(uint32_t));
        for (int cy = 0; cy < FIX_DIM; ++cy) {
            uint32_t* f = (uint32_t*)(fix_grid +
                ((size_t)cy * FIX_DIM + 70u) * LEVEL_VOXEL_STRIDE + 8);
            col_saved[cy] = *f;
            *f = 0u;
        }
        EntitySlot* stranded = place(&ctx, 1, "stranded", 1, 5);
        int wall_cx, wall_cy;
        cell_of(1.0f, 5.0f, &wall_cx, &wall_cy);
        {
            /* target on the far side of grid column 70 (world x = 39) */
            float tx[2] = { 0, 0 };
            (void)tx;
            move_to(stranded, 39.0f, 5.0f, 0);
        }
        run(&ctx, 2, 0.5f);
        CHECK(stranded->wp_count == 0 && stranded->cmd_head != NULL &&
                  stranded->x == 1.0f,
              "an unreachable target installs no route and the walker stays");
        cmd_chain_purge(stranded);
        for (int cy = 0; cy < FIX_DIM; ++cy) {
            *(uint32_t*)(fix_grid +
                ((size_t)cy * FIX_DIM + 70u) * LEVEL_VOXEL_STRIDE + 8) =
                col_saved[cy];
        }
        free(col_saved);
        for (int wy = 1; wy <= 5; ++wy) {
            *fix_flags(5.0f, (float)wy) = wall_saved[wy - 1];
        }

        /* Idle probe (FUN_0043a960): the flag registers only on '\t' cells.
         * The flag is consumed either way. */
        EntitySlot* idler2 = place(&ctx, 1, "idler2", 8, 0);
        idler2->stats_record = NULL;
        int idle1 = ai_idle_count();
        idler2->flags_3ec |= 0x40u;
        run(&ctx, 1, 0.5f);
        CHECK(ai_idle_count() == idle1,
              "idle registration blocked off the '\\t' tile");
        {
            int ix, iy;
            cell_of(8.0f, 0.0f, &ix, &iy);
            uint8_t* const iv = fix_grid +
                ((size_t)iy * FIX_DIM + (size_t)ix) * LEVEL_VOXEL_STRIDE;
            iv[2] = '\t';
            idler2->flags_3ec |= 0x40u;
            run(&ctx, 1, 0.5f);
            CHECK(ai_idle_count() == idle1 + 1,
                  "idle registration on '\\t' tile");
            iv[2] = 0;
        }
    }

    /* --- legacy wave-flood builder (FUN_00430ee0/FUN_00430530), direct ---- *
     * ai_path_build is no longer the mover's route source (the faithful
     * creator seeds nothing and the tick routes through the terrain path
     * finder), but the builder is still ported and callable. These cases
     * drive it directly on a terrain-less synthetic grid (0x10 layer bit
     * walkable, 0x40 wall, 0x40000 wave mark) — issue #70. */
    {
        static uint8_t voxels[16 * 16 * 0x48];
        LevelState map;
        memset(&map, 0, sizeof(map));
        memset(voxels, 0, sizeof(voxels));
        level_set_int(&map, LEVEL_OFF_WIDTH, 16);
        level_set_int(&map, LEVEL_OFF_HEIGHT, 16);
        level_store_ptr(&map, LEVEL_OFF_VOXELS, voxels);
        map.ptr_terrain = s_fix_terrain; /* start cells resolve through it */
        LevelState* const saved_level = ctx.level;
        ctx.level = &map;
        /* With a terrain bound, the seed's northwest-2x2 TerrainCellInitialize
         * pass rewrites those cells' flag words from the (all-zero) terrain,
         * clobbering the synthetic walk bits around the goal. Re-lay the
         * grid before every build so each case starts from a clean map. */
        auto relay = [&](int wall_column) {
            memset(voxels, 0, sizeof(voxels));
            for (int cy = 0; cy <= 13; ++cy) {
                for (int cx = 0; cx <= 13; ++cx) {
                    *(uint32_t*)(voxels + ((size_t)cy * 16 + cx) * 0x48 + 8) |=
                        0x10u;
                }
                if (wall_column) {
                    *(uint32_t*)(voxels + ((size_t)cy * 16 + 5) * 0x48 + 8) |=
                        0x40u;
                }
            }
        };

        /* Multi-cell path on a clean grid: a straight 4-cell flood to a
         * target one margin-row away, installed in walk order. */
        EntitySlot* runner = place(&ctx, 1, "runner", 0, 0);
        set_cell(runner, 1, 1);
        relay(0);
        /* The seed's init pass zeroes the goal's northwest 2x2 block
         * ((4,0),(5,0),(4,1),(5,1)) before the spread, so the faithful
         * route cannot run straight along row 1: it drops to row 2 and
         * climbs into the goal from (5,2). */
        CHECK(ai_path_build(&ctx, runner, 5.0f, 1.0f) == 6 &&
                  runner->wp_count == 6,
              "wave-flood builds the multi-cell path on a clean grid");
        CHECK(runner->wp[0].x == 1.0f && runner->wp[0].y == 2.0f &&
                  runner->wp[5].x == 5.0f && runner->wp[5].y == 1.0f,
              "the traceback installs the flooded cells in walk order");
        CHECK(ai_wave_flood_count() > runner->wp_count,
              "the wave spread marks more cells with flag 0x40000 than the path uses");

        /* Retargeting mid-move (PR #87 feedback): the mover's own cell
         * carries its occupancy reservation, so a rebuild must exempt it. */
        EntitySlot* retarget = place(&ctx, 1, "retarget", 0, 0);
        set_cell(retarget, 1, 5);
        relay(0);
        const int first_len = ai_path_build(&ctx, retarget, 8.0f, 5.0f);
        CHECK(first_len >= 7, "the first build routes the move");
        relay(0);
        level_tile_mark(&map, 1, 5, 1); /* the mover's own reservation */
        CHECK(ai_path_build(&ctx, retarget, 8.0f, 5.0f) == first_len,
              "the rebuild exempts the mover's own reservation at the start");

        /* Seed flag footprint (PR #87 feedback): record 12 raises the
         * flag over the northwest 2x2 block including the seed cell. */
        EntitySlot* seedcell = place(&ctx, 1, "seedcell", 0, 0);
        set_cell(seedcell, 3, 12);
        relay(0);
        /* The target sits one step WEST of the start: an eastward target
         * would put the start inside the goal's zeroed footprint, which the
         * faithful init pass makes unwalkable. */
        CHECK(ai_path_build(&ctx, seedcell, 2.0f, 12.0f) == 1,
              "a one-step target roots a one-cell traceback");
        CHECK(ai_wave_flood_count() == 5,
              "the seed raises record 12's northwest 2x2 footprint flag");

        /* Unreachable target: a full-height blocked column floods the target
         * side, exhausts, and the traceback never roots at the start. */
        relay(1);
        EntitySlot* stranded2 = place(&ctx, 1, "stranded2", 0, 0);
        set_cell(stranded2, 1, 5);
        CHECK(ai_path_build(&ctx, stranded2, 9.0f, 5.0f) == 0,
              "unreachable target across the full wall yields no path");
        CHECK(stranded2->wp_count == 0,
              "a failed build installs nothing");
        CHECK(ai_wave_flood_count() > 0,
              "the wave still spread across the target side before exhausting");

        /* Equal cells: the seed validates the target (it is the start), so
         * the traceback is a zero-cell walk and yields no path. */
        EntitySlot* parked2 = place(&ctx, 1, "parked2", 0, 0);
        set_cell(parked2, 1, 3);
        CHECK(ai_path_build(&ctx, parked2, 1.0f, 3.0f) == 0,
              "start-equal-target builds no path");

        /* No-grid case: no handle, no state. */
        ctx.level = NULL;
        EntitySlot* gridless = place(&ctx, 1, "gridless", 0, 0);
        CHECK(ai_path_build(&ctx, gridless, 6.0f, 6.0f) == 0,
              "no grid handle: no path and no flood state");
        CHECK(ai_wave_flood_count() == 0,
              "a no-grid build owns no wave work state");
        /* The faithful scheduler has no straight-line fallback either: with
         * no level the move is never routed and the walker stays put. */
        move_to(gridless, 6.0f, 6.0f, 0);
        run(&ctx, 3, 0.5f);
        CHECK(gridless->wp_count == 0 && gridless->x == 0.0f &&
                  gridless->cmd_head != NULL,
              "no level: the move is left unrouted and the walker stays");
        cmd_chain_purge(gridless);
        ctx.level = saved_level;
    }

    /* --- per-terrain movement speed (FUN_00446610 route speed) ----------- *
     * The ordinary mover's rate is the Stats RouteSpeed scaled per cell by
     * 1 - (1 - surface_scale(+0xd4)) * cell_cost * 0.25 (cell_cost from the
     * voxel flag word: (flags & 3) + 1, or 0 with 0x200000), so the rows
     * differ by flag word, not by the legacy terrain-type speed table. */
    {
        uint8_t speed_stats[ENTITY_STATS_RECORD_SIZE];
        memcpy(speed_stats, s_fix_stats, sizeof(speed_stats));
        const float half_surface = 0.5f;
        memcpy(speed_stats + 0xd4, &half_surface, sizeof(half_surface));
        for (int wx = 4; wx <= 7; ++wx) {
            *fix_flags((float)wx, 5.0f) = 0x8u | 3u;       /* cost 4: slow */
            *fix_flags((float)wx, 7.0f) = 0x8u | 0x200000u; /* cost 0: fast */
        }
        ctx.stats_speed_sec[4] = 2.0f;  /* corpus units: sec/cell -> 0.5 cps */
        ctx.stats_speed_sec[5] = 0.25f; /* -> 4 cps */

        /* The legacy table still backs the target-chase adapter; the probe
         * reads it by terrain index, and unset/out-of-range types fall back
         * to the default (1/AI_MOVE_SPEED_CPS). */
        CHECK(ai_stats_speed(&ctx, 4) == 2.0f, "speed table entry for type 4");
        CHECK(ai_stats_speed(&ctx, 5) == 0.25f, "speed table entry for type 5");
        CHECK(ai_stats_speed(&ctx, 0) * AI_MOVE_SPEED_CPS == 1.0f,
              "unset entry defaults to the port speed");
        CHECK(ai_stats_speed(&ctx, -1) * AI_MOVE_SPEED_CPS == 1.0f &&
                  ai_stats_speed(&ctx, AI_STATS_TERRAIN_TYPES) *
                      AI_MOVE_SPEED_CPS == 1.0f,
              "out-of-range terrain types are guarded to the default");

        EntitySlot* slow = place(&ctx, 1, "slow", 3, 5);
        EntitySlot* plain = place(&ctx, 1, "plain", 3, 6);
        EntitySlot* fast = place(&ctx, 1, "fast", 3, 7);
        slow->stats_record = plain->stats_record = fast->stats_record =
            speed_stats;
        move_to(slow, 11.0f, 5.0f, 0);
        move_to(plain, 11.0f, 6.0f, 0);
        move_to(fast, 11.0f, 7.0f, 0);

        int t_slow = 0, t_plain = 0, t_fast = 0;
        float lag = 0.0f;
        for (int t = 1; t <= 64 && !(t_slow && t_plain && t_fast); ++t) {
            ai_tick(&ctx, 0.5f);
            if (t == 1) {
                CHECK(slow->wp_count > 1, "the speed rows walk straight-through "
                                          "paths");
            }
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

        /* Arrival retires the path (cursor and count reset together). */
        CHECK(slow->wp_index == slow->wp_count && slow->cmd_head == NULL,
              "slow walker arrival retires the path");
        CHECK(fabsf(fast->x - 11.0f) < 0.01f && fabsf(fast->y - 7.0f) < 0.01f,
              "fast walker lands exactly");

        for (int wx = 4; wx <= 7; ++wx) {
            *fix_flags((float)wx, 5.0f) = 0x8u;
            *fix_flags((float)wx, 7.0f) = 0x8u;
        }
        slow->stats_record = plain->stats_record = fast->stats_record =
            s_fix_stats;
        ctx.stats_speed_sec[4] = 0.0f;
        ctx.stats_speed_sec[5] = 0.0f;
    }

    /* Move and target records stay on entity chains; dropping a target
     * leaves the move intact and does not alter the task-only global list. */
    void* global_before_drop = cmd_global_head();
    EntitySlot* dropper = place(&ctx, 1, "dropper", 11, 5);
    move_to(dropper, 10.0f, 5.0f, 0);
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
    move_to(linked, 11.0f, 5.0f, 0);
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

    /* Rockfall-queue eligibility (2026-10-24). */
    {
        const int before = ai_idle_count();
        EntitySlot e;
        memset(&e, 0, sizeof(e));
        e.type = 1;
        entity_queue_for_rockfall_processing_if_eligible(NULL);
        entity_queue_for_rockfall_processing_if_eligible(&e);
        CHECK(ai_idle_count() == before,
              "no level means the terrain-qualified record is skipped");
        entity_queue_for_rockfall_processing_if_eligible(&e);
        CHECK(ai_idle_count() == before,
              "a repeated record without a level remains skipped");
        EntitySlot other;
        memset(&other, 0, sizeof(other));
        other.type = 5;
        entity_queue_for_rockfall_processing_if_eligible(&other);
        CHECK(ai_idle_count() == before, "a type-5 entity never lands");
    }

    /* Activity-request wrappers (2026-10-24): corpus indices 5 and 30. */
    {
        GameContext ctx;
        game_context_init(&ctx);
        int idx = entity_create(&ctx, 1, "ActReq", 0.0f, 0.0f, 0.0f, 0.0f);
        CHECK(idx >= 0, "activity-request probe slot created");
        if (idx >= 0) {
            EntitySlot* e = &ctx.entity[0][idx];
            e->flags_3e0 = 0u;
            e->flags_3e4 = 0u;
            entity_dispatch_event5_and_set_flag(e);
            CHECK(e->activity == 5
                      && (e->flags_3e0 & 0x200000u) != 0u,
                  "the index-5 request sets activity 5 and bit 0x200000");
            CHECK(entity_set_activity_place(e) == 1
                      && e->activity == 30
                      && (e->flags_3e4 & 0x200u) != 0u,
                  "the index-30 request sets activity 30 and bit 0x200");
            entity_dispatch_event5_and_set_flag(NULL); /* guarded */
            CHECK(entity_set_activity_place(NULL) == 1, "the null store answers 1");
        }
    }

    /* EntityActivitySceneStateUpdate (2026-10-24): the gate + clear. */
    {
        entity_state_flags_set(0u);
        GameContext ctx;
        game_context_init(&ctx);
        LevelState lv;
        memset(&lv, 0, sizeof lv);
        ctx.level = &lv;
        global_object_field98_add_for_level(&lv, 7);
        entity_activity_scene_state_update(&ctx);
        CHECK(entity_state_flags_peek() == 4u && level_field98_get(&lv) == 0u,
              "the gate-off pass raises bit 4 and clears +0x98");
        entity_state_flags_set(0x10u);
        global_object_field98_add_for_level(&lv, 3);
        entity_activity_scene_state_update(&ctx);
        CHECK(entity_state_flags_peek() == 0x18u && level_field98_get(&lv) == 3u,
              "the gate-on pass raises bit 8 and leaves the state alone");
        entity_state_flags_set(0u);
        entity_activity_scene_state_update(NULL); /* level guard */
        CHECK(entity_state_flags_peek() == 4u, "the NULL ctx still updates");
        entity_state_flags_set(0u);
    }

    /* SceneObjectHasTaskType4_5_7 (2026-10-24): gate + chain. */
    {
        EntitySlot e;
        memset(&e, 0, sizeof e);
        CHECK(scene_object_has_task_type_4_5_7(&e) == 0,
              "the type gate answers for non-type-2");
        e.type = 2;
        CHECK(scene_object_has_task_type_4_5_7(&e) == 0,
              "type 2 with no activities answers 0");
        e.act_count = 1;
        e.act_list[0] = 5;
        CHECK(scene_object_has_task_type_4_5_7(&e) == 1,
              "the teleport entry answers through the chain");
        e.act_list[0] = 7;
        CHECK(scene_object_has_task_type_4_5_7(&e) == 1,
              "the reinforce entry answers through the chain");
        e.act_count = 0;
        CHECK(scene_object_has_task_type_4_5_7(NULL) == 0,
              "the NULL slot is guarded");
    }

    /* The entity activity gate band (2026-10-24). */
    {
        EntitySlot e;
        memset(&e, 0, sizeof e);
        /* EntityDispatchEvent31ForPendingFlag: the three-condition gate. */
        e.type = 3;
        entity_dispatch_event31_for_pending_flag(&e);
        CHECK(e.activity == 0 && e.flags_3e4 == 0u,
              "the clear pending bit keeps the gate silent");
        e.flags_3e8 = 0x80000000u;
        entity_dispatch_event31_for_pending_flag(&e);
        CHECK(e.flags_3e4 == 0x20000000u && e.activity == 0x31
                  && e.activity_last == 0x31,
              "the pending pass raises the bit and sets activity 0x31");
        e.activity = 0;
        entity_dispatch_event31_for_pending_flag(&e);
        CHECK(e.activity == 0, "the raised marker bit keeps the gate silent");
        e.flags_3e4 = 0u;
        e.activity = 0;
        e.type = 1;
        entity_dispatch_event31_for_pending_flag(&e);
        CHECK(e.activity == 0, "the type gate stays silent");
        entity_dispatch_event31_for_pending_flag(NULL); /* guarded */

        /* EntityActivity23StateTransition: the half-word + gated pass. */
        memset(&e, 0, sizeof e);
        e.type = 2;
        entity_activity23_state_transition(&e);
        CHECK(e.flags_3e4 == 0x800u && (e.flags_3e0 & 0x40000000u) != 0u
                  && e.activity == 0x23 && e.cmd_head != NULL,
              "the type-2 pass raises the halves and registers idle");
        e.flags_3e0 |= 0x400u; /* raise the gate bit the pass checks */
        e.activity = 0;
        e.type = 3;
        entity_activity23_state_transition(&e);
        CHECK(e.activity == 0 && (e.flags_3e4 & 0x800u) != 0u,
              "the raised 0x400 bit leaves the activity alone");
        memset(&e, 0, sizeof e);
        e.type = 1;
        entity_activity23_state_transition(&e);
        CHECK(e.activity == 0x23 && (e.flags_3e4 & 0x800u) == 0u,
              "the gate pass skips the type half-word");
        entity_activity23_state_transition(NULL); /* guarded */
    }

    /* The entity counter + link band (2026-10-24). */
    {
        GameContext ctx;
        game_context_init(&ctx);
        LevelState lv;
        memset(&lv, 0, sizeof lv);
        ctx.level = &lv;
        /* SimulationUpdateCounterAdd: the loop + scene-state pass. */
        entity_state_flags_set(0u);
        global_object_field98_add_for_level(&lv, 9);
        simulation_update_counter_add(&ctx, 3);
        CHECK((int)level_field8c_get(&lv) == 3 && entity_state_flags_peek() == 4u
                  && level_field98_get(&lv) == 0u,
              "the counter loops and the state pass clears +0x98");
        simulation_update_counter_add(&ctx, 0);
        CHECK((int)level_field8c_get(&lv) == 3, "a zero count leaves it alone");
        simulation_update_counter_add(NULL, 2); /* guarded */
        entity_state_flags_set(0u);

        /* EntityFlag80000000SetOrClear: set/clear + the state pass. */
        EntitySlot e;
        memset(&e, 0, sizeof e);
        global_object_field98_add_for_level(&lv, 5);
        entity_flag80000000_set_or_clear(&ctx, &e, 0);
        CHECK(e.flags_3e8 == 0x80000000u && entity_state_flags_peek() == 4u
                  && level_field98_get(&lv) == 0u,
              "the clear param raises the high bit and runs the pass");
        entity_flag80000000_set_or_clear(&ctx, &e, 1);
        CHECK(e.flags_3e8 == 0u, "the set param clears the high bit");
        entity_flag80000000_set_or_clear(&ctx, NULL, 0); /* guarded */
        entity_state_flags_set(0u);

        /* EntityNestedFloatAddUnlessBlocked: the gate + fold. */
        static char rec[0xe0];
        memset(rec, 0, sizeof rec);
        const float add = 2.5f;
        __builtin_memcpy(rec + 0xdc, &add, sizeof add);
        memset(&e, 0, sizeof e);
        e.stats_record = rec;
        e.type = 5;
        float v = 1.0f;
        CHECK(entity_nested_float_add_unless_blocked(&e, &v) == 0
                  && v == 3.5f,
              "the non-type-4 fold lands regardless of the gate");
        e.type = 4;
        v = 1.0f;
        entity_nested_float_add_unless_blocked(&e, &v);
        CHECK(v == 1.0f, "the ineligible type-4 entity skips the fold");
        e.flags_3e8 = 0x20000000u; /* eligible half of the strict clause */
        entity_nested_float_add_unless_blocked(&e, &v);
        CHECK(v == 3.5f, "the eligible type-4 fold lands");
        entity_nested_float_add_unless_blocked(NULL, &v); /* guarded */

        /* LinkedEntityFlagAndCommandUpdate: the link flag + drop. */
        EntitySlot parent;
        EntitySlot child;
        memset(&parent, 0, sizeof parent);
        memset(&child, 0, sizeof child);
        child.type = 4;
        parent.link_2fc = (uintptr_t)&child;
        linked_entity_flag_and_command_update(&parent);
        CHECK(parent.flags_3ec == 0x40u && child.flags_3ec == 0x40u
                  && parent.cmd_head == NULL,
              "the linked pair raises the bit and drops kind 0x1c");
        child.type = 3;
        parent.flags_3ec = 0u;
        linked_entity_flag_and_command_update(&parent);
        CHECK(parent.flags_3ec == 0u, "the non-type-4 link stays silent");
        parent.link_2fc = 0u;
        linked_entity_flag_and_command_update(&parent); /* clear link */
        linked_entity_flag_and_command_update(NULL); /* guarded */
    }

    /* EntityRandomActivityAttempt (FUN_00443670, 2026-10-24). */
    {
        EntitySlot e;
        memset(&e, 0, sizeof e);
        CHECK(entity_random_activity_attempt(NULL) == 0, "NULL is guarded");
        /* The faithful dispatch's success test is the type-2/type-3 scene
         * runner's answer (FUN_00447c40 result); a bare slot has no bound
         * runner (that needs a type-config row and the renderer callbacks),
         * so all five draws fail: the call answers 0, leaves the last draw
         * committed in the code band, and never raises the 0x8000 bit. */
        const int took = entity_random_activity_attempt(&e);
        CHECK(took == 0 && e.activity >= 0x38 && e.activity <= 0x4c
                  && (e.flags_3e0 & 0x8000u) == 0u
                  && e.activity_last == e.activity,
              "a cold slot without a scene runner exhausts five code-band draws");

        /* The gate pair answers zero before any draw. */
        e.flags_3e0 |= 0x400u;
        CHECK(entity_random_activity_attempt(&e) == 0, "the gate bit blocks");
        e.flags_3e0 = 0u;
        e.flags_3e0 |= 0x2u; /* mask-word intersection (bit 1 is in the mask) */
        CHECK(entity_random_activity_attempt(&e) == 0, "the mask blocks");
        e.flags_3e0 = 0u;

        /* Repeat attempts keep failing the same way (no runner), each call
         * drawing fresh code-band activities and committing the last. */
        for (int hops = 0; hops < 3; ++hops) {
            CHECK(entity_random_activity_attempt(&e) == 0 &&
                      e.activity >= 0x38 && e.activity <= 0x4c &&
                      (e.flags_3e0 & 0x8000u) == 0u,
                  "a repeat attempt without a runner stays failed");
        }
    }

    /* Object registry windows (2026-10-25). */
    {
        ai_object_registries_reset();
        EntitySlot e4, e2a, e2b, ef;
        memset(&e4, 0, sizeof e4);
        memset(&e2a, 0, sizeof e2a);
        memset(&e2b, 0, sizeof e2b);
        memset(&ef, 0, sizeof ef);
        /* The registries gate on the corpus type word (`type`); `etype` is
         * the Stats row index now, not the type code. */
        e4.type = 4;
        e2a.type = 2;
        e2b.type = 2;
        static char caprec[0x150];
        memset(caprec, 0, sizeof caprec);
        const uint32_t capbit = 0x80u;
        __builtin_memcpy(caprec + 0x14c, &capbit, sizeof capbit);
        /* ef starts with no stats record: the gate reads zero. */

        type4_object_registry_insert(NULL); /* guarded */
        type4_object_registry_insert(&e2a); /* the type-4 gate skips */
        void* rec = (void*)1;
        uint32_t tmr = 1u;
        ai_object_registry_slot_peek(1, 0, &rec, &tmr);
        CHECK(rec == NULL, "the gated insert leaves the window empty");
        type4_object_registry_insert(&e4);
        ai_object_registry_slot_peek(1, 0, &rec, &tmr);
        CHECK(rec == &e4 && tmr == 0u, "the type-4 insert rides slot zero");
        type4_object_registry_insert(&e4); /* the no-duplicate rule */
        int seen = 0;
        for (int i = 0; i < 20; ++i) {
            ai_object_registry_slot_peek(1, i, &rec, NULL);
            if (rec == &e4) {
                ++seen;
            }
        }
        CHECK(seen == 1, "the repeat insert stays deduped");

        flagged_object_timer_registry_insert(&ef); /* capability gate off */
        ai_object_registry_slot_peek(0, 0, &rec, NULL);
        CHECK(rec == NULL, "the gated flagged insert skips");
        flagged_object_timer_registry_insert(NULL); /* guarded */
        /* Raise the gate through the stats word. */
        ef.stats_record = caprec;
        flagged_object_timer_registry_insert(&ef);
        ai_object_registry_slot_peek(0, 0, &rec, &tmr);
        CHECK(rec == &ef && tmr == 0x42160000u,
              "the flagged insert stores with the seed");
        const uint32_t late = 7u;
        ai_object_registry_slot_set(0, 0, &ef, late);
        flagged_object_timer_registry_insert(&ef); /* the hit re-seeds */
        ai_object_registry_slot_peek(0, 0, &rec, &tmr);
        CHECK(tmr == 0x42160000u, "the hit re-seeds the timer");

        type2_timed_registry_insert(&e2a);
        type2_timed_registry_insert(&e2b);
        ai_object_registry_slot_peek(2, 0, &rec, &tmr);
        void* rec2 = NULL;
        ai_object_registry_slot_peek(2, 1, &rec2, NULL);
        CHECK(rec == &e2a && rec2 == &e2b
                  && tmr == 0x4a095440u,
              "the timed inserts append with the seed");
        ai_object_registry_slot_set(2, 0, &e2a, 3u);
        type2_timed_registry_insert(&e2a); /* the hit re-seeds */
        ai_object_registry_slot_peek(2, 0, &rec, &tmr);
        CHECK(tmr == 0x4a095440u, "the timed hit re-seeds");
        ai_object_registry_slot_set(2, 0, &e2a, 5u);
        ai_object_registry_slot_set(2, 1, &e2b, 5u);
        set_type2_object_record_value(&e4, 0x1234u); /* gate skips */
        ai_object_registry_slot_peek(2, 0, NULL, &tmr);
        CHECK(tmr == 5u, "the non-type-2 broadcast is gated off");
        set_type2_object_record_value(NULL, 0x1234u); /* guarded */
        set_type2_object_record_value(&e2a, 0x1234u);
        uint32_t t0 = 0, t1 = 0;
        ai_object_registry_slot_peek(2, 0, NULL, &t0);
        ai_object_registry_slot_peek(2, 1, NULL, &t1);
        CHECK(t0 == 0x1234u && t1 == 0x1234u,
              "the broadcast lands on the occupied slots");

        entity_pointer_registries_remove(&e2a);
        ai_object_registry_slot_peek(2, 0, &rec, NULL);
        ai_object_registry_slot_peek(2, 1, &rec2, NULL);
        CHECK(rec == NULL && rec2 == &e2b, "the remove drops the match");
        entity_pointer_registries_remove(&ef);
        ai_object_registry_slot_peek(0, 0, &rec, NULL);
        CHECK(rec == NULL, "the remove clears the flagged window");
        entity_pointer_registries_remove(NULL); /* guarded sweep */

        CHECK(ai_object_registry_window_count(0) == 20
                  && ai_object_registry_window_count(1) == 20
                  && ai_object_registry_window_count(2) == 50,
              "the windows ride the corpus geometries");
        ai_object_registry_slot_peek(2, 99999, &rec, &tmr); /* guarded */
        ai_object_registry_slot_set(1, -1, &e4, 0u); /* guarded */
        ai_object_registries_reset();
        ai_object_registry_slot_peek(1, 0, &rec, NULL);
        CHECK(rec == NULL, "the reset empties the windows");
    }

    /* The idle-tile update band (2026-10-25): link store, unregister, and
     * the activity/terrain eligibility transition. */
    {
        ai_idle_table_reset();
        /* Earlier blocks' scratch contexts stole (and dropped) the entity
         * pool owner; the rockfall forwarder reads the level through it, and
         * the faithful probe needs an idle-type tile under the entity. */
        entity_pool_callback_context_set(&ctx);
        uint8_t* const idle_cell = (uint8_t*)fix_flags(3.0f, 5.0f) - 8;
        idle_cell[2] = (uint8_t)TILE_TYPE_IDLE;
        EntitySlot idle, four;
        memset(&idle, 0, sizeof idle);
        memset(&four, 0, sizeof four);
        idle.type = 1;
        idle.x = 3.0f;
        idle.y = 5.0f;
        four.type = 4;
        entity_queue_for_rockfall_processing_if_eligible(&idle);
        CHECK(ai_idle_count() == 1, "the probe lands one entry");
        entity_update_and_unregister_at_tile(3, 5, &four);
        CHECK(idle.link_2fc == (uintptr_t)&four, "the link word stores");
        CHECK((idle.flags_3ec & 0x40u) != 0u && (four.flags_3ec & 0x40u) != 0u,
              "the type-4 link raises both +0x3ec bits");
        entity_update_and_unregister_at_tile(3, 5, NULL);
        CHECK(idle.link_2fc == (uintptr_t)&four,
              "the blanked slot no longer hits (unregister)");
        entity_update_and_unregister_at_tile(9, 9, NULL); /* coord miss */
        CHECK((four.flags_3ec & 0x40u) != 0u, "the coordinate miss is inert");
        ai_idle_table_reset();
        CHECK(ai_idle_count() == 0, "the hook empties the table");
        idle_cell[2] = 0;
    }

    /* EntityActivityTerrainEligibilityTransition (2026-10-25): the gate,
     * the fail fold, the spent-timer pass, and the handoff. */
    {
        EntitySlot e;
        memset(&e, 0, sizeof e);
        int out = -1;
        CHECK(entity_activity_terrain_eligibility_transition(NULL, &e, 1.0f,
                                                             2.0f, &out) == 0
                  && out == 0,
              "the gate-off answer clears the result word");
        GameContext ctx;
        game_context_init(&ctx);
        e.flags_3e0 = 0x40u;
        CHECK(entity_activity_terrain_eligibility_transition(&ctx, &e, 1.0f,
                                                             2.0f, NULL) == 0,
              "the missing level stays inert (deviation guard)");

        static LevelState lv;
        static uint8_t vox[8 * 8 * 0x48];
        memset(&lv, 0, sizeof lv);
        lv.ptr_voxels = vox;
        level_set_int(&lv, LEVEL_OFF_WIDTH, 8);
        level_set_int(&lv, LEVEL_OFF_HEIGHT, 8);
        ctx.level = &lv;
        e.flags_3e8 = 0x4u;
        e.flags_3e0 = 0x40u;
        out = -1;
        CHECK(entity_activity_terrain_eligibility_transition(&ctx, &e, 3.5f,
                                                             3.5f, &out) == 1
                  && out == 0,
              "the ineligible cell answers 1 with the result word cleared");
        CHECK(e.activity == 11 && e.activity_param == 1,
              "the fail fold lands activity 11 parameter 1");
        CHECK(e.flags_3e0 == 0x80000000u,
              "the fail fold moves the gate bit to the eligibility bit");

        /* Eligible cell (1,2): the walkable flag raised. */
        uint8_t* rec = vox + ((size_t)2 * 8 + 1) * 0x48;
        const uint32_t walk = 0x10u;
        __builtin_memcpy(rec + 0x08, &walk, sizeof walk);
        e.activity_timer = 0.0f;
        e.flags_3e0 = 0x40u;
        out = -1;
        CHECK(entity_activity_terrain_eligibility_transition(&ctx, &e, 1.5f,
                                                             2.5f, &out) == 1
                  && out == 0,
              "the spent timer returns 1 untouched");
        uint32_t now = 0u;
        __builtin_memcpy(&now, rec + 0x08, sizeof now);
        CHECK(now == 0x10u, "the spent pass queues no work");

        e.activity_timer = 1.0f;
        e.flags_3e0 = 0x40u;
        e.pos_pair_2f4 = 5.0f;
        e.pos_pair_2f8 = 6.0f;
        out = 0;
        CHECK(entity_activity_terrain_eligibility_transition(&ctx, &e, 1.5f,
                                                             2.5f, &out) == 1
                  && out == 1,
              "the running timer hands off with the result word raised");
        CHECK(e.activity == 0 && e.activity_param == 0,
              "the handoff folds activity 0");
        CHECK(e.flags_3e0 == 0u, "the handoff clears the gate bit alone");
        CHECK(e.pos_pair_2f4 == -1.0f && e.pos_pair_2f8 == -1.0f,
              "the coordinate pair blanks to -1");
        __builtin_memcpy(&now, rec + 0x08, sizeof now);
        /* The faithful transition runs the cell init pass after raising the
         * work bit; that pass re-derives the walkable layer bit (0x10) from
         * the terrain-less cell, so only the work bit survives. */
        CHECK(now == 0x20u, "the queued transition raises the work bit");
        ai_idle_table_reset();
    }

    /* Idle-table pointer removal (2026-10-25): the match-zeroing walk
     * with the count word riding (holes) and the foreign-pointer walk. The
     * faithful table append has no dedupe, so a re-raised flag on a kept
     * entry registers it again. The registrations need relief-free entities
     * on idle-type tiles of the fixture level. */
    {
        ai_idle_table_reset();
        entity_pool_callback_context_set(&ctx);
        EntitySlot* a = place(&ctx, 1, "rem_a", 0.0f, 0.0f);
        EntitySlot* b = place(&ctx, 1, "rem_b", 4.0f, 0.0f);
        a->stats_record = NULL;
        b->stats_record = NULL;
        uint8_t* const cell_a = (uint8_t*)fix_flags(0.0f, 0.0f) - 8;
        uint8_t* const cell_b = (uint8_t*)fix_flags(4.0f, 0.0f) - 8;
        cell_a[2] = (uint8_t)TILE_TYPE_IDLE;
        cell_b[2] = (uint8_t)TILE_TYPE_IDLE;
        a->flags_3ec |= 0x40u;
        b->flags_3ec |= 0x40u;
        run(&ctx, 1, 0.5f);
        CHECK(ai_idle_count() == 2, "both idlers register");
        ai_idle_table_pointer_remove(a);
        CHECK(ai_idle_count() == 2, "the remove leaves the count word");
        a->flags_3ec |= 0x40u;
        run(&ctx, 1, 0.5f);
        CHECK(ai_idle_count() == 3, "the removed entry re-lands past the hole");
        b->flags_3ec |= 0x40u;
        run(&ctx, 1, 0.5f);
        CHECK(ai_idle_count() == 4, "the kept entry appends again (no dedupe)");
        ai_idle_table_pointer_remove(NULL);
        CHECK(ai_idle_count() == 4, "the foreign pointer walks past");
        ai_idle_table_reset();
        cell_a[2] = 0;
        cell_b[2] = 0;
    }

    /* Activity toggle (2026-10-25): the off/on fold on the +0x3ec word
     * with the activity request riding through the set-fold. */
    {
        EntitySlot e;
        memset(&e, 0, sizeof e);
        e.flags_3ec = 0x2000u;
        e.activity_param = 7;
        entity_activity_toggle(&e, 0);
        CHECK(e.flags_3ec == 0x4000u, "the off edge drops bit 13 and raises bit 14");
        CHECK(e.activity == 0x36 && e.activity_last == 0x36,
              "the off edge requests activity 0x36");
        CHECK(e.activity_param == 0, "the request resets the param");
        e.activity = 1;
        entity_activity_toggle(&e, 0);
        CHECK(e.flags_3ec == 0x4000u && e.activity == 1,
              "the second off call rides (bit 13 already clear)");
        memset(&e, 0, sizeof e);
        e.flags_3ec = 0x1000u;
        entity_activity_toggle(&e, 1);
        CHECK(e.flags_3ec == 0x1000u && e.activity == 0x35
                  && e.activity_last == 0x35,
              "the on edge with bit 13 raised rides");
        memset(&e, 0, sizeof e);
        entity_activity_toggle(&e, 1);
        CHECK(e.flags_3ec == 0x1000u && e.activity == 0x35,
              "the on edge raises bit 12 and requests 0x35");
        memset(&e, 0, sizeof e);
        e.flags_3ec = 0x2000u;
        entity_activity_toggle(&e, 1);
        CHECK(e.flags_3ec == 0x2000u && e.activity == 0,
              "the on edge with bit 13 raised rides");
        entity_activity_toggle(NULL, 1); /* the guarded entity (dev) */
        memset(&e, 0, sizeof e);
    }

    /* EntityStatePairPredicate (FUN_0043c200) — 2026-10-25. */
    {
        GameContext pool;
        game_context_init(&pool);
        static char t12[0x150], t13[0x150];
        memset(t12, 0, sizeof t12);
        memset(t13, 0, sizeof t13);
        stats_type_table_install(12, 3, t12, 1);
        stats_type_table_install(13, 0, t13, 1);
        const float minus5 = -5.0f, minus1 = -1.0f, two = 2.0f, one = 1.0f;

        EntitySlot* add = place(&pool, 1, "pairA", 0, 0);
        static char erec[0xe0];
        memset(erec, 0, sizeof erec);
        __builtin_memcpy(erec + 0xdc, &two, sizeof two);
        add->stats_record = erec;
        add->flags_3e8 = 0u;

        /* (12,3) is zero: the answer is 1 without the walk (the pool fold
         * and the second read would otherwise drive the sum to -3). */
        float zero = 0.0f;
        __builtin_memcpy(t12 + 0xdc, &zero, sizeof zero);
        __builtin_memcpy(t13 + 0xdc, &minus5, sizeof minus5);
        CHECK(entity_state_pair_predicate(&pool, 12, 3, 13, 0) == 1,
              "the zero first read short-circuits to 1");

        /* nonzero first read: 1 + (-1) + fold(2) -> positive -> 1. */
        __builtin_memcpy(t12 + 0xdc, &one, sizeof one);
        __builtin_memcpy(t13 + 0xdc, &minus1, sizeof minus1);
        CHECK(entity_state_pair_predicate(&pool, 12, 3, 13, 0) == 1,
              "the fold lifts the sum over zero");

        /* raised bit-0x80000 gate: the walk skips, 1 + (-1) + 0 -> 0. */
        add->flags_3e8 |= 0x80000u;
        CHECK(entity_state_pair_predicate(&pool, 12, 3, 13, 0) == 0,
              "the gated entity drops out of the fold");
        add->flags_3e8 = 0u;

        /* the <= 0 comparison rides zero itself */
        __builtin_memcpy(t13 + 0xdc, &zero, sizeof zero);
        CHECK(entity_state_pair_predicate(&pool, 12, 3, 13, 0) == 1,
              "1 + 0 + fold 2 is positive");

        /* guarded reads answer zero (the port window, not the corpus stride) */
        CHECK(entity_state_pair_predicate(&pool, 12, 99, 13, 0) == 1,
              "the out-of-window first read short-circuits");
        /* NULL ctx skips the walk; the re-reads still answer */
        __builtin_memcpy(t13 + 0xdc, &minus1, sizeof minus1);
        CHECK(entity_state_pair_predicate(NULL, 12, 3, 13, 0) == 0,
              "NULL ctx folds nothing (dev)");

        stats_type_table_install(12, 3, NULL, 0); /* release both cells */
        stats_type_table_install(13, 0, NULL, 0);
    }

    /* ActiveEntityListContainsCode (FUN_004031e0) — 2026-10-25. */
    {
        GameContext sctx;
        game_context_init(&sctx);
        EntitySlot* s1 = place(&sctx, 1, "act1", 0, 0);
        EntitySlot* s2 = place(&sctx, 1, "act2", 1, 0);
        s1->act_list[0] = 7;
        s1->act_count = 1;
        s2->act_list[0] = 8;
        s2->act_count = 1;

        active_entity_list_install(NULL, 0);
        CHECK(active_entity_list_count() == 0 &&
                  active_entity_list_contains_code(&sctx, 7) == 0,
              "the cold list answers 0");

        const EntitySlot* entries[2] = { NULL, s2 };
        active_entity_list_install(entries, 2);
        CHECK(active_entity_list_count() == 2, "the install counts");
        CHECK(active_entity_list_contains_code(&sctx, 7) == 0,
              "the list holds no code 7");
        CHECK(active_entity_list_contains_code(&sctx, 8) == 1,
              "the NULL entry walks past into the hit");

        /* the selection branch isolates the list (pinned) */
        CHECK(selection_add(&sctx, s2) == 1, "select s2");
        const EntitySlot* list1[1] = { s1 };
        active_entity_list_install(list1, 1);
        CHECK(active_entity_list_contains_code(&sctx, 7) == 0,
              "the selection branch ignores the active list");
        CHECK(active_entity_list_contains_code(&sctx, 8) == 1,
              "the selection branch hits the selected entity");

        active_entity_list_install(NULL, -3); /* guarded reset */
        CHECK(active_entity_list_count() == 0, "the negative count clears");
    }

    if (s_failures != 0) {
        fprintf(stderr, "ai_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("ai_test: OK\n");
    return 0;
}
