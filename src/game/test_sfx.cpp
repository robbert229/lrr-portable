// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_sfx — alias registration, stream/multi sample markers, two-frame
 * request delivery, and a global cooldown without an audio device. */
#include "game/sfx_link_names.h"
#include "game/sfx.h"
#include "gods98/sound3d.h"
#include "test/lrr_test.h"

#include <vector>

static std::string loadedName;
static int loadedStream, loadedMulti, loadedVolume;
struct SampleLoad {
    std::string name;
    int stream, multi, volume;
};
static std::vector<SampleLoad> loads;
static int playedMode, playedSlot, playedLoop;
static unsigned int playCount;

extern "C" int __wrap_sound_bank_entry_load(const char* name, int stream,
                                            int multi, int volume)
{
    loadedName = name;
    loadedStream = stream;
    loadedMulti = multi;
    loadedVolume = volume;
    loads.push_back({name, stream, multi, volume});
    return 5;
}

extern "C" int __wrap_s_play_seam(int mode, intptr_t, int slot, int loop, const float*)
{
    playedMode = mode;
    playedSlot = slot;
    playedLoop = loop;
    ++playCount;
    return 15;
}

extern "C" void __wrap_sound_source_spatial_refresh_all(void) {}

int main(void)
{
    sfx_initialise();
    int drill = -1;
    int custom = -1;
    CHECK_EQ(sfx_get_type("sfx_DRILL", &drill), 1);
    CHECK_EQ(drill, 2);
    CHECK_EQ(sfx_get_type("SFX_TestOnly", &custom), 0);
    CHECK_EQ(custom, -1);
    sfx_set_sample_populate_mode(1);
    CHECK_EQ(sfx_get_type("SFX_TestOnly", &custom), 1);
    CHECK_EQ(custom, 45);
    sfx_set_sample_populate_mode(0);
    int found = -1;
    CHECK_EQ(sfx_get_type("sfx_testonly", &found), 1);
    CHECK_EQ(found, custom);
    CHECK_EQ(sfx_get_type(nullptr, &found), 0);

    char sample[] = "*@synthetic";
    CHECK_EQ(sfx_load_sample_property(sample, custom), 1);
    CHECK_STREQ(loadedName.c_str(), "synthetic");
    CHECK_EQ(loadedStream, 1);
    CHECK_EQ(loadedMulti, 1);
    CHECK_EQ(loadedVolume, 0);
    CHECK_EQ(sfx_random_get_sound3d_handle(custom), 5);
    CHECK_EQ(sfx_random_get_sound3d_handle(0), -1);

    sfx_set_sound_on(1, 0);
    CHECK_EQ(sfx_set_global_sample_duration_if_le0(2.0f), 1);
    sfx_add_to_queue(custom, 1);
    sfx_update(0.5f);
    CHECK_EQ(playCount, 0u);
    CHECK_EQ(sfx_set_global_sample_duration_if_le0(3.0f), 0);
    sfx_update(1.5f);
    CHECK_EQ(playCount, 1u);
    CHECK_EQ(playedMode, 2);
    CHECK_EQ(playedSlot, 5);
    CHECK_EQ(playedLoop, 1);
    CHECK_EQ(sfx_set_global_sample_duration_if_le0(3.0f), 1);
    sfx_update(0.5f);
    CHECK_EQ(playCount, 1u);
    sfx_set_sound_on(0, 0);
    sfx_random_play_sound_normal(custom, 0);
    CHECK_EQ(playCount, 1u);

    // Characterize the loader's existing routing at the bank boundary.
    // The volume marker belongs to each sample, while the stream marker
    // persists across the comma-separated property, as on main.
    char compound[] = "*#-321#@first,#-7#second,third";
    loads.clear();
    CHECK_EQ(sfx_load_sample_property(compound, custom), 1);
    if (CHECK_EQ(loads.size(), 3u)) {
        CHECK_STREQ(loads[0].name.c_str(), "first");
        CHECK_EQ(loads[0].volume, -321);
        CHECK_EQ(loads[0].stream, 1);
        CHECK_EQ(loads[0].multi, 1);
        CHECK_STREQ(loads[1].name.c_str(), "second");
        CHECK_EQ(loads[1].volume, -7);
        CHECK_EQ(loads[1].stream, 1);
        CHECK_EQ(loads[1].multi, 0);
        CHECK_STREQ(loads[2].name.c_str(), "third");
        CHECK_EQ(loads[2].volume, 0);
        CHECK_EQ(loads[2].stream, 1);
        CHECK_EQ(loads[2].multi, 0);
    }
    return lrr_test_result();
}
