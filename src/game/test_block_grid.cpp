// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_block_grid — lrr_block_index (block_grid.h), which resolves a block
 * grid index as the original's 32-bit code did, and the units that index
 * the level's grids with it.
 *
 * A NaN world position converts to INT_MIN block coordinates. On Win32,
 * `width * INT_MIN + INT_MIN` times the block size wraps to a byte offset
 * of 0, so the original reads block 0; on 64-bit the same index reads
 * gigabytes past the grid. The crash this guards was a power crystal with
 * a NaN position in legoobject_energy_low_target_route_command_tick. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/block_grid.h"
#include "game/lego_level_record.h"
#include "game/legoobject_record.h"
#include "game/roof.h"
#include "game/spiderweb.h"
#include "game/terrain_cell_objects.h"
#include "game/terrain_map.h"
/* Last: its macros name fields the other headers declare. */
#include "game/lego_globs.h"

extern "C" {
uint32_t terrain_cell_occupied_or_flag_current(int bx, int by, int checkFlags2);
uint32_t terrain_cell_flag_800_bounded_current(uint32_t bx, uint32_t by);
uint32_t terrain_cell_record_byte3_current(int x, int y);
void legoobject_energy_low_target_route_command_tick(Lego_Object* obj, float elapsed);
}

/* The test's stand-ins (--wrap, CMakeLists.txt): a container at a NaN
 * world position, and the scene calls the grids' users make. */
static float s_position[3];
static int s_hidden_group = -1;
static int s_coloured_group = -1;

extern "C" void __wrap_ddiscene_object_position_get(void*, void*, float* pos)
{
    pos[0] = s_position[0];
    pos[1] = s_position[1];
    pos[2] = s_position[2];
}
extern "C" unsigned int __wrap_entity_state_flags_get(const Lego_Object*) { return 0; }
extern "C" void __wrap_container_mesh_hide_group(void*, unsigned long group, int)
{
    s_hidden_group = (int)group;
}
extern "C" void __wrap_scene_mesh_color_and_alpha_set(void*, unsigned long group, float, float,
                                                      float, float)
{
    s_coloured_group = (int)group;
}

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* The level's block on Win32: 0x58 bytes (the units' Lego_Block). */
static const uint32_t WIN32_BLOCK_SIZE = 0x58;

/* The index as the units compute it: `width * y + x` with an unsigned
 * width, so in 32-bit unsigned arithmetic. */
static uint32_t unit_index(uint32_t width, int x, int y)
{
    return width * (uint32_t)y + (uint32_t)x;
}

/* The block the original reads for an index: its 32-bit byte offset from
 * the grid, wrapped. -1 when that offset lands outside the grid, where the
 * original reads other memory. */
static long original_block(uint32_t index, uint32_t count)
{
    uint32_t offset = index * WIN32_BLOCK_SIZE;
    if (offset % WIN32_BLOCK_SIZE != 0 || offset / WIN32_BLOCK_SIZE >= count) return -1;
    return (long)(offset / WIN32_BLOCK_SIZE);
}

static void test_index(void)
{
    /* Inside the grid: the index itself. */
    CHECK(lrr_block_index(0, 4, 3) == 0, "index: the first block");
    CHECK(lrr_block_index(11, 4, 3) == 11, "index: the last block");
    CHECK(lrr_block_index(unit_index(4, 2, 1), 4, 3) == 6, "index: (2, 1) is 6");
    /* x == width runs into the next row, as the original's arithmetic. */
    CHECK(lrr_block_index(unit_index(4, 4, 0), 4, 3) == 4, "index: (4, 0) is the next row's first");

    /* A NaN position: INT_MIN coordinates give block 0, for an even and an
     * odd width (width * INT_MIN is 0 or INT_MIN). */
    CHECK(lrr_block_index(unit_index(4, INT32_MIN, INT32_MIN), 4, 3) == 0, "index: NaN, even width");
    CHECK(lrr_block_index(unit_index(5, INT32_MIN, INT32_MIN), 5, 3) == 0, "index: NaN, odd width");
    /* A NaN x with a real y: the original's offset loses the 2^31, so it
     * reads (0, y). */
    CHECK(lrr_block_index(unit_index(4, INT32_MIN, 1), 4, 3) == 4, "index: NaN x only reads (0, y)");
    /* A NaN y with a real x: width * INT_MIN wraps to 0 for an even
     * width, so the original reads (x, 0). */
    CHECK(lrr_block_index(unit_index(4, 1, INT32_MIN), 4, 3) == 1, "index: NaN y only reads (x, 0)");
    /* A neighbour of a NaN position: INT_MIN + 1 reads block 1, as the
     * original's wrapped byte offset does. */
    CHECK(lrr_block_index(unit_index(4, INT32_MIN + 1, INT32_MIN), 4, 3) == 1,
          "index: a NaN position's east neighbour is block 1");

    /* Past either end of the array: block 0, not a read outside it. */
    CHECK(lrr_block_index(unit_index(4, -1, 0), 4, 3) == 0, "index: before the first block");
    CHECK(lrr_block_index(unit_index(4, 0, 3), 4, 3) == 0, "index: past the last block");
    CHECK(lrr_block_index(UINT32_MAX, 4, 3) == 0, "index: UINT32_MAX");
    CHECK(lrr_block_index(5, 0, 0) == 0, "index: an empty grid");
}

/* Wherever the original's read stays inside the grid, the helper picks the
 * same block; everywhere else it picks one inside the grid. */
static void test_matches_original(void)
{
    static const int coords[] = { INT32_MIN, INT32_MIN + 1, -1000, -3, -2, -1, 0, 1, 2,
                                  3, 4, 5, 6, 7, 8, 9, 10, 1000, INT32_MAX };
    const size_t n = sizeof(coords) / sizeof(coords[0]);
    uint32_t width, height;
    size_t i, j;
    int mismatches = 0, outside = 0;

    for (width = 1; width <= 9; width++) {
        for (height = 1; height <= 9; height++) {
            for (i = 0; i < n; i++) {
                for (j = 0; j < n; j++) {
                    uint32_t index = unit_index(width, coords[i], coords[j]);
                    uint32_t got = lrr_block_index(index, width, height);
                    long want = original_block(index, width * height);
                    if (got >= width * height) outside++;
                    if (want >= 0 && (long)got != want) mismatches++;
                }
            }
        }
    }
    CHECK(mismatches == 0, "original: the same block wherever the original's read is in the grid");
    CHECK(outside == 0, "original: never a block outside the grid");
}

/* The fixture: a 4x3 level (Lego_Level, Map3D) with one block of padding
 * on each side of its grid, which spiderweb_get_angle's neighbour reads
 * (block[-1], block[1]) reach from block 0. */
enum { W = 4, H = 3 };
static Lego_Block s_storage[W * H + 2];
static TerrainMapBlock s_map_blocks[W * H];
static Lego_Level s_level;
static TerrainMap s_map;

static void fixture_reset(void)
{
    memset(s_storage, 0, sizeof(s_storage));
    memset(&s_level, 0, sizeof(s_level));
    memset(&s_map, 0, sizeof(s_map));
    memset(s_map_blocks, 0, sizeof(s_map_blocks));
    s_map.blocks = s_map_blocks;
    s_map.gridWidth = W - 1;
    s_map.gridHeight = H - 1;
    s_map.blockWidth = W;
    s_map.blockHeight = H;
    s_map.blockSize = 40.0f;
    s_level.width = W;
    s_level.height = H;
    s_level.map = &s_map;
    s_level.blocks = &s_storage[1];
    g_active_level_retail = &s_level;
}

/* The source of the coordinates: Map3D's world to block conversion. */
static void test_nan_position(void)
{
    int bx = 0, by = 0;

    fixture_reset();
    terrain_map_world_to_block(&s_map, NAN, NAN, &bx, &by);
    CHECK(bx == INT32_MIN && by == INT32_MIN, "nan: a NaN position gives INT_MIN block coordinates");
    terrain_map_world_to_block(&s_map, 60.0f, -50.0f, &bx, &by);
    CHECK(bx == 1 && by == 1, "nan: a real position gives its block");
}

/* Lego_Level.blocks through lego_unit.c (00432aa0, 00432b00). */
static void test_level_blocks(void)
{
    fixture_reset();
    s_level.blocks[0].flags1 = 0x400;
    CHECK(terrain_cell_occupied_or_flag_current(INT32_MIN, INT32_MIN, 0) == 0x400,
          "level: a NaN position reads block 0");
    CHECK(terrain_cell_occupied_or_flag_current(INT32_MIN, INT32_MIN, 1) == 1,
          "level: a NaN position reads block 0 (flags2 path)");
    s_level.blocks[0].flags1 = 0;
    s_level.blocks[W + 1].flags1 = 0x400;
    CHECK(terrain_cell_occupied_or_flag_current(INT32_MIN, INT32_MIN, 0) == 0,
          "level: block 0's flags, not another block's");
    CHECK(terrain_cell_occupied_or_flag_current(1, 1, 0) == 0x400, "level: (1, 1) reads its block");
    CHECK(terrain_cell_occupied_or_flag_current(W, 0, 0) == 0,
          "level: (width, 0) reads the next row's first block, as the original");
    s_level.blocks[W].flags1 = 0x400;
    CHECK(terrain_cell_occupied_or_flag_current(W, 0, 0) == 0x400,
          "level: (width, 0) is block (0, 1)");
    s_level.blocks[W + 1].flags1 = 0x800;
    CHECK(terrain_cell_flag_800_bounded_current(1, 1) == 0x800, "level: the 0x800 read");
    CHECK(s_storage[0].flags1 == 0 && s_storage[W * H + 1].flags1 == 0, "level: the padding is untouched");
}

/* The electric fence grid, a level-sized array of its own (0040d240). */
static void test_fence_grid(void)
{
    fixture_reset();
    electric_fence_initialise(&s_level);
    CHECK(electric_fence_can_place_at_block(INT32_MIN, INT32_MIN) == 0,
          "fence: a NaN position, block 0 not a ground block: no fence");
    s_level.blocks[0].flags1 = 0x8;
    CHECK(electric_fence_can_place_at_block(INT32_MIN, INT32_MIN) == 0,
          "fence: a NaN position, no building or fence beside it: no fence");
    s_level.blocks[1].flags1 = 0x8 | 0x400;
    CHECK(electric_fence_can_place_at_block(0, 0) == 1,
          "fence: block 0 beside a building can take a fence");
    electric_fence_shutdown();
}

/* The spider web grid and its level block (004663f0). */
static void test_spiderweb(void)
{
    float theta = 0.0f;

    fixture_reset();
    spiderweb_initialise(&s_level);
    s_level.blocks[0].flags1 = 0x8;
    s_level.blocks[1].flags1 = 0x10;
    s_level.blocks[-1].flags1 = 0x10;
    CHECK(spiderweb_get_angle(INT32_MIN, INT32_MIN, &theta) == 1 && theta > 1.5f,
          "spiderweb: a NaN position takes block 0's walls");
    spiderweb_shutdown();
}

/* LEGO_3A_BLOCKS, lego_unit.c's other name for the level's blocks
 * (004332c0): a NaN position and its east neighbour. */
static void test_level_alias(void)
{
    fixture_reset();
    s_level.blocks[0].f_03 = 0x5a;
    s_level.blocks[1].f_03 = 0xa5;
    CHECK(terrain_cell_record_byte3_current(INT32_MIN, INT32_MIN) == 0x5a,
          "alias: a NaN position reads block 0");
    CHECK(terrain_cell_record_byte3_current(INT32_MIN + 1, INT32_MIN) == 0xa5,
          "alias: its east neighbour reads block 1, as the original");
    CHECK(terrain_cell_record_byte3_current(1, 0) == 0xa5, "alias: (1, 0) reads block 1");
}

/* Map3D's own grid (0044f4e0): the highlight read and write. */
static void test_map_blocks(void)
{
    fixture_reset();
    s_map_blocks[0].highlight = 3;
    CHECK(terrain_map_block_highlight_set(&s_map, INT32_MIN, INT32_MIN, 1) == 3,
          "map: a NaN position reads block 0's highlight");
    CHECK(s_map_blocks[0].highlight == 1, "map: and writes block 0's");
    s_map_blocks[W + 2].highlight = 2;
    CHECK(terrain_map_block_highlight_set(&s_map, 2, 1, 0) == 2, "map: (2, 1) is its block");
    CHECK(s_map_blocks[W + 2].highlight == 0, "map: and turning it off clears it");
}

/* The roof's grid (004634f0), indexed with unsigned coordinates. */
static void test_roof_grid(void)
{
    RoofBlock grid[W * H];
    unsigned int i;

    memset(&roofGlobs, 0, sizeof(roofGlobs));
    for (i = 0; i < W * H; i++) grid[i].groupID = 100 + i;
    roofGlobs.grid = grid;
    roofGlobs.width = W;
    roofGlobs.height = H;
    roof_hide_block((unsigned int)INT32_MIN, (unsigned int)INT32_MIN);
    CHECK(roofGlobs.hiddenCount == 1 && roofGlobs.hiddenList[0] == &grid[0] && s_hidden_group == 100,
          "roof: a NaN position hides block 0");
    roof_hide_block(3, 2);
    CHECK(roofGlobs.hiddenCount == 2 && roofGlobs.hiddenList[1] == &grid[11] && s_hidden_group == 111,
          "roof: (3, 2) hides its block");
    memset(&roofGlobs, 0, sizeof(roofGlobs));
}

/* The crash: a power crystal (type 6) whose container is at a NaN world
 * position, in its per-frame update (0043f7d0). Its block resolves to
 * block 0, so on lava there it melts, as in the original; elsewhere it is
 * left alone. */
static void test_nan_crystal(void)
{
    Lego_Object* crystal = (Lego_Object*)calloc(1, sizeof(Lego_Object));

    fixture_reset();
    s_position[0] = s_position[1] = s_position[2] = NAN;
    crystal->type = 6;
    crystal->health = 100.0f;
    crystal->energy = 100.0f;
    crystal->flags3 = 0x80000000u; /* no drain: the stats tables are not loaded */

    s_level.blocks[0].f_02 = 1;
    legoobject_energy_low_target_route_command_tick(crystal, 1.0f);
    CHECK(crystal->health == 100.0f, "crystal: a NaN position off lava survives the update");

    s_level.blocks[0].f_02 = 6;
    legoobject_energy_low_target_route_command_tick(crystal, 1.0f);
    CHECK(crystal->health == -1.0f && (crystal->flags3 & 0x800000) != 0,
          "crystal: a NaN position with lava at block 0 melts, as the original");
    CHECK(s_level.f_09c == 1, "crystal: and counts as lost");

    s_position[0] = 60.0f;
    s_position[1] = -50.0f;
    s_position[2] = 0.0f;
    crystal->health = 100.0f;
    crystal->flags3 = 0x80000000u;
    s_level.blocks[0].f_02 = 1;
    s_level.blocks[W + 1].f_02 = 6;
    legoobject_energy_low_target_route_command_tick(crystal, 1.0f);
    CHECK(crystal->health == -1.0f, "crystal: a real position reads its own block, (1, 1)");
    free(crystal);
}

int main(void)
{
    test_index();
    test_matches_original();
    test_nan_position();
    test_level_blocks();
    test_fence_grid();
    test_spiderweb();
    test_level_alias();
    test_map_blocks();
    test_roof_grid();
    test_nan_crystal();

    if (s_failures != 0) {
        printf("block_grid_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("block_grid_test: OK\n");
    return 0;
}
