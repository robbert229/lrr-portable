// SPDX-License-Identifier: LGPL-3.0-or-later
// miniwin's key messages as a window procedure sees them: SDL key events
// arrive as WM_KEYDOWN/WM_KEYUP (or WM_SYSKEYDOWN/WM_SYSKEYUP with Alt, and
// for F10) carrying Windows virtual-key codes, not SDL keycodes, with
// Windows' lParam flags; and Alt+F4 reaches the window as WM_CLOSE through
// DefWindowProcA. Synthetic only: a dummy-driver SDL window, no game data.
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include <stdio.h>
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

struct Seen {
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
};

static std::vector<Seen> seen;

static LRESULT CALLBACK Procedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    seen.push_back({message, wParam, lParam});
    if (message == WM_CLOSE) {
        return 0; // As Main.c's windowed procedure: the game decides.
    }
    // As Main.c's windowed procedure: keys go to the default handling.
    return DefWindowProcA(hWnd, message, wParam, lParam);
}

static void Pump()
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static SDL_WindowID windowId;

static void PushKey(bool down, SDL_Keycode key, SDL_Scancode scancode, SDL_Keymod mod = SDL_KMOD_NONE,
                    bool repeat = false)
{
    SDL_Event event = {};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.windowID = windowId;
    event.key.key = key;
    event.key.scancode = scancode;
    event.key.mod = mod;
    event.key.down = down;
    event.key.repeat = repeat;
    CHECK(SDL_PushEvent(&event));
}

// One key press: the message and wParam the window procedure receives.
static Seen Press(SDL_Keycode key, SDL_Scancode scancode, SDL_Keymod mod = SDL_KMOD_NONE)
{
    seen.clear();
    PushKey(true, key, scancode, mod);
    Pump();
    CHECK(seen.size() == 1);
    return seen.empty() ? Seen{0, 0, 0} : seen[0];
}

struct Case {
    SDL_Keycode key;
    SDL_Scancode scancode;
    WPARAM vk;
};

int main()
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    WNDCLASSA wc = {};
    wc.lpfnWndProc = Procedure;
    wc.lpszClassName = "test_miniwin_keys";
    CHECK(RegisterClassA(&wc) != 0);
    HWND hWnd = CreateWindowExA(0, wc.lpszClassName, "", WS_POPUP, 0, 0, 640, 480, NULL, NULL, NULL, NULL);
    CHECK(hWnd != NULL);
    if (!hWnd) {
        return 1;
    }
    windowId = SDL_GetWindowID(reinterpret_cast<SDL_Window*>(hWnd));
    Pump();

    // The SDL keycode to Windows virtual-key table.
    static const Case cases[] = {
        {SDLK_A, SDL_SCANCODE_A, 'A'},
        {SDLK_Z, SDL_SCANCODE_Z, 'Z'},
        {SDLK_0, SDL_SCANCODE_0, '0'},
        {SDLK_9, SDL_SCANCODE_9, '9'},
        {SDLK_ESCAPE, SDL_SCANCODE_ESCAPE, VK_ESCAPE},
        {SDLK_RETURN, SDL_SCANCODE_RETURN, VK_RETURN},
        {SDLK_KP_ENTER, SDL_SCANCODE_KP_ENTER, VK_RETURN},
        {SDLK_SPACE, SDL_SCANCODE_SPACE, VK_SPACE},
        {SDLK_BACKSPACE, SDL_SCANCODE_BACKSPACE, VK_BACK},
        {SDLK_TAB, SDL_SCANCODE_TAB, VK_TAB},
        {SDLK_LSHIFT, SDL_SCANCODE_LSHIFT, VK_SHIFT},
        {SDLK_RSHIFT, SDL_SCANCODE_RSHIFT, VK_SHIFT},
        {SDLK_LCTRL, SDL_SCANCODE_LCTRL, VK_CONTROL},
        {SDLK_RCTRL, SDL_SCANCODE_RCTRL, VK_CONTROL},
        {SDLK_PAUSE, SDL_SCANCODE_PAUSE, VK_PAUSE},
        {SDLK_CAPSLOCK, SDL_SCANCODE_CAPSLOCK, VK_CAPITAL},
        {SDLK_PAGEUP, SDL_SCANCODE_PAGEUP, VK_PRIOR},
        {SDLK_PAGEDOWN, SDL_SCANCODE_PAGEDOWN, VK_NEXT},
        {SDLK_END, SDL_SCANCODE_END, VK_END},
        {SDLK_HOME, SDL_SCANCODE_HOME, VK_HOME},
        {SDLK_LEFT, SDL_SCANCODE_LEFT, VK_LEFT},
        {SDLK_UP, SDL_SCANCODE_UP, VK_UP},
        {SDLK_RIGHT, SDL_SCANCODE_RIGHT, VK_RIGHT},
        {SDLK_DOWN, SDL_SCANCODE_DOWN, VK_DOWN},
        {SDLK_INSERT, SDL_SCANCODE_INSERT, VK_INSERT},
        {SDLK_DELETE, SDL_SCANCODE_DELETE, VK_DELETE},
        {SDLK_PRINTSCREEN, SDL_SCANCODE_PRINTSCREEN, VK_SNAPSHOT},
        {SDLK_F1, SDL_SCANCODE_F1, VK_F1},
        {SDLK_F12, SDL_SCANCODE_F12, VK_F12},
        {SDLK_KP_0, SDL_SCANCODE_KP_0, VK_NUMPAD0},
        {SDLK_KP_9, SDL_SCANCODE_KP_9, VK_NUMPAD9},
        {SDLK_KP_MULTIPLY, SDL_SCANCODE_KP_MULTIPLY, VK_MULTIPLY},
        {SDLK_KP_PLUS, SDL_SCANCODE_KP_PLUS, VK_ADD},
        {SDLK_KP_MINUS, SDL_SCANCODE_KP_MINUS, VK_SUBTRACT},
        {SDLK_KP_PERIOD, SDL_SCANCODE_KP_PERIOD, VK_DECIMAL},
        {SDLK_KP_DIVIDE, SDL_SCANCODE_KP_DIVIDE, VK_DIVIDE},
        {SDLK_NUMLOCKCLEAR, SDL_SCANCODE_NUMLOCKCLEAR, VK_NUMLOCK},
        {SDLK_SCROLLLOCK, SDL_SCANCODE_SCROLLLOCK, VK_SCROLL},
        {SDLK_SEMICOLON, SDL_SCANCODE_SEMICOLON, VK_OEM_1},
        {SDLK_EQUALS, SDL_SCANCODE_EQUALS, VK_OEM_PLUS},
        {SDLK_COMMA, SDL_SCANCODE_COMMA, VK_OEM_COMMA},
        {SDLK_MINUS, SDL_SCANCODE_MINUS, VK_OEM_MINUS},
        {SDLK_PERIOD, SDL_SCANCODE_PERIOD, VK_OEM_PERIOD},
        {SDLK_SLASH, SDL_SCANCODE_SLASH, VK_OEM_2},
        {SDLK_GRAVE, SDL_SCANCODE_GRAVE, VK_OEM_3},
        {SDLK_LEFTBRACKET, SDL_SCANCODE_LEFTBRACKET, VK_OEM_4},
        {SDLK_BACKSLASH, SDL_SCANCODE_BACKSLASH, VK_OEM_5},
        {SDLK_RIGHTBRACKET, SDL_SCANCODE_RIGHTBRACKET, VK_OEM_6},
        {SDLK_APOSTROPHE, SDL_SCANCODE_APOSTROPHE, VK_OEM_7},
        // A layout's letter follows the layout (AZERTY's A is at Q's place).
        {SDLK_A, SDL_SCANCODE_Q, 'A'},
        // A key the layout gives no Latin character: its physical position.
        {0x0444 /* Cyrillic ef */, SDL_SCANCODE_A, 'A'},
        {0x00e9 /* e acute */, SDL_SCANCODE_2, '2'},
        {SDLK_UNKNOWN, SDL_SCANCODE_NONUSBACKSLASH, VK_OEM_102},
    };
    for (const Case& c : cases) {
        Seen s = Press(c.key, c.scancode);
        if (s.message != WM_KEYDOWN || s.wParam != c.vk) {
            fprintf(stderr, "key 0x%x scancode %d: message 0x%x wParam 0x%x, want WM_KEYDOWN 0x%x\n",
                    (unsigned) c.key, (int) c.scancode, s.message, (unsigned) s.wParam, (unsigned) c.vk);
            ++failures;
        }
        PushKey(false, c.key, c.scancode);
        Pump();
    }

    // Never an SDL keycode: SDLK_ESCAPE happens to equal VK_ESCAPE, but an
    // arrow key (SDLK_UP is 0x40000052) must not leak through.
    CHECK(Press(SDLK_UP, SDL_SCANCODE_UP).wParam == VK_UP);

    // lParam: repeat count 1; extended, previous-state and transition flags.
    {
        Seen down = Press(SDLK_A, SDL_SCANCODE_A);
        CHECK(LOWORD(down.lParam) == 1);
        CHECK(HIWORD(down.lParam) == 0);

        seen.clear();
        PushKey(true, SDLK_A, SDL_SCANCODE_A, SDL_KMOD_NONE, true);
        PushKey(false, SDLK_A, SDL_SCANCODE_A);
        Pump();
        CHECK(seen.size() == 2);
        if (seen.size() == 2) {
            CHECK(seen[0].message == WM_KEYDOWN && (HIWORD(seen[0].lParam) & KF_REPEAT));
            CHECK(!(HIWORD(seen[0].lParam) & KF_UP));
            CHECK(seen[1].message == WM_KEYUP && seen[1].wParam == 'A');
            CHECK((HIWORD(seen[1].lParam) & (KF_REPEAT | KF_UP)) == (KF_REPEAT | KF_UP));
            CHECK(LOWORD(seen[1].lParam) == 1);
        }

        CHECK(HIWORD(Press(SDLK_RCTRL, SDL_SCANCODE_RCTRL).lParam) & KF_EXTENDED);
        CHECK(HIWORD(Press(SDLK_LEFT, SDL_SCANCODE_LEFT).lParam) & KF_EXTENDED);
        CHECK(!(HIWORD(Press(SDLK_KP_4, SDL_SCANCODE_KP_4).lParam) & KF_EXTENDED));
        CHECK(!(HIWORD(Press(SDLK_LCTRL, SDL_SCANCODE_LCTRL).lParam) & KF_EXTENDED));
    }

    // Alt and F10 make system keys; Ctrl+Alt does not.
    {
        Seen alt = Press(SDLK_LALT, SDL_SCANCODE_LALT, SDL_KMOD_LALT);
        CHECK(alt.message == WM_SYSKEYDOWN && alt.wParam == VK_MENU);
        CHECK(HIWORD(alt.lParam) & KF_ALTDOWN);

        Seen altX = Press(SDLK_X, SDL_SCANCODE_X, SDL_KMOD_LALT);
        CHECK(altX.message == WM_SYSKEYDOWN && altX.wParam == 'X');
        CHECK(HIWORD(altX.lParam) & KF_ALTDOWN);

        seen.clear();
        PushKey(false, SDLK_LALT, SDL_SCANCODE_LALT);
        Pump();
        CHECK(seen.size() == 1 && seen[0].message == WM_SYSKEYUP && seen[0].wParam == VK_MENU);

        Seen f10 = Press(SDLK_F10, SDL_SCANCODE_F10);
        CHECK(f10.message == WM_SYSKEYDOWN && f10.wParam == VK_F10);
        CHECK(!(HIWORD(f10.lParam) & KF_ALTDOWN));

        Seen ctrlAlt = Press(SDLK_X, SDL_SCANCODE_X, (SDL_Keymod) (SDL_KMOD_LALT | SDL_KMOD_LCTRL));
        CHECK(ctrlAlt.message == WM_KEYDOWN && ctrlAlt.wParam == 'X');
    }

    // Alt+F4: the default handling sends WM_CLOSE; F4 alone does not.
    {
        seen.clear();
        PushKey(true, SDLK_F4, SDL_SCANCODE_F4);
        Pump();
        CHECK(seen.size() == 1 && seen[0].message == WM_KEYDOWN && seen[0].wParam == VK_F4);

        seen.clear();
        PushKey(true, SDLK_F4, SDL_SCANCODE_F4, SDL_KMOD_LALT);
        Pump();
        CHECK(seen.size() == 2);
        if (seen.size() == 2) {
            CHECK(seen[0].message == WM_SYSKEYDOWN && seen[0].wParam == VK_F4);
            CHECK(seen[1].message == WM_CLOSE);
        }
    }

    CHECK(DestroyWindow(hWnd));
    SDL_Quit();

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("miniwin keys: ok\n");
    return 0;
}
