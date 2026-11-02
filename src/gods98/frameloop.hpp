// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — native frame loop (the visible-milestone driver).
 *
 * Mirrors the corpus's WinMain frame loop (finding 02/20) at the level the
 * reconstruction can stand today:
 *
 *   mount wads (exe-basename prefix) -> window + DDraw surface (platform
 *   seam + miniwin) -> load a couple of real corpus BMPs through the texture
 *   loader -> loop { pump events; EngineTick; lock; draw; present } ->
 *   EngineShutdown + teardown.
 *
 * Env knobs (headless/CI verification):
 *   LEGORRR_WAD_DIR        directory containing LegoRR*.wad (default: cwd)
 *   LEGORRR_AUTOSTOP_FRAMES  stop after N frames (0 = until quit/ESC)
 *   LEGORRR_SCREENSHOT      write a PPM of the last frame to this path
 *
 * The presentation step (miniwin has no surface Flip implementation) is the
 * platform glue's SDL_UpdateWindowSurface — documented divergence; the winapi
 * reference build keeps real DDraw Flip instead.
 */
#ifndef LEGORRR_FRAMELOOP_HPP
#define LEGORRR_FRAMELOOP_HPP

#include <stdint.h>
#include <stddef.h>
#include "platform/platform.h"

typedef struct MainLoopFramePumpHooks {
    void* context;
    void (*window_messages)(void* context);
    void (*input_poll)(void* context);
    void (*renderer_begin)(void* context);
    void (*renderer_present)(void* context);
    void (*projection_overlay)(void* context, int first, int second);
} MainLoopFramePumpHooks;
void main_loop_frame_pump_hooks_set(const MainLoopFramePumpHooks* hooks);
void main_loop_frame_pump(int draw_projection_overlay); /* FUN_00477f30 */

typedef struct WindowPortRect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} WindowPortRect;

typedef struct WindowRuntimeState {
    void* owner;
    void* window;
    uint32_t flags;
    uint32_t app_active;
    uint32_t close_requested;
    uint32_t left_button_down;
    uint32_t right_button_down;
    uint32_t mouse_capture;
    uint32_t mouse_activity;
    uint32_t left_double_click;
    uint32_t right_double_click;
    /* Mouse words written by the retail window procedures at 0x76ab3c..58. */
    uint32_t mouse_word_3c;
    uint32_t mouse_word_40;
    uint32_t mouse_word_4c;
    uint32_t mouse_word_50;
    uint32_t mouse_word_54;
    uint32_t mouse_word_58;
    int32_t window_style;
    size_t callback_triplet[3];
    uint32_t callback_triplet_valid;
} WindowRuntimeState;
extern WindowRuntimeState s_window_runtime_state;
extern uint32_t s_window_application_active; /* DAT_00505808 */
extern uint32_t s_window_close_requested; /* DAT_0050580c */

typedef struct WindowRuntimeOps {
    void* context;
    int (*device_caps_query)(void* context, void* window, int index);
    int (*adjust_window_rect)(void* context, WindowPortRect* rect,
                              int32_t style, int has_menu);
    void (*mouse_capture)(void* context, void* window);
    void (*mouse_release)(void* context);
} WindowRuntimeOps;

typedef ptrdiff_t (*WindowDefaultProc)(void* context, void* window,
    uint32_t message, size_t wparam, ptrdiff_t lparam);

#if defined(_MSC_VER)
#define LEGORRR_WINDOW_PROC_CALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define LEGORRR_WINDOW_PROC_CALL __attribute__((stdcall))
#else
#define LEGORRR_WINDOW_PROC_CALL
#endif

void window_runtime_ops_set(const WindowRuntimeOps* ops);
void window_runtime_ops_install_defaults(const WindowRuntimeOps* ops);
void window_runtime_default_proc_set(WindowDefaultProc proc, void* context);
void window_runtime_window_handle_set(void* window);
void window_runtime_mode_set(int fullscreen, int32_t style);
void window_runtime_flags_set(uint32_t flags);
uint32_t window_runtime_flags_get(void);
const WindowRuntimeState* window_runtime_state_get(void);
void window_runtime_state_initialize(void* window);
void window_state_initialize_retail(void* window); /* FUN_0047c170 */
void window_runtime_display_caps_query(void); /* 0x00477f80 */
int window_runtime_callback_triplet_set(const size_t* triplet);
int window_runtime_callback_triplet_get(size_t triplet[3]);
int window_runtime_message_dispatch_filter(const void* message); /* 0x00478010 */
void window_runtime_adjust_rect(WindowPortRect* rect);
ptrdiff_t window_runtime_game_window_proc(void* window, uint32_t message,
    size_t wparam, ptrdiff_t lparam);
ptrdiff_t window_runtime_activation_close_proc(void* window,
    uint32_t message,
    size_t wparam, ptrdiff_t lparam);

/* Run the engine frame loop on a window. Returns 0 on clean exit. */
int LegorrrRun(void);
/* One-time application setup, split from recurring frame work. */
int frameloop_application_init(void);
int frameloop_application_tick(float dt);
void frameloop_application_events_set(const PlatformEvent* events, int count);
PlatformWindow* frameloop_application_window_get(void);
void frameloop_application_shutdown(void);
void frameloop_application_start_level_override_set(const char* level_name);
void frameloop_application_sound_disabled_set(int disabled);
void window_title_set(const char* title);

#endif /* LEGORRR_FRAMELOOP_HPP */
