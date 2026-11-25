// SPDX-License-Identifier: LGPL-3.0-or-later
/* Nintendo 3DS configuration defaults. Adapted from isle-portable's
 * ISLE/3ds/config.cpp (LGPL-3.0). */
#include "platform/native/3ds/config.h"

#include "platform/native/config.h"

#include <SDL3/SDL_log.h>
#include <iniparser.h>

void N3DS_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
    SDL_Log("Overriding default config for 3DS");

    // TODO: Save path: can we use libctru FS save data functions? Would be neat, especially for CIA install
    // Extra / at the end causes some issues
    iniparser_set(p_dictionary, LRR_CONFIG_SAVEPATH, "sdmc:/3ds/LegoRR");
    // The game data (Data and the WADs) beside the saves.
    iniparser_set(p_dictionary, LRR_CONFIG_DISKPATH, "sdmc:/3ds/LegoRR");
    // The 4:3 picture: a wider one draws more of the level each frame.
    iniparser_set(p_dictionary, "tweaks:widescreen", "false");
}
