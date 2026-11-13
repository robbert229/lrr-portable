// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit LATE-002 (text 0049d030-0049dab0; strings
 * 004b2d0c-004b2db0; the common initGlobs at 005580a0; finding 613):
 * gods98's Init.c, the start-up display selection. Init_Initialise
 * enumerates the DirectDraw drivers. With "best" it walks them from the
 * last, and takes the first valid mode of the first driver whose device
 * list has a hardware device; otherwise it runs the "Mode Selection"
 * dialog (resource 101: driver, device and mode lists, and Full Screen /
 * Window buttons). It then sets up the display through DirectDraw.c. The
 * binary attests none of the names; the functions use the gods98 Init_*
 * names in snake case.
 *
 * The original links this object after the import libraries, like
 * LATE-001 (finding 608). The code is Windows only: the native port has no
 * DirectDraw, so the file defines only initGlobs there (init_port.cpp keeps
 * the port's valid-mode filters in it).
 *
 * C-compatible (ADR 0005): C in the matching build, C++ in the MinGW
 * reference build. */
#include "init.h"

// GLOBAL: LEGORR 0x005580a0
Init_Globs initGlobs;

#if !defined(LEGORRR_BACKEND_MINIWIN)

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "crt/vc5names.h"
#include "init_imports.h"

#define INIT_DIALOG_MODESELECTION   101
#define INIT_IDC_DRIVERLIST         1000
#define INIT_IDC_DEVICELIST         1002
#define INIT_IDC_MODELIST           1003
#define INIT_IDC_FULLSCREEN         1004
#define INIT_IDC_WINDOW             1005
#define INIT_IDC_MODELISTTEXT       1006

static BOOL CALLBACK init_dialog_handler(HWND hWndDlg, UINT message,
                                         WPARAM wParam, LPARAM lParam);
void init_set_full_screen(HWND hWndDlg, BOOL on);
void init_set_mode_list(HWND hWndDlg);
void init_set_device_list(HWND hWndDlg);
void init_handle_window_button(HWND hWndDlg);

// FUNCTION: LEGORR 0x0049d030
int init_initialise(int setup, int debug, int best, int window,
                    const char* noHALMsg)
{
    int res = 1;

    initGlobs.selDriver = NULL;
    initGlobs.selDevice = NULL;
    initGlobs.selMode = NULL;
    initGlobs.wasFullScreen = TRUE;
    initGlobs.selFullScreen = TRUE;
    initGlobs.validModeCount = 0;

    init_add_valid_mode(640, 480, 16);
    if (debug) {
        init_add_valid_mode(800, 600, 16);
        init_add_valid_mode(1024, 768, 16);
    }

    directdraw_enum_drivers(initGlobs.drivers, &initGlobs.driverCount);
    if (initGlobs.driverCount) {
        initGlobs.selDriver = &initGlobs.drivers[0];

        if (best) {
            int found = FALSE;
            int driverLoop, deviceLoop;
            unsigned int modeLoop;

            res = 1;
            initGlobs.selFullScreen = !window;
            for (driverLoop = initGlobs.driverCount - 1; driverLoop >= 0; driverLoop--) {
                if (directdraw_enum_devices(&initGlobs.drivers[driverLoop], initGlobs.devices, &initGlobs.deviceCount)) {
                    directdraw_enum_modes(&initGlobs.drivers[driverLoop], initGlobs.selFullScreen, initGlobs.modes, &initGlobs.modeCount);
                    for (modeLoop = 0; modeLoop < initGlobs.modeCount; modeLoop++) {
                        if (init_is_valid_mode(modeLoop)) {
                            for (deviceLoop = initGlobs.deviceCount - 1; deviceLoop >= 0; deviceLoop--) {
                                if (initGlobs.devices[deviceLoop].flags & DIRECTDRAW_FLAG_DEVICE_HARDWARE) {
                                    initGlobs.selDriver = &initGlobs.drivers[driverLoop];
                                    initGlobs.selDevice = &initGlobs.devices[deviceLoop];
                                    initGlobs.selMode = &initGlobs.modes[modeLoop];
                                    found = TRUE;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                } else {
                    // STRING: LEGORR 0x004b2d0c
                    MessageBoxA(NULL, "Please install DirectX version 6 or later", "Error", MB_OK);
                    return 0;
                }
                if (found == TRUE) break;
            }

            if (!found) {
                MessageBoxA(NULL, noHALMsg, "Error", MB_OK);
                return 0;
            }
        } else if (setup) {
            res = DialogBoxParamA((HINSTANCE)g_main_instance_handle, MAKEINTRESOURCE(INIT_DIALOG_MODESELECTION),
                                  (HWND)g_platform_cursor_window_handle, init_dialog_handler, 0);
        }

        if (res == 1) {
            if (initGlobs.selFullScreen) {
                return directdraw_setup(TRUE, initGlobs.selDriver, initGlobs.selDevice, initGlobs.selMode, 0, 0, 320, 200);
            } else if (initGlobs.selMode) {
                return directdraw_setup(FALSE, NULL, initGlobs.selDevice, NULL, 100, 100, initGlobs.selMode->width, initGlobs.selMode->height);
            } else {
                return directdraw_setup(FALSE, NULL, initGlobs.selDevice, NULL, 40, 40, 640, 480);
            }
        }
    }

    return 0;
}

// FUNCTION: LEGORR 0x0049d2f0
static BOOL CALLBACK init_dialog_handler(HWND hWndDlg, UINT message,
                                         WPARAM wParam, LPARAM lParam)
{
    unsigned int mode;
    RECT dlgRect, desktopRect;
    char buffer[1024];

    if (message == WM_INITDIALOG) {
        HWND hWndDesktop, hWndButton, hWndList;
        unsigned int loop;

        hWndDesktop = GetDesktopWindow();
        GetWindowRect(hWndDesktop, &desktopRect);
        GetWindowRect(hWndDlg, &dlgRect);
        MoveWindow(hWndDlg, (desktopRect.right / 2) - ((dlgRect.right - dlgRect.left) / 2),
                   (desktopRect.bottom / 2) - ((dlgRect.bottom - dlgRect.top) / 2),
                   dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top, TRUE);

        if (initGlobs.selFullScreen) hWndButton = GetDlgItem(hWndDlg, INIT_IDC_FULLSCREEN);
        else hWndButton = GetDlgItem(hWndDlg, INIT_IDC_WINDOW);
        SendMessageA(hWndButton, BM_SETCHECK, 1, 0);

        hWndList = GetDlgItem(hWndDlg, INIT_IDC_DRIVERLIST);
        for (loop = 0; loop < initGlobs.driverCount; loop++) {
            SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)initGlobs.drivers[loop].desc);
        }
        SendMessageA(hWndList, LB_SETCURSEL, 0, 0);
        SetFocus(hWndList);

        init_handle_window_button(hWndDlg);
        init_set_device_list(hWndDlg);
        init_set_mode_list(hWndDlg);

        return FALSE;
    }

    if (message == WM_COMMAND) {
        unsigned int selIndex;

        if (HIWORD(wParam) == LBN_SELCHANGE) {
            if (LOWORD(wParam) == INIT_IDC_DEVICELIST) {
                selIndex = SendMessageA((HWND)lParam, LB_GETCURSEL, 0, 0);
                initGlobs.selDevice = &initGlobs.devices[selIndex];
                return TRUE;
            } else if (LOWORD(wParam) == INIT_IDC_MODELIST) {
                selIndex = SendMessageA((HWND)lParam, LB_GETCURSEL, 0, 0);
                SendMessageA((HWND)lParam, LB_GETTEXT, selIndex, (LPARAM)buffer);
                if (init_get_mode(buffer, &mode)) {
                    initGlobs.selMode = &initGlobs.modes[mode];
                    return TRUE;
                }
            } else if (LOWORD(wParam) == INIT_IDC_DRIVERLIST) {
                selIndex = SendMessageA((HWND)lParam, LB_GETCURSEL, 0, 0);
                initGlobs.selDriver = &initGlobs.drivers[selIndex];
                init_handle_window_button(hWndDlg);
                init_set_mode_list(hWndDlg);
                init_set_device_list(hWndDlg);
                return TRUE;
            }
        } else if (HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD(wParam) == INIT_IDC_WINDOW) init_set_full_screen(hWndDlg, FALSE);
            if (LOWORD(wParam) == INIT_IDC_FULLSCREEN) {
                init_set_full_screen(hWndDlg, TRUE);
                return TRUE;
            }
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) EndDialog(hWndDlg, wParam);
        }
        return TRUE;
    }

    return FALSE;
}

// FUNCTION: LEGORR 0x0049d5e0
void init_set_full_screen(HWND hWndDlg, BOOL on)
{
    HWND hWndText;

    initGlobs.selFullScreen = on;
    init_set_mode_list(hWndDlg);

    hWndText = GetDlgItem(hWndDlg, INIT_IDC_MODELISTTEXT);
    // STRING: LEGORR 0x004b2d44
    if (on) SendMessageA(hWndText, WM_SETTEXT, 0, (LPARAM)"Screen Mode");
    // STRING: LEGORR 0x004b2d38
    else SendMessageA(hWndText, WM_SETTEXT, 0, (LPARAM)"Window Size");
}

// FUNCTION: LEGORR 0x0049d630
void init_set_mode_list(HWND hWndDlg)
{
    HWND hWndList, hWndOK;
    unsigned int loop;
    BOOL first = TRUE;
    char buffer[256];

    hWndList = GetDlgItem(hWndDlg, INIT_IDC_MODELIST);
    while (SendMessageA(hWndList, LB_DELETESTRING, 0, 0) != LB_ERR);

    directdraw_enum_modes(initGlobs.selDriver, initGlobs.selFullScreen, initGlobs.modes, &initGlobs.modeCount);
    for (loop = 0; loop < initGlobs.modeCount; loop++) {
        if (init_is_valid_mode(loop)) {
            SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)initGlobs.modes[loop].desc);
            if (first) {
                initGlobs.selMode = &initGlobs.modes[loop];
                first = FALSE;
            }
        }
    }

    hWndOK = GetDlgItem(hWndDlg, IDOK);
    if (first) {
        // STRING: LEGORR 0x004b2d70
        if (!initGlobs.selFullScreen) sprintf(buffer, "No supported %i bit modes found", window_runtime_display_caps_query());
        // STRING: LEGORR 0x004b2d50
        else sprintf(buffer, "No supported screen modes found");
        SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)buffer);
        EnableWindow(hWndOK, FALSE);
    } else EnableWindow(hWndOK, TRUE);

    SendMessageA(hWndList, LB_SETCURSEL, 0, 0);
}

// FUNCTION: LEGORR 0x0049d780
void init_set_device_list(HWND hWndDlg)
{
    HWND hWndList;
    unsigned int loop, deviceNum = 0;
    LRESULT result;

    hWndList = GetDlgItem(hWndDlg, INIT_IDC_DEVICELIST);
    /* The result variable, not the bare loop of Init_SetModeList: without
     * it MSVC 5.0 swaps the list handle and the loop counter between esi
     * and ebx/edi. */
    result = SendMessageA(hWndList, LB_DELETESTRING, 0, 0);
    while (result != LB_ERR) result = SendMessageA(hWndList, LB_DELETESTRING, 0, 0);

    if (directdraw_enum_devices(initGlobs.selDriver, initGlobs.devices, &initGlobs.deviceCount)) {
        for (loop = 0; loop < initGlobs.deviceCount; loop++) {
            if (initGlobs.devices[loop].flags & DIRECTDRAW_FLAG_DEVICE_HARDWARE) deviceNum = loop;
            SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)initGlobs.devices[loop].desc);
        }
        initGlobs.selDevice = &initGlobs.devices[deviceNum];
    } else {
        // STRING: LEGORR 0x004b2d90
        SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)"Error: DirectX6 not installed.");
    }
    SendMessageA(hWndList, LB_SETCURSEL, deviceNum, 0);
}

// FUNCTION: LEGORR 0x0049d870
void init_add_valid_mode(unsigned int width, unsigned int height,
                         unsigned int depth)
{
    initGlobs.validModes[initGlobs.validModeCount].width = width;
    initGlobs.validModes[initGlobs.validModeCount].height = height;
    initGlobs.validModes[initGlobs.validModeCount].bitDepth = depth;
    initGlobs.validModeCount++;
}

// FUNCTION: LEGORR 0x0049d8d0
int init_is_valid_mode(unsigned int mode)
{
    unsigned int loop;

    if (initGlobs.validModeCount) {
        for (loop = 0; loop < initGlobs.validModeCount; loop++) {
            if (initGlobs.validModes[loop].width == 0 || initGlobs.validModes[loop].width == initGlobs.modes[mode].width) {
                if (initGlobs.validModes[loop].height == 0 || initGlobs.validModes[loop].height == initGlobs.modes[mode].height) {
                    if (initGlobs.validModes[loop].bitDepth == 0 || initGlobs.validModes[loop].bitDepth == initGlobs.modes[mode].bitDepth) {
                        return TRUE;
                    }
                }
            }
        }
        return FALSE;
    }

    return TRUE;
}

// FUNCTION: LEGORR 0x0049d950
int init_get_mode(const char* name, unsigned int* mode)
{
    unsigned int loop;

    for (loop = 0; loop < initGlobs.modeCount; loop++) {
        if (strcmp(initGlobs.modes[loop].desc, name) == 0) {
            *mode = loop;
            return TRUE;
        }
    }

    return FALSE;
}

// FUNCTION: LEGORR 0x0049d9d0
void init_handle_window_button(HWND hWndDlg)
{
    HWND hWndButton;

    if (initGlobs.selDriver->flags & DIRECTDRAW_FLAG_DRIVER_WINDOWOK) {
        hWndButton = GetDlgItem(hWndDlg, INIT_IDC_WINDOW);
        EnableWindow(hWndButton, TRUE);
        if (!initGlobs.wasFullScreen) {
            init_set_full_screen(hWndDlg, FALSE);
            hWndButton = GetDlgItem(hWndDlg, INIT_IDC_FULLSCREEN);
            SendMessageA(hWndButton, BM_SETCHECK, 0, 0);
            hWndButton = GetDlgItem(hWndDlg, INIT_IDC_WINDOW);
            SendMessageA(hWndButton, BM_SETCHECK, 1, 0);
        }
    } else {
        initGlobs.wasFullScreen = initGlobs.selFullScreen;
        init_set_full_screen(hWndDlg, TRUE);
        hWndButton = GetDlgItem(hWndDlg, INIT_IDC_WINDOW);
        SendMessageA(hWndButton, BM_SETCHECK, 0, 0);
        EnableWindow(hWndButton, FALSE);
        hWndButton = GetDlgItem(hWndDlg, INIT_IDC_FULLSCREEN);
        SendMessageA(hWndButton, BM_SETCHECK, 1, 0);
    }
}

#endif /* !LEGORRR_BACKEND_MINIWIN */
