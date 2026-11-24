// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_lifecycle — the host's application lifecycle (platform/native/
 * lifecycle.h): what the game's main loop reads as the application's
 * activity after suspend and resume events, and the exit request. The 3DS's
 * APT hooks report these events (3ds/apthooks.cpp); the 3DS itself is not
 * needed here. */
#include <stdio.h>

#include <SDL3/SDL.h>

#include "platform/native/lifecycle.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* Whether SDL has a quit request queued, and takes it. */
static bool TakeQuit()
{
    SDL_PumpEvents();
    SDL_Event event;
    return SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_QUIT, SDL_EVENT_QUIT) == 1;
}

int main()
{
    CHECK(SDL_Init(SDL_INIT_EVENTS), "SDL events initialise");

    /* No event: the desktop, always active. */
    CHECK(LRR_ApplicationActive() == 1, "active without any event");
    CHECK(LRR_ApplicationActive() == 1, "still active on the next frame");

    /* A suspend: inactive for as long as it lasts. */
    LRR_ReportLifecycle(LRR_LIFECYCLE_SUSPEND);
    CHECK(LRR_ApplicationActive() == 0, "inactive after a suspend");
    CHECK(LRR_ApplicationActive() == 0, "inactive for the whole suspend");

    /* The resume: one more inactive frame, so the game sees the focus come
     * back before it takes it, then active. */
    LRR_ReportLifecycle(LRR_LIFECYCLE_RESUME);
    CHECK(LRR_ApplicationActive() == 0, "the first frame after a resume is inactive");
    CHECK(LRR_ApplicationActive() == 1, "active the frame after that");
    CHECK(LRR_ApplicationActive() == 1, "and from then on");

    /* A resume nobody suspended, as the 3DS reports when it wakes with
     * nothing to wake from: no change. */
    LRR_ReportLifecycle(LRR_LIFECYCLE_RESUME);
    CHECK(LRR_ApplicationActive() == 1, "a resume without a suspend changes nothing");

    /* Sleep and the HOME menu together, then both ends: the 3DS reports
     * sleep and wake, and suspend and restore. A second suspend is no
     * different from the first, and one resume ends it. */
    LRR_ReportLifecycle(LRR_LIFECYCLE_SUSPEND);
    LRR_ReportLifecycle(LRR_LIFECYCLE_SUSPEND);
    CHECK(LRR_ApplicationActive() == 0, "inactive after two suspends");
    LRR_ReportLifecycle(LRR_LIFECYCLE_RESUME);
    LRR_ReportLifecycle(LRR_LIFECYCLE_RESUME);
    CHECK(LRR_ApplicationActive() == 0, "inactive on the first frame after two resumes");
    CHECK(LRR_ApplicationActive() == 1, "active after that");

    /* A suspend that arrives before the game has looked at the resume
     * keeps the application inactive. */
    LRR_ReportLifecycle(LRR_LIFECYCLE_SUSPEND);
    LRR_ReportLifecycle(LRR_LIFECYCLE_RESUME);
    LRR_ReportLifecycle(LRR_LIFECYCLE_SUSPEND);
    CHECK(LRR_ApplicationActive() == 0, "a second suspend before the resume is seen");
    CHECK(LRR_ApplicationActive() == 0, "keeps it inactive");
    LRR_ReportLifecycle(LRR_LIFECYCLE_RESUME);
    CHECK(LRR_ApplicationActive() == 0, "until the next resume's inactive frame");
    CHECK(LRR_ApplicationActive() == 1, "and then active");

    /* Exit: a quit request, the window's close button. Suspend and resume
     * post none. */
    CHECK(!TakeQuit(), "no quit request so far");
    LRR_ReportLifecycle(LRR_LIFECYCLE_EXIT);
    CHECK(TakeQuit(), "exit posts SDL_EVENT_QUIT");
    CHECK(!TakeQuit(), "exit posts it once");
    CHECK(LRR_ApplicationActive() == 1, "exit leaves the activity alone");

    SDL_Quit();
    if (s_failures == 0) printf("PASS: lifecycle\n");
    return s_failures != 0;
}
