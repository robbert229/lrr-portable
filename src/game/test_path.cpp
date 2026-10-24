// SPDX-License-Identifier: LGPL-3.0-or-later
// Terrain grid route search (finding 447) — path_grid_search,
// path_neighbor_search, path_route_check, path_coord_probe,
// path_route_adapter, path_route_install.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/game.hpp"
#include "game/level.hpp"
#include "game/path.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static int iabs(int v) { return v < 0 ? -v : v; }

/* Synthetic level fixture: a w*h voxel grid; every cell starts empty
 * (type 0, flags 0), which the NULL-entity route predicate reads as
 * blocked (no flag-0x8 bit) — tests make cells routable with
 * map_open()/cell_set(). */
static LevelState s_map;
static uint8_t* s_vox;

static void map_begin(int w, int h)
{
    s_vox = (uint8_t*)calloc((size_t)w * h, LEVEL_VOXEL_STRIDE);
    memset(&s_map, 0, sizeof(s_map));
    level_set_int(&s_map, LEVEL_OFF_WIDTH, w);
    level_set_int(&s_map, LEVEL_OFF_HEIGHT, h);
    level_store_ptr(&s_map, LEVEL_OFF_VOXELS, s_vox);
}

static void map_free(void)
{
    free(s_vox);
    s_vox = NULL;
    memset(&s_map, 0, sizeof(s_map));
}

/* cell_set + explicit record byte +0 (the corpus reads it for bit-0
 * entities; the .map loaders leave it nonzero, calloc starts it at 0). */
static void cell_set2(int x, int y, uint8_t type, uint8_t byte0,
                      uint32_t flags)
{
    uint8_t* rec =
        s_vox + ((size_t)y * level_width(&s_map) + x) * LEVEL_VOXEL_STRIDE;
    rec[0] = byte0;
    rec[2] = type;
    __builtin_memcpy(rec + 8, &flags, sizeof(flags));
}

static void cell_set(int x, int y, uint8_t type, uint32_t flags)
{
    cell_set2(x, y, type, 0, flags);
}

static void map_open(int w, int h)
{
    map_begin(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            cell_set(x, y, 0, PATH_FLAG_WALLED);
        }
    }
}

/* Every consecutive route step must be a cardinal 4-neighbour step. */
static void check_adjacent(const int* xr, const int* yr, int n)
{
    for (int i = 0; i + 1 < n; ++i) {
        CHECK(iabs(xr[i + 1] - xr[i]) + iabs(yr[i + 1] - yr[i]) == 1,
              "consecutive route cells are 4-adjacent");
    }
}

static void test_coord_probe(void)
{
    map_begin(8, 8);
    cell_set(3, 4, 0, PATH_FLAG_PROBE);
    CHECK(path_coord_probe(&s_map, 3, 4) == 1, "probe flag seen");
    CHECK(path_coord_probe(&s_map, 2, 4) == 0, "probe flag absent");
    CHECK(path_coord_probe(&s_map, -1, 4) == 0, "probe OOB x rejected");
    CHECK(path_coord_probe(&s_map, 3, 8) == 0, "probe OOB y rejected");
    CHECK(path_coord_probe(NULL, 3, 4) == 0, "probe on no grid rejected");
    map_free();
}

static void test_route_check(void)
{
    map_begin(8, 8);

    /* NULL entity: the cell needs the flag-0x8 bit; hard flag rejects. */
    CHECK(path_route_check(&s_map, NULL, 1, 1, 1, 1, 0) == 0,
          "NULL ent: no walled flag blocks");
    cell_set(1, 1, 0, PATH_FLAG_WALLED);
    CHECK(path_route_check(&s_map, NULL, 1, 1, 1, 1, 0) == 1,
          "NULL ent: walled flag passes");
    cell_set(1, 1, 0, PATH_FLAG_WALLED | PATH_FLAG_BLOCKED);
    CHECK(path_route_check(&s_map, NULL, 1, 1, 1, 1, 0) == 0,
          "hard-block flag rejects");
    cell_set(1, 1, 0, PATH_FLAG_WALLED);

    EntitySlot ent;
    memset(&ent, 0, sizeof(ent));
    ent.x = 5.0f;
    ent.y = 5.0f;

    /* Entity stats gates: zero stats block every normal cell; the
     * 0x2000000 walk bit opens them. */
    cell_set(2, 2, 0, PATH_FLAG_WALLED);
    CHECK(path_route_check(&s_map, &ent, 2, 2, 2, 2, 0) == 0,
          "statless ent blocked (needs 0x2000000)");
    ent.stats_word = 0x2000000u;
    CHECK(path_route_check(&s_map, &ent, 2, 2, 2, 2, 0) == 1,
          "walk bit passes normal cell");

    /* The entity's own cell always passes in mode 0, even statless. */
    EntitySlot statless;
    memset(&statless, 0, sizeof(statless));
    statless.x = 2.0f;
    statless.y = 2.0f;
    CHECK(path_route_check(&s_map, &statless, 2, 2, 2, 2, 0) == 1,
          "own cell always passes (mode 0)");

    /* Type 0xb rejects. */
    EntitySlot fence;
    memset(&fence, 0, sizeof(fence));
    fence.type = 0xb;
    fence.stats_word = 0x2000000u;
    CHECK(path_route_check(&s_map, &fence, 2, 2, 2, 2, 0) == 0,
          "type 0xb rejects");

    /* Idle tile '\t' (9) needs stats bit 0x1000000. */
    cell_set(3, 3, PATH_TYPE_IDLE, PATH_FLAG_WALLED);
    CHECK(path_route_check(&s_map, &ent, 3, 3, 3, 3, 0) == 0,
          "idle tile needs bit 0x1000000");
    ent.stats_word = 0x2000000u | 0x1000000u;
    CHECK(path_route_check(&s_map, &ent, 3, 3, 3, 3, 0) == 1,
          "idle tile passes with bit 0x1000000");

    /* stats bit 0 set: traversable flag raises the result to 3; a lava
     * west neighbour blocks a traversable cell (finding 447 edge probe).
     * Bit-0 entities also require a nonzero record byte +0 (cell_set
     * fixes the type byte; poke +0 explicitly). */
    EntitySlot bit0;
    memset(&bit0, 0, sizeof(bit0));
    bit0.stats_word = 0x2000000u | 1u;
    cell_set2(4, 4, 0, 1, PATH_FLAG_WALLED | PATH_FLAG_TRAVERSABLE);
    CHECK(path_route_check(&s_map, &bit0, 4, 4, 4, 4, 0) == 3,
          "traversable bit-0 ent returns 3");
    cell_set2(6, 6, 0, 1, PATH_FLAG_WALLED | PATH_FLAG_TRAVERSABLE);
    cell_set2(5, 6, PATH_TYPE_LAVA, 1, PATH_FLAG_WALLED | PATH_FLAG_TRAVERSABLE);
    CHECK(path_route_check(&s_map, &bit0, 6, 6, 6, 6, 0) == 0,
          "lava west neighbour blocks traversable cell");
    cell_set2(6, 5, 0, 1, PATH_FLAG_WALLED | PATH_FLAG_TRAVERSABLE);
    CHECK(path_route_check(&s_map, &bit0, 6, 6, 6, 6, 0) == 0,
          "north lava neighbour blocks traversable cell");

    /* mode != 0 rejects the blocking flag; mode 0 ignores it. */
    cell_set(0, 0, 0, PATH_FLAG_WALLED | PATH_FLAG_BLOCKING);
    CHECK(path_route_check(&s_map, &ent, 0, 0, 0, 0, 0) == 1,
          "blocking flag ignored in mode 0");
    CHECK(path_route_check(&s_map, &ent, 0, 0, 0, 0, 1) == 0,
          "blocking flag rejects in mode 1");

    /* Diagonal probe: both corner cells need flag 0x8 for the corner-cut
     * result 2. */
    cell_set(1, 1, 0, PATH_FLAG_WALLED);
    CHECK(path_route_check(&s_map, &ent, 1, 1, 2, 2, 0) == 1,
          "diagonal without corner walls returns 1");
    cell_set(2, 1, 0, PATH_FLAG_WALLED);
    cell_set(1, 2, 0, PATH_FLAG_WALLED);
    CHECK(path_route_check(&s_map, &ent, 1, 1, 2, 2, 0) == 2,
          "diagonal with corner walls returns 2");

    map_free();
}

static void test_grid_search(void)
{
    int* xr = NULL;
    int* yr = NULL;
    int n = 0;

    map_open(10, 10);
    CHECK(path_grid_search(&s_map, NULL, 0, 0, 2, 1, &xr, &yr, &n, NULL,
                           NULL) == 1,
          "open-field BFS succeeds");
    CHECK(n == 4, "BFS count == Manhattan + 1");
    CHECK(xr[0] == 0 && yr[0] == 0, "BFS starts at the start cell");
    CHECK(xr[n - 1] == 2 && yr[n - 1] == 1, "BFS ends at the target cell");
    check_adjacent(xr, yr, n);
    free(xr);
    free(yr);
    xr = yr = NULL;

    /* A target the route predicate rejects yields no path. */
    cell_set(2, 1, 0, PATH_FLAG_WALLED | PATH_FLAG_BLOCKED);
    CHECK(path_grid_search(&s_map, NULL, 0, 0, 2, 1, &xr, &yr, &n, NULL,
                           NULL) == 0,
          "blocked target rejected before the search");
    cell_set(2, 1, 0, PATH_FLAG_WALLED);

    /* A blocked cell reroutes the shortest path around it. */
    cell_set(3, 0, 0, PATH_FLAG_WALLED | PATH_FLAG_BLOCKED);
    CHECK(path_grid_search(&s_map, NULL, 0, 0, 5, 0, &xr, &yr, &n, NULL,
                           NULL) == 1,
          "BFS reroutes around a blocked cell");
    check_adjacent(xr, yr, n);
    CHECK(xr[n - 1] == 5 && yr[n - 1] == 0, "reroute ends at the target");
    for (int i = 0; i < n; ++i) {
        CHECK(!(xr[i] == 3 && yr[i] == 0), "reroute never steps on the block");
    }
    free(xr);
    free(yr);
    xr = yr = NULL;

    /* Unreachable target. */
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            cell_set(5 + dx, 5 + dy, 0, PATH_FLAG_WALLED | PATH_FLAG_BLOCKED);
        }
    }
    CHECK(path_grid_search(&s_map, NULL, 0, 0, 5, 5, &xr, &yr, &n, NULL,
                           NULL) == 0,
          "unreachable target returns 0");
    CHECK(xr == NULL && yr == NULL && n == 0, "failure leaves outputs NULL/0");

    /* Off-grid start/target. */
    CHECK(path_grid_search(&s_map, NULL, 99, 0, 5, 5, &xr, &yr, &n, NULL,
                           NULL) == 0,
          "off-grid start returns 0");
    CHECK(path_grid_search(&s_map, NULL, 0, 0, 99, 5, &xr, &yr, &n, NULL,
                           NULL) == 0,
          "off-grid target returns 0 (no callback)");

    map_free();
}

/* Callback fixtures. */
static int path_cb_count(const EntitySlot* ent, const int cell[2], void* data)
{
    (void)ent;
    (void)data;
    ++*(int*)data;
    return 0;
}

static int path_cb_stop(const EntitySlot* ent, const int cell[2], void* data)
{
    (void)ent;
    int* where = (int*)data;
    if (cell[0] == where[0] && cell[1] == where[1]) {
        return 1;
    }
    return 0;
}

static void test_grid_search_cb(void)
{
    int* xr = NULL;
    int* yr = NULL;
    int n = 0;

    /* No callback: the target must itself be routable, so sealing it off
     * from the predicate (but not from the callback path) is rejected. */
    map_open(10, 10);
    int visits = 0;
    CHECK(path_grid_search(&s_map, NULL, 0, 0, 9, 0, &xr, &yr, &n,
                           path_cb_count, &visits) == 0,
          "a never-accepting callback drains the search to 0");
    CHECK(visits > 0, "callback is invoked during the search");
    CHECK(xr == NULL && yr == NULL && n == 0, "drained search leaves no path");

    /* The callback terminates the search at its accepted cell. */
    int stop[2] = { 3, 3 };
    CHECK(path_grid_search(&s_map, NULL, 0, 0, 9, 9, &xr, &yr, &n,
                           path_cb_stop, stop) == 1,
          "callback search terminates at the accepted cell");
    CHECK(xr[n - 1] == 3 && yr[n - 1] == 3, "callback stop cell is the end");
    check_adjacent(xr, yr, n);
    CHECK(xr[0] == 0 && yr[0] == 0, "callback search keeps the start");
    free(xr);
    free(yr);
    map_free();
}

static void test_neighbor_search(void)
{
    int* xr = NULL;
    int* yr = NULL;
    int n = 0;

    map_open(10, 10);
    /* Straight run: clean row path, count = hops + 1. */
    CHECK(path_neighbor_search(&s_map, NULL, 0, 0, 4, 0, &xr, &yr, &n, NULL,
                               NULL) == 1,
          "neighbor search on an open field succeeds");
    CHECK(n == 5, "neighbor count == hops + 1 on a straight run");
    CHECK(xr[0] == 0 && yr[0] == 0, "neighbor path starts at the start");
    CHECK(xr[n - 1] == 4 && yr[n - 1] == 0, "neighbor path ends at the target");
    check_adjacent(xr, yr, n);
    free(xr);
    free(yr);
    xr = yr = NULL;

    /* Half-cost probes on the y=1 row make that branch cheaper, so the
     * reconstruction follows it: the path must pass through (2,1), which
     * the y=0 branch would not. The probe flag sits on the cell *beyond*
     * each westward-to-eastward candidate — cells 2..5 on the row. */
    for (int x = 2; x <= 5; ++x) {
        cell_set(x, 1, 0, PATH_FLAG_WALLED | PATH_FLAG_PROBE);
    }
    CHECK(path_neighbor_search(&s_map, NULL, 0, 0, 4, 1, &xr, &yr, &n, NULL,
                               NULL) == 1,
          "probed branch is reachable");
    check_adjacent(xr, yr, n);
    int on_cheap_row = 0;
    for (int i = 0; i < n; ++i) {
        if (xr[i] == 2 && yr[i] == 1) {
            on_cheap_row = 1;
        }
    }
    CHECK(on_cheap_row == 1, "half-cost probes steer the reconstruction "
                             "onto the cheap row");
    free(xr);
    free(yr);
    xr = yr = NULL;

    /* Unreachable target. */
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            cell_set(5 + dx, 5 + dy, 0, PATH_FLAG_WALLED | PATH_FLAG_BLOCKED);
        }
    }
    CHECK(path_neighbor_search(&s_map, NULL, 0, 0, 5, 5, &xr, &yr, &n, NULL,
                               NULL) == 0,
          "unreachable target returns 0");

    /* Several start/target pairs: the output must always be a 4-adjacent
     * route with the right endpoints — the contract the corpus's
     * TerrainGridPathSearch fallback guarantees when the relaxation
     * reconstruction overshoots. */
    static const int pairs[][4] = {
        { 0, 0, 8, 8 }, { 1, 2, 7, 3 }, { 9, 0, 0, 9 }, { 4, 4, 4, 7 },
    };
    for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); ++i) {
        int sx = pairs[i][0], sy = pairs[i][1], tx = pairs[i][2],
            ty = pairs[i][3];
        CHECK(path_neighbor_search(&s_map, NULL, sx, sy, tx, ty, &xr, &yr,
                                   &n, NULL, NULL) == 1,
              "pair search succeeds");
        CHECK(xr[0] == sx && yr[0] == sy, "pair keeps the start");
        CHECK(xr[n - 1] == tx && yr[n - 1] == ty, "pair keeps the target");
        check_adjacent(xr, yr, n);
        free(xr);
        free(yr);
        xr = yr = NULL;
    }

    map_free();
}

static void test_adapter(void)
{
    map_open(10, 10);
    int* xr = NULL;
    int* yr = NULL;
    int n = 0;
    CHECK(path_route_adapter(&s_map, NULL, 0, 0, 3, 0, &xr, &yr, &n) == 1,
          "adapter forwards to the neighbor search");
    CHECK(xr[0] == 0 && yr[0] == 0 && xr[n - 1] == 3 && yr[n - 1] == 0,
          "adapter route has the right endpoints");
    check_adjacent(xr, yr, n);
    free(xr);
    free(yr);
    map_free();
}

static void test_route_install(void)
{
    GameContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    int xr[4] = { 0, 1, 2, 3 };
    int yr[4] = { 0, 0, 0, 0 };

    EntitySlot e;
    memset(&e, 0, sizeof(e));
    e.flags_3e0 = 0x10000000u; /* a +0x3e0 mask bit blocks the install */
    CHECK(path_route_install(&ctx, &e, xr, yr, 4) == 0,
          "FUN_0043baf0 gate blocks when a mask bit is set");
    e.flags_3e0 = 0;
    CHECK(path_route_install(&ctx, &e, xr, yr, 4) == 1, "install succeeds");
    CHECK(e.wp_count == 4 && e.wp_index == 0, "install sets count/index");
    for (int i = 0; i < 4; ++i) {
        CHECK(e.wp[i].x == (float)xr[i] && e.wp[i].y == (float)yr[i],
              "install copies the route coords");
        CHECK(e.wp[i].pending != 0, "install marks every waypoint pending");
    }
    CHECK((e.flags_3e0 & 1u) != 0, "install raises +0x3e0 bit 1");
    CHECK((e.flags_3e8 & 0x400u) == 0, "install clears +0x3e8 bit 0x400");
    CHECK((e.flags_3ec & 0x40u) == 0, "install clears +0x3ec bit 0x40");

    EntitySlot big;
    memset(&big, 0, sizeof(big));
    CHECK(path_route_install(&ctx, &big, xr, yr, AI_WAYPOINTS_MAX + 1) == 0,
          "count over the port waypoint bound rejected");

    EntitySlot z;
    memset(&z, 0, sizeof(z));
    z.wp_count = 9;
    z.wp_index = 9;
    CHECK(path_route_install(&ctx, &z, NULL, NULL, 0) == 1,
          "count-0 install succeeds");
    CHECK(z.wp_count == 0 && z.wp_index == 0, "count-0 install empties the list");
}

int main(void)
{
    test_coord_probe();
    test_route_check();
    test_grid_search();
    test_grid_search_cb();
    test_neighbor_search();
    test_adapter();
    test_route_install();

    if (s_failures != 0) {
        fprintf(stderr, "path_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("path_test: OK\n");
    return 0;
}
