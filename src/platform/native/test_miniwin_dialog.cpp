// SPDX-License-Identifier: LGPL-3.0-or-later
// miniwin's modal dialogs as gods98 Init.c's "Mode Selection" procedure
// uses them: DialogBoxParamA runs a registered template, the procedure
// fills list boxes and checks radio buttons through SendMessageA, and the
// mouse and keyboard reach it as WM_COMMAND notifications until EndDialog.
// Synthetic only: a dummy-driver SDL window, no game data.
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>
#include <vector>

static int failures;

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) {                                                     \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,         \
                    __LINE__, #expr);                                      \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

enum { ID_LIST = 100, ID_RADIO_A = 101, ID_RADIO_B = 102, ID_LABEL = 103 };

constexpr DWORD kChild = 0x40000000L | 0x10000000L;

// Dialog units; the dialog draws two pixels per unit on each axis.
const MINIWIN_DLGITEM kItems[] = {
    {MINIWIN_DLGCLASS_BUTTON, kChild | WS_TABSTOP | BS_DEFPUSHBUTTON, 10, 80, 40, 14, IDOK, "OK"},
    {MINIWIN_DLGCLASS_BUTTON, kChild | WS_TABSTOP | BS_PUSHBUTTON, 60, 80, 40, 14, IDCANCEL, "Cancel"},
    {MINIWIN_DLGCLASS_LISTBOX, kChild | WS_TABSTOP | LBS_NOTIFY, 10, 10, 100, 40, ID_LIST, ""},
    {MINIWIN_DLGCLASS_STATIC, kChild | WS_GROUP | SS_LEFT, 10, 52, 100, 8, ID_LABEL, "Label"},
    {MINIWIN_DLGCLASS_BUTTON, kChild | BS_AUTORADIOBUTTON, 10, 60, 50, 10, ID_RADIO_A, "A"},
    {MINIWIN_DLGCLASS_BUTTON, kChild | BS_AUTORADIOBUTTON, 60, 60, 50, 10, ID_RADIO_B, "B"},
};

const MINIWIN_DLGTEMPLATE kTemplate = {
    0x80c800c0L, 120, 100, "Test", kItems, sizeof(kItems) / sizeof(kItems[0]),
};

struct Command {
    WORD id;
    WORD code;
};

static std::vector<Command> commands;
static HWND dialog;
static bool ended;
static int listSelectionAtOk = -2;
static bool radioBAtOk;

static SDL_WindowID WindowId()
{
    return SDL_GetWindowID(reinterpret_cast<SDL_Window*>(dialog));
}

// A click at dialog pixel (x, y).
static void PushClick(float x, float y)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = WindowId();
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = true;
    event.button.clicks = 1;
    event.button.x = x;
    event.button.y = y;
    SDL_PushEvent(&event);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.down = false;
    SDL_PushEvent(&event);
}

static void PushKey(SDL_Keycode key)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = WindowId();
    event.key.key = key;
    event.key.down = true;
    SDL_PushEvent(&event);
}

// The scripted run: select the list's second row, which enables OK, pick
// radio B, then press Enter for the default button.
static INT_PTR CALLBACK ScriptedProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG) {
        dialog = hWnd;
        CHECK(lParam == 1234);
        HWND list = GetDlgItem(hWnd, ID_LIST);
        CHECK(list != NULL);
        CHECK(GetDlgItem(hWnd, 999) == NULL);
        CHECK(SendMessageA(list, LB_ADDSTRING, 0, (LPARAM) "stale") == 0);
        // Init.c empties its lists by deleting until LB_ERR.
        int deletions = 0;
        while (SendMessageA(list, LB_DELETESTRING, 0, 0) != LB_ERR) {
            deletions++;
        }
        CHECK(deletions == 1);
        CHECK(SendMessageA(list, LB_ADDSTRING, 0, (LPARAM) "first") == 0);
        CHECK(SendMessageA(list, LB_ADDSTRING, 0, (LPARAM) "second") == 1);
        CHECK(SendMessageA(list, LB_SETCURSEL, 0, 0) == 0);
        char text[32];
        CHECK(SendMessageA(list, LB_GETTEXT, 1, (LPARAM) text) == 6);
        CHECK(strcmp(text, "second") == 0);
        CHECK(SendMessageA(list, LB_GETTEXT, 5, (LPARAM) text) == LB_ERR);

        HWND label = GetDlgItem(hWnd, ID_LABEL);
        SendMessageA(label, WM_SETTEXT, 0, (LPARAM) "Window Size");
        CHECK(SendMessageA(label, WM_GETTEXT, sizeof(text), (LPARAM) text) == 11);
        CHECK(strcmp(text, "Window Size") == 0);

        SendMessageA(GetDlgItem(hWnd, ID_RADIO_A), BM_SETCHECK, BST_CHECKED, 0);
        EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);
        SetFocus(list);

        PushKey(SDLK_RETURN);              // OK is disabled: nothing.
        PushClick(30, 20 + 2 + 11 + 5);    // The list's second row.
        PushClick(120 + 4, 120 + 10);      // Radio B.
        PushKey(SDLK_RETURN);              // OK.
        return FALSE;
    }
    if (message == WM_COMMAND) {
        commands.push_back({LOWORD(wParam), HIWORD(wParam)});
        if (LOWORD(wParam) == ID_LIST && HIWORD(wParam) == LBN_SELCHANGE) {
            EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            listSelectionAtOk = (int) SendMessageA(GetDlgItem(hWnd, ID_LIST), LB_GETCURSEL, 0, 0);
            radioBAtOk = SendMessageA(GetDlgItem(hWnd, ID_RADIO_B), BM_GETCHECK, 0, 0) == BST_CHECKED;
            CHECK(SendMessageA(GetDlgItem(hWnd, ID_RADIO_A), BM_GETCHECK, 0, 0) == BST_UNCHECKED);
            EndDialog(hWnd, wParam);
        }
        return TRUE;
    }
    return FALSE;
}

// Escape cancels.
static INT_PTR CALLBACK EscapeProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG) {
        dialog = hWnd;
        PushKey(SDLK_ESCAPE);
        return TRUE;
    }
    if (message == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)) {
        EndDialog(hWnd, wParam);
        return TRUE;
    }
    return FALSE;
}

int main()
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    CHECK(DialogBoxParamA(NULL, MAKEINTRESOURCE(55), NULL, ScriptedProcedure, 0) == -1);
    CHECK(MiniwinRegisterDialogTemplate(MAKEINTRESOURCE(55), &kTemplate));

    INT_PTR result = DialogBoxParamA(NULL, MAKEINTRESOURCE(55), NULL, ScriptedProcedure, 1234);
    CHECK(result == IDOK);
    CHECK(listSelectionAtOk == 1);
    CHECK(radioBAtOk);
    CHECK(commands.size() == 3);
    if (commands.size() == 3) {
        CHECK(commands[0].id == ID_LIST && commands[0].code == LBN_SELCHANGE);
        CHECK(commands[1].id == ID_RADIO_B && commands[1].code == BN_CLICKED);
        CHECK(commands[2].id == IDOK && commands[2].code == BN_CLICKED);
    }
    // The dialog's controls are gone with it.
    CHECK(GetDlgItem(dialog, ID_LIST) == NULL);

    CHECK(DialogBoxParamA(NULL, MAKEINTRESOURCE(55), NULL, EscapeProcedure, 0) == IDCANCEL);

    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin dialog: ok\n");
    return 0;
}
