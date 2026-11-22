// SPDX-License-Identifier: LGPL-3.0-or-later
/* Android configuration defaults. Adapted from isle-portable's
 * ISLE/android/config.cpp (LGPL-3.0). */
#include "platform/native/android/config.h"

#include "platform/native/config.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_system.h>
#include <iniparser.h>

#include <string>

void Android_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
    SDL_Log("Overriding default config for Android");

    const char* data = SDL_GetAndroidExternalStoragePath();
    const std::string storage = data ? data : "";
    std::string savedata = storage + "/saves/";

    if (!SDL_GetPathInfo(savedata.c_str(), NULL)) {
        SDL_CreateDirectory(savedata.c_str());
    }

    iniparser_set(p_dictionary, LRR_CONFIG_SAVEPATH, savedata.c_str());
    // The game data (Data and the WADs) in the external storage directory;
    // without one, the working directory.
    if (!storage.empty()) {
        iniparser_set(p_dictionary, LRR_CONFIG_DISKPATH, (storage + "/").c_str());
    }
}
