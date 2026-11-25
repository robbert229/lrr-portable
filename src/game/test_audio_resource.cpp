// SPDX-License-Identifier: LGPL-3.0-or-later
/* Independent sample-bank and SFX resource lifecycles using only generated PCM. */
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "platform/native/test_fixture.h"
#include "game/sfx_link_names.h"
#include "game/sfx.h"
#include "gods98/audio.h"
#include "gods98/config.h"
#include "gods98/files.h"
#include "gods98/sound3d.h"
#include "library/audio_sdl.h"
#include "test/lrr_test.h"
#include <miniwin/d3drm.h>

extern "C" AudioBuffer* sound_handle_buffer_get(int handle);
extern "C" int sound_streamed_wave_play(const char* filename, int loop, int volume);
extern "C" void sound_tracked_buffers_set_max_distance(float value);
extern "C" void simtick_directsound_listener_view_set(IDirect3DRMFrame3*);
extern "C" void Container_Frame_RemoveAppData(IDirect3DRMFrame3*);

namespace {
constexpr unsigned rate = 8000;
constexpr unsigned frames = 800;
bool completed;

void exit_guard()
{
    if (!completed) {
        std::fputs("FAIL: resource test exited prematurely\n", stderr);
        std::fflush(stderr);
        std::_Exit(1);
    }
}

void le16(std::vector<unsigned char>& out, uint16_t value)
{
    out.push_back(static_cast<unsigned char>(value));
    out.push_back(static_cast<unsigned char>(value >> 8));
}

void le32(std::vector<unsigned char>& out, uint32_t value)
{
    le16(out, static_cast<uint16_t>(value));
    le16(out, static_cast<uint16_t>(value >> 16));
}

void tag(std::vector<unsigned char>& out, const char* text)
{
    out.insert(out.end(), text, text + 4);
}

bool write_wave(const char* path, bool tone)
{
    // 100 ms, mono PCM; either unsigned 8-bit silence or a quiet 16-bit square wave.
    const unsigned bytes = tone ? 2 : 1;
    std::vector<unsigned char> wave;
    tag(wave, "RIFF");
    le32(wave, 36 + frames * bytes);
    tag(wave, "WAVE");
    tag(wave, "fmt ");
    le32(wave, 16);
    le16(wave, 1);
    le16(wave, 1);
    le32(wave, rate);
    le32(wave, rate * bytes);
    le16(wave, bytes);
    le16(wave, bytes * 8);
    tag(wave, "data");
    le32(wave, frames * bytes);
    for (unsigned i = 0; i < frames; ++i) {
        if (tone) le16(wave, static_cast<uint16_t>(i % 40 < 20 ? 1024 : -1024));
        else wave.push_back(128);
    }
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(wave.data()), wave.size());
    file.close();
    return file.good();
}

bool setup(const std::string& scenario)
{
    if (!CHECK(fixture_enter(("audio_resource_" + scenario).c_str()) &&
               fixture_mkdir("Data"), "isolated data directory")) return false;
    if (!CHECK(write_wave("Data/silence.wav", false) && write_wave("Data/tone.wav", true),
               "write original PCM fixtures")) return false;
    // A zero-entry archive ensures the file mount needs no external media.
    std::ofstream wad("AudioFixture0.wad", std::ios::binary);
    const char empty[] = {'W', 'W', 'A', 'D', 0, 0, 0, 0};
    wad.write(empty, sizeof(empty));
    wad.close();
    if (!CHECK(wad.good(), "write empty archive")) return false;
    file_initialise("AudioFixture", 0, "");
    if (!CHECK(SDL_Init(SDL_INIT_AUDIO), SDL_GetError())) return false;
    CHECK_EQ(audio_system_init(nullptr), 1);
    return CHECK(s_bank_sound_on, "dummy audio device initialized");
}

void check_buffer(int handle, bool playing)
{
    AudioBuffer* buffer = sound_handle_buffer_get(handle);
    if (!CHECK(buffer != nullptr, "sample owns a real audio buffer")) return;
    DWORD status = 0, frequency = 0;
    CHECK_EQ(audio_buffer_get_status(buffer, &status), AUDIO_OK);
    CHECK_EQ((status & AUDIO_STATUS_PLAYING) != 0, playing);
    CHECK_EQ(audio_buffer_get_frequency(buffer, &frequency), AUDIO_OK);
    CHECK_EQ(frequency, rate);
}

void check_attached_sample(int slot)
{
    IDirect3DRM* rm = nullptr;
    IDirect3DRM3* rm3 = nullptr;
    IDirect3DRMFrame3* root = nullptr;
    IDirect3DRMFrame3* source = nullptr;
    if (CHECK_EQ(Direct3DRMCreate(&rm), DD_OK) &&
        CHECK_EQ(rm->QueryInterface(IID_IDirect3DRM3, reinterpret_cast<void**>(&rm3)), DD_OK) &&
        CHECK_EQ(rm3->CreateFrame(nullptr, &root), DD_OK) &&
        CHECK_EQ(rm3->CreateFrame(root, &source), DD_OK)) {
        simtick_directsound_listener_view_set(source);
        CHECK_EQ(source->SetPosition(root, 2.0f, 3.0f, 4.0f), DD_OK);
        simtick_sound_keepalive();
        CHECK_NEAR(sound3dState.frames.oldPos.x, 2.0f, 0.00001f);
        CHECK_EQ(s_play_seam(0, reinterpret_cast<intptr_t>(source), slot, 1, nullptr), slot * 3);
        CHECK_EQ(sound3dState.frames.updateFrameList[0], source);
        CHECK_EQ(source->SetPosition(root, 4.0f, 5.0f, 6.0f), DD_OK);
        sound_source_spatial_refresh_all();
        sound_tracked_buffers_set_max_distance(100.0f);
        sound_buffer_stop_handle(slot * 3);
        sound_source_spatial_refresh_all();
        for (auto* frame : sound3dState.frames.updateFrameList) CHECK_EQ(frame, nullptr);
        check_buffer(slot * 3, false);
        simtick_directsound_listener_view_set(nullptr);
        Container_Frame_RemoveAppData(source);
    }
    if (source) source->Release();
    if (root) root->Release();
    if (rm3) rm3->Release();
    if (rm) rm->Release();
}

void check_bank()
{
    CHECK_EQ(sound_bank_entry_load("missing", 0, 0, 0), -1);
    CHECK_EQ(sound_bank_entry_load("missing", 1, 0, 0), -1);
    const int single = sound_bank_entry_load("silence", 0, 0, -450);
    const int multi = sound_bank_entry_load("tone", 0, 1, -10001);
    if (!CHECK_EQ(single, 0) || !CHECK_EQ(multi, 1)) return;
    CHECK_NEAR(sound_streamed_wave_buffer_step_rate(single), 0.1f, 0.00001f);
    CHECK_NEAR(sound_streamed_wave_buffer_step_rate(multi), 0.1f, 0.00001f);
    CHECK_EQ(sound_bank_entry_volume(single), -450);
    CHECK_EQ(sound_bank_entry_volume(multi), 0);
    CHECK_EQ(sound_handle_buffer_get(single * 3 + 1), nullptr);
    for (int voice = 0; voice < 3; ++voice) check_buffer(multi * 3 + voice, false);
    sound_bank_entry_volume_set(multi, -725);
    for (int voice = 0; voice < 3; ++voice) {
        LONG volume = 0;
        CHECK_EQ(audio_buffer_get_volume(sound_handle_buffer_get(multi * 3 + voice), &volume), AUDIO_OK);
        CHECK_EQ(volume, -725);
    }

    const float position[] = {2.0f, 3.0f, 4.0f};
    for (int voice : {1, 2, 0, 1}) {
        const int handle = s_play_seam(1, 0, multi, 1, position);
        CHECK_EQ(handle, multi * 3 + voice);
        if (handle >= 0) check_buffer(handle, true);
    }
    CHECK_EQ(s_play_seam(2, 0, single, 1, nullptr), single * 3);
    sound_tracked_buffers_set_min_distance(2.0f);
    sound_source_spatial_refresh_all();
    sound_subsystem_stop_all();
    sound_source_spatial_refresh_all();
    check_buffer(single * 3, false);
    for (int voice = 0; voice < 3; ++voice) check_buffer(multi * 3 + voice, false);
    check_attached_sample(single);
    CHECK_EQ(sound_bank_entry_release(single), 1);
    CHECK_EQ(sound_bank_entry_release(multi), 1);
    for (int voice = 0; voice < 3; ++voice) CHECK_EQ(sound_handle_buffer_get(multi * 3 + voice), nullptr);
    const int reused = sound_bank_entry_load("tone", 0, 0, 0);
    CHECK_EQ(reused, single);
    if (reused >= 0) CHECK_EQ(sound_bank_entry_release(reused), 1);
}

void check_capacity()
{
    // The documented bank has 600 slots. Use stream metadata so this
    // checks admission and release without opening hundreds of devices/buffers.
    std::vector<int> slots;
    for (int i = 0; i < 600; ++i) {
        const int slot = sound_bank_entry_load("silence", 1, 0, 0);
        if (!CHECK_EQ(slot, i)) break;
        slots.push_back(slot);
    }
    if (CHECK_EQ(slots.size(), 600u)) {
        CHECK_EQ(sound_bank_entry_load("tone", 0, 0, 0), -1);
        CHECK_EQ(sound_bank_entry_release(slots[300]), 1);
        const int reopened = sound_bank_entry_load("tone", 0, 0, 0);
        CHECK_EQ(reopened, slots[300]);
        if (reopened >= 0) check_buffer(reopened * 3, false);
    }
    for (int slot : slots) CHECK_EQ(sound_bank_entry_release(slot), 1);
    const int first = sound_bank_entry_load("silence", 0, 0, 0);
    CHECK_EQ(first, 0);
    if (first >= 0) CHECK_EQ(sound_bank_entry_release(first), 1);
}

void check_streams()
{
    const int a = sound_bank_entry_load("silence", 1, 0, -300);
    const int b = sound_bank_entry_load("tone", 1, 1, -600);
    if (!CHECK_EQ(a, 0) || !CHECK_EQ(b, 1)) return;
    // The stream bank uses whole-file length, rather than just PCM bytes.
    CHECK_NEAR(sound_streamed_wave_buffer_step_rate(a), (44.0f + frames) / rate, 0.00001f);
    CHECK_NEAR(sound_streamed_wave_buffer_step_rate(b), (44.0f + frames * 2) / (rate * 2), 0.00001f);
    CHECK_EQ(s_play_seam(2, 0, a, 0, nullptr), a * 3);
    CHECK_EQ(s_play_seam(2, 0, b, 1, nullptr), b * 3);
    // Replace an active stream, then stop each independent channel.
    CHECK_EQ(s_play_seam(2, 0, b, 0, nullptr), b * 3);
    sound_source_spatial_refresh_all();
    CHECK_EQ(sound_stream_stop_and_release(0), 1);
    CHECK_EQ(sound_stream_stop_and_release(1), 1);
    CHECK_EQ(sound_stream_stop_and_release(1), 1);
    fixture_remove_all("Data/tone.wav");
    CHECK(!fixture_exists("Data/tone.wav"), "stream released its file");
    CHECK_EQ(s_play_seam(2, 0, b, 1, nullptr), -1);
    CHECK_EQ(sound_streamed_wave_play("Data/tone.wav", 0, 0), 0);
    CHECK(write_wave("Data/tone.wav", true), "restore generated stream");
    CHECK_EQ(s_play_seam(2, 0, b, 1, nullptr), b * 3);
    sound_subsystem_stop_all();
    CHECK_EQ(sound_bank_entry_release(a), 1);
    CHECK_EQ(sound_bank_entry_release(b), 1);
}

int load_alias(BlockFile* config, const char* label, const char* path, bool expected)
{
    int alias = -1;
    CHECK_EQ(sfx_get_type(label, &alias), 1);
    char* value = block_get_str_copy(config, path);
    if (!CHECK(value != nullptr, "read synthetic sample property")) return -1;
    CHECK_EQ(sfx_load_sample_property(value, alias), expected ? 1 : 0);
    std::free(value);
    return alias;
}

void check_aliases()
{
    std::ofstream config("Data/audio.cfg");
    config << "Samples { Plain silence Poly *tone Stream @silence Absent missing }\n";
    config.close();
    if (!CHECK(config.good(), "write abstract sample records")) return;
    block_text_token_pool_init();
    BlockFile* records = block_open("audio.cfg");
    if (!CHECK(records != nullptr, "load sample configuration")) return;
    // Slot zero is valid in the bank but is treated as a sentinel by the SFX
    // playback wrappers. Reserve it with a real sample, not a stub.
    const int reserve = sound_bank_entry_load("silence", 0, 0, 0);
    CHECK_EQ(reserve, 0);
    sfx_initialise();
    sfx_set_sample_populate_mode(1);
    const int plain = load_alias(records, "SyntheticPlain", "Samples::Plain", true);
    const int poly = load_alias(records, "SyntheticPoly", "Samples::Poly", true);
    const int stream = load_alias(records, "SyntheticStream", "Samples::Stream", true);
    const int absent = load_alias(records, "SyntheticAbsent", "Samples::Absent", false);
    sfx_set_sample_populate_mode(0);
    block_close(records);
    block_text_token_pool_destroy();
    if (!CHECK(plain >= 0 && poly >= 0 && stream >= 0 && absent >= 0, "registered labels")) return;
    CHECK_EQ(sfx_random_get_sound3d_handle(absent), -1);
    const int plainSlot = sfx_random_get_sound3d_handle(plain);
    const int polySlot = sfx_random_get_sound3d_handle(poly);
    const int streamSlot = sfx_random_get_sound3d_handle(stream);
    if (!CHECK_EQ(plainSlot, 1) || !CHECK_EQ(polySlot, 2) || !CHECK_EQ(streamSlot, 3)) return;
    int lookup = -1;
    CHECK_EQ(sfx_get_type("syntheticpoly", &lookup), 1);
    CHECK_EQ(lookup, poly);
    CHECK_EQ(sfx_get_type("Unregistered", &lookup), 0);
    CHECK_NEAR(sfx_random_get_sample_play_time(plain), 0.1f, 0.00001f);
    sfx_random_set_buffer_volume(poly, -900);
    CHECK_EQ(sfx_random_get_buffer_volume(poly), -900);
    sfx_set_sound_on(1, 0);
    int handle = -1;
    CHECK_EQ(sfx_random_set_and_play_global_sample(plain, &handle), 1);
    CHECK_EQ(handle, plainSlot * 3);
    CHECK_EQ(sfx_random_set_and_play_global_sample(poly, nullptr), 0);
    sfx_stop_global_sample();
    sfx_sound3d_update();
    float pos[] = {1.0f, 2.0f, 3.0f};
    sfx_set_queue_mode(1, 0);
    sfx_random_play_sound3d_on_frame_internal(nullptr, poly, 1, 0, pos);
    for (int voice = 0; voice < 3; ++voice) check_buffer(polySlot * 3 + voice, false);
    sfx_set_queue_mode(0, 1);
    check_buffer(polySlot * 3 + 1, true);
    CHECK_EQ(sfx_random_play_sound_normal(stream, 1), streamSlot * 3);
    sfx_set_sound_on_and_stop_all(0);
    CHECK_EQ(sfx_is_sound_on(), 0);
    sfx_sound3d_update();
    check_buffer(polySlot * 3 + 1, false);
    for (int slot : {reserve, plainSlot, polySlot, streamSlot})
        CHECK_EQ(sound_bank_entry_release(slot), 1);
}
} // namespace

int main(int argc, char** argv)
{
    const std::string scenario = argc > 1 ? argv[1] : "bank";
    if (!CHECK(scenario == "bank" || scenario == "streams" || scenario == "aliases" || scenario == "capacity",
               "known scenario")) return lrr_test_result();
    std::atexit(exit_guard);
    if (setup(scenario)) {
        if (scenario == "bank") check_bank();
        else if (scenario == "streams") check_streams();
        else if (scenario == "aliases") check_aliases();
        else check_capacity();
        audio_active_shutdown();
        CHECK_EQ(s_bank_sound_on, 0);
        CHECK_EQ(sound_bank_entry_load("silence", 0, 0, 0), -1);
        SDL_Quit();
    }
    completed = true;
    return lrr_test_result();
}
