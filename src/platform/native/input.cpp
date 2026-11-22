// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port's input layer (input.h). */
#include "platform/native/input.h"

#include <SDL3/SDL.h>
#include <miniwin/miniwinpresentation.h>

#include "platform/native/config.h"
#include "platform/native/input_latch.h"
#include "platform/native/touch_gestures.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace {

/* The events the watch keeps for the next poll, at most. */
const size_t kEventLimit = 1024;
/* How far inside the picture's edge, in its pixels, a pointer that must
 * not scroll the map is kept: more than Lego.cfg's MouseScrollBorder (5). */
const float kInsideMargin = 16.0f;

bool g_started = false;
LRR_TouchConfig g_config;

/* Filled by the watch, on whatever thread SDL queues events from. */
SDL_Mutex* g_mutex = NULL;
std::vector<SDL_Event> g_events;
bool g_focusLost = false;

/* The game's thread only. */
LRR_InputLatch g_latch;
LRR_TouchGestures g_gestures;
SDL_WindowID g_windowID = 0;
/* The buttons the touch layer has pressed and not released, a bit each. */
unsigned int g_touchButtons = 0;
bool g_textWanted = false;
bool g_textWantedLast = false;
bool g_textAuto = false;
bool g_keys[SDL_SCANCODE_COUNT];

bool IsTouchScreen(SDL_TouchID touchID)
{
    /* A finger on a touch screen; not a trackpad, and not the touches SDL
     * makes from a mouse. */
    return touchID != SDL_MOUSE_TOUCHID && SDL_GetTouchDeviceType(touchID) == SDL_TOUCH_DEVICE_DIRECT;
}

bool SDLCALL Watch(void* userdata, SDL_Event* event)
{
    (void)userdata;
    bool keep = false;
    switch (event->type) {
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        keep = g_config.enabled && IsTouchScreen(event->tfinger.touchID);
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        keep = !event->key.repeat;
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        SDL_LockMutex(g_mutex);
        g_events.clear();
        g_focusLost = true;
        SDL_UnlockMutex(g_mutex);
        break;
    default:
        break;
    }
    if (keep) {
        SDL_LockMutex(g_mutex);
        if (g_events.size() < kEventLimit) {
            g_events.push_back(*event);
        }
        SDL_UnlockMutex(g_mutex);
    }
    return true;
}

void Start()
{
    g_started = true;
    g_config = LRR_GetTouchConfig();
    if (g_config.enabled) {
        /* The gestures make the mouse; SDL's own mapping would press the
         * left button the moment a finger lands. */
        SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
        LRR_TouchSettings settings;
        settings.longPressHover = g_config.longPressHover;
        settings.scroll = g_config.scroll;
        settings.zoom = g_config.zoom;
        settings.rotate = g_config.rotate;
        settings.twoFingerTap = g_config.twoFingerTap;
        settings.threeFingerKeyboard = g_config.threeFingerKeyboard;
        g_gestures.SetSettings(settings);
        SDL_Log("Touch controls on (tweaks:touch)");
    }
    if (g_config.backEscape) {
        /* Android: Back stays in the game, as a key, rather than closing
         * the activity. */
        SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    }
    g_mutex = SDL_CreateMutex();
    if (g_mutex == NULL || !SDL_AddEventWatch(Watch, NULL)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "Input: no event watch: %s", SDL_GetError());
    }
}

SDL_Window* GameWindow()
{
    SDL_Window* window = g_windowID != 0 ? SDL_GetWindowFromID(g_windowID) : NULL;
    if (window == NULL) {
        window = SDL_GetKeyboardFocus();
    }
    if (window == NULL) {
        window = SDL_GetMouseFocus();
    }
    return window;
}

/* A finger's position, in the gestures' units (the window's height is 1). */
void FingerPosition(const SDL_TouchFingerEvent& finger, float* x, float* y)
{
    int w = 0;
    int h = 0;
    SDL_Window* window = SDL_GetWindowFromID(finger.windowID);
    if (window == NULL || !SDL_GetWindowSize(window, &w, &h) || h <= 0) {
        w = h = 1;
    }
    *x = finger.x * (float)w / (float)h;
    *y = finger.y;
}

void Feed(const SDL_Event& event)
{
    switch (event.type) {
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED: {
        const SDL_TouchFingerEvent& finger = event.tfinger;
        float x, y;
        if (finger.windowID != 0) {
            g_windowID = finger.windowID;
        }
        FingerPosition(finger, &x, &y);
        uint64_t ms = SDL_NS_TO_MS(finger.timestamp);
        if (event.type == SDL_EVENT_FINGER_DOWN) {
            g_gestures.FingerDown(finger.fingerID, x, y, ms);
        }
        else if (event.type == SDL_EVENT_FINGER_MOTION) {
            g_gestures.FingerMove(finger.fingerID, x, y, ms);
        }
        else {
            g_gestures.FingerUp(finger.fingerID, x, y, ms, event.type == SDL_EVENT_FINGER_CANCELED);
        }
        break;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        SDL_Scancode scancode = event.key.scancode;
        if (scancode == SDL_SCANCODE_AC_BACK && g_config.backEscape) {
            scancode = SDL_SCANCODE_ESCAPE;
        }
        g_latch.Key((int)scancode, event.key.down);
        break;
    }
    default:
        break;
    }
}

/* Moves the pointer as the gestures ask. Positions in the picture go
 * through miniwin's fit of it in the window: the window's centre is the
 * picture's, which gives the picture's size. */
void MoveCursor(const LRR_TouchOutput& out)
{
    if (out.cursor == LRR_TouchOutput::kCursorKeep) {
        return;
    }
    SDL_Window* window = GameWindow();
    int w = 0;
    int h = 0;
    if (window == NULL || !SDL_GetWindowSize(window, &w, &h) || w <= 0 || h <= 0) {
        return;
    }
    if (out.cursor == LRR_TouchOutput::kCursorAt) {
        SDL_WarpMouseInWindow(window, out.x * (float)h, out.y * (float)h);
        return;
    }

    float centreX, centreY;
    Miniwin_WindowToVirtual(window, (float)w * 0.5f, (float)h * 0.5f, &centreX, &centreY);
    float width = centreX * 2.0f;
    float height = centreY * 2.0f;
    float vx, vy;
    if (out.cursor == LRR_TouchOutput::kCursorInside) {
        Miniwin_WindowToVirtual(window, out.x * (float)h, out.y * (float)h, &vx, &vy);
        float margin = std::min(kInsideMargin, std::min(centreX, centreY));
        vx = std::max(margin, std::min(vx, width - 1.0f - margin));
        vy = std::max(margin, std::min(vy, height - 1.0f - margin));
    }
    else {
        /* kCursorEdge: where the direction from the centre leaves the
         * picture. The fit scales both axes alike, so a direction in the
         * window is the same direction in the picture. */
        float t = 1e9f;
        if (std::fabs(out.x) > 1e-6f) {
            t = std::min(t, centreX / std::fabs(out.x));
        }
        if (std::fabs(out.y) > 1e-6f) {
            t = std::min(t, centreY / std::fabs(out.y));
        }
        if (t >= 1e9f) {
            return;
        }
        vx = std::max(0.0f, std::min(centreX + out.x * t, width - 1.0f));
        vy = std::max(0.0f, std::min(centreY + out.y * t, height - 1.0f));
    }
    float wx, wy;
    Miniwin_VirtualToWindow(window, (int)vx, (int)vy, &wx, &wy);
    SDL_WarpMouseInWindow(window, wx, wy);
}

/* Queues a mouse-button change for miniwin's message pump. */
void PushButton(const LRR_ButtonChange& change)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = change.down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.timestamp = SDL_GetTicksNS();
    event.button.windowID = g_windowID;
    event.button.which = SDL_TOUCH_MOUSEID;
    event.button.button = (Uint8)change.button;
    event.button.down = change.down;
    event.button.clicks = 1;
    SDL_GetMouseState(&event.button.x, &event.button.y);
    SDL_PushEvent(&event);
    unsigned int bit = change.button > 0 && change.button < 32 ? 1u << change.button : 0;
    g_touchButtons = change.down ? (g_touchButtons | bit) : (g_touchButtons & ~bit);
}

/* Focus lost: drop what is pending, and release the buttons the touch
 * layer holds, as SDL releases its keys. */
void LoseFocus()
{
    g_latch.Reset();
    g_gestures.Reset();
    for (int button = 1; button < 32; ++button) {
        if (g_touchButtons & (1u << button)) {
            PushButton(LRR_ButtonChange{button, false});
        }
    }
}

void UpdateTextInput(bool toggle)
{
    SDL_Window* window = GameWindow();
    if (window == NULL) {
        return;
    }
    if (toggle) {
        if (SDL_TextInputActive(window)) {
            SDL_StopTextInput(window);
        }
        else {
            SDL_StartTextInput(window);
        }
        g_textAuto = false;
    }

    bool wanted = g_textWanted;
    bool started = wanted && !g_textWantedLast;
    g_textWanted = false;
    g_textWantedLast = wanted;
    if (!g_config.keyboard || !SDL_HasScreenKeyboardSupport()) {
        return;
    }
    /* Opened when the game starts taking text, and closed when it stops,
     * unless the player opened it; if the player closes it meanwhile, it
     * stays closed. */
    if (started && !SDL_TextInputActive(window)) {
        SDL_StartTextInput(window);
        g_textAuto = true;
    }
    else if (!wanted && g_textAuto) {
        if (SDL_TextInputActive(window)) {
            SDL_StopTextInput(window);
        }
        g_textAuto = false;
    }
}

} // namespace

const bool* LRR_InputKeyboardState(int* numkeys)
{
    if (!g_started) {
        Start();
    }

    std::vector<SDL_Event> events;
    bool focusLost = false;
    if (g_mutex != NULL) {
        SDL_LockMutex(g_mutex);
        events.swap(g_events);
        focusLost = g_focusLost;
        g_focusLost = false;
        SDL_UnlockMutex(g_mutex);
    }
    if (focusLost) {
        LoseFocus();
    }
    for (const SDL_Event& event : events) {
        Feed(event);
    }

    LRR_TouchOutput out;
    g_gestures.Update(SDL_GetTicks(), &out);
    MoveCursor(out);
    for (const LRR_ButtonChange& change : out.buttons) {
        g_latch.Button(change.button, change.down);
    }
    for (SDL_Scancode scancode : out.keyTaps) {
        g_latch.Key((int)scancode, true);
        g_latch.Key((int)scancode, false);
    }
    std::vector<LRR_ButtonChange> buttons;
    g_latch.Poll(&buttons);
    for (const LRR_ButtonChange& change : buttons) {
        PushButton(change);
    }
    UpdateTextInput(out.toggleKeyboard);

    int count = 0;
    const bool* state = SDL_GetKeyboardState(&count);
    if (count > SDL_SCANCODE_COUNT) {
        count = SDL_SCANCODE_COUNT;
    }
    for (int i = 0; i < count; ++i) {
        g_keys[i] = (state != NULL && state[i]) || g_latch.KeyDown(i);
    }
    for (SDL_Scancode scancode : out.keysHeld) {
        if ((int)scancode < count) {
            g_keys[scancode] = true;
        }
    }
    if (g_config.backEscape && state != NULL && SDL_SCANCODE_AC_BACK < count && state[SDL_SCANCODE_AC_BACK]) {
        g_keys[SDL_SCANCODE_ESCAPE] = true;
    }
    *numkeys = count;
    return g_keys;
}

void LRR_InputTextWanted(void)
{
    g_textWanted = true;
}
