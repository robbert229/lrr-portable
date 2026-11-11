// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port-only: load a loose wave file into a Sound through the original's
 * mmio wave readers (soundsys.c: WaveOpenFile, WaveStartDataRead,
 * WaveReadFile, WaveCloseReadFile), as 3DSound.c streams them.
 *
 * Kept out of sound.cpp so that only this file sees the Win32 headers that
 * gods98/soundsys.h brings in: in the matching build, every declaration a
 * translation unit adds shifts the symbol numbering of the code after it
 * (finding 506). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "sound.hpp"
#include "soundsys.h"

Sound* sound_wave_file_load(const char* name)
{
    const char* resolved;
    HMMIO hmmio = NULL;
    WAVEFORMATEX* format = NULL;
    MMCKINFO riff;
    MMCKINFO data;
    UINT bytes_read = 0;
    LONG data_start;
    LONG file_end;
    Sound* sound = NULL;

    if (name == NULL) return NULL;
    /* The port's loose-file rule: a mounted WAD entry of the same name wins,
     * and the caller then loads it from the archive. */
    resolved = resource_path_resolve_against_base(name);
    if (resolved == NULL || game_file_open_route_select(resolved, "rb") != 1)
        return NULL;

    if (sound_wave_file_open_read_chunks(resolved, &hmmio, &format, &riff) != 0)
        goto cleanup;
    if (sound_wave_data_chunk_open(&hmmio, &data, &riff) != 0) goto cleanup;
    /* Finding 108: the readers keep non-PCM formats, but playback rejects
     * them before creating a buffer. */
    if (format->wFormatTag != WAVE_FORMAT_PCM) goto cleanup;

    /* The Sound records the whole file's size (the corpus's streamed-size
     * role); measure it and return to the data. */
    data_start = mmioSeek(hmmio, 0, SEEK_CUR);
    file_end = mmioSeek(hmmio, 0, SEEK_END);
    if (data_start < 0 || file_end < 0 ||
        mmioSeek(hmmio, data_start, SEEK_SET) != data_start)
        goto cleanup;

    sound = (Sound*)calloc(1, sizeof(*sound));
    if (sound == NULL) goto cleanup;
    sound->format.format_tag = format->wFormatTag;
    sound->format.channels = format->nChannels;
    sound->format.sample_rate = format->nSamplesPerSec;
    sound->format.byte_rate = format->nAvgBytesPerSec;
    sound->format.block_align = format->nBlockAlign;
    sound->format.bits_per_sample = format->wBitsPerSample;
    sound->data_len = data.cksize;
    sound->file_len = (uint32_t)file_end;
    sound->data = (uint8_t*)malloc(sound->data_len != 0u ? sound->data_len : 1u);
    if (sound->data == NULL) {
        free(sound);
        sound = NULL;
        goto cleanup;
    }
    if (sound_wave_data_read(hmmio, sound->data_len, sound->data, &data,
                             &bytes_read) != 0 ||
        bytes_read != sound->data_len) {
        sound_free(sound);
        sound = NULL;
    }

cleanup:
    sound_wave_handle_and_buffer_release(&hmmio, &format);
    return sound;
}
