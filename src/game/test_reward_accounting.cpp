// SPDX-License-Identifier: LGPL-3.0-or-later
/* Mission result accounting on the existing generated grid. No reward
 * screen, save image, or shipped game data is used. */
#include <string>
#include <vector>
#include "gods98/config.h"
#include "gods98/directdraw.h"
#include "game/lego_unit.h"
#include "game/reward.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"

namespace {
void accounting(bool configured)
{
    auto* level = legoGlobs.currLevel;
    if (!CHECK(level != nullptr, "loaded abstract level") ||
        !CHECK(rewardGlobs.itemPart != nullptr, "initialised reward records")) return;
    CHECK_EQ(reward_current_level_get(), nullptr);
    CHECK_EQ(reward_level_start(), 0);
    CHECK_EQ(reward_prepare_results(), 0);
    CHECK_EQ(reward_format_values(), 0);

    const std::string config = std::string("Fixture { ") + level->name +
        (configured ? " { Reward { Enable TRUE Modifier 3 "
                      "Importance { Crystals 2 Oxygen 1 Timer 1 Caverns 1 } "
                      "Quota { Crystals 8 Timer 4 Caverns 4 } } } "
                      "Reward { Timer 12 DisplayText FALSE DisplayImages FALSE DisplayFlics FALSE } }"
                    : " { Reward { } } }");
    BlockFile* parsed = block_parse(config.c_str(), config.size());
    if (!CHECK(parsed != nullptr, "parse invented result settings")) return;

    // Borrow the already-initialised, generated grayscale font for the
    // result formatter's owned summary labels; no screen is shown.
    auto* oldConfig = rewardGlobs.config;
    auto* oldRoot = rewardGlobs.gameName;
    auto* oldFont = rewardGlobs.font;
    auto* oldTitle = rewardGlobs.titleFont;
    auto* oldComplete = rewardGlobs.completeText;
    auto* oldFailed = rewardGlobs.failedText;
    char complete[] = "Sample_complete";
    char failed[] = "Sample_failed";
    rewardGlobs.config = parsed;
    rewardGlobs.gameName = "Fixture";
    rewardGlobs.font = rewardGlobs.titleFont = static_cast<TextureSurface*>(legoGlobs.fontBriefingHi);
    rewardGlobs.completeText = complete;
    rewardGlobs.failedText = failed;

    CHECK_EQ(reward_prepare_level(), 1);
    auto* result = reward_current_level_get();
    if (CHECK(result != nullptr, "prepared result record")) {
        auto* items = result->items;
        CHECK_EQ(items, result->itemsArray);
        CHECK_EQ(result->itemCount, static_cast<unsigned int>(REWARD_ITEM_COUNT));
        CHECK_EQ(result->enabled, configured ? 1 : 0);
        CHECK_EQ(result->modifier, configured ? 3u : 0u);
        CHECK_EQ(rewardGlobs.timer, configured ? 12.0f : 76.9375f);
        CHECK_EQ(rewardGlobs.displayText, configured ? 0 : 1);
        CHECK_EQ(rewardGlobs.displayImages, configured ? 0 : 1);
        CHECK_EQ(rewardGlobs.displayFlics, configured ? 0 : 1);
        CHECK_EQ(items[REWARD_ITEM_CRYSTALS].quota, configured ? 8u : 0u);
        CHECK_EQ(items[REWARD_ITEM_CRYSTALS].importance, configured ? 2.0f : 0.0f);
        CHECK_EQ(items[REWARD_ITEM_ORE].importance, 0.0f);
        CHECK_EQ(reward_level_start(), 1);
        CHECK_EQ(rewardGlobs.level.buildingCounts.count, 0u);
        CHECK_EQ(rewardGlobs.level.buildingCounts.gridCount, 0u);

        // Stage the result inputs directly: this test is about aggregation
        // of the harvested-resource, wall, oxygen and elapsed-time counters,
        // independently of the collection and excavation scenario tests.
        const std::vector<Lego_Block> savedBlocks(level->blocks,
            level->blocks + level->width * level->height);
        for (unsigned i = 0; i < level->width * level->height; ++i) {
            level->blocks[i].f_02 = 1; // no diggable material
            level->blocks[i].f_05 = 0;
            level->blocks[i].flags1 = 0;
        }
        auto& a = level->blocks[level->width * 2 + 1];
        a.f_02 = 8; a.f_05 = 5; // wall, three crystals
        auto& b = level->blocks[level->width * 2 + 2];
        b.f_02 = 10; b.f_05 = 10; // wall, four seam crystals and five ore
        auto& c = level->blocks[level->width * 3 + 1];
        c.f_02 = 8; c.f_05 = 3;
        c.flags1 = 0x8u | 0x100000u; // cleared wall; alternate tier excluded
        auto& d = level->blocks[level->width * 3 + 2];
        d.f_02 = 6; d.flags1 = 0x8u; // liquid floor, excluded from wall count
        level->f_08c = configured ? 4 : 9;
        level->crystals = 2;
        level->oxygenLevel = 75;
        level->status = configured ? 1 : 2;
        for (int i = 0; i < 50; ++i) reward_score_add(1.0f);
        global_float_at_00554adc_add_one();
        global_float_at_00554adc_add_one();
        global_float_at_00554efc_add_one();
        global_float_at_00554efc_add_one();
        CHECK_EQ(reward_prepare_results(), 1);
        for (unsigned i = 0; i < savedBlocks.size(); ++i) {
            level->blocks[i].f_02 = savedBlocks[i].f_02;
            level->blocks[i].f_05 = savedBlocks[i].f_05;
            level->blocks[i].flags1 = savedBlocks[i].flags1;
        }
        CHECK_EQ(result->blockCount, level->map->blockWidth * level->map->blockHeight);
        CHECK_EQ(items[REWARD_ITEM_CRYSTALS].total, configured ? 8.0f : 7.0f);
        CHECK_EQ(items[REWARD_ITEM_CRYSTALS].actual, configured ? 4.0f : 9.0f);
        CHECK_EQ(items[REWARD_ITEM_CRYSTALS].percent, configured ? 50.0f : 100.0f);
        CHECK_EQ(items[REWARD_ITEM_ORE].actual, 2.0f);
        CHECK_EQ(items[REWARD_ITEM_DIGGABLE].total, 3.0f);
        CHECK_EQ(items[REWARD_ITEM_ORE].total, 17.0f);
        CHECK_EQ(result->walls.total, 3.0f);
        CHECK_EQ(result->walls.actual, 1.0f);
        CHECK_EQ(items[REWARD_ITEM_DIGGABLE].actual, 2.0f);
        CHECK_EQ(items[REWARD_ITEM_CAVERNS].actual, 2.0f);
        CHECK_EQ(items[REWARD_ITEM_CAVERNS].percent, configured ? 50.0f : 0.0f);
        CHECK_EQ(items[REWARD_ITEM_OXYGEN].percent, 75.0f);
        CHECK_EQ(items[REWARD_ITEM_TIMER].actual, 50.0f);
        CHECK_EQ(items[REWARD_ITEM_TIMER].total, 1.0f);
        CHECK_EQ(items[REWARD_ITEM_TIMER].percent, configured ? 50.0f : 0.0f);
        CHECK_EQ(items[REWARD_ITEM_TIMER].percentInt, 2u);
        CHECK_STREQ(items[REWARD_ITEM_TIMER].percentText, "00:00.01");
        CHECK_STREQ(items[REWARD_ITEM_CRYSTALS].percentText, configured ? "50%%" : "100%%");
        CHECK_NEAR(items[REWARD_ITEM_SCORE].percent, configured ? 58.0f : 100.0f, 1e-4f);
        CHECK_STREQ(rewardGlobs.statusText, configured ? "Sample complete" : "Sample failed");
        if (CHECK(rewardGlobs.scroll != nullptr, "result owns summary record")) {
            if (CHECK_EQ(rewardGlobs.scroll->labelCount, 2u)) {
                CHECK_STREQ(rewardGlobs.scroll->labels[0]->text, level->fullName);
                CHECK_STREQ(rewardGlobs.scroll->labels[1]->text, rewardGlobs.statusText);
            }
            CHECK_EQ(reward_scroll_free(&rewardGlobs.scroll), 1);
            rewardGlobs.scroll = nullptr;
        }
        CHECK_EQ(rewardGlobs.level.itemsArray[REWARD_ITEM_TIMER].actual, 0.0f);
        CHECK_EQ(rewardGlobs.level.itemsArray[REWARD_ITEM_DIGGABLE].actual, 0.0f);
        CHECK_EQ(rewardGlobs.level.itemsArray[REWARD_ITEM_CAVERNS].actual, 0.0f);
        reward_free_level();
        rewardGlobs.current = nullptr;
    }
    rewardGlobs.config = oldConfig;
    rewardGlobs.gameName = oldRoot;
    rewardGlobs.font = oldFont;
    rewardGlobs.titleFont = oldTitle;
    rewardGlobs.completeText = oldComplete;
    rewardGlobs.failedText = oldFailed;
    block_close(parsed);
}
}

int main(int argc, char** argv)
{
    const std::string variant = argc == 2 ? argv[1] : "configured";
    if (!CHECK(variant == "configured" || variant == "defaults", "known variant") ||
        !synthetic_fixture::initialise_full_game(("reward_accounting_" + variant).c_str(),
                                                 "RewardAccounting")) return lrr_test_result();
    accounting(variant == "configured");
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("reward_accounting");
}
