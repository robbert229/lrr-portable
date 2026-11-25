// SPDX-License-Identifier: LGPL-3.0-or-later
/* Touch gestures (issue #986): finger events turned into the mouse and keys
 * Rock Raiders understands, for touch screens (Android and iOS, and a
 * desktop touch screen when it is switched on). Shared by every host;
 * platform/native/input.cpp feeds it SDL's finger events once per poll.
 * isle-portable (ISLE/isleapp.cpp, LGPL-3.0) likewise turns SDL's touch-to-
 * mouse mapping off and makes the mouse from finger events itself, and
 * sends Space for a double tap; the gestures here are chosen for this game.
 *
 * What the game does with the mouse and keys decides the gestures:
 * - The left button selects, presses the interface's buttons, and drags.
 * - The game shows a tooltip when the pointer rests on something with no
 *   button held. A touch screen has no pointer that rests without
 *   pressing, so holding a finger still (a long press) moves the pointer
 *   without pressing: the tooltip shows, and lifting the finger clicks
 *   nothing.
 * - The right button is the left button: without -dualmouse, Main.c's
 *   window procedure sets both buttons' words for either button. So no
 *   gesture is spent on it unless the player asks for one (two-finger tap).
 * - The map scrolls only while the pointer is within MouseScrollBorder
 *   (Lego.cfg, 5 pixels) of the picture's edge, towards the point under
 *   the pointer. A two-finger drag parks the pointer on the edge, on the
 *   side opposite the drag, for a time in proportion to the drag's length,
 *   so the map follows the fingers.
 * - In the top view, - and = zoom out and in, and the left and right arrows
 *   turn the view. A pinch holds - or = for a time in proportion to the
 *   pinch, and a two-finger twist holds an arrow.
 * - Space shows the units' status bubbles and closes the briefing; a
 *   two-finger tap sends it.
 * - A three-finger tap opens or closes the on-screen keyboard, for the
 *   game's other keys.
 *
 * Positions are in the window, scaled so that its height is 1 (x runs from
 * 0 to the window's aspect ratio), so distances do not depend on the
 * screen's shape. Times are in milliseconds. Plain C++ with no SDL calls,
 * so the tests drive it directly (test_touch_input.cpp). */
#ifndef LEGORRR_PLATFORM_NATIVE_TOUCH_GESTURES_H
#define LEGORRR_PLATFORM_NATIVE_TOUCH_GESTURES_H

#include <cstdint>
#include <vector>

#include <SDL3/SDL_scancode.h>

#include "platform/native/config.h"
#include "platform/native/input_latch.h"

/* Which gestures are on (the tweaks:touch keys, LRR_TouchConfig). */
struct LRR_TouchSettings {
    bool longPressHover = true;
    bool scroll = true;
    bool zoom = true;
    bool rotate = true;
    LRR_TouchTwoFingerTap twoFingerTap = LRR_TOUCH_TAP_SPACE;
    bool threeFingerKeyboard = true;
};

/* What one poll's gestures do. */
struct LRR_TouchOutput {
    enum Cursor {
        /* Leave the pointer where it is. */
        kCursorKeep,
        /* Move it to (x, y). */
        kCursorAt,
        /* Move it to (x, y), kept far enough inside the picture's edge that
         * the map does not scroll. */
        kCursorInside,
        /* Park it on the picture's edge, in the direction (x, y) from the
         * picture's centre, so the map scrolls that way. */
        kCursorEdge,
    };
    Cursor cursor = kCursorKeep;
    float x = 0.0f;
    float y = 0.0f;
    /* Mouse-button changes, in order, for the input latch. */
    std::vector<LRR_ButtonChange> buttons;
    /* Keys to press and release, for the input latch. */
    std::vector<SDL_Scancode> keyTaps;
    /* Keys held down for this poll. */
    std::vector<SDL_Scancode> keysHeld;
    /* Open the on-screen keyboard, or close it when it is open. */
    bool toggleKeyboard = false;
};

class LRR_TouchGestures {
public:
    /* A finger that moves this far from where it touched is a drag. */
    static constexpr float kSlop = 0.03f;
    /* A finger held still this long is a long press. */
    static constexpr uint64_t kLongPressMs = 500;
    /* A two- or three-finger touch lifted within this time, without moving,
     * is a tap. */
    static constexpr uint64_t kMultiTapMs = 400;
    /* Two fingers that move together this far are a two-finger drag. */
    static constexpr float kMultiSlop = 0.04f;
    /* Two fingers whose distance changes by this factor (its log) are a
     * pinch. */
    static constexpr float kPinchStart = 0.08f;
    /* Two fingers that turn by this angle (radians) are a twist. */
    static constexpr float kTwistStart = 0.15f;
    /* The time the pointer stays on the edge per screen height of drag. */
    static constexpr float kScrollMsPerHeight = 700.0f;
    /* The time - or = is held for a pinch that changes the fingers'
     * distance by a factor of e. */
    static constexpr float kZoomMsPerE = 1000.0f;
    /* The time an arrow is held per radian of twist: the game turns the
     * view 0.02 radians per 40 ms frame of a held arrow, so the view turns
     * as far as the fingers do. */
    static constexpr float kRotateMsPerRadian = 2000.0f;
    /* The longest poll interval counted towards the times above: the
     * game's own cap on a frame's time (3.0 units of 40 ms). */
    static constexpr uint64_t kMaxPollMs = 120;

    explicit LRR_TouchGestures(const LRR_TouchSettings& settings = LRR_TouchSettings());

    void SetSettings(const LRR_TouchSettings& settings) { m_settings = settings; }

    /* A finger's events, in arrival order. A cancelled touch (the system
     * took it) clicks nothing. */
    void FingerDown(uint64_t id, float x, float y, uint64_t ms);
    void FingerMove(uint64_t id, float x, float y, uint64_t ms);
    void FingerUp(uint64_t id, float x, float y, uint64_t ms, bool canceled);

    /* The game polls at `ms`: what the gestures since the last poll do. */
    void Update(uint64_t ms, LRR_TouchOutput* out);

    /* Drops every finger and what they owed, as on losing focus. */
    void Reset();

private:
    enum State {
        kIdle,
        /* One finger, not yet a tap, a drag or a long press. */
        kOnePending,
        /* One finger dragging with the left button down. */
        kOneDrag,
        /* One finger moving the pointer after a long press. */
        kOneHover,
        /* Two or more fingers. */
        kMulti,
        /* Fingers left over from a gesture that has ended. */
        kDrain,
    };
    struct Finger {
        uint64_t id;
        float x;
        float y;
        float startX;
        float startY;
    };

    int FindFinger(uint64_t id) const;
    void LongPress();
    void StartMulti();
    void TrackMulti();
    void EndMulti(uint64_t ms, bool canceled);
    void SetCursor(LRR_TouchOutput::Cursor cursor, float x, float y);

    LRR_TouchSettings m_settings;
    std::vector<Finger> m_fingers;
    State m_state = kIdle;
    uint64_t m_downMs = 0;
    size_t m_maxFingers = 0;
    bool m_canceled = false;

    /* Two-finger tracking: the last centroid, distance and angle, what has
     * accumulated since the gesture began, and which parts have begun. */
    bool m_trackValid = false;
    float m_lastCx = 0.0f;
    float m_lastCy = 0.0f;
    float m_lastDist = 0.0f;
    float m_lastAngle = 0.0f;
    float m_moveX = 0.0f;
    float m_moveY = 0.0f;
    float m_logRatio = 0.0f;
    float m_angle = 0.0f;
    bool m_moved = false;
    bool m_scrolling = false;
    bool m_zooming = false;
    bool m_rotating = false;

    /* What the two-finger gestures still owe the game, in milliseconds of
     * a parked pointer or a held key, and where the pointer goes after. */
    float m_scrollX = 0.0f;
    float m_scrollY = 0.0f;
    float m_zoomMs = 0.0f;
    float m_rotateMs = 0.0f;
    bool m_parked = false;
    float m_restX = 0.0f;
    float m_restY = 0.0f;

    bool m_haveUpdate = false;
    uint64_t m_lastUpdateMs = 0;
    LRR_TouchOutput m_pending;
};

#endif /* LEGORRR_PLATFORM_NATIVE_TOUCH_GESTURES_H */
