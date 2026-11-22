// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web build's filesystem (filesystem.h). Adapted from isle-portable's
 * ISLE/emscripten/filesystem.cpp (LGPL-3.0): the configuration and save
 * parts. isle-portable also streams the game's data files over a WasmFS
 * fetch backend; the port does not do that yet. */
#include "platform/native/emscripten/filesystem.h"

#include "gods98/files_native.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <emscripten.h>
#include <emscripten/wasmfs.h>

#include <string>

static backend_t opfs = nullptr;

/* Makes each directory along `p_path` on the OPFS backend. */
static void Emscripten_CreateOPFSDirectories(std::string p_path)
{
    char* parse = &p_path[0];
    while ((parse = SDL_strchr(++parse, '/'))) {
        *parse = '\0';
        wasmfs_create_directory(p_path.c_str(), 0644, opfs);
        *parse = '/';
    }
}

bool Emscripten_OPFSDisabled()
{
    return MAIN_THREAD_EM_ASM_INT({return !!Module["disableOpfs"]});
}

bool Emscripten_SetupConfig(const char* p_iniConfig)
{
    if (Emscripten_OPFSDisabled()) {
        SDL_Log("OPFS is disabled; ignoring .ini path");
        return false;
    }

    opfs = wasmfs_create_opfs_backend();
    Emscripten_CreateOPFSDirectories(p_iniConfig);
    return true;
}

void Emscripten_SetupFilesystem()
{
    const char* savePath = files_native_save_path();

    if (savePath && *savePath && !Emscripten_OPFSDisabled()) {
        if (!opfs) {
            opfs = wasmfs_create_opfs_backend();
        }

        std::string path = savePath;
        if (path[path.length() - 1] != '/') {
            path += "/";
        }

        Emscripten_CreateOPFSDirectories(path);
    }
}
