// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_loader — section matching and byte accounting across profile
 * reads and writes, with leftward growth checked through its draw output. */
#include <cstdlib>
#include <fstream>
#include <iterator>

#include "platform/native/test_fixture.h"
#include "game/loader.h"
#include "gods98/files.h"
#include "test/lrr_test.h"

static Image background, bar;
static Point2F barPosition, barSize;
static bool fileSystemReady;

static void fail_if_startup_exited()
{
    if (!fileSystemReady) {
        std::puts("FAIL: the file system exited before the test could run");
        std::fflush(stdout);
        std::_Exit(1);
    }
}

extern "C" Image* __wrap_Image_LoadBMPScaled(const char* name, unsigned int, unsigned int)
{
    if (std::strcmp(name, "load") == 0) return &background;
    if (std::strcmp(name, "bar") == 0) return &bar;
    return nullptr;
}

extern "C" void __wrap_Image_DisplayScaled(Image* image, const Area2F*,
                                            const Point2F* position, const Point2F* size)
{
    if (image == &bar) {
        barPosition = *position;
        barSize = *size;
    }
}

extern "C" void __wrap_Image_Remove(Image*)
{
}

extern "C" void __wrap_main_loop_frame_pump(int)
{
}

int main(void)
{
    const std::string cwd = fixture_cwd();
    constexpr char fixture[] = "loader_profile_fixture";
    if (!CHECK(!cwd.empty() && fixture_enter(fixture), "the scratch directory is ready"))
        return lrr_test_result("loader");
    if (!CHECK(fixture_mkdir("Data"), "the scratch data directory is ready"))
        return lrr_test_result("loader");
    {
        std::ofstream profile("Data/Input.txt");
        profile << "Game Data=8\nMenus=16\n";
        profile.close();
        if (!CHECK(profile.good(), "the input profile is written"))
            return lrr_test_result("loader");
    }
    std::atexit(fail_if_startup_exited);
    file_initialise("loader_test", 0, "");
    fileSystemReady = true;

    constexpr int growLeft = 3;
    Area2F bounds = { 10.0f, 4.0f, 8.0f, 2.0f };
    loader_initialise("load", "unused", nullptr, "Input.txt", growLeft, "bar", &bounds, nullptr);
    loader_display_loading_bar("gAmE dAtA");
    CHECK_EQ(barPosition.x, 18.0f);
    CHECK_EQ(barSize.x, 0.0f);
    loader_file_load_callback("first", 2u, nullptr);
    CHECK_EQ(barPosition.x, 16.0f);
    CHECK_EQ(barPosition.y, 4.0f);
    CHECK_EQ(barSize.x, 2.0f);
    CHECK_EQ(barSize.y, 2.0f);
    loader_file_load_callback("second", 10u, nullptr);
    CHECK_EQ(barPosition.x, 10.0f);
    CHECK_EQ(barSize.x, 8.0f);

    loader_display_loading_bar(nullptr);
    loader_file_load_callback("ignored", 50u, nullptr);
    loader_display_loading_bar("Sounds");
    loader_file_load_callback("new-section-file", 7u, nullptr);
    CHECK_EQ(barSize.x, 0.0f);
    loader_display_loading_bar(nullptr);
    loader_shutdown("Output.txt");
    file_set_load_callback(nullptr, nullptr);

    {
        std::ifstream profile("Data/Output.txt");
        CHECK(profile.good(), "the output profile is readable");
        const std::string result(std::istreambuf_iterator<char>{profile}, {});
        CHECK_STREQ(result.c_str(), "Game Data=12\nMenus=16\nSounds=7\n");
    }
    if (CHECK(fixture_chdir(cwd), "the initial working directory is restored"))
        fixture_remove_all(fixture);
    return lrr_test_result("loader");
}
