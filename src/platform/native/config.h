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
 * tweaks:Training Save, tweaks:display, tweaks:widescreen, tweaks:controls,
 * tweaks:fix_idleness_syndrome, tweaks:fps_overlay, and the touch controls'
 * tweaks:touch keys and tweaks:back_button.
 *
 * And it holds cheats, off by default, in their own section, [cheats]:
 * cheats:unlimited_raiders and cheats:unlimited_oxygen. */
#ifndef LEGORRR_PLATFORM_NATIVE_CONFIG_H
#define LEGORRR_PLATFORM_NATIVE_CONFIG_H

/* SDL_GetPrefPath's organisation and application: the preference
 * directory, e.g. ~/.local/share/lrr-portable/LegoRR/ on Linux. */
#define LRR_CONFIG_ORG "lrr-portable"
#define LRR_CONFIG_APP "LegoRR"
/* The file in the preference directory, its sections, and the [legorr]
 * keys, which a host's defaults override (<platform>/config.cpp). */
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
/* The section for the cheats. */
#define LRR_CONFIG_CHEATS_SECTION "cheats"

/* The settings: one per key of the file, each one row of the table in
 * config.cpp, which gives its key, its kind, its default and, for a
 * choice, its values' names. The table drives reading the file, writing
 * its defaults, and LRR_GetSetting.
 *
 * A value is read by its setting's kind, in any case:
 * - a switch (false or true): "true", "on", "yes" and "1" are true;
 *   "false", "off", "no", "0" and "original" are false; past those, a value
 *   that starts with y, t or 1 is true and one that starts with n, f or 0
 *   is false, as iniparser_getboolean reads one;
 * - a choice: one of its names, the original's first; "original", "off",
 *   "false", "no" and "0" are the original's; and a choice of two also
 *   takes "true", "on", "yes" and "1" for its second;
 * - a number: as strtol reads it, in base 0;
 * - text, or a directory: as written.
 * Any other value, and a missing key, takes the default. */
enum LRR_Setting {
    LRR_SETTING_SAVEPATH,              /* legorr:savepath, a directory */
    LRR_SETTING_DISKPATH,              /* legorr:diskpath, a directory */
    LRR_SETTING_FULLSCREEN,            /* legorr:Full Screen, a switch */
    LRR_SETTING_XRES,                  /* legorr:Horizontal Resolution */
    LRR_SETTING_YRES,                  /* legorr:Vertical Resolution */
    LRR_SETTING_DEVICE,                /* legorr:3D Device, text */
    LRR_SETTING_TRAINING_SAVE,         /* tweaks:Training Save, a choice */
    LRR_SETTING_DISPLAY,               /* tweaks:display, a choice */
    LRR_SETTING_WIDESCREEN,            /* tweaks:widescreen, a switch */
    LRR_SETTING_CONTROLS,              /* tweaks:controls, a choice */
    LRR_SETTING_FIX_IDLENESS_SYNDROME, /* tweaks:fix_idleness_syndrome */
    LRR_SETTING_FPS_OVERLAY,           /* tweaks:fps_overlay, a switch */
    LRR_SETTING_TOUCH,                 /* tweaks:touch, a switch */
    LRR_SETTING_TOUCH_LONG_PRESS,      /* tweaks:touch_long_press, a choice */
    LRR_SETTING_TOUCH_SCROLL,          /* tweaks:touch_scroll, a switch */
    LRR_SETTING_TOUCH_ZOOM,            /* tweaks:touch_zoom, a switch */
    LRR_SETTING_TOUCH_ROTATE,          /* tweaks:touch_rotate, a switch */
    LRR_SETTING_TOUCH_TWO_FINGER_TAP,  /* tweaks:touch_two_finger_tap */
    LRR_SETTING_TOUCH_THREE_FINGER_TAP, /* tweaks:touch_three_finger_tap */
    LRR_SETTING_TOUCH_KEYBOARD,        /* tweaks:touch_keyboard, a choice */
    LRR_SETTING_BACK_BUTTON,           /* tweaks:back_button, a choice */
    LRR_SETTING_UNLIMITED_RAIDERS,     /* cheats:unlimited_raiders */
    LRR_SETTING_UNLIMITED_OXYGEN,      /* cheats:unlimited_oxygen */
    LRR_SETTING_COUNT
};

/* A setting's value, as LRR_LoadConfig or LRR_SetSetting left it, or its
 * default before either runs: a switch's 0 or 1, a choice's index into its
 * names, a number; 0 for text. Cheap enough to call every tick. */
int LRR_GetSetting(LRR_Setting setting);

/* Sets `setting` in this process as if the file said `value`, read by the
 * setting's kind (NULL: as if the key were missing, so the default), and
 * applies it as LRR_LoadConfig would (a directory reaches
 * files_native_set_save_path or files_native_set_disk_path). The file is
 * neither read nor written, and the next LRR_LoadConfig reads it again.
 * legorr:savepath's default is the save directory LRR_LoadConfig chose,
 * empty before it runs.
 *
 * It overrides a setting for one run: entry.cpp sets legorr:diskpath from
 * --game this way. And it is the tests' seam: a unit reads the setting
 * through its getter below, on every host, without a file. */
void LRR_SetSetting(LRR_Setting setting, const char* value);

/* The display settings, in place of the dialog's choices. device is a 3D
 * device's description as DirectDraw.c lists it, e.g. "Miniwin driver
 * (Miniwin Emulation)"; empty selects the platform's default device
 * (LRR_ChooseDevice). With tweaks:display = fit (the default), the
 * resolution is the window's size when it opens, any size; the game keeps
 * Init.c's first valid mode, 640x480, and the window fits it, unless
 * tweaks:widescreen widens the mode to a window wider than 4:3. With
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

/* tweaks:widescreen (docs/divergences.md, "Widescreen"). The original drew
 * a 4:3 picture. "true", the default, widens the game's mode to the
 * window's aspect when the window is wider than 4:3 (the desktop's in full
 * screen): the 3D view shows more at the sides, the in-level interface
 * keeps to the screen's edges, and the 640x480 screens (the front end, the
 * rewards, loading) are centred. It needs tweaks:display = fit; "false"
 * keeps the 4:3 picture, with bars at the sides. */

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

/* tweaks:fps_overlay (docs/divergences.md, "Frame rate overlay"). The
 * original shows no frame rate. "true" draws the frames a second in the
 * top right corner of the screen; "false", the default, draws nothing. */

/* What a two-finger tap sends (tweaks:touch_two_finger_tap): "space", the
 * default, "right click", or "off". */
enum LRR_TouchTwoFingerTap {
    LRR_TOUCH_TAP_OFF = 0,
    LRR_TOUCH_TAP_SPACE = 1,
    LRR_TOUCH_TAP_RIGHT_CLICK = 2,
};

/* The touch controls (docs/divergences.md, "Touch controls";
 * platform/native/touch_gestures.h). Each key's "off" turns its part off.
 * - tweaks:touch: "true" or "false". True, the port makes the mouse from
 *   finger events with the gestures below, and SDL's own touch-to-mouse
 *   mapping (a touch is the left button, held) is off; false keeps SDL's
 *   mapping. True by default on Android and iOS, false elsewhere: on a
 *   desktop the pointer follows a finger only where the system lets SDL
 *   move it.
 * - tweaks:touch_long_press: "hover" (a finger held still moves the pointer
 *   without pressing, so tooltips show) or "off" (it holds the left
 *   button).
 * - tweaks:touch_scroll, touch_zoom, touch_rotate: "true" or "false", for a
 *   two-finger drag (scrolls the map), a pinch (zooms) and a twist (turns
 *   the view).
 * - tweaks:touch_two_finger_tap: LRR_TouchTwoFingerTap.
 * - tweaks:touch_three_finger_tap: "keyboard" (opens or closes the
 *   on-screen keyboard) or "off".
 * - tweaks:touch_keyboard: "auto" (the on-screen keyboard opens while the
 *   game takes typed text, on hosts that have one) or "off".
 * - tweaks:back_button: "escape" (Android's Back button is Escape) or
 *   "off" (Back is left to the system).
 * The keys other than tweaks:touch work only while it is on, except
 * touch_keyboard and back_button, which work either way. */
struct LRR_TouchConfig {
    bool enabled;
    bool longPressHover;
    bool scroll;
    bool zoom;
    bool rotate;
    LRR_TouchTwoFingerTap twoFingerTap;
    bool threeFingerKeyboard;
    bool keyboard;
    bool backEscape;
};

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

/* Reads the configuration and applies it (files_native_set_save_path,
 * files_native_set_disk_path). First it chooses where the port's files
 * live, the data directory:
 * - `dataPath` (--data, entry.cpp), the one directory a launcher gives for
 *   everything the port writes, made when it is missing; the saves go in its
 *   Saves subdirectory;
 * - else the host's preference directory, which also holds the saves.
 * That save directory is legorr:savepath's default. The file is `iniPath`
 * (--ini), or LRR_CONFIG_FILE in the data directory when it is NULL, which
 * is written with defaults when it is missing or empty. NULL or "" for
 * `dataPath` is the preference directory, as before launchers.
 * False when an explicit `iniPath` does not load, or the defaults cannot
 * be written. */
bool LRR_LoadConfig(const char* iniPath, const char* dataPath = nullptr);

/* The display settings LRR_LoadConfig read, or the defaults before it
 * runs: a 640x480 window on the platform's default device. */
const LRR_DisplayConfig* LRR_GetDisplayConfig();

/* The settings the units read, by their types. Each is its row's value
 * (LRR_GetSetting), so its default is the table's: LRR_TRAINING_SAVE_PROMPT,
 * LRR_DISPLAY_FIT, true (tweaks:widescreen), LRR_CONTROLS_MODERN, true
 * (tweaks:fix_idleness_syndrome), and false for the rest. */
inline LRR_TrainingSave LRR_GetTrainingSave()
{
    return static_cast<LRR_TrainingSave>(LRR_GetSetting(LRR_SETTING_TRAINING_SAVE));
}

inline LRR_DisplayScaling LRR_GetDisplayScaling()
{
    return static_cast<LRR_DisplayScaling>(LRR_GetSetting(LRR_SETTING_DISPLAY));
}

inline bool LRR_GetWidescreen()
{
    return LRR_GetSetting(LRR_SETTING_WIDESCREEN) != 0;
}

inline LRR_Controls LRR_GetControls()
{
    return static_cast<LRR_Controls>(LRR_GetSetting(LRR_SETTING_CONTROLS));
}

inline bool LRR_GetFixIdlenessSyndrome()
{
    return LRR_GetSetting(LRR_SETTING_FIX_IDLENESS_SYNDROME) != 0;
}

inline bool LRR_GetFpsOverlay()
{
    return LRR_GetSetting(LRR_SETTING_FPS_OVERLAY) != 0;
}

/* The touch settings, from their rows (LRR_TouchConfig). */
inline LRR_TouchConfig LRR_GetTouchConfig()
{
    LRR_TouchConfig touch;
    touch.enabled = LRR_GetSetting(LRR_SETTING_TOUCH) != 0;
    touch.longPressHover = LRR_GetSetting(LRR_SETTING_TOUCH_LONG_PRESS) != 0;
    touch.scroll = LRR_GetSetting(LRR_SETTING_TOUCH_SCROLL) != 0;
    touch.zoom = LRR_GetSetting(LRR_SETTING_TOUCH_ZOOM) != 0;
    touch.rotate = LRR_GetSetting(LRR_SETTING_TOUCH_ROTATE) != 0;
    touch.twoFingerTap = static_cast<LRR_TouchTwoFingerTap>(LRR_GetSetting(LRR_SETTING_TOUCH_TWO_FINGER_TAP));
    touch.threeFingerKeyboard = LRR_GetSetting(LRR_SETTING_TOUCH_THREE_FINGER_TAP) != 0;
    touch.keyboard = LRR_GetSetting(LRR_SETTING_TOUCH_KEYBOARD) != 0;
    touch.backEscape = LRR_GetSetting(LRR_SETTING_BACK_BUTTON) != 0;
    return touch;
}

inline bool LRR_GetUnlimitedRaiders()
{
    return LRR_GetSetting(LRR_SETTING_UNLIMITED_RAIDERS) != 0;
}

inline bool LRR_GetUnlimitedOxygen()
{
    return LRR_GetSetting(LRR_SETTING_UNLIMITED_OXYGEN) != 0;
}

#endif /* LEGORRR_PLATFORM_NATIVE_CONFIG_H */
