// SPDX-License-Identifier: LGPL-3.0-or-later
/* soundsys.c — read PCM metadata and bounded sample data from a RIFF/WAVE
 * with an odd-sized ancillary chunk, and reject a different RIFF form. */
#include "gods98/soundsys.h"
#include "test/lrr_test.h"

int main()
{
    unsigned char wave[] = {
        'R', 'I', 'F', 'F', 50, 0, 0, 0, 'W', 'A', 'V', 'E',
        'J', 'U', 'N', 'K', 1, 0, 0, 0, 0xa5, 0,
        'f', 'm', 't', ' ', 16, 0, 0, 0,
        1, 0, 1, 0, 0x40, 0x1f, 0, 0,
        0x40, 0x1f, 0, 0, 1, 0, 8, 0,
        'd', 'a', 't', 'a', 4, 0, 0, 0, 0, 0x80, 0xff, 0x20,
    };
    HMMIO handle = nullptr;
    WAVEFORMATEX* format = nullptr;
    MMCKINFO riff = {};
    if (CHECK_EQ(sound_wave_memory_format_read(wave, sizeof(wave), &handle, &format, &riff), 0) &&
        CHECK(handle != nullptr && format != nullptr, "PCM format and handle are returned")) {
        CHECK_EQ(format->wFormatTag, WAVE_FORMAT_PCM);
        CHECK_EQ(format->nChannels, 1u);
        CHECK_EQ(format->nSamplesPerSec, 8000u);
        CHECK_EQ(format->nAvgBytesPerSec, 8000u);
        CHECK_EQ(format->nBlockAlign, 1u);
        CHECK_EQ(format->wBitsPerSample, 8u);
        CHECK_EQ(format->cbSize, 0u);

        MMCKINFO data = {};
        if (CHECK_EQ(sound_wave_data_chunk_open(&handle, &data, &riff), 0)) {
            CHECK_EQ(data.cksize, 4u);
            BYTE samples[5] = {0, 0, 0, 0, 0xee};
            UINT amount = 0;
            CHECK_EQ(sound_wave_data_read(handle, 2, samples, &data, &amount), 0);
            CHECK_EQ(amount, 2u);
            CHECK_EQ(data.cksize, 2u);
            CHECK_EQ(sound_wave_data_read(handle, 3, samples + 2, &data, &amount), 0);
            CHECK_EQ(amount, 2u);
            CHECK_EQ(data.cksize, 0u);
            const BYTE expected[] = {0, 0x80, 0xff, 0x20, 0xee};
            CHECK_EQ(std::memcmp(samples, expected, sizeof(samples)), 0);
            CHECK_EQ(sound_wave_data_read(handle, 1, samples, &data, &amount), 0);
            CHECK_EQ(amount, 0u);
        }
    }
    CHECK_EQ(sound_wave_handle_and_buffer_release(&handle, &format), 0);
    CHECK_EQ(handle, nullptr);
    CHECK_EQ(format, nullptr);

    std::memcpy(wave + 8, "AVI ", 4);
    CHECK(sound_wave_memory_format_read(wave, sizeof(wave), &handle, &format, &riff) != 0,
          "a RIFF form other than WAVE is rejected");
    CHECK_EQ(handle, nullptr);
    CHECK_EQ(format, nullptr);
    sound_wave_handle_and_buffer_release(&handle, &format);
    return lrr_test_result();
}
