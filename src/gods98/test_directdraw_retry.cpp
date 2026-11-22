// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_directdraw_retry — a failed directdraw_setup leaves nothing behind
 * (issue #966). Init.c's [library:config] block runs directdraw_setup
 * again on the next device when the chosen one cannot draw (SDL GPU, listed
 * but unable to claim the window), so each failed attempt must release what
 * it created: the surfaces, the clippers (DirectDraw.c) and the Direct3DRM
 * object (Main.c's direct3drm_device_initialize), and clear their pointers.
 *
 * The test makes the device creation fail with a device GUID miniwin does
 * not know (Direct3DRMImpl::CreateDeviceFromSurface returns
 * E_NOINTERFACE), on SDL's dummy video driver, and counts the live objects
 * miniwin allocates with operator new before and after repeated failed
 * attempts: a leak shows as a count that grows with each attempt. A
 * successful setup on the software device follows. Synthetic only: no game
 * data. */
#include <miniwin/ddraw.h>
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <new>

#include "gods98/directdraw.h"
#include "gods98/main_globs.h"

extern "C" int main_init_app(HINSTANCE hInstance);

static int s_failures = 0;

static void CHECK(bool cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* The blocks allocated with operator new that are still live: miniwin's
 * surfaces, clippers, DirectDraw and Direct3DRM objects are all made that
 * way. */
static std::atomic<long> s_liveBlocks{0};

void* operator new(std::size_t size)
{
    void* p = malloc(size ? size : 1);
    if (!p) throw std::bad_alloc();
    ++s_liveBlocks;
    return p;
}

void* operator new[](std::size_t size)
{
    return operator new(size);
}

void operator delete(void* p) noexcept
{
    if (p) {
        --s_liveBlocks;
        free(p);
    }
}

void operator delete[](void* p) noexcept
{
    operator delete(p);
}

void operator delete(void* p, std::size_t) noexcept
{
    operator delete(p);
}

void operator delete[](void* p, std::size_t) noexcept
{
    operator delete(p);
}

static const unsigned int kWidth = 640;
static const unsigned int kHeight = 480;

/* One windowed attempt, as Init.c makes it. */
static int Attempt(const DirectDraw_Device* device)
{
    return directdraw_setup(FALSE, NULL, device, NULL, 40, 40, kWidth, kHeight);
}

/* One full-screen attempt, which makes the back clipper only. */
static int AttemptFullScreen(const DirectDraw_Driver* driver, const DirectDraw_Device* device)
{
    return directdraw_setup(TRUE, driver, device, NULL, 0, 0, 320, 200);
}

static void CheckNothingKept(const char* when)
{
    char what[128];
    snprintf(what, sizeof(what), "%s: the DirectDraw object is released and cleared", when);
    CHECK(directDrawGlobs.lpDirectDraw == NULL, what);
    snprintf(what, sizeof(what), "%s: the front surface is released and cleared", when);
    CHECK(directDrawGlobs.fSurf == NULL, what);
    snprintf(what, sizeof(what), "%s: the back surface is released and cleared", when);
    CHECK(directDrawGlobs.bSurf == NULL, what);
    snprintf(what, sizeof(what), "%s: the front clipper is released and cleared", when);
    CHECK(directDrawGlobs.lpFrontClipper == NULL, what);
    snprintf(what, sizeof(what), "%s: the back clipper is released and cleared", when);
    CHECK(directDrawGlobs.lpBackClipper == NULL, what);
    snprintf(what, sizeof(what), "%s: the Direct3DRM object is released and cleared", when);
    CHECK(mainGlobs.lpD3DRM == NULL, what);
    snprintf(what, sizeof(what), "%s: no Direct3DRM device was made", when);
    CHECK(mainGlobs.device == NULL, what);
}

int main(void)
{
    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL);
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    snprintf(mainGlobs.programName, sizeof(mainGlobs.programName), "%s", "directdraw_retry_test");
    mainGlobs.className = mainGlobs.programName;
    if (!main_init_app(NULL) || mainGlobs.hWnd == NULL) {
        fprintf(stderr, "main_init_app failed: %s\n", SDL_GetError());
        return 1;
    }
    directdraw_initialise(mainGlobs.hWnd);

    DirectDraw_Driver drivers[16];
    DirectDraw_Device devices[16];
    unsigned int driverCount = 0;
    unsigned int deviceCount = 0;
    memset(drivers, 0, sizeof(drivers));
    memset(devices, 0, sizeof(devices));
    CHECK(directdraw_enum_drivers(drivers, &driverCount) && driverCount > 0, "miniwin lists a driver");
    CHECK(directdraw_enum_devices(&drivers[0], devices, &deviceCount) && deviceCount > 0, "miniwin lists devices");

    const DirectDraw_Device* software = NULL;
    for (unsigned int i = 0; i < deviceCount; ++i) {
        if (strstr(devices[i].desc, "Miniwin Emulation")) software = &devices[i];
    }
    CHECK(software != NULL, "miniwin lists the software device");
    if (s_failures) return 1;

    /* A device miniwin has no renderer for: CreateDeviceFromSurface fails,
     * after directdraw_setup has made its surfaces and clippers and
     * direct3drm_device_initialize its Direct3DRM object. */
    DirectDraw_Device bad = *software;
    memset(&bad.guid, 0xAB, sizeof(bad.guid));
    snprintf(bad.desc, sizeof(bad.desc), "Miniwin driver (No such device)");

    /* The first attempt warms what miniwin makes once (its window state,
     * enumeration caches); each further failure must leave the count where
     * it was. */
    CHECK(Attempt(&bad) == FALSE, "an unknown device fails to set up");
    CheckNothingKept("first failure");
    const long afterFirst = s_liveBlocks;

    for (int i = 0; i < 3; ++i) {
        CHECK(Attempt(&bad) == FALSE, "the retry on an unknown device fails again");
        CheckNothingKept("repeated failure");
    }
    const long afterRepeats = s_liveBlocks;
    if (afterRepeats != afterFirst) {
        printf("live blocks: %ld after the first failed attempt, %ld after three more\n", afterFirst, afterRepeats);
    }
    CHECK(afterRepeats == afterFirst, "failed attempts leave no objects behind");

    /* The same in full screen. */
    CHECK(AttemptFullScreen(&drivers[0], &bad) == FALSE, "an unknown device fails to set up in full screen");
    CheckNothingKept("full-screen failure");
    const long fullFirst = s_liveBlocks;
    for (int i = 0; i < 3; ++i) {
        CHECK(AttemptFullScreen(&drivers[0], &bad) == FALSE, "the full-screen retry fails again");
        CheckNothingKept("repeated full-screen failure");
    }
    CHECK(s_liveBlocks == fullFirst, "failed full-screen attempts leave no objects behind");

    /* Then the software device, as the fallback chooses it. */
    CHECK(Attempt(software) == TRUE, "the software device sets up after the failures");
    CHECK(mainGlobs.lpD3DRM != NULL, "a successful setup keeps the Direct3DRM object");
    CHECK(mainGlobs.device != NULL, "a successful setup keeps the Direct3DRM device");
    CHECK(directDrawGlobs.lpFrontClipper != NULL && directDrawGlobs.lpBackClipper != NULL,
          "a successful setup keeps both clippers");

    SDL_Quit();
    if (s_failures) {
        printf("%d check(s) failed\n", s_failures);
        return 1;
    }
    return 0;
}
