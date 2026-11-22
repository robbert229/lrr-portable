// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_touch_input — the input latch (input_latch.h) and the touch gestures
 * (touch_gestures.h), driven with synthetic events and polls (issue #986).
 * The latch: a press and its release between two polls still reach the
 * game, a press seen for one poll and released in the next; the gestures:
 * which mouse buttons, keys and pointer moves each touch makes. */
#include <math.h>
#include <stdio.h>

#include <vector>

#include "lrr_test.h"
#include "platform/native/input_latch.h"
#include "platform/native/touch_gestures.h"

static bool Is(const std::vector<LRR_ButtonChange>& changes, size_t index, int button, bool down)
{
    return index < changes.size() && changes[index].button == button && changes[index].down == down;
}

static bool Holds(const LRR_TouchOutput& out, SDL_Scancode key)
{
    for (SDL_Scancode held : out.keysHeld) {
        if (held == key) return true;
    }
    return false;
}

/* The window procedure's view of the left button: down after a press
 * message, up after a release, as Main.c keeps it. */
struct GameButton {
    bool down = false;
    void Deliver(const std::vector<LRR_ButtonChange>& changes)
    {
        for (const LRR_ButtonChange& change : changes) {
            if (change.button == 1) down = change.down;
        }
    }
};

static void test_latch_short_click()
{
    /* A tap shorter than a frame: press and release before one poll. */
    LRR_InputLatch latch;
    GameButton game;
    std::vector<LRR_ButtonChange> out;
    latch.Button(1, true);
    latch.Button(1, false);

    latch.Poll(&out);
    game.Deliver(out);
    CHECK(out.size() == 1 && Is(out, 0, 1, true), "latch: the press alone in the first poll");
    CHECK(game.down, "latch: the game sees the one-frame tap's press");

    out.clear();
    latch.Poll(&out);
    game.Deliver(out);
    CHECK(out.size() == 1 && Is(out, 0, 1, false), "latch: the release in the next poll");
    CHECK(!game.down, "latch: the game sees the release a poll later");

    out.clear();
    latch.Poll(&out);
    CHECK(out.empty(), "latch: nothing left");
}

static void test_latch_double_click()
{
    /* Two taps before a poll: four polls, the game sees both presses. */
    LRR_InputLatch latch;
    std::vector<LRR_ButtonChange> out;
    int presses = 0;
    latch.Button(1, true);
    latch.Button(1, false);
    latch.Button(1, true);
    latch.Button(1, false);
    for (int poll = 0; poll < 4; ++poll) {
        out.clear();
        latch.Poll(&out);
        CHECK(out.size() == 1, "latch: one change of a button per poll");
        if (!out.empty() && out[0].down) ++presses;
    }
    CHECK(presses == 2, "latch: both taps' presses reach the game");
}

static void test_latch_two_buttons()
{
    /* Different buttons do not hold each other up. */
    LRR_InputLatch latch;
    std::vector<LRR_ButtonChange> out;
    latch.Button(1, true);
    latch.Button(3, true);
    latch.Poll(&out);
    CHECK(out.size() == 2 && Is(out, 0, 1, true) && Is(out, 1, 3, true), "latch: two buttons in one poll");
}

static void test_latch_short_key()
{
    /* A key pressed and released before one poll is down for one poll. */
    LRR_InputLatch latch;
    latch.Key(44, true);
    latch.Key(44, false);
    latch.Poll(NULL);
    CHECK(latch.KeyDown(44), "latch: a one-frame key is down in the first poll");
    latch.Poll(NULL);
    CHECK(!latch.KeyDown(44), "latch: and up in the next");
}

static void test_latch_typed_word()
{
    /* An on-screen keyboard sends a word at once: one key per poll, in
     * order, each released before the next is pressed. */
    LRR_InputLatch latch;
    const int word[] = {11, 8, 15, 15, 18}; /* h e l l o */
    for (int key : word) {
        latch.Key(key, true);
        latch.Key(key, false);
    }
    std::vector<int> typed;
    bool prev[LRR_InputLatch::kKeyCount] = {};
    for (int poll = 0; poll < 12; ++poll) {
        latch.Poll(NULL);
        int down = 0;
        for (int key = 0; key < LRR_InputLatch::kKeyCount; ++key) {
            if (latch.KeyDown(key)) {
                ++down;
                if (!prev[key]) typed.push_back(key);
            }
            prev[key] = latch.KeyDown(key);
        }
        CHECK(down <= 1, "latch: at most one typed key down at a time");
    }
    CHECK(typed == std::vector<int>(word, word + 5), "latch: the word's keys reach the game in order, both l's");
}

static void test_latch_shifted_key()
{
    /* Shift and a letter: Shift is still down when the letter is. */
    LRR_InputLatch latch;
    latch.Key(225, true); /* left shift */
    latch.Key(4, true);   /* a */
    latch.Key(4, false);
    latch.Key(225, false);
    bool together = false;
    for (int poll = 0; poll < 5; ++poll) {
        latch.Poll(NULL);
        if (latch.KeyDown(225) && latch.KeyDown(4)) together = true;
    }
    CHECK(together, "latch: Shift held with the letter");
    CHECK(!latch.KeyDown(225) && !latch.KeyDown(4), "latch: both released at the end");
}

static void test_latch_held_key()
{
    /* A key held across polls stays down; Reset releases it. */
    LRR_InputLatch latch;
    latch.Key(41, true);
    latch.Poll(NULL);
    latch.Poll(NULL);
    CHECK(latch.KeyDown(41), "latch: a held key stays down");
    latch.Reset();
    CHECK(!latch.KeyDown(41), "latch: Reset releases it");
    latch.Key(-1, true);
    latch.Key(LRR_InputLatch::kKeyCount, true);
    latch.Poll(NULL);
    CHECK(!latch.KeyDown(-1), "latch: key codes out of range are ignored");
}

static void test_tap()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.5f, 0.4f, 1000);
    touch.FingerUp(1, 0.5f, 0.4f, 1030, false);
    touch.Update(1040, &out);
    CHECK(out.cursor == LRR_TouchOutput::kCursorAt && out.x == 0.5f && out.y == 0.4f, "tap: pointer to the finger");
    CHECK(out.buttons.size() == 2 && Is(out.buttons, 0, 1, true) && Is(out.buttons, 1, 1, false),
          "tap: a left click");

    /* Through the latch, a tap between two polls is seen by the game. */
    LRR_InputLatch latch;
    GameButton game;
    std::vector<LRR_ButtonChange> polled;
    for (const LRR_ButtonChange& change : out.buttons) latch.Button(change.button, change.down);
    latch.Poll(&polled);
    game.Deliver(polled);
    CHECK(game.down, "tap: through the latch, the game sees the press");
    polled.clear();
    latch.Poll(&polled);
    game.Deliver(polled);
    CHECK(!game.down, "tap: then the release");
}

static void test_touch_moves_pointer_without_pressing()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.5f, 0.5f, 1000);
    touch.Update(1016, &out);
    CHECK(out.buttons.empty(), "touch: no button while the finger rests");
    CHECK(out.cursor == LRR_TouchOutput::kCursorAt, "touch: pointer to the finger");
}

static void test_drag()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.5f, 0.5f, 1000);
    touch.FingerMove(1, 0.51f, 0.5f, 1010);
    touch.Update(1016, &out);
    CHECK(out.buttons.empty(), "drag: a move within the slop presses nothing");
    touch.FingerMove(1, 0.6f, 0.5f, 1030);
    touch.Update(1032, &out);
    CHECK(out.buttons.size() == 1 && Is(out.buttons, 0, 1, true), "drag: left down past the slop");
    CHECK(out.cursor == LRR_TouchOutput::kCursorAt && out.x == 0.6f, "drag: pointer follows");
    touch.FingerUp(1, 0.7f, 0.5f, 1050, false);
    touch.Update(1052, &out);
    CHECK(out.buttons.size() == 1 && Is(out.buttons, 0, 1, false), "drag: left up on lift");
}

static void test_long_press_hover()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.5f, 0.5f, 1000);
    touch.Update(1000 + LRR_TouchGestures::kLongPressMs + 10, &out);
    CHECK(out.buttons.empty(), "long press: no button");
    touch.FingerMove(1, 0.7f, 0.5f, 1600);
    touch.Update(1610, &out);
    CHECK(out.buttons.empty() && out.cursor == LRR_TouchOutput::kCursorAt && out.x == 0.7f,
          "long press: then the pointer follows without pressing");
    touch.FingerUp(1, 0.7f, 0.5f, 1700, false);
    touch.Update(1710, &out);
    CHECK(out.buttons.empty(), "long press: lifting clicks nothing");
}

static void test_long_press_off()
{
    LRR_TouchSettings settings;
    settings.longPressHover = false;
    LRR_TouchGestures touch(settings);
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.5f, 0.5f, 1000);
    touch.Update(1000 + LRR_TouchGestures::kLongPressMs + 10, &out);
    CHECK(out.buttons.size() == 1 && Is(out.buttons, 0, 1, true), "long press off: holding presses left");
    touch.FingerUp(1, 0.5f, 0.5f, 1700, false);
    touch.Update(1710, &out);
    CHECK(out.buttons.size() == 1 && Is(out.buttons, 0, 1, false), "long press off: lifting releases it");
}

static void test_canceled_tap()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.5f, 0.5f, 1000);
    touch.FingerUp(1, 0.5f, 0.5f, 1030, true);
    touch.Update(1040, &out);
    CHECK(out.buttons.empty(), "cancel: a cancelled touch clicks nothing");
}

static void test_two_finger_tap()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.4f, 0.5f, 1000);
    touch.FingerDown(2, 0.6f, 0.5f, 1020);
    touch.Update(1030, &out);
    touch.FingerUp(1, 0.4f, 0.5f, 1100, false);
    touch.FingerUp(2, 0.6f, 0.5f, 1110, false);
    touch.Update(1120, &out);
    CHECK(out.buttons.empty(), "two-finger tap: no click");
    CHECK(out.keyTaps.size() == 1 && out.keyTaps[0] == SDL_SCANCODE_SPACE, "two-finger tap: Space");

    LRR_TouchSettings settings;
    settings.twoFingerTap = LRR_TOUCH_TAP_RIGHT_CLICK;
    LRR_TouchGestures right(settings);
    right.FingerDown(1, 0.4f, 0.5f, 1000);
    right.FingerDown(2, 0.6f, 0.5f, 1020);
    right.FingerUp(1, 0.4f, 0.5f, 1100, false);
    right.FingerUp(2, 0.6f, 0.5f, 1110, false);
    right.Update(1120, &out);
    CHECK(out.keyTaps.empty() && out.buttons.size() == 2 && Is(out.buttons, 0, 3, true) &&
              Is(out.buttons, 1, 3, false),
          "two-finger tap = right click: a right click");

    settings.twoFingerTap = LRR_TOUCH_TAP_OFF;
    LRR_TouchGestures off(settings);
    off.FingerDown(1, 0.4f, 0.5f, 1000);
    off.FingerDown(2, 0.6f, 0.5f, 1020);
    off.FingerUp(1, 0.4f, 0.5f, 1100, false);
    off.FingerUp(2, 0.6f, 0.5f, 1110, false);
    off.Update(1120, &out);
    CHECK(out.keyTaps.empty() && out.buttons.empty(), "two-finger tap off: nothing");
}

static void test_slow_two_finger_touch_is_not_a_tap()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.4f, 0.5f, 1000);
    touch.FingerDown(2, 0.6f, 0.5f, 1020);
    touch.FingerUp(1, 0.4f, 0.5f, 1000 + LRR_TouchGestures::kMultiTapMs + 100, false);
    touch.FingerUp(2, 0.6f, 0.5f, 1000 + LRR_TouchGestures::kMultiTapMs + 110, false);
    touch.Update(2000, &out);
    CHECK(out.keyTaps.empty() && out.buttons.empty(), "two fingers held long: no tap");
}

static void test_three_finger_tap()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.3f, 0.5f, 1000);
    touch.FingerDown(2, 0.5f, 0.5f, 1010);
    touch.FingerDown(3, 0.7f, 0.5f, 1020);
    touch.FingerUp(1, 0.3f, 0.5f, 1100, false);
    touch.FingerUp(2, 0.5f, 0.5f, 1105, false);
    touch.FingerUp(3, 0.7f, 0.5f, 1110, false);
    touch.Update(1120, &out);
    CHECK(out.toggleKeyboard && out.keyTaps.empty() && out.buttons.empty(), "three-finger tap: the keyboard");
}

static void test_two_finger_drag_scrolls()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.4f, 0.5f, 1000);
    touch.FingerDown(2, 0.6f, 0.5f, 1010);
    touch.Update(1016, &out);
    /* Both fingers right by a fifth of the height. */
    for (int step = 1; step <= 10; ++step) {
        float d = 0.02f * (float)step;
        touch.FingerMove(1, 0.4f + d, 0.5f, 1016 + step * 5);
        touch.FingerMove(2, 0.6f + d, 0.5f, 1016 + step * 5);
    }
    touch.Update(1080, &out);
    int parked = out.cursor == LRR_TouchOutput::kCursorEdge ? 1 : 0;
    CHECK(out.buttons.empty(), "two-finger drag: no click");
    CHECK(out.cursor == LRR_TouchOutput::kCursorEdge && out.x < -0.99f,
          "two-finger drag right: pointer on the left edge (the map follows the fingers)");
    CHECK(out.keysHeld.empty(), "two-finger drag: no zoom or turn");

    /* It stays parked while the drag is owed, then goes back inside. */
    touch.FingerUp(1, 0.6f, 0.5f, 1090, false);
    touch.FingerUp(2, 0.8f, 0.5f, 1090, false);
    uint64_t ms = 1080;
    bool inside = false;
    for (int poll = 0; poll < 100 && !inside; ++poll) {
        ms += 16;
        touch.Update(ms, &out);
        if (out.cursor == LRR_TouchOutput::kCursorEdge) ++parked;
        if (out.cursor == LRR_TouchOutput::kCursorInside) inside = true;
    }
    /* A fifth of the height owes about 140 ms: the first poll (64 ms since
     * the last) and about 5 more of 16 ms. */
    CHECK(parked >= 5 && parked <= 8, "two-finger drag: parked in proportion to the drag");
    CHECK(inside, "two-finger drag: then the pointer goes back inside");
    touch.Update(ms + 16, &out);
    CHECK(out.cursor == LRR_TouchOutput::kCursorKeep, "two-finger drag: and stays");
}

static void test_pinch_zooms()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.4f, 0.5f, 1000);
    touch.FingerDown(2, 0.6f, 0.5f, 1010);
    touch.Update(1016, &out);
    /* Spread to twice the distance, about the same centre. */
    for (int step = 1; step <= 10; ++step) {
        float d = 0.01f * (float)step;
        touch.FingerMove(1, 0.4f - d, 0.5f, 1016 + step * 5);
        touch.FingerMove(2, 0.6f + d, 0.5f, 1016 + step * 5);
    }
    touch.Update(1080, &out);
    CHECK(Holds(out, SDL_SCANCODE_EQUALS), "spread: = (zoom in) held");
    CHECK(out.cursor != LRR_TouchOutput::kCursorEdge, "spread: no scroll");
    CHECK(!Holds(out, SDL_SCANCODE_LEFT) && !Holds(out, SDL_SCANCODE_RIGHT), "spread: no turn");

    LRR_TouchGestures pinch;
    pinch.FingerDown(1, 0.3f, 0.5f, 1000);
    pinch.FingerDown(2, 0.7f, 0.5f, 1010);
    for (int step = 1; step <= 10; ++step) {
        float d = 0.01f * (float)step;
        pinch.FingerMove(1, 0.3f + d, 0.5f, 1016 + step * 5);
        pinch.FingerMove(2, 0.7f - d, 0.5f, 1016 + step * 5);
    }
    pinch.Update(1080, &out);
    CHECK(Holds(out, SDL_SCANCODE_MINUS), "pinch: - (zoom out) held");

    LRR_TouchSettings settings;
    settings.zoom = false;
    LRR_TouchGestures off(settings);
    off.FingerDown(1, 0.3f, 0.5f, 1000);
    off.FingerDown(2, 0.7f, 0.5f, 1010);
    for (int step = 1; step <= 10; ++step) {
        float d = 0.01f * (float)step;
        off.FingerMove(1, 0.3f + d, 0.5f, 1016 + step * 5);
        off.FingerMove(2, 0.7f - d, 0.5f, 1016 + step * 5);
    }
    off.Update(1080, &out);
    CHECK(out.keysHeld.empty(), "zoom off: no key");
    off.FingerUp(1, 0.4f, 0.5f, 1090, false);
    off.FingerUp(2, 0.6f, 0.5f, 1095, false);
    off.Update(1100, &out);
    CHECK(out.keyTaps.empty(), "zoom off: a pinch is still not a tap");
}

static void test_twist_turns()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.4f, 0.5f, 1000);
    touch.FingerDown(2, 0.6f, 0.5f, 1010);
    touch.Update(1016, &out);
    /* Turn the pair clockwise on the screen (y down) by 0.5 radians. */
    for (int step = 1; step <= 10; ++step) {
        float a = 0.05f * (float)step;
        float c = 0.1f * cosf(a);
        float s = 0.1f * sinf(a);
        touch.FingerMove(1, 0.5f - c, 0.5f - s, 1016 + step * 5);
        touch.FingerMove(2, 0.5f + c, 0.5f + s, 1016 + step * 5);
    }
    touch.Update(1080, &out);
    CHECK(Holds(out, SDL_SCANCODE_RIGHT), "twist clockwise: the right arrow held");
    CHECK(!Holds(out, SDL_SCANCODE_EQUALS) && !Holds(out, SDL_SCANCODE_MINUS), "twist: no zoom");
}

static void test_drag_ignores_second_finger()
{
    LRR_TouchGestures touch;
    LRR_TouchOutput out;
    touch.FingerDown(1, 0.5f, 0.5f, 1000);
    touch.FingerMove(1, 0.6f, 0.5f, 1010);
    touch.Update(1016, &out);
    touch.FingerDown(2, 0.2f, 0.2f, 1020);
    touch.FingerUp(2, 0.2f, 0.2f, 1040, false);
    touch.Update(1050, &out);
    CHECK(out.buttons.empty() && out.keyTaps.empty(), "drag: a second finger does nothing");
    touch.FingerUp(1, 0.6f, 0.5f, 1060, false);
    touch.Update(1070, &out);
    CHECK(out.buttons.size() == 1 && Is(out.buttons, 0, 1, false), "drag: still ends with left up");
}

int main(void)
{
    test_latch_short_click();
    test_latch_double_click();
    test_latch_two_buttons();
    test_latch_short_key();
    test_latch_typed_word();
    test_latch_shifted_key();
    test_latch_held_key();
    test_tap();
    test_touch_moves_pointer_without_pressing();
    test_drag();
    test_long_press_hover();
    test_long_press_off();
    test_canceled_tap();
    test_two_finger_tap();
    test_slow_two_finger_touch_is_not_a_tap();
    test_three_finger_tap();
    test_two_finger_drag_scrolls();
    test_pinch_zooms();
    test_twist_turns();
    test_drag_ignores_second_finger();
    return lrr_test_result("touch_input");
}
