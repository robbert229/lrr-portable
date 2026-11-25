// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstring>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <miniwin/windows.h>

#include "gods98/init.h"
#include "gods98/main_globs.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

extern "C" int main_init_app(HINSTANCE);
static std::string failure;
static std::vector<const DirectDraw_Device*> attempts;

// These doubles only remove external device availability or reject setup:
// no game callbacks or successful device setup are fabricated.
extern "C" int __real_directdraw_enum_drivers(DirectDraw_Driver*, unsigned*);
extern "C" int __wrap_directdraw_enum_drivers(DirectDraw_Driver* drivers, unsigned* count)
{
    if (failure == "no-driver") {
        *count = 0;
        return 0;
    }
    return __real_directdraw_enum_drivers(drivers, count);
}

extern "C" int __real_directdraw_enum_devices(const DirectDraw_Driver*, DirectDraw_Device*, unsigned*);
extern "C" int __wrap_directdraw_enum_devices(const DirectDraw_Driver* driver, DirectDraw_Device* devices,
                                               unsigned* count)
{
    int result = __real_directdraw_enum_devices(driver, devices, count);
    if (failure == "no-hardware") {
        for (unsigned i = 0; i < *count; ++i)
            devices[i].flags &= ~DIRECTDRAW_FLAG_DEVICE_HARDWARE;
    }
    return result;
}

extern "C" int __wrap_directdraw_setup(int, const DirectDraw_Driver*, const DirectDraw_Device* device,
                                       const DirectDraw_Mode*, unsigned, unsigned, unsigned, unsigned)
{
    attempts.push_back(device);
    return 0;
}

int main(int argc, char** argv)
{
    failure = argc == 2 ? argv[1] : "no-driver";
    if (!CHECK(failure == "no-driver" || failure == "no-hardware" || failure == "setup-fails",
               "known scenario")) return lrr_test_result();
    if (!CHECK(fixture_enter(("startup_device_error_" + failure).c_str()), "isolated scratch directory") ||
        !CHECK(SDL_Init(SDL_INIT_VIDEO), SDL_GetError())) return lrr_test_result();
    std::strcpy(mainGlobs.programName, "StartupDeviceError");
    mainGlobs.className = mainGlobs.programName;
    if (!CHECK(main_init_app(nullptr), "create real dummy window")) return lrr_test_result();
    directdraw_initialise(mainGlobs.hWnd);
    CHECK_EQ(init_initialise(1, 0, failure == "no-hardware", 1, "No hardware device"), 0);
    if (failure == "no-driver") {
        CHECK_EQ(initGlobs.driverCount, 0u);
        CHECK(initGlobs.selDevice == nullptr, "no device chosen");
        CHECK(attempts.empty(), "no setup attempted");
    } else if (failure == "no-hardware") {
        CHECK(initGlobs.deviceCount > 0, "enumerated non-hardware devices");
        CHECK(initGlobs.selDevice == nullptr, "best rejected non-hardware devices");
        CHECK(attempts.empty(), "no setup attempted");
    } else {
        CHECK(initGlobs.deviceCount > 0, "enumerated devices");
        CHECK_EQ(attempts.size(), initGlobs.deviceCount);
        for (size_t i = 0; i < attempts.size(); ++i) {
            CHECK(attempts[i] != nullptr, "attempted real enumerated device");
            for (size_t j = 0; j < i; ++j) CHECK(attempts[i] != attempts[j], "no repeated failed device");
        }
    }
    directdraw_shutdown();
    DestroyWindow(static_cast<HWND>(mainGlobs.hWnd));
    return lrr_test_result();
}
