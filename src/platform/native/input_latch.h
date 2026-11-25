// SPDX-License-Identifier: LGPL-3.0-or-later
/* The input latch (issue #986): key and mouse-button changes held until the
 * game has polled them.
 *
 * The game never reads input events. Once a frame, Input.c copies SDL's
 * keyboard state into its key map (input_state_poll_retail), and Main.c's
 * window procedure keeps the mouse buttons' up or down state from the
 * frame's WM_*BUTTON* messages; the game then compares this frame's state
 * with the last. A press and its release that both arrive between two
 * polls leave the state as it was, and the game never sees the press. A
 * tap on a touch screen often lasts less than a frame, and an on-screen
 * keyboard sends each character's press and release together, so both
 * would be lost.
 *
 * The latch queues the changes in arrival order and applies them one poll
 * at a time:
 * - a key or button changes at most once per poll, so every press is seen
 *   down for at least one poll, and a release and the next press of the
 *   same key (a doubled letter, a double click) are seen apart;
 * - at most one key press is applied per poll, so the characters of a word
 *   the on-screen keyboard sends at once reach the game one per frame, in
 *   order (the game types the keys held in a frame in key-code order).
 *
 * Plain C++ with no SDL calls, so the tests drive it directly
 * (test_touch_input.cpp); platform/native/input.cpp feeds it SDL's events. */
#ifndef LEGORRR_PLATFORM_NATIVE_INPUT_LATCH_H
#define LEGORRR_PLATFORM_NATIVE_INPUT_LATCH_H

#include <cstddef>
#include <deque>
#include <vector>

/* A mouse button's press or release, with SDL's button numbers
 * (1 left, 3 right). */
struct LRR_ButtonChange {
    int button;
    bool down;
};

class LRR_InputLatch {
public:
    /* The key codes it tracks: SDL scancodes, 0 to kKeyCount - 1. */
    static const int kKeyCount = 512;
    /* The changes it queues at most of each kind; more are dropped. */
    static const size_t kQueueLimit = 256;

    LRR_InputLatch();

    /* A key's press or release, in arrival order. A key code out of range
     * is ignored. */
    void Key(int scancode, bool down);
    /* A mouse button's press or release, in arrival order. */
    void Button(int button, bool down);

    /* The game polls: applies the queued changes this poll may take and
     * appends the button changes to deliver now, in order, to `buttons`. */
    void Poll(std::vector<LRR_ButtonChange>* buttons);

    /* Whether `scancode` is down as of the last Poll. */
    bool KeyDown(int scancode) const;

    /* Drops the queues and releases every key, as on losing focus. */
    void Reset();

private:
    struct KeyChange {
        int scancode;
        bool down;
    };
    std::deque<KeyChange> m_keys;
    std::deque<LRR_ButtonChange> m_buttons;
    bool m_held[kKeyCount];
};

#endif /* LEGORRR_PLATFORM_NATIVE_INPUT_LATCH_H */
