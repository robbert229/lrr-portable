// SPDX-License-Identifier: LGPL-3.0-or-later
/* platform.h — the portability seam.
 *
 * Game-code below this line is platform-neutral. The only thing that varies
 * by backend is which header set satisfies the DX6-era API surface recorded
 * in docs/findings/15-compat-audit.md §1:
 *
 *   native : miniwin (https://github.com/isledecomp/isle-portable miniwin/)
 *            — source-compatible shim over SDL3, primary end-goal build
 *   winapi : the real DirectX headers from MinGW-w64 — the reference build,
 *            A/B-compared against the shipped binary under the A.2 harness
 *
 * Keep this file shallow: re-export the platform's windows/directdraw headers
 * under one roof, plus the platform-window + presentation glue that miniwin
 * deliberately leaves to the app (miniwin has no window-creation or blanket
 * present step: it is a framebuffer seam — window/events/present are app's
 * job, findings 15/16). Do not put game logic here (docs/adr/0003).
 */
#ifndef LEGORRR_PLATFORM_H
#define LEGORRR_PLATFORM_H

#include <stdint.h>
#include <cstddef>

#if defined(LEGORRR_BACKEND_MINIWIN)
#  include <miniwin/windows.h>
#  include <miniwin/ddraw.h>
#elif defined(LEGORRR_BACKEND_WINAPI)
#  include <windows.h>
/* TODO(winapi): vendored DirectDraw headers for the reference build
 * (MinGW-w64 ships no DirectDraw headers). The winapi adapter is A/B-only
 * (ADR 0004) and is not wired here yet. */
#else
#  error "no platform defined — set the meson option platform=native|winapi"
#endif

/* --- Platform window + presentation (backend-specific glue) --------------- */
/* miniwin exposes DDraw surfaces as framebuffers but never creates the
 * window nor presents; those are the platform glue's job. The winapi backend
 * would implement these with RegisterClassEx/CreateWindowEx + real Flip. */

typedef struct PlatformWindow PlatformWindow;

/* Active window and configured client limits mirrored for the input sampler. */
extern void* g_platform_cursor_window_handle;
extern int g_platform_cursor_width;
extern int g_platform_cursor_height;

/* --- Canonical key codes (platform-neutral event vocabulary) --------------
 * The engine's input tables (findings 04/05) are indexed by 1-byte codes.
 * Printable ASCII (0x20..0x7e) passes through; non-printable controls get
 * stable codes < 0x100 here, with each backend mapping its own scancodes. */
#define PLATFORM_KEY_UP    0x80
#define PLATFORM_KEY_DOWN  0x81
#define PLATFORM_KEY_LEFT  0x82
#define PLATFORM_KEY_RIGHT 0x83
#define PLATFORM_KEY_ESC   0x1b /* ASCII ESC */
#define PLATFORM_KEY_ENTER 0x0d /* ASCII CR (menu activate; backends map it) */

#define PLATFORM_EVENT_NONE    0
#define PLATFORM_EVENT_KEYDOWN 1
#define PLATFORM_EVENT_KEYUP   2
#define PLATFORM_EVENT_MOUSE   3
#define PLATFORM_EVENT_QUIT    4

/* Optional physical mouse vocabulary in PlatformEvent.code. Zero retains
 * the old aggregate event contract; consumers must not infer a button from
 * an untagged event. No PlatformEvent layout or native producer change. */
#define PLATFORM_MOUSE_TAG       0x1000
#define PLATFORM_MOUSE_LEFT      1
#define PLATFORM_MOUSE_RIGHT     2
#define PLATFORM_MOUSE_BUTTONS   3
#define PLATFORM_MOUSE_DOWN      0x10
#define PLATFORM_MOUSE_UP        0x20
#define PLATFORM_MOUSE_CANCEL    0x40
#define PLATFORM_MOUSE_HELD_SHIFT 8

typedef struct PlatformEvent {
    int kind;   /* PLATFORM_EVENT_* */
    int code;   /* canonical key code, or tagged PLATFORM_MOUSE_* */
    int mx;     /* absolute mouse x (MOUSE) */
    int my;     /* absolute mouse y (MOUSE) */
    int button; /* 1 = button-press edge (MOUSE); 0 = motion/release */
    int held;   /* 1 while a button is down at the event (MOUSE); 0 = up */
} PlatformEvent;

PlatformWindow* platform_create_window(const char* title, int width, int height);
void            platform_destroy_window(PlatformWindow* win);
void            platform_set_window_title(const char* title);

/* The DX-era HWND to hand to SetCooperativeLevel (miniwin reinterprets it
 * as an SDL_Window*). */
void* platform_window_hwnd(PlatformWindow* win);

int  platform_window_width(PlatformWindow* win);
int  platform_window_height(PlatformWindow* win);

/* Read the active window's cursor in client-relative coordinates. Returns
 * nonzero when the backend can produce a sample. */
int  platform_cursor_position_get(int* x, int* y);
void platform_cursor_position_set(int x, int y);

/* Pump platform events into `out` (up to cap); returns the number written.
 * A PLATFORM_EVENT_QUIT entry counts toward that total. */
int  platform_pump_events(PlatformWindow* win, PlatformEvent* out, int cap);

/* Convenience: pump and report whether the app should quit
 * (PLATFORM_EVENT_QUIT, or PLATFORM_EVENT_KEYDOWN for PLATFORM_KEY_ESC). */
int  platform_poll_events(PlatformWindow* win);

/* --- Audio (DirectSound analog, finding 14 §1) ---------------------------
 * The corpus's SFX path is DIRECTSOUNDCreate + buffers (WINMM mmio decode);
 * miniwin ships no DirectSound, so the reconstruction's audio surface here
 * is a PCM submit interface over SDL's audio stream (the winapi reference
 * build would map onto DirectSound ord 1). One shot per play; one loop
 * track; master gain (volume). */
typedef struct PlatformAudio PlatformAudio;

PlatformAudio* platform_audio_open(void);
void           platform_audio_close(PlatformAudio* a);

/* Play one shot: mono 16-bit signed PCM, `frames` frames at `sample_rate`
 * Hz. Returns 0 on success, -1 on failure (device/stream unavailable). */
int  platform_audio_play(PlatformAudio* a, const int16_t* pcm, int frames,
                         int sample_rate);
/* Submit a one-shot owned by a host-width voice token. Backends with
 * independently stoppable submissions retain the token for selective stop. */
int  platform_audio_play_voice(PlatformAudio* a, const int16_t* pcm,
                               int frames, int sample_rate, uintptr_t voice);
void platform_audio_stop_voice(PlatformAudio* a, uintptr_t voice);

/* Play one shot: interleaved L,R 16-bit signed PCM (stereo). */
int  platform_audio_play_stereo(PlatformAudio* a, const int16_t* lr,
                                int frames, int sample_rate);

/* Seed/replace the continuous loop track (interleaved L,R 16-bit PCM; the
 * engine re-seeds when pan/volume-source changes). Passing NULL stops it. */
int  platform_audio_loop(PlatformAudio* a, const int16_t* lr,
                         int frames, int sample_rate);

/* Keep the loop fed (call once per frame while looping). */
void platform_audio_refill(PlatformAudio* a);

/* Master gain 0..1 on the audio stream (volume, FUN_00464ef0 analog). */
int   platform_audio_set_gain(PlatformAudio* a, float gain);
float platform_audio_get_gain(PlatformAudio* a);

/* Bytes currently buffered in the stream (diagnostics/tests). */
int  platform_audio_queued(PlatformAudio* a);
/* Duration of queued output, in milliseconds; unlike queued bytes this is
 * comparable across backend sample-rate conversions. */
int  platform_audio_queued_milliseconds(PlatformAudio* a);

/* Stop / clear pending output + the loop track. */
void platform_audio_stop(PlatformAudio* a);

/* CD-DA is a separate Windows media device in the reference build. The
 * native backend reports it unavailable; it does not pretend an audio
 * stream or an uninstalled callback is a CD device. */
int  platform_cd_audio_open(int* error_code);
int  platform_cd_audio_set_tmsf(int* error_code);
int  platform_cd_audio_play(int one_based_track, int* error_code);
int  platform_cd_audio_stop(int* error_code);
int  platform_cd_audio_close(int* error_code);
int  platform_cd_audio_status(char* mode, int mode_capacity,
                              int* current_track, int* error_code);
int  platform_cd_audio_error_text(int error_code, char* output,
                                  int output_capacity);
uint32_t platform_time_milliseconds(void);

/* Commit a framebuffer (RGBA32 byte order R,G,B,A on little-endian) to the
 * window. `pixels`/`pitch` are what the DDraw surface Lock handed back. */
void platform_present_pixels(PlatformWindow* win,
                             const void* pixels, int pitch,
                             int width, int height);
typedef struct PlatformRenderSurfaceOps {
    void* context;
    int (*fullscreen_get)(void* context);
    void (*surface_flip)(void* context, PlatformWindow* win,
                         int first_flag, int second_flag);
    void (*windowed_blit)(void* context, PlatformWindow* win,
                          const void* pixels, int pitch,
                          int width, int height);
} PlatformRenderSurfaceOps;
void platform_render_surface_ops_set(const PlatformRenderSurfaceOps* ops);

/* Optional: dump the last presented framebuffer to a PPM for headless/CI
 * verification; no-op unless LEGORRR_SCREENSHOT is set. */
void platform_screenshot(PlatformWindow* win, const void* pixels, int pitch,
                         int width, int height, int frame);

#endif /* LEGORRR_PLATFORM_H */
