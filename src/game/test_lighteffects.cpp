// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_lighteffects — spotlight dimming and restoration, disable/reset
 * behavior, and inclusive move limits relative to a position snapshot. */
#include <cmath>
#include <string>
#include "game/lighteffects.h"
#include "crt/vc5names.h"
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

extern "C" void __wrap_ddiscene_object_position_set(void* scene, void*, float x, float y, float z)
{
    auto& light = *static_cast<Light*>(scene);
    light.position[0] = x;
    light.position[1] = y;
    light.position[2] = z;
}

static void check_rgb(const Light& light, float r, float g, float b)
{
    CHECK_NEAR(light.color[0], r, 1e-6);
    CHECK_NEAR(light.color[1], g, 1e-6);
    CHECK_NEAR(light.color[2], b, 1e-6);
    CHECK_EQ(light.color[3], 1.0f);
}

static void check_color(const Light& light, float brightness)
{
    for (unsigned int i = 0; i < 3u; ++i)
        CHECK_NEAR(light.color[i], brightness, 1e-6);
    CHECK_EQ(light.color[3], 1.0f);
}

static void dimmer_and_limits()
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
}

using Loader = int (*)(BlockFile*, const char*);
static bool load(Loader loader, const char* settings, int expected)
{
    const std::string text = std::string("Sample { LightEffects { ") + settings + " } }";
    auto* config = block_parse(text.c_str(), text.size());
    if (!CHECK(config != nullptr, "parse synthetic light configuration")) return false;
    const bool result = CHECK_EQ(loader(config, "Sample"), expected);
    block_close(config);
    return result;
}

static void blink()
{
    Light light{};
    light_effects_initialise(&light, nullptr, 0.4f, 0.5f, 0.6f);
    if (!load(light_effects_load_blink, "BlinkRGBMax 51:102:25", 0) ||
        !load(light_effects_load_blink,
              "BlinkRGBMax 51:102:25 MaxChangeAllowed 51 RandomRangeForTimeBetweenBlinks 0.4:0.4", 1))
        return;
    crt_srand(1729);
    // The first call starts an event; later events are separated by ten
    // update units. All three channels share one signed random factor.
    bool changed = false;
    for (unsigned i = 0; i < 12; ++i) {
        light_effects_update_blink(i == 0 ? 0 : 1);
        light_effects_update_colour();
        const float redDelta = light.color[0] - 0.4f;
        changed |= std::fabs(redDelta) > 1e-5f;
        CHECK(std::fabs(redDelta) < 0.2f, "red drift stays within configured budget");
        CHECK_NEAR(light.color[1] - 0.5f, redDelta * 2, 1e-6);
        CHECK_NEAR(light.color[2] - 0.6f, redDelta * (25.0f/51), 1e-6);
        const float r = light.color[0], g = light.color[1], b = light.color[2];
        light_effects_update_blink(9);
        light_effects_update_colour();
        check_rgb(light, r, g, b);
    }
    CHECK(changed, "seeded blink changes color");
    // A zero drift budget rejects even the sign-flipped alternative.
    if (!load(light_effects_load_blink,
              "BlinkRGBMax 51:102:25 MaxChangeAllowed 0 RandomRangeForTimeBetweenBlinks 0.4:0.4", 1))
        return;
    const float r = light.color[0], g = light.color[1], b = light.color[2];
    light_effects_update_blink(100);
    light_effects_update_colour();
    check_rgb(light, r, g, b);
}

static void fade(bool negative)
{
    Light light{};
    light_effects_initialise(&light, nullptr, 0.4f, 0.5f, 0.6f);
    if (!load(light_effects_load_fade, "FadeRGBMin 51:25:76 FadeRGBMax 51:25:76 "
              "RandomRangeForTimeBetweenFades 0.8:0.8", 0)) return;
    const std::string rgb = negative ? "-51:-25:-76" : "51:25:76";
    const std::string settings = "FadeRGBMin " + rgb + " FadeRGBMax " + rgb +
        " RandomRangeForTimeBetweenFades 0.8:0.8 RandomRangeForFadeTimeFade 0.4:0.4 "
        "RandomRangeForHoldTimeOfFade 0.2:0.2";
    if (!load(light_effects_load_fade, settings.c_str(), 1)) return;
    const float sign = negative ? -1.0f : 1.0f;
    auto check = [&](float fraction) {
        light_effects_update_colour();
        check_rgb(light, 0.4f + sign*fraction*51/255,
                  0.5f + sign*fraction*25/255, 0.6f + sign*fraction*76/255);
    };
    // Exercise the real phase boundaries without the separate dimmer.
    light_effects_update_fade(0);
    check(0);
    light_effects_update_fade(5);
    check(0.5f);
    light_effects_update_fade(8); // overshoot clamps to the destination
    check(1);
    light_effects_update_fade(4);
    check(1);
    light_effects_update_fade(1); // hold expires; return starts next update
    check(1);
    light_effects_update_fade(5);
    check(0.5f);
    light_effects_update_fade(8);
    check(0);
    light_effects_update_fade(19);
    check(0);
    light_effects_update_fade(1);
    check(0);
    light_effects_update_fade(5);
    check(0.5f);
}

static void movement()
{
    const float origin[] = {10, 20, 30};
    Light light{{10, 20, 30}, {}};
    light_effects_initialise(&light, nullptr, 0.4f, 0.5f, 0.6f);
    if (!load(light_effects_load_move, "RandomRangeForTimeBetweenMoves 0.4:0.4", 0) ||
        !load(light_effects_load_move,
              "RandomRangeForTimeBetweenMoves 0.4:0.4 RandomRangeForSpeedOfMove 0.08:0.08 "
              "RandomRangeForDistOfMove 6:6 MoveLimit 2:2:2", 1)) return;
    crt_srand(31415);
    light_effects_update_move(0); // choose direction, no displacement yet
    for (unsigned i = 0; i < 3; ++i) CHECK_EQ(light.position[i], origin[i]);
    light_effects_update_move(1);
    float offset[3]{}, squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        offset[i] = light.position[i] - origin[i];
        squared += offset[i]*offset[i];
    }
    CHECK_NEAR(std::sqrt(squared), 2.0f, 1e-5);
    CHECK_EQ(light_effects_check_move_limit(light.position), 1);
    // A second step in the same direction exceeds at least one of the
    // cube's limits; the unit tries the reverse step instead.
    light_effects_update_move(1);
    for (unsigned i = 0; i < 3; ++i) CHECK_NEAR(light.position[i], origin[i], 1e-5);
    light_effects_update_move(1);
    for (unsigned i = 0; i < 3; ++i) CHECK_NEAR(light.position[i], origin[i]+offset[i], 1e-5);
    light_effects_update_move(9);
    for (unsigned i = 0; i < 3; ++i) CHECK_NEAR(light.position[i], origin[i]+offset[i], 1e-5);
    // A valid zero-sized limit allows neither direction; movement stops.
    if (!load(light_effects_load_move,
              "RandomRangeForTimeBetweenMoves 0.4:0.4 RandomRangeForSpeedOfMove 0.08:0.08 "
              "RandomRangeForDistOfMove 6:6 MoveLimit 0:0:0", 1)) return;
    light_effects_invalidate_position();
    light_effects_update_move(1);
    light_effects_update_move(1);
    light_effects_update_move(1);
    for (unsigned i = 0; i < 3; ++i) CHECK_NEAR(light.position[i], origin[i]+offset[i], 1e-5);
    CHECK_EQ(light_effects_check_move_limit(light.position), 1);
}

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "dimmer";
    if (scenario == "dimmer") dimmer_and_limits();
    else if (scenario == "blink") blink();
    else if (scenario == "fade") fade(false);
    else if (scenario == "fade-negative") fade(true);
    else if (scenario == "move") movement();
    else CHECK(false, "known scenario");
    return lrr_test_result("lighteffects");
}
