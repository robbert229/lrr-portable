// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_device_choice — the 3D device the [library:config] substitution in
 * Init.c chooses (LRR_ChooseDevice, platform/native/config.h), from
 * synthetic device lists shaped like the ones miniwin builds on each
 * platform, with each platform's default device. Only the software and SDL
 * GPU devices draw the level (miniwin's Miniwin_DeviceDrawsImmediateMode). */
#include <stdio.h>
#include <string.h>

#include <initializer_list>
#include <vector>

#include "platform/native/android/config.h"
#include "platform/native/config.h"
#include "platform/native/emscripten/config.h"
#include "platform/native/ios/config.h"
#include "platform/native/switch/config.h"

static int s_failures = 0;

static void CHECK(bool cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* The devices miniwin lists, by DirectDraw.c's descriptions. */
static const char* const kSdlGpu = LRR_DEVICE_SDLGPU;
static const char* const kGles3 = "Miniwin driver (OpenGL ES 3.0 HAL)";
static const char* const kGles2 = "Miniwin driver (OpenGL ES 2.0 HAL)";
static const char* const kGl1 = "Miniwin driver (OpenGL 1.1 HAL)";
static const char* const kDirectX9 = "Miniwin driver (DirectX 9 HAL)";
static const char* const kGxm = "Miniwin driver (GXM HAL)";
static const char* const kPaletteSw = "Miniwin driver (Miniwin Paletted Software)";
static const char* const kSoftware = LRR_DEVICE_SOFTWARE;

static std::vector<LRR_DeviceCandidate> List(std::initializer_list<const char*> descs)
{
    std::vector<LRR_DeviceCandidate> list;
    for (const char* desc : descs) {
        const bool draws = desc == kSdlGpu || desc == kSoftware;
        list.push_back({desc, draws, false});
    }
    return list;
}

/* The description of the chosen device, or "" for none. */
static const char* Choose(const std::vector<LRR_DeviceCandidate>& list, const char* configured,
                          const char* platformDefault)
{
    const int index = LRR_ChooseDevice(list.data(), (unsigned int)list.size(), configured, platformDefault);
    return index >= 0 ? list[index].desc : "";
}

static bool Is(const char* chosen, const char* expected)
{
    return strcmp(chosen, expected) == 0;
}

int main(void)
{
    CHECK(Is(LRR_Default3DDevice(), kSdlGpu), "desktop: SDL GPU by default");

    /* The desktop: miniwin lists SDL GPU first when it can create a device
     * (a Vulkan, Metal or Direct3D 12 driver), and the software renderer
     * last; the OpenGL devices between them cannot draw the level. */
    const auto desktop = List({kSdlGpu, kGles3, kGles2, kGl1, kPaletteSw, kSoftware});
    CHECK(Is(Choose(desktop, "", kSdlGpu), kSdlGpu), "desktop: SDL GPU when it is available");
    CHECK(Is(Choose(desktop, kSoftware, kSdlGpu), kSoftware), "desktop: an explicit software device wins");
    CHECK(Is(Choose(desktop, kSdlGpu, kSdlGpu), kSdlGpu), "desktop: an explicit SDL GPU device");
    CHECK(Is(Choose(desktop, kGles3, kSdlGpu), kSdlGpu),
          "desktop: an explicit device that cannot draw the level falls back to the default");
    CHECK(Is(Choose(desktop, "Miniwin driver (Typo)", kSdlGpu), kSdlGpu),
          "desktop: an explicit device that is not listed falls back to the default");

    /* No Vulkan, Metal or Direct3D 12 driver: SDL GPU is not listed. */
    const auto noGpu = List({kGles3, kGles2, kGl1, kPaletteSw, kSoftware});
    CHECK(Is(Choose(noGpu, "", kSdlGpu), kSoftware), "desktop without SDL GPU: the software renderer");
    CHECK(Is(Choose(noGpu, kSdlGpu, kSdlGpu), kSoftware),
          "desktop without SDL GPU: an explicit SDL GPU device falls back to software");

    /* CI: SDL's dummy video driver lists only the software devices. */
    const auto dummy = List({kPaletteSw, kSoftware});
    CHECK(Is(Choose(dummy, "", kSdlGpu), kSoftware), "dummy video driver: the software renderer");

    /* Windows adds Direct3D 9, which cannot draw the level either. */
    const auto windows = List({kSdlGpu, kGl1, kDirectX9, kPaletteSw, kSoftware});
    CHECK(Is(Choose(windows, "", kSdlGpu), kSdlGpu), "Windows: SDL GPU");
    CHECK(Is(Choose(List({kGl1, kDirectX9, kPaletteSw, kSoftware}), "", kSdlGpu), kSoftware),
          "Windows without SDL GPU: the software renderer, not the last hardware device");

    /* SDL GPU listed, but the display cannot be set up on it (it cannot
     * claim the window): Init.c marks it failed and chooses again. */
    auto failed = desktop;
    failed[0].failed = true;
    CHECK(Is(Choose(failed, "", kSdlGpu), kSoftware), "SDL GPU failed: the software renderer");
    CHECK(Is(Choose(failed, kSdlGpu, kSdlGpu), kSoftware),
          "an explicit SDL GPU device that failed: the software renderer");
    failed[5].failed = true;
    CHECK(Is(Choose(failed, "", kSdlGpu), kGles3),
          "every drawing device failed: the first device left, without 3D");
    for (LRR_DeviceCandidate& device : failed) device.failed = true;
    CHECK(Choose(failed, "", kSdlGpu)[0] == '\0', "every device failed: none");
    CHECK(Choose({}, "", kSdlGpu)[0] == '\0', "no devices: none");

    /* Switch: miniwin builds no software renderer (nor OpenGL 1 or
     * OpenGL ES 2), so the last hardware device cannot draw the level. */
    CHECK(Is(NX_DEFAULT_3D_DEVICE, kSdlGpu), "Switch: SDL GPU by default");
    const auto nx = List({kSdlGpu, kGles3, kPaletteSw});
    CHECK(Is(Choose(nx, "", NX_DEFAULT_3D_DEVICE), kSdlGpu), "Switch: SDL GPU");
    CHECK(Is(Choose(nx, kGles3, NX_DEFAULT_3D_DEVICE), kSdlGpu),
          "Switch: an explicit OpenGL ES 3 device falls back to SDL GPU");
    const auto nxNoGpu = List({kGles3, kPaletteSw});
    CHECK(Is(Choose(nxNoGpu, "", NX_DEFAULT_3D_DEVICE), kGles3),
          "Switch without SDL GPU: no device draws; the first, without 3D");

    /* Android and iOS: SDL GPU (Vulkan, Metal); the software renderer is
     * listed but too slow. */
    CHECK(Is(ANDROID_DEFAULT_3D_DEVICE, kSdlGpu), "Android: SDL GPU by default");
    CHECK(Is(IOS_DEFAULT_3D_DEVICE, kSdlGpu), "iOS: SDL GPU by default");
    const auto android = List({kSdlGpu, kGles3, kGles2, kPaletteSw, kSoftware});
    CHECK(Is(Choose(android, "", ANDROID_DEFAULT_3D_DEVICE), kSdlGpu), "Android: SDL GPU");
    CHECK(Is(Choose(android, "", IOS_DEFAULT_3D_DEVICE), kSdlGpu), "iOS: SDL GPU");
    CHECK(Is(Choose(android, kSoftware, ANDROID_DEFAULT_3D_DEVICE), kSoftware),
          "Android: an explicit software device wins");

    /* The web: the software renderer, even if SDL GPU were listed. */
    CHECK(Is(EMSCRIPTEN_DEFAULT_3D_DEVICE, kSoftware), "web: software by default");
    const auto web = List({kGles3, kGles2, kPaletteSw, kSoftware});
    CHECK(Is(Choose(web, "", EMSCRIPTEN_DEFAULT_3D_DEVICE), kSoftware), "web: the software renderer");
    CHECK(Is(Choose(List({kSdlGpu, kGles3, kSoftware}), "", EMSCRIPTEN_DEFAULT_3D_DEVICE), kSoftware),
          "web: the software renderer before SDL GPU");

    /* Vita: GXM, no software or SDL GPU device; nothing draws yet. */
    const auto vita = List({kPaletteSw, kGxm});
    CHECK(Is(Choose(vita, "", kSdlGpu), kPaletteSw), "Vita: no device draws; the first, without 3D");
    CHECK(Is(Choose(vita, kGxm, kSdlGpu), kGxm), "Vita: no device draws; the explicit one");

    if (s_failures) {
        printf("%d check(s) failed\n", s_failures);
        return 1;
    }
    printf("device choice: all checks passed\n");
    return 0;
}
