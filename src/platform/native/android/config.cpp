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
    std::string savedata = std::string(data ? data : "") + "/saves/";

    if (!SDL_GetPathInfo(savedata.c_str(), NULL)) {
        SDL_CreateDirectory(savedata.c_str());
    }

    iniparser_set(p_dictionary, LRR_CONFIG_SAVEPATH, savedata.c_str());
}
