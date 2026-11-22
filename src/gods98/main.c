// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-005 (text 004777a0-00478df0; strings
 * 004aabf0-004aae20; bss 00505800-0050608c; finding 641): gods98's Main.c.
 * WinMain runs the start-up sequence (the command line, the registry
 * options, Files.c, Config.c, Input.c, the window, DirectDraw.c, the sound,
 * Init.c's display selection, the AVI player, Draw.c, then Lego.c's
 * Gods_Go), the main loop over the game's three callbacks, and the shutdown
 * sequence. The rest of the unit is the window and its two window
 * procedures, the Direct3D device set-up and render-state cache, and the
 * CD-audio mixer volume. The binary attests none of the names; the
 * functions keep the names the port gave them.
 *
 * The process entry (platform/native/entry.cpp) calls WinMain with the
 * command line; miniwin provides the window, the message queue, DirectDraw
 * and Direct3DRM (ADR 0007). WINMM's time comes from SDL3
 * ([library:time]); the CD-audio mixer is the audio library's
 * ([library:audio]), and KERNEL32's single-instance mutex is a seam stub
 * (platform/native/seam_stubs.cpp).
 *
 * The unit's .bss is the original's mainGlobs, one external struct
 * (finding 679); other units read thirteen of its fields by the port's
 * names, which main_globs.h makes the fields. */
#include "main.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <miniwin/windows.h>
#include "winmm.h"
#include "library/audio_sdl.h"

#include "crt/vc5names.h"
#include "avi_player.h"
#include "config.h"
#include "directdraw.h"
#include "draw.h"
#include "dxerror.h"
#include "files.h"
#include "g98_024.h"
#include "g98_029.h"
#include "init.h"
#include "soundsys.h"
#include "utils.h"
#include "main_imports.h"
#define LEGORRR_MAIN_GLOBS_OWNER
#include "gods98/main_globs.h"
#include "platform/native/config.h" // [divergence:display-fit] LRR_GetDisplayScaling

#define MAIN_MAXRENDERSTATES        200

#define MAIN_FLAG_UPDATED           0x00000001
#define MAIN_FLAG_FULLSCREEN        0x00000002
#define MAIN_FLAG_VIDEOTEXTURE      0x00000004
#define MAIN_FLAG_MIPMAPENABLED     0x00000008
#define MAIN_FLAG_PAUSED            0x00000010
#define MAIN_FLAG_DONTMANAGETEXTURES 0x00000020
#define MAIN_FLAG_BEST              0x00000040
#define MAIN_FLAG_DETERMINISTIC     0x00000080
#define MAIN_FLAG_WINDOW            0x00000100
#define MAIN_FLAG_STARTLEVEL        0x00000200
#define MAIN_FLAG_CLEANSAVES        0x00000400
#define MAIN_FLAG_DEBUGMODE         0x00002000
#define MAIN_FLAG_DUALMOUSE         0x00004000
#define MAIN_FLAG_DEBUGCOMPLETE     0x00008000
#define MAIN_FLAG_TESTERCALL        0x00010000
#define MAIN_FLAG_LEVELSOPEN        0x00020000
#define MAIN_FLAG_FORCETEXTUREMANAGEMENT 0x00040000
#define MAIN_FLAG_FORCEVERTEXFOG    0x00080000
#define MAIN_FLAG_REDUCESAMPLES     0x00100000
#define MAIN_FLAG_SHOWVERSION       0x00200000
#define MAIN_FLAG_REDUCEANIMATION   0x00400000
#define MAIN_FLAG_REDUCEPROMESHES   0x00800000
#define MAIN_FLAG_REDUCEFLICS       0x01000000
#define MAIN_FLAG_REDUCEIMAGES      0x02000000

#include <miniwin/d3d.h>
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>
#include <SDL3/SDL_timer.h>

/* WinMain's shutdown passes FALSE to the shared empty function at
 * 00484c90 (Flic_Nothing, which takes one pointer). */
#define main_shutdown_nothing() (flic_opcode27_fallback_noop(NULL))

#define lpD3DRM()   ((LPDIRECT3DRM3)s_main_d3drm)
#define lpDevice()  ((LPDIRECT3DRMDEVICE3)s_main_device)
#define lpIMDevice() ((LPDIRECT3DDEVICE3)s_main_im_device)

/* The game's three callbacks (Main_State). */
typedef struct Main_State {
    int (*Initialise)(void);
    int (*MainLoop)(float elapsed);
    void (*Shutdown)(void);
} Main_State;

/* A render state Main_ChangeRenderState changed, and its value before. */
typedef struct Main_StateChangeData {
    unsigned int origValue;
    int changed;
} Main_StateChangeData;

/* IID_IDirectDrawSurface, IID_IDirect3DDevice3 and IID_IDirect3DRM3 come
 * from miniwin's headers (the original links them from dxguid.lib, at
 * 0049f6d8 and 0049fbd8 for the last two). */

/* The unit's .bss, mainGlobs (main_globs.h): one external definition with
 * `= { 0 }`, so it stays in the object's .bss (finding 669). Reward.c and
 * Advisor.c read fixedFrameTiming and startLevel, and in the matching build
 * every unit that reads a field the port shares reads it here. */
typedef struct Main_Globs {
    void* hWnd;
    void* hInst;
    unsigned int active;
    unsigned int exit;
    char* className;
    char programName[256];
    void* lpD3DRM;
    void* device;
    void* imDevice;
    unsigned int fogMethod;
    int appWidth;
    int appHeight;
    Main_State currState;
    int stateSet;
    float fixedFrameTiming;
    Main_StateChangeData renderStates[MAIN_MAXRENDERSTATES];
    unsigned int style;
    unsigned int flags;
    int programmerLevel;
    char startLevel[256];
    unsigned int clFlags;
} Main_Globs;

#ifdef __cplusplus
extern "C" {
#endif
// GLOBAL: LEGORR 0x00505800
Main_Globs mainGlobs = { 0 };
#ifdef __cplusplus
}
#endif

// STRING: LEGORR 0x004aac34
#define MAIN_REGISTRY_KEY "SOFTWARE\\LEGO Media\\Games\\Rock Raiders"

/* The unit's functions, with C linkage in the C++ builds too: other
 * modules call them by these names (frameloop.h, surface.hpp, audio.hpp,
 * crt.hpp and the converted units' import headers). */
#ifdef __cplusplus
extern "C" {
#endif
void low_texture_flag_set(void);
void winmain_parse_options(const char* lpCmdLine, int* nosound,
                           int* insistOnCD);
void main_loop_frame_pump(int clear);
unsigned int render_feature_flags_get(void);
int window_runtime_display_caps_query(void);
void renderer_begin_frame_once_retail(void);
int window_runtime_callback_triplet_set(const Main_State* state);
unsigned long main_time_get(void);
int window_runtime_message_dispatch_filter(const MSG* msg);
void window_message_dispatch_pump(void);
void renderer_window_mode_apply(int fullScreen, int xPos, int yPos,
                                int width, int height);
int direct3drm_device_initialize(DirectDraw_Device* dev, void* ddraw1,
                                 LPDIRECTDRAWSURFACE4 backSurf, int doubleBuffered);
void window_runtime_adjust_rect(LPRECT rect);
void direct3d_render_preferences_apply(int quality, int dither,
                                       int filter, int mipmap,
                                       int linearMipMap);
void window_title_set(const char* title);
int main_init_app(HINSTANCE hInstance);
LRESULT window_game_window_procedure_retail(HWND hWnd, UINT message,
                                            WPARAM wParam, LPARAM lParam);
LRESULT window_activation_close_retail(HWND hWnd, UINT message,
                                       WPARAM wParam, LPARAM lParam);
void direct3d_render_state_set_cached(DWORD dwRenderStateType,
                                      DWORD dwRenderState);
void d3d_material_state_flush(void);
void music_mixer_volume_set(float leftVolume, float rightVolume);
int ledger_stub_004789a0_MusicMixerVolumeGet(float* leftVolume,
                                             float* rightVolume);
int audio_mixer_control_init(float* leftVolume, float* rightVolume,
                             int set);
#ifdef __cplusplus
}
#endif

// FUNCTION: LEGORR 0x004777a0
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int cmdShow)
{
    char commandLine[1024];
    char tempStr[1024];
    char noHALMsg[1024];
    int nosound = FALSE;
    int insistOnCD = FALSE;
    // STRING: LEGORR 0x004aac84
    const char* productName = "Lego Rock Raiders";

    if (productName) {
        char mutexName[128];
        // STRING: LEGORR 0x004aac78
        snprintf(mutexName, sizeof(mutexName), "%s Mutex", productName);
        CreateMutexA(NULL, TRUE, mutexName);
        if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;
    }

    {
        char* s;
        char* argv0;
        char* ext;

        for (argv0 = s = GetCommandLineA(); *s != '\0'; s++) {
            if (*s == '\\') argv0 = s + 1;
        }
        strcpy(mainGlobs.programName, argv0);
        for (s = mainGlobs.programName; *s != '\0'; s++) {
            if (*s == '"') *s = '\0';
        }
        for (ext = s = mainGlobs.programName; *s != '\0'; s++) {
            if (*s == '.') ext = s + 1;
        }
        if (ext != mainGlobs.programName) {
            strupr(ext);
            // STRING: LEGORR 0x004aac70
            if ((s = strstr(mainGlobs.programName, ".EXE")) != NULL) *s = '\0';
        }
    }

    mainGlobs.className = mainGlobs.programName;
    s_window_application_active = FALSE;
    s_window_close_requested = FALSE;
    mainGlobs.stateSet = FALSE;
    g_main_instance_handle = hInstance;
    mainGlobs.fixedFrameTiming = 0.0f;
    s_engine_feature_word = 0;

    // STRING: LEGORR 0x004aac5c
    if (registry_path_accessor(MAIN_REGISTRY_KEY, "StandardParameters", 0,
                               tempStr, 1024)) {
        // STRING: LEGORR 0x004aac2c
        snprintf(commandLine, sizeof(commandLine), "%s %s", lpCmdLine, tempStr);
    } else {
        snprintf(commandLine, sizeof(commandLine), "%s", lpCmdLine);
    }

    winmain_parse_options(commandLine, &nosound, &insistOnCD);

    // STRING: LEGORR 0x004aac1c
    if (!registry_path_accessor(MAIN_REGISTRY_KEY, "NoHALMessage", 0,
                                noHALMsg, 1024)) {
        // STRING: LEGORR 0x004aabf0
        snprintf(noHALMsg, sizeof(noHALMsg), "No DirectX 3D accelerator could be found.");
    }

    ledger_stub_0048b260_StartupStreamStateReset();
    level_map_slots_reset();
    file_initialise(mainGlobs.programName, insistOnCD, MAIN_REGISTRY_KEY);
    block_text_token_pool_init();
    input_direct_initialize();

    if (main_init_app(hInstance)) {
        directdraw_initialise(g_platform_cursor_window_handle);
        if (media_cd_audio_device_state_update(nosound) &&
            init_initialise(TRUE, s_engine_feature_word & MAIN_FLAG_DEBUGMODE,
                            s_engine_feature_word & MAIN_FLAG_BEST,
                            s_engine_feature_word & MAIN_FLAG_WINDOW,
                            noHALMsg)) {
            avi_initialise(directDrawGlobs.lpDirectDraw);
            surface_render_setup(NULL);
            lego_game_config_callback_registry_init(mainGlobs.programName);

            if (mainGlobs.stateSet) {
                if (mainGlobs.currState.Initialise != NULL) {
                    if (!mainGlobs.currState.Initialise()) {
                        mainGlobs.currState.Initialise = NULL;
                        mainGlobs.currState.MainLoop = NULL;
                        mainGlobs.currState.Shutdown = NULL;
                    }
                }

                if (mainGlobs.currState.MainLoop != NULL) {
                    float time = 1.0f;
                    unsigned long lastTime, currTime;

                    lastTime = (DWORD)SDL_GetTicks(); // [library:time]
                    while (!s_window_close_requested) {
                        s_window_mouse_word_4c = 0;
                        s_window_mouse_word_50 = 0;
                        s_window_mouse_word_54 = 0;
                        s_window_mouse_word_58 = 0;

                        window_message_dispatch_pump();

                        if (s_engine_feature_word & MAIN_FLAG_FULLSCREEN)
                            s_window_application_active = TRUE;

                        input_state_poll_retail();
                        input_cursor_position_update();

                        if (!mainGlobs.currState.MainLoop(time))
                            s_window_close_requested = TRUE;

                        renderer_begin_frame_once_retail();
                        directdraw_flip();

                        s_engine_feature_word &= ~MAIN_FLAG_UPDATED;

                        if (s_engine_feature_word & MAIN_FLAG_DETERMINISTIC) {
                            time = 25.0f / 30.0f;
                        } else if (s_engine_feature_word & MAIN_FLAG_PAUSED) {
                            time = 0.0f;
                            lastTime = (DWORD)SDL_GetTicks(); // [library:time]
                        } else if (0.0f == mainGlobs.fixedFrameTiming) {
                            currTime = (DWORD)SDL_GetTicks(); // [library:time]
                            time = (float)(currTime - lastTime) * 0.025f;
                            lastTime = currTime;
                            if (time > 3.0f) time = 3.0f;
                        } else {
                            time = mainGlobs.fixedFrameTiming;
                            lastTime = (DWORD)SDL_GetTicks(); // [library:time]
                        }
                    }
                }

                if (mainGlobs.currState.Shutdown != NULL)
                    mainGlobs.currState.Shutdown();
            }
        }

        directdraw_shutdown();
        DestroyWindow((HWND)g_platform_cursor_window_handle);
    }

    input_direct_shutdown();
    block_text_token_pool_destroy();
    main_shutdown_nothing();
    ledger_stub_0048b290_StartupStreamsClose();
    startup_config_stream_load();

    return 0;
}

// FUNCTION: LEGORR 0x00477bd0
void low_texture_flag_set(void)
{
    if (!(s_engine_feature_word & MAIN_FLAG_FORCETEXTUREMANAGEMENT))
        s_engine_feature_word |= MAIN_FLAG_DONTMANAGETEXTURES;
}

// FUNCTION: LEGORR 0x00477bf0
void winmain_parse_options(const char* lpCmdLine, int* nosound,
                           int* insistOnCD)
{
    const char* s;

    // STRING: LEGORR 0x004aada4
    if (case_insensitive_substring_find(lpCmdLine, "-insistOnCD")) *insistOnCD = TRUE;
    // STRING: LEGORR 0x004aad98
    if (case_insensitive_substring_find(lpCmdLine, "-nosound")) *nosound = TRUE;
    // STRING: LEGORR 0x004aad90
    if (case_insensitive_substring_find(lpCmdLine, "-debug")) s_engine_feature_word |= MAIN_FLAG_DEBUGMODE;
    // STRING: LEGORR 0x004aad8c
    if (case_insensitive_substring_find(lpCmdLine, "-nm")) s_engine_feature_word |= MAIN_FLAG_DONTMANAGETEXTURES;
    // STRING: LEGORR 0x004aad84
    if (case_insensitive_substring_find(lpCmdLine, "-ftm")) s_engine_feature_word |= MAIN_FLAG_FORCETEXTUREMANAGEMENT;
    // STRING: LEGORR 0x004aad7c
    if (case_insensitive_substring_find(lpCmdLine, "-fvf")) s_engine_feature_word |= MAIN_FLAG_FORCEVERTEXFOG;
    // STRING: LEGORR 0x004aad74
    if (case_insensitive_substring_find(lpCmdLine, "-best")) s_engine_feature_word |= MAIN_FLAG_BEST;
    // STRING: LEGORR 0x004aad6c
    if (case_insensitive_substring_find(lpCmdLine, "-window")) s_engine_feature_word |= MAIN_FLAG_WINDOW;
    // STRING: LEGORR 0x004aad60
    if (case_insensitive_substring_find(lpCmdLine, "-dualmouse")) s_engine_feature_word |= MAIN_FLAG_DUALMOUSE;
    // STRING: LEGORR 0x004aad50
    if (case_insensitive_substring_find(lpCmdLine, "-debugcomplete")) s_engine_feature_word |= MAIN_FLAG_DEBUGCOMPLETE;
    // STRING: LEGORR 0x004aad44
    if (case_insensitive_substring_find(lpCmdLine, "-testercall")) {
        s_application_mode = 2;
        s_engine_feature_word |= MAIN_FLAG_TESTERCALL;
    }
    // STRING: LEGORR 0x004aad38
    if (case_insensitive_substring_find(lpCmdLine, "-testlevels")) s_engine_feature_word |= MAIN_FLAG_LEVELSOPEN;
    // STRING: LEGORR 0x004aad28
    if (case_insensitive_substring_find(lpCmdLine, "-reducesamples")) s_engine_feature_word |= MAIN_FLAG_REDUCESAMPLES;
    // STRING: LEGORR 0x004aad18
    if (case_insensitive_substring_find(lpCmdLine, "-showversion")) s_engine_feature_word |= MAIN_FLAG_SHOWVERSION;
    // STRING: LEGORR 0x004aad04
    if (case_insensitive_substring_find(lpCmdLine, "-reduceanimation")) s_engine_feature_word |= MAIN_FLAG_REDUCEANIMATION;
    // STRING: LEGORR 0x004aacf0
    if (case_insensitive_substring_find(lpCmdLine, "-reducepromeshes")) s_engine_feature_word |= MAIN_FLAG_REDUCEPROMESHES;
    // STRING: LEGORR 0x004aace0
    if (case_insensitive_substring_find(lpCmdLine, "-reduceflics")) s_engine_feature_word |= MAIN_FLAG_REDUCEFLICS;
    // STRING: LEGORR 0x004aacd0
    if (case_insensitive_substring_find(lpCmdLine, "-reduceimages")) s_engine_feature_word |= MAIN_FLAG_REDUCEIMAGES;

    // STRING: LEGORR 0x004aacc4
    if ((s = case_insensitive_substring_find(lpCmdLine, "-startlevel")) != NULL) {
        const char* level = s + 11;
        int i = 0;
        while (*level != '\0' && *level == ' ') level++;
        while (*level != '\0' && *level != ' ') mainGlobs.startLevel[i++] = *level++;
        mainGlobs.startLevel[i] = '\0';
        s_engine_feature_word |= MAIN_FLAG_STARTLEVEL;
    }

    // STRING: LEGORR 0x004aacbc
    if ((s = case_insensitive_substring_find(lpCmdLine, "-flags")) != NULL) {
        s_render_feature_flags = atoi(s + 6);
    }

    // STRING: LEGORR 0x004aacb0
    if ((s = case_insensitive_substring_find(lpCmdLine, "-fpslock")) != NULL) {
        unsigned int fps = atoi(s + 8);
        if (fps) mainGlobs.fixedFrameTiming = 25.0f / fps;
    }

    // STRING: LEGORR 0x004aaca4
    if ((s = case_insensitive_substring_find(lpCmdLine, "-programmer")) != NULL) {
        s_application_mode = atoi(s + 11);
        if (!s_application_mode) s_application_mode = 1;
    } else {
        s_application_mode = 0;
    }

    // STRING: LEGORR 0x004aac98
    if (case_insensitive_substring_find(lpCmdLine, "-cleansaves")) s_engine_feature_word |= MAIN_FLAG_CLEANSAVES;
}

// FUNCTION: LEGORR 0x00477f30
void main_loop_frame_pump(int clear)
{
    window_message_dispatch_pump();
    input_state_poll_retail();
    input_cursor_position_update();
    renderer_begin_frame_once_retail();
    directdraw_flip();
    if (clear) directdraw_clear(NULL, 0);
    s_engine_feature_word &= ~MAIN_FLAG_UPDATED;
}

// FUNCTION: LEGORR 0x00477f70
unsigned int render_feature_flags_get(void)
{
    return s_render_feature_flags;
}

// FUNCTION: LEGORR 0x00477f80
int window_runtime_display_caps_query(void)
{
    HDC hdc = GetDC((HWND)g_platform_cursor_window_handle);
    return GetDeviceCaps(hdc, BITSPIXEL);
}

// FUNCTION: LEGORR 0x00477fa0
void renderer_begin_frame_once_retail(void)
{
    if (!(s_engine_feature_word & MAIN_FLAG_UPDATED)) {
        lpDevice()->Update();
        s_engine_feature_word |= MAIN_FLAG_UPDATED;
    }
}

// FUNCTION: LEGORR 0x00477fd0
int window_runtime_callback_triplet_set(const Main_State* state)
{
    if (state) {
        mainGlobs.currState = *state;
        mainGlobs.stateSet = TRUE;
        return TRUE;
    }
    return FALSE;
}

/* Main_GetTime: the original compiles it to a bare jump through the
 * timeGetTime import, and Reward.c calls it directly (finding 574). */
// FUNCTION: LEGORR 0x00478000
unsigned long main_time_get(void)
{
    return (DWORD)SDL_GetTicks(); // [library:time]
}

// FUNCTION: LEGORR 0x00478010
int window_runtime_message_dispatch_filter(const MSG* msg)
{
    if ((s_engine_feature_word & MAIN_FLAG_FULLSCREEN) &&
        (msg->message == WM_ACTIVATEAPP || msg->message == WM_SYSKEYDOWN ||
         msg->message == WM_SYSKEYUP)) {
        return FALSE;
    }
    return TRUE;
}

// FUNCTION: LEGORR 0x00478040
void window_message_dispatch_pump(void)
{
    MSG msg;

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (window_runtime_message_dispatch_filter(&msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

// FUNCTION: LEGORR 0x004780b0
void renderer_window_mode_apply(int fullScreen, int xPos, int yPos,
                                int width, int height)
{
    RECT rect;

    s_display_w = width;
    s_display_h = height;
    if (fullScreen) s_engine_feature_word |= MAIN_FLAG_FULLSCREEN;
    window_mode_flag_set(fullScreen);

    if (!fullScreen) {
        rect.left = xPos;
        mainGlobs.style = WS_POPUP | WS_SYSMENU | WS_CAPTION;
        /* [divergence:display-fit] With tweaks:Display = fit, the
         * window can be resized: miniwin fits the game's picture into it
         * (docs/divergences.md). */
        if (LRR_GetDisplayScaling() == LRR_DISPLAY_FIT) mainGlobs.style |= WS_THICKFRAME;
        rect.top = yPos;
        rect.right = xPos + width;
        rect.bottom = yPos + height;
        window_runtime_adjust_rect(&rect);
        SetWindowLongA((HWND)g_platform_cursor_window_handle, GWL_STYLE, mainGlobs.style);
        SetWindowPos((HWND)g_platform_cursor_window_handle, NULL, rect.left, rect.top,
                     rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
        ShowCursor(FALSE);
    } else {
        GetWindowRect(GetDesktopWindow(), &rect);
        SetWindowPos((HWND)g_platform_cursor_window_handle, NULL, rect.left, rect.top,
                     rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
        SetCursor(NULL);
    }

    ShowWindow((HWND)g_platform_cursor_window_handle, SW_SHOW);
    SetActiveWindow((HWND)g_platform_cursor_window_handle);
}

// FUNCTION: LEGORR 0x004781d0
int direct3drm_device_initialize(DirectDraw_Device* dev, void* ddraw1,
                                 LPDIRECTDRAWSURFACE4 backSurf, int doubleBuffered)
{
    LPDIRECT3DRM lpD3DRM1;
    LPDIRECTDRAWSURFACE surf1 = NULL;
    LPDIRECT3DDEVICE2 imdev2;
    HRESULT r = D3DRM_OK;
    HRESULT qr;
    GUID* guid = NULL;

    if (dev) {
        guid = (GUID*)&dev->guid;
        if (dev->flags & DIRECTDRAW_FLAG_DEVICE_VIDEOTEXTURE)
            s_engine_feature_word |= MAIN_FLAG_VIDEOTEXTURE;
    }

    if ((s_engine_feature_word & MAIN_FLAG_FORCEVERTEXFOG) ||
        !(dev->flags & DIRECTDRAW_FLAG_DEVICE_HARDWARE)) {
        s_scene_fog_mode = 1;
    } else {
        s_scene_fog_mode = 2;
    }

    if (Direct3DRMCreate(&lpD3DRM1) == D3DRM_OK) {
        /* s_main_d3drm holds its own reference to the object, so the
         * IDirect3DRM reference is released here (the original keeps it for
         * the rest of the run). */
        qr = lpD3DRM1->QueryInterface(IID_IDirect3DRM3, &s_main_d3drm);
        lpD3DRM1->Release();
        if (qr == D3DRM_OK) {
            backSurf->QueryInterface(IID_IDirectDrawSurface, (void**)&surf1);
            r = lpD3DRM()->CreateDeviceFromSurface(
                    guid, (LPDIRECTDRAW)ddraw1, surf1, 0,
                    (LPDIRECT3DRMDEVICE3*)&s_main_device);
            if (surf1) surf1->Release();
            if (r == D3DRM_OK) {
                lpDevice()->GetDirect3DDevice2(&imdev2);
                imdev2->QueryInterface(IID_IDirect3DDevice3, &s_main_im_device);
                imdev2->Release();
                if (doubleBuffered)
                    lpDevice()->SetBufferCount(2);
                return TRUE;
            } else {
                directx_result_diagnostic_dispatch(
                    // STRING: LEGORR 0x004aadb0
                    r, 3, "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Main.c", 568);
                /* Init.c can set the display up again on the next device,
                 * which would overwrite the pointer and leak the object. */
                lpD3DRM()->Release();
                s_main_d3drm = NULL;
            }
        }
    }

    if (r != D3DRM_OK) {
        directx_result_diagnostic_dispatch(
            r, 1, "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Main.c", 576);
    }
    return FALSE;
}

// FUNCTION: LEGORR 0x00478310
void window_runtime_adjust_rect(LPRECT rect)
{
    if (!(s_engine_feature_word & MAIN_FLAG_FULLSCREEN))
        AdjustWindowRect(rect, mainGlobs.style, FALSE);
}

// FUNCTION: LEGORR 0x00478330
void direct3d_render_preferences_apply(int quality, int dither,
                                       int filter, int mipmap,
                                       int linearMipMap)
{
    DWORD renderQuality = 0x40;     /* D3DRMRENDER_WIREFRAME */
    DWORD textureQuality;

    if (quality == 0) renderQuality = 0x40;   /* D3DRMRENDER_WIREFRAME */
    if (quality == 2) renderQuality = 0x88;   /* D3DRMRENDER_FLAT */
    if (quality == 1) renderQuality = 0x80;   /* D3DRMRENDER_UNLITFLAT */
    if (quality == 3) renderQuality = 0x89;   /* D3DRMRENDER_GOURAUD */

    if (filter) {
        if (mipmap) {
            /* D3DRMTEXTURE_LINEARMIPLINEAR or D3DRMTEXTURE_MIPLINEAR */
            textureQuality = linearMipMap ? 5 : 3;
            s_engine_feature_word |= MAIN_FLAG_MIPMAPENABLED;
        } else {
            textureQuality = 1;     /* D3DRMTEXTURE_LINEAR */
        }
    } else {
        textureQuality = 0;         /* D3DRMTEXTURE_NEAREST */
    }

    if (lpDevice()) {
        lpDevice()->SetDither(dither);
        lpDevice()->SetQuality((D3DRMRENDERQUALITY)renderQuality);
        lpDevice()->SetTextureQuality((D3DRMTEXTUREQUALITY)textureQuality);
    }
}

// FUNCTION: LEGORR 0x004783d0
void window_title_set(const char* title)
{
    SetWindowTextA((HWND)g_platform_cursor_window_handle, title);
}

/* Main_WndProc, the window class's procedure: both handlers below are
 * cdecl. */
static LRESULT CALLBACK main_window_procedure(HWND hWnd, UINT message,
                                              WPARAM wParam, LPARAM lParam);

// FUNCTION: LEGORR 0x004783f0
int main_init_app(HINSTANCE hInstance)
{
    WNDCLASSA wc;

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = main_window_procedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = mainGlobs.className;

    if (RegisterClassA(&wc)) {
        g_platform_cursor_window_handle = CreateWindowExA(
            WS_EX_APPWINDOW, mainGlobs.className, "", WS_POPUP | WS_SYSMENU,
            0, 0, 100, 100, NULL, NULL, hInstance, NULL);
        if (g_platform_cursor_window_handle) {
            SetFocus((HWND)g_platform_cursor_window_handle);
            return TRUE;
        } else {
            // STRING: LEGORR 0x004aae00
            // STRING: LEGORR 0x004aae20
            MessageBoxA(NULL, "Unable to Create Main Window", "Fatal Error", MB_OK);
        }
    } else {
        // STRING: LEGORR 0x004aade0
        MessageBoxA(NULL, "Unable to register window class", "Fatal Error", MB_OK);
    }
    return FALSE;
}

// FUNCTION: LEGORR 0x004784c0
LRESULT window_game_window_procedure_retail(HWND hWnd, UINT message,
                                            WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        break;

    case WM_ACTIVATEAPP:
        s_window_application_active = wParam;
        return 0;

    case WM_DESTROY:
    case WM_CLOSE:
    case WM_QUIT:
        s_window_close_requested = TRUE;
        break;

    case WM_WINDOWPOSCHANGING:
        ((WINDOWPOS*)lParam)->flags &= ~SWP_NOOWNERZORDER;
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (s_engine_feature_word & MAIN_FLAG_DUALMOUSE) {
            switch (message) {
            case WM_LBUTTONDOWN:
                g_renderer_word_c = s_window_mouse_word_4c = TRUE;
                return 0;
            case WM_LBUTTONUP:
                g_renderer_word_c = s_window_mouse_word_4c = FALSE;
                return 0;
            case WM_RBUTTONDOWN:
                s_window_mouse_word_40 = s_window_mouse_word_50 = TRUE;
                return 0;
            case WM_RBUTTONUP:
                s_window_mouse_word_40 = s_window_mouse_word_50 = FALSE;
                return 0;
            }
        } else {
            switch (message) {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                if (!g_renderer_word_c) {
                    s_window_mouse_word_50 = s_window_mouse_word_4c = TRUE;
                }
                g_renderer_word_c = s_window_mouse_word_40 = TRUE;
                s_window_mouse_word_54 = TRUE;
                return 0;
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
                s_window_mouse_word_50 = s_window_mouse_word_4c = FALSE;
                g_renderer_word_c = s_window_mouse_word_40 = FALSE;
                break;
            }
        }
        /* Falls through. */
    case WM_LBUTTONDBLCLK:
        s_window_mouse_word_54 = TRUE;
        return 0;

    case WM_RBUTTONDBLCLK:
        s_window_mouse_word_58 = TRUE;
        return 0;
    case WM_MOVE:
    case WM_SIZE:
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_MOUSEMOVE:
    case WM_ENTERMENULOOP:
    case WM_EXITMENULOOP:
        return 0;

    }

    return DefWindowProcA(hWnd, message, wParam, lParam);
}

// FUNCTION: LEGORR 0x004786c0
LRESULT window_activation_close_retail(HWND hWnd, UINT message,
                                       WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_ACTIVATE:
        return 0;

    case WM_ACTIVATEAPP:
        s_window_application_active = wParam;
        return 0;

    case WM_CLOSE:
        s_window_close_requested = TRUE;
        return 0;

    case WM_DESTROY:
    case WM_QUIT:
        s_window_close_requested = TRUE;
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        if (s_engine_feature_word & MAIN_FLAG_DUALMOUSE) {
            switch (message) {
            case WM_LBUTTONDOWN:
                SetCapture(hWnd);
                g_renderer_word_c = s_window_mouse_word_4c = TRUE;
                s_window_mouse_word_54 = TRUE;
                return 0;
            case WM_LBUTTONUP:
                ReleaseCapture();
                s_window_mouse_word_54 = TRUE;
                g_renderer_word_c = s_window_mouse_word_4c = FALSE;
                return 0;
            case WM_RBUTTONDOWN:
                s_window_mouse_word_40 = s_window_mouse_word_50 = TRUE;
                s_window_mouse_word_54 = TRUE;
                return 0;
            case WM_RBUTTONUP:
                s_window_mouse_word_54 = TRUE;
                s_window_mouse_word_40 = s_window_mouse_word_50 = FALSE;
                return 0;
            }
        } else {
            switch (message) {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                SetCapture(hWnd);
                if (!g_renderer_word_c) {
                    s_window_mouse_word_50 = s_window_mouse_word_4c = TRUE;
                }
                g_renderer_word_c = s_window_mouse_word_40 = TRUE;
                s_window_mouse_word_54 = TRUE;
                return 0;
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
                ReleaseCapture();
                s_window_mouse_word_50 = s_window_mouse_word_4c = FALSE;
                g_renderer_word_c = s_window_mouse_word_40 = FALSE;
                break;
            }
        }
        /* Falls through. */
    case WM_LBUTTONDBLCLK:
        s_window_mouse_word_54 = TRUE;
        return 0;

    case WM_RBUTTONDBLCLK:
        s_window_mouse_word_58 = TRUE;
        return 0;
    }

    return DefWindowProcA(hWnd, message, wParam, lParam);
}

// FUNCTION: LEGORR 0x00478880
static LRESULT CALLBACK main_window_procedure(HWND hWnd, UINT message,
                                              WPARAM wParam, LPARAM lParam)
{
    if (s_engine_feature_word & MAIN_FLAG_FULLSCREEN)
        return window_game_window_procedure_retail(hWnd, message, wParam, lParam);
    else
        return window_activation_close_retail(hWnd, message, wParam, lParam);
}

// FUNCTION: LEGORR 0x004788d0
void direct3d_render_state_set_cached(DWORD dwRenderStateType,
                                      DWORD dwRenderState)
{
    Main_StateChangeData* data;
    DWORD currValue;

    lpIMDevice()->GetRenderState((D3DRENDERSTATETYPE)dwRenderStateType, &currValue);
    if (currValue != dwRenderState) {
        lpIMDevice()->SetRenderState((D3DRENDERSTATETYPE)dwRenderStateType, dwRenderState);
        data = &mainGlobs.renderStates[dwRenderStateType];
        if (data->changed) {
            if (data->origValue == currValue) data->changed = FALSE;
        } else {
            data->changed = TRUE;
            data->origValue = currValue;
        }
    }
}

// FUNCTION: LEGORR 0x00478940
void d3d_material_state_flush(void)
{
    unsigned int loop;

    for (loop = 0; loop < MAIN_MAXRENDERSTATES; loop++) {
        if (mainGlobs.renderStates[loop].changed) {
            lpIMDevice()->SetRenderState((D3DRENDERSTATETYPE)loop,
                                         mainGlobs.renderStates[loop].origValue);
            mainGlobs.renderStates[loop].changed = FALSE;
        }
    }
}

// FUNCTION: LEGORR 0x00478980
void music_mixer_volume_set(float leftVolume, float rightVolume)
{
    audio_mixer_control_init(&leftVolume, &rightVolume, TRUE);
}

// FUNCTION: LEGORR 0x004789a0
int ledger_stub_004789a0_MusicMixerVolumeGet(float* leftVolume,
                                             float* rightVolume)
{
    return audio_mixer_control_init(leftVolume, rightVolume, FALSE);
}

// FUNCTION: LEGORR 0x004789c0
int audio_mixer_control_init(float* leftVolume, float* rightVolume, int set)
{
    HMIXER hMixer;
    int result;
    unsigned int numDevs;
    unsigned int device, dest, src, control, channel;
    unsigned int connections;
    MIXERCAPSA caps;
    MIXERLINEA line;
    MIXERLINECONTROLSA lineControls;
    MIXERCONTROLDETAILS details;
    MIXERCONTROLA* controls;
    DWORD* values;
    DWORD left, right, min, range;

    numDevs = audio_mixer_get_num_devs(); // [library:audio]
    result = FALSE;

    if (set) {
        if (*leftVolume > 1.0f) *leftVolume = 1.0f;
        if (*leftVolume < 0.0f) *leftVolume = 0.0f;
        if (*rightVolume > 1.0f) *rightVolume = 1.0f;
        if (*rightVolume < 0.0f) *rightVolume = 0.0f;
    }

    for (device = 0; device < numDevs; device++) {
        if (audio_mixer_open(&hMixer, device, 0, 0, 0) == MMSYSERR_NOERROR) { // [library:audio]
            memset(&caps, 0, sizeof(caps));
            audio_mixer_get_dev_caps(device, &caps, sizeof(caps)); // [library:audio]
            for (dest = 0; dest < caps.cDestinations; dest++) {
                memset(&line, 0, sizeof(line));
                line.cbStruct = sizeof(line);
                line.dwDestination = dest;
                if (audio_mixer_get_line_info((HMIXEROBJ)hMixer, &line, // [library:audio]
                                      MIXER_GETLINEINFOF_DESTINATION) == MMSYSERR_NOERROR) {
                    connections = line.cConnections;
                    for (src = 0; src < connections; src++) {
                        line.cbStruct = sizeof(line);
                        line.dwDestination = dest;
                        line.dwSource = src;
                        if (audio_mixer_get_line_info((HMIXEROBJ)hMixer, &line, // [library:audio]
                                              MIXER_GETLINEINFOF_SOURCE) == MMSYSERR_NOERROR &&
                            line.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC) {
                            memset(&lineControls, 0, sizeof(lineControls));
                            lineControls.cbStruct = sizeof(lineControls);
                            lineControls.dwLineID = line.dwLineID;
                            lineControls.cControls = line.cControls;
                            lineControls.cbmxctrl = sizeof(MIXERCONTROLA);
                            controls = (MIXERCONTROLA*)malloc(sizeof(MIXERCONTROLA) * line.cControls);
                            lineControls.pamxctrl = controls;
                            if (audio_mixer_get_line_controls((HMIXEROBJ)hMixer, &lineControls, // [library:audio]
                                                      MIXER_GETLINECONTROLSF_ALL) == MMSYSERR_NOERROR) {
                                for (control = 0; control < line.cControls; control++) {
                                    if ((controls[control].dwControlType & MIXERCONTROL_CT_CLASS_FADER) &&
                                        (controls[control].dwControlType & MIXERCONTROL_CONTROLTYPE_VOLUME)) {
                                        memset(&details, 0, sizeof(details));
                                        details.cbStruct = sizeof(details);
                                        details.dwControlID = controls[control].dwControlID;
                                        details.cChannels = line.cChannels;
                                        details.cbDetails = sizeof(DWORD);
                                        details.cMultipleItems = 0;
                                        values = (DWORD*)malloc(sizeof(DWORD) * line.cChannels);
                                        details.paDetails = values;
                                        if (set) {
                                            min = controls[control].Bounds.dwMinimum;
                                            range = controls[control].Bounds.dwMaximum - min;
                                            left = (DWORD)(*leftVolume * range) + min;
                                            right = (DWORD)(*rightVolume * range) + min;
                                            if (line.cChannels == 2) {
                                                values[0] = left;
                                                values[1] = right;
                                            } else {
                                                for (channel = 0; channel < line.cChannels; channel++) {
                                                    values[channel] = (left + right) / 2;
                                                }
                                            }
                                            if (audio_mixer_set_control_details((HMIXEROBJ)hMixer, &details, // [library:audio]
                                                                       MIXER_SETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR) {
                                                result = TRUE;
                                            }
                                        } else {
                                            if (audio_mixer_get_control_details((HMIXEROBJ)hMixer, &details, // [library:audio]
                                                                        MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR) {
                                                *leftVolume = (float)(values[0] - controls[control].Bounds.dwMinimum) /
                                                    (float)(controls[control].Bounds.dwMaximum - controls[control].Bounds.dwMinimum);
                                                if (line.cChannels == 2) {
                                                    *rightVolume = (float)(values[1] - controls[control].Bounds.dwMinimum) /
                                                        (float)(controls[control].Bounds.dwMaximum - controls[control].Bounds.dwMinimum);
                                                } else {
                                                    *rightVolume = *leftVolume;
                                                }
                                                result = TRUE;
                                            }
                                        }
                                        free(values);
                                    }
                                }
                            }
                            free(controls);
                        }
                    }
                }
            }
            audio_mixer_close(hMixer); // [library:audio]
        }
    }

    return result;
}

