// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_config_settings — the settings of the port's configuration file
 * (platform/native/config.h, LRR_Setting), set in-process with
 * LRR_SetSetting: each setting's default, the spellings each kind of value
 * takes, and the typed getters the units read. No file is read or
 * written, so this runs on every host; test_config covers the file. */
#include <string.h>

/* Before files_native.h, whose macros would rename the declarations a
 * Windows C library's <io.h> makes (as test_config.cpp orders them). */
#include <string>

#include "gods98/files_native.h"
#include "platform/native/config.h"
#include "test/lrr_test.h"

namespace {

/* The value `setting` takes from `text`. */
int Read(LRR_Setting setting, const char* text)
{
    LRR_SetSetting(setting, text);
    return LRR_GetSetting(setting);
}

void Defaults()
{
    /* Before LRR_LoadConfig, the getters give the table's defaults. */
    const LRR_DisplayConfig* display = LRR_GetDisplayConfig();
    CHECK(!display->fullScreen, "default: a window");
    CHECK_EQ(display->width, 640);
    CHECK_EQ(display->height, 480);
    CHECK_STREQ(display->device, "");
    CHECK_EQ(LRR_GetTrainingSave(), LRR_TRAINING_SAVE_PROMPT);
    CHECK_EQ(LRR_GetDisplayScaling(), LRR_DISPLAY_FIT);
    CHECK(LRR_GetWidescreen(), "default: widescreen");
    CHECK_EQ(LRR_GetControls(), LRR_CONTROLS_MODERN);
    CHECK(LRR_GetFixIdlenessSyndrome(), "default: the idleness syndrome is fixed");
    CHECK(!LRR_GetFpsOverlay(), "default: no frame rate overlay");
    CHECK(!LRR_GetUnlimitedRaiders(), "default: the raider limit is kept");
    CHECK(!LRR_GetUnlimitedOxygen(), "default: the air drains");
    const LRR_TouchConfig touch = LRR_GetTouchConfig();
#if defined(ANDROID) || defined(IOS)
    CHECK(touch.enabled, "default: touch controls on, on a touch host");
#else
    CHECK(!touch.enabled, "default: touch controls off, on a desktop");
#endif
    CHECK(touch.longPressHover && touch.scroll && touch.zoom && touch.rotate &&
              touch.twoFingerTap == LRR_TOUCH_TAP_SPACE && touch.threeFingerKeyboard && touch.keyboard &&
              touch.backEscape,
          "default: every touch gesture is on");
}

/* A switch: the words, iniparser_getboolean's first letters, and the
 * default for anything else. */
void Switches()
{
    const LRR_Setting switches[] = {
        LRR_SETTING_FULLSCREEN,
        LRR_SETTING_WIDESCREEN,
        LRR_SETTING_FIX_IDLENESS_SYNDROME,
        LRR_SETTING_FPS_OVERLAY,
        LRR_SETTING_TOUCH,
        LRR_SETTING_TOUCH_SCROLL,
        LRR_SETTING_TOUCH_ZOOM,
        LRR_SETTING_TOUCH_ROTATE,
        LRR_SETTING_UNLIMITED_RAIDERS,
        LRR_SETTING_UNLIMITED_OXYGEN,
    };
    for (LRR_Setting setting : switches) {
        for (const char* text : { "true", "TRUE", "on", "On", "yes", "YES", "1", "t", "y", "Yep" }) {
            CHECK_EQ(Read(setting, text), 1);
        }
        for (const char* text : { "false", "False", "off", "OFF", "no", "No", "0", "original", "Original",
                                  "f", "n", "nope" }) {
            CHECK_EQ(Read(setting, text), 0);
        }
    }
    /* Anything else, and no value, is the default. */
    CHECK_EQ(Read(LRR_SETTING_FIX_IDLENESS_SYNDROME, "bogus"), 1);
    CHECK_EQ(Read(LRR_SETTING_FIX_IDLENESS_SYNDROME, ""), 1);
    CHECK_EQ(Read(LRR_SETTING_FIX_IDLENESS_SYNDROME, NULL), 1);
    CHECK_EQ(Read(LRR_SETTING_UNLIMITED_OXYGEN, "bogus"), 0);
    CHECK_EQ(Read(LRR_SETTING_UNLIMITED_OXYGEN, ""), 0);
    CHECK_EQ(Read(LRR_SETTING_UNLIMITED_OXYGEN, NULL), 0);
    CHECK_EQ(Read(LRR_SETTING_FULLSCREEN, "bogus"), 0);
    CHECK_EQ(Read(LRR_SETTING_FULLSCREEN, NULL), 0);
}

/* A choice: its names, the original's words, a two-name choice's true
 * words, and the default for anything else. */
void Choices()
{
    struct Case {
        LRR_Setting setting;
        const char* original;
        const char* other;
    };
    const Case cases[] = {
        { LRR_SETTING_TRAINING_SAVE, "off", "prompt" },
        { LRR_SETTING_DISPLAY, "original", "fit" },
        { LRR_SETTING_CONTROLS, "original", "modern" },
        { LRR_SETTING_TOUCH_LONG_PRESS, "off", "hover" },
        { LRR_SETTING_TOUCH_THREE_FINGER_TAP, "off", "keyboard" },
        { LRR_SETTING_TOUCH_KEYBOARD, "off", "auto" },
        { LRR_SETTING_BACK_BUTTON, "off", "escape" },
    };
    for (const Case& c : cases) {
        CHECK_EQ(Read(c.setting, c.original), 0);
        CHECK_EQ(Read(c.setting, c.other), 1);
        for (const char* text : { "original", "Original", "off", "Off", "false", "no", "0" }) {
            CHECK_EQ(Read(c.setting, text), 0);
        }
        for (const char* text : { "true", "on", "yes", "1" }) {
            CHECK_EQ(Read(c.setting, text), 1);
        }
        /* Each choice's default is its second value. */
        for (const char* text : { "bogus", "", "f", "n" }) {
            CHECK_EQ(Read(c.setting, text), 1);
        }
        CHECK_EQ(Read(c.setting, NULL), 1);
    }
    CHECK_EQ(Read(LRR_SETTING_TRAINING_SAVE, "OFF"), LRR_TRAINING_SAVE_OFF);
    CHECK_EQ(Read(LRR_SETTING_DISPLAY, "Fit"), LRR_DISPLAY_FIT);
    CHECK_EQ(Read(LRR_SETTING_CONTROLS, "MODERN"), LRR_CONTROLS_MODERN);

    /* The two-finger tap, a choice of three: no true words, and Space for
     * anything it does not know. */
    CHECK_EQ(Read(LRR_SETTING_TOUCH_TWO_FINGER_TAP, "off"), LRR_TOUCH_TAP_OFF);
    CHECK_EQ(Read(LRR_SETTING_TOUCH_TWO_FINGER_TAP, "false"), LRR_TOUCH_TAP_OFF);
    CHECK_EQ(Read(LRR_SETTING_TOUCH_TWO_FINGER_TAP, "Space"), LRR_TOUCH_TAP_SPACE);
    CHECK_EQ(Read(LRR_SETTING_TOUCH_TWO_FINGER_TAP, "right click"), LRR_TOUCH_TAP_RIGHT_CLICK);
    CHECK_EQ(Read(LRR_SETTING_TOUCH_TWO_FINGER_TAP, "true"), LRR_TOUCH_TAP_SPACE);
    CHECK_EQ(Read(LRR_SETTING_TOUCH_TWO_FINGER_TAP, "bogus"), LRR_TOUCH_TAP_SPACE);
    CHECK_EQ(Read(LRR_SETTING_TOUCH_TWO_FINGER_TAP, NULL), LRR_TOUCH_TAP_SPACE);
}

/* The typed getters read the settings LRR_SetSetting set. */
void Getters()
{
    LRR_SetSetting(LRR_SETTING_TRAINING_SAVE, "off");
    CHECK_EQ(LRR_GetTrainingSave(), LRR_TRAINING_SAVE_OFF);
    LRR_SetSetting(LRR_SETTING_DISPLAY, "original");
    CHECK_EQ(LRR_GetDisplayScaling(), LRR_DISPLAY_ORIGINAL);
    LRR_SetSetting(LRR_SETTING_CONTROLS, "original");
    CHECK_EQ(LRR_GetControls(), LRR_CONTROLS_ORIGINAL);
    LRR_SetSetting(LRR_SETTING_FIX_IDLENESS_SYNDROME, "false");
    CHECK(!LRR_GetFixIdlenessSyndrome(), "set: the original's job assignment");
    LRR_SetSetting(LRR_SETTING_FPS_OVERLAY, "true");
    CHECK(LRR_GetFpsOverlay(), "set: the frame rate overlay");
    LRR_SetSetting(LRR_SETTING_UNLIMITED_RAIDERS, "true");
    CHECK(LRR_GetUnlimitedRaiders(), "set: unlimited raiders");
    CHECK(!LRR_GetUnlimitedOxygen(), "set: unlimited raiders, apart from unlimited oxygen");
    LRR_SetSetting(LRR_SETTING_UNLIMITED_OXYGEN, "true");
    CHECK(LRR_GetUnlimitedOxygen(), "set: unlimited oxygen");

    /* The display settings, in place of the Mode Selection dialog. */
    LRR_SetSetting(LRR_SETTING_FULLSCREEN, "true");
    LRR_SetSetting(LRR_SETTING_XRES, "800");
    LRR_SetSetting(LRR_SETTING_YRES, "0x258"); /* strtol, base 0: 600 */
    LRR_SetSetting(LRR_SETTING_DEVICE, LRR_DEVICE_SOFTWARE);
    const LRR_DisplayConfig* display = LRR_GetDisplayConfig();
    CHECK(display->fullScreen, "set: full screen");
    CHECK_EQ(display->width, 800);
    CHECK_EQ(display->height, 600);
    CHECK_STREQ(display->device, LRR_DEVICE_SOFTWARE);
    CHECK_EQ(Read(LRR_SETTING_XRES, "bogus"), 0); /* as iniparser_getint */
    LRR_SetSetting(LRR_SETTING_XRES, NULL);
    LRR_SetSetting(LRR_SETTING_DEVICE, NULL);
    CHECK_EQ(LRR_GetDisplayConfig()->width, 640);
    CHECK_STREQ(LRR_GetDisplayConfig()->device, "");

    /* A directory is applied to the file layer, as LRR_LoadConfig does. */
    LRR_SetSetting(LRR_SETTING_SAVEPATH, "saves/");
    CHECK_STREQ(files_native_save_path(), "saves");
    LRR_SetSetting(LRR_SETTING_SAVEPATH, "");
    CHECK(files_native_save_path() == NULL, "set: an empty savepath keeps Data\\Saves");
    LRR_SetSetting(LRR_SETTING_DISKPATH, NULL);
    CHECK(files_native_disk_path() == NULL, "set: no diskpath keeps the working directory");

    /* NULL is the default again. */
    for (int i = 0; i < LRR_SETTING_COUNT; ++i) {
        LRR_SetSetting(static_cast<LRR_Setting>(i), NULL);
    }
    Defaults();
}

} // namespace

int main(void)
{
    Defaults();
    Switches();
    Choices();
    Getters();
    return lrr_test_result("config_settings");
}
