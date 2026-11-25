// SPDX-License-Identifier: LGPL-3.0-or-later
/* The input latch (input_latch.h). */
#include "platform/native/input_latch.h"

#include <cstring>

LRR_InputLatch::LRR_InputLatch()
{
    memset(m_held, 0, sizeof(m_held));
}

void LRR_InputLatch::Key(int scancode, bool down)
{
    if (scancode < 0 || scancode >= kKeyCount || m_keys.size() >= kQueueLimit) {
        return;
    }
    m_keys.push_back(KeyChange{scancode, down});
}

void LRR_InputLatch::Button(int button, bool down)
{
    if (m_buttons.size() >= kQueueLimit) {
        return;
    }
    m_buttons.push_back(LRR_ButtonChange{button, down});
}

void LRR_InputLatch::Poll(std::vector<LRR_ButtonChange>* buttons)
{
    /* Keys: stop at a second press, at the release of a key pressed in
     * this poll, or at the press of a key released in it (a doubled
     * letter would otherwise stay down and type once). */
    bool changedThisPoll[kKeyCount];
    bool anyPress = false;
    memset(changedThisPoll, 0, sizeof(changedThisPoll));
    while (!m_keys.empty()) {
        const KeyChange change = m_keys.front();
        if (changedThisPoll[change.scancode]) {
            break;
        }
        if (change.down) {
            if (anyPress) {
                break;
            }
            /* A press of a key already down (a repeat) changes nothing. */
            if (!m_held[change.scancode]) {
                m_held[change.scancode] = true;
                changedThisPoll[change.scancode] = true;
                anyPress = true;
            }
        }
        else if (m_held[change.scancode]) {
            m_held[change.scancode] = false;
            changedThisPoll[change.scancode] = true;
        }
        m_keys.pop_front();
    }

    /* Buttons: stop at a button's second change in this poll, so a press
     * and its release (or a release and the next press) reach the game in
     * different frames. */
    unsigned int buttonsThisPoll = 0; /* a bit per button, 1 to 31 */
    while (!m_buttons.empty()) {
        const LRR_ButtonChange change = m_buttons.front();
        unsigned int bit = change.button > 0 && change.button < 32 ? 1u << change.button : 0;
        if (buttonsThisPoll & bit) {
            break;
        }
        buttonsThisPoll |= bit;
        if (buttons != NULL) {
            buttons->push_back(change);
        }
        m_buttons.pop_front();
    }
}

bool LRR_InputLatch::KeyDown(int scancode) const
{
    return scancode >= 0 && scancode < kKeyCount && m_held[scancode];
}

void LRR_InputLatch::Reset()
{
    m_keys.clear();
    m_buttons.clear();
    memset(m_held, 0, sizeof(m_held));
}
