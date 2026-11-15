// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-013 (text 0047ed90-0047f130; the __FILE__
 * string 004aaed0; its state in the commons, 0076aa00-0076ac80; finding
 * 683): gods98's Input.c. It opens DirectInput and its keyboard, polls the
 * keyboard into the two key maps, counts the keys held, closes DirectInput,
 * samples the cursor into the client-relative mouse words and moves it.
 * The binary attests none of the names; the functions keep the names the
 * port gave them.
 *
 * Windows only. The Windows builds (the matching build and the MinGW
 * reference build) compile the original's code, which calls DirectInput
 * through its COM method tables, imports DirectInputCreateA and links
 * c_dfDIKeyboard and GUID_SysKeyboard from dinput.lib and dxguid.lib, as the
 * original does (finding 574). The native port has no DirectInput: there
 * the file is empty, and input.cpp gives the six functions their native
 * bodies (PORT-ONLY).
 *
 * The state is the original's, in the commons: the previous and current
 * key maps, the DirectInput object, the keyboard device, its acquired flag,
 * and the cursor position and movement. The other units read the key maps
 * and the mouse words by their port names, so the state is a set of
 * separate commons rather than the original's one Input_Globs (finding 683,
 * section 2).
 *
 * C-compatible (ADR 0005): C in the matching build, C++ in the MinGW
 * reference build. */
#include "input_unit.h"

#if !defined(LEGORRR_BACKEND_MINIWIN)

#include <string.h>
#include <windows.h>

#include "dxerror.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MSVC 5.0's SDK has no dinput.h, so the unit declares the DirectInput 5
 * layouts it uses. Unused method slots are placeholders. */
typedef struct InputGuid {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} InputGuid;

typedef struct DirectInputIface DirectInputIface;              /* IDirectInputA */
typedef struct DirectInputDeviceIface DirectInputDeviceIface;  /* IDirectInputDeviceA */
typedef struct DirectInputDataFormat DirectInputDataFormat;    /* DIDATAFORMAT */

typedef struct DirectInputIfaceVtbl {
    HRESULT (WINAPI* QueryInterface)(DirectInputIface*, const InputGuid*, void**);
    ULONG (WINAPI* AddRef)(DirectInputIface*);
    ULONG (WINAPI* Release)(DirectInputIface*);
    HRESULT (WINAPI* CreateDevice)(DirectInputIface*, const InputGuid*,
                                   DirectInputDeviceIface**, void*);
} DirectInputIfaceVtbl;

struct DirectInputIface {
    const DirectInputIfaceVtbl* lpVtbl;
};

typedef struct DirectInputDeviceIfaceVtbl {
    HRESULT (WINAPI* QueryInterface)(DirectInputDeviceIface*, const InputGuid*,
                                     void**);
    ULONG (WINAPI* AddRef)(DirectInputDeviceIface*);
    ULONG (WINAPI* Release)(DirectInputDeviceIface*);
    void* GetCapabilities;
    void* EnumObjects;
    void* GetProperty;
    void* SetProperty;
    HRESULT (WINAPI* Acquire)(DirectInputDeviceIface*);
    HRESULT (WINAPI* Unacquire)(DirectInputDeviceIface*);
    HRESULT (WINAPI* GetDeviceState)(DirectInputDeviceIface*, DWORD, void*);
    void* GetDeviceData;
    HRESULT (WINAPI* SetDataFormat)(DirectInputDeviceIface*,
                                    const DirectInputDataFormat*);
    void* SetEventNotification;
    HRESULT (WINAPI* SetCooperativeLevel)(DirectInputDeviceIface*, HWND, DWORD);
} DirectInputDeviceIfaceVtbl;

struct DirectInputDeviceIface {
    const DirectInputDeviceIfaceVtbl* lpVtbl;
};

#define INPUT_DIRECTINPUT_VERSION 0x0500
#define INPUT_DISCL_NONEXCLUSIVE_FOREGROUND 6 /* DISCL_NONEXCLUSIVE | DISCL_FOREGROUND */
#define INPUT_DIERR_INPUTLOST 0x8007001EL

HRESULT WINAPI DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion,
                                  DirectInputIface** lplpDirectInput,
                                  void* punkOuter);
/* Library data: the markers name the original's addresses for Reccmp. */
// GLOBAL: LEGORR 0x0049c790
extern const DirectInputDataFormat c_dfDIKeyboard;   /* dinput.lib */
// GLOBAL: LEGORR 0x004a0148
extern const InputGuid GUID_SysKeyboard;             /* dxguid.lib */

/* Main.c's state: mainGlobs' hWnd, hInst, active, appWidth, appHeight and
 * flags. In the matching build the port names are its fields
 * (main_globs.h, finding 679); the MinGW reference build keeps the port's
 * variables. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#include "main_globs.h"
#else
extern void* g_platform_cursor_window_handle;
extern void* g_main_instance_handle;
extern unsigned int s_window_application_active;
extern int s_display_w;
extern int s_display_h;
extern unsigned int s_engine_feature_word;
#endif

/* The unit's state, in the commons (tentative definitions). */
// GLOBAL: LEGORR 0x0076aa00
unsigned char s_global_previous_keys[256];
// GLOBAL: LEGORR 0x0076ab00
DirectInputIface* s_input_lpdi;
// GLOBAL: LEGORR 0x0076ab04
DirectInputDeviceIface* s_input_lpdi_keyboard;
// GLOBAL: LEGORR 0x0076ab0c
int s_input_keyboard_acquired;
// GLOBAL: LEGORR 0x0076ab24
unsigned int s_renderer_word_a;
// GLOBAL: LEGORR 0x0076ab28
unsigned int s_renderer_word_b;
// GLOBAL: LEGORR 0x0076ab34
int s_cursor_delta_x;
// GLOBAL: LEGORR 0x0076ab38
int s_cursor_delta_y;
// GLOBAL: LEGORR 0x0076ab3c
unsigned int g_renderer_word_c;
// GLOBAL: LEGORR 0x0076ab40
unsigned int s_window_mouse_word_40;
// GLOBAL: LEGORR 0x0076ab4c
unsigned int s_window_mouse_word_4c;
// GLOBAL: LEGORR 0x0076ab50
unsigned int s_window_mouse_word_50;
// GLOBAL: LEGORR 0x0076ab54
unsigned int s_window_mouse_word_54;
// GLOBAL: LEGORR 0x0076ab58
unsigned int s_window_mouse_word_58;
// GLOBAL: LEGORR 0x0076ab80
unsigned char s_global_current_keys[256];

// FUNCTION: LEGORR 0x0047ed90
int input_direct_initialize(void)
{
    InputGuid guid = GUID_SysKeyboard;
    HRESULT r;

    if (DirectInputCreateA((HINSTANCE)g_main_instance_handle,
                           INPUT_DIRECTINPUT_VERSION, &s_input_lpdi, NULL) != 0) {
        directx_result_diagnostic_dispatch(
            DirectInputCreateA((HINSTANCE)g_main_instance_handle,
                               INPUT_DIRECTINPUT_VERSION, &s_input_lpdi, NULL),
            DXERROR_MODULE_DINPUT,
            // STRING: LEGORR 0x004aaed0
            "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Input.c", 118);
        return 0;
    }
    if (s_input_lpdi->lpVtbl->CreateDevice(s_input_lpdi, &guid,
                                           &s_input_lpdi_keyboard, NULL) != 0) {
        directx_result_diagnostic_dispatch(
            s_input_lpdi->lpVtbl->CreateDevice(s_input_lpdi, &guid,
                                               &s_input_lpdi_keyboard, NULL),
            DXERROR_MODULE_DINPUT,
            "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Input.c", 121);
        return 0;
    }
    if (s_input_lpdi_keyboard->lpVtbl->SetDataFormat(s_input_lpdi_keyboard,
                                                     &c_dfDIKeyboard) != 0) {
        directx_result_diagnostic_dispatch(
            s_input_lpdi_keyboard->lpVtbl->SetDataFormat(s_input_lpdi_keyboard,
                                                         &c_dfDIKeyboard),
            DXERROR_MODULE_DINPUT,
            "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Input.c", 124);
        return 0;
    }
    if (s_input_lpdi_keyboard->lpVtbl->SetCooperativeLevel(
            s_input_lpdi_keyboard, (HWND)g_platform_cursor_window_handle,
            INPUT_DISCL_NONEXCLUSIVE_FOREGROUND) != 0) {
        directx_result_diagnostic_dispatch(
            s_input_lpdi_keyboard->lpVtbl->SetCooperativeLevel(
                s_input_lpdi_keyboard, (HWND)g_platform_cursor_window_handle,
                INPUT_DISCL_NONEXCLUSIVE_FOREGROUND),
            DXERROR_MODULE_DINPUT,
            "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Input.c", 127);
        return 0;
    }
    r = s_input_lpdi_keyboard->lpVtbl->Acquire(s_input_lpdi_keyboard);
    s_input_keyboard_acquired = r == 0;
    return 1;
}

// FUNCTION: LEGORR 0x0047eef0
void input_state_poll_retail(void)
{
    int active;
    HRESULT r;

    if (s_engine_feature_word & 2) active = 1;
    else active = s_window_application_active;

    if (!active) {
        memset(s_global_current_keys, 0, sizeof(s_global_current_keys));
        memset(s_global_previous_keys, 0, sizeof(s_global_previous_keys));
        s_input_lpdi_keyboard->lpVtbl->Unacquire(s_input_lpdi_keyboard);
        return;
    }

    if (s_input_lpdi_keyboard) {
        if (s_input_keyboard_acquired) {
            memcpy(s_global_previous_keys, s_global_current_keys,
                   sizeof(s_global_previous_keys));
            memset(s_global_current_keys, 0, sizeof(s_global_current_keys));
            r = s_input_lpdi_keyboard->lpVtbl->GetDeviceState(
                s_input_lpdi_keyboard, sizeof(s_global_current_keys),
                s_global_current_keys);
        } else {
            r = INPUT_DIERR_INPUTLOST;
        }
        if (r != 0) {
            s_input_keyboard_acquired = 0;
            if (s_input_lpdi_keyboard->lpVtbl->Acquire(s_input_lpdi_keyboard) == 0)
                s_input_keyboard_acquired = 1;
        }
    }
}

// FUNCTION: LEGORR 0x0047efb0
int input_keyboard_down_key_count(void)
{
    int count = 0;
    unsigned char* key = s_global_current_keys;
    int i;

    for (i = 256; i != 0; i--) {
        if (*key) count++;
        key++;
    }
    return count;
}

// FUNCTION: LEGORR 0x0047efd0
void input_direct_shutdown(void)
{
    if (s_input_keyboard_acquired) {
        s_input_lpdi_keyboard->lpVtbl->Unacquire(s_input_lpdi_keyboard);
        s_input_keyboard_acquired = 0;
    }
    if (s_input_lpdi_keyboard)
        s_input_lpdi_keyboard->lpVtbl->Release(s_input_lpdi_keyboard);
    if (s_input_lpdi) s_input_lpdi->lpVtbl->Release(s_input_lpdi);
}

// FUNCTION: LEGORR 0x0047f010
void input_cursor_position_update(void)
{
    RECT rect;
    POINT offset;
    POINT pos;
    int x;
    int y;

    if (GetClientRect((HWND)g_platform_cursor_window_handle, &rect)) {
        offset.y = 0;
        offset.x = 0;
        ClientToScreen((HWND)g_platform_cursor_window_handle, &offset);
        GetCursorPos(&pos);
        x = pos.x - offset.x;
        y = pos.y - offset.y;
        if (x >= s_display_w) x = s_display_w - 1;
        if (y >= s_display_h) y = s_display_h - 1;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        s_cursor_delta_x = x - s_renderer_word_a;
        s_cursor_delta_y = y - s_renderer_word_b;
        s_renderer_word_a = x;
        s_renderer_word_b = y;
    }
}

// FUNCTION: LEGORR 0x0047f0d0
void input_cursor_client_position_set(int x, int y)
{
    POINT offset;

    offset.y = 0;
    offset.x = 0;
    ClientToScreen((HWND)g_platform_cursor_window_handle, &offset);
    s_renderer_word_a = x;
    s_renderer_word_b = y;
    s_cursor_delta_x = 0;
    s_cursor_delta_y = 0;
    SetCursorPos(offset.x + x, offset.y + y);
}

#ifdef __cplusplus
}
#endif

#endif /* !LEGORRR_BACKEND_MINIWIN */
