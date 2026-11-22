// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port's configuration file ([library:config], ADR 0007), as
 * isle-portable's isle.ini: an ini file in the user's preference directory,
 * written with defaults the first time, and per-platform defaults for the
 * hosts whose file layout differs (3DS, Switch, iOS, Android, the web).
 *
 * It holds the save directory for now: legorr:savepath. The original kept
 * its saves in Data\Saves under the install; the port keeps them in a
 * writable per-user directory unless the file says otherwise. An empty
 * savepath keeps the original's Data\Saves. */
#ifndef LEGORRR_PLATFORM_NATIVE_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_CONFIG_H

/* SDL_GetPrefPath's organisation and application: the preference
 * directory, e.g. ~/.local/share/lrr-portable/LegoRR/ on Linux. */
#define LRR_CONFIG_ORG "lrr-portable"
#define LRR_CONFIG_APP "LegoRR"
/* The file in the preference directory, and its keys. */
#define LRR_CONFIG_FILE "LegoRR.ini"
#define LRR_CONFIG_SECTION "legorr"
#define LRR_CONFIG_SAVEPATH "legorr:savepath"

/* Reads the configuration from `iniPath`, or from LRR_CONFIG_FILE in the
 * preference directory when it is NULL, writing that file with defaults
 * when it is missing or empty, and applies it (files_native_set_save_path).
 * False when an explicit `iniPath` does not load, or the defaults cannot
 * be written. */
bool LRR_LoadConfig(const char* iniPath);

#endif /* LEGORRR_PLATFORM_NATIVE_CONFIG_H */
