// SPDX-License-Identifier: LGPL-3.0-or-later
// The Direct3D device the miniwin rendering tests draw with. By default it
// is the software renderer on SDL's dummy video driver, which CI runs. Set
// MINIWIN_TEST_DEVICE to another device's name (such as "SDL3 GPU HAL") and
// MINIWIN_TEST_VIDEO_DRIVER to a driver it can draw on (such as "offscreen")
// to run the same checks on another backend; a test whose device miniwin
// does not offer exits with kMiniwinTestSkip.
#pragma once

#include <miniwin/d3d.h>

#include <SDL3/SDL.h>
#include <stdio.h>

static const int kMiniwinTestSkip = 77;

inline const char* MiniwinTestDevice()
{
    const char* device = SDL_getenv("MINIWIN_TEST_DEVICE");
    return device && *device ? device : "Miniwin Emulation";
}

inline const char* MiniwinTestVideoDriver()
{
    const char* driver = SDL_getenv("MINIWIN_TEST_VIDEO_DRIVER");
    return driver && *driver ? driver : "dummy";
}

// Set when enumeration did not offer MiniwinTestDevice().
inline bool g_miniwinTestDeviceMissing = false;

// Whether the enumerated guid is still zero, that is, the device is missing.
inline bool MiniwinTestDeviceMissing(const GUID& guid)
{
    static const GUID none = {};
    g_miniwinTestDeviceMissing = SDL_memcmp(&guid, &none, sizeof(GUID)) == 0;
    if (g_miniwinTestDeviceMissing) {
        fprintf(stderr, "device \"%s\" not offered: skipped\n", MiniwinTestDevice());
    }
    return g_miniwinTestDeviceMissing;
}

// Whether retained-mode meshes have ADR 0008's additions (decal
// transparency, specular scaled by a light's reach). Only the software
// backend has them; other backends draw immediate mode only.
inline bool MiniwinTestRetainedAdditions(const char* check)
{
    if (SDL_strcmp(MiniwinTestDevice(), "Miniwin Emulation") == 0) {
        return true;
    }
    fprintf(stderr, "%s: retained mode, software backend only: skipped\n", check);
    return false;
}
