// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstring>
#include <fstream>
#include <string>

#include <SDL3/SDL.h>
#include <miniwin/windows.h>

#include "gods98/directdraw.h"
#include "gods98/init.h"
#include "gods98/main_globs.h"
#include "platform/native/config.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

extern "C" {
void winmain_parse_options(const char*, int*, int*);
const char* main_get_start_level(void);
int main_init_app(HINSTANCE);
}

static void check_options()
{
    int noSound = 0, insistOnCD = 0;
    winmain_parse_options("-NM -FVF -BEST -DUALMOUSE -DEBUGCOMPLETE -TESTERCALL "
                          "-TESTLEVELS -REDUCESAMPLES -SHOWVERSION -REDUCEANIMATION "
                          "-REDUCEPROMESHES -REDUCEFLICS -REDUCEIMAGES -CLEANSAVES",
                          &noSound, &insistOnCD);
    CHECK_EQ(s_engine_feature_word, 0x03fbe460u);
    CHECK_EQ(s_application_mode, 0); // No explicit -programmer overrides the tester mode.
    CHECK_EQ(noSound, 0);
    CHECK_EQ(insistOnCD, 0);
    CHECK(main_get_start_level() == nullptr, "no start-level override");
    s_engine_feature_word = 0;
    winmain_parse_options("-startlevel Abstract -programmer 10 -fpslock 25",
                          &noSound, &insistOnCD);
    CHECK_STREQ(main_get_start_level(), "Abstract");
    CHECK_EQ(s_application_mode, 10);
    CHECK_EQ(mainGlobs.fixedFrameTiming, 1.0f);
}

static int check_device(const std::string& scenario)
{
    const std::string directory = "startup_device_" + scenario;
    if (!CHECK(fixture_enter(directory.c_str()), "enter scratch directory")) return lrr_test_result();
    const std::string root = fixture_cwd();
    fixture_setenv("XDG_DATA_HOME", root + "/xdg");
    fixture_setenv("CFFIXED_USER_HOME", root + "/home");
    fixture_mkdir(root + "/home");
    char* pref = SDL_GetPrefPath(LRR_CONFIG_ORG, LRR_CONFIG_APP);
    const std::string prefDir = pref ? pref : "";
    SDL_free(pref);
    // Match test_config: never read or write the user's real preferences.
    if (prefDir.compare(0, root.size() + 1, root + "/") != 0) return 77;

    const bool wide = scenario.compare(0, 4, "wide") == 0;
    const bool fit = scenario == "fit" || wide;
    const bool best = scenario == "best";
    /* tweaks:widescreen: the window's aspect at the mode's height, 4:1 at
     * most; a window no wider than 4:3 keeps 640x480. */
    const int windowW = scenario == "wide" ? 2560 : scenario == "wide-tall" ? 1280 : scenario == "wide-cap" ? 4000 : 640;
    const int windowH = scenario == "wide" ? 1440 : scenario == "wide-tall" ? 1024 : scenario == "wide-cap" ? 400 : 480;
    const unsigned modeW = scenario == "wide" ? 853u : scenario == "wide-cap" ? 1920u : 640u;
    {
        std::ofstream ini("fixture.ini");
        ini << "[legorr]\n"
            << "Full Screen = false\nHorizontal Resolution = " << windowW << "\nVertical Resolution = " << windowH
            << "\n3D Device = " << (scenario == "fit" ? "Unknown device" : LRR_DEVICE_SOFTWARE) << "\n"
            << "savepath = " << root << "\n"
            << "[tweaks]\ndisplay = " << (fit ? "fit" : "original") << "\n";
        ini.close();
        if (!CHECK(ini.good(), "write isolated preferences")) return lrr_test_result();
    }
    if (!CHECK(LRR_LoadConfig("fixture.ini"), "load explicit host preferences") ||
        !CHECK(SDL_Init(SDL_INIT_VIDEO), SDL_GetError())) return lrr_test_result();
    std::strcpy(mainGlobs.programName, "HostStartupTest");
    mainGlobs.className = mainGlobs.programName;
    if (!CHECK(main_init_app(nullptr), "create real dummy window")) return lrr_test_result();
    directdraw_initialise(mainGlobs.hWnd);
    int ok = init_initialise(1, fit ? 0 : 1, best, 1, "No hardware device");
    if (CHECK(ok, "initialise display")) {
        CHECK(initGlobs.selDevice != nullptr, "selected device");
        if (initGlobs.selDevice) {
            if (best)
                CHECK(initGlobs.selDevice->flags & DIRECTDRAW_FLAG_DEVICE_HARDWARE, "best picks a hardware-flagged device");
            else
                CHECK_STREQ(initGlobs.selDevice->desc, LRR_DEVICE_SOFTWARE);
        }
        CHECK(initGlobs.selMode != nullptr, "selected mode");
        CHECK_EQ(initGlobs.selFullScreen, 0);
        CHECK_EQ(initGlobs.validModeCount, fit ? (modeW != 640u ? 2u : 1u) : 3u);
        CHECK_EQ(directDrawGlobs.width, modeW);
        CHECK_EQ(directDrawGlobs.height, 480u);
        CHECK_EQ(directdraw_widescreen_margin(), static_cast<int>(modeW) - 640);
        unsigned mode = 0;
        if (initGlobs.selMode) {
            CHECK(init_get_mode(initGlobs.selMode->desc, &mode), "find selected mode by name");
            CHECK(init_is_valid_mode(mode), "selected mode passes filter");
        }
        CHECK_EQ(init_get_mode("not a mode", &mode), 0);
    }
    directdraw_shutdown();
    DestroyWindow(static_cast<HWND>(mainGlobs.hWnd));
    return lrr_test_result();
}

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "options";
    if (scenario == "options") {
        check_options();
        return lrr_test_result();
    }
    if (!CHECK(scenario == "fit" || scenario == "original" || scenario == "best" || scenario == "wide" ||
                   scenario == "wide-tall" || scenario == "wide-cap",
               "known scenario"))
        return lrr_test_result();
    return check_device(scenario);
}
