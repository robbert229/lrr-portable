// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web build's filesystem, as isle-portable's
 * ISLE/emscripten/filesystem.h: the configuration file and the saves live
 * in the browser's Origin Private File System (OPFS), so they survive a
 * reload. The page can turn OPFS off with Module.disableOpfs. */
#ifndef LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_FILESYSTEM_H
#define LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_FILESYSTEM_H

inline static const char* Emscripten_savePath = "/save";
/* The data directory (legorr:diskpath): where the game data (Data and the
 * WADs) is mounted, as isle-portable's "/LEGO". */
inline static const char* Emscripten_dataPath = "/LegoRR";

bool Emscripten_OPFSDisabled();
/* Makes the OPFS directories that hold an explicit configuration file. */
bool Emscripten_SetupConfig(const char* p_iniConfig);
/* Mounts the save directory (files_native_save_path) on OPFS. */
void Emscripten_SetupFilesystem();

#endif /* LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_FILESYSTEM_H */
