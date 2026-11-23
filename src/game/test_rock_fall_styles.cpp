// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_rock_fall_styles — style-name lookup from parsed config and
 * miscellaneous effect loading when only the object format is available. */
#include "game/rock_fall_styles_link_names.h"
#include "game/rock_fall_styles.h"
#include "gods98/config.h"
#include "test/lrr_test.h"

struct Effect_Misc;
extern "C" Effect_Misc* effect_get_misc_effect_data(unsigned int type);
extern "C" int effect_load_misc_effect(Effect_Misc* misc, void* root, const char* filename);
extern "C" int effect_get_rock_fall_style(const char* name, unsigned int* style);
extern "C" void effect_load_rock_fall_styles_all(BlockFile* config, const char* gameName, void* root);

struct Scene {
    bool hidden;
};
static Scene crumble;

extern "C" void* __wrap_container_load(void*, const char* name, const char* format, int)
{
    if (std::strcmp(name, "Crumble") == 0 && std::strcmp(format, "LWO") == 0)
        return &crumble;
    return nullptr;
}

extern "C" void __wrap_scene_render_lock_toggle(void* object, int hide)
{
    static_cast<Scene*>(object)->hidden = hide != 0;
}

int main(void)
{
    const char config[] = "Game { RockFallStyles { Granite Pieces Ice Shards } }";
    BlockFile* root = block_parse(config, sizeof(config) - 1u);
    if (!CHECK(root != nullptr, "the style config parses"))
        return lrr_test_result("rock_fall_styles");
    effect_load_rock_fall_styles_all(root, "Game", nullptr);
    block_close(root);
    block_text_token_pool_destroy();

    unsigned int style = 9;
    CHECK_EQ(effect_get_rock_fall_style("granite", &style), 1);
    CHECK_EQ(style, 0u);
    CHECK_EQ(effect_get_rock_fall_style("ICE", &style), 1);
    CHECK_EQ(style, 1u);
    CHECK_EQ(effect_get_rock_fall_style("Missing", &style), 0);
    CHECK_EQ(style, 1u);

    Effect_Misc* first = effect_get_misc_effect_data(0u);
    Effect_Misc* last = effect_get_misc_effect_data(9u);
    CHECK(first != nullptr && last != nullptr && first != last,
          "valid particle types have distinct slots");
    CHECK_EQ(effect_get_misc_effect_data(10u), nullptr);
    if (first != nullptr) {
        CHECK_EQ(effect_load_misc_effect(first, nullptr, nullptr), 0);
        CHECK_EQ(effect_load_misc_effect(first, nullptr, "Missing"), 0);
        CHECK_EQ(effect_load_misc_effect(first, nullptr, "Crumble"), 1);
        CHECK(crumble.hidden, "the loaded effect template starts hidden");
    }
    return lrr_test_result("rock_fall_styles");
}
