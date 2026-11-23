// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_spiderweb — web health, trapped-raider release when a web breaks,
 * and rebuilding a clean grid after restarting. */
#include "game/spiderweb.h"
#include "test/lrr_test.h"

extern "C" int __wrap_legoobject_position_to_terrain_cell(Lego_Object*, int* x, int* y)
{
    *x = 2;
    *y = 1;
    return 1;
}

extern "C" uint32_t __wrap_object_type_name_index_find(const char* name)
{
    return std::strcmp(name, "SpiderWeb") == 0 ? 7u : 0u;
}

extern "C" float __wrap_weapons_get_damage_for_object(int weaponType, const void*)
{
    return weaponType == 7 ? 20.0f : 0.0f;
}

int main(void)
{
    constexpr unsigned int stuck = 0x2000000;
    constexpr unsigned int removing = 0x800000;
    Lego_Level level = {};
    level.width = 3;
    level.height = 2;
    spiderweb_initialise(&level);
    if (!CHECK(spiderWebGlobs.webBlocks != nullptr, "the web grid is allocated"))
        return lrr_test_result();

    Lego_Object web = {};
    web.type = 12;
    web.flags1 = stuck;
    Lego_Object raider = {};
    raider.type = 2;
    raider.flags1 = stuck;
    spiderweb_add(2, 1, &web);
    const SpiderWeb_Block& cell = spiderWebGlobs.webBlocks[5];
    CHECK_EQ(cell.webObj, &web);
    CHECK_EQ(cell.health, 100.0f);
    CHECK_EQ(cell.flags, static_cast<unsigned int>(SPIDERWEB_FLAG_WEB));
    CHECK_EQ(spiderWebGlobs.webBlocks[4].flags, 0u);

    float elapsed = 4.0f;
    spiderweb_callback_update(&raider, &elapsed);
    CHECK_EQ(cell.health, 20.0f);
    CHECK_EQ(raider.flags1 & stuck, stuck);
    elapsed = 2.0f;
    spiderweb_callback_update(&raider, &elapsed);
    CHECK_EQ(cell.flags & SPIDERWEB_FLAG_WEB, 0u);
    CHECK_EQ(web.flags3 & removing, removing);
    CHECK_EQ(web.flags1 & stuck, 0u);
    CHECK_EQ(raider.flags1 & stuck, 0u);

    spiderweb_restart(&level);
    if (CHECK(spiderWebGlobs.webBlocks != nullptr, "the restarted grid is allocated")) {
        CHECK_EQ(spiderWebGlobs.webBlocks[5].webObj, nullptr);
        CHECK_EQ(spiderWebGlobs.webBlocks[5].health, 0.0f);
        CHECK_EQ(spiderWebGlobs.webBlocks[5].flags, 0u);
    }
    spiderweb_shutdown();
    CHECK_EQ(spiderWebGlobs.webBlocks, nullptr);
    return lrr_test_result();
}
