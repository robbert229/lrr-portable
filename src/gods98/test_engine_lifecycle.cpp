// SPDX-License-Identifier: LGPL-3.0-or-later
/* WinMain as an engine host, with generic application callbacks instead of
 * the game. No model, level or asset data is supplied. */
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "platform/native/test_fixture.h"
#include <miniwin/windows.h>
#include <miniwin/miniwinpresentation.h>
#include "gods98/config.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/init.h"
#include "gods98/input_globs.h"
#include "gods98/main_globs.h"
#include "platform/native/config.h"
#include "platform/native/lifecycle.h"
#include "test/lrr_test.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
extern "C" char* seam_command_line;

/* Same private callback record as main.c. */
typedef struct Main_State {
    int (*Initialise)(void);
    int (*MainLoop)(float elapsed);
    void (*Shutdown)(void);
} Main_State;
extern "C" int window_runtime_callback_triplet_set(const Main_State*);
extern "C" void renderer_begin_frame_once_retail(void);
extern "C" int window_runtime_message_dispatch_filter(const MSG*);

namespace {
constexpr unsigned updated = 0x1;
constexpr unsigned fullscreen = 0x2;
constexpr unsigned paused = 0x10;
constexpr unsigned deterministic = 0x80;

std::string scenario;
std::vector<float> times;
unsigned registrations, inits, shutdowns;
SDL_WindowID windowId;
BlockFile* applicationConfig;
bool returned;
Container* sceneRoot;
Container* sceneMesh;
Container* sceneLight;

bool config_variant()
{
    return scenario == "fit-config" || scenario == "debug-config" ||
           scenario == "resource-reject";
}

void release_scene()
{
    if (sceneLight) Container_Remove(sceneLight);
    if (sceneMesh) Container_Remove(sceneMesh);
    if (sceneRoot) Container_Shutdown();
    sceneLight = sceneMesh = sceneRoot = nullptr;
}

bool setup_scene()
{
    sceneRoot = Container_Initialise("Fixture");
    if (!CHECK(sceneRoot != nullptr, "create generic scene root")) return false;
    Container_SetSharedTextureDirectory("unused");
    sceneMesh = Container_MakeMesh2(sceneRoot, 1);
    sceneLight = Container_MakeLight(sceneRoot, static_cast<unsigned long>(D3DRMLIGHT_SPOT),
                                     0.25f, 0.5f, 0.75f);
    if (!CHECK(sceneMesh && sceneLight, "create empty mesh and light")) return false;
    CHECK_EQ(Container_GetParent(sceneMesh), sceneRoot);
    CHECK_EQ(Container_GetParent(sceneLight), sceneRoot);
    CHECK_EQ(Container_Frame_GetOwner(Container_GetMasterFrame(sceneMesh)), sceneMesh);
    CHECK(Container_Light_SetSpotPenumbra(sceneLight, 0.5f), "set generic light penumbra");
    CHECK(Container_Light_SetSpotUmbra(sceneLight, 0.25f), "set generic light umbra");
    CHECK(Container_Light_SetSpotRange(sceneLight, 25.0f), "set generic light range");
    Container* missing = Container_Load(sceneRoot, "absent", "ACT", 0);
    CHECK_EQ(missing, nullptr);
    CHECK_EQ(Container_Load(sceneRoot, "empty", "ACT", 0), nullptr);
    CHECK_EQ(Container_Load(sceneRoot, "absent", "ANIM", 0), nullptr);
    CHECK_EQ(Container_Load(sceneRoot, "absent", "MESH", 0), nullptr);
    CHECK_EQ(Container_Load(sceneRoot, "absent", "unknown", 0), nullptr);
    // Missing optional resources must not invalidate the existing scene.
    CHECK_EQ(Container_GetParent(sceneMesh), sceneRoot);
    const bool required = block_get_bool_path(applicationConfig, "Fixture::RequireResource") == 1;
    return (!required || missing != nullptr) && lrr_test_failures() == 0;
}

void exit_guard()
{
    // file_initialise can exit(0) on a missing data directory. A test must
    // never pass unless WinMain returned and the assertions completed.
    if (!returned) {
        std::fputs("FAIL: WinMain exited before completing the test\n", stderr);
        std::fflush(stderr);
        std::_Exit(1);
    }
}

void push_window_event(Uint32 type)
{
    SDL_Event event = {};
    event.type = type;
    event.window.windowID = windowId;
    CHECK(SDL_PushEvent(&event), "queue synthetic window event");
}

void push_mouse_event(Uint32 type, Uint8 button = SDL_BUTTON_MIDDLE)
{
    SDL_Event event = {};
    event.type = type;
    event.button.windowID = windowId;
    event.button.button = button;
    CHECK(SDL_PushEvent(&event), "queue synthetic mouse button");
}

int init_callback()
{
    ++inits;
    CHECK_EQ(registrations, 1u);
    CHECK(times.empty(), "initialise before the first tick");
    CHECK(s_main_device != nullptr, "real retained-mode device set up");
    CHECK(s_main_im_device != nullptr, "real immediate-mode device set up");
    CHECK_EQ(directDrawGlobs.width, scenario == "debug-config" ? 800u : 640u);
    CHECK_EQ(directDrawGlobs.height, scenario == "debug-config" ? 600u : 480u);
    CHECK(block_text_token_pool_resident_peek() != 0, "configuration pool is live");
    CHECK(windowId != 0, "real miniwin window");
    // Check the filter in the actual selected display mode, independently
    // of whether the dummy driver sends system key messages.
    MSG msg = {};
    for (unsigned message : {WM_ACTIVATEAPP, WM_SYSKEYDOWN, WM_SYSKEYUP}) {
        msg.message = message;
        CHECK_EQ(window_runtime_message_dispatch_filter(&msg), scenario == "fullscreen" ? 0 : 1);
    }
    msg.message = WM_CLOSE;
    CHECK_EQ(window_runtime_message_dispatch_filter(&msg), 1);
    // Generic application configuration, read through the real file layer.
    applicationConfig = block_open("application.cfg");
    if (!CHECK(applicationConfig != nullptr, "read isolated application record")) return 0;
    CHECK_STREQ(block_get_strptr(applicationConfig, "Fixture::Value"), "Example");
    if (config_variant()) {
        CHECK_EQ(initGlobs.validModeCount, scenario == "debug-config" ? 3u : 1u);
        CHECK(initGlobs.selMode != nullptr, "selected display mode");
        if (initGlobs.selMode) CHECK_EQ(initGlobs.selMode->width, directDrawGlobs.width);
        CHECK(initGlobs.selDevice != nullptr, "selected renderer");
        if (initGlobs.selDevice) CHECK_STREQ(initGlobs.selDevice->desc, LRR_DEVICE_SOFTWARE);
        const bool fit = scenario == "fit-config";
        CHECK_EQ(Miniwin_GetRenderResolution(), fit ? MiniwinRenderResolution::Window :
                                                    MiniwinRenderResolution::Virtual);
        int w = 0, h = 0;
        CHECK(SDL_GetWindowSize(SDL_GetWindowFromID(windowId), &w, &h), "query host window");
        CHECK_EQ(w, fit ? 713 : scenario == "debug-config" ? 800 : 640);
        CHECK_EQ(h, fit ? 521 : scenario == "debug-config" ? 600 : 480);
        CHECK_EQ(block_get_bool_path(applicationConfig, "Fixture::Enabled"), fit ? 1 : 0);
        CHECK_EQ(block_get_bool_path(applicationConfig, "Fixture::Optional"), 2);
        const bool ready = setup_scene();
        CHECK_EQ(ready, scenario != "resource-reject");
        if (!ready) {
            release_scene();
            block_close(applicationConfig);
            applicationConfig = nullptr;
            return 0;
        }
    }
    if (scenario == "reject") {
        block_close(applicationConfig);
        applicationConfig = nullptr;
        return 0;
    }
    return 1;
}

void shutdown_callback()
{
    ++shutdowns;
    CHECK_EQ(registrations, 1u);
    CHECK_EQ(inits, 1u);
    CHECK(block_text_token_pool_resident_peek() != 0, "callback shuts down before engine config pool");
    CHECK(SDL_GetWindowFromID(windowId) != nullptr, "callback shuts down before window destruction");
    if (config_variant()) {
        CHECK(sceneRoot && sceneMesh && sceneLight, "scene still owned by application");
        release_scene();
    }
    if (CHECK(applicationConfig != nullptr, "application still owns its configuration"))
        block_close(applicationConfig);
    applicationConfig = nullptr;
}

int tick_callback(float elapsed)
{
    times.push_back(elapsed);
    const size_t tick = times.size();
    CHECK_EQ(inits, 1u);
    CHECK_EQ(shutdowns, 0u);
    CHECK(applicationConfig != nullptr, "application initialized");
    CHECK(!(s_engine_feature_word & updated), "per-frame renderer update flag cleared");

    if (config_variant()) {
        CHECK_EQ(Container_GetParent(sceneMesh), sceneRoot);
        const bool dual = scenario == "debug-config";
        switch (tick) {
        case 1:
            push_mouse_event(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT);
            break;
        case 2:
            CHECK_EQ(g_renderer_word_c, 1);
            CHECK_EQ(s_window_mouse_word_40, dual ? 0 : 1);
            push_mouse_event(SDL_EVENT_MOUSE_BUTTON_UP, SDL_BUTTON_LEFT);
            push_mouse_event(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_RIGHT);
            break;
        case 3:
            CHECK_EQ(g_renderer_word_c, dual ? 0 : 1);
            CHECK_EQ(s_window_mouse_word_40, 1);
            push_mouse_event(SDL_EVENT_MOUSE_BUTTON_UP, SDL_BUTTON_RIGHT);
            break;
        case 4:
            CHECK_EQ(g_renderer_word_c, 0);
            CHECK_EQ(s_window_mouse_word_40, 0);
            return 0;
        }
    } else if (scenario == "fixed") {
        switch (tick) {
        case 1:
            // Update only once even if an application requests it twice.
            renderer_begin_frame_once_retail();
            CHECK(s_engine_feature_word & updated, "renderer update latched");
            renderer_begin_frame_once_retail();
            break;
        case 2: s_engine_feature_word |= paused; break;
        case 3: s_engine_feature_word |= deterministic; break;
        case 4: s_engine_feature_word &= ~(paused | deterministic); break;
        case 5: return 0;
        }
    } else if (scenario == "realtime") {
        if (tick == 1) SDL_Delay(140); // Real time past the 3-frame clamp.
        if (tick == 3) return 0;
    } else if (scenario == "events") {
        switch (tick) {
        case 1:
            push_window_event(SDL_EVENT_WINDOW_FOCUS_LOST);
            break;
        case 2: {
            CHECK_EQ(s_window_application_active, 0u);
            CHECK_EQ(s_input_keyboard_acquired, 0);
            push_window_event(SDL_EVENT_WINDOW_FOCUS_GAINED);
            push_mouse_event(SDL_EVENT_MOUSE_BUTTON_DOWN);
            SDL_Event wheel = {};
            wheel.type = SDL_EVENT_MOUSE_WHEEL;
            wheel.wheel.windowID = windowId;
            wheel.wheel.y = 1.0f;
            CHECK(SDL_PushEvent(&wheel), "queue synthetic wheel event");
            break;
        }
        case 3:
            CHECK_EQ(s_window_application_active, 1u);
            CHECK_EQ(s_input_keyboard_acquired, 1);
            CHECK_EQ(inputMiddleButton, 1);
            CHECK_EQ(inputWheelDelta, WHEEL_DELTA);
            push_mouse_event(SDL_EVENT_MOUSE_BUTTON_UP);
            LRR_ReportLifecycle(LRR_LIFECYCLE_EXIT);
            break;
        case 4:
            CHECK(s_window_close_requested, "quit event reached WinMain");
            CHECK_EQ(inputMiddleButton, 0);
            CHECK_EQ(inputWheelDelta, 0);
            return 1; // Only the real quit message should stop the loop.
        }
    } else if (scenario == "fullscreen") {
        CHECK(s_engine_feature_word & fullscreen, "fullscreen device configured");
        switch (tick) {
        case 1:
            CHECK_EQ(s_window_application_active, 1u);
            LRR_ReportLifecycle(LRR_LIFECYCLE_SUSPEND);
            break;
        case 2:
            CHECK_EQ(s_window_application_active, 0u);
            LRR_ReportLifecycle(LRR_LIFECYCLE_RESUME);
            break;
        case 3:
            CHECK_EQ(s_window_application_active, 0u);
            break;
        case 4:
            CHECK_EQ(s_window_application_active, 1u);
            return 0;
        }
    }
    if (tick >= 6) {
        CHECK(false, "bounded callback loop exceeded expected frame count");
        return 0;
    }
    return 1;
}
} // namespace

// Only the application's registration seam is replaced in this test
// executable. The callback registry and WinMain itself are the real unit.
extern "C" void engine_lifecycle_register_test_callbacks(const char* name)
{
    ++registrations;
    CHECK_STREQ(name, "EngineHostTest");
    CHECK_EQ(inits, 0u);
    windowId = SDL_GetWindowID(reinterpret_cast<SDL_Window*>(mainGlobs.hWnd));
    CHECK(windowId != 0, "real miniwin window at registration");
    if (scenario == "no-state") {
        CHECK_EQ(window_runtime_callback_triplet_set(nullptr), 0);
        CHECK_EQ(mainGlobs.stateSet, 0);
        return;
    }
    const Main_State state = {
        init_callback, scenario == "no-loop" ? nullptr : tick_callback, shutdown_callback
    };
    CHECK(window_runtime_callback_triplet_set(&state), "register generic callback state");
}

int main(int argc, char** argv)
{
    scenario = argc == 2 ? argv[1] : "fixed";
    if (!CHECK(scenario == "fixed" || scenario == "realtime" || scenario == "events" ||
               scenario == "fullscreen" || scenario == "reject" || scenario == "no-loop" || scenario == "no-state" ||
               config_variant(),
               "known scenario")) return lrr_test_result();
    const std::string dir = "engine_lifecycle_" + scenario;
    if (!CHECK(fixture_enter(dir.c_str()) && fixture_mkdir("Data"), "isolated scratch directory"))
        return lrr_test_result();
    const std::string root = fixture_cwd();
    const std::string prefs = root + "/prefs";
    {
        std::ofstream config("Data/application.cfg");
        config << "Fixture { Value Example Enabled " << (scenario == "fit-config" ? "TRUE" : "FALSE")
               << " RequireResource " << (scenario == "resource-reject" ? "TRUE" : "FALSE") << " }\n";
        config.close();
        if (!CHECK(config.good(), "write generic application config")) return lrr_test_result();
        if (config_variant()) {
            if (!CHECK(fixture_mkdir("Data/empty"), "create isolated resource directory")) return lrr_test_result();
            std::ofstream empty("Data/empty/empty.ae");
            empty << "Fixture { Value Example }\n";
            empty.close();
            if (!CHECK(empty.good(), "write config without optional activity group")) return lrr_test_result();
        }
        std::ofstream ini("fixture.ini");
        ini << "[legorr]\nFull Screen = " << (scenario == "fullscreen" ? "true" : "false")
            << "\nHorizontal Resolution = " << (scenario == "debug-config" ? 800 : config_variant() ? 713 : 640)
            << "\nVertical Resolution = " << (scenario == "debug-config" ? 600 : config_variant() ? 521 : 480) << "\n"
            << "3D Device = " << LRR_DEVICE_SOFTWARE << "\n"
            << "[tweaks]\ndisplay = " << (scenario == "fit-config" ? "fit" : "original") << "\nfps_overlay = "
            << (scenario == "realtime" ? "true" : "false") << "\n";
        ini.close();
        if (!CHECK(ini.good(), "write host preferences")) return lrr_test_result();
    }
    if (!CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError())) return lrr_test_result();
    LRR_SetLaunchOptions(root.c_str(), prefs.c_str());
    if (!CHECK(LRR_LoadConfig("fixture.ini"), "load isolated host preferences")) return lrr_test_result();
    char commandLine[] = "\"C:\\Scratch\\EngineHostTest.exe\"";
    seam_command_line = commandLine;
    char fixedArgs[] = "-nosound -window -fpslock 50";
    char realArgs[] = "-nosound -window";
    char debugArgs[] = "-nosound -window -debug -dualmouse -fpslock 50";
    std::atexit(exit_guard);
    CHECK_EQ(WinMain(nullptr, nullptr, scenario == "realtime" ? realArgs :
                         scenario == "debug-config" ? debugArgs : fixedArgs, SW_SHOWNORMAL), 0);
    CHECK_EQ(registrations, 1u);
    CHECK_EQ(inits, scenario == "no-state" ? 0u : 1u);
    CHECK_EQ(shutdowns, (scenario == "reject" || scenario == "no-state" || scenario == "resource-reject") ? 0u : 1u);
    CHECK(sceneRoot == nullptr && sceneMesh == nullptr && sceneLight == nullptr, "no application scene owners");
    CHECK(applicationConfig == nullptr, "application owns no config after exit");
    CHECK_EQ(block_text_token_pool_resident_peek(), 0);
    CHECK_EQ(block_text_token_pool_head_peek(), nullptr);
    CHECK_EQ(s_input_keyboard_acquired, 0);
    CHECK(SDL_GetWindowFromID(windowId) == nullptr, "WinMain destroyed its window");

    if (scenario == "no-state") {
        CHECK_EQ(mainGlobs.stateSet, 0);
        CHECK(times.empty(), "unregistered application never ticks");
    } else if (scenario == "reject" || scenario == "resource-reject") {
        CHECK(times.empty(), "rejected init never ticks");
        for (const void* callback : mainGlobs.currState) CHECK(callback == nullptr, "callback cleared");
    } else if (scenario == "no-loop") {
        CHECK(times.empty(), "no loop when none registered");
    } else if (scenario == "fixed") {
        const float expected[] = {1.0f, 0.5f, 0.0f, 25.0f / 30.0f, 0.5f};
        if (CHECK_EQ(times.size(), sizeof(expected)/sizeof(expected[0])))
            for (size_t i = 0; i < times.size(); ++i) CHECK_NEAR(times[i], expected[i], 0.000001f);
    } else if (scenario == "realtime") {
        if (CHECK_EQ(times.size(), 3u)) {
            CHECK_EQ(times[0], 1.0f);
            CHECK_EQ(times[1], 3.0f);
            CHECK(times[2] >= 0.0f && times[2] <= 3.0f, "real-time step bounded");
        }
    } else {
        CHECK_EQ(times.size(), 4u);
    }
    SDL_Quit();
    returned = true;
    return lrr_test_result();
}
