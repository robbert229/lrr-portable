// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_config — the port's configuration file (platform/native/config.h),
 * as isle-portable's isle.ini: written with defaults in the preference
 * directory the first time, read from there or from an explicit path, and
 * applied to the save directory, the data directory and the display settings. XDG_DATA_HOME, or
 * CFFIXED_USER_HOME on macOS, points the preference directory into the
 * fixture. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include <SDL3/SDL.h>

#include "gods98/files_native.h"
#include "platform/native/config.h"
#include "platform/native/test_fixture.h"

static const char* FIX = "config_fixture";
static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static std::string read_text(const std::string& path)
{
    std::string text;
    FILE* fp = fopen(path.c_str(), "rb");
    char buf[256];
    size_t n;

    if (fp == NULL) return text;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) text.append(buf, n);
    fclose(fp);
    return text;
}

static void write_text(const std::string& path, const std::string& text)
{
    FILE* fp = fopen(path.c_str(), "wb");
    fputs(text.c_str(), fp);
    fclose(fp);
}

static std::string save_path(void)
{
    const char* path = files_native_save_path();
    return path != NULL ? path : "";
}

static std::string disk_path(void)
{
    const char* path = files_native_disk_path();
    return path != NULL ? path : "";
}

int main(void)
{
    if (!fixture_enter(FIX)) {
        printf("FAIL: chdir %s\n", FIX);
        return 1;
    }
    const std::string root = fixture_cwd();
    /* The preference directory, inside the fixture: XDG_DATA_HOME on
     * Linux and the BSDs, CoreFoundation's home override on macOS. A host
     * that keeps it elsewhere (Windows: the user's AppData) is skipped, so
     * a run never touches the user's own LegoRR.ini. */
    fixture_setenv("XDG_DATA_HOME", root + "/xdg");
    fixture_setenv("CFFIXED_USER_HOME", root + "/home");
    fixture_mkdir(root + "/home");
    char* pref = SDL_GetPrefPath(LRR_CONFIG_ORG, LRR_CONFIG_APP);
    std::string prefDir = pref != NULL ? pref : "";
    SDL_free(pref);
    while (prefDir.size() > 1 && prefDir.back() == '/') prefDir.pop_back();
    if (prefDir.compare(0, root.size() + 1, root + "/") != 0) {
        printf("SKIP: the preference directory \"%s\" is outside the fixture\n", prefDir.c_str());
        return 77;
    }
    const std::string defaultIni = prefDir + "/" LRR_CONFIG_FILE;

    /* First run: the defaults are written to the preference directory, and
     * the saves go there. */
    CHECK(LRR_LoadConfig(NULL), "default: loads");
    std::string text = read_text(defaultIni);
    CHECK(!text.empty(), "default: the file is written in the preference directory");
    CHECK(text.find("[" LRR_CONFIG_SECTION "]") != std::string::npos, "default: its section");
    CHECK(text.find("savepath") != std::string::npos &&
              text.find(prefDir) != std::string::npos,
          "default: savepath is the preference directory");
    CHECK(save_path() == prefDir, "default: the save directory is the preference directory");
    /* iniparser writes its keys in lower case, and reads them in any. */
    CHECK(text.find("full screen") != std::string::npos &&
              text.find("horizontal resolution") != std::string::npos &&
              text.find("vertical resolution") != std::string::npos &&
              text.find("3d device") != std::string::npos,
          "default: the display settings are written");
    CHECK(text.find("diskpath") != std::string::npos, "default: diskpath is written");
    CHECK(files_native_disk_path() == NULL, "default: the data is read from the working directory");
    const LRR_DisplayConfig* display = LRR_GetDisplayConfig();
    CHECK(!display->fullScreen && display->width == 640 && display->height == 480 &&
              display->device[0] == '\0',
          "default: a 640x480 window on the default device");
    CHECK(text.find("[" LRR_CONFIG_TWEAKS_SECTION "]") != std::string::npos,
          "default: the [tweaks] section is written");
    CHECK(text.find("training save") > text.find("[" LRR_CONFIG_TWEAKS_SECTION "]") &&
              text.find("prompt") != std::string::npos,
          "default: training save = prompt is written under [tweaks]");
    CHECK(LRR_GetTrainingSave() == LRR_TRAINING_SAVE_PROMPT, "default: training save prompts");
    CHECK(text.find("display") > text.find("[" LRR_CONFIG_TWEAKS_SECTION "]") &&
              text.find("fit") != std::string::npos,
          "default: display = fit is written under [tweaks]");
    CHECK(LRR_GetDisplayScaling() == LRR_DISPLAY_FIT, "default: the display fits the window");
    CHECK(text.find("controls") > text.find("[" LRR_CONFIG_TWEAKS_SECTION "]") &&
              text.find("modern") != std::string::npos,
          "default: controls = modern is written under [tweaks]");
    CHECK(LRR_GetControls() == LRR_CONTROLS_MODERN, "default: the modern controls");
    CHECK(text.find("fix_idleness_syndrome") > text.find("[" LRR_CONFIG_TWEAKS_SECTION "]") &&
              text.find("true", text.find("fix_idleness_syndrome")) != std::string::npos,
          "default: fix_idleness_syndrome = true is written under [tweaks]");
    CHECK(LRR_GetFixIdlenessSyndrome(), "default: the idleness syndrome is fixed");
    CHECK(text.find("[" LRR_CONFIG_CHEATS_SECTION "]") != std::string::npos &&
              text.find("unlimited_raiders") > text.find("[" LRR_CONFIG_CHEATS_SECTION "]") &&
              text.find("false", text.find("unlimited_raiders")) != std::string::npos,
          "default: unlimited_raiders = false is written under [cheats]");
    CHECK(!LRR_GetUnlimitedRaiders(), "default: the raider limit is kept");
    CHECK(text.find("unlimited_oxygen") > text.find("[" LRR_CONFIG_CHEATS_SECTION "]") &&
              text.find("false", text.find("unlimited_oxygen")) != std::string::npos,
          "default: unlimited_oxygen = false is written under [cheats]");
    CHECK(!LRR_GetUnlimitedOxygen(), "default: the air drains");
    CHECK(text.find("fps_overlay") > text.find("[" LRR_CONFIG_TWEAKS_SECTION "]") &&
              text.find("false", text.find("fps_overlay")) != std::string::npos,
          "default: fps_overlay = false is written under [tweaks]");
    CHECK(!LRR_GetFpsOverlay(), "default: no frame rate overlay");
    CHECK(text.find("touch_two_finger_tap") > text.find("[" LRR_CONFIG_TWEAKS_SECTION "]") &&
              text.find("touch_two_finger_tap") < text.find("[" LRR_CONFIG_CHEATS_SECTION "]") &&
              text.find("back_button") != std::string::npos,
          "default: the touch keys are written under [tweaks]");
    LRR_TouchConfig touch = LRR_GetTouchConfig();
    CHECK(touch.longPressHover && touch.scroll && touch.zoom && touch.rotate &&
              touch.twoFingerTap == LRR_TOUCH_TAP_SPACE && touch.threeFingerKeyboard && touch.keyboard &&
              touch.backEscape,
          "default: every touch gesture is on");
#if defined(ANDROID) || defined(IOS)
    CHECK(touch.enabled, "default: touch controls on, on a touch host");
#else
    CHECK(!touch.enabled, "default: touch controls off, on a desktop");
#endif

    /* A later run reads the file the user edited. */
    write_text(defaultIni, "[" LRR_CONFIG_SECTION "]\nsavepath = " + root + "/elsewhere/\n");
    CHECK(LRR_LoadConfig(NULL), "edited: loads");
    CHECK(save_path() == root + "/elsewhere", "edited: the save directory follows the file");
    CHECK(read_text(defaultIni).find("elsewhere") != std::string::npos,
          "edited: the file is not rewritten");

    /* An empty file is rewritten with the defaults. */
    write_text(defaultIni, "");
    CHECK(LRR_LoadConfig(NULL) && save_path() == prefDir, "empty: the defaults again");

    /* --ini <path>. */
    const std::string custom = root + "/custom.ini";
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nsavepath = " + root + "/custom-saves\n");
    CHECK(LRR_LoadConfig(custom.c_str()), "explicit: loads");
    CHECK(save_path() == root + "/custom-saves", "explicit: its savepath");

    /* An empty savepath keeps the original's Data\Saves. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nsavepath =\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && files_native_save_path() == NULL,
          "explicit: an empty savepath keeps Data\\Saves");

    /* A file without the key takes the preference directory. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nother = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && save_path() == prefDir,
          "explicit: no savepath takes the preference directory");

    /* diskpath names the directory that holds Data and the WADs. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\ndiskpath = " + root + "/install/\n");
    CHECK(LRR_LoadConfig(custom.c_str()), "diskpath: loads");
    CHECK(disk_path() == root + "/install", "diskpath: the data directory follows the file");

    /* A file without it (or with it empty) keeps the working directory. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nother = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && files_native_disk_path() == NULL,
          "diskpath: none keeps the working directory");

    /* The display settings, in place of the Mode Selection dialog. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\n"
                       "Full Screen = true\n"
                       "Horizontal Resolution = 800\n"
                       "Vertical Resolution = 600\n"
                       "3D Device = Miniwin driver (Miniwin Emulation)\n");
    CHECK(LRR_LoadConfig(custom.c_str()), "display: loads");
    display = LRR_GetDisplayConfig();
    CHECK(display->fullScreen, "display: full screen");
    CHECK(display->width == 800 && display->height == 600, "display: the resolution");
    CHECK(strcmp(display->device, "Miniwin driver (Miniwin Emulation)") == 0, "display: the device");

    /* A file without them takes the defaults again. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nother = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()), "display defaults: loads");
    display = LRR_GetDisplayConfig();
    CHECK(!display->fullScreen && display->width == 640 && display->height == 480 &&
              display->device[0] == '\0',
          "display defaults: a 640x480 window on the default device");

    CHECK(LRR_GetTrainingSave() == LRR_TRAINING_SAVE_PROMPT,
          "training save: a file without the key prompts");

    /* tweaks:Training Save: "off" keeps the original; anything else prompts. */
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nTraining Save = off\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetTrainingSave() == LRR_TRAINING_SAVE_OFF,
          "training save: off");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nTraining Save = OFF\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetTrainingSave() == LRR_TRAINING_SAVE_OFF,
          "training save: off in any case");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nTraining Save = prompt\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetTrainingSave() == LRR_TRAINING_SAVE_PROMPT,
          "training save: prompt");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nTraining Save = bogus\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetTrainingSave() == LRR_TRAINING_SAVE_PROMPT,
          "training save: an unknown value prompts");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nTraining Save = off\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetTrainingSave() == LRR_TRAINING_SAVE_PROMPT,
          "training save: the key is read from [tweaks], not [legorr]");

    /* tweaks:display: "original" keeps the original; anything else fits. */
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ndisplay = original\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetDisplayScaling() == LRR_DISPLAY_ORIGINAL,
          "display: original");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ndisplay = Original\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetDisplayScaling() == LRR_DISPLAY_ORIGINAL,
          "display: original in any case");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ndisplay = fit\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetDisplayScaling() == LRR_DISPLAY_FIT, "display: fit");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ndisplay = bogus\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetDisplayScaling() == LRR_DISPLAY_FIT,
          "display: an unknown value fits");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\ndisplay = original\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetDisplayScaling() == LRR_DISPLAY_FIT,
          "display: the key is read from [tweaks], not [legorr]");

    /* tweaks:controls: "original" keeps the original's controls; anything
     * else adds the camera controls. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nother = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetControls() == LRR_CONTROLS_MODERN,
          "controls: a file without the key is modern");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ncontrols = modern\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetControls() == LRR_CONTROLS_MODERN, "controls: modern");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ncontrols = Original\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetControls() == LRR_CONTROLS_ORIGINAL,
          "controls: original in any case");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ncontrols = original\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetControls() == LRR_CONTROLS_ORIGINAL,
          "controls: original");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ncontrols = bogus\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetControls() == LRR_CONTROLS_MODERN,
          "controls: an unknown value is modern");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\ncontrols = original\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetControls() == LRR_CONTROLS_MODERN,
          "controls: the key is read from [tweaks], not [legorr]");

    /* tweaks:fix_idleness_syndrome: "false" keeps the original's job
     * assignment; anything else fixes it. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nother = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetFixIdlenessSyndrome(),
          "fix_idleness_syndrome: a file without the key fixes it");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfix_idleness_syndrome = false\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetFixIdlenessSyndrome(), "fix_idleness_syndrome: false");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfix_idleness_syndrome = Off\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetFixIdlenessSyndrome(),
          "fix_idleness_syndrome: off in any case");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfix_idleness_syndrome = 0\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetFixIdlenessSyndrome(), "fix_idleness_syndrome: 0");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfix_idleness_syndrome = true\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetFixIdlenessSyndrome(), "fix_idleness_syndrome: true");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nfix_idleness_syndrome = false\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetFixIdlenessSyndrome(),
          "fix_idleness_syndrome: the key is read from [tweaks], not [legorr]");

    /* cheats:unlimited_raiders: "true" lifts the raider limit; anything
     * else keeps it. */
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_raiders = true\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetUnlimitedRaiders(), "unlimited_raiders: true");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nother = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetUnlimitedRaiders(),
          "unlimited_raiders: a file without the key keeps the limit");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_raiders = On\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetUnlimitedRaiders(), "unlimited_raiders: on in any case");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_raiders = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetUnlimitedRaiders(), "unlimited_raiders: 1");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_raiders = false\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetUnlimitedRaiders(), "unlimited_raiders: false");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_raiders = bogus\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetUnlimitedRaiders(),
          "unlimited_raiders: an unknown value keeps the limit");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nunlimited_raiders = true\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetUnlimitedRaiders(),
          "unlimited_raiders: the key is read from [cheats], not [tweaks]");

    /* cheats:unlimited_oxygen: "true" stops the air draining; anything
     * else keeps the original's air supply. */
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_oxygen = true\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetUnlimitedOxygen() && !LRR_GetUnlimitedRaiders(),
          "unlimited_oxygen: true, apart from unlimited_raiders");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nother = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetUnlimitedOxygen(),
          "unlimited_oxygen: a file without the key keeps the air supply");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_oxygen = Yes\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetUnlimitedOxygen(), "unlimited_oxygen: yes in any case");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_oxygen = false\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetUnlimitedOxygen(), "unlimited_oxygen: false");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_oxygen = bogus\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetUnlimitedOxygen(),
          "unlimited_oxygen: an unknown value keeps the air supply");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nunlimited_oxygen = true\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetUnlimitedOxygen(),
          "unlimited_oxygen: the key is read from [cheats], not [tweaks]");

    /* --game, which a launcher passes: entry.cpp sets legorr:diskpath for
     * the run (LRR_SetSetting), over the file's, which is not changed. */
    write_text(custom, "[" LRR_CONFIG_SECTION "]\ndiskpath = " + root + "/install/\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && disk_path() == root + "/install", "--game: the file's diskpath");
    LRR_SetSetting(LRR_SETTING_DISKPATH, (root + "/launcher-game").c_str());
    CHECK(disk_path() == root + "/launcher-game", "--game: the folder replaces diskpath");
    CHECK(read_text(custom).find("install") != std::string::npos &&
              read_text(custom).find("launcher-game") == std::string::npos,
          "--game: the file is not rewritten");

    /* --data is the one directory: the file is made inside it, the saves go
     * in its Saves, and a file that exists is read from there. */
    const std::string dataDir = root + "/launcher-data";
    CHECK(LRR_LoadConfig(NULL, dataDir.c_str()), "--data: loads");
    CHECK(fixture_is_directory(dataDir), "--data: the folder is made");
    text = read_text(dataDir + "/" LRR_CONFIG_FILE);
    CHECK(!text.empty() && text.find("[" LRR_CONFIG_SECTION "]") != std::string::npos,
          "--data: the defaults are written inside it");
    CHECK(text.find("Saves") != std::string::npos, "--data: savepath is written as its Saves folder");
    CHECK(save_path() == dataDir + "/Saves", "--data: the saves go in its Saves folder");
    CHECK(files_native_disk_path() == NULL,
          "--data: the data is still read from the working directory, and --game lasted one load");
    write_text(dataDir + "/" LRR_CONFIG_FILE, "[" LRR_CONFIG_TWEAKS_SECTION "]\ncontrols = original\n");
    CHECK(LRR_LoadConfig(NULL, dataDir.c_str()) && LRR_GetControls() == LRR_CONTROLS_ORIGINAL,
          "--data: the file inside it is read");
    CHECK(save_path() == dataDir + "/Saves", "--data: a file without savepath keeps the saves inside it");
    CHECK(LRR_LoadConfig(custom.c_str(), dataDir.c_str()) && save_path() == dataDir + "/Saves",
          "--data: --ini names another file, and a missing savepath still follows --data");

    /* Both, a trailing slash, and neither again. */
    CHECK(LRR_LoadConfig(NULL, (dataDir + "/").c_str()), "--data: a trailing slash");
    LRR_SetSetting(LRR_SETTING_DISKPATH, (root + "/launcher-game").c_str());
    CHECK(save_path() == dataDir + "/Saves" && disk_path() == root + "/launcher-game",
          "--game and --data: both apply");
    CHECK(LRR_LoadConfig(NULL) && save_path() == prefDir && files_native_disk_path() == NULL,
          "no launch options: the preference directory again");

    /* tweaks:fps_overlay: "true" draws the frame rate; anything else
     * draws nothing. */
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfps_overlay = true\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetFpsOverlay(), "fps_overlay: true");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfps_overlay = On\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetFpsOverlay(), "fps_overlay: on in any case");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfps_overlay = false\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetFpsOverlay(), "fps_overlay: false");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfps_overlay = bogus\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetFpsOverlay(), "fps_overlay: an unknown value draws nothing");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nother = 1\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetFpsOverlay(), "fps_overlay: a file without the key draws nothing");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nfps_overlay = true\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetFpsOverlay(), "fps_overlay: the key is read from [tweaks], not [cheats]");
    /* The touch keys: "off" (or false) turns each part off. */
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ntouch = on\ntouch_long_press = off\n"
                       "touch_scroll = off\ntouch_zoom = no\ntouch_rotate = 0\n"
                       "touch_two_finger_tap = Right Click\ntouch_three_finger_tap = off\n"
                       "touch_keyboard = off\nback_button = off\n");
    CHECK(LRR_LoadConfig(custom.c_str()), "touch: loads");
    touch = LRR_GetTouchConfig();
    CHECK(touch.enabled && !touch.longPressHover && !touch.scroll && !touch.zoom && !touch.rotate &&
              touch.twoFingerTap == LRR_TOUCH_TAP_RIGHT_CLICK && !touch.threeFingerKeyboard && !touch.keyboard &&
              !touch.backEscape,
          "touch: each key turns its part off");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\ntouch = false\ntouch_two_finger_tap = off\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetTouchConfig().enabled &&
              LRR_GetTouchConfig().twoFingerTap == LRR_TOUCH_TAP_OFF,
          "touch: off, and the two-finger tap off");
    /* One reading per kind of value (config.h, LRR_Setting), so a key
     * takes the spellings of its kind: these were each read otherwise
     * before the settings shared one table. */
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nTraining Save = original\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetTrainingSave() == LRR_TRAINING_SAVE_OFF,
          "training save: original is off");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nFull Screen = on\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetDisplayConfig()->fullScreen, "full screen: on");
    write_text(custom, "[" LRR_CONFIG_SECTION "]\nFull Screen = Yes\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetDisplayConfig()->fullScreen, "full screen: yes, as before");
    write_text(custom, "[" LRR_CONFIG_TWEAKS_SECTION "]\nfix_idleness_syndrome = n\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && !LRR_GetFixIdlenessSyndrome(), "fix_idleness_syndrome: n");
    write_text(custom, "[" LRR_CONFIG_CHEATS_SECTION "]\nunlimited_oxygen = y\nunlimited_raiders = T\n");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetUnlimitedOxygen() && LRR_GetUnlimitedRaiders(),
          "cheats: y and t");

    /* A setting set in-process (LRR_SetSetting) is read by its getter,
     * and the next load reads the file again. */
    LRR_SetSetting(LRR_SETTING_UNLIMITED_OXYGEN, "false");
    CHECK(!LRR_GetUnlimitedOxygen(), "set: in-process");
    CHECK(LRR_LoadConfig(custom.c_str()) && LRR_GetUnlimitedOxygen(), "set: the file again");
    LRR_SetSetting(LRR_SETTING_SAVEPATH, NULL);
    CHECK(save_path() == prefDir, "set: savepath's default is the save directory, the preference directory's");

    /* An explicit file that does not load fails, as isle-portable's does. */
    CHECK(!LRR_LoadConfig((root + "/missing.ini").c_str()), "explicit: a missing file fails");

    if (s_failures != 0) {
        printf("config_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("config_test: OK\n");
    return 0;
}
