// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_level_boot — a game session on a synthetic level: Lego.c's level
 * loader (Lego_LoadLevel, 0x00429890) reads an 8x8 level, and its main loop
 * (Lego_MainLoop, 0x004232e0) runs 250 frames on a headless software
 * device, simulation and render.
 *
 * Lego_Initialise cannot run here: it gives up unless every object type
 * loads a real model. So boot_game() performs the parts of it that the
 * loader and the main loop rely on, in its order, with a config, maps,
 * images and one triangle model that the test writes. The level has two
 * caves split by a two-block dirt wall, an ore and a power crystal.
 *
 * Establishes: the maps load into blocks (rock, floor, wall); the object
 * list creates its objects at their blocks; destroying half the wall
 * collapses the unsupported other half and joins the caves; the main loop
 * runs 250 frames and keeps the level, the objects and the open blocks. */
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>

#include "gods98/container_link_names.h"
#include "gods98/config.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/files.h"
#include "gods98/main_globs.h"
#include "gods98/viewports_link_names.h"
#include "gods98/viewports.h"
#include "game/camera_control.h"
#include "game/lego_level_record.h"
#include "game/legoobject_record.h"
#include "game/legoobject_unit.h"
#include "game/terrain_map.h"
#include "game/lego_unit.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
/* Last: its macros name fields the other headers declare. */
#include "game/lego_globs.h"

#include "test/synthetic_level_fixture.h"
using namespace synthetic_fixture;

/* Names each step on stderr, unbuffered, so a crash's log shows where. */
static void step(const char* name)
{
    std::fprintf(stderr, "level_boot: %s\n", name);
    std::fflush(stderr);
}

static Lego_Block* block_at(Lego_Level* level, unsigned x, unsigned y)
{
    return &level->blocks[y * level->width + x];
}

/* The objects in the level, by LegoObject_Type, and where the ore is. */
struct Census {
    int ore = 0, crystals = 0;
    float oreX = 0.0f, oreY = 0.0f;
};

static int count_object(Lego_Object* obj, void* data)
{
    Census* census = static_cast<Census*>(data);
    if (obj->type == 7) {
        ++census->ore;
        legoobject_transform_position_get(obj, &census->oreX, &census->oreY);
    } else if (obj->type == 6) {
        ++census->crystals;
    }
    return 0;
}

static Census census()
{
    Census census;
    legoobject_run_through_lists(count_object, &census, 0);
    return census;
}

static void check_level()
{
    if (!boot_game()) return;
    step("load the level");
    int loaded = lego_level_load("Level01");
    CHECK(loaded, "load the level");
    if (!loaded) return;
    Lego_Level* level = (Lego_Level*)legoGlobs.currLevel;
    if (!CHECK(level != nullptr, "level is current")) return;
    CHECK_EQ(level->width, W);
    CHECK_EQ(level->height, H);

    /* The map as loaded: rock round the edge, two caves either side of a
     * two-block dirt wall at x = 3 and 4. Flag 0x8 marks a floor. */
    CHECK(!(block_at(level, 0, 3)->flags1 & 0x8), "the edge is rock");
    CHECK(block_at(level, 2, 3)->flags1 & 0x8, "the left cave is floor");
    CHECK(block_at(level, 5, 3)->flags1 & 0x8, "the right cave is floor");
    CHECK(!(block_at(level, 3, 3)->flags1 & 0x8), "the wall's left half stands");
    CHECK(!(block_at(level, 4, 3)->flags1 & 0x8), "the wall's right half stands");
    CHECK_EQ(block_at(level, 4, 3)->f_02, 4);

    /* The object list's ore and crystal, at their blocks' centres. */
    Census before = census();
    CHECK_EQ(before.ore, 1);
    CHECK_EQ(before.crystals, 1);
    int bx = -1, by = -1;
    CHECK(terrain_map_world_to_block(level->map, before.oreX, before.oreY, &bx, &by), "the ore is on the map");
    CHECK_EQ(bx, 2);
    CHECK_EQ(by, 2);

    step("destroy the wall");
    /* Destroying one half of the wall leaves the other half one block
     * thick, which collapses too: the caves join. */
    CHECK_EQ(lego_terrain_cell_mutate(level, 4, 3, 0), 1);
    CHECK(block_at(level, 4, 3)->flags1 & 0x8, "the destroyed block is floor");
    CHECK(block_at(level, 3, 3)->flags1 & 0x8, "the unsupported half collapses");
    CHECK_EQ(lego_terrain_cell_mutate(level, 0, 3, 0), 0);

    step("main loop");
    /* The main loop: simulation, effects, interface and a software render
     * of each frame. */
    for (int frame = 0; frame < 250; ++frame) lego_engine_tick(1.0f);
    CHECK(legoGlobs.currLevel == level, "the level is still current after 250 frames");
    Census after = census();
    CHECK_EQ(after.ore, 1);
    CHECK_EQ(after.crystals, 1);
    CHECK_NEAR(after.oreX, before.oreX, 0.5f);
    CHECK_NEAR(after.oreY, before.oreY, 0.5f);
    CHECK(block_at(level, 3, 3)->flags1 & 0x8, "the joined caves stay open");
}

int main()
{
    if (CHECK(make_fixture("level_boot_fixture"), "write the fixture") &&
        initialise_headless("LevelBootTest")) check_level();
    return lrr_test_result();
}
