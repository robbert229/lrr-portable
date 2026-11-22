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
 * and the 3D device. The port starts without the dialog.
 *
 * And it holds the port's opt-outs from its divergences from the original
 * game (docs/divergences.md), in their own section, [tweaks]:
 * tweaks:Training Save. */
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
/* The section for the divergences' switches. */
#define LRR_CONFIG_TWEAKS_SECTION "tweaks"
#define LRR_CONFIG_TRAININGSAVE "tweaks:Training Save"

/* The display settings, in place of the dialog's choices. device is a 3D
 * device's description as DirectDraw.c lists it, e.g. "Miniwin driver
 * (Miniwin Emulation)"; empty selects the platform's default device
 * (LRR_ChooseDevice). The resolution must be one of Init.c's valid modes
 * (640x480, and 800x600 or 1024x768 with -debug); otherwise the first
 * valid mode is used. */
struct LRR_DisplayConfig {
    bool fullScreen;
    int width;
    int height;
    char device[256];
};

/* The 3D devices, by their descriptions as DirectDraw.c lists them. */
#define LRR_DEVICE_SDLGPU "Miniwin driver (SDL3 GPU HAL)"
#define LRR_DEVICE_SOFTWARE "Miniwin driver (Miniwin Emulation)"

/* A 3D device the [library:config] substitution in Init.c can choose:
 * its description as DirectDraw.c lists it, whether it draws the level
 * (miniwin's Miniwin_DeviceDrawsImmediateMode: the level is drawn in
 * immediate mode, and a device without it shows no 3D), and whether setting
 * the display up on it failed. */
struct LRR_DeviceCandidate {
    const char* desc;
    bool drawsLevel;
    bool failed;
};

/* The index of the device to use, in place of the dialog's choice, by
 * capability, never by the device's position in miniwin's list:
 * - `configured` (legorr:3D Device), when it is listed: the file wins, with
 *   a warning when the device cannot draw the level;
 * - else `platformDefault` (LRR_Default3DDevice()), when it is listed and
 *   draws the level;
 * - else the first device that draws the level;
 * - else `platformDefault`, else the first device, so the game still
 *   starts, without 3D, and logs it.
 * A failed device is never chosen; -1 when no device is left. The choice,
 * and each step past `configured` or `platformDefault`, is logged. */
int LRR_ChooseDevice(const LRR_DeviceCandidate* devices, unsigned int count,
                     const char* configured, const char* platformDefault);

/* Marks devices[failed] failed, as the display could not be set up on it,
 * logs it, and chooses again with legorr:3D Device and the platform's
 * default (LRR_ChooseDevice). */
int LRR_ChooseDeviceAfterFailure(LRR_DeviceCandidate* devices, unsigned int count, unsigned int failed);

/* This platform's default 3D device, used when legorr:3D Device is empty
 * or names a device that is not listed or cannot draw the level. SDL GPU
 * where the platform has no override in its config.h (the desktop: Vulkan,
 * Metal or Direct3D 12), falling back to the first device that draws, the
 * software renderer, where SDL GPU cannot create a device. */
const char* LRR_Default3DDevice();

/* tweaks:Training Save (docs/divergences.md, "Save after a training
 * mission"). The original shows no rewards screen, and so no Save button,
 * after a level whose Lego.cfg block has no Reward section (the eight
 * training missions), and keeps their progress only in a slot the Load menu
 * cannot open. "prompt", the default, opens the Save Game screen after such
 * a level is completed; "off" keeps the original's behaviour. */
enum LRR_TrainingSave {
    LRR_TRAINING_SAVE_OFF = 0,
    LRR_TRAINING_SAVE_PROMPT = 1,
};

/* Reads the configuration from `iniPath`, or from LRR_CONFIG_FILE in the
 * preference directory when it is NULL, writing that file with defaults
 * when it is missing or empty, and applies it (files_native_set_save_path,
 * files_native_set_disk_path).
 * False when an explicit `iniPath` does not load, or the defaults cannot
 * be written. */
bool LRR_LoadConfig(const char* iniPath);

/* The display settings LRR_LoadConfig read, or the defaults before it
 * runs: a 640x480 window on the platform's default device. */
const LRR_DisplayConfig* LRR_GetDisplayConfig();

/* The tweaks:Training Save setting LRR_LoadConfig read, or the default,
 * LRR_TRAINING_SAVE_PROMPT, before it runs. */
LRR_TrainingSave LRR_GetTrainingSave();

#endif /* LEGORRR_PLATFORM_NATIVE_CONFIG_H */
