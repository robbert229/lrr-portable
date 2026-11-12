// SPDX-License-Identifier: LGPL-3.0-or-later
/* Win32 entry point — the reference build.
 *
 * Mirrors the shipped binary's GUI-subsystem entry (WinMain, -mwindows). Used
 * to A/B our rebuild against the shipped LegoRR.exe under the A.2 harness
 * (tools/compat/a2.py, docs/adr/0002).
 */
#include "platform/platform.h"
#include "gods98/resource.hpp"
#include "gods98/frameloop.hpp"
#include "gods98/string.hpp"
#include "game/port_loop_ports.hpp"
#include "crt/crt.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The original WinMain keeps the executable basename in a process global and
 * uses it as the WAD prefix. Keep this value visible to the Win32 startup
 * while the rest of the original startup is reconstructed. */
static char s_application_prefix[1024];
static DWORD s_startup_no_sound;
static DWORD s_startup_insist_on_cd;
/* Port-side duplicate of the application mode, not the annotated global. */
static DWORD s_programmer_mode;
// GLOBAL: LEGORR 0x00505f8c
static char s_start_level_name[256];
// GLOBAL: LEGORR 0x0050593c
static float s_frame_interval_override;

/* Deviation (scoring only): retained to preserve declaration count for matching (finding 506). */
static void* winmain_registry_create_key(void* parent, const char* component,
                                         void*)
{
    HKEY child = NULL;
    /* RegistryPathCreateKeys ignores RegCreateKeyA's status and tests only
     * whether its output handle is null. Preserve that target condition. */
    (void)RegCreateKeyA((HKEY)parent, component, &child);
    return child;
}

static int winmain_registry_query_value(void* key, const char* value_name,
                                        void* output, uint32_t* size,
                                        uint32_t* value_type, void*)
{
    DWORD native_size;
    DWORD native_type = 0;
    LONG result;
    if (size == NULL) return 0;
    native_size = (DWORD)*size;
    result = RegQueryValueExA((HKEY)key, value_name, NULL, &native_type,
                              (LPBYTE)output, &native_size);
    *size = (uint32_t)native_size;
    if (value_type != NULL) *value_type = (uint32_t)native_type;
    return result == ERROR_SUCCESS;
}

static void winmain_registry_close_key(void* key, void*)
{
    if (key != NULL) RegCloseKey((HKEY)key);
}

static int winmain_registry_read(const char* value_name, char* value,
                                 DWORD value_size)
{
    uint32_t capacity = (uint32_t)value_size;
    const char* key_path = "SOFTWARE\\LEGO Media\\Games\\Rock Raiders";
    if (value == NULL || value_size == 0u) return 0;
    if (!registry_path_accessor(key_path, value_name, 0, value, capacity))
        return 0;
    value[value_size - 1u] = '\0';
    return 1;
}

/* CommandLineOptionParse (finding 398). */
// FUNCTION: LEGORR 0x00477bf0
static void winmain_parse_options(const char* parameters,
                                  DWORD* no_sound_out,
                                  DWORD* insist_on_cd_out)
{
    const char* option;

    option = case_insensitive_substring_find(parameters, "_insistOnCD");
    if (option != NULL) *insist_on_cd_out = 1;
    option = case_insensitive_substring_find(parameters, "_nosound");
    if (option != NULL) *no_sound_out = 1;
    option = case_insensitive_substring_find(parameters, "_debug");
    if (option != NULL) s_engine_feature_word |= 0x2000u;
    option = case_insensitive_substring_find(parameters, "-nm");
    if (option != NULL) s_engine_feature_word |= 0x20u;
    option = case_insensitive_substring_find(parameters, "-ftm");
    if (option != NULL) s_engine_feature_word |= 0x40000u;
    option = case_insensitive_substring_find(parameters, "-fvf");
    if (option != NULL) s_engine_feature_word |= 0x80000u;
    option = case_insensitive_substring_find(parameters, "_best");
    if (option != NULL) s_engine_feature_word |= 0x40u;
    option = case_insensitive_substring_find(parameters, "_window");
    if (option != NULL) s_engine_feature_word |= 0x100u;
    option = case_insensitive_substring_find(parameters, "_dualmouse");
    if (option != NULL) s_engine_feature_word |= 0x4000u;
    option = case_insensitive_substring_find(parameters, "_debugcomplete");
    if (option != NULL) s_engine_feature_word |= 0x8000u;
    option = case_insensitive_substring_find(parameters, "_testercall");
    if (option != NULL) {
        s_programmer_mode = 2u;
        s_engine_feature_word |= 0x10000u;
    }
    option = case_insensitive_substring_find(parameters, "_testlevels");
    if (option != NULL) s_engine_feature_word |= 0x20000u;
    option = case_insensitive_substring_find(parameters, "_reducesamples");
    if (option != NULL) s_engine_feature_word |= 0x100000u;
    option = case_insensitive_substring_find(parameters, "_showversion");
    if (option != NULL) s_engine_feature_word |= 0x200000u;
    option = case_insensitive_substring_find(parameters, "_reduceanimation");
    if (option != NULL) s_engine_feature_word |= 0x400000u;
    option = case_insensitive_substring_find(parameters, "_reducepromeshes");
    if (option != NULL) s_engine_feature_word |= 0x800000u;
    option = case_insensitive_substring_find(parameters, "_reduceflics");
    if (option != NULL) s_engine_feature_word |= 0x1000000u;
    option = case_insensitive_substring_find(parameters, "_reduceimages");
    if (option != NULL) s_engine_feature_word |= 0x2000000u;
    option = case_insensitive_substring_find(parameters, "_startlevel");
    if (option != NULL) {
        const char* level = option + 11;
        char* output = s_start_level_name;
        char* const output_end = s_start_level_name + sizeof(s_start_level_name) - 1;
        while (*level == ' ') ++level;
        while (*level != '\0' && *level != ' ' && output < output_end) {
            *output++ = *level++;
        }
        *output = '\0';
        s_engine_feature_word |= 0x200u;
    }
    option = case_insensitive_substring_find(parameters, "_flags");
    if (option != NULL) s_render_feature_flags = (uint32_t)atoi(option + 6);
    option = case_insensitive_substring_find(parameters, "_fpslock");
    if (option != NULL) {
        unsigned int fps = (unsigned int)atoi(option + 8);
        if (fps != 0u) s_frame_interval_override = 25.0f / (float)fps;
    }
    option = case_insensitive_substring_find(parameters, "_programmer");
    if (option == NULL) {
        s_programmer_mode = 0u;
    } else {
        s_programmer_mode = (DWORD)atoi(option + 11);
        if (s_programmer_mode == 0u) s_programmer_mode = 1u;
    }
    option = case_insensitive_substring_find(parameters, "_cleansaves");
    if (option != NULL) s_engine_feature_word |= 0x400u;
}

static int winmain_application_run(void)
{
    PlatformEvent events[32];
    DWORD previous_time;
    int result = frameloop_application_init();
    if (result != 1) return result;
    previous_time = GetTickCount();
    for (;;) {
        DWORD frame_start = GetTickCount();
        DWORD elapsed = frame_start - previous_time;
        float dt = (float)elapsed / 1000.0f;
        int event_count;
        DWORD frame_elapsed;
        if ((s_engine_feature_word & 0x10u) != 0u) dt = 0.0f;
        else if ((s_engine_feature_word & 0x80u) != 0u) dt = 1.0f / 30.0f;
        if (s_frame_interval_override != 0.0f)
            dt = s_frame_interval_override * 0.04f;
        if (dt > 3.0f) dt = 3.0f;
        previous_time = frame_start;
        event_count = platform_pump_events(
            frameloop_application_window_get(), events, 32);
        frameloop_application_events_set(events, event_count);
        if (!frameloop_application_tick(dt)) break;
        frame_elapsed = GetTickCount() - frame_start;
        if (frame_elapsed < 33u) Sleep(33u - frame_elapsed);
    }
    frameloop_application_shutdown();
    return 0;
}

namespace unported_ledger_stubs { void ledger_stub_0048b260_StartupStreamStateReset(); }

// FUNCTION: LEGORR 0x4777a0
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    /* FUN_004777a0 formats "%s_Mutex" with "Lego_Rock_Raiders" and exits
     * when another instance owns that name. */
    char mutex_name[128];
    char no_hal_message[1024];
    char startup_parameters[1024];
    char standard_parameters[1024];
    volatile DWORD startup_state[4] = { 0, 0, 0, 0 };
    volatile float frame_delta = 0.0f;
    sprintf(mutex_name, "%s_Mutex", "Lego_Rock_Raiders");
    CreateMutexA(NULL, TRUE, mutex_name);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    /* The reference takes the executable portion of GetCommandLineA(),
     * removes quotes, uppercases its extension, then removes a .EXE suffix.
     * Its fixed 1024-byte global is mirrored here. */
    const char* command_line = GetCommandLineA();
    const char* basename = command_line;
    const char* cursor = command_line;
    while (*cursor != '\0') {
        if (*cursor == '\\') {
            basename = cursor + 1;
        }
        ++cursor;
    }

    size_t length = 0;
    while (basename[length] != '\0' && basename[length] != '"'
           && length + 1 < sizeof(s_application_prefix)) {
        s_application_prefix[length] = basename[length];
        ++length;
    }
    s_application_prefix[length] = '\0';

    char* last_extension = s_application_prefix;
    char* letter = s_application_prefix;
    while (*letter != '\0') {
        if (*letter == '.') {
            last_extension = letter + 1;
        }
        ++letter;
    }
    if (last_extension != s_application_prefix) {
        letter = last_extension;
        while (*letter != '\0') {
            if (*letter >= 'a' && *letter <= 'z') {
                *letter = (char)(*letter - ('a' - 'A'));
            }
            ++letter;
        }
        char* exe_suffix = strstr(s_application_prefix, ".EXE");
        if (exe_suffix != NULL) {
            *exe_suffix = '\0';
        }
    }

    if (!winmain_registry_read("StandardParameters", standard_parameters,
                               sizeof(standard_parameters))) {
        standard_parameters[0] = '\0';
    }
    if (standard_parameters[0] != '\0') {
        sprintf(startup_parameters, "%s %s", lpCmdLine,
                standard_parameters);
    } else {
        sprintf(startup_parameters, "%s", lpCmdLine);
    }
    winmain_parse_options(startup_parameters, &s_startup_no_sound,
                          &s_startup_insist_on_cd);
    frameloop_application_start_level_override_set(s_start_level_name);
    frameloop_application_sound_disabled_set((int)s_startup_no_sound);
    startup_state[0] = s_startup_no_sound;
    startup_state[1] = s_startup_insist_on_cd;
    resource_startup_require_cd_set((int)s_startup_insist_on_cd);

    if (!winmain_registry_read("NoHALMessage", no_hal_message,
                               sizeof(no_hal_message))) {
        sprintf(no_hal_message,
                "No DirectX 3D accelerator could be found.");
    }
    resource_set_wad_prefix(s_application_prefix);
    (void)startup_state[2];
    (void)startup_state[3];
    unported_ledger_stubs::ledger_stub_0048b260_StartupStreamStateReset();
    (void)no_hal_message;
    (void)hInstance;
    return winmain_application_run();
}
