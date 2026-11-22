// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port's configuration file (config.h). Adapted from isle-portable's
 * IsleApp::LoadConfig (ISLE/isleapp.cpp, LGPL-3.0), which replaces Lego
 * Island's registry with isle.ini in the same way. */
#include "platform/native/config.h"

#include <SDL3/SDL.h>
#include <iniparser.h>

#include "gods98/files_native.h"

#ifdef __3DS__
#include "platform/native/3ds/config.h"
#endif
#ifdef __SWITCH__
#include "platform/native/switch/config.h"
#endif
#ifdef IOS
#include "platform/native/ios/config.h"
#endif
#ifdef ANDROID
#include "platform/native/android/config.h"
#endif
#ifdef __EMSCRIPTEN__
#include "platform/native/emscripten/config.h"
#include "platform/native/emscripten/filesystem.h"
#endif

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

/* How a setting's value is read and written (config.h, LRR_Setting). */
enum class Kind {
    Switch,    /* false or true, written "false" or "true" */
    Choice,    /* one of its names, the original's first */
    Number,    /* an int */
    Text,      /* a string */
    Directory, /* a string, applied to the file layer */
};

/* One key of the file. */
struct Row {
    LRR_Setting setting;        /* its position in the table */
    const char* key;            /* iniparser's "section:key" */
    Kind kind;
    int number;                 /* a switch's, choice's or number's default */
    const char* text;           /* a text's or directory's default; NULL is
                                 * the save directory */
    const char* const* names;   /* a choice's names, NULL-terminated */
    void (*apply)(const char*); /* where a directory is applied */
};

constexpr Row Switch(LRR_Setting setting, const char* key, bool value)
{
    return {setting, key, Kind::Switch, value ? 1 : 0, NULL, NULL, NULL};
}

constexpr Row Choice(LRR_Setting setting, const char* key, const char* const* names, int value)
{
    return {setting, key, Kind::Choice, value, NULL, names, NULL};
}

constexpr Row Number(LRR_Setting setting, const char* key, int value)
{
    return {setting, key, Kind::Number, value, NULL, NULL, NULL};
}

constexpr Row Text(LRR_Setting setting, const char* key, const char* value)
{
    return {setting, key, Kind::Text, 0, value, NULL, NULL};
}

constexpr Row Directory(LRR_Setting setting, const char* key, const char* value, void (*apply)(const char*))
{
    return {setting, key, Kind::Directory, 0, value, NULL, apply};
}

/* The save directory LRR_LoadConfig chose, as a directory's default. */
constexpr const char* kSaveDirectory = NULL;

constexpr const char* kTrainingSaveNames[] = {"off", "prompt", NULL};
constexpr const char* kDisplayNames[] = {"original", "fit", NULL};
constexpr const char* kControlsNames[] = {"original", "modern", NULL};
constexpr const char* kLongPressNames[] = {"off", "hover", NULL};
constexpr const char* kTwoFingerTapNames[] = {"off", "space", "right click", NULL};
constexpr const char* kThreeFingerTapNames[] = {"off", "keyboard", NULL};
constexpr const char* kTouchKeyboardNames[] = {"off", "auto", NULL};
constexpr const char* kBackButtonNames[] = {"off", "escape", NULL};

/* The touch controls are on by default where the screen is the only
 * pointer (LRR_TouchConfig). */
#if defined(ANDROID) || defined(IOS)
constexpr bool kDefaultTouch = true;
#else
constexpr bool kDefaultTouch = false;
#endif

/* The settings, in the order they are written to a new file. A choice's
 * names are indexed by its enum (config.h): the original's is 0. */
constexpr Row kTable[] = {
    /* The original found its saves in Data\Saves and its data in the
     * working directory; a host overrides both (DefaultConfig). */
    Directory(LRR_SETTING_SAVEPATH, LRR_CONFIG_SAVEPATH, kSaveDirectory, files_native_set_save_path),
    Directory(LRR_SETTING_DISKPATH, LRR_CONFIG_DISKPATH, "", files_native_set_disk_path),
    /* The Mode Selection dialog's choices. */
    Switch(LRR_SETTING_FULLSCREEN, LRR_CONFIG_FULLSCREEN, false),
    Number(LRR_SETTING_XRES, LRR_CONFIG_XRES, 640),
    Number(LRR_SETTING_YRES, LRR_CONFIG_YRES, 480),
    Text(LRR_SETTING_DEVICE, LRR_CONFIG_DEVICE, ""),
    /* The divergences' switches (docs/divergences.md). */
    Choice(LRR_SETTING_TRAINING_SAVE, "tweaks:Training Save", kTrainingSaveNames, LRR_TRAINING_SAVE_PROMPT),
    Choice(LRR_SETTING_DISPLAY, "tweaks:display", kDisplayNames, LRR_DISPLAY_FIT),
    Choice(LRR_SETTING_CONTROLS, "tweaks:controls", kControlsNames, LRR_CONTROLS_MODERN),
    Switch(LRR_SETTING_FIX_IDLENESS_SYNDROME, "tweaks:fix_idleness_syndrome", true),
    Switch(LRR_SETTING_FPS_OVERLAY, "tweaks:fps_overlay", false),
    /* The touch controls (LRR_TouchConfig). */
    Switch(LRR_SETTING_TOUCH, "tweaks:touch", kDefaultTouch),
    Choice(LRR_SETTING_TOUCH_LONG_PRESS, "tweaks:touch_long_press", kLongPressNames, 1),
    Switch(LRR_SETTING_TOUCH_SCROLL, "tweaks:touch_scroll", true),
    Switch(LRR_SETTING_TOUCH_ZOOM, "tweaks:touch_zoom", true),
    Switch(LRR_SETTING_TOUCH_ROTATE, "tweaks:touch_rotate", true),
    Choice(LRR_SETTING_TOUCH_TWO_FINGER_TAP, "tweaks:touch_two_finger_tap", kTwoFingerTapNames,
           LRR_TOUCH_TAP_SPACE),
    Choice(LRR_SETTING_TOUCH_THREE_FINGER_TAP, "tweaks:touch_three_finger_tap", kThreeFingerTapNames, 1),
    Choice(LRR_SETTING_TOUCH_KEYBOARD, "tweaks:touch_keyboard", kTouchKeyboardNames, 1),
    Choice(LRR_SETTING_BACK_BUTTON, "tweaks:back_button", kBackButtonNames, 1),
    /* The cheats, off by default. */
    Switch(LRR_SETTING_UNLIMITED_RAIDERS, "cheats:unlimited_raiders", false),
    Switch(LRR_SETTING_UNLIMITED_OXYGEN, "cheats:unlimited_oxygen", false),
};

constexpr bool TableInOrder()
{
    for (int i = 0; i < LRR_SETTING_COUNT; ++i) {
        if (kTable[i].setting != i) {
            return false;
        }
    }
    return true;
}

static_assert(sizeof(kTable) / sizeof(kTable[0]) == LRR_SETTING_COUNT, "one row per LRR_Setting");
static_assert(TableInOrder(), "the rows in LRR_Setting's order");

/* The spellings of false (and of a choice's original) and of true. */
constexpr const char* kFalseWords[] = {"false", "off", "no", "0", "original", NULL};
constexpr const char* kTrueWords[] = {"true", "on", "yes", "1", NULL};

/* The index of `value` in `words`, in any case, or -1. */
int Find(const char* const* words, const char* value)
{
    for (int i = 0; words[i] != NULL; ++i) {
        if (SDL_strcasecmp(value, words[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int ParseSwitch(const char* value, int fallback)
{
    if (Find(kTrueWords, value) >= 0) {
        return 1;
    }
    if (Find(kFalseWords, value) >= 0) {
        return 0;
    }
    /* iniparser_getboolean's reading, which legorr:Full Screen had. */
    switch (value[0]) {
    case 'y': case 'Y': case 't': case 'T': case '1':
        return 1;
    case 'n': case 'N': case 'f': case 'F': case '0':
        return 0;
    default:
        return fallback;
    }
}

int ParseChoice(const char* const* names, const char* value, int fallback)
{
    int index = Find(names, value);

    if (index >= 0) {
        return index;
    }
    if (Find(kFalseWords, value) >= 0) {
        return 0;
    }
    if (names[0] != NULL && names[1] != NULL && names[2] == NULL && Find(kTrueWords, value) >= 0) {
        return 1;
    }
    return fallback;
}

/* A setting's value. */
struct Value {
    int number;
    std::string text;
};

/* The save directory LRR_LoadConfig chose: legorr:savepath's default. */
std::string g_saveDir;

/* The values, each its default until it is read or set. */
struct Values {
    Value value[LRR_SETTING_COUNT];

    Values()
    {
        for (const Row& row : kTable) {
            value[row.setting].number = row.number;
            value[row.setting].text = row.text != NULL ? row.text : "";
        }
    }
};

Value* GetValues()
{
    static Values values;
    return values.value;
}

/* The text a row's default is written as. */
std::string DefaultText(const Row& row)
{
    switch (row.kind) {
    case Kind::Switch:
        return row.number != 0 ? "true" : "false";
    case Kind::Choice:
        return row.names[row.number];
    case Kind::Number:
        return std::to_string(row.number);
    case Kind::Text:
    case Kind::Directory:
        return row.text != NULL ? row.text : g_saveDir;
    }
    return "";
}

/* Reads `text` (NULL: the key is missing) into the row's value, and applies
 * it. */
void Set(const Row& row, const char* text)
{
    Value& value = GetValues()[row.setting];

    if (text == NULL) {
        value.number = row.number;
        if (row.kind == Kind::Text || row.kind == Kind::Directory) {
            value.text = DefaultText(row);
        }
    }
    else {
        switch (row.kind) {
        case Kind::Switch:
            value.number = ParseSwitch(text, row.number);
            break;
        case Kind::Choice:
            value.number = ParseChoice(row.names, text, row.number);
            break;
        case Kind::Number:
            value.number = (int)strtol(text, NULL, 0);
            break;
        case Kind::Text:
        case Kind::Directory:
            value.text = text;
            break;
        }
    }
    if (row.apply != NULL) {
        row.apply(value.text.c_str());
    }
}

/* The host's directory for the configuration file and the saves; "" when
 * the host has none. */
std::string PreferencePath()
{
#ifdef IOS
    const char* documents = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    return documents != NULL ? documents : "";
#elif defined(ANDROID)
    const char* storage = SDL_GetAndroidExternalStoragePath();
    return storage != NULL ? std::string(storage) + "/" : "";
#else
    char* prefPath = SDL_GetPrefPath(LRR_CONFIG_ORG, LRR_CONFIG_APP);
    std::string path = prefPath != NULL ? prefPath : "";
    SDL_free(prefPath);
    return path;
#endif
}

/* The data directory (config.h, LRR_LoadConfig): --data, made here as
 * SDL_GetPrefPath makes the preference directory, or the preference
 * directory. */
std::string DataDirectory(const char* dataPath)
{
    if (dataPath == NULL || dataPath[0] == '\0') {
        return PreferencePath();
    }
    std::string path = dataPath;
    if (path.back() != '/' && path.back() != '\\') {
        path += '/';
    }
    SDL_CreateDirectory(path.c_str());
    return path;
}

/* The defaults written to a new configuration file. */
dictionary* DefaultConfig()
{
    dictionary* dict = dictionary_new(0);
    std::string section;

    for (const Row& row : kTable) {
        const char* colon = SDL_strchr(row.key, ':');
        if (section.compare(0, std::string::npos, row.key, colon - row.key) != 0) {
            section.assign(row.key, colon - row.key);
            iniparser_set(dict, section.c_str(), NULL);
        }
        iniparser_set(dict, row.key, DefaultText(row).c_str());
    }

#ifdef __3DS__
    N3DS_SetupDefaultConfigOverrides(dict);
#endif
#ifdef __SWITCH__
    NX_SetupDefaultConfigOverrides(dict);
#endif
#ifdef IOS
    IOS_SetupDefaultConfigOverrides(dict);
#endif
#ifdef ANDROID
    Android_SetupDefaultConfigOverrides(dict);
#endif
    return dict;
}

bool Listed(const LRR_DeviceCandidate& device, const char* desc)
{
    return desc != NULL && desc[0] != '\0' && SDL_strcmp(device.desc, desc) == 0;
}

/* The index of the listed, unfailed device named `desc` that draws the
 * level (or need not), or -1. */
int FindDevice(const LRR_DeviceCandidate* devices, unsigned int count, const char* desc, bool mustDraw)
{
    for (unsigned int i = 0; i < count; ++i) {
        if (!devices[i].failed && Listed(devices[i], desc) && (devices[i].drawsLevel || !mustDraw)) {
            return (int)i;
        }
    }
    return -1;
}

/* Why the device named `desc` cannot be chosen, for the log. */
const char* WhyUnusable(const LRR_DeviceCandidate* devices, unsigned int count, const char* desc)
{
    for (unsigned int i = 0; i < count; ++i) {
        if (Listed(devices[i], desc)) {
            return devices[i].failed ? "failed" : "cannot draw the level";
        }
    }
    return "is not available";
}

} // namespace

int LRR_ChooseDevice(const LRR_DeviceCandidate* devices, unsigned int count,
                     const char* configured, const char* platformDefault)
{
    int index;

    if (configured != NULL && configured[0] != '\0') {
        index = FindDevice(devices, count, configured, false);
        if (index >= 0) {
            if (devices[index].drawsLevel) {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Using 3D device \"%s\" (legorr:3D Device)",
                            devices[index].desc);
            }
            else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Using 3D device \"%s\" (legorr:3D Device), which cannot draw the level: no 3D",
                            devices[index].desc);
            }
            return index;
        }
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "3D device \"%s\" %s; choosing another", configured,
                    WhyUnusable(devices, count, configured));
    }

    index = FindDevice(devices, count, platformDefault, true);
    if (index >= 0) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Using 3D device \"%s\" (the default)", devices[index].desc);
        return index;
    }
    if (platformDefault != NULL && platformDefault[0] != '\0') {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Default 3D device \"%s\" %s", platformDefault,
                    WhyUnusable(devices, count, platformDefault));
    }

    for (unsigned int i = 0; i < count; ++i) {
        if (!devices[i].failed && devices[i].drawsLevel) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Using 3D device \"%s\"", devices[i].desc);
            return (int)i;
        }
    }

    index = FindDevice(devices, count, platformDefault, false);
    for (unsigned int i = 0; index < 0 && i < count; ++i) {
        if (!devices[i].failed) {
            index = (int)i;
        }
    }
    if (index >= 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "No 3D device can draw the level; using \"%s\", which shows no 3D", devices[index].desc);
    }
    return index;
}

int LRR_ChooseDeviceAfterFailure(LRR_DeviceCandidate* devices, unsigned int count, unsigned int failed)
{
    if (failed >= count) {
        return -1;
    }
    devices[failed].failed = true;
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot set the display up on 3D device \"%s\"", devices[failed].desc);
    return LRR_ChooseDevice(devices, count, LRR_GetDisplayConfig()->device, LRR_Default3DDevice());
}

const char* LRR_Default3DDevice()
{
#if defined(__3DS__)
    return N3DS_DEFAULT_3D_DEVICE;
#elif defined(__SWITCH__)
    return NX_DEFAULT_3D_DEVICE;
#elif defined(IOS)
    return IOS_DEFAULT_3D_DEVICE;
#elif defined(ANDROID)
    return ANDROID_DEFAULT_3D_DEVICE;
#elif defined(__EMSCRIPTEN__)
    return EMSCRIPTEN_DEFAULT_3D_DEVICE;
#else
    return LRR_DEVICE_SDLGPU;
#endif
}

bool LRR_UseOpenGLWindow(const char* configured, const char* platformDefault)
{
    const char* device = configured != NULL && configured[0] != '\0' ? configured : platformDefault;
    return device != NULL && SDL_strstr(device, "OpenGL") != NULL;
}

bool LRR_LoadConfig(const char* iniPath, const char* dataPath)
{
#ifdef __EMSCRIPTEN__
    if (iniPath && !Emscripten_SetupConfig(iniPath)) {
        iniPath = NULL;
    }
#endif
    /* Where the files live, before what the file says. */
    const std::string dataDir = DataDirectory(dataPath);
    const bool launched = dataPath != NULL && dataPath[0] != '\0';
    g_saveDir = launched ? dataDir + "Saves/" : dataDir;

    std::string iniConfig;
    if (iniPath) {
        iniConfig = iniPath;
    }
    else if (!dataDir.empty()) {
        iniConfig = dataDir + LRR_CONFIG_FILE;
    }
    else {
        iniConfig = LRR_CONFIG_FILE;
    }

    SDL_Log("Reading configuration from \"%s\"", iniConfig.c_str());

    dictionary* dict = iniparser_load(iniConfig.c_str());

    // [library:config]
    // Load sane defaults if the dictionary failed to load.
    if (!dict || dict->n == 0) {
        iniparser_freedict(dict);

        if (iniPath) {
#ifdef __EMSCRIPTEN__
            SDL_Log("Invalid config path '%s', falling back to defaults", iniPath);
            iniPath = NULL;
            iniConfig = !dataDir.empty() ? dataDir + LRR_CONFIG_FILE : LRR_CONFIG_FILE;
#else
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid config path '%s'", iniPath);
            return false;
#endif
        }

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading sane defaults");
        FILE* iniFP = fopen(iniConfig.c_str(), "wb");

        if (!iniFP) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to write config at '%s': %s",
                iniConfig.c_str(),
                strerror(errno)
            );
            return false;
        }

        dict = DefaultConfig();
        iniparser_dump_ini(dict, iniFP);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "New config written at '%s'", iniConfig.c_str());
        fclose(iniFP);
    }

#ifdef __EMSCRIPTEN__
    Emscripten_SetupDefaultConfigOverrides(dict);
#endif
#ifdef IOS
    // [library:config]
    // iOS relocates both the app bundle and the data container on every app update,
    // so absolute paths stored in the config go stale. Re-derive them on every launch.
    IOS_SetupDefaultConfigOverrides(dict);
#endif

    for (const Row& row : kTable) {
        Set(row, iniparser_getstring(dict, row.key, NULL));
    }
    iniparser_freedict(dict);

#ifdef __EMSCRIPTEN__
    Emscripten_SetupFilesystem();
#endif
    return true;
}

int LRR_GetSetting(LRR_Setting setting)
{
    return setting >= 0 && setting < LRR_SETTING_COUNT ? GetValues()[setting].number : 0;
}

void LRR_SetSetting(LRR_Setting setting, const char* value)
{
    if (setting >= 0 && setting < LRR_SETTING_COUNT) {
        Set(kTable[setting], value);
    }
}

const LRR_DisplayConfig* LRR_GetDisplayConfig()
{
    static LRR_DisplayConfig display;
    const Value* values = GetValues();

    display.fullScreen = values[LRR_SETTING_FULLSCREEN].number != 0;
    display.width = values[LRR_SETTING_XRES].number;
    display.height = values[LRR_SETTING_YRES].number;
    SDL_strlcpy(display.device, values[LRR_SETTING_DEVICE].text.c_str(), sizeof(display.device));
#ifdef __3DS__
    /* A 3DS has no windows, and a windowed display takes the window's size,
     * the screen's 320x240: the display is full screen, at the configured
     * mode (640x480), which the Citro3D renderer fits onto the bottom
     * screen. */
    display.fullScreen = true;
#endif
    return &display;
}
