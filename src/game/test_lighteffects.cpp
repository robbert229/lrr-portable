// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_lighteffects — spotlight dimming and restoration, disable/reset
 * behavior, and inclusive move limits relative to a position snapshot. */
#include "game/lighteffects.h"
#include "test/lrr_test.h"

struct Light {
    float position[3];
    float color[4];
};

extern "C" void __wrap_render_object_set_packed_rgba_from_float_channels(
    void* scene, float red, float green, float blue, float alpha)
{
    auto& light = *static_cast<Light*>(scene);
    light.color[0] = red;
    light.color[1] = green;
    light.color[2] = blue;
    light.color[3] = alpha;
}

extern "C" void __wrap_ddiscene_object_position_get(void* scene, void*, float* position)
{
    const auto& light = *static_cast<Light*>(scene);
    for (unsigned int i = 0; i < 3u; ++i)
        position[i] = light.position[i];
}

static void check_color(const Light& light, float brightness)
{
    for (unsigned int i = 0; i < 3u; ++i)
        CHECK_NEAR(light.color[i], brightness, 1e-6);
    CHECK_EQ(light.color[3], 1.0f);
}

int main(void)
{
    Light light = { { 10.0f, 20.0f, 30.0f }, {} };
    light_effects_initialise(&light, nullptr, 0.5f, 0.5f, 0.5f);
    check_color(light, 0.5f);
    light_effects_set_dimmer_mode(1);
    light_effects_update(2.5f);
    check_color(light, 0.25f);
    light_effects_update(2.5f);
    light_effects_update(0.0f);
    check_color(light, 0.0f);
    light_effects_set_dimmer_mode(0);
    light_effects_update(5.0f);
    check_color(light, 0.5f);

    light_effects_set_dimmer_mode(1);
    light_effects_update(5.0f);
    check_color(light, 0.0f);
    light_effects_set_disabled(1);
    light_effects_update(5.0f);
    check_color(light, 0.5f);

    light_effects_set_move(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 4.0f, 8.0f);
    const float boundary[] = { 12.0f, 16.0f, 38.0f };
    const float outsideX[] = { 7.0f, 20.0f, 30.0f };
    const float outsideY[] = { 10.0f, 25.0f, 30.0f };
    const float outsideZ[] = { 10.0f, 20.0f, 21.0f };
    CHECK_EQ(light_effects_check_move_limit(boundary), 1);
    CHECK_EQ(light_effects_check_move_limit(outsideX), 0);
    CHECK_EQ(light_effects_check_move_limit(outsideY), 0);
    CHECK_EQ(light_effects_check_move_limit(outsideZ), 0);
    light.position[0] = 7.0f;
    light_effects_invalidate_position();
    CHECK_EQ(light_effects_check_move_limit(outsideX), 1);
    CHECK_EQ(light_effects_check_move_limit(boundary), 0);
    return lrr_test_result("lighteffects");
}
