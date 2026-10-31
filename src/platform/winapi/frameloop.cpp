// SPDX-License-Identifier: LGPL-3.0-or-later
/* winapi platform glue — Win32 window/message APIs with presentation calls
 * delegated through the installable rendering interface.
 */
#include "platform/platform.h"
#include "gods98/frameloop.hpp"

#include <stdlib.h>
#include <string.h>

struct PlatformWindow {
    HWND hwnd;
    int width;
    int height;
};

static const char s_platform_window_class[] = "LegoRRPortableWindow";
static PlatformWindow* s_active_window;

namespace unported_ledger_stubs {
ptrdiff_t ledger_stub_004784c0_GameWindowProcedure(
    void* window, uint32_t message, size_t wparam, ptrdiff_t lparam);
int ledger_stub_00478010_WindowMessageDispatchFilter(uint32_t message);
}

static ptrdiff_t platform_default_window_proc(void* context, void* window,
    uint32_t message, size_t wparam, ptrdiff_t lparam)
{
    (void)context;
    return (ptrdiff_t)DefWindowProcA((HWND)window, (UINT)message,
                                   (WPARAM)wparam, (LPARAM)lparam);
}

static int platform_device_caps_query(void* context, void* window, int index)
{
    HDC dc;
    int value;
    (void)context;
    dc = GetDC((HWND)window);
    if (dc == NULL) return 0;
    value = GetDeviceCaps(dc, index);
    ReleaseDC((HWND)window, dc);
    return value;
}

static int platform_adjust_window_rect(void* context, WindowPortRect* port_rect,
                                       int32_t style, int has_menu)
{
    RECT rect;
    (void)context;
    if (port_rect == NULL) return 0;
    rect.left = port_rect->left;
    rect.top = port_rect->top;
    rect.right = port_rect->right;
    rect.bottom = port_rect->bottom;
    if (!AdjustWindowRect(&rect, (DWORD)style, has_menu != 0)) return 0;
    port_rect->left = rect.left;
    port_rect->top = rect.top;
    port_rect->right = rect.right;
    port_rect->bottom = rect.bottom;
    return 1;
}

static void platform_mouse_capture(void* context, void* window)
{
    (void)context;
    SetCapture((HWND)window);
}

static void platform_mouse_release(void* context)
{
    (void)context;
    ReleaseCapture();
}

static LRESULT CALLBACK platform_window_proc(HWND hwnd, UINT message,
                                              WPARAM wparam, LPARAM lparam)
{
    if (s_active_window != NULL && s_active_window->hwnd == hwnd &&
        message == WM_SIZE) {
        s_active_window->width = (int)(lparam & 0xffff);
        s_active_window->height = (int)((lparam >> 16) & 0xffff);
    }
    return (LRESULT)unported_ledger_stubs::ledger_stub_004784c0_GameWindowProcedure(
        (void*)hwnd, (uint32_t)message, (size_t)wparam,
        (ptrdiff_t)lparam);
}

static PlatformRenderSurfaceOps s_platform_render_surface_ops;

void platform_render_surface_ops_set(const PlatformRenderSurfaceOps* ops)
{
    if (ops != NULL) s_platform_render_surface_ops = *ops;
    else memset(&s_platform_render_surface_ops, 0,
                sizeof(s_platform_render_surface_ops));
}

// FUNCTION: LEGORR 0x004783f0
PlatformWindow* platform_create_window(const char* title, int width, int height)
{
    WNDCLASSA wc;
    ATOM class_atom;
    HWND hwnd;
    PlatformWindow* win;
    WindowRuntimeOps window_ops;
    HINSTANCE instance = GetModuleHandleA(NULL);
    memset(&window_ops, 0, sizeof(window_ops));
    window_ops.device_caps_query = platform_device_caps_query;
    window_ops.adjust_window_rect = platform_adjust_window_rect;
    window_ops.mouse_capture = platform_mouse_capture;
    window_ops.mouse_release = platform_mouse_release;
    window_runtime_ops_install_defaults(&window_ops);
    memset(&wc, 0, sizeof(wc));
    wc.style = 8;
    wc.lpfnWndProc = platform_window_proc;
    wc.hInstance = instance;
    wc.lpszClassName = s_platform_window_class;
    class_atom = RegisterClassA(&wc);
    if (class_atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return NULL;
    }
    window_runtime_default_proc_set(platform_default_window_proc, NULL);
    if (title == NULL) title = "Lego Rock Raiders";
    hwnd = CreateWindowExA(0x00040000u, s_platform_window_class, title,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        width > 0 ? width : 640, height > 0 ? height : 480,
        NULL, NULL, instance, NULL);
    if (hwnd == NULL) return NULL;
    win = (PlatformWindow*)calloc(1, sizeof(*win));
    if (win == NULL) {
        DestroyWindow(hwnd);
        return NULL;
    }
    win->hwnd = hwnd;
    win->width = width > 0 ? width : 640;
    win->height = height > 0 ? height : 480;
    s_active_window = win;
    window_runtime_window_handle_set((void*)hwnd);
    SetFocus(hwnd);
    return win;
}

void platform_destroy_window(PlatformWindow* win)
{
    if (win == NULL) return;
    if (win->hwnd != NULL) DestroyWindow(win->hwnd);
    if (s_active_window == win) {
        s_active_window = NULL;
        window_runtime_window_handle_set(NULL);
    }
    free(win);
}

void platform_set_window_title(const char* title)
{
    if (s_active_window != NULL && s_active_window->hwnd != NULL &&
        title != NULL) SetWindowTextA(s_active_window->hwnd, title);
}

void* platform_window_hwnd(PlatformWindow* win)
{
    return win != NULL ? (void*)win->hwnd : NULL;
}

int platform_window_width(PlatformWindow* win)
{
    return win != NULL ? win->width : 0;
}

int platform_window_height(PlatformWindow* win)
{
    return win != NULL ? win->height : 0;
}

int platform_poll_events(PlatformWindow* win)
{
    PlatformEvent events[32];
    int count = platform_pump_events(win, events, 32);
    int i;
    for (i = 0; i < count; ++i) {
        if (events[i].kind == PLATFORM_EVENT_QUIT ||
            (events[i].kind == PLATFORM_EVENT_KEYDOWN &&
             events[i].code == PLATFORM_KEY_ESC)) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00478040
int platform_pump_events(PlatformWindow* win, PlatformEvent* out, int cap)
{
    MSG message;
    int count = 0;
    if (win == NULL || out == NULL || cap <= 0) return 0;
    while (count < cap && PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        PlatformEvent* event = &out[count];
        int accepted = unported_ledger_stubs::ledger_stub_00478010_WindowMessageDispatchFilter(
            (uint32_t)message.message);
        if (message.message == WM_QUIT) {
            event->kind = PLATFORM_EVENT_QUIT;
            event->code = event->mx = event->my = 0;
            event->button = event->held = 0;
            ++count;
            continue;
        }
        if (accepted) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        event->kind = PLATFORM_EVENT_NONE;
        event->code = event->mx = event->my = 0;
        event->button = event->held = 0;
        if (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN ||
            message.message == WM_KEYUP || message.message == WM_SYSKEYUP) {
            int code = 0;
            if (message.wParam >= 0x20 && message.wParam <= 0x7e)
                code = (int)message.wParam;
            else if (message.wParam == VK_UP) code = PLATFORM_KEY_UP;
            else if (message.wParam == VK_DOWN) code = PLATFORM_KEY_DOWN;
            else if (message.wParam == VK_LEFT) code = PLATFORM_KEY_LEFT;
            else if (message.wParam == VK_RIGHT) code = PLATFORM_KEY_RIGHT;
            else if (message.wParam == VK_ESCAPE) code = PLATFORM_KEY_ESC;
            else if (message.wParam == VK_RETURN) code = PLATFORM_KEY_ENTER;
            if (accepted && code != 0) {
                event->kind = (message.message == WM_KEYDOWN ||
                               message.message == WM_SYSKEYDOWN)
                    ? PLATFORM_EVENT_KEYDOWN : PLATFORM_EVENT_KEYUP;
                event->code = code;
            }
        }
        else if (message.message == WM_MOUSEMOVE ||
                 message.message == WM_LBUTTONDOWN ||
                 message.message == WM_LBUTTONUP ||
                 message.message == WM_RBUTTONDOWN ||
                 message.message == WM_RBUTTONUP) {
            event->kind = PLATFORM_EVENT_MOUSE;
            event->mx = (short)(message.lParam & 0xffff);
            event->my = (short)((message.lParam >> 16) & 0xffff);
            event->button = (message.message == WM_LBUTTONDOWN ||
                            message.message == WM_RBUTTONDOWN) ? 1 : 0;
            event->held = (message.wParam & (MK_LBUTTON | MK_RBUTTON)) != 0;
        }
        else if (message.message == WM_CLOSE) {
            event->kind = PLATFORM_EVENT_QUIT;
        }
        if (event->kind != PLATFORM_EVENT_NONE) ++count;
    }
    return count;
}

// FUNCTION: LEGORR 0x0047c8d0
void platform_present_pixels(PlatformWindow* win, const void* pixels, int pitch,
                             int width, int height)
{
    int fullscreen = 0;
    if (s_platform_render_surface_ops.fullscreen_get != NULL) {
        fullscreen = s_platform_render_surface_ops.fullscreen_get(
            s_platform_render_surface_ops.context);
    }
    if (fullscreen) {
        if (s_platform_render_surface_ops.surface_flip != NULL) {
            s_platform_render_surface_ops.surface_flip(
                s_platform_render_surface_ops.context, win, 0, 1);
        }
    }
    else if (s_platform_render_surface_ops.windowed_blit != NULL) {
        s_platform_render_surface_ops.windowed_blit(
            s_platform_render_surface_ops.context, win, pixels, pitch,
            width, height);
    }
}

void platform_screenshot(PlatformWindow* win, const void* pixels, int pitch,
                         int width, int height, int frame)
{
    (void)win; (void)pixels; (void)pitch; (void)width; (void)height;
    (void)frame;
}

PlatformAudio* platform_audio_open(void)
{
    return NULL;
}

void platform_audio_close(PlatformAudio* a)
{
    (void)a;
}

int platform_audio_play(PlatformAudio* a, const int16_t* pcm, int frames,
                        int sample_rate)
{
    (void)a; (void)pcm; (void)frames; (void)sample_rate;
    return -1;
}

int platform_audio_play_stereo(PlatformAudio* a, const int16_t* lr,
                               int frames, int sample_rate)
{
    (void)a; (void)lr; (void)frames; (void)sample_rate;
    return -1;
}

int platform_audio_loop(PlatformAudio* a, const int16_t* lr,
                        int frames, int sample_rate)
{
    (void)a; (void)lr; (void)frames; (void)sample_rate;
    return -1;
}

void platform_audio_refill(PlatformAudio* a)
{
    (void)a;
}

int platform_audio_set_gain(PlatformAudio* a, float gain)
{
    (void)a; (void)gain;
    return -1;
}

float platform_audio_get_gain(PlatformAudio* a)
{
    (void)a;
    return 0.0f;
}

int platform_audio_queued(PlatformAudio* a)
{
    (void)a;
    return 0;
}

void platform_audio_stop(PlatformAudio* a)
{
    (void)a;
}
