// SPDX-License-Identifier: LGPL-3.0-or-later
/* Web (Emscripten) configuration defaults. Adapted from isle-portable's
 * ISLE/emscripten/config.cpp (LGPL-3.0). config.cpp applies them on every
 * launch: the save directory is the OPFS mount filesystem.cpp makes. */
#include "platform/native/emscripten/config.h"

#include "platform/native/config.h"
#include "platform/native/emscripten/filesystem.h"

#include <SDL3/SDL_log.h>
#include <iniparser.h>

void Emscripten_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
    SDL_Log("Overriding default config for Emscripten");

    iniparser_set(p_dictionary, LRR_CONFIG_SAVEPATH, Emscripten_savePath);
}
