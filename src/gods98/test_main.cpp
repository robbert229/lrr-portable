// SPDX-License-Identifier: LGPL-3.0-or-later
/* main.c — command-line options update engine flags, startup level and
 * frame timing. Forced texture management wins over the low-texture hint. */
#include "gods98/main_globs.h"
#include "test/lrr_test.h"

extern "C" void winmain_parse_options(const char*, int*, int*);
extern "C" void low_texture_flag_set(void);
extern "C" unsigned int render_feature_flags_get(void);

int main()
{
    constexpr unsigned int unmanagedTextures = 0x00000020;
    constexpr unsigned int window = 0x00000100;
    constexpr unsigned int startLevel = 0x00000200;
    constexpr unsigned int debug = 0x00002000;
    constexpr unsigned int forcedTextures = 0x00040000;
    int noSound = 0, insistOnCD = 0;
    s_engine_feature_word = 0;
    winmain_parse_options("-NoSound -INSISTONCD -Window -Debug -startlevel  Levels01 "
                          "-fpslock 50 -flags 23 -programmer 3", &noSound, &insistOnCD);
    CHECK_EQ(noSound, 1);
    CHECK_EQ(insistOnCD, 1);
    CHECK_EQ(s_engine_feature_word, window | startLevel | debug);
    CHECK_STREQ(mainGlobs.startLevel, "Levels01");
    CHECK_NEAR(mainGlobs.fixedFrameTiming, 0.5f, 1e-6);
    CHECK_EQ(render_feature_flags_get(), 23u);
    CHECK_EQ(s_application_mode, 3);

    low_texture_flag_set();
    CHECK_EQ(s_engine_feature_word, window | startLevel | debug | unmanagedTextures);

    s_engine_feature_word = 0;
    noSound = insistOnCD = 0;
    winmain_parse_options("-ftm -programmer -fpslock 0", &noSound, &insistOnCD);
    low_texture_flag_set();
    CHECK_EQ(noSound, 0);
    CHECK_EQ(insistOnCD, 0);
    CHECK_EQ(s_engine_feature_word, forcedTextures);
    CHECK_EQ(s_application_mode, 1);
    CHECK_NEAR(mainGlobs.fixedFrameTiming, 0.5f, 1e-6);
    return lrr_test_result();
}
