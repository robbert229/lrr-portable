// SPDX-License-Identifier: LGPL-3.0-or-later
/* Actual optional layer loading on the fixture's abstract grid. The layer
 * values below are invented independently of any shipped map. */
#include <cstdlib>
#include "gods98/directdraw.h"
#include "game/lego_unit.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"
extern "C" int lego_keep_running(void);
extern "C" void nerps_block_pointer_enumerate(int index,
    int (*callback)(NERPs_BlockPointer*, unsigned, unsigned, void*), void* data);

namespace {
using namespace synthetic_fixture;

const char ExtraGrid[] = R"(
 GridC {
   FullName Grid_C nextlevel GridA Seed 43 blocksize 40 roughlevel 0 textureset TS
   surfacemap L\surf.map predugmap L\dug.map terrainmap L\terr.map
   PTLFile L\empty.ptl olistfile L\objects.ol EmergeCreature TVCamera
   CryOreMap L\counts.map:1 PathMap L\paths.map:1 ErodeMap L\erosion.map
   EmergeMap L\triggers.map AIMap L\nodes.map FallinMap L\falls.map
   blockPointersMap L\markers.map:1 FallinMultiplier 2
   NoDrain TRUE DragBoxRGB 32:64:128
   FogColourRGB 16:64:128 HighFogColourRGB 32:96:160 FogRate 8
 }
)";

unsigned counts(unsigned x, unsigned y) {
    return y == 3 && x >= 1 && x <= 4 ? 4 * x - 1 : 1;
}
unsigned paths(unsigned x, unsigned y) {
    if (x == 1 && y == 2) return 3; // power path
    if (x == 2 && y == 2) return 2; // rubble
    if (x == 3 && y == 2) return 3; // wall cannot carry a power path
    return 1;
}
unsigned erosion(unsigned x, unsigned y) {
    if (x == 2 && y == 3) return 3;
    if (x == 5 && y == 3) return 2;
    return 0;
}
unsigned triggers(unsigned x, unsigned y) {
    if (y == 4) {
        if (x == 1) return 1;
        if (x == 2) return 2;
        if (x == 5) return 7;
        if (x == 6) return 9;
    }
    return 0;
}
unsigned nodes(unsigned x, unsigned y) { return x == 2 && y == 5 ? 17 : 0; }
unsigned falls(unsigned x, unsigned y) {
    if (x == 5 && y == 5) return 2;
    if (x == 6 && y == 5) return 7;
    return 0;
}
unsigned markers(unsigned x, unsigned y) {
    return (x == 2 || x == 5) && y == 4 ? 5 : 1;
}
int record_marker(NERPs_BlockPointer*, unsigned x, unsigned y, void* data) {
    CHECK_EQ(y, 4u);
    *static_cast<unsigned*>(data) |= 1u << x;
    return 1;
}

Lego_Block& cell(unsigned x, unsigned y)
{
    auto* level = legoGlobs.currLevel;
    return level->blocks[y * level->width + x];
}

bool write_layers()
{
    return CHECK(write_map("Data/L/counts.map", W, H, counts), "generated resource counts") &&
           CHECK(write_map("Data/L/paths.map", W, H, paths), "generated path records") &&
           CHECK(write_map("Data/L/erosion.map", W, H, erosion), "generated erosion records") &&
           CHECK(write_map("Data/L/triggers.map", W, H, triggers), "generated trigger groups") &&
           CHECK(write_map("Data/L/nodes.map", W, H, nodes), "generated node records") &&
           CHECK(write_map("Data/L/short.map", W - 1, H, nodes), "mismatched dimensions") &&
           CHECK(write_map("Data/L/falls.map", W, H, falls), "generated fall-in records") &&
           CHECK(write_map("Data/L/markers.map", W, H, markers), "generated marker records");
}

void check_layers()
{
    auto* level = legoGlobs.currLevel;
    CHECK_STREQ(level->name, "GridC");
    CHECK_EQ(level->width, W);
    CHECK_EQ(level->height, H);
    for (int x = 1; x <= 4; ++x) {
        int pos[] = {x, 3};
        unsigned crystal0, crystal1, ore0, ore1;
        CHECK_EQ(cell(x, 3).f_05, 4 * x - 2);
        CHECK_EQ(lego_terrain_cell_byte_state_classify_current(pos, &crystal0, &crystal1, &ore0, &ore1), 1);
        const unsigned expected[] = {1, 3, 5, 10};
        CHECK_EQ(ore0, expected[x - 1]);
        CHECK_EQ(crystal0 | crystal1 | ore1, 0u);
    }
    CHECK_EQ(cell(1, 1).f_05, 0u);
    CHECK(cell(1, 2).flags1 & 0x20000000, "power path loaded");
    CHECK_EQ(cell(2, 2).flags1 & 3, 3u);
    CHECK_EQ(cell(3, 2).flags1 & 0x20000000, 0u);
    CHECK_EQ(cell(2, 3).f_06, 2u);
    CHECK_EQ(cell(5, 3).f_06, 1u);
    CHECK_EQ(cell(2, 5).f_34, 17u);
    CHECK_EQ(cell(1, 5).f_34, 0u);
    CHECK_EQ(lego_load_layer_m(level, "L\\short.map"), 0);
    CHECK_EQ(lego_load_layer_m(level, "L\\absent.map"), 0);
    CHECK_EQ(cell(2, 5).f_34, 17u);
    CHECK_EQ(level->f_110, 1);
    CHECK_EQ(cell(2, 4).f_04, 0u); // transferred from the grid into the marker index
    unsigned foundMarkers = 0;
    nerps_block_pointer_enumerate(4, record_marker, &foundMarkers);
    CHECK_EQ(foundMarkers, (1u << 2) | (1u << 5));
    CHECK_EQ(cell(1, 4).f_04, 0u);
    CHECK_EQ(cell(5, 5).f_3c, 2u);
    CHECK_EQ(cell(5, 5).f_38, 0);
    CHECK_EQ(cell(6, 5).f_3c, 3u);
    CHECK_EQ(cell(6, 5).f_38, 1);
    CHECK(cell(5, 5).f_40 >= 0 && cell(5, 5).f_40 < 100, "first fall timer range");
    CHECK(cell(6, 5).f_40 >= 0 && cell(6, 5).f_40 < 350, "spreading fall timer range");
    CHECK_EQ(legoGlobs_f_556f18, 1);
    CHECK_EQ(legoGlobs_f_556f14, 2);
    CHECK_NEAR(legoGlobs_dragBoxRGB[0], 32.0f / 255, 0.0001f);
    CHECK_NEAR(legoGlobs_dragBoxRGB[2], 128.0f / 255, 0.0001f);
    CHECK_NEAR(legoGlobs_fogColourRGB[1], 64.0f / 255, 0.0001f);
    CHECK(legoGlobs.flags1 & 0x10000, "alternate fog enabled");

    if (!CHECK_EQ(level->f_00c, 2u) || !CHECK(level->emergeMap != nullptr, "trigger records")) return;
    const auto* group = static_cast<const LegoEmergeTrigger2A*>(level->emergeMap);
    CHECK_EQ(group[0].blockPos.x, 1);
    CHECK_EQ(group[0].emergePoints[0].blockPos.x, 2);
    CHECK_EQ(group[0].emergePoints[0].used, 1);
    CHECK_EQ(group[0].emergePoints[1].used, 0);
    CHECK_EQ(group[1].blockPos.x, 5);
    CHECK_EQ(group[1].emergePoints[0].used, 0);
    CHECK_EQ(group[1].emergePoints[1].blockPos.x, 6);
    CHECK_EQ(group[1].emergePoints[1].used, 1);
    CHECK_EQ(cell(1, 4).flags2 & 0x80, 0x80u);
    CHECK_EQ(cell(6, 4).flags2 & 0x40, 0x40u);
}

void check_reset()
{
    const auto* level = legoGlobs.currLevel;
    CHECK_STREQ(level->name, "GridA");
    CHECK_EQ(level->emergeMap, nullptr);
    CHECK_EQ(level->f_00c, 0u);
    CHECK_EQ(level->f_110, 0);
    CHECK_EQ(level->blockPointers[4], nullptr);
    CHECK_EQ(legoGlobs_f_556f18, 0);
    CHECK_EQ(legoGlobs_f_556f14, 1);
    CHECK_EQ(legoGlobs.flags1 & 0x8000, 0u);
    CHECK_EQ(cell(2, 5).f_34, 0u);
    CHECK_EQ(cell(2, 4).f_04, 0u);
    CHECK_EQ(cell(1, 2).flags1 & 0x20000000, 0u);
    CHECK_EQ(cell(6, 5).f_3c, 0u);
    CHECK_EQ(cell(1, 3).f_05, 0u);
}
}

int main()
{
    if (!initialise_full_game("optional_grid_layers", "OptionalGridLayers", nullptr, ExtraGrid)) return lrr_test_result();
    if (!write_layers()) return lrr_test_result();
    const char* next = lego_level_free();
    std::free(const_cast<char*>(next));
    if (CHECK(lego_level_load("GridC"), "load configured optional layers")) {
        check_layers();
        if (CHECK(lego_keep_running(), "transition to grid with no optional layers"))
            check_reset();
    }
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("optional_grid_layers");
}
