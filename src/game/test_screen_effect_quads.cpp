// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_screen_effect_quads — the fixed pool for damage numbers, reuse
 * after the lifetime expires or the pool is cleared, and object filters. */
#include "game/legoobject_record.h"
#include "game/screen_effect_quads_link_names.h"
#include "game/screen_effect_quads.h"
#include "test/lrr_test.h"

static uint32_t statsFlags;

extern "C" void __wrap_scene_render_lock_toggle(void*, int)
{
}

extern "C" uint32_t __wrap_entity_state_flags_get(const Lego_Object*)
{
    return statsFlags;
}

int main(void)
{
    DamageFont_Data* slots[10] = {};
    for (auto& slot : slots) {
        slot = damage_font_get_next_free();
        CHECK(slot != nullptr, "the pool has room for ten numbers");
    }
    for (unsigned int i = 0; i < 10u; ++i) {
        for (unsigned int j = 0; j < i; ++j)
            CHECK(slots[i] != slots[j], "active numbers have distinct slots");
    }
    CHECK_EQ(damage_font_get_next_free(), nullptr);
    if (slots[0] != nullptr) {
        damage_font_update(slots[0], 15.0f);
        CHECK_EQ(damage_font_get_next_free(), nullptr);
        damage_font_update(slots[0], 0.0f);
        CHECK_EQ(damage_font_get_next_free(), slots[0]);
    }
    damage_font_cleanup();
    CHECK_EQ(damage_font_get_next_free(), slots[0]);

    Lego_Object object = {};
    object.type = 2; // Minifigure.
    CHECK_EQ(damage_font_live_object_check_can_show_damage(&object), 1);
    statsFlags = 0x08000000u; // Suppress damage numbers.
    CHECK_EQ(damage_font_live_object_check_can_show_damage(&object), 0);
    statsFlags = 0;
    object.type = 8; // Dynamite.
    CHECK_EQ(damage_font_live_object_check_can_show_damage(&object), 0);
    object.type = 13; // Upgrade part.
    CHECK_EQ(damage_font_live_object_check_can_show_damage(&object), 0);
    return lrr_test_result("screen_effect_quads");
}
