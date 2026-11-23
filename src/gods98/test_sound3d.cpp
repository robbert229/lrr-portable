// SPDX-License-Identifier: LGPL-3.0-or-later
/* sound3d.c — sample-bank volume boundaries and disabled-audio behavior
 * without opening a mixer or loading a WAV. */
#include "gods98/sound3d.h"
#include "test/lrr_test.h"

int main()
{
    const int silent = constant_negative_10000_get();
    CHECK_EQ(silent, -10000);
    CHECK_EQ(sound_volume_value_valid(silent - 1), 0);
    CHECK_EQ(sound_volume_value_valid(silent), 1);
    CHECK_EQ(sound_volume_value_valid(-1), 1);
    CHECK_EQ(sound_volume_value_valid(0), 1);
    CHECK_EQ(sound_volume_value_valid(1), 0);

    sound_bank_entry_volume_set(2, silent);
    sound_bank_entry_volume_set(5, -675);
    CHECK_EQ(sound_bank_entry_volume(2), silent);
    CHECK_EQ(sound_bank_entry_volume(5), -675);
    sound_bank_entry_volume_set(2, silent - 1);
    CHECK_EQ(sound_bank_entry_volume(2), 0);
    CHECK_EQ(sound_bank_entry_volume(5), -675);
    sound_bank_entry_volume_set(5, 1);
    CHECK_EQ(sound_bank_entry_volume(5), 0);

    CHECK_EQ(sound_bank_entry_load("synthetic", 0, 0, 0), -1);
    CHECK_EQ(sound_bank_entry_release(-1), 0);
    CHECK_EQ(s_play_seam(2, 0, 2, 0, nullptr), -1);
    CHECK_NEAR(sound_streamed_wave_buffer_step_rate(-1), 0.0f, 1e-6);

    const float position[] = {1.0f, 2.0f, 3.0f};
    const float same[] = {1.0f, 2.0f, 3.0f};
    const float different[] = {1.0f, 2.0f, 3.5f};
    CHECK_EQ(vec_exact_equal(position, same), 1);
    CHECK_EQ(vec_exact_equal(position, different), 0);
    return lrr_test_result();
}
