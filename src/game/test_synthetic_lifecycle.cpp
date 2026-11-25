// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstring>
#include "game/lego_unit.h"
#include "game/lego_level_record.h"
#include "game/panel_unit.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

int main(int argc, char** argv)
{
    using namespace synthetic_fixture;
    if (argc == 2 && std::strcmp(argv[1], "missing-config") == 0) {
        if (!CHECK(make_fixture("synthetic_missing_config_fixture"), "generate artificial inputs") ||
            !initialise_headless("SyntheticMissingConfig")) return lrr_test_result();
        lego_game_config_callback_registry_init("Lego");
        CHECK_EQ(lego_game_config_init(), 0);
        CHECK(legoGlobs.currLevel == nullptr, "no level after failed startup");
        return lrr_test_result();
    }
    if (!CHECK(make_fixture("synthetic_lifecycle_fixture"), "generate artificial inputs") ||
        !initialise_headless("SyntheticLifecycle") || !boot_game())
        return lrr_test_result();

    CHECK(legoGlobs.currLevel == nullptr, "no level before loading");
    // Unload resets the top panel selection buttons. Supply three empty test buttons.
    const unsigned types[] = {0, 1, 2};
    const float areas[12] = {};
    const int tips[3] = {};
    panel_texture_slots_allocate(PANEL_TOP, 3, types, areas, nullptr, nullptr, nullptr, tips);
    for (unsigned iteration = 0; iteration < 2; ++iteration) {
        if (!CHECK(lego_level_load("Level01"), "load artificial inputs")) break;
        if (!CHECK(legoGlobs.currLevel != nullptr, "level installed")) break;
        CHECK_EQ(legoGlobs.currLevel->width, W);
        CHECK_EQ(legoGlobs.currLevel->height, H);
        for (unsigned frame = 0; frame < 3; ++frame) lego_engine_tick(1.0f);
        CHECK(legoGlobs.currLevel != nullptr, "level survives ticks");
        lego_level_free();
        CHECK(legoGlobs.currLevel == nullptr, "unload clears current level");
    }
    CHECK(lego_level_free() == nullptr, "unload with no current level");
    return lrr_test_result();
}
