// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web build's filesystem, as isle-portable's
 * ISLE/emscripten/filesystem.h: the configuration file and the saves live
 * in the browser's Origin Private File System (OPFS), so they survive a
 * reload (the page can turn OPFS off with Module.disableOpfs), and the game
 * data is streamed from a host over a WasmFS fetch backend. */
#ifndef LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_FILESYSTEM_H
#define LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_FILESYSTEM_H

inline static const char* Emscripten_savePath = "/save";
/* The data directory (legorr:diskpath): where the game data (Data and the
 * WADs) is mounted, as isle-portable's "/LEGO". */
inline static const char* Emscripten_dataPath = "/LegoRR";

/* The game data is streamed from <LRR_EMSCRIPTEN_HOST>/LegoRR, a build
 * setting (filesystem.cpp); the data directory holds the WADs, Data and the
 * manifest (manifest.h). */

bool Emscripten_OPFSDisabled();
/* Makes the OPFS directories that hold an explicit configuration file. */
bool Emscripten_SetupConfig(const char* p_iniConfig);
/* Mounts the save directory (files_native_save_path) on OPFS, and the game
 * data (Emscripten_dataPath) on the fetch backend. */
void Emscripten_SetupFilesystem();

#endif /* LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_FILESYSTEM_H */
