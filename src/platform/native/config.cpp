// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port's configuration file (config.h). Adapted from isle-portable's
 * IsleApp::LoadConfig (ISLE/isleapp.cpp, LGPL-3.0), which replaces Lego
 * Island's registry with isle.ini in the same way. */
#include "platform/native/config.h"

#include <SDL3/SDL.h>
#include <iniparser.h>
#include <miniwin/windows.h>

#include "gods98/files_native.h"

#ifdef __3DS__
#include "platform/native/3ds/config.h"
#endif
#ifdef __SWITCH__
#include "platform/native/switch/config.h"
#endif
#ifdef IOS
#include "platform/native/ios/config.h"
#endif
#ifdef ANDROID
#include "platform/native/android/config.h"
#endif
#ifdef __EMSCRIPTEN__
#include "platform/native/emscripten/config.h"
#include "platform/native/emscripten/filesystem.h"
#endif

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

constexpr LRR_DisplayConfig kDefaultDisplay = {false, 640, 480, "", 1};

LRR_DisplayConfig g_display = kDefaultDisplay;

/* The directory that holds the configuration file and, by default, the
 * saves; "" when the host has none. */
std::string PreferencePath()
{
#ifdef IOS
    const char* documents = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    return documents != NULL ? documents : "";
#elif defined(ANDROID)
    const char* storage = SDL_GetAndroidExternalStoragePath();
    return storage != NULL ? std::string(storage) + "/" : "";
#else
    char* prefPath = SDL_GetPrefPath(LRR_CONFIG_ORG, LRR_CONFIG_APP);
    std::string path = prefPath != NULL ? prefPath : "";
    SDL_free(prefPath);
    return path;
#endif
}

/* The defaults written to a new configuration file. */
dictionary* DefaultConfig(const std::string& prefPath)
{
    dictionary* dict = dictionary_new(0);
    iniparser_set(dict, LRR_CONFIG_SECTION, NULL);
    iniparser_set(dict, LRR_CONFIG_SAVEPATH, prefPath.c_str());
    iniparser_set(dict, LRR_CONFIG_FULLSCREEN, kDefaultDisplay.fullScreen ? "true" : "false");
    iniparser_set(dict, LRR_CONFIG_XRES, std::to_string(kDefaultDisplay.width).c_str());
    iniparser_set(dict, LRR_CONFIG_YRES, std::to_string(kDefaultDisplay.height).c_str());
    iniparser_set(dict, LRR_CONFIG_DEVICE, kDefaultDisplay.device);
    iniparser_set(dict, LRR_CONFIG_WINDOWSCALE, std::to_string(kDefaultDisplay.windowScale).c_str());

#ifdef __3DS__
    N3DS_SetupDefaultConfigOverrides(dict);
#endif
#ifdef __SWITCH__
    NX_SetupDefaultConfigOverrides(dict);
#endif
#ifdef IOS
    IOS_SetupDefaultConfigOverrides(dict);
#endif
#ifdef ANDROID
    Android_SetupDefaultConfigOverrides(dict);
#endif
    return dict;
}

} // namespace

bool LRR_LoadConfig(const char* iniPath)
{
#ifdef __EMSCRIPTEN__
    if (iniPath && !Emscripten_SetupConfig(iniPath)) {
        iniPath = NULL;
    }
#endif
    const std::string prefPath = PreferencePath();

    std::string iniConfig;
    if (iniPath) {
        iniConfig = iniPath;
    }
    else if (!prefPath.empty()) {
        iniConfig = prefPath + LRR_CONFIG_FILE;
    }
    else {
        iniConfig = LRR_CONFIG_FILE;
    }

    SDL_Log("Reading configuration from \"%s\"", iniConfig.c_str());

    dictionary* dict = iniparser_load(iniConfig.c_str());

    // [library:config]
    // Load sane defaults if the dictionary failed to load.
    if (!dict || dict->n == 0) {
        iniparser_freedict(dict);

        if (iniPath) {
#ifdef __EMSCRIPTEN__
            SDL_Log("Invalid config path '%s', falling back to defaults", iniPath);
            iniPath = NULL;
            iniConfig = !prefPath.empty() ? prefPath + LRR_CONFIG_FILE : LRR_CONFIG_FILE;
#else
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid config path '%s'", iniPath);
            return false;
#endif
        }

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading sane defaults");
        FILE* iniFP = fopen(iniConfig.c_str(), "wb");

        if (!iniFP) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to write config at '%s': %s",
                iniConfig.c_str(),
                strerror(errno)
            );
            return false;
        }

        dict = DefaultConfig(prefPath);
        iniparser_dump_ini(dict, iniFP);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "New config written at '%s'", iniConfig.c_str());
        fclose(iniFP);
    }

#ifdef __EMSCRIPTEN__
    Emscripten_SetupDefaultConfigOverrides(dict);
#endif
#ifdef IOS
    // [library:config]
    // iOS relocates both the app bundle and the data container on every app update,
    // so absolute paths stored in the config go stale. Re-derive them on every launch.
    IOS_SetupDefaultConfigOverrides(dict);
#endif

    files_native_set_save_path(iniparser_getstring(dict, LRR_CONFIG_SAVEPATH, prefPath.c_str()));

    g_display = kDefaultDisplay;
    g_display.fullScreen = iniparser_getboolean(dict, LRR_CONFIG_FULLSCREEN, g_display.fullScreen) != 0;
    g_display.width = iniparser_getint(dict, LRR_CONFIG_XRES, g_display.width);
    g_display.height = iniparser_getint(dict, LRR_CONFIG_YRES, g_display.height);
    SDL_strlcpy(g_display.device, iniparser_getstring(dict, LRR_CONFIG_DEVICE, ""), sizeof(g_display.device));
    int windowScale = iniparser_getint(dict, LRR_CONFIG_WINDOWSCALE, g_display.windowScale);
    if (windowScale < 1 || windowScale > LRR_CONFIG_MAX_WINDOW_SCALE) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "Window Scale %d is not between 1 and %d; using 1",
            windowScale,
            LRR_CONFIG_MAX_WINDOW_SCALE
        );
        windowScale = 1;
    }
    g_display.windowScale = windowScale;
    MiniwinSetWindowScale(g_display.windowScale);
    iniparser_freedict(dict);

#ifdef __EMSCRIPTEN__
    Emscripten_SetupFilesystem();
#endif
    return true;
}

const LRR_DisplayConfig* LRR_GetDisplayConfig()
{
    return &g_display;
}
