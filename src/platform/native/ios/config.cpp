// SPDX-License-Identifier: LGPL-3.0-or-later
/* iOS configuration defaults. Adapted from isle-portable's
 * ISLE/ios/config.cpp (LGPL-3.0). config.cpp applies them on every launch
 * as well: iOS moves the app's data container on each update, so a stored
 * absolute path goes stale. */
#include "platform/native/ios/config.h"

#include "platform/native/config.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>
#include <iniparser.h>

void IOS_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
    SDL_Log("Overriding default config for iOS");

    // The Documents folder: the user can reach it through the Files app and
    // file sharing, and it is backed up with the device.
    iniparser_set(p_dictionary, LRR_CONFIG_SAVEPATH, SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS));
}
