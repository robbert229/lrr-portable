// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_device_choice — the 3D device the [library:config] substitution in
 * Init.c chooses (LRR_ChooseDevice, platform/native/config.h), from
 * synthetic device lists shaped like the ones miniwin builds on each
 * platform, with each platform's default device. Only the software, SDL
 * GPU and OpenGL ES 3 devices draw the level (miniwin's
 * Miniwin_DeviceDrawsImmediateMode). miniwin lists the OpenGL devices only
 * on an OpenGL window, and SDL GPU only on another window
 * (LRR_UseOpenGLWindow). */
#include <stdio.h>
#include <string.h>

#include <initializer_list>
#include <vector>

#include "platform/native/3ds/config.h"
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
static const char* const kGles3 = LRR_DEVICE_GLES3;
static const char* const kGles2 = "Miniwin driver (OpenGL ES 2.0 HAL)";
static const char* const kGl1 = "Miniwin driver (OpenGL 1.1 HAL)";
static const char* const kDirectX9 = "Miniwin driver (DirectX 9 HAL)";
static const char* const kGxm = "Miniwin driver (GXM HAL)";
static const char* const kCitro3D = "Miniwin driver (Citro3D)";
static const char* const kPaletteSw = "Miniwin driver (Miniwin Paletted Software)";
static const char* const kSoftware = LRR_DEVICE_SOFTWARE;

static std::vector<LRR_DeviceCandidate> List(std::initializer_list<const char*> descs)
{
    std::vector<LRR_DeviceCandidate> list;
    for (const char* desc : descs) {
        const bool draws = desc == kSdlGpu || desc == kSoftware || desc == kGles3;
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

    /* The main window is an OpenGL window only when the configured device,
     * or with none the platform's default, is an OpenGL device. */
    CHECK(!LRR_UseOpenGLWindow("", kSdlGpu), "OpenGL window: not for the SDL GPU default");
    CHECK(!LRR_UseOpenGLWindow(NULL, kSoftware), "OpenGL window: not for the software default");
    CHECK(LRR_UseOpenGLWindow(kGles3, kSdlGpu), "OpenGL window: for a configured OpenGL ES 3 device");
    CHECK(LRR_UseOpenGLWindow(kGl1, kSdlGpu), "OpenGL window: for a configured OpenGL 1 device");
    CHECK(!LRR_UseOpenGLWindow(kSoftware, kGles3), "OpenGL window: the configured device wins over the default");
    CHECK(LRR_UseOpenGLWindow("", kGles3), "OpenGL window: for an OpenGL ES 3 default");
    CHECK(!LRR_UseOpenGLWindow(NULL, NULL), "OpenGL window: not without a device");

    /* The desktop: miniwin lists SDL GPU first when it can create a device
     * (a Vulkan, Metal or Direct3D 12 driver), and the software renderer
     * last. The window is not an OpenGL window, so no OpenGL device is
     * listed. */
    const auto desktop = List({kSdlGpu, kPaletteSw, kSoftware});
    CHECK(Is(Choose(desktop, "", kSdlGpu), kSdlGpu), "desktop: SDL GPU when it is available");
    CHECK(Is(Choose(desktop, kSoftware, kSdlGpu), kSoftware), "desktop: an explicit software device wins");
    CHECK(Is(Choose(desktop, kSdlGpu, kSdlGpu), kSdlGpu), "desktop: an explicit SDL GPU device");
    CHECK(Is(Choose(desktop, kPaletteSw, kSdlGpu), kPaletteSw),
          "desktop: an explicit device wins even when it cannot draw the level");
    CHECK(Is(Choose(desktop, "Miniwin driver (Typo)", kSdlGpu), kSdlGpu),
          "desktop: an explicit device that is not listed falls back to the default");

    /* The desktop with an OpenGL device configured: an OpenGL window, on
     * which the OpenGL devices are listed and SDL GPU is not. */
    const auto desktopGl = List({kGles3, kGles2, kGl1, kPaletteSw, kSoftware});
    CHECK(Is(Choose(desktopGl, kGles3, kSdlGpu), kGles3), "desktop: an explicit OpenGL ES 3 device");
    CHECK(Is(Choose(desktopGl, kGl1, kSdlGpu), kGl1),
          "desktop: an explicit OpenGL 1 device wins even though it cannot draw the level");
    auto glFailed = desktopGl;
    glFailed[0].failed = true;
    CHECK(Is(Choose(glFailed, kGles3, kSdlGpu), kSoftware),
          "desktop: an OpenGL ES 3 device that failed falls back to the software renderer");

    /* No Vulkan, Metal or Direct3D 12 driver: SDL GPU is not listed. */
    const auto noGpu = List({kPaletteSw, kSoftware});
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
    failed[2].failed = true;
    CHECK(Is(Choose(failed, "", kSdlGpu), kPaletteSw),
          "every drawing device failed: the first device left, without 3D");
    /* Init.c's retry: LRR_ChooseDeviceAfterFailure marks the device failed
     * and chooses again (legorr:3D Device is empty before LRR_LoadConfig). */
    auto retry = desktop;
    CHECK(Is(retry[LRR_ChooseDeviceAfterFailure(retry.data(), (unsigned int)retry.size(), 0)].desc, kSoftware) &&
              retry[0].failed,
          "retry: SDL GPU failed, then the software renderer");
    CHECK(LRR_ChooseDeviceAfterFailure(retry.data(), (unsigned int)retry.size(), 99) == -1,
          "retry: an index past the list");
    for (LRR_DeviceCandidate& device : failed) device.failed = true;
    CHECK(Choose(failed, "", kSdlGpu)[0] == '\0', "every device failed: none");
    CHECK(Choose({}, "", kSdlGpu)[0] == '\0', "no devices: none");

    /* Switch: miniwin builds no software renderer (nor OpenGL 1 or
     * OpenGL ES 2), so the last hardware device cannot draw the level. */
    CHECK(Is(NX_DEFAULT_3D_DEVICE, kSdlGpu), "Switch: SDL GPU by default");
    const auto nx = List({kSdlGpu, kGles3, kPaletteSw});
    CHECK(Is(Choose(nx, "", NX_DEFAULT_3D_DEVICE), kSdlGpu), "Switch: SDL GPU");
    CHECK(Is(Choose(nx, kGles3, NX_DEFAULT_3D_DEVICE), kGles3), "Switch: an explicit OpenGL ES 3 device wins");
    CHECK(Is(Choose(nx, kSoftware, NX_DEFAULT_3D_DEVICE), kSdlGpu),
          "Switch: an explicit software device, which is not built there, falls back to SDL GPU");
    const auto nxNoGpu = List({kGles3, kPaletteSw});
    CHECK(Is(Choose(nxNoGpu, "", NX_DEFAULT_3D_DEVICE), kGles3), "Switch without SDL GPU: OpenGL ES 3");

    /* Android and iOS: SDL GPU (Vulkan, Metal); the software renderer is
     * listed but too slow. */
    CHECK(Is(ANDROID_DEFAULT_3D_DEVICE, kSdlGpu), "Android: SDL GPU by default");
    CHECK(Is(IOS_DEFAULT_3D_DEVICE, kSdlGpu), "iOS: SDL GPU by default");
    const auto android = List({kSdlGpu, kGles3, kGles2, kPaletteSw, kSoftware});
    CHECK(Is(Choose(android, "", ANDROID_DEFAULT_3D_DEVICE), kSdlGpu), "Android: SDL GPU");
    CHECK(Is(Choose(android, "", IOS_DEFAULT_3D_DEVICE), kSdlGpu), "iOS: SDL GPU");
    CHECK(Is(Choose(android, kSoftware, ANDROID_DEFAULT_3D_DEVICE), kSoftware),
          "Android: an explicit software device wins");

    /* The web: the software renderer, even if SDL GPU or OpenGL ES 3 were
     * listed, as WebGL cannot present from WinMain's worker. */
    CHECK(Is(EMSCRIPTEN_DEFAULT_3D_DEVICE, kSoftware), "web: software by default");
    const auto web = List({kGles3, kGles2, kPaletteSw, kSoftware});
    CHECK(Is(Choose(web, "", EMSCRIPTEN_DEFAULT_3D_DEVICE), kSoftware), "web: the software renderer");
    CHECK(Is(Choose(List({kSdlGpu, kGles3, kSoftware}), "", EMSCRIPTEN_DEFAULT_3D_DEVICE), kSoftware),
          "web: the software renderer before SDL GPU");

    /* 3DS: Citro3D, the palette and software renderers; no SDL GPU or
     * OpenGL. Citro3D draws the level, so the default picks it; were it
     * unable to, the software renderer would. */
    CHECK(Is(N3DS_DEFAULT_3D_DEVICE, kCitro3D), "3DS: Citro3D by default");
    auto n3ds = List({kCitro3D, kPaletteSw, kSoftware});
    n3ds[0].drawsLevel = true;
    CHECK(Is(Choose(n3ds, "", N3DS_DEFAULT_3D_DEVICE), kCitro3D),
          "3DS: Citro3D, which draws the level, before the software renderer");
    CHECK(Is(Choose(n3ds, kSoftware, N3DS_DEFAULT_3D_DEVICE), kSoftware), "3DS: an explicit software device wins");
    auto n3dsNoDraw = n3ds;
    n3dsNoDraw[0].drawsLevel = false;
    CHECK(Is(Choose(n3dsNoDraw, "", N3DS_DEFAULT_3D_DEVICE), kSoftware),
          "3DS: a Citro3D that could not draw the level gives way to the software renderer");

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
