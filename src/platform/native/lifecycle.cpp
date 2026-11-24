// SPDX-License-Identifier: LGPL-3.0-or-later
/* The host's application lifecycle (lifecycle.h). */
#include "platform/native/lifecycle.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

#include <atomic>

namespace {

enum State { kActive, kSuspended, kResumed };

std::atomic<int> g_state(kActive);

} // namespace

extern "C" {

void LRR_ReportLifecycle(LRR_LifecycleEvent event)
{
    switch (event) {
    case LRR_LIFECYCLE_SUSPEND:
        SDL_Log("lifecycle: suspend");
        g_state = kSuspended;
        break;
    case LRR_LIFECYCLE_RESUME:
        SDL_Log("lifecycle: resume");
        /* A resume with no suspend before it changes nothing. */
        if (g_state == kSuspended) g_state = kResumed;
        break;
    case LRR_LIFECYCLE_EXIT: {
        SDL_Log("lifecycle: exit requested");
        /* What the window's close button gives: miniwin's pump turns it
         * into WM_CLOSE for the game's window. */
        SDL_Event quit = {};
        quit.type = SDL_EVENT_QUIT;
        if (!SDL_PushEvent(&quit)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "lifecycle: cannot post the exit request");
        }
        break;
    }
    }
}

int LRR_ApplicationActive(void)
{
    int state = g_state;
    if (state == kResumed) {
        g_state = kActive;
        return 0;
    }
    return state == kActive;
}

} // extern "C"
