// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_config — the port's configuration file (platform/native/config.h),
 * as isle-portable's isle.ini: written with defaults in the preference
 * directory the first time, read from there or from an explicit path, and
 * applied to the save directory, the data directory and the display settings. XDG_DATA_HOME points the preference
 * directory into the fixture. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "gods98/files_native.h"
#include "platform/native/config.h"

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
    char cwd[1024];
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "rm -rf %s", FIX);
    if (system(cmd) != 0) return 1;
    mkdir(FIX, 0755);
    if (chdir(FIX) != 0 || getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("FAIL: chdir %s\n", FIX);
        return 1;
    }
    const std::string root = cwd;
    setenv("XDG_DATA_HOME", (root + "/xdg").c_str(), 1);
    const std::string prefDir = root + "/xdg/" LRR_CONFIG_ORG "/" LRR_CONFIG_APP;
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

    /* An explicit file that does not load fails, as isle-portable's does. */
    CHECK(!LRR_LoadConfig((root + "/missing.ini").c_str()), "explicit: a missing file fails");

    if (s_failures != 0) {
        printf("config_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("config_test: OK\n");
    return 0;
}
