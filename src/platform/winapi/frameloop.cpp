// SPDX-License-Identifier: LGPL-3.0-or-later
/* winapi platform glue — Win32 window/message APIs with presentation calls
 * delegated through the installable rendering interface.
 */
#include "platform/platform.h"
#include "gods98/frameloop.hpp"
#include "gods98/surface.hpp"
#include "gods98/view.hpp"
#include "crt/crt.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mmsystem.h>

#if !defined(_WIN64)
/* Use the bundled DLL, as the retail Direct3DRMDeviceInit does. The old
 * SDK used by the MSVC5 build need not supply the newer RM3 declarations;
 * these are the stdcall slots from IDirect3DRM3/IDirect3DRMFrame3. */
static HMODULE s_scene_rm_module;
static void* s_scene_rm_factory;
static const GUID s_scene_rm3_iid = {
    0x4516ec83, 0x8f20, 0x11d0,
    { 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3 }
};

void scene_winapi_interface_release(void* object)
{
    typedef ULONG (WINAPI *ReleaseFn)(void*);
    if (object != NULL) {
        void** vtable = *(void***)object;
        ((ReleaseFn)vtable[2])(object);
    }
}

/* The device and retained viewport are borrowed. The returned RM viewport
 * owns a QI reference; a later failure can leave the current viewport changed. */
int scene_winapi_user_visual_context_open(void* immediate_device,
    void* retained_viewport, float transform[16], void** parameter_out,
    void** interface_out)
{
    typedef HRESULT (WINAPI *TransformFn)(void*, DWORD, float*);
    typedef HRESULT (WINAPI *GetViewportFn)(void*, void**);
    typedef HRESULT (WINAPI *QueryFn)(void*, const GUID*, void**);
    typedef HRESULT (WINAPI *SetViewportFn)(void*, void*);
    typedef DWORD (WINAPI *GetAppDataFn)(void*);
    static const GUID iid_viewport3 = { 0xb0ab3b61, 0x33d7, 0x11d1,
        { 0xa9, 0x81, 0x00, 0xc0, 0x4f, 0xd7, 0xb1, 0x74 } };
    static const GUID iid_rm_viewport2 = { 0x4a1b1be6, 0xbfed, 0x11d1,
        { 0x8e, 0xd8, 0x00, 0xa0, 0xc9, 0x67, 0xa4, 0x82 } };
    void* legacy = NULL;
    void* viewport3 = NULL;
    void* rm_viewport2 = NULL;
    HRESULT result;
    if (parameter_out != NULL) *parameter_out = NULL;
    if (interface_out != NULL) *interface_out = NULL;
    if (immediate_device == NULL || retained_viewport == NULL ||
        transform == NULL || parameter_out == NULL || interface_out == NULL ||
        parameter_out == interface_out) return 0;
    result = ((TransformFn)(*(void***)immediate_device)[0x68 / 4])(
        immediate_device, 1, transform);
    if (result < 0) return 0;
    result = ((GetViewportFn)(*(void***)retained_viewport)[0x98 / 4])(
        retained_viewport, &legacy);
    if (result < 0 || legacy == NULL) {
        scene_winapi_interface_release(legacy);
        return 0;
    }
    result = ((QueryFn)(*(void***)legacy)[0])(legacy, &iid_viewport3, &viewport3);
    scene_winapi_interface_release(legacy);
    if (result < 0 || viewport3 == NULL) {
        scene_winapi_interface_release(viewport3);
        return 0;
    }
    result = ((SetViewportFn)(*(void***)immediate_device)[0x30 / 4])(
        immediate_device, viewport3);
    scene_winapi_interface_release(viewport3);
    if (result < 0) return 0;
    result = ((QueryFn)(*(void***)retained_viewport)[0])(
        retained_viewport, &iid_rm_viewport2, &rm_viewport2);
    if (result < 0 || rm_viewport2 == NULL) {
        scene_winapi_interface_release(rm_viewport2);
        return 0;
    }
    *parameter_out = (void*)((GetAppDataFn)(*(void***)rm_viewport2)[0x1c / 4])(
        rm_viewport2);
    *interface_out = rm_viewport2;
    return 1;
}

void scene_winapi_shutdown(void)
{
    scene_winapi_interface_release(s_scene_rm_factory);
    s_scene_rm_factory = NULL;
    if (s_scene_rm_module != NULL) FreeLibrary(s_scene_rm_module);
    s_scene_rm_module = NULL;
}

int scene_winapi_initialize(void)
{
    typedef HRESULT (WINAPI *CreateFn)(void**);
    typedef HRESULT (WINAPI *QueryFn)(void*, const GUID*, void**);
    void* rm = NULL;
    CreateFn create;
    HRESULT result;
    char dll_path[MAX_PATH];
    DWORD path_length;
    if (s_scene_rm_factory != NULL) return 0;
    /* The launch working directory owns the shipped runtime assets. Bare
     * LoadLibrary searches the candidate EXE/system directories before it,
     * unlike the retail EXE's adjacent imported DLL. Resolve that asset
     * explicitly so both executables use the same native RM implementation. */
    path_length = GetFullPathNameA("D3DRM.DLL", sizeof(dll_path),
                                   dll_path, NULL);
    if (path_length == 0 || path_length >= sizeof(dll_path)) {
        fprintf(stderr, "scene: D3DRM asset path failed length=%lu error=%lu\n",
                path_length, GetLastError());
        return 0;
    }
    s_scene_rm_module = LoadLibraryA(dll_path);
    if (s_scene_rm_module == NULL) {
        fprintf(stderr, "scene: D3DRM asset load failed path=%s error=%lu\n",
                dll_path, GetLastError());
        return 0;
    }
    create = (CreateFn)GetProcAddress(s_scene_rm_module, "Direct3DRMCreate");
    if (create == NULL || create(&rm) != 0 || rm == NULL) {
        scene_winapi_interface_release(rm);
        scene_winapi_shutdown();
        return 0;
    }
    result = ((QueryFn)(*(void***)rm)[0])(
        rm, &s_scene_rm3_iid, &s_scene_rm_factory);
    scene_winapi_interface_release(rm);
    if (result != 0 || s_scene_rm_factory == NULL) {
        scene_winapi_shutdown();
        return 0;
    }
    return 1;
}

int scene_winapi_user_visual_create(SceneUserVisualCallback callback,
                                    void* cookie, void** visual)
{
    typedef HRESULT (WINAPI *CreateUserVisualFn)(void*, SceneUserVisualCallback,
                                                void*, void**);
    HRESULT result;
    void* created = NULL;
    if (visual == NULL) return 0;
    *visual = NULL;
    if (s_scene_rm_factory == NULL || callback == NULL) return 0;
    result = ((CreateUserVisualFn)(*(void***)s_scene_rm_factory)[0x58 / 4])(
        s_scene_rm_factory, callback, cookie, &created);
    if (result < 0 || created == NULL) {
        scene_winapi_interface_release(created);
        return 0;
    }
    *visual = created;
    return 1;
}

int scene_winapi_device3_acquire(void* retained_device, void** immediate)
{
    typedef HRESULT (WINAPI *DeviceGetFn)(void*, void**);
    typedef HRESULT (WINAPI *QueryFn)(void*, const GUID*, void**);
    static const GUID iid_device3 = {
        0xb0ab3b60, 0x33d7, 0x11d1,
        { 0xa9, 0x81, 0x00, 0xc0, 0x4f, 0xd7, 0xb1, 0x74 }
    };
    void* device2 = NULL;
    void* device3 = NULL;
    HRESULT result;
    if (immediate == NULL) return 0;
    *immediate = NULL;
    if (retained_device == NULL) return 0;
    /* Direct3DRMDeviceInit47828d..4782ad uses the same RM3 slot/IID. */
    result = ((DeviceGetFn)(*(void***)retained_device)[0x98 / 4])(
        retained_device, &device2);
    if (result < 0 || device2 == NULL) {
        scene_winapi_interface_release(device2);
        return 0;
    }
    result = ((QueryFn)(*(void***)device2)[0])(
        device2, &iid_device3, &device3);
    scene_winapi_interface_release(device2);
    if (result < 0 || device3 == NULL) {
        scene_winapi_interface_release(device3);
        return 0;
    }
    *immediate = device3;
    return 1;
}

int scene_winapi_frame_create(void* parent, void** frame)
{
    typedef HRESULT (WINAPI *CreateFrameFn)(void*, void*, void**);
    return (int)((CreateFrameFn)(*(void***)s_scene_rm_factory)[0x10 / 4])(
        s_scene_rm_factory, parent, frame);
}

void scene_winapi_frame_release(void* frame)
{
    typedef HRESULT (WINAPI *ParentGetFn)(void*, void**);
    typedef HRESULT (WINAPI *DeleteChildFn)(void*, void*);
    void* parent = NULL;
    void** vtable = *(void***)frame;
    if (((ParentGetFn)vtable[0x5c / 4])(frame, &parent) == 0 &&
        parent != NULL) {
        ((DeleteChildFn)(*(void***)parent)[0x94 / 4])(parent, frame);
    }
    scene_winapi_interface_release(parent);
    scene_winapi_interface_release(frame);
}

int scene_winapi_light_create(unsigned int kind, const float rgb[3],
                               void** light)
{
    typedef HRESULT (WINAPI *CreateLightFn)(void*, unsigned int, float,
                                           float, float, void**);
    return (int)((CreateLightFn)(*(void***)s_scene_rm_factory)[0x30 / 4])(
        s_scene_rm_factory, kind, rgb[0], rgb[1], rgb[2], light);
}
#endif

struct PlatformWindow {
    HWND hwnd;
    /* Client pixels are presentation geometry, never input/UI limits. */
    int width;
    int height;
    int frame_width;
    int frame_height;
    int pointer_x;
    int pointer_y;
    int pointer_cancel_pending;
    int pointer_release_expected;
};

static const char s_platform_window_class[] = "LegoRRPortableWindow";
static PlatformWindow* s_active_window;
// GLOBAL: LEGORR 0x00505800
void* g_platform_cursor_window_handle;
int g_platform_cursor_width;
int g_platform_cursor_height;

static void platform_frame_dimensions_set(PlatformWindow* win,
                                           int width, int height)
{
    win->frame_width = width;
    win->frame_height = height;
    if (win == s_active_window) {
        g_platform_cursor_width = width;
        g_platform_cursor_height = height;
        display_dimensions_set(width, height);
    }
}

static int platform_client_dimensions_update(PlatformWindow* win)
{
    RECT client;
    if (win == NULL || win->hwnd == NULL ||
        !GetClientRect(win->hwnd, &client)) return 0;
    win->width = client.right - client.left;
    win->height = client.bottom - client.top;
    return !IsIconic(win->hwnd) && win->width > 0 && win->height > 0;
}

static int platform_coordinate_clamp(int value, int extent)
{
    if (value < 0) return 0;
    if (value >= extent) return extent - 1;
    return value;
}

/* StretchDIBits fills the entire client. Use the same per-axis ratio for
 * messages and GetCursorPos, clamping captured/outside positions first.
 * int64_t is __int64 in the VC5 compatibility header. */
static int platform_client_to_frame(PlatformWindow* win, int x, int y)
{
    if (!platform_client_dimensions_update(win) ||
        win->frame_width <= 0 || win->frame_height <= 0) return 0;
    x = platform_coordinate_clamp(x, win->width);
    y = platform_coordinate_clamp(y, win->height);
    win->pointer_x = (int)((int64_t)x * win->frame_width / win->width);
    win->pointer_y = (int)((int64_t)y * win->frame_height / win->height);
    return 1;
}

/* Select the first client pixel in a logical pixel's bucket. Rounding up
 * makes get(set(x)) == x when the client is at least as large as the frame.
 * A smaller client necessarily quantizes some logical positions. */
static int platform_frame_to_client(int value, int frame, int client)
{
    int result;
    value = platform_coordinate_clamp(value, frame);
    result = (int)(((int64_t)value * client + frame - 1) / frame);
    return platform_coordinate_clamp(result, client);
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
    ptrdiff_t result;
    if (s_active_window != NULL && s_active_window->hwnd == hwnd &&
        message == WM_SIZE) {
        s_active_window->width = (int)(lparam & 0xffff);
        s_active_window->height = (int)((lparam >> 16) & 0xffff);
    }
    /* Focus/capture messages can be sent synchronously, bypassing PeekMessage.
     * Deliver their cancellation through the same ordered event ingress. */
    if (s_active_window != NULL && s_active_window->hwnd == hwnd &&
        (message == WM_KILLFOCUS || message == WM_CANCELMODE ||
         (message == WM_ACTIVATEAPP && wparam == 0) ||
         (message == WM_CAPTURECHANGED &&
          !s_active_window->pointer_release_expected)))
        s_active_window->pointer_cancel_pending = 1;
    /* Retail's registered LAB_00478880 trampoline selects the handler from
     * DAT_00505f84 bit 1: fullscreen uses GameWindowProcedure; windowed mode
     * uses MainWindowActivationAndCloseHandler (which captures the mouse). */
    if ((window_runtime_flags_get() & 2u) != 0u) {
        result = window_runtime_game_window_proc(
            (void*)hwnd, (uint32_t)message, (size_t)wparam,
            (ptrdiff_t)lparam);
    }
    else {
        result = window_runtime_activation_close_proc(
            (void*)hwnd, (uint32_t)message, (size_t)wparam,
            (ptrdiff_t)lparam);
    }
    return (LRESULT)result;
}

static PlatformRenderSurfaceOps s_platform_render_surface_ops;
static unsigned char* s_platform_present_bgra;
static size_t s_platform_present_capacity;

void platform_render_surface_ops_set(const PlatformRenderSurfaceOps* ops)
{
    if (ops != NULL) s_platform_render_surface_ops = *ops;
    else memset(&s_platform_render_surface_ops, 0,
                sizeof(s_platform_render_surface_ops));
}

// PORT-ONLY: the port's Windows window; Main.c's Main_InitApp (004783f0) is in gods98/main.c (G98-005).
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
    s_active_window = win;
    g_platform_cursor_window_handle = (void*)hwnd;
    platform_frame_dimensions_set(win, width > 0 ? width : 640,
                                   height > 0 ? height : 480);
    /* CreateWindow's synchronous WM_SIZE preceded s_active_window. */
    (void)platform_client_dimensions_update(win);
    window_runtime_state_initialize((void*)hwnd);
    SetFocus(hwnd);
    return win;
}

void platform_destroy_window(PlatformWindow* win)
{
    if (win == NULL) return;
    if (win->hwnd != NULL) DestroyWindow(win->hwnd);
    if (s_active_window == win) {
        s_active_window = NULL;
        g_platform_cursor_window_handle = NULL;
        g_platform_cursor_width = 0;
        g_platform_cursor_height = 0;
        window_runtime_state_initialize(NULL);
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

int platform_cursor_position_get(int* x, int* y)
{
    /* Client-relative input is measured in retained framebuffer pixels. */
    POINT cursor;
    PlatformWindow* win = s_active_window;
    if (win == NULL || x == NULL || y == NULL ||
        !GetCursorPos(&cursor) || !ScreenToClient(win->hwnd, &cursor) ||
        !platform_client_to_frame(win, cursor.x, cursor.y)) {
        return 0;
    }
    *x = win->pointer_x;
    *y = win->pointer_y;
    return 1;
}

void platform_cursor_position_set(int x, int y)
{
    PlatformWindow* win = s_active_window;
    POINT point;
    if (!platform_client_dimensions_update(win) ||
        win->frame_width <= 0 || win->frame_height <= 0) return;
    point.x = platform_frame_to_client(x, win->frame_width, win->width);
    point.y = platform_frame_to_client(y, win->frame_height, win->height);
    if (!ClientToScreen(win->hwnd, &point)) return;
    SetCursorPos(point.x, point.y);
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

int platform_pump_events(PlatformWindow* win, PlatformEvent* out, int cap)
{
    MSG message;
    int count = 0;
    int pointer_cancelled = 0;
    if (win == NULL || out == NULL || cap <= 0) return 0;
    while (count < cap) {
        PlatformEvent* event = &out[count];
        int accepted;
        int have_message;
        if (win->pointer_cancel_pending) {
            pointer_cancelled = 1;
            win->pointer_cancel_pending = 0;
        }
        /* PeekMessage can dispatch a sent cancellation even when it returns
         * no queued message. Preserve that cancellation in this batch. */
        have_message = PeekMessageA(&message, NULL, 0, 0, PM_REMOVE);
        if (win->pointer_cancel_pending) {
            pointer_cancelled = 1;
            win->pointer_cancel_pending = 0;
        }
        if (!have_message) break;
        accepted = window_runtime_message_dispatch_filter(&message);
        if (message.message == WM_QUIT) {
            event->kind = PLATFORM_EVENT_QUIT;
            event->code = event->mx = event->my = 0;
            event->button = event->held = 0;
            ++count;
            continue;
        }
        if (accepted) {
            /* The retail handler releases capture on either button's up.
             * That expected notification is not a cancellation of the other
             * physical button; retain capture while it remains held. */
            win->pointer_release_expected =
                message.message == WM_LBUTTONUP ||
                message.message == WM_RBUTTONUP;
            TranslateMessage(&message);
            DispatchMessageA(&message);
            if (win->pointer_release_expected &&
                (message.wParam & (MK_LBUTTON | MK_RBUTTON)) != 0)
                SetCapture(win->hwnd);
            win->pointer_release_expected = 0;
        }
        event->kind = PLATFORM_EVENT_NONE;
        event->code = event->mx = event->my = 0;
        event->button = event->held = 0;
        if (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN ||
            message.message == WM_KEYUP || message.message == WM_SYSKEYUP) {
            int code = 0;
            /* Navigation VKs overlap printable ASCII. Translate them before
             * the legacy printable fallback, on both press and release. */
            if (message.wParam == VK_UP) code = PLATFORM_KEY_UP;
            else if (message.wParam == VK_DOWN) code = PLATFORM_KEY_DOWN;
            else if (message.wParam == VK_LEFT) code = PLATFORM_KEY_LEFT;
            else if (message.wParam == VK_RIGHT) code = PLATFORM_KEY_RIGHT;
            else if (message.wParam == VK_ESCAPE) code = PLATFORM_KEY_ESC;
            else if (message.wParam == VK_RETURN) code = PLATFORM_KEY_ENTER;
            /* OEM VKs sit above printable ASCII; the camera zoom keys
             * (DIK_MINUS/DIK_EQUALS in the original) use '-' and '='. */
            else if (message.wParam == 0xbd) code = '-'; /* VK_OEM_MINUS */
            else if (message.wParam == 0xbb) code = '='; /* VK_OEM_PLUS */
            else if (message.wParam >= 'A' && message.wParam <= 'Z')
                code = (int)message.wParam - 'A' + 'a';
            else if (message.wParam >= 0x20 && message.wParam <= 0x7e)
                code = (int)message.wParam;
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
            if (!platform_client_to_frame(win,
                (short)(message.lParam & 0xffff),
                (short)((message.lParam >> 16) & 0xffff))) {
                if (message.message != WM_LBUTTONUP &&
                    message.message != WM_RBUTTONUP) continue;
            }
            /* Preserve received releases with the last logical pair when
             * minimized; no new press may activate a stale hit target. */
            event->mx = win->pointer_x;
            event->my = win->pointer_y;
            event->button = (message.message == WM_LBUTTONDOWN ||
                            message.message == WM_RBUTTONDOWN) ? 1 : 0;
            event->held = (message.wParam & (MK_LBUTTON | MK_RBUTTON)) != 0;
            /* Preserve physical identity and each removed message's order.
             * button/held remain the aggregate menu compatibility view. */
            int buttons = 0;
            if ((message.wParam & MK_LBUTTON) != 0)
                buttons |= PLATFORM_MOUSE_LEFT;
            if ((message.wParam & MK_RBUTTON) != 0)
                buttons |= PLATFORM_MOUSE_RIGHT;
            event->code = PLATFORM_MOUSE_TAG |
                (buttons << PLATFORM_MOUSE_HELD_SHIFT);
            if (message.message == WM_LBUTTONDOWN ||
                message.message == WM_LBUTTONUP)
                event->code |= PLATFORM_MOUSE_LEFT;
            if (message.message == WM_RBUTTONDOWN ||
                message.message == WM_RBUTTONUP)
                event->code |= PLATFORM_MOUSE_RIGHT;
            if (event->button) event->code |= PLATFORM_MOUSE_DOWN;
            if (message.message == WM_LBUTTONUP ||
                message.message == WM_RBUTTONUP)
                event->code |= PLATFORM_MOUSE_UP;
        }
        else if (message.message == WM_KILLFOCUS ||
                 message.message == WM_CANCELMODE ||
                 (message.message == WM_ACTIVATEAPP && message.wParam == 0)) {
            event->kind = PLATFORM_EVENT_MOUSE;
            event->mx = win->pointer_x;
            event->my = win->pointer_y;
            event->code = PLATFORM_MOUSE_TAG | PLATFORM_MOUSE_CANCEL;
        }
        else if (message.message == WM_CLOSE) {
            event->kind = PLATFORM_EVENT_QUIT;
        }
        if (win->pointer_cancel_pending ||
            (event->kind == PLATFORM_EVENT_MOUSE &&
             (event->code & PLATFORM_MOUSE_CANCEL) != 0)) {
            pointer_cancelled = 1;
            win->pointer_cancel_pending = 0;
        }
        if (event->kind != PLATFORM_EVENT_NONE) ++count;
    }
    if (win->pointer_cancel_pending) pointer_cancelled = 1;
    if (pointer_cancelled) {
        int kept = 0;
        int i;
        /* Cancel the whole pointer batch, including a DOWN collected before
         * a synchronous focus loss. Keep keyboard and quit events in order.
         * Removing pointer records makes room even at the capacity boundary. */
        for (i = 0; i < count; ++i) {
            if (out[i].kind != PLATFORM_EVENT_MOUSE) {
                if (kept != i) out[kept] = out[i];
                ++kept;
            }
        }
        count = kept;
        win->pointer_cancel_pending = 1;
        if (count < cap) {
            PlatformEvent* event = &out[count++];
            memset(event, 0, sizeof(*event));
            event->kind = PLATFORM_EVENT_MOUSE;
            event->mx = win->pointer_x;
            event->my = win->pointer_y;
            event->code = PLATFORM_MOUSE_TAG | PLATFORM_MOUSE_CANCEL;
            win->pointer_cancel_pending = 0;
        }
        /* A batch full of keyboard/quit events contains no pointer edge that
         * can act. Retain cancellation for the next pump without losing keys. */
    }
    return count;
}

void platform_present_pixels(PlatformWindow* win, const void* pixels, int pitch,
                             int width, int height)
{
    BITMAPINFO bitmap_info;
    RECT client;
    HDC dc;
    size_t required;
    int y;
    int fullscreen = 0;
    if (win == NULL || win->hwnd == NULL || pixels == NULL ||
        pitch <= 0 || width <= 0 || height <= 0) return;
    /* Retain submitted framebuffer dimensions across client/mode resizes.
     * Creation supplies the logical extent before the first presentation. */
    if (win->frame_width != width || win->frame_height != height) {
        platform_frame_dimensions_set(win, width, height);
        win->pointer_x = platform_coordinate_clamp(win->pointer_x, width);
        win->pointer_y = platform_coordinate_clamp(win->pointer_y, height);
    }
#if !defined(_WIN64)
    if (surface_winapi_present_rgba(pixels, pitch, width, height)) return;
#endif
    if (s_platform_render_surface_ops.fullscreen_get != NULL) {
        fullscreen = s_platform_render_surface_ops.fullscreen_get(
            s_platform_render_surface_ops.context);
    }
    if (fullscreen) {
        if (s_platform_render_surface_ops.surface_flip != NULL) {
            s_platform_render_surface_ops.surface_flip(
                s_platform_render_surface_ops.context, win, 0, 1);
            return;
        }
    }
    else if (s_platform_render_surface_ops.windowed_blit != NULL) {
        s_platform_render_surface_ops.windowed_blit(
            s_platform_render_surface_ops.context, win, pixels, pitch,
            width, height);
        return;
    }

    /* The WinAPI build has no DirectDraw primary/back-buffer renderer wired
     * yet. Present its locked RGBA framebuffer through GDI so the reconstructed
     * front end remains visible while surface creation still uses DirectDraw. */
    required = (size_t)width * (size_t)height * 4u;
    if (required > s_platform_present_capacity) {
        unsigned char* resized = (unsigned char*)realloc(
            s_platform_present_bgra, required);
        if (resized == NULL) return;
        s_platform_present_bgra = resized;
        s_platform_present_capacity = required;
    }
    for (y = 0; y < height; ++y) {
        const unsigned char* source = (const unsigned char*)pixels +
            (size_t)y * (size_t)pitch;
        unsigned char* destination = s_platform_present_bgra +
            (size_t)y * (size_t)width * 4u;
        int x;
        for (x = 0; x < width; ++x) {
            destination[(size_t)x * 4u + 0u] = source[(size_t)x * 4u + 2u];
            destination[(size_t)x * 4u + 1u] = source[(size_t)x * 4u + 1u];
            destination[(size_t)x * 4u + 2u] = source[(size_t)x * 4u + 0u];
            destination[(size_t)x * 4u + 3u] = 0u;
        }
    }
    memset(&bitmap_info, 0, sizeof(bitmap_info));
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = -height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    if (!GetClientRect(win->hwnd, &client)) return;
    dc = GetDC(win->hwnd);
    if (dc == NULL) return;
    StretchDIBits(dc, 0, 0, client.right - client.left,
        client.bottom - client.top, 0, 0, width, height,
        s_platform_present_bgra, &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(win->hwnd, dc);
}

void platform_screenshot(PlatformWindow* win, const void* pixels, int pitch,
                         int width, int height, int frame)
{
    (void)win; (void)pixels; (void)pitch; (void)width; (void)height;
    (void)frame;
}

typedef struct PlatformAudioSubmission {
    HWAVEOUT output;
    WAVEHDR header;
    DWORD bytes;
    DWORD sample_rate;
    WORD channels;
    int is_loop;
    uintptr_t voice;
    struct PlatformAudioSubmission* next;
} PlatformAudioSubmission;

struct PlatformAudio {
    PlatformAudioSubmission* submissions;
    PlatformAudioSubmission* loop;
    float gain;
};

static DWORD platform_audio_volume(float gain)
{
    DWORD channel = (DWORD)(gain * 65535.0f);
    return channel | (channel << 16);
}

static void platform_audio_submission_destroy(PlatformAudioSubmission* item,
                                              int reset)
{
    if (item == NULL) return;
    if (item->output != NULL) {
        if (reset) waveOutReset(item->output);
        waveOutUnprepareHeader(item->output, &item->header,
                               sizeof(item->header));
        waveOutClose(item->output);
    }
    free(item->header.lpData);
    free(item);
}

static void platform_audio_reap(PlatformAudio* a)
{
    PlatformAudioSubmission** link;
    if (a == NULL) return;
    link = &a->submissions;
    while (*link != NULL) {
        PlatformAudioSubmission* item = *link;
        if (!item->is_loop && (item->header.dwFlags & WHDR_DONE) != 0) {
            *link = item->next;
            platform_audio_submission_destroy(item, 0);
        }
        else {
            link = &item->next;
        }
    }
}

static int platform_audio_submit(PlatformAudio* a, const int16_t* pcm,
                                 int frames, int sample_rate, int channels,
                                 int loop, uintptr_t voice)
{
    PlatformAudioSubmission* item;
    WAVEFORMATEX format;
    DWORD bytes;
    MMRESULT result;

    platform_audio_reap(a);
    if (a == NULL || pcm == NULL || frames <= 0 || sample_rate <= 0 ||
        (channels != 1 && channels != 2) ||
        sample_rate > 0x3fffffff / (channels * 2) ||
        frames > 0x3fffffff / (channels * 2)) return -1;
    bytes = (DWORD)(frames * channels * 2);
    item = (PlatformAudioSubmission*)calloc(1, sizeof(*item));
    if (item == NULL) return -1;
    item->header.lpData = (LPSTR)malloc(bytes);
    if (item->header.lpData == NULL) {
        free(item);
        return -1;
    }
    memcpy(item->header.lpData, pcm, bytes);
    item->bytes = bytes;
    item->sample_rate = (DWORD)sample_rate;
    item->channels = (WORD)channels;
    item->is_loop = loop;
    item->voice = voice;

    memset(&format, 0, sizeof(format));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = (WORD)channels;
    format.nSamplesPerSec = (DWORD)sample_rate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = (WORD)(channels * 2);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    result = waveOutOpen(&item->output, WAVE_MAPPER, &format, 0, 0,
                         CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        platform_audio_submission_destroy(item, 0);
        return -1;
    }
    waveOutSetVolume(item->output, platform_audio_volume(a->gain));
    item->header.dwBufferLength = bytes;
    if (loop) {
        item->header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        item->header.dwLoops = 0xffffffffUL;
    }
    result = waveOutPrepareHeader(item->output, &item->header,
                                  sizeof(item->header));
    if (result != MMSYSERR_NOERROR) {
        platform_audio_submission_destroy(item, 0);
        return -1;
    }
    result = waveOutWrite(item->output, &item->header, sizeof(item->header));
    if (result != MMSYSERR_NOERROR) {
        platform_audio_submission_destroy(item, 1);
        return -1;
    }
    item->next = a->submissions;
    a->submissions = item;
    if (loop) a->loop = item;
    return 0;
}

PlatformAudio* platform_audio_open(void)
{
    PlatformAudio* a;
    if (waveOutGetNumDevs() == 0) return NULL;
    a = (PlatformAudio*)calloc(1, sizeof(*a));
    if (a != NULL) a->gain = 1.0f;
    return a;
}

void platform_audio_close(PlatformAudio* a)
{
    if (a == NULL) return;
    platform_audio_stop(a);
    free(a);
}

int platform_audio_play(PlatformAudio* a, const int16_t* pcm, int frames,
                        int sample_rate)
{
    return platform_audio_submit(a, pcm, frames, sample_rate, 1, 0, 0u);
}

int platform_audio_play_voice(PlatformAudio* a, const int16_t* pcm,
                              int frames, int sample_rate, uintptr_t voice)
{
    return platform_audio_submit(a, pcm, frames, sample_rate, 1, 0, voice);
}

void platform_audio_stop_voice(PlatformAudio* a, uintptr_t voice)
{
    PlatformAudioSubmission** link;
    if (a == NULL || voice == 0u) return;
    link = &a->submissions;
    while (*link != NULL) {
        PlatformAudioSubmission* item = *link;
        if (!item->is_loop && item->voice == voice) {
            *link = item->next;
            if (a->loop == item) a->loop = NULL;
            platform_audio_submission_destroy(item, 1);
        }
        else {
            link = &item->next;
        }
    }
}

int platform_audio_play_stereo(PlatformAudio* a, const int16_t* lr,
                               int frames, int sample_rate)
{
    return platform_audio_submit(a, lr, frames, sample_rate, 2, 0, 0u);
}

int platform_audio_loop(PlatformAudio* a, const int16_t* lr,
                        int frames, int sample_rate)
{
    if (a == NULL) return -1;
    if (a->loop != NULL) {
        PlatformAudioSubmission** link = &a->submissions;
        while (*link != NULL && *link != a->loop) link = &(*link)->next;
        if (*link != NULL) {
            PlatformAudioSubmission* old = *link;
            *link = old->next;
            a->loop = NULL;
            platform_audio_submission_destroy(old, 1);
        }
    }
    if (lr == NULL || frames <= 0 || sample_rate <= 0) return 0;
    return platform_audio_submit(a, lr, frames, sample_rate, 2, 1, 0u);
}

void platform_audio_refill(PlatformAudio* a)
{
    platform_audio_reap(a);
}

int platform_audio_set_gain(PlatformAudio* a, float gain)
{
    PlatformAudioSubmission* item;
    int result = 0;
    if (a == NULL) return -1;
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    a->gain = gain;
    for (item = a->submissions; item != NULL; item = item->next) {
        if (waveOutSetVolume(item->output, platform_audio_volume(gain)) !=
            MMSYSERR_NOERROR) result = -1;
    }
    return result;
}

float platform_audio_get_gain(PlatformAudio* a)
{
    return a != NULL ? a->gain : 0.0f;
}

int platform_audio_queued(PlatformAudio* a)
{
    PlatformAudioSubmission* item;
    unsigned long total = 0;
    if (a == NULL) return 0;
    platform_audio_reap(a);
    for (item = a->submissions; item != NULL; item = item->next) {
        DWORD played = 0;
        DWORD queued;
        MMTIME position;
        if (item->is_loop) {
            queued = item->bytes;
        }
        else {
            memset(&position, 0, sizeof(position));
            position.wType = TIME_BYTES;
            if (waveOutGetPosition(item->output, &position,
                                   sizeof(position)) == MMSYSERR_NOERROR) {
                if (position.wType == TIME_BYTES) played = position.u.cb;
                else if (position.wType == TIME_MS) {
                    played = (DWORD)((double)position.u.ms *
                        (double)item->sample_rate * item->channels * 2.0 /
                        1000.0);
                }
            }
            if (played > item->bytes) played = item->bytes;
            queued = item->bytes - played;
        }
        total += queued;
        if (total > 0x7fffffffUL) return 0x7fffffff;
    }
    return (int)total;
}

int platform_audio_queued_milliseconds(PlatformAudio* a)
{
    PlatformAudioSubmission* item;
    double total = 0.0;
    if (a == NULL) return 0;
    platform_audio_reap(a);
    for (item = a->submissions; item != NULL; item = item->next) {
        DWORD played = 0;
        DWORD queued;
        MMTIME position;
        if (item->is_loop) {
            queued = item->bytes;
        }
        else {
            memset(&position, 0, sizeof(position));
            position.wType = TIME_BYTES;
            if (waveOutGetPosition(item->output, &position,
                                   sizeof(position)) == MMSYSERR_NOERROR) {
                if (position.wType == TIME_BYTES) played = position.u.cb;
                else if (position.wType == TIME_MS) {
                    played = (DWORD)((double)position.u.ms *
                        (double)item->sample_rate * item->channels * 2.0 /
                        1000.0);
                }
            }
            if (played > item->bytes) played = item->bytes;
            queued = item->bytes - played;
        }
        total += ((double)queued * 1000.0) /
                 ((double)item->sample_rate * item->channels * 2.0);
    }
    if (total > 0x7fffffff) return 0x7fffffff;
    return (int)total;
}

void platform_audio_stop(PlatformAudio* a)
{
    PlatformAudioSubmission* item;
    if (a == NULL) return;
    item = a->submissions;
    a->submissions = NULL;
    a->loop = NULL;
    while (item != NULL) {
        PlatformAudioSubmission* next = item->next;
        platform_audio_submission_destroy(item, 1);
        item = next;
    }
}

static LONG platform_mixer_ftol(double value)
{
    double integral = floor((double)value);
    double fraction = (double)value - integral;
    LONG whole = (LONG)integral;
    /* VC5's __ftol follows the x87 round-to-nearest-even control mode. */
    if (fraction > 0.5 || (fraction == 0.5 && (whole & 1) != 0)) ++whole;
    return whole;
}

/* WindowsMixerControlInit (FUN_004789c0): walk every mixer device,
 * destination, and connected source. On CD source lines, inspect the full
 * control array using the target's two dwControlType masks; query or update
 * every matching control and keep the final successful value/result. */
static int platform_cd_mixer_volume_access(float* music, float* sfx,
                                           int apply)
{
    UINT device_count;
    UINT device_index;
    int succeeded = 0;
    float requested_music = 0.0f;
    float requested_sfx = 0.0f;

    if (music == NULL || sfx == NULL) return 0;
    if (apply != 0) {
        requested_music = *music;
        requested_sfx = *sfx;
        if (requested_music > 1.0f) requested_music = 1.0f;
        if (requested_music < 0.0f) requested_music = 0.0f;
        if (requested_sfx > 1.0f) requested_sfx = 1.0f;
        if (requested_sfx < 0.0f) requested_sfx = 0.0f;
    } else {
        *music = 0.0f;
        *sfx = 0.0f;
    }

    device_count = mixerGetNumDevs();
    for (device_index = 0; device_index < device_count; ++device_index) {
        HMIXER mixer = NULL;
        MIXERCAPSA caps;
        UINT destination_index;

        if (mixerOpen(&mixer, device_index, 0, 0, CALLBACK_NULL) !=
            MMSYSERR_NOERROR) continue;
        memset(&caps, 0, sizeof(caps));
        if (mixerGetDevCapsA((UINT)device_index, &caps,
                             sizeof(caps)) == MMSYSERR_NOERROR) {
            for (destination_index = 0;
                 destination_index < caps.cDestinations;
                 ++destination_index) {
                MIXERLINEA destination_line;
                DWORD source_index;

                memset(&destination_line, 0, sizeof(destination_line));
                destination_line.cbStruct = sizeof(destination_line);
                destination_line.dwDestination = destination_index;
                if (mixerGetLineInfoA((HMIXEROBJ)mixer, &destination_line,
                                      MIXER_GETLINEINFOF_DESTINATION) !=
                    MMSYSERR_NOERROR) continue;
                for (source_index = 0;
                     source_index < destination_line.cConnections;
                     ++source_index) {
                    MIXERLINEA source_line;
                    MIXERLINECONTROLSA line_controls;
                    MIXERCONTROL* controls;
                    DWORD control_index;

                    memset(&source_line, 0, sizeof(source_line));
                    source_line.cbStruct = sizeof(source_line);
                    source_line.dwDestination = destination_index;
                    source_line.dwSource = source_index;
                    if (mixerGetLineInfoA((HMIXEROBJ)mixer, &source_line,
                                          MIXER_GETLINEINFOF_SOURCE) !=
                        MMSYSERR_NOERROR ||
                        source_line.dwComponentType !=
                            MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC ||
                        source_line.cControls == 0u ||
                        source_line.cControls > 0x7fffffffu /
                            sizeof(MIXERCONTROL)) continue;

                    controls = (MIXERCONTROL*)calloc(
                        source_line.cControls, sizeof(*controls));
                    if (controls == NULL) continue;
                    for (control_index = 0u;
                         control_index < source_line.cControls;
                         ++control_index)
                        controls[control_index].cbStruct = sizeof(MIXERCONTROL);
                    memset(&line_controls, 0, sizeof(line_controls));
                    line_controls.cbStruct = sizeof(line_controls);
                    line_controls.dwLineID = source_line.dwLineID;
                    line_controls.cControls = source_line.cControls;
                    line_controls.cbmxctrl = sizeof(MIXERCONTROL);
                    line_controls.pamxctrl = controls;
                    if (mixerGetLineControlsA((HMIXEROBJ)mixer,
                                              &line_controls,
                                              MIXER_GETLINECONTROLSF_ALL) ==
                        MMSYSERR_NOERROR) {
                        for (control_index = 0u;
                             control_index < source_line.cControls;
                             ++control_index) {
                            MIXERCONTROL* control = &controls[control_index];
                            MIXERCONTROLDETAILS details;
                            MIXERCONTROLDETAILS_UNSIGNED* values;
                            DWORD channel_count = source_line.cChannels;
                            LONG minimum = control->Bounds.lMinimum;
                            LONG maximum = control->Bounds.lMaximum;
                            LONG range = maximum - minimum;
                            DWORD channel;

                            if ((control->dwControlType & 0x50000000u) == 0u ||
                                (control->dwControlType & 0x50030001u) == 0u ||
                                channel_count == 0u || range <= 0 ||
                                channel_count > 0x7fffffffu /
                                    sizeof(*values)) continue;
                            values = (MIXERCONTROLDETAILS_UNSIGNED*)calloc(
                                channel_count, sizeof(*values));
                            if (values == NULL) continue;
                            memset(&details, 0, sizeof(details));
                            details.cbStruct = sizeof(details);
                            details.dwControlID = control->dwControlID;
                            details.cChannels = channel_count;
                            details.cbDetails = sizeof(*values);
                            details.paDetails = values;
                            if (apply == 0) {
                                if (mixerGetControlDetailsA(
                                        (HMIXEROBJ)mixer, &details,
                                        MIXER_GETCONTROLDETAILSF_VALUE) ==
                                    MMSYSERR_NOERROR) {
                                    float first =
                                        (float)((LONG)values[0].dwValue -
                                                minimum) / (float)range;
                                    float second = first;
                                    if (channel_count == 2u) {
                                        second = (float)((LONG)values[1].dwValue -
                                                         minimum) /
                                                 (float)range;
                                    }
                                    *music = first;
                                    *sfx = second;
                                    succeeded = 1;
                                }
                            } else {
                                LONG first = platform_mixer_ftol(
                                    (double)requested_music * (double)range) +
                                    minimum;
                                LONG second = platform_mixer_ftol(
                                    (double)requested_sfx * (double)range) +
                                    minimum;
                                if (channel_count == 2u) {
                                    values[0].dwValue = (DWORD)first;
                                    values[1].dwValue = (DWORD)second;
                                } else {
                                    DWORD average = (DWORD)
                                        ((first + second) >> 1);
                                    for (channel = 0u; channel < channel_count;
                                         ++channel)
                                        values[channel].dwValue = average;
                                }
                                if (mixerSetControlDetails(
                                        (HMIXEROBJ)mixer, &details,
                                        MIXER_SETCONTROLDETAILSF_VALUE) ==
                                    MMSYSERR_NOERROR) succeeded = 1;
                            }
                            free(values);
                        }
                    }
                    free(controls);
                }
            }
        }
        mixerClose((HMIXER)mixer);
    }
    return succeeded;
}

int platform_cd_mixer_volume_get(float* music, float* sfx)
{
    return platform_cd_mixer_volume_access(music, sfx, 0);
}

int platform_cd_mixer_volume_set(float music, float sfx)
{
    return platform_cd_mixer_volume_access(&music, &sfx, 1);
}

static int s_platform_cd_audio_open;
static char s_platform_cd_audio_result[200];

static int platform_cd_audio_command(const char* command, int* error_code)
{
    MCIERROR result = mciSendStringA(command, s_platform_cd_audio_result,
                                     sizeof(s_platform_cd_audio_result), NULL);
    if (error_code != NULL) *error_code = (int)result;
    return result == 0 ? 1 : 0;
}

int platform_cd_audio_open(int* error_code)
{
    int opened = platform_cd_audio_command("open cdaudio", error_code);
    if (opened) s_platform_cd_audio_open = 1;
    return opened;
}

int platform_cd_audio_set_tmsf(int* error_code)
{
    return platform_cd_audio_command("set cdaudio time format tmsf",
                                     error_code);
}

int platform_cd_audio_play(int one_based_track, int* error_code)
{
    char command[64];
    sprintf(command, "play cdaudio from %d", one_based_track);
    return platform_cd_audio_command(command, error_code);
}

int platform_cd_audio_stop(int* error_code)
{
    return platform_cd_audio_command("stop cdaudio", error_code);
}

int platform_cd_audio_close(int* error_code)
{
    int closed = platform_cd_audio_command("close cdaudio", error_code);
    if (closed) s_platform_cd_audio_open = 0;
    return closed;
}

int platform_cd_audio_status(char* mode, int mode_capacity,
                             int* current_track, int* error_code)
{
    MCIERROR result;
    if (mode == NULL || mode_capacity <= 0 || current_track == NULL) {
        if (error_code != NULL) *error_code = MCIERR_PARAM_OVERFLOW;
        return 0;
    }
    result = mciSendStringA("status cdaudio mode", s_platform_cd_audio_result,
                            sizeof(s_platform_cd_audio_result), NULL);
    if (mode_capacity > 1) {
        strncpy(mode, s_platform_cd_audio_result,
                (size_t)mode_capacity - 1u);
        mode[mode_capacity - 1] = '\0';
    }
    else mode[0] = '\0';
    *current_track = 0;
    if (lstrcmpiA(mode, "stopped") != 0) {
        result = mciSendStringA("status cdaudio current track",
                                s_platform_cd_audio_result,
                                sizeof(s_platform_cd_audio_result), NULL);
        *current_track = atoi(s_platform_cd_audio_result);
    }
    if (error_code != NULL) *error_code = (int)result;
    /* Retail consumes the status text even when mciSendStringA reports an
     * error; the shared result buffer may therefore preserve earlier text. */
    return 1;
}

int platform_cd_audio_error_text(int error_code, char* output,
                                 int output_capacity)
{
    int result;
    if (output == NULL || output_capacity <= 0) return 0;
    result = mciGetErrorStringA((MCIERROR)error_code, output,
                                (UINT)output_capacity) ? 1 : 0;
    if (result) {
        strncpy(s_platform_cd_audio_result, output,
                sizeof(s_platform_cd_audio_result) - 1u);
        s_platform_cd_audio_result[
            sizeof(s_platform_cd_audio_result) - 1u] = '\0';
    }
    return result;
}

uint32_t platform_time_milliseconds(void)
{
    return (uint32_t)timeGetTime();
}

/* Main.c's module instance (DAT_00505804, after the window at 00505800).
 * Init.c (gods98/init.c, LATE-002) passes it to DialogBoxParamA. The port
 * never sets it: NULL selects the executable, the same module. Defined
 * after every function, so that it does not move this file's symbol
 * numbering (finding 538). */
extern "C" void* g_main_instance_handle;
// GLOBAL: LEGORR 0x00505804
void* g_main_instance_handle;
