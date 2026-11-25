// SPDX-License-Identifier: LGPL-3.0-or-later
/* Touch gestures (touch_gestures.h). */
#include "platform/native/touch_gestures.h"

#include <cmath>

namespace {

const float kPi = 3.14159265358979f;

float Distance(float ax, float ay, float bx, float by)
{
    return std::hypot(bx - ax, by - ay);
}

/* An angle difference in (-pi, pi]. */
float WrapAngle(float angle)
{
    while (angle > kPi) {
        angle -= 2.0f * kPi;
    }
    while (angle <= -kPi) {
        angle += 2.0f * kPi;
    }
    return angle;
}

/* Spends up to `ms` of an owed time, towards zero. */
float Spend(float owed, float ms)
{
    if (owed > 0.0f) {
        return owed > ms ? owed - ms : 0.0f;
    }
    return owed < -ms ? owed + ms : 0.0f;
}

} // namespace

LRR_TouchGestures::LRR_TouchGestures(const LRR_TouchSettings& settings) : m_settings(settings)
{
}

int LRR_TouchGestures::FindFinger(uint64_t id) const
{
    for (size_t i = 0; i < m_fingers.size(); ++i) {
        if (m_fingers[i].id == id) {
            return (int)i;
        }
    }
    return -1;
}

void LRR_TouchGestures::SetCursor(LRR_TouchOutput::Cursor cursor, float x, float y)
{
    m_pending.cursor = cursor;
    m_pending.x = x;
    m_pending.y = y;
}

void LRR_TouchGestures::FingerDown(uint64_t id, float x, float y, uint64_t ms)
{
    if (FindFinger(id) >= 0) {
        return;
    }
    m_fingers.push_back(Finger{id, x, y, x, y});

    switch (m_state) {
    case kIdle:
        m_state = kOnePending;
        m_downMs = ms;
        m_maxFingers = 1;
        m_canceled = false;
        SetCursor(LRR_TouchOutput::kCursorAt, x, y);
        break;
    case kOnePending:
    case kOneHover:
        /* No button is down yet: the touch becomes a two-finger one. */
        StartMulti();
        break;
    case kMulti:
        if (m_fingers.size() > m_maxFingers) {
            m_maxFingers = m_fingers.size();
        }
        if (m_fingers.size() == 2) {
            m_trackValid = false;
            TrackMulti();
        }
        break;
    case kOneDrag:
    case kDrain:
        /* A finger added to a drag, or after a gesture, does nothing. */
        break;
    }
}

void LRR_TouchGestures::FingerMove(uint64_t id, float x, float y, uint64_t ms)
{
    (void)ms;
    int index = FindFinger(id);
    if (index < 0) {
        return;
    }
    Finger& finger = m_fingers[index];
    finger.x = x;
    finger.y = y;

    switch (m_state) {
    case kOnePending:
        SetCursor(LRR_TouchOutput::kCursorAt, x, y);
        if (Distance(finger.startX, finger.startY, x, y) > kSlop) {
            m_state = kOneDrag;
            m_pending.buttons.push_back(LRR_ButtonChange{1, true});
        }
        break;
    case kOneDrag:
    case kOneHover:
        if (index == 0) {
            SetCursor(LRR_TouchOutput::kCursorAt, x, y);
        }
        break;
    case kMulti:
        if (index < 2) {
            TrackMulti();
        }
        break;
    case kIdle:
    case kDrain:
        break;
    }
}

void LRR_TouchGestures::FingerUp(uint64_t id, float x, float y, uint64_t ms, bool canceled)
{
    int index = FindFinger(id);
    if (index < 0) {
        return;
    }
    m_fingers.erase(m_fingers.begin() + index);
    if (canceled) {
        m_canceled = true;
    }

    switch (m_state) {
    case kOnePending:
        /* The only finger: a tap, or a long press the last poll had not
         * seen yet. */
        if (!canceled) {
            SetCursor(LRR_TouchOutput::kCursorAt, x, y);
            if (ms - m_downMs < kLongPressMs || !m_settings.longPressHover) {
                m_pending.buttons.push_back(LRR_ButtonChange{1, true});
                m_pending.buttons.push_back(LRR_ButtonChange{1, false});
            }
        }
        m_state = kIdle;
        break;
    case kOneDrag:
        if (index == 0) {
            m_pending.buttons.push_back(LRR_ButtonChange{1, false});
            m_state = m_fingers.empty() ? kIdle : kDrain;
        }
        break;
    case kOneHover:
        if (index == 0) {
            m_state = m_fingers.empty() ? kIdle : kDrain;
        }
        break;
    case kMulti:
        if (m_fingers.empty()) {
            EndMulti(ms, m_canceled);
        }
        else {
            /* Track again from the fingers left, without a jump. */
            m_trackValid = false;
            TrackMulti();
        }
        break;
    case kDrain:
        if (m_fingers.empty()) {
            m_state = kIdle;
        }
        break;
    case kIdle:
        break;
    }
}

void LRR_TouchGestures::LongPress()
{
    if (m_settings.longPressHover) {
        m_state = kOneHover;
    }
    else {
        /* Holding a finger still holds the left button, as a mouse. */
        m_state = kOneDrag;
        m_pending.buttons.push_back(LRR_ButtonChange{1, true});
    }
}

void LRR_TouchGestures::StartMulti()
{
    m_state = kMulti;
    if (m_fingers.size() > m_maxFingers) {
        m_maxFingers = m_fingers.size();
    }
    m_trackValid = false;
    m_moveX = m_moveY = 0.0f;
    m_logRatio = 0.0f;
    m_angle = 0.0f;
    m_moved = false;
    m_scrolling = m_zooming = m_rotating = false;
    TrackMulti();
}

void LRR_TouchGestures::TrackMulti()
{
    if (m_fingers.size() < 2) {
        return;
    }
    const Finger& a = m_fingers[0];
    const Finger& b = m_fingers[1];
    float cx = (a.x + b.x) * 0.5f;
    float cy = (a.y + b.y) * 0.5f;
    float dist = Distance(a.x, a.y, b.x, b.y);
    float angle = std::atan2(b.y - a.y, b.x - a.x);
    m_restX = cx;
    m_restY = cy;

    if (!m_trackValid || dist < 1e-4f || m_lastDist < 1e-4f) {
        m_trackValid = dist >= 1e-4f;
        m_lastCx = cx;
        m_lastCy = cy;
        m_lastDist = dist;
        m_lastAngle = angle;
        return;
    }

    float dx = cx - m_lastCx;
    float dy = cy - m_lastCy;
    float dlog = std::log(dist / m_lastDist);
    float dangle = WrapAngle(angle - m_lastAngle);
    m_lastCx = cx;
    m_lastCy = cy;
    m_lastDist = dist;
    m_lastAngle = angle;

    m_moveX += dx;
    m_moveY += dy;
    m_logRatio += dlog;
    m_angle += dangle;

    /* Each part begins once it passes its own threshold, so a pinch does
     * not also turn the view, and stays on until the fingers lift. Any of
     * them makes the touch no longer a tap, whether or not it is on. */
    bool dragged = std::hypot(m_moveX, m_moveY) > kMultiSlop;
    bool pinched = std::fabs(m_logRatio) > kPinchStart;
    bool twisted = std::fabs(m_angle) > kTwistStart;
    if (dragged || pinched || twisted) {
        m_moved = true;
    }
    if (m_settings.scroll && dragged) {
        m_scrolling = true;
    }
    if (m_settings.zoom && pinched) {
        m_zooming = true;
    }
    if (m_settings.rotate && twisted) {
        m_rotating = true;
    }

    /* The map follows the fingers: the view moves the other way. */
    if (m_scrolling) {
        m_scrollX -= dx * kScrollMsPerHeight;
        m_scrollY -= dy * kScrollMsPerHeight;
    }
    if (m_zooming) {
        m_zoomMs += dlog * kZoomMsPerE;
    }
    if (m_rotating) {
        m_rotateMs += dangle * kRotateMsPerRadian;
    }
}

void LRR_TouchGestures::EndMulti(uint64_t ms, bool canceled)
{
    if (!m_moved && !canceled && ms - m_downMs <= kMultiTapMs) {
        if (m_maxFingers >= 3) {
            if (m_settings.threeFingerKeyboard) {
                m_pending.toggleKeyboard = true;
            }
        }
        else if (m_settings.twoFingerTap == LRR_TOUCH_TAP_SPACE) {
            m_pending.keyTaps.push_back(SDL_SCANCODE_SPACE);
        }
        else if (m_settings.twoFingerTap == LRR_TOUCH_TAP_RIGHT_CLICK) {
            SetCursor(LRR_TouchOutput::kCursorInside, m_restX, m_restY);
            m_pending.buttons.push_back(LRR_ButtonChange{3, true});
            m_pending.buttons.push_back(LRR_ButtonChange{3, false});
        }
    }
    m_state = kIdle;
}

void LRR_TouchGestures::Update(uint64_t ms, LRR_TouchOutput* out)
{
    float dt = 0.0f;
    if (m_haveUpdate && ms > m_lastUpdateMs) {
        uint64_t elapsed = ms - m_lastUpdateMs;
        dt = (float)(elapsed < kMaxPollMs ? elapsed : kMaxPollMs);
    }
    m_haveUpdate = true;
    m_lastUpdateMs = ms;

    if (m_state == kOnePending && ms - m_downMs >= kLongPressMs) {
        LongPress();
    }

    *out = m_pending;
    m_pending = LRR_TouchOutput();

    /* Scrolling: the pointer on the edge until the drag is paid for, then
     * back inside, where the fingers were. */
    float owed = std::hypot(m_scrollX, m_scrollY);
    if (owed >= 1.0f) {
        out->cursor = LRR_TouchOutput::kCursorEdge;
        out->x = m_scrollX / owed;
        out->y = m_scrollY / owed;
        m_parked = true;
        float left = owed > dt ? owed - dt : 0.0f;
        m_scrollX *= left / owed;
        m_scrollY *= left / owed;
    }
    else {
        m_scrollX = m_scrollY = 0.0f;
        if (m_parked) {
            out->cursor = LRR_TouchOutput::kCursorInside;
            out->x = m_restX;
            out->y = m_restY;
            m_parked = false;
        }
    }

    /* Zoom: = in for spread fingers, - out for pinched ones. */
    if (m_zoomMs >= 1.0f) {
        out->keysHeld.push_back(SDL_SCANCODE_EQUALS);
    }
    else if (m_zoomMs <= -1.0f) {
        out->keysHeld.push_back(SDL_SCANCODE_MINUS);
    }
    m_zoomMs = Spend(m_zoomMs, dt);

    /* Turning: the angle grows as the fingers turn clockwise on the screen
     * (y runs down). The right arrow turns the view anticlockwise, so the
     * map turns clockwise with the fingers. */
    if (m_rotateMs >= 1.0f) {
        out->keysHeld.push_back(SDL_SCANCODE_RIGHT);
    }
    else if (m_rotateMs <= -1.0f) {
        out->keysHeld.push_back(SDL_SCANCODE_LEFT);
    }
    m_rotateMs = Spend(m_rotateMs, dt);
}

void LRR_TouchGestures::Reset()
{
    m_fingers.clear();
    m_state = kIdle;
    m_trackValid = false;
    m_scrollX = m_scrollY = 0.0f;
    m_zoomMs = 0.0f;
    m_rotateMs = 0.0f;
    m_parked = false;
    m_pending = LRR_TouchOutput();
}
