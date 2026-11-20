// SPDX-License-Identifier: LGPL-3.0-or-later
/* The audio substitution ([library:audio], ADR 0007) on synthetic waves.
 *
 *   audio_test dummy   SDL's dummy driver: the library's arithmetic (dB to
 *                      gain, the 3D pan and rolloff, the mixer's fader),
 *                      soundsys.c's wave readers through the mmio
 *                      substitution, DirectSound buffers (play, loop, stop,
 *                      duplicate, volume, formats), the mixer through
 *                      main.c's CD volume, and the MCI cdaudio device with
 *                      and without track files.
 *   audio_test disk    SDL's disk driver, whose output file is the mix:
 *                      a streamed 8-bit wave played by sound3d.c's stream
 *                      path (the menu overlays' path), a CD track through
 *                      soundsys.c at full and half music volume, a short
 *                      stream at SFX slider 5, then a mono 3D buffer
 *                      panned right, left, by a turned listener and at 45
 *                      degrees. The mix must hold each, at the levels and
 *                      on the channels their formats and settings imply.
 *
 * Each run works in a fresh temporary directory. Exit 77 skips. */
#include <SDL3/SDL.h>

#include "gods98/audio.h"
#include "gods98/sound3d.h"
#include "gods98/soundsys.h"
#include "library/audio_sdl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <unistd.h>

extern "C" {
int sound_streamed_wave_play(const char* fName, int loop, int volume);
/* main.c (G98-005): Main_SetCDVolume and Main_GetCDVolume. */
void music_mixer_volume_set(float leftVolume, float rightVolume);
int ledger_stub_004789a0_MusicMixerVolumeGet(float* leftVolume, float* rightVolume);
}

namespace {

int s_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,        \
                    __LINE__, #cond);                                     \
            s_failures++;                                                 \
        }                                                                 \
    } while (0)

void PutLe16(std::vector<char>& out, unsigned value)
{
    out.push_back((char)(value & 0xff));
    out.push_back((char)((value >> 8) & 0xff));
}

void PutLe32(std::vector<char>& out, unsigned long value)
{
    PutLe16(out, (unsigned)(value & 0xffff));
    PutLe16(out, (unsigned)((value >> 16) & 0xffff));
}

void PutId(std::vector<char>& out, const char* id)
{
    out.insert(out.end(), id, id + 4);
}

/* A RIFF WAVE: 'fmt ', an odd-sized 'LIST' the readers must skip (with its
 * pad byte), then 'data'. */
std::vector<char> MakeWave(unsigned channels, unsigned rate, unsigned bits,
                           const std::vector<char>& samples)
{
    std::vector<char> body;
    PutId(body, "WAVE");
    PutId(body, "fmt ");
    PutLe32(body, 16);
    PutLe16(body, 1);
    PutLe16(body, channels);
    PutLe32(body, rate);
    unsigned block = channels * bits / 8;
    PutLe32(body, rate * block);
    PutLe16(body, block);
    PutLe16(body, bits);
    PutId(body, "LIST");
    PutLe32(body, 3);
    body.insert(body.end(), { 'a', 'b', 'c', 0 });
    PutId(body, "data");
    PutLe32(body, (unsigned long)samples.size());
    body.insert(body.end(), samples.begin(), samples.end());
    if (samples.size() & 1) body.push_back(0);
    std::vector<char> wave;
    PutId(wave, "RIFF");
    PutLe32(wave, (unsigned long)body.size());
    wave.insert(wave.end(), body.begin(), body.end());
    return wave;
}

/* 8-bit unsigned mono: a square wave of +-0.75 at 441 Hz. */
std::vector<char> SquareU8(unsigned rate, double seconds)
{
    std::vector<char> samples((size_t)(rate * seconds));
    unsigned period = rate / 441;
    for (size_t i = 0; i < samples.size(); i++) {
        samples[i] = (char)((i % period) < period / 2 ? 0xe0 : 0x20);
    }
    return samples;
}

/* 16-bit, mono or stereo: a sine of amplitude 0.5 at 440 Hz. */
std::vector<char> SineS16(unsigned rate, double seconds, int channels = 2)
{
    std::vector<char> samples;
    size_t frames = (size_t)(rate * seconds);
    for (size_t i = 0; i < frames; i++) {
        int value = (int)lround(0.5 * 32767.0 * sin(2.0 * M_PI * 440.0 * i / rate));
        for (int c = 0; c < channels; c++) PutLe16(samples, (unsigned)(value & 0xffff));
    }
    return samples;
}

bool Near(float value, float want, float tolerance = 1.0e-4f)
{
    return fabsf(value - want) <= tolerance;
}

bool WriteFile(const std::string& path, const std::vector<char>& bytes)
{
    FILE* file = fopen(path.c_str(), "wb");
    if (!file) return false;
    bool ok = fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
    return fclose(file) == 0 && ok;
}

std::string MakeTempDir()
{
    char pattern[] = "/tmp/lrr-audio-test-XXXXXX";
    const char* dir = mkdtemp(pattern);
    return dir ? std::string(dir) : std::string();
}

/* Waits up to timeoutMs for done(). */
template <typename F>
bool WaitFor(F done, int timeoutMs)
{
    Uint64 end = SDL_GetTicks() + (Uint64)timeoutMs;
    while (SDL_GetTicks() < end) {
        if (done()) return true;
        SDL_Delay(5);
    }
    return done();
}

/* --- dummy: the readers, the buffers and the CD device ------------------- */

void TestArithmetic()
{
    using namespace audio_sdl_detail;
    /* DirectSound's volume: hundredths of a decibel. */
    CHECK(GainOfVolume(0) == 1.0f);
    CHECK(GainOfVolume(-10000) == 0.0f);
    CHECK(GainOfVolume(-20000) == 0.0f);
    CHECK(Near(GainOfVolume(-600), 0.501187f));
    CHECK(Near(GainOfVolume(-2000), 0.1f));
    CHECK(Near(GainOfVolume(-300), 0.707946f));

    /* The mixer's fader. */
    CHECK(GainOfFader(0) == 0.0f);
    CHECK(GainOfFader(0xffff) == 1.0f);
    CHECK(GainOfFader(0x10000) == 1.0f);
    CHECK(Near(GainOfFader(0x8000), 0.500008f));

    /* The pan, for DirectSound's default listener: +x is right. */
    const float front[3] = { 0.0f, 0.0f, 1.0f };
    const float top[3] = { 0.0f, 1.0f, 0.0f };
    float left, right;
    const float ahead[3] = { 0.0f, 0.0f, 5.0f };
    GainsOf3D(ahead, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(left == 1.0f && right == 1.0f);
    const float behind[3] = { 0.0f, 0.0f, -5.0f };
    GainsOf3D(behind, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(Near(left, 1.0f) && Near(right, 1.0f));
    const float above[3] = { 0.0f, 5.0f, 0.0f };
    GainsOf3D(above, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(Near(left, 1.0f) && Near(right, 1.0f));
    const float onRight[3] = { 5.0f, 0.0f, 0.0f };
    GainsOf3D(onRight, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(Near(left, 0.0f) && right == 1.0f);
    const float onLeft[3] = { -5.0f, 0.0f, 0.0f };
    GainsOf3D(onLeft, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(left == 1.0f && Near(right, 0.0f));
    const float rightAhead[3] = { 3.0f, 0.0f, 3.0f };
    GainsOf3D(rightAhead, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(Near(left, 1.0f - sqrtf(0.5f)) && right == 1.0f);
    /* At the listener: no direction, no pan. */
    const float here[3] = { 0.0f, 0.0f, 0.0f };
    GainsOf3D(here, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(left == 1.0f && right == 1.0f);

    /* A turned listener: facing +x, its right is -z. */
    const float east[3] = { 1.0f, 0.0f, 0.0f };
    const float north[3] = { 0.0f, 0.0f, 5.0f };
    GainsOf3D(north, east, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(left == 1.0f && Near(right, 0.0f));
    /* Looking down, as the game's camera does, with +z up the screen:
     * its right is still +x, and height does not pan. */
    const float down[3] = { 0.0f, -1.0f, 0.0f };
    const float screenUp[3] = { 0.0f, 0.0f, 1.0f };
    GainsOf3D(onRight, down, screenUp, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(Near(left, 0.0f) && right == 1.0f);
    const float below[3] = { 0.0f, -8.0f, 0.0f };
    GainsOf3D(below, down, screenUp, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(Near(left, 1.0f) && Near(right, 1.0f));

    /* The rolloff: min / distance to the factor's power, silent at max. */
    const float far[3] = { 0.0f, 0.0f, 40.0f };
    GainsOf3D(far, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(Near(left, 0.25f) && Near(right, 0.25f));
    GainsOf3D(far, front, top, 10.0f, 100.0f, 2.0f, &left, &right);
    CHECK(Near(left, 0.0625f) && Near(right, 0.0625f));
    GainsOf3D(far, front, top, 10.0f, 40.0f, 1.0f, &left, &right);
    CHECK(left == 0.0f && right == 0.0f);
    const float farRight[3] = { 40.0f, 0.0f, 0.0f };
    GainsOf3D(farRight, front, top, 10.0f, 100.0f, 1.0f, &left, &right);
    CHECK(Near(left, 0.0f) && Near(right, 0.25f));
}

/* The mixer: its one CD-audio line, through main.c's mixer walk. */
void TestMixer()
{
    HMIXER mixer = NULL;
    CHECK(audio_mixer_get_num_devs() == 1);
    CHECK(audio_mixer_open(&mixer, 1, 0, 0, 0) != MMSYSERR_NOERROR && mixer == NULL);
    CHECK(audio_mixer_open(&mixer, 0, 0, 0, 0) == MMSYSERR_NOERROR && mixer != NULL);
    MIXERLINEA line;
    memset(&line, 0, sizeof(line));
    line.cbStruct = sizeof(line);
    line.dwSource = 1;
    CHECK(audio_mixer_get_line_info((HMIXEROBJ)mixer, &line, MIXER_GETLINEINFOF_SOURCE) != MMSYSERR_NOERROR);
    CHECK(audio_mixer_get_line_info((HMIXEROBJ)NULL, &line, MIXER_GETLINEINFOF_DESTINATION) ==
          MMSYSERR_INVALHANDLE);
    CHECK(audio_mixer_close(mixer) == MMSYSERR_NOERROR);

    /* Full volume at start, then what the slider sets. */
    float left = -1.0f, right = -1.0f;
    CHECK(ledger_stub_004789a0_MusicMixerVolumeGet(&left, &right) == 1);
    CHECK(left == 1.0f && right == 1.0f);
    music_mixer_volume_set(5.0f / 9.0f, 5.0f / 9.0f);
    CHECK(ledger_stub_004789a0_MusicMixerVolumeGet(&left, &right) == 1);
    CHECK(Near(left, 5.0f / 9.0f, 1.0e-4f) && Near(right, 5.0f / 9.0f, 1.0e-4f));
    music_mixer_volume_set(0.25f, 0.75f);
    CHECK(ledger_stub_004789a0_MusicMixerVolumeGet(&left, &right) == 1);
    CHECK(Near(left, 0.25f, 1.0e-4f) && Near(right, 0.75f, 1.0e-4f));
    music_mixer_volume_set(2.0f, -1.0f); /* clamped by the unit */
    CHECK(ledger_stub_004789a0_MusicMixerVolumeGet(&left, &right) == 1);
    CHECK(left == 1.0f && right == 0.0f);
    music_mixer_volume_set(1.0f, 1.0f);
}

void TestWaveReaders(const std::string& dir)
{
    std::vector<char> samples = SquareU8(11025, 0.05);
    samples.push_back((char)0x80); /* an odd data size */
    std::vector<char> wave = MakeWave(1, 11025, 8, samples);

    /* WaveOpenFile over memory, then WaveStartDataRead and WaveReadFile. */
    HMMIO hmmio = NULL;
    WAVEFORMATEX* format = NULL;
    MMCKINFO riff, data;
    CHECK(sound_wave_memory_format_read(wave.data(), (LONG)wave.size(), &hmmio,
                                        &format, &riff) == 0);
    CHECK(hmmio != NULL && format != NULL);
    if (!hmmio || !format) return;
    CHECK(format->wFormatTag == WAVE_FORMAT_PCM);
    CHECK(format->nChannels == 1);
    CHECK(format->nSamplesPerSec == 11025);
    CHECK(format->wBitsPerSample == 8);
    CHECK(format->cbSize == 0);
    CHECK(sound_wave_data_chunk_open(&hmmio, &data, &riff) == 0);
    CHECK(data.cksize == samples.size());
    std::vector<BYTE> read(samples.size() + 16, 0);
    UINT actual = 0;
    CHECK(sound_wave_data_read(hmmio, (UINT)read.size(), read.data(), &data, &actual) == 0);
    CHECK(actual == samples.size());
    CHECK(memcmp(read.data(), samples.data(), samples.size()) == 0);
    /* Reading again, as a looping stream does: the data chunk anew. */
    CHECK(sound_wave_data_chunk_open(&hmmio, &data, &riff) == 0);
    CHECK(sound_wave_data_read(hmmio, 4, read.data(), &data, &actual) == 0);
    CHECK(actual == 4 && memcmp(read.data(), samples.data(), 4) == 0);
    sound_wave_handle_and_buffer_release(&hmmio, &format);
    CHECK(hmmio == NULL && format == NULL);

    /* Not a wave. */
    std::vector<char> junk(64, 'x');
    CHECK(sound_wave_memory_format_read(junk.data(), (LONG)junk.size(), &hmmio,
                                        &format, &riff) != 0);
    CHECK(hmmio == NULL && format == NULL);

    /* From a file, named with Windows separators and the wrong case, as
     * the units name them. */
    CHECK(WriteFile(dir + "/Synth.WAV", wave));
    std::string name = dir + "\\SYNTH.wav";
    CHECK(sound_wave_file_average_bytes_per_second_read(name.c_str()) == 11025);
    CHECK(sound_wave_file_open_read_chunks(name.c_str(), &hmmio, &format, &riff) == 0);
    if (!hmmio || !format) return;
    CHECK(format->nSamplesPerSec == 11025);
    CHECK(sound_wave_data_chunk_open(&hmmio, &data, &riff) == 0);
    CHECK(data.cksize == samples.size());
    sound_wave_handle_and_buffer_release(&hmmio, &format);
    std::string missing = dir + "/missing.wav";
    CHECK(sound_wave_file_open_read_chunks(missing.c_str(), &hmmio, &format, &riff) != 0);
}

void TestBuffers()
{
    AudioDevice* device = NULL;
    CHECK(audio_create(&device) == AUDIO_OK);
    if (!device) return;

    /* The primary buffer: the device's volume. */
    AudioBuffer* primary = NULL;
    CHECK(audio_create_buffer(device, AUDIO_BUFFER_PRIMARY, 0, NULL, &primary) == AUDIO_OK);
    AudioListener* listener = NULL;
    CHECK(audio_buffer_query_listener(primary, &listener) == AUDIO_OK && listener);
    LONG volume = 1;
    CHECK(audio_buffer_set_volume(primary, -300) == AUDIO_OK);
    CHECK(audio_buffer_get_volume(primary, &volume) == AUDIO_OK && volume == -300);

    /* Formats: 8- and 16-bit PCM only. */
    WAVEFORMATEX format = { WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8, 0 };
    AudioBuffer* buffer = NULL;
    WAVEFORMATEX bad = format;
    bad.wBitsPerSample = 24;
    CHECK(audio_create_buffer(device, 0, 1024, &bad, &buffer) == AUDIO_ERR_BADFORMAT);
    bad = format;
    bad.wFormatTag = 2;
    CHECK(audio_create_buffer(device, 0, 1024, &bad, &buffer) == AUDIO_ERR_BADFORMAT);

    /* A one-shot of 0.1 s: written through Lock, it plays to its end. */
    std::vector<char> square = SquareU8(11025, 0.1);
    CHECK(audio_create_buffer(device, 0, (DWORD)square.size(), &format, &buffer) == AUDIO_OK);
    if (!buffer) return;
    void* audio1 = NULL;
    void* audio2 = NULL;
    DWORD bytes1 = 0, bytes2 = 0;
    CHECK(audio_buffer_lock(buffer, 0, (DWORD)square.size(), &audio1, &bytes1, &audio2, &bytes2, 0) == AUDIO_OK);
    CHECK(bytes1 == square.size() && audio2 == NULL && bytes2 == 0);
    memcpy(audio1, square.data(), bytes1);
    CHECK(audio_buffer_unlock(buffer, audio1, bytes1, audio2, bytes2) == AUDIO_OK);
    DWORD frequency = 0;
    CHECK(audio_buffer_get_frequency(buffer, &frequency) == AUDIO_OK && frequency == 11025);
    /* A lock across the end wraps to the start. */
    CHECK(audio_buffer_lock(buffer, (DWORD)square.size() - 10, 30, &audio1, &bytes1, &audio2, &bytes2, 0) == AUDIO_OK);
    CHECK(bytes1 == 10 && audio2 != NULL && bytes2 == 20);
    audio_buffer_unlock(buffer, audio1, bytes1, audio2, bytes2);

    DWORD status = 0;
    CHECK(audio_buffer_get_status(buffer, &status) == AUDIO_OK && status == 0);
    CHECK(audio_buffer_set_volume(buffer, -600) == AUDIO_OK);
    CHECK(audio_buffer_get_volume(buffer, &volume) == AUDIO_OK && volume == -600);
    CHECK(audio_buffer_set_volume(buffer, 1) == AUDIO_ERR_INVALIDPARAM);
    CHECK(audio_buffer_set_current_position(buffer, 0) == AUDIO_OK);
    CHECK(audio_buffer_play(buffer, 0, 0, 0) == AUDIO_OK);
    CHECK(audio_buffer_get_status(buffer, &status) == AUDIO_OK && (status & AUDIO_STATUS_PLAYING));
    bool ended = WaitFor([&] {
        audio_buffer_get_status(buffer, &status);
        return status == 0;
    }, 3000);
    CHECK(ended);

    /* Looping: still playing after several lengths, the cursor wrapping;
     * Stop ends it at once. */
    CHECK(audio_buffer_set_current_position(buffer, 0) == AUDIO_OK);
    CHECK(audio_buffer_play(buffer, 0, 0, AUDIO_PLAY_LOOPING) == AUDIO_OK);
    DWORD play = 0, write = 0, lastPlay = 0;
    int moves = 0, wraps = 0;
    Uint64 until = SDL_GetTicks() + 450;
    while (SDL_GetTicks() < until) {
        CHECK(audio_buffer_get_current_position(buffer, &play, &write) == AUDIO_OK);
        CHECK(play < square.size() && write < square.size());
        if (play != lastPlay) moves++;
        if (play < lastPlay) wraps++;
        lastPlay = play;
        SDL_Delay(10);
    }
    CHECK(moves > 3);
    CHECK(wraps >= 2);
    CHECK(audio_buffer_get_status(buffer, &status) == AUDIO_OK &&
          status == (AUDIO_STATUS_PLAYING | AUDIO_STATUS_LOOPING));
    CHECK(audio_buffer_stop(buffer) == AUDIO_OK);
    CHECK(audio_buffer_get_status(buffer, &status) == AUDIO_OK && status == 0);
    SDL_Delay(50);
    CHECK(audio_buffer_get_status(buffer, &status) == AUDIO_OK && status == 0);

    /* A duplicate shares the data and plays on its own. */
    AudioBuffer* duplicate = NULL;
    CHECK(audio_duplicate_buffer(device, buffer, &duplicate) == AUDIO_OK && duplicate);
    if (duplicate) {
        CHECK(audio_buffer_lock(duplicate, 0, 4, &audio1, &bytes1, &audio2, &bytes2, 0) == AUDIO_OK);
        CHECK(bytes1 == 4 && memcmp(audio1, square.data(), 4) == 0);
        audio_buffer_unlock(duplicate, audio1, bytes1, audio2, bytes2);
        CHECK(audio_buffer_play(duplicate, 0, 0, 0) == AUDIO_OK);
        CHECK(audio_buffer_get_status(buffer, &status) == AUDIO_OK && status == 0);
        CHECK(audio_buffer_release(duplicate) == 0);
    }

    /* The 3D view: a reference of its own, silent beyond max distance. */
    AudioBuffer* view = NULL;
    CHECK(audio_buffer_query_3d(buffer, &view) == AUDIO_OK && view == buffer);
    CHECK(audio_buffer_set_max_distance(view, 10.0f, 0) == AUDIO_OK);
    CHECK(audio_buffer_set_3d_position(view, 100.0f, 0.0f, 0.0f, 0) == AUDIO_OK);
    CHECK(audio_listener_commit_deferred_settings(listener) == AUDIO_OK);
    CHECK(audio_buffer_release(view) == 1);

    /* 16-bit stereo. */
    WAVEFORMATEX stereo = { WAVE_FORMAT_PCM, 2, 22050, 88200, 4, 16, 0 };
    AudioBuffer* buffer16 = NULL;
    CHECK(audio_create_buffer(device, 0, 4410, &stereo, &buffer16) == AUDIO_OK && buffer16);
    if (buffer16) {
        CHECK(audio_buffer_play(buffer16, 0, 0, 0) == AUDIO_OK);
        CHECK(WaitFor([&] {
            audio_buffer_get_status(buffer16, &status);
            return status == 0;
        }, 3000));
        CHECK(audio_buffer_release(buffer16) == 0);
    }

    CHECK(audio_buffer_release(buffer) == 0);
    CHECK(audio_buffer_release(primary) == 0);
    audio_release(device);
}

void TestCdAudio(const std::string& dir)
{
    char result[200];
    CHECK(chdir(dir.c_str()) == 0);

    /* No track files: no disc, and the unit's play fails quietly. */
    CHECK(audio_mci_send_string("open cdaudio", result, sizeof(result), NULL) != 0);
    CHECK(audio_mci_send_string("status cdaudio mode", result, sizeof(result), NULL) != 0);
    CHECK(media_cd_audio_device_open(1) == 0);
    CHECK(audio_mci_get_error_string(276, result, sizeof(result)) && result[0] != '\0');

    /* Track 2 of the disc, named in another case; track 3 is missing. */
    CHECK(SDL_CreateDirectory((dir + "/music").c_str()));
    CHECK(WriteFile(dir + "/music/track02.WAV", MakeWave(2, 22050, 16, SineS16(22050, 0.2))));
    CHECK(audio_mci_send_string("open cdaudio", result, sizeof(result), NULL) == 0);
    CHECK(audio_mci_send_string("set cdaudio time format tmsf", result, sizeof(result), NULL) == 0);
    CHECK(audio_mci_send_string("play cdaudio from 5", result, sizeof(result), NULL) != 0);
    CHECK(audio_mci_send_string("play cdaudio from 2", result, sizeof(result), NULL) == 0);
    CHECK(audio_mci_send_string("status cdaudio mode", result, sizeof(result), NULL) == 0);
    CHECK(strcmp(result, "playing") == 0);
    CHECK(audio_mci_send_string("status cdaudio current track", result, sizeof(result), NULL) == 0);
    CHECK(strcmp(result, "2") == 0);
    CHECK(WaitFor([&] {
        audio_mci_send_string("status cdaudio mode", result, sizeof(result), NULL);
        return strcmp(result, "stopped") == 0;
    }, 3000));
    CHECK(audio_mci_send_string("status cdaudio current track", result, sizeof(result), NULL) == 0);
    CHECK(strcmp(result, "3") == 0);
    CHECK(audio_mci_send_string("stop cdaudio", result, sizeof(result), NULL) == 0);
    CHECK(audio_mci_send_string("close cdaudio", result, sizeof(result), NULL) == 0);

    /* Through soundsys.c: Play_CDTrack plays the 0-based track 1 (disc
     * track 2) and Status_CDTrack reports it until it ends. */
    CHECK(media_cd_audio_device_open(1) == 1);
    CHECK(media_cd_audio_track_completion_poll(1) == 1);
    CHECK(WaitFor([] { return media_cd_audio_track_completion_poll(1) == 0; }, 3000));
    CHECK(media_cd_audio_stop_and_close() == 1);
}

int RunDummy()
{
    std::string dir = MakeTempDir();
    if (dir.empty()) return 1;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "skip: %s\n", SDL_GetError());
        return 77;
    }
    TestArithmetic();
    TestMixer();
    TestWaveReaders(dir);
    TestBuffers();
    /* Sound_Initialise starts 3D sound on the library's device. */
    CHECK(media_cd_audio_device_state_update(0) == 1);
    CHECK(media_cd_audio_playback_available() == 1);
    TestCdAudio(dir);
    audio_active_shutdown();
    CHECK(media_cd_audio_playback_available() == 0);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return s_failures ? 1 : 0;
}

/* --- disk: the mix -------------------------------------------------------- */

/* A run of sound: the left channel's peak, mean and RMS, and the right
 * channel's RMS. */
struct Segment {
    size_t start, end; /* frames */
    float peak;
    double mean;
    double rms;
    double rmsRight;
};

/* The runs of sound on either channel of a stereo float mix, split at
 * 50 ms of silence. */
std::vector<Segment> FindSegments(const std::vector<float>& mix, int rate)
{
    std::vector<Segment> segments;
    size_t frames = mix.size() / 2;
    size_t gap = (size_t)rate / 20;
    size_t quiet = gap;
    for (size_t i = 0; i < frames; i++) {
        float value = std::max(fabsf(mix[2 * i]), fabsf(mix[2 * i + 1]));
        if (value > 0.01f) {
            if (quiet >= gap) segments.push_back({ i, i, 0.0f, 0.0, 0.0, 0.0 });
            Segment& segment = segments.back();
            segment.end = i;
            quiet = 0;
        } else {
            quiet++;
        }
    }
    for (Segment& segment : segments) {
        double sum = 0.0, squares = 0.0, squaresRight = 0.0;
        for (size_t i = segment.start; i <= segment.end; i++) {
            segment.peak = std::max(segment.peak, fabsf(mix[2 * i]));
            sum += mix[2 * i];
            squares += (double)mix[2 * i] * mix[2 * i];
            squaresRight += (double)mix[2 * i + 1] * mix[2 * i + 1];
        }
        double count = (double)(segment.end - segment.start + 1);
        segment.mean = sum / count;
        segment.rms = sqrt(squares / count);
        segment.rmsRight = sqrt(squaresRight / count);
    }
    return segments;
}

int RunDisk()
{
    std::string dir = MakeTempDir();
    if (dir.empty()) return 1;
    std::string output = dir + "/mix.raw";
    const int rate = 44100;
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "disk");
    SDL_SetHint(SDL_HINT_AUDIO_DISK_OUTPUT_FILE, output.c_str());
    SDL_SetHint(SDL_HINT_AUDIO_FORMAT, "F32LE");
    SDL_SetHint(SDL_HINT_AUDIO_CHANNELS, "2");
    SDL_SetHint(SDL_HINT_AUDIO_FREQUENCY, "44100");
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "skip: no disk audio driver: %s\n", SDL_GetError());
        return 77;
    }
    CHECK(chdir(dir.c_str()) == 0);
    CHECK(media_cd_audio_device_state_update(0) == 1);
    /* The overlays' streamed sample plays at the primary buffer's volume,
     * which Sound3D_Initialise lowers to -300; play it at full volume. */
    audio_sound_volume_set(0);

    /* A streamed wave longer than the 3 s ring, as sound3d.c streams the
     * menu overlays' sounds, ticked as the game's frame loop ticks it. */
    const double streamSeconds = 3.5;
    CHECK(WriteFile(dir + "/Stream.wav", MakeWave(1, 11025, 8, SquareU8(11025, streamSeconds))));
    std::string stream = dir + "\\stream.WAV";
    CHECK(sound_streamed_wave_play(stream.c_str(), 0, 0) == 1);
    Uint64 until = SDL_GetTicks() + (Uint64)(streamSeconds * 1000) + 1200;
    while (SDL_GetTicks() < until) {
        sound_source_spatial_refresh_all();
        SDL_Delay(15);
    }
    sound_stream_stop_and_release(0);

    /* Then the disc's track 2 through soundsys.c, at full music volume and
     * at half. */
    CHECK(SDL_CreateDirectory((dir + "/Music").c_str()));
    CHECK(WriteFile(dir + "/Music/Track02.wav", MakeWave(2, 22050, 16, SineS16(22050, 0.5))));
    CHECK(media_cd_audio_device_open(1) == 1);
    SDL_Delay(900);
    CHECK(media_cd_audio_stop_and_close() == 1);
    music_mixer_volume_set(0.5f, 0.5f);
    CHECK(media_cd_audio_device_open(1) == 1);
    SDL_Delay(900);
    CHECK(media_cd_audio_stop_and_close() == 1);
    music_mixer_volume_set(1.0f, 1.0f);

    /* A short streamed wave at the SFX slider's sixth step (-10 dB). */
    audio_sound_volume_index_set(5);
    const double shortSeconds = 0.4;
    CHECK(WriteFile(dir + "/Short.wav", MakeWave(1, 11025, 8, SquareU8(11025, shortSeconds))));
    std::string shortStream = dir + "/Short.wav";
    CHECK(sound_streamed_wave_play(shortStream.c_str(), 0, 0) == 1);
    until = SDL_GetTicks() + (Uint64)(shortSeconds * 1000) + 600;
    while (SDL_GetTicks() < until) {
        sound_source_spatial_refresh_all();
        SDL_Delay(15);
    }
    sound_stream_stop_and_release(0);
    audio_sound_volume_set(0);

    /* A mono 3D buffer on a device of the test's own, inside its minimum
     * distance unless a case says otherwise. */
    AudioDevice* device = NULL;
    AudioBuffer* primary = NULL;
    AudioListener* listener = NULL;
    AudioBuffer* buffer = NULL;
    AudioBuffer* view = NULL;
    std::vector<char> sine = SineS16(22050, 0.3, 1);
    WAVEFORMATEX mono = { WAVE_FORMAT_PCM, 1, 22050, 44100, 2, 16, 0 };
    CHECK(audio_create(&device) == AUDIO_OK);
    CHECK(audio_create_buffer(device, AUDIO_BUFFER_PRIMARY, 0, NULL, &primary) == AUDIO_OK);
    CHECK(audio_buffer_query_listener(primary, &listener) == AUDIO_OK);
    CHECK(audio_create_buffer(device, 0, (DWORD)sine.size(), &mono, &buffer) == AUDIO_OK);
    if (buffer) {
        void* audio1;
        DWORD bytes1;
        CHECK(audio_buffer_lock(buffer, 0, (DWORD)sine.size(), &audio1, &bytes1, NULL, NULL, 0) == AUDIO_OK);
        memcpy(audio1, sine.data(), bytes1);
        audio_buffer_unlock(buffer, audio1, bytes1, NULL, 0);
        CHECK(audio_buffer_query_3d(buffer, &view) == AUDIO_OK);
        audio_buffer_set_min_distance(view, 20.0f, 0);
        audio_buffer_set_max_distance(view, 800.0f, 0);
        auto play = [&](float x, float y, float z) {
            DWORD status = 0;
            audio_buffer_set_3d_position(view, x, y, z, 0);
            audio_listener_commit_deferred_settings(listener);
            audio_buffer_set_current_position(buffer, 0);
            audio_buffer_play(buffer, 0, 0, 0);
            CHECK(WaitFor([&] {
                audio_buffer_get_status(buffer, &status);
                return status == 0;
            }, 3000));
            SDL_Delay(150);
        };
        play(10.0f, 0.0f, 0.0f);   /* right */
        play(-10.0f, 0.0f, 0.0f);  /* left */
        /* Facing +x, the listener's right is -z: +z is on its left, and
         * at 40 its gain is 20 / 40. */
        audio_listener_set_orientation(listener, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0);
        play(0.0f, 0.0f, 40.0f);
        audio_listener_set_orientation(listener, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0);
        play(10.0f, 0.0f, 10.0f);  /* 45 degrees right */
        audio_buffer_release(view);
        audio_buffer_release(buffer);
    }
    audio_buffer_release(primary);
    audio_release(device);
    /* The disk device's format, which the hints ask for but need not
     * get. */
    SDL_AudioSpec spec;
    int sampleFrames = 0;
    CHECK(SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &sampleFrames));
    audio_active_shutdown();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    if (spec.channels != 2 || spec.freq != rate ||
        (spec.format != SDL_AUDIO_F32 && spec.format != SDL_AUDIO_S16)) {
        fprintf(stderr, "skip: disk format %#x, %d channels, %d Hz\n", (unsigned)spec.format,
                spec.channels, spec.freq);
        return 77;
    }

    size_t length = 0;
    void* data = SDL_LoadFile(output.c_str(), &length);
    CHECK(data != NULL && length > 0);
    if (!data) return 1;
    std::vector<float> mix;
    if (spec.format == SDL_AUDIO_F32) {
        const float* samples = static_cast<const float*>(data);
        mix.assign(samples, samples + length / sizeof(float));
    } else {
        const Sint16* samples = static_cast<const Sint16*>(data);
        for (size_t i = 0; i < length / sizeof(Sint16); i++) mix.push_back(samples[i] / 32768.0f);
    }
    SDL_free(data);
    std::vector<Segment> segments = FindSegments(mix, rate);
    for (const Segment& segment : segments) {
        printf("sound %.3f-%.3f s, peak %.3f, rms %.3f, mean %.4f\n", (double)segment.start / rate,
               (double)segment.end / rate, segment.peak, segment.rms, segment.mean);
    }
    CHECK(segments.size() == 8);
    if (segments.size() == 8) {
        /* The unsigned 8-bit square at +-0.75 (RMS 0.75; read as signed
         * it would be +-0.25), unbroken across the ring's refills and
         * centred on zero. */
        double seconds = (double)(segments[0].end - segments[0].start) / rate;
        CHECK(fabs(seconds - streamSeconds) < 0.25);
        CHECK(segments[0].rms > 0.65 && segments[0].rms < 0.85);
        CHECK(fabs(segments[0].mean) < 0.05);
        /* The 16-bit stereo sine at 0.5 (RMS 0.354). */
        seconds = (double)(segments[1].end - segments[1].start) / rate;
        CHECK(fabs(seconds - 0.5) < 0.1);
        CHECK(segments[1].peak > 0.4f && segments[1].peak < 0.6f);
        CHECK(segments[1].rms > 0.3 && segments[1].rms < 0.4);
        CHECK(fabs(segments[1].rmsRight - segments[1].rms) < 0.01);
        /* At half music volume (RMS 0.177). */
        seconds = (double)(segments[2].end - segments[2].start) / rate;
        CHECK(fabs(seconds - 0.5) < 0.1);
        CHECK(segments[2].peak > 0.2f && segments[2].peak < 0.3f);
        CHECK(segments[2].rms > 0.15 && segments[2].rms < 0.2);
        /* The square at -10 dB (RMS 0.237). */
        seconds = (double)(segments[3].end - segments[3].start) / rate;
        CHECK(fabs(seconds - shortSeconds) < 0.1);
        CHECK(segments[3].rms > 0.2 && segments[3].rms < 0.27);
        /* The mono sine at 0.5 (RMS 0.354): hard right, hard left, left
         * at half, and right with the left at 1 - sin 45 (RMS 0.104). */
        CHECK(segments[4].rms < 0.01 && segments[4].rmsRight > 0.3 && segments[4].rmsRight < 0.4);
        CHECK(segments[5].rmsRight < 0.01 && segments[5].rms > 0.3 && segments[5].rms < 0.4);
        CHECK(segments[6].rmsRight < 0.01 && segments[6].rms > 0.15 && segments[6].rms < 0.2);
        CHECK(segments[7].rmsRight > 0.3 && segments[7].rmsRight < 0.4);
        CHECK(segments[7].rms > 0.08 && segments[7].rms < 0.13);
    }
    return s_failures ? 1 : 0;
}

} // namespace

int main(int argc, char** argv)
{
    std::string mode = argc > 1 ? argv[1] : "dummy";
    int result = mode == "disk" ? RunDisk() : RunDummy();
    if (result == 0) printf("audio %s: ok\n", mode.c_str());
    return result;
}
