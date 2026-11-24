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
 * tweaks:Training Save, tweaks:display, tweaks:controls and
 * tweaks:fix_idleness_syndrome.
 *
 * And it holds cheats, off by default, in their own section, [cheats]:
 * cheats:unlimited_raiders and cheats:unlimited_oxygen. */
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
#define LRR_CONFIG_DISPLAY "tweaks:display"
#define LRR_CONFIG_CONTROLS "tweaks:controls"
#define LRR_CONFIG_FIXIDLENESS "tweaks:fix_idleness_syndrome"
/* The section for the cheats. */
#define LRR_CONFIG_CHEATS_SECTION "cheats"
#define LRR_CONFIG_UNLIMITEDRAIDERS "cheats:unlimited_raiders"
#define LRR_CONFIG_UNLIMITEDOXYGEN "cheats:unlimited_oxygen"

/* The display settings, in place of the dialog's choices. device is a 3D
 * device's description as DirectDraw.c lists it, e.g. "Miniwin driver
 * (Miniwin Emulation)"; empty selects the platform's default device
 * (LRR_ChooseDevice). With tweaks:display = fit (the default), the
 * resolution is the window's size when it opens, any size; the game keeps
 * Init.c's first valid mode, 640x480, and the window fits it. With
 * tweaks:display = original, the resolution must be one of Init.c's valid
 * modes (640x480, and 800x600 or 1024x768 with -debug), and the window has
 * the mode's size; otherwise the first valid mode is used. Full screen
 * fills the desktop either way. */
struct LRR_DisplayConfig {
    bool fullScreen;
    int width;
    int height;
    char device[256];
};

/* The 3D devices, by their descriptions as DirectDraw.c lists them. */
#define LRR_DEVICE_SDLGPU "Miniwin driver (SDL3 GPU HAL)"
#define LRR_DEVICE_SOFTWARE "Miniwin driver (Miniwin Emulation)"
#define LRR_DEVICE_GLES3 "Miniwin driver (OpenGL ES 3.0 HAL)"

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

/* Whether the main window must be an OpenGL window (miniwin's
 * Miniwin_SetOpenGLWindows), without which miniwin lists no OpenGL device.
 * The window is created before the devices are listed, so this is decided
 * from the device the game will most likely choose: `configured`
 * (legorr:3D Device) when it is set, else `platformDefault`. It is true
 * when that device's description names OpenGL. An OpenGL window is never
 * asked for otherwise: on it SDL GPU may not be listed (Android), and on
 * the web the canvas would be taken for WebGL. */
bool LRR_UseOpenGLWindow(const char* configured, const char* platformDefault);

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

/* tweaks:display (docs/divergences.md, "A resizable window, and the 3D view
 * at the window's resolution"). The original drew a fixed 640x480 picture.
 * "fit", the default, makes the window resizable, opens it at the
 * configured resolution, and draws the 3D view at the window's resolution,
 * with the 640x480 interface scaled to fit; "original" keeps a fixed window
 * of the mode's size and draws the whole picture at the mode's resolution,
 * scaled to fit only in full screen. */
enum LRR_DisplayScaling {
    LRR_DISPLAY_ORIGINAL = 0,
    LRR_DISPLAY_FIT = 1,
};

/* tweaks:controls (docs/divergences.md, "Keyboard and mouse camera
 * controls"). The original pans the top-down view only when the cursor
 * touches the edge of the screen. "modern" adds, in the top-down view:
 * W, A, S and D to pan, Q and E to rotate, the mouse wheel to zoom and a
 * middle-button drag to pan; edge panning still works. While debug keys
 * are on (Lego.cfg AllowDebugKeys), W, A, S, D, Q and E keep their debug
 * meanings and do not move the camera. "modern" is the default;
 * "original" keeps the original's controls. */
enum LRR_Controls {
    LRR_CONTROLS_ORIGINAL = 0,
    LRR_CONTROLS_MODERN = 1,
};

/* tweaks:fix_idleness_syndrome (docs/divergences.md, "Idle raiders take
 * the jobs waiting for them"). In the original, AITask.c's job assignment
 * can leave idle units without work while jobs wait: a job with a unit on
 * the way takes the nearest idle unit each tick and gives it nothing, and
 * one unit that cannot reach a job stops every other unit from taking it.
 * "true", the default, fixes both; "false" keeps the original's
 * behaviour. */

/* cheats:unlimited_raiders (docs/divergences.md, "Cheat: unlimited
 * raiders"). The original greys out the Teleport Rock Raider button, and
 * its teleporters hold back queued raiders, once the level's raiders would
 * use more oxygen than its Support Stations give (Lego.cfg OxygenCoef,
 * LegoObject.c 0043c200). "true" skips that check, so raiders can always
 * be teleported in from any teleporter that takes them;
 * "false", the default, keeps the original's limit. */

/* cheats:unlimited_oxygen (docs/divergences.md, "Cheat: unlimited
 * oxygen"). In the original, a level's air drains while its raiders
 * breathe more than its Support Stations make (Lego.c 00424560), and the
 * level is lost when the air runs out. "true" stops the air draining, so
 * it stays at the full 100 the level starts with; "false", the default,
 * keeps the original's air supply. */

/* The launch options a launcher passes (--game and --data, entry.cpp), for
 * the next LRR_LoadConfig; NULL or "" leaves an option unset, and without
 * either the configuration is read and written exactly as before.
 *
 * `gamePath` is the directory that holds the WADs and Data, in place of
 * legorr:diskpath for this run (the file is not changed).
 *
 * `dataPath` is the one directory for everything the port writes: the
 * configuration file, unless --ini names another, is LegoRR.ini inside it,
 * and the saves go in its Saves subdirectory unless the file's savepath
 * names another place. The directory is made when it is missing. */
void LRR_SetLaunchOptions(const char* gamePath, const char* dataPath);

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

/* The tweaks:display setting LRR_LoadConfig read, or the default,
 * LRR_DISPLAY_FIT, before it runs. */
LRR_DisplayScaling LRR_GetDisplayScaling();

/* The tweaks:controls setting LRR_LoadConfig read, or the default,
 * LRR_CONTROLS_MODERN, before it runs. */
LRR_Controls LRR_GetControls();

/* The tweaks:fix_idleness_syndrome setting LRR_LoadConfig read, or the
 * default, true, before it runs. */
bool LRR_GetFixIdlenessSyndrome();

/* The cheats:unlimited_raiders setting LRR_LoadConfig read, or the
 * default, false, before it runs. */
bool LRR_GetUnlimitedRaiders();

/* The cheats:unlimited_oxygen setting LRR_LoadConfig read, or the
 * default, false, before it runs. */
bool LRR_GetUnlimitedOxygen();

#endif /* LEGORRR_PLATFORM_NATIVE_CONFIG_H */
