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

/* The host the game data is streamed from: the LRR_EMSCRIPTEN_HOST build
 * setting, as isle-portable's ISLE_EMSCRIPTEN_HOST. The data directory is
 * at <host>/LegoRR, which holds the WADs, Data and the manifest
 * (manifest.h). Empty is the page's own origin. */
#ifndef LRR_EMSCRIPTEN_HOST
#define LRR_EMSCRIPTEN_HOST ""
#endif
inline static const char* Emscripten_streamHost = LRR_EMSCRIPTEN_HOST;

/* Whether the WADs are fetched in full at start-up, with progress events
 * for the page (the LRR_EMSCRIPTEN_PRELOAD_WADS build setting). */
#ifndef LRR_EMSCRIPTEN_PRELOAD_WADS
#define LRR_EMSCRIPTEN_PRELOAD_WADS 1
#endif

/* The fetch backend's chunk size, the LRR_EMSCRIPTEN_CHUNK_SIZE build
 * setting. A file of at most twice this, or from a host that sends no
 * "Accept-Ranges: bytes", is fetched whole on first use; a larger one is
 * fetched in chunks of this size with range requests, as the WADs are. */
#ifndef LRR_EMSCRIPTEN_CHUNK_SIZE
#define LRR_EMSCRIPTEN_CHUNK_SIZE (512 * 1024)
#endif

bool Emscripten_OPFSDisabled();
/* Makes the OPFS directories that hold an explicit configuration file. */
bool Emscripten_SetupConfig(const char* p_iniConfig);
/* Mounts the save directory (files_native_save_path) on OPFS, and the game
 * data (Emscripten_dataPath) on the fetch backend. */
void Emscripten_SetupFilesystem();

#endif /* LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_FILESYSTEM_H */
