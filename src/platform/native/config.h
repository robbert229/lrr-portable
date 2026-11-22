// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port's configuration file ([library:config], ADR 0007), as
 * isle-portable's isle.ini: an ini file in the user's preference directory,
 * written with defaults the first time, and per-platform defaults for the
 * hosts whose file layout differs (3DS, Switch, iOS, Android, the web).
 *
 * It holds the save directory, legorr:savepath. The original kept its
 * saves in Data\Saves under the install; the port keeps them in a writable
 * per-user directory unless the file says otherwise. An empty savepath
 * keeps the original's Data\Saves.
 *
 * It holds the data directory, legorr:diskpath, as isle.ini's
 * isle:diskpath: the directory that holds Data and the WADs. The original
 * found them in the working directory, so it only ran when started inside
 * the install; empty (the default on the desktop) keeps that.
 *
 * It also holds the display settings that the original's start-up "Mode
 * Selection" dialog (gods98 Init.c) asked for, as isle.ini holds the ones
 * Lego Island's CONFIG.EXE set: full screen or a window, the resolution,
 * and the 3D device. The port starts without the dialog. */
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
#define LRR_CONFIG_DISKPATH "legorr:diskpath"
#define LRR_CONFIG_FULLSCREEN "legorr:Full Screen"
#define LRR_CONFIG_XRES "legorr:Horizontal Resolution"
#define LRR_CONFIG_YRES "legorr:Vertical Resolution"
#define LRR_CONFIG_DEVICE "legorr:3D Device"

/* The display settings, in place of the dialog's choices. device is a 3D
 * device's description as DirectDraw.c lists it, e.g. "Miniwin driver
 * (Miniwin Emulation)"; empty selects the device the dialog would, the last
 * hardware device. The resolution must be one of Init.c's valid modes
 * (640x480, and 800x600 or 1024x768 with -debug); otherwise the first
 * valid mode is used. */
struct LRR_DisplayConfig {
    bool fullScreen;
    int width;
    int height;
    char device[256];
};

/* Reads the configuration from `iniPath`, or from LRR_CONFIG_FILE in the
 * preference directory when it is NULL, writing that file with defaults
 * when it is missing or empty, and applies it (files_native_set_save_path,
 * files_native_set_disk_path).
 * False when an explicit `iniPath` does not load, or the defaults cannot
 * be written. */
bool LRR_LoadConfig(const char* iniPath);

/* The display settings LRR_LoadConfig read, or the defaults before it
 * runs: a 640x480 window on the dialog's default device. */
const LRR_DisplayConfig* LRR_GetDisplayConfig();

#endif /* LEGORRR_PLATFORM_NATIVE_CONFIG_H */
