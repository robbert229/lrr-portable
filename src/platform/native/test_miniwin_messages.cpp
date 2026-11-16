// SPDX-License-Identifier: LGPL-3.0-or-later
// miniwin's message queue as Main.c's window procedure sees it: SDL mouse
// events arrive as WM_MOUSEMOVE and button messages in client coordinates,
// and SDL's quit event (a closed window, or SIGTERM) as WM_CLOSE.
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
    return DefWindowProcA(hWnd, message, wParam, lParam);
}

// Pumps the queue as Main.c's window_message_dispatch_pump does.
static void Pump()
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static const Seen* Find(UINT message)
{
    for (const Seen& s : seen) {
        if (s.message == message) {
            return &s;
        }
    }
    return nullptr;
}

static void PushButton(SDL_WindowID id, Uint32 type, Uint8 button, float x, float y)
{
    SDL_Event event = {};
    event.type = type;
    event.button.windowID = id;
    event.button.button = button;
    event.button.down = type == SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.clicks = 1;
    event.button.x = x;
    event.button.y = y;
    CHECK(SDL_PushEvent(&event));
}

int main()
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    WNDCLASSA wc = {};
    wc.lpfnWndProc = Procedure;
    wc.lpszClassName = "test_miniwin_messages";
    CHECK(RegisterClassA(&wc) != 0);
    HWND hWnd = CreateWindowExA(0, wc.lpszClassName, "", WS_POPUP, 100, 100, 640, 480, NULL, NULL, NULL, NULL);
    CHECK(hWnd != NULL);
    if (!hWnd) {
        return 1;
    }
    SDL_WindowID id = SDL_GetWindowID(reinterpret_cast<SDL_Window*>(hWnd));
    Pump();
    seen.clear();

    // Mouse: client coordinates, unchanged by the window's position.
    SDL_Event motion = {};
    motion.type = SDL_EVENT_MOUSE_MOTION;
    motion.motion.windowID = id;
    motion.motion.x = 320.0f;
    motion.motion.y = 240.0f;
    CHECK(SDL_PushEvent(&motion));
    PushButton(id, SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, 123.0f, 45.0f);
    PushButton(id, SDL_EVENT_MOUSE_BUTTON_UP, SDL_BUTTON_LEFT, 123.0f, 45.0f);
    PushButton(id, SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_RIGHT, 7.0f, 8.0f);
    Pump();
    const Seen* move = Find(WM_MOUSEMOVE);
    CHECK(move && LOWORD(move->lParam) == 320 && HIWORD(move->lParam) == 240);
    const Seen* down = Find(WM_LBUTTONDOWN);
    CHECK(down && LOWORD(down->lParam) == 123 && HIWORD(down->lParam) == 45);
    CHECK(Find(WM_LBUTTONUP) != nullptr);
    const Seen* right = Find(WM_RBUTTONDOWN);
    CHECK(right && LOWORD(right->lParam) == 7 && HIWORD(right->lParam) == 8);

    // Screen coordinates are client coordinates: ClientToScreen leaves the
    // origin where Input.c and DirectDraw.c expect it.
    POINT origin = {0, 0};
    CHECK(ClientToScreen(hWnd, &origin) && origin.x == 0 && origin.y == 0);

    // Quit: SDL's quit event reaches the window as WM_CLOSE.
    seen.clear();
    SDL_Event quit = {};
    quit.type = SDL_EVENT_QUIT;
    CHECK(SDL_PushEvent(&quit));
    Pump();
    CHECK(Find(WM_CLOSE) != nullptr);

    // Closing the window itself does the same.
    seen.clear();
    SDL_Event close = {};
    close.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    close.window.windowID = id;
    CHECK(SDL_PushEvent(&close));
    Pump();
    CHECK(Find(WM_CLOSE) != nullptr);

    CHECK(DestroyWindow(hWnd));
    SDL_Quit();

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("miniwin messages: ok\n");
    return 0;
}
