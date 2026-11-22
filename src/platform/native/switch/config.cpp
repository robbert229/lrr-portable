// SPDX-License-Identifier: LGPL-3.0-or-later
/* Nintendo Switch configuration defaults. Adapted from isle-portable's
 * ISLE/switch/config.cpp (LGPL-3.0). */
#include "platform/native/switch/config.h"

#include "platform/native/config.h"

#include <SDL3/SDL_log.h>
#include <iniparser.h>

void NX_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
    SDL_Log("Overriding default config for Switch");

    iniparser_set(p_dictionary, LRR_CONFIG_SAVEPATH, "sdmc:/switch/LegoRR/");
    iniparser_set(p_dictionary, LRR_CONFIG_DISKPATH, "sdmc:/switch/LegoRR/");
}
