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

    /* An explicit file that does not load fails, as isle-portable's does. */
    CHECK(!LRR_LoadConfig((root + "/missing.ini").c_str()), "explicit: a missing file fails");

    if (s_failures != 0) {
        printf("config_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("config_test: OK\n");
    return 0;
}
