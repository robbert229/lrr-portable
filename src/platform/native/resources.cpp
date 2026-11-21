// SPDX-License-Identifier: LGPL-3.0-or-later
/* The program's resources (ADR 0007): the port's counterpart of the
 * shipped LegoRR.exe's resource section, which miniwin reads through
 * MiniwinRegisterDialogTemplate. The process entry registers them before
 * it calls WinMain, as the Windows loader maps the section before the
 * program starts.
 *
 * DIALOG 101 is gods98 Init.c's "Mode Selection" dialog (finding 613): the
 * driver, device and mode lists, OK and Cancel, and the Full Screen and
 * Window buttons. The layout, styles and control ids are the shipped
 * resource's; init.c's dialog procedure fills the controls. */
#include <miniwin/windows.h>

void RegisterResources();

namespace {

constexpr DWORD kChild = 0x40000000L | 0x10000000L; /* WS_CHILD | WS_VISIBLE */
constexpr DWORD kListBox = kChild | 0x00800000L /* WS_BORDER */ | 0x00200000L /* WS_VSCROLL */ |
                           WS_TABSTOP | 0x0100L /* LBS_NOINTEGRALHEIGHT */ | LBS_NOTIFY;

const MINIWIN_DLGITEM kModeSelectionItems[] = {
    {MINIWIN_DLGCLASS_BUTTON, kChild | WS_TABSTOP | BS_DEFPUSHBUTTON, 65, 145, 50, 14, IDOK, "OK"},
    {MINIWIN_DLGCLASS_BUTTON, kChild | WS_TABSTOP | BS_PUSHBUTTON, 125, 145, 50, 14, IDCANCEL, "Cancel"},
    {MINIWIN_DLGCLASS_LISTBOX, kListBox, 5, 15, 230, 30, 1000, ""},
    {MINIWIN_DLGCLASS_LISTBOX, kListBox, 5, 60, 230, 35, 1002, ""},
    {MINIWIN_DLGCLASS_LISTBOX, kListBox, 100, 100, 135, 40, 1003, ""},
    {MINIWIN_DLGCLASS_STATIC, kChild | WS_GROUP | SS_LEFT, 5, 50, 230, 8, 0xffff, "Device"},
    {MINIWIN_DLGCLASS_STATIC, kChild | WS_GROUP | SS_LEFT, 5, 5, 230, 8, 0xffff, "Driver"},
    {MINIWIN_DLGCLASS_STATIC, kChild | WS_GROUP | SS_RIGHT, 5, 100, 89, 8, 1006, "Screen Modes"},
    {MINIWIN_DLGCLASS_BUTTON, kChild | BS_AUTORADIOBUTTON, 5, 115, 51, 10, 1004, "Full Screen"},
    {MINIWIN_DLGCLASS_BUTTON, kChild | BS_AUTORADIOBUTTON, 5, 125, 42, 10, 1005, "Window"},
};

/* WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_SETFONT */
const MINIWIN_DLGTEMPLATE kModeSelection = {
    0x80c800c0L, 242, 166, "Mode Selection",
    kModeSelectionItems, sizeof(kModeSelectionItems) / sizeof(kModeSelectionItems[0]),
};

} // namespace

void RegisterResources()
{
    MiniwinRegisterDialogTemplate(MAKEINTRESOURCE(101), &kModeSelection);
}
