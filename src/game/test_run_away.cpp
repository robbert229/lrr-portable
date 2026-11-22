// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_run_away — LegoObject.c's run away (0x00444460,
 * legoobject_flagged_vector_step_command_issue) routes an object to the
 * point legoGlobs.MiniFigureRunAway along a direction.
 *
 * When that point lies in the object's own block, the original builds a
 * one-block route from the stack addresses of its block coordinates and
 * then passes them to free. The MSVC CRT let that pass; glibc aborts with
 * "free(): invalid pointer". The port frees only the lists the route
 * search allocated.
 *
 * The test links with --wrap for the container position, the object's
 * stats, the idle bubble and free, so the unit reaches the stubs below:
 * free records the stack addresses it is given instead of aborting. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/lego_level_record.h"
#include "game/legoobject_record.h"
#include "game/legoobject_unit.h"
#include "game/terrain_map.h"
/* Last: its macros name fields the other headers declare. */
#include "game/lego_globs.h"

extern "C" void legoobject_flagged_vector_step_command_issue(Lego_Object* obj, float* dir);

/* The stack's extent: main's frame is the top, the stub's own the
 * bottom. */
static uintptr_t s_stack_top;
static int s_stack_frees;
static int s_heap_frees;
static int s_idle_registered;

extern "C" void __real_free(void* p);
extern "C" void __wrap_free(void* p)
{
    volatile char here = 0;
    uintptr_t addr = (uintptr_t)p;

    if (p == NULL) return;
    if (addr >= (uintptr_t)&here && addr < s_stack_top) {
        ++s_stack_frees;
        return;
    }
    ++s_heap_frees;
    __real_free(p);
}

/* The object's container stands at s_position. */
static float s_position[3];

extern "C" void __wrap_ddiscene_object_position_get(void*, void*, float* pos)
{
    pos[0] = s_position[0];
    pos[1] = s_position[1];
    pos[2] = s_position[2];
}
extern "C" void __wrap_idle_register(Lego_Object*) { ++s_idle_registered; }

/* A minifigure's stats for the route search: it crosses ordinary ground
 * (0x2000000) and nothing else. */
extern "C" unsigned int __wrap_entity_stats_word_get(const Lego_Object*) { return 0x2000000; }
extern "C" unsigned int __wrap_entity_capability_flags_get(const Lego_Object*) { return 0; }
extern "C" unsigned int __wrap_entity_state_flags_get(const Lego_Object*) { return 0; }

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* The fixture: an open 8x8 level of 40-unit floor blocks (flags1 8).
 * Block (bx, by) spans x in [40 bx, 40 bx + 40) and y in
 * (-40 by - 40, -40 by]. */
enum { W = 8, H = 8 };
static const float BLOCK = 40.0f;
static Lego_Block s_blocks[W * H];
static TerrainMapBlock s_map_blocks[W * H];
static Lego_Level s_level;
static TerrainMap s_map;
static Lego_Object s_obj;

static void fixture_reset(float runAway)
{
    memset(s_blocks, 0, sizeof(s_blocks));
    memset(s_map_blocks, 0, sizeof(s_map_blocks));
    memset(&s_level, 0, sizeof(s_level));
    memset(&s_map, 0, sizeof(s_map));
    memset(&s_obj, 0, sizeof(s_obj));
    for (int i = 0; i < W * H; i++) s_blocks[i].flags1 = 8;
    s_map.blocks = s_map_blocks;
    s_map.gridWidth = W - 1;
    s_map.gridHeight = H - 1;
    s_map.blockWidth = W;
    s_map.blockHeight = H;
    s_map.blockSize = BLOCK;
    s_level.width = W;
    s_level.height = H;
    s_level.map = &s_map;
    s_level.blocks = s_blocks;
    g_active_level_retail = &s_level;
    legoGlobs_MiniFigureRunAway = runAway;
    s_stack_frees = 0;
    s_heap_frees = 0;
    s_idle_registered = 0;
}

/* Stand the object in the middle of block (bx, by). */
static void place(int bx, int by)
{
    s_position[0] = BLOCK * (float)bx + BLOCK / 2.0f;
    s_position[1] = -(BLOCK * (float)by + BLOCK / 2.0f);
    s_position[2] = 0.0f;
}

static void route_release(void)
{
    __real_free(s_obj.routeBlocks);
    s_obj.routeBlocks = NULL;
}

/* The run-away point stays in the object's block: a one-block route,
 * and nothing on the stack reaches free. */
static void test_one_block_route(void)
{
    float dir[2] = { 1.0f, 0.0f };
    const LegoObjG_RouteBlock* route;

    fixture_reset(BLOCK / 4.0f);
    place(3, 3);
    legoobject_flagged_vector_step_command_issue(&s_obj, dir);

    CHECK(s_stack_frees == 0, "one block: no stack address is freed");
    CHECK((s_obj.flags1 & 0x800000) != 0, "one block: the object is marked as running away");
    CHECK(s_idle_registered == 1, "one block: the object is registered idle");
    CHECK(s_obj.routeBlocksTotal == 1, "one block: a route of one block");
    route = (const LegoObjG_RouteBlock*)s_obj.routeBlocks;
    if (route != NULL && s_obj.routeBlocksTotal == 1) {
        CHECK(route[0].blockPos[0] == 3 && route[0].blockPos[1] == 3,
              "one block: the route is the object's own block");
        CHECK((route[0].flags & 0x40) != 0, "one block: its last block is flagged 0x40");
    }
    route_release();
}

/* The run-away point lies in another block: the route search allocates
 * the lists, and both go back to the heap. */
static void test_searched_route(void)
{
    float dir[2] = { 1.0f, 0.0f };
    const LegoObjG_RouteBlock* route;

    fixture_reset(BLOCK);
    place(3, 3);
    legoobject_flagged_vector_step_command_issue(&s_obj, dir);

    CHECK(s_stack_frees == 0, "searched: no stack address is freed");
    CHECK(s_obj.routeBlocksTotal == 2, "searched: a route of two blocks");
    CHECK(s_heap_frees >= 2, "searched: the route search's lists are freed");
    route = (const LegoObjG_RouteBlock*)s_obj.routeBlocks;
    if (route != NULL && s_obj.routeBlocksTotal == 2) {
        CHECK(route[0].blockPos[0] == 3 && route[0].blockPos[1] == 3,
              "searched: the route starts in the object's block");
        CHECK(route[1].blockPos[0] == 4 && route[1].blockPos[1] == 3,
              "searched: the route ends in the run-away block");
        CHECK((route[1].flags & 0x40) != 0, "searched: its last block is flagged 0x40");
    }
    route_release();
}

int main(void)
{
    volatile char top = 0;

    s_stack_top = (uintptr_t)&top;
    test_one_block_route();
    test_searched_route();

    if (s_failures == 0) printf("run_away: ok\n");
    return s_failures != 0;
}
