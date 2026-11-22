// SPDX-License-Identifier: LGPL-3.0-or-later
// miniwin's window scale (MiniwinSetWindowScale), as LegoRR.ini's
// "window scale" sets it: the SDL window is the scale times the size the
// game gives it, and the game still sees its own size in the client and
// window rectangles and its own coordinates in mouse messages.
// Synthetic only: a dummy-driver SDL window, no game data.
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

static std::vector<MSG> seen;

static LRESULT CALLBACK Procedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MSG msg = {};
    msg.message = message;
    msg.lParam = lParam;
    seen.push_back(msg);
    return message == WM_CLOSE ? 0 : DefWindowProcA(hWnd, message, wParam, lParam);
}

static void Pump()
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        DispatchMessageA(&msg);
    }
}

static const MSG* Find(UINT message)
{
    for (const MSG& m : seen) {
        if (m.message == message) return &m;
    }
    return nullptr;
}

int main()
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    MiniwinSetWindowScale(2);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = Procedure;
    wc.lpszClassName = "test_miniwin_window_scale";
    CHECK(RegisterClassA(&wc) != 0);
    HWND hWnd = CreateWindowExA(0, wc.lpszClassName, "", WS_POPUP, 0, 0, 320, 200, NULL, NULL, NULL, NULL);
    CHECK(hWnd != NULL);
    if (!hWnd) return 1;
    SDL_Window* window = reinterpret_cast<SDL_Window*>(hWnd);

    // A new window is scaled; the game sees its own size.
    int w = 0, h = 0;
    CHECK(SDL_GetWindowSize(window, &w, &h) && w == 640 && h == 400);
    RECT rect;
    CHECK(GetClientRect(hWnd, &rect) && rect.right == 320 && rect.bottom == 200);

    // Main.c's windowed set-up: SetWindowPos to the mode's size.
    CHECK(SetWindowPos(hWnd, NULL, 100, 100, 640, 480, SWP_NOZORDER));
    CHECK(SDL_GetWindowSize(window, &w, &h) && w == 1280 && h == 960);
    CHECK(GetClientRect(hWnd, &rect) && rect.right == 640 && rect.bottom == 480);
    CHECK(GetWindowRect(hWnd, &rect) && rect.right - rect.left == 640 && rect.bottom - rect.top == 480);

    // Mouse messages are in the game's coordinates.
    Pump();
    seen.clear();
    SDL_Event motion = {};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = SDL_GetWindowID(window);
    motion.motion.x = 1000.0f;
    motion.motion.y = 800.0f;
    CHECK(SDL_PushEvent(&motion));
    SDL_Event button = {};
    button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    button.button.windowID = SDL_GetWindowID(window);
    button.button.button = SDL_BUTTON_LEFT;
    button.button.down = true;
    button.button.clicks = 1;
    button.button.x = 1279.0f;
    button.button.y = 959.0f;
    CHECK(SDL_PushEvent(&button));
    Pump();
    const MSG* move = Find(WM_MOUSEMOVE);
    CHECK(move && LOWORD(move->lParam) == 500 && HIWORD(move->lParam) == 400);
    const MSG* down = Find(WM_LBUTTONDOWN);
    CHECK(down && LOWORD(down->lParam) == 639 && HIWORD(down->lParam) == 479);

    // Scale 1 turns it off again.
    MiniwinSetWindowScale(1);
    CHECK(SetWindowPos(hWnd, NULL, 100, 100, 640, 480, SWP_NOZORDER));
    CHECK(SDL_GetWindowSize(window, &w, &h) && w == 640 && h == 480);
    CHECK(GetClientRect(hWnd, &rect) && rect.right == 640 && rect.bottom == 480);

    CHECK(DestroyWindow(hWnd));
    SDL_Quit();

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("miniwin window scale: ok\n");
    return 0;
}
