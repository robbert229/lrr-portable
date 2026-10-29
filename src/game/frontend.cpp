// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — front end / main menu (finding 23). See frontend.hpp.
 *
 * Reading the menu groups follows the shipped chip query shape exactly:
 * "<root> Menu::<name> <Menu%i> <key>" with the `::` group separator
 * (DAT_004aab18 — block.cpp tokenizes both ' ' and "::"). The level
 * pickers graft the StartLevel / TutorialStartLevel + NextLevel chains
 * (LevelChainBuild @ 0x4154b0 semantics) onto the picker sub-menus; each
 * row carries the level's FrontEndX/Y scatter position (the config's
 * per-level cavern coords) so the picker looks like a map.
 *
 * Actions (frontend_tick) — keyboard-first for this milestone:
 *   Enter on a picker row        -> FE_EVT_START_LEVEL (pending_level set)
 *   Enter on a Trigger, flag!=0  -> contextual action (quit/back/confirm)
 *   Enter on a Next to "Menu%i"  -> navigate submenus
 *   Esc on a submenu             -> back to the root submenu
 *   Esc on the root submenu      -> quit
 *   Slider/Cycle                 -> adjust the option (writes into fe->opts
 *                                  when attached; issue #85)
 *   TextInput                    -> display-only (parsed, not editable)
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "frontend.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/input.hpp"
#include "gods98/crt.hpp"
#include "util.hpp"
#include "select.hpp"
#include "gods98/view.hpp"
#include "gods98/string.hpp"

#if defined(LEGORRR_BACKEND_WINAPI)
#pragma data_seg(".data$zzzzzz")
/* The first table and the menu globals follow the reference data layout. */
__declspec(allocate(".data$zzzzzz")) static unsigned char
    s_menu_action_table_gap[0x3aad0] = { 0 };
#pragma data_seg()
#define MENU_ACTION_TABLE_ALLOC __declspec(allocate(".data$zzzzzz"))
#else
#define MENU_ACTION_TABLE_ALLOC
#endif
// The corpus keeps this 74-pointer label table in the runtime-filled band at
// 0x4dced0 (zero-init at image load), so it takes no // GLOBAL annotation:
// static initializers against a zero-filled corpus tail break reccmp-datacmp.
MENU_ACTION_TABLE_ALLOC static const char* const s_interface_menu_action_labels[] = {
    "Interface_MenuItem_BackToDefault",
    "Interface_MenuItem_TeleportMan",
    "Interface_MenuItem_BuildBuilding",
    "Interface_MenuItem_BuildSmallVehicle",
    "Interface_MenuItem_BuildLargeVehicle",
    "Interface_MenuItem_LayPath",
    "Interface_MenuItem_RepairLava",
    "Interface_MenuItem_GeologistTest",
    "Interface_MenuItem_ClearRubble",
    "Interface_MenuItem_Dam",
    "Interface_MenuItem_SelectMan",
    "Interface_MenuItem_DeleteMan",
    "Interface_MenuItem_DeleteVehicle",
    "Interface_MenuItem_DeleteBuilding",
    "Interface_MenuItem_Attack",
    "Interface_MenuItem_SelectVehicle",
    "Interface_MenuItem_UnLoadVehicle",
    "Interface_MenuItem_UnLoadMinifigure",
    "Interface_MenuItem_VehiclePickUp",
    "Interface_MenuItem_MinifigurePickUp",
    "Interface_MenuItem_GetOut",
    "Interface_MenuItem_GetIn",
    "Interface_MenuItem_GotoDock",
    "Interface_MenuItem_Dig",
    "Interface_MenuItem_Reinforce",
    "Interface_MenuItem_Dynamite",
    "Interface_MenuItem_UpgradeMan",
    "Interface_MenuItem_GoFeed",
    "Interface_MenuItem_GotoFirstPerson",
    "Interface_MenuItem_GotoSecondPerson",
    "Interface_MenuItem_TrainSkill",
    "Interface_MenuItem_PowerOn",
    "Interface_MenuItem_PowerOff",
    "Interface_MenuItem_UpgradeBuilding",
    "Interface_MenuItem_Repair",
    "Interface_MenuItem_SelectBuilding",
    "Interface_MenuItem_GotoTopView",
    "Interface_MenuItem_GetTool",
    "Interface_MenuItem_GetDrill",
    "Interface_MenuItem_GetSpade",
    "Interface_MenuItem_GetHammer",
    "Interface_MenuItem_GetSpanner",
    "Interface_MenuItem_TrainDriver",
    "Interface_MenuItem_TrainEngineer",
    "Interface_MenuItem_TrainGeologist",
    "Interface_MenuItem_TrainPilot",
    "Interface_MenuItem_TrainSailor",
    "Interface_MenuItem_TrainDynamite",
    "Interface_MenuItem_Build",
    "Interface_MenuItem_PlaceFence",
    "Interface_MenuItem_MakeTeleporterPrimary",
    "Interface_MenuItem_DeselectDig",
    "Interface_MenuItem_ClearSelection",
    "Interface_MenuItem_Encyclopedia",
    "Interface_MenuItem_EjectCrystal",
    "Interface_MenuItem_EjectOre",
    "Interface_MenuItem_TrackObject",
    "Interface_MenuItem_LegoManGoto",
    "Interface_MenuItem_LegoManDig",
    "Interface_MenuItem_VehicleGoto",
    "Interface_MenuItem_VehicleDig",
    "Interface_MenuItem_RemovePath",
    "Interface_MenuItem_UpgradeVehicle",
    "Interface_MenuItem_UpgradeEngine",
    "Interface_MenuItem_UpgardeDrill",
    "Interface_MenuItem_UpgardeScan",
    "Interface_MenuItem_UpgardeCarry",
    "Interface_MenuItem_DeleteElectricFence",
    "Interface_MenuItem_GetLaser",
    "Interface_MenuItem_GetPusherGun",
    "Interface_MenuItem_GetFreezerGun",
    "Interface_MenuItem_GetBirdScarer",
    "Interface_MenuItem_DropBirdScarer",
    "Interface_MenuItem_CancelConstruction",
};
#undef MENU_ACTION_TABLE_ALLOC
#if defined(LEGORRR_BACKEND_WINAPI)
#pragma data_seg(".data$zzzzzz")
__declspec(allocate(".data$zzzzzz")) static unsigned char
    s_reference_menu_data_pad0[0x79144] = { 0 };
__declspec(allocate(".data$zzzzzz")) static unsigned char
    s_menu_global_bytes[0x13c0] = { 0 };
#pragma data_seg()
#define s_menu_global_words ((uint32_t*)s_menu_global_bytes)
/* main_menu_text_item (FUN_004103d0) reads these three dwords individually;
 * dedicated annotated statics carry them (whole-object resolution — the
 * s_menu_global_bytes band cannot hold the annotation, same precedent as the
 * s_fe_flow_* carve-outs). Own storage, zero-init like the corpus's tail. */
// GLOBAL: LEGORR 0x0055613c
static void* DAT_0055613c;
// GLOBAL: LEGORR 0x00556f50
static void* DAT_00556f50;
// GLOBAL: LEGORR 0x00556f54
static void* DAT_00556f54;
#define s_fe_flow (s_menu_global_words + 0x3b7)
/* The five flow words land as own storage: the corpus addresses them as
 * individual dwords, and Reccmp only resolves whole-object operands (the
 * s_menu_global_bytes band cannot carry the annotation). */
// GLOBAL: LEGORR 0x0055701c
static int s_fe_flow_quit;
// GLOBAL: LEGORR 0x00557020
static int s_fe_flow_save_slot;
// GLOBAL: LEGORR 0x00557024
static int s_fe_flow_ingame;
// GLOBAL: LEGORR 0x00557048
static int s_fe_flow_level_start;
// GLOBAL: LEGORR 0x0055704c
static int s_fe_flow_menu_options;
#define FE_FLOW_LEVEL_START_SLOT 11
#define FE_FLOW_MENU_OPTIONS_SLOT 12
#define FE_FLOW_QUIT_WORD       s_fe_flow_quit
#define FE_FLOW_SAVE_SLOT       s_fe_flow_save_slot
#define FE_FLOW_INGAME_SLOT     s_fe_flow_ingame
#define FE_FLOW_LEVEL_START     s_fe_flow_level_start
#define FE_FLOW_MENU_OPTIONS    s_fe_flow_menu_options
#else
static void* DAT_0055613c;
static void* DAT_00556f50;
static void* DAT_00556f54;
static int s_fe_flow[5];
#define FE_FLOW_LEVEL_START_SLOT 3
#define FE_FLOW_MENU_OPTIONS_SLOT 4
#define FE_FLOW_QUIT_WORD       s_fe_flow[0]
#define FE_FLOW_SAVE_SLOT       s_fe_flow[1]
#define FE_FLOW_INGAME_SLOT     s_fe_flow[2]
#define FE_FLOW_LEVEL_START     s_fe_flow[FE_FLOW_LEVEL_START_SLOT]
#define FE_FLOW_MENU_OPTIONS    s_fe_flow[FE_FLOW_MENU_OPTIONS_SLOT]
#endif
/* The wipe quad lands as own storage: the corpus addresses each word as
 * an individual dword (objdump-verified 0x5574ec/f0/f4/f8) and Reccmp
 * only resolves whole-object operands (the s_menu_global_bytes band
 * cannot carry the annotation, and its winapi indices were word-shifted). */
// GLOBAL: LEGORR 0x005574ec
static uint32_t s_menu_wipe_arm;
// GLOBAL: LEGORR 0x005574f0
static uint32_t s_menu_wipe;
// GLOBAL: LEGORR 0x005574f4
static uint32_t s_menu_wipe_stop;
// GLOBAL: LEGORR 0x005574f8
static float s_menu_wipe_clock;
static float s_menu_plane_pan[2]; /* DAT_005574dc/e0 roles */
static int s_main_menu_text_item_pending;
static TextureSurface* s_interface_message_tab_info_text;
static int s_back_button_width;
static int fe_sav_menu_list_from_config(FrontEnd* fe, int list_id,
                                        const char* start_level);
static int s_back_button_height;
static TextureSurface* s_back_button_highlight;
static TextureSurface* s_back_button_pressed;
static char s_back_button_action[FE_NAME_MAX];
static FeSurroundImage s_interface_surround_images[FE_SURROUND_IMAGES_MAX];
static FeInterfaceImage s_interface_images[FE_INTERFACE_ACTION_COUNT];
static FeInterfaceBuildImage* s_interface_build_images[2];
static uint32_t s_interface_build_image_counts[2];
typedef struct FeKeyboardNameEntry {
    uint8_t code;
    const char* name;
} FeKeyboardNameEntry;
static const FeKeyboardNameEntry s_keyboard_name_entries[] = {
    { 1, "KEY_ESCAPE" },
    { 2, "KEY_ONE" },
    { 3, "KEY_TWO" },
    { 4, "KEY_THREE" },
    { 5, "KEY_FOUR" },
    { 6, "KEY_FIVE" },
    { 7, "KEY_SIX" },
    { 8, "KEY_SEVEN" },
    { 9, "KEY_EIGHT" },
    { 10, "KEY_NINE" },
    { 11, "KEY_ZERO" },
    { 12, "KEY_MINUS" },
    { 13, "KEY_EQUALS" },
    { 14, "KEY_BACKSPACE" },
    { 15, "KEY_TAB" },
    { 16, "KEY_Q" },
    { 17, "KEY_W" },
    { 18, "KEY_E" },
    { 19, "KEY_R" },
    { 20, "KEY_T" },
    { 21, "KEY_Y" },
    { 22, "KEY_U" },
    { 23, "KEY_I" },
    { 24, "KEY_O" },
    { 25, "KEY_P" },
    { 26, "KEY_LEFTBRACE" },
    { 27, "KEY_RIGHTBRACE" },
    { 28, "KEY_RETURN" },
    { 29, "KEY_LEFTCTRL" },
    { 30, "KEY_A" },
    { 31, "KEY_S" },
    { 32, "KEY_D" },
    { 33, "KEY_F" },
    { 34, "KEY_G" },
    { 35, "KEY_H" },
    { 36, "KEY_J" },
    { 37, "KEY_K" },
    { 38, "KEY_L" },
    { 39, "KEY_SEMICOLON" },
    { 40, "KEY_AT" },
    { 41, "KEY_RSINGLEQUOTE" },
    { 42, "KEY_LEFTSHIFT" },
    { 43, "KEY_HASH" },
    { 44, "KEY_Z" },
    { 45, "KEY_X" },
    { 46, "KEY_C" },
    { 47, "KEY_V" },
    { 48, "KEY_B" },
    { 49, "KEY_N" },
    { 50, "KEY_M" },
    { 51, "KEY_LEFTARROW" },
    { 52, "KEY_RIGHTARROW" },
    { 53, "KEY_QUESTIONMARK" },
    { 54, "KEY_RIGHTSHIFT" },
    { 55, "KEYPAD_ASTERISK" },
    { 56, "KEY_ALT" },
    { 57, "KEY_SPACE" },
    { 58, "KEY_CAPLOCK" },
    { 59, "KEY_F1" },
    { 60, "KEY_F2" },
    { 61, "KEY_F3" },
    { 62, "KEY_F4" },
    { 63, "KEY_F5" },
    { 64, "KEY_F6" },
    { 65, "KEY_F7" },
    { 66, "KEY_F8" },
    { 67, "KEY_F9" },
    { 68, "KEY_F10" },
    { 69, "KEYPAD_NUMLOCK" },
    { 70, "KEY_SCROLLLOCK" },
    { 71, "KEYPAD_7" },
    { 72, "KEYPAD_8" },
    { 73, "KEYPAD_9" },
    { 74, "KEYPAD_MINUS" },
    { 75, "KEYPAD_4" },
    { 76, "KEYPAD_5" },
    { 77, "KEYPAD_6" },
    { 78, "KEYPAD_PLUS" },
    { 79, "KEYPAD_1" },
    { 80, "KEYPAD_2" },
    { 81, "KEYPAD_3" },
    { 82, "KEYPAD_0" },
    { 83, "KEYPAD_DELETE" },
    { 86, "KEY_BACKSLASH" },
    { 87, "KEY_F11" },
    { 88, "KEY_F12" },
    { 156, "KEYPAD_ENTER" },
    { 157, "KEY_RIGHTCTRL" },
    { 181, "KEYPAD_FORWARDSLASH" },
    { 183, "KEY_PRINTSCREEN" },
    { 184, "KEY_ALTGR" },
    { 199, "KEY_HOME" },
    { 200, "KEY_CURSORUP" },
    { 201, "KEY_PGUP" },
    { 203, "KEY_CURSORLEFT" },
    { 205, "KEY_CURSORRIGHT" },
    { 207, "KEY_END" },
    { 208, "KEY_CURSORDOWN" },
    { 209, "KEY_PGDN" },
    { 210, "KEY_INSERT" },
    { 211, "KEY_DELETE" },
};
/* DAT_005765e0 is a zero-filled virtual tail; keyboard_key_names_init
 * builds this table at runtime, so both sides start zero and the
 * whole-object annotation is byte-safe. */
// GLOBAL: LEGORR 0x005765e0
static const char* s_keyboard_key_names[256];
#include "gods98/resource.hpp"
#include "entity.hpp"

static int s_lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 0x20 : c; }
static int s_toupper(int c) { return (c >= 'a' && c <= 'z') ? c - 0x20 : c; }
static int s_strcaseeq(const char* a, const char* b);

// FUNCTION: LEGORR 0x00485a20
void keyboard_key_names_init(void)
{
    memset(s_keyboard_key_names, 0, sizeof(s_keyboard_key_names));
    for (size_t i = 0; i < sizeof(s_keyboard_name_entries)
                         / sizeof(s_keyboard_name_entries[0]); ++i) {
        const FeKeyboardNameEntry* entry = &s_keyboard_name_entries[i];
        s_keyboard_key_names[entry->code] = entry->name;
    }
}

/* FUN_00485e30 corpus walk: the key-name table strides by four with an inc
 * counter, null slots are skipped in-walk, and the index byte is answered
 * through the raw out pointer (sole-caller audit: both callers pass an
 * array member and a member address — the corpus strides them blindly and
 * the port no longer guards). */
// FUNCTION: LEGORR 0x00485e30
int named_option_index_find_case_insensitive(const char* name,
                                             uint8_t* out_index)
{
    const char* const* p = s_keyboard_key_names;
    int index = 0;
    while (p < s_keyboard_key_names +
                (sizeof(s_keyboard_key_names) / sizeof(s_keyboard_key_names[0]))) {
        const char* entry = *p;
        if (entry != NULL && strcasecmp_lite(entry, name) == 0) {
            *out_index = (uint8_t)index;
            return 1;
        }
        ++p;
        ++index;
    }
    return 0;
}

/* UiTextUnderscoresToSpaces (FUN_004102b0): the corpus returns its input
 * unchanged when it exceeds the 255-byte scratch buffer; shorter strings
 * are copied to shared storage and normalized there. */
// FUNCTION: LEGORR 0x004102b0
const char* ui_text_underscores_to_spaces(const char* text)
{
    static char display[256];
    if (text == NULL) {
        return NULL;
    }
    size_t length = 0;
    while (length < sizeof(display) && text[length] != '\0') {
        ++length;
    }
    if (length == sizeof(display)) {
        return text;
    }
    memcpy(display, text, length + 1);
    for (size_t i = 0; i < length; ++i) {
        if (display[i] == '_') {
            display[i] = ' ';
        }
    }
    return display;
}

/* Menu config identifiers use underscores where the original UI shows
 * spaces (for example Start_Game → Start Game). Keep the raw value in
 * FeItem for navigation and option lookup; normalize only at presentation.
 */
static void fe_display_label(const char* label, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0) {
        return;
    }
    const char* display = ui_text_underscores_to_spaces(label);
    if (display == NULL) {
        out[0] = '\0';
        return;
    }
    size_t i = 0;
    while (display[i] != '\0' && i + 1 < out_size) {
        out[i] = display[i];
        ++i;
    }
    out[i] = '\0';
}

static float s_interface_menu_action_xy[2];
static const void* s_interface_menu_action_context;

/* CaseInsensitiveStringTableIndexFind (FUN_0041a9c0): the index is the
 * position in the 74-key table above, matching the corpus's zero-based
 * pointer walk. */
// FUNCTION: LEGORR 0x0041a9c0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) int interface_menu_action_label_find(const char* name,
                                                       int* out_index)
{
    __asm {
        push ebx
        mov ebx, dword ptr [esp + 8]
        push esi
        push edi
        xor edi, edi
        mov esi, OFFSET s_interface_menu_action_labels
    interface_menu_action_label_find_loop:
        mov eax, dword ptr [esi]
        push ebx
        push eax
        call strcasecmp_lite
        add esp, 8
        test eax, eax
        je interface_menu_action_label_find_found
        add esi, 4
        inc edi
        cmp esi, OFFSET s_interface_menu_action_labels + 0x128
        jb interface_menu_action_label_find_loop
        xor eax, eax
        pop edi
        pop esi
        pop ebx
        ret
    interface_menu_action_label_find_found:
        mov ecx, dword ptr [esp + 0x14]
        mov eax, 1
        mov dword ptr [ecx], edi
        pop edi
        pop esi
        pop ebx
        ret
    }
}
#else
int interface_menu_action_label_find(const char* name, int* out_index)
{
    if (name == NULL || out_index == NULL) return 0;
    const size_t count = sizeof(s_interface_menu_action_labels)
                         / sizeof(s_interface_menu_action_labels[0]);
    for (size_t i = 0; i < count; ++i) {
        if (s_strcaseeq(name, s_interface_menu_action_labels[i])) {
            *out_index = (int)i;
            return 1;
        }
    }
    return 0;
}
#endif

const char* interface_menu_action_label_get(int index)
{
    const size_t count = sizeof(s_interface_menu_action_labels)
                         / sizeof(s_interface_menu_action_labels[0]);
    return index >= 0 && (size_t)index < count
        ? s_interface_menu_action_labels[index] : NULL;
}

void interface_menu_action_layout_peek(float out_xy[2])
{
    if (out_xy != NULL) {
        out_xy[0] = s_interface_menu_action_xy[0];
        out_xy[1] = s_interface_menu_action_xy[1];
    }
}

const void* interface_menu_action_context_peek(void)
{
    return s_interface_menu_action_context;
}

/* InterfaceMenuActionLabelsInit (FUN_0041a300): load the action key table,
 * enable timed-effect entry 4, and install the cursor/selection region. The
 * name table itself uses immutable executable strings in the port. */
// FUNCTION: LEGORR 0x0041a300
void interface_menu_action_labels_init(uint32_t x, uint32_t y,
                                       const void* context)
{
    timed_effect_persistence_set(4, 1);
    s_interface_menu_action_xy[0] = (float)x;
    s_interface_menu_action_xy[1] = (float)y;
    s_interface_menu_action_context = context;
    interaction_action_code_map_initialize();
    selection_region_parameters_set(x, y,
                                    (uint32_t)(display_width_get() + 10),
                                    y, 750.0f);
    objective_selection_state_reset();
}

static int s_strcaseeq(const char* a, const char* b)
{
    if (a == NULL || b == NULL) {
        return a == b;
    }
    while (*a && *b) {
        if (s_lower((unsigned char)*a) != s_lower((unsigned char)*b)) {
            return 0;
        }
        ++a; ++b;
    }
    return *a == *b;
}

/* Case-insensitive substring search ("LevelPick" in "Interface\LevelPicker\…"). */
static int s_stristr(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    for (size_t i = 0; haystack[i] != '\0'; i++) {
        size_t j = 0;
        while (needle[j] != '\0'
               && s_lower((unsigned char)haystack[i + j])
                  == s_lower((unsigned char)needle[j])) {
            ++j;
        }
        if (needle[j] == '\0') {
            return 1;
        }
    }
    return 0;
}

/* "Levels::Tutorial01" -> "Tutorial01" (the level group name). A name with
 * no "::" passes through. */
static const char* fe_level_group_name(const char* name)
{
    if (name == NULL) {
        return NULL;
    }
    const char* p = strstr(name, "::");
    return p != NULL ? p + 2 : name;
}

/* The cfg submenu record name ("Menu<i+1>"). */
static const char* fe_menu_name_idx(int idx)
{
    static char n[16];
    snprintf(n, sizeof(n), "Menu%d", idx + 1);
    return n;
}

/* --- config value helpers (Levels-container aware, like #37) ------------- */

static const char* fe_cfg_str(BlockFile* cfg, const char* level,
                              const char* key, char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL || key == NULL) {
        return NULL;
    }
    const char* root = util_root_name(cfg);
    char path[FE_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s Levels %s %s", root, level, key);
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        snprintf(path, sizeof(path), "%s %s %s", root, level, key);
        v = block_get_strptr(cfg, path);
    }
    if (v == NULL) {
        return NULL;
    }
    snprintf(out, out_sz, "%s", v);
    return out;
}

static int fe_cfg_int(BlockFile* cfg, const char* level, const char* key,
                      int dflt)
{
    char buf[FE_PATH_MAX * 2];
    const char* v = fe_cfg_str(cfg, level, key, buf, sizeof(buf));
    if (v == NULL) {
        return dflt;
    }
    return (int)strtol(v, NULL, 10);
}

/* --- the StartLevel / TutorialStartLevel + NextLevel chains -------------- */

// FUNCTION: LEGORR 0x004154b0
static int fe_build_chain(BlockFile* cfg, const char* start_key,
                          char names[][FE_LEVEL_NAME_MAX],
                          char labels[][FE_NAME_MAX], int max)
{
    if (cfg == NULL) {
        return 0;
    }
    const char* root = util_root_name(cfg);
    char path[FE_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s Main %s", root, start_key);
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return 0;
    }
    int n = 0;
    const char* group = fe_level_group_name(v);
    while (group != NULL && group[0] != '\0' && n < max) {
        snprintf(names[n], FE_LEVEL_NAME_MAX, "%s", group);
        char lbl[FE_NAME_MAX];
        if (fe_cfg_str(cfg, group, "FullName", lbl, sizeof(lbl)) != NULL) {
            snprintf(labels[n], FE_NAME_MAX, "%s", lbl);
        } else {
            snprintf(labels[n], FE_NAME_MAX, "%s", group);
        }
        char nl[FE_LEVEL_NAME_MAX];
        const char* next = fe_cfg_str(cfg, group, "NextLevel", nl,
                                      sizeof(nl));
        group = next != NULL ? fe_level_group_name(nl) : NULL;
        ++n;
    }
    return n;
}

/* --- Item%i value grammar ------------------------------------------------- */

/* Item-kind matcher. The corpus compares all six names through the CRT's
 * case-folded compare (six `call 0x48ff10` sites, literals pushed second:
 * `push 0x4a23dc /*"cycle" / push esi / call 0x48ff10 / add esp,8 / test
 * eax,eax / jne`), answers 0..4 from mov-immediates, and folds the tail
 * match arithmetically (`neg eax / sbb eax,eax / and al,0xfa / add eax,5`
 * — MSVC's `? 5 : -1` idiom). Unknown tokens answer -1, which the caller's
 * switch routes to the generic banner arm; the earlier local spelling
 * (s_strcaseeq, no "next" case, unknown folded to Next) predates the
 * corpus read. */
/* The corpus compares against its .data token strings at 0x4a23ac..0x4a23e3
 * (eight/12-byte strides, image bytes verified against the reference PE:
 * next, realslider, slider, textinput, trigger, cycle). Annotating them as
 * DATs lets each `push` land on the corpus's immediate address — the six
 * `push imm` vs `push STRING` pairs were this function's entire residual. */
// GLOBAL: LEGORR 0x004a23ac
static const char s_item_token_next[5] = "next";
// GLOBAL: LEGORR 0x004a23b4
static const char s_item_token_realslider[11] = "realslider";
// GLOBAL: LEGORR 0x004a23c0
static const char s_item_token_slider[7] = "slider";
// GLOBAL: LEGORR 0x004a23c8
static const char s_item_token_textinput[10] = "textinput";
// GLOBAL: LEGORR 0x004a23d4
static const char s_item_token_trigger[8] = "trigger";
// GLOBAL: LEGORR 0x004a23dc
static const char s_item_token_cycle[6] = "cycle";

// FUNCTION: LEGORR 0x00413e60
static int fe_item_kind(const char* tok)
{
    if (strcasecmp_lite(tok, s_item_token_cycle) == 0)      return FE_ITEM_CYCLE;
    if (strcasecmp_lite(tok, s_item_token_trigger) == 0)    return FE_ITEM_TRIGGER;
    if (strcasecmp_lite(tok, s_item_token_textinput) == 0)  return FE_ITEM_TEXTINPUT;
    if (strcasecmp_lite(tok, s_item_token_slider) == 0)     return FE_ITEM_SLIDER;
    if (strcasecmp_lite(tok, s_item_token_realslider) == 0) return FE_ITEM_REALSLIDER;
    return strcasecmp_lite(tok, s_item_token_next) == 0 ? FE_ITEM_NEXT : -1;
}

/* Slider initial values per banner — the corpus's option defaults
 * (OptionsLoadOrDefault: speed 1, SFX 8, music 5, brightness 8). */
static int fe_option_default(const char* banner, int lo, int hi);

/* Split an Item value ("Next:0:0:Start_Game:Menu2", "Trigger:…:Yes:1",
 * "Slider:x1:y1:x2:y2:banner:lo:hi:img…") into ':'-fields. Position x/y are
 * fields 1/2 for every kind; the display label differs per kind (finding 23):
 * Next/Trigger put it at field 3, Slider/Cycle/RealSlider at field 5
 * (x1:y1:x2:y2). Image-backed Next/Trigger entries carry normal and
 * selected images plus a tooltip before their target/flag. */
static void fe_parse_item(const char* value, FeItem* it)
{
    it->kind = FE_ITEM_NEXT;
    it->x = 0; it->y = 0; it->flag = 0; it->level_start = 0;
    it->load_slot = 0;
    it->min = 0; it->max = 0; it->value = 0; it->cycle_count = 0;
    it->label[0] = '\0'; it->target[0] = '\0';
    it->image_normal[0] = it->image_selected[0] = '\0';
    if (value == NULL) {
        return;
    }
    char fields[12][FE_PATH_MAX];
    int n = 0;
    const char* p = value;
    while (n < 12) {
        while (*p == ':') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        const char* s = p;
        while (*p != '\0' && *p != ':') {
            ++p;
        }
        int len = (int)(p - s);
        if (len >= FE_PATH_MAX) {
            len = FE_PATH_MAX - 1;
        }
        memcpy(fields[n], s, (size_t)len);
        fields[n][len] = '\0';
        ++n;
    }
    if (n == 0) {
        return;
    }
    it->kind = fe_item_kind(fields[0]);
    int atoi1 = (n > 1) ? (int)strtol(fields[1], NULL, 10) : 0;
    int atoi2 = (n > 2) ? (int)strtol(fields[2], NULL, 10) : 0;
    switch (it->kind) {
    case FE_ITEM_NEXT: {
        it->x = atoi1;
        it->y = atoi2;
        const int image_form = n > 7 && s_stristr(fields[3], ".bmp");
        if (image_form) {
            snprintf(it->image_normal, sizeof(it->image_normal), "%s",
                     fields[3]);
            snprintf(it->image_selected, sizeof(it->image_selected), "%s",
                     fields[4]);
            snprintf(it->label, FE_NAME_MAX, "%s", fields[6]);
            snprintf(it->target, FE_NAME_MAX, "%s", fields[7]);
        } else if (n > 3) {
            snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
            if (n > 4) {
                snprintf(it->target, FE_NAME_MAX, "%s", fields[4]);
            }
        }
        break;
    }
    case FE_ITEM_TRIGGER: {
        it->x = atoi1;
        it->y = atoi2;
        /* image trigger form: Trigger:x:y:imgN:imgG:imgD:tooltip:1 */
        int image_form = (n > 3) && s_stristr(fields[3], ".bmp");
        if (image_form) {
            snprintf(it->image_normal, sizeof(it->image_normal), "%s",
                     fields[3]);
            if (n > 4) {
                snprintf(it->image_selected, sizeof(it->image_selected),
                         "%s", fields[4]);
            }
            if (n > 6) {
                snprintf(it->label, FE_NAME_MAX, "%s", fields[6]);
            } else {
                snprintf(it->label, FE_NAME_MAX, "…");
            }
            if (n > 7) {
                it->flag = (int)strtol(fields[7], NULL, 10);
            }
        } else {
            if (n > 3) {
                snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
            }
            if (n > 4) {
                it->flag = (int)strtol(fields[4], NULL, 10);
            }
        }
        break;
    }
    default: /* slider / cycle / realslider / textinput: banner at field 5 */
        it->x = atoi1;
        it->y = atoi2;
        if (n > 5) {
            snprintf(it->label, FE_NAME_MAX, "%s", fields[5]);
        }
        if (it->label[0] == '\0' && n > 3) {
            snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
        }
        if (it->kind == FE_ITEM_SLIDER) {
            /* Slider:x1:y1:x2:y2:banner:lo:hi:bar… */
            it->min = (n > 6) ? (int)strtol(fields[6], NULL, 10) : 0;
            it->max = (n > 7) ? (int)strtol(fields[7], NULL, 10) : 100;
            it->flag = it->min;
            it->value = fe_option_default(it->label, it->min, it->max);
        } else if (it->kind == FE_ITEM_REALSLIDER) {
            /* RealSlider:type:x1:y1:x2:y2:banner:lo:hi:step */
            it->min = (n > 6) ? (int)strtol(fields[6], NULL, 10) : 0;
            it->max = (n > 7) ? (int)strtol(fields[7], NULL, 10) : 100;
            it->value = fe_option_default(it->label, it->min, it->max);
        } else if (it->kind == FE_ITEM_CYCLE) {
            /* Cycle:x1:y1:x2:y2:banner:count:opt1:opt2:… */
            int count = (n > 6) ? (int)strtol(fields[6], NULL, 10) : 0;
            if (count > 4) {
                count = 4;
            }
            it->cycle_count = count;
            it->min = 0;
            it->max = count > 0 ? count - 1 : 0;
            it->value = 0;
            for (int k = 0; k < count && (7 + k) < n; k++) {
                snprintf(it->cycle_opts[k], sizeof(it->cycle_opts[0]), "%s",
                         fields[7 + k]);
            }
        }
        break;
    }
}

/* Slider initial values per banner — the corpus's option defaults
 * (OptionsLoadOrDefault: speed 1, SFX 8, music 5, brightness 8). With
 * `fe.opts` attached (issue #85) the *effective* store value wins —
 * fe_options_seed overrides these — and menu edits write back. */
static int fe_option_default(const char* banner, int lo, int hi)
{
    int d = lo;
    if (s_stristr(banner, "sfx")) { d = 8; }
    else if (s_stristr(banner, "music")) { d = 5; }
    else if (s_stristr(banner, "bright")) { d = 8; }
    else if (s_stristr(banner, "speed")) { d = 1; }
    if (d < lo) { d = lo; }
    if (d > hi) { d = hi; }
    return d;
}

/* --- functional options (issue #85) --------------------------------------
 * `fe->opts` (not owned) is the effective-value store: seeding puts the
 * store's value into the widget, adjustments write it back. Labels that map
 * to no option id (level names, plain triggers) are left display-only. */

static int fe_options_widget(int kind)
{
    return kind == FE_ITEM_SLIDER || kind == FE_ITEM_REALSLIDER
        || kind == FE_ITEM_CYCLE;
}

static void fe_options_seed_item(FrontEnd* fe, FeItem* it)
{
    int v = 0;
    if (fe->opts == NULL || !fe_options_widget(it->kind)) {
        return;
    }
    if (!options_get_label(fe->opts, it->label, &v)) {
        return;
    }
    if (v < it->min) {
        v = it->min;
    }
    if (v > it->max) {
        v = it->max;
    }
    it->value = v;
}

void frontend_options_sync(FrontEnd* fe)
{
    if (fe == NULL || fe->opts == NULL) {
        return;
    }
    for (int m = 0; m < FE_MENU_COUNT; m++) {
        for (int s = 0; s < fe->menus[m].submenu_count; s++) {
            FeSubmenu* sub = &fe->menus[m].submenus[s];
            for (int i = 0; i < sub->item_count; i++) {
                fe_options_seed_item(fe, &sub->items[i]);
            }
        }
    }
}

/* Write the widget's new value back into the store (issue #85): every option
 * adjustment lands here, so the frame loop's live consumers (dt scale,
 * brightness, help overlay, audio) read the effective value. */
static void fe_options_write(FrontEnd* fe, FeItem* it)
{
    if (fe->opts != NULL && fe_options_widget(it->kind)) {
        options_set_label(fe->opts, it->label, it->value);
    }
}

/* --- menu group building -------------------------------------------------- */

static void fe_parse_submenu(FrontEnd* fe, FeMenu* m, const char* name,
                             int idx)
{
    if (fe == NULL || m == NULL || name == NULL || idx < 0
        || idx >= FE_SUBMENUS_MAX || fe->cfg == NULL) {
        return;
    }
    FeSubmenu* sub = &m->submenus[idx];
    sub->item_count = 0;
    sub->base_x = sub->base_y = 0;
    sub->title_anchor_x = sub->title_anchor_y = 0;
    sub->auto_center = 0;
    sub->display_title = 0;
    sub->can_scroll = 0;
    sub->play_random = 0;
    sub->scroll = sub->scroll_extent = 0;
    sub->full_name[0] = sub->title[0] = '\0';
    sub->image[0] = sub->image_dark[0] = '\0';
    sub->lofont[0] = sub->hifont[0] = '\0';
    memset(sub->items, 0, sizeof(sub->items));
    memset(sub->overlays, 0, sizeof(sub->overlays));
    sub->overlay_count = 0;
    char pre[FE_PATH_MAX * 2];
    snprintf(pre, sizeof(pre), "%s Menu::%s %s", util_root_name(fe->cfg),
             name, fe_menu_name_idx(idx));
    char buf[FE_PATH_MAX * 2];

    snprintf(buf, sizeof(buf), "%s Title", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->title, FE_NAME_MAX, "%s", block_get_strptr(fe->cfg, buf));
    }
    snprintf(buf, sizeof(buf), "%s FullName", pre);
    const char* v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        snprintf(sub->full_name, sizeof(sub->full_name), "%s", v);
        for (char* p = sub->full_name; *p != '\0'; ++p) {
            if (*p == '_') *p = ' ';
        }
    }

    /* Position x:y */
    snprintf(buf, sizeof(buf), "%s Position", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        const char* c = strchr(v, ':');
        if (c != NULL) {
            char* x_end = NULL;
            long x = strtol(v, &x_end, 10);
            if (x_end == c) {
                char* y_end = NULL;
                long y = strtol(c + 1, &y_end, 10);
                if (y_end != c + 1) {
                    sub->base_x = (int)x;
                    sub->base_y = (int)y;
                }
            }
        }
    }

    snprintf(buf, sizeof(buf), "%s AutoCenter", pre);
    sub->auto_center = block_get_bool(fe->cfg, buf) == 1;
    snprintf(buf, sizeof(buf), "%s DisplayTitle", pre);
    sub->display_title = block_get_bool(fe->cfg, buf) == 1;
    /* Anchored is stored as x:y; title placement currently uses its y value. */
    snprintf(buf, sizeof(buf), "%s Anchored", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        const char* c = strchr(v, ':');
        if (c != NULL) {
            char xbuf[16];
            size_t xlen = (size_t)(c - v);
            if (xlen >= sizeof(xbuf)) xlen = sizeof(xbuf) - 1;
            memcpy(xbuf, v, xlen);
            xbuf[xlen] = '\0';
            sub->title_anchor_x = (int)strtol(xbuf, NULL, 10);
            sub->title_anchor_y = (int)strtol(c + 1, NULL, 10);
        }
    }
    snprintf(buf, sizeof(buf), "%s CanScroll", pre);
    sub->can_scroll = block_get_bool(fe->cfg, buf) == 1;
    snprintf(buf, sizeof(buf), "%s MenuImage", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        /* "img.bmp:0:0:1" -> keep only the path part (temp first:
         * snprintf with overlapping src/dst would be UB) */
        char raw[FE_PATH_MAX];
        snprintf(raw, sizeof(raw), "%s", v);
        const char* c = strchr(raw, ':');
        snprintf(sub->image, FE_PATH_MAX, "%.*s",
                 c != NULL ? (int)(c - raw) : (int)strlen(raw), raw);
    }
    snprintf(buf, sizeof(buf), "%s MenuImageDark", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        char raw[FE_PATH_MAX];
        snprintf(raw, sizeof(raw), "%s", v);
        const char* c = strchr(raw, ':');
        snprintf(sub->image_dark, sizeof(sub->image_dark), "%.*s",
                 c != NULL ? (int)(c - raw) : (int)strlen(raw), raw);
    }

    /* Bitmap fonts: LoFont = unselected, HiFont = selected (MenuFont is
     * the measurement face — fall back to it for both when the pair is
     * absent). */
    snprintf(buf, sizeof(buf), "%s LoFont", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->lofont, FE_PATH_MAX, "%s",
                 block_get_strptr(fe->cfg, buf));
    }
    snprintf(buf, sizeof(buf), "%s HiFont", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->hifont, FE_PATH_MAX, "%s",
                 block_get_strptr(fe->cfg, buf));
    }
    if (sub->lofont[0] == '\0' || sub->hifont[0] == '\0') {
        snprintf(buf, sizeof(buf), "%s MenuFont", pre);
        const char* mf = block_get_strptr(fe->cfg, buf);
        if (mf != NULL) {
            if (sub->lofont[0] == '\0') {
                snprintf(sub->lofont, FE_PATH_MAX, "%s", mf);
            }
            if (sub->hifont[0] == '\0') {
                snprintf(sub->hifont, FE_PATH_MAX, "%s", mf);
            }
        }
    }

    snprintf(buf, sizeof(buf), "%s ItemCount", pre);
    int ic = block_get_int(fe->cfg, buf, 0);
    if (ic > FE_ITEMS_MAX) {
        ic = FE_ITEMS_MAX;
    }
    sub->item_count = ic;
    for (int item_index = 0; item_index < ic; item_index++) {
        snprintf(buf, sizeof(buf), "%s Item%d", pre, item_index + 1);
        const char* item = block_get_strptr(fe->cfg, buf);
        if (item == NULL) {
            continue;
        }
        fe_parse_item(item, &sub->items[item_index]);
    }

    /* FLIC overlays (`!Overlay%i <avi\X.flh>:<SFX>:x:y`) + PlayRandom —
     * finding 23's decoration lane; the `.flh` players open lazily at
     * display time (the paths may be loose Data/AVI copies). */
    snprintf(buf, sizeof(buf), "%s PlayRandom", pre);
    sub->play_random = block_get_bool(fe->cfg, buf) == 1;
    sub->overlay_count = 0;
    for (int overlay_index =  1;
         overlay_index <= FE_OVERLAYS_MAX &&
             sub->overlay_count < FE_OVERLAYS_MAX;
         ++overlay_index) {
        snprintf(buf, sizeof(buf), "%s !Overlay%d", pre, overlay_index);
        const char* ov = block_get_strptr(fe->cfg, buf);
        if (ov == NULL || ov[0] == '\0') {
            continue;
        }
        FeOverlay* o = &sub->overlays[sub->overlay_count];
        o->path[0] = o->sfx[0] = '\0';
        o->x = o->y = 0;
        o->flic = NULL;
        /* value = path:sfx:x:y (path may itself contain ':' — take the
         * first token as path, then sfx, x, y). */
        const char* p = ov;
        const char* c1 = strchr(p, ':');
        size_t pl = c1 != NULL ? (size_t)(c1 - p) : strlen(p);
        if (pl >= sizeof(o->path)) {
            pl = sizeof(o->path) - 1;
        }
        memcpy(o->path, p, pl);
        o->path[pl] = '\0';
        if (c1 != NULL) {
            const char* sfx = c1 + 1;
            const char* c2 = strchr(sfx, ':');
            size_t sl = c2 != NULL ? (size_t)(c2 - sfx) : strlen(sfx);
            if (sl >= sizeof(o->sfx)) {
                sl = sizeof(o->sfx) - 1;
            }
            memcpy(o->sfx, sfx, sl);
            o->sfx[sl] = '\0';
            if (c2 != NULL) {
                o->x = (int)strtol(c2 + 1, NULL, 10);
                const char* c3 = strchr(c2 + 1, ':');
                if (c3 != NULL && c3[1] != '\0') {
                    o->y = (int)strtol(c3 + 1, NULL, 10);
                }
            }
        }
        ++sub->overlay_count;
    }
}

/* Per-menu config constructor (the surrounding frontend_build composes the
 * main/save/pause/options menus; submenu/item parsing is delegated below). */
// FUNCTION: LEGORR 0x004140c0
static int fe_parse_menu(FrontEnd* fe, int mi, const char* name)
{
    if (fe == NULL || mi < 0 || mi >= FE_MENU_COUNT || name == NULL) {
        return 0;
    }
    FeMenu* m = &fe->menus[mi];
    m->submenu_count = 0;
    snprintf(m->name, FE_NAME_MAX, "%s", name);
    if (fe->cfg == NULL) {
        return 0;
    }
    char buf[FE_PATH_MAX * 2];
    snprintf(buf, sizeof(buf), "%s Menu::%s MenuCount", util_root_name(fe->cfg),
             name);
    int count = block_get_int(fe->cfg, buf, 0);
    if (count < 0) {
        count = 0;
    }
    if (count > FE_SUBMENUS_MAX) {
        count = FE_SUBMENUS_MAX;
    }
    m->submenu_count = count;
    for (int i = 0; i < count; i++) {
        fe_parse_submenu(fe, m, name, i);
    }
    return count > 0;
}

/* Graft a level chain onto the picker sub-menus: each row becomes a Next
 * item with x/y = the level's FrontEndX/Y scatter position and
 * level_start = 1. Campaign -> "LevelPick" art, tutorial -> "LevelPickT". */
static void fe_graft_picker(FrontEnd* fe, FeMenu* m, int tutorial)
{
    for (int i = 0; i < m->submenu_count; i++) {
        FeSubmenu* sub = &m->submenus[i];
        if (tutorial) {
            if (!s_stristr(sub->image, "LevelPickT")) {
                continue;
            }
        } else {
            if (!s_stristr(sub->image, "LevelPick")
                || s_stristr(sub->image, "LevelPickT")) {
                continue;
            }
        }
        int base = sub->item_count;
        int n = tutorial ? fe->tutorial_n : fe->campaign_n;
        for (int k = 0; k < n && base + k < FE_ITEMS_MAX; k++) {
            FeItem* it = &sub->items[base + k];
            it->kind = FE_ITEM_NEXT;
            it->level_start = 1;
            const char* lvl = tutorial ? fe->tutorial[k] : fe->campaign[k];
            snprintf(it->target, FE_NAME_MAX, "%s", lvl);
            snprintf(it->label, FE_NAME_MAX, "%s",
                     tutorial ? fe->tutorial_label[k] : fe->campaign_label[k]);
            it->x = fe_cfg_int(fe->cfg, lvl, "FrontEndX", 0);
            it->y = fe_cfg_int(fe->cfg, lvl, "FrontEndY", 0);
            sub->item_count = base + k + 1;
        }
        /* The campaign/tutorial picker plane is taller than any window
         * (FrontEndY scatters to ~2100), so the receiving sub scrolls —
         * the corpus gates its rows with the DAT_005574e0 content clamp;
         * flagging the grafted sub is the port-side equivalent. */
        if (sub->item_count > base) {
            sub->can_scroll = 1;
            sub->scroll_extent = 0;
        }
    }
}

/* --- public API ----------------------------------------------------------- */

/* Close every open overlay flic (menu switch / close). */
static void fe_overlay_close_all(FrontEnd* fe)
{
    for (int m = 0; m < FE_MENU_COUNT; ++m) {
        for (int s = 0; s < fe->menus[m].submenu_count; ++s) {
            FeSubmenu* sub = &fe->menus[m].submenus[s];
            for (int i = 0; i < sub->overlay_count; ++i) {
                if (sub->overlays[i].flic != NULL) {
                    flic_close(sub->overlays[i].flic);
                    sub->overlays[i].flic = NULL;
                }
                sub->overlays[i].tried = 0;
            }
        }
    }
    fe->overlays_for = NULL;
    fe->overlay_active = -1;
}

/* Keep the existing all-overlay handling for submenus without PlayRandom. */
const FeSubmenu* frontend_active_submenu(const FrontEnd* fe);

static void fe_overlay_open_all(FrontEnd* fe, const FeSubmenu* sub)
{
    if (fe == NULL || sub == NULL) {
        return;
    }
    for (int i = 0; i < sub->overlay_count; ++i) {
        FeOverlay* o = (FeOverlay*)&sub->overlays[i];
        if (o->tried) {
            continue;
        }
        o->tried = 1;
        o->flic = flic_open(o->path);
    }
    fe->overlays_for = sub;
}

/* FUN_0048e170: the game's 15-bit LCG. Seed this front-end instance from
 * wall-clock seconds and its address so menu sequences vary between runs
 * without consuming the gameplay random stream. */
static uint32_t fe_overlay_random(FrontEnd* fe)
{
    fe->overlay_rng = fe->overlay_rng * 0x343fdu + 0x269ec3u;
    return (fe->overlay_rng >> 16) & 0x7fffu;
}

static void fe_overlay_play_sfx(FrontEnd* fe, const FeOverlay* o)
{
    if (fe == NULL || o == NULL || fe->audio == NULL
        || o->sfx[0] == '\0' || s_strcaseeq(o->sfx, "SFX_NULL")) {
        return;
    }

    /* Overlay values name an SFX alias, while Lego.cfg maps that alias under
     * Samples to a WAVE resource (often prefixed by '@' for streamed sounds). */
    char key[FE_NAME_MAX + 32];
    snprintf(key, sizeof(key), "%s Samples !%s", util_root_name(fe->cfg),
             o->sfx);
    const char* resource = block_get_strptr(fe->cfg, key);
    if (resource == NULL || resource[0] == '\0') {
        return;
    }
    if (resource[0] == '@') {
        ++resource;
    }
    audio_play_named(fe->audio, resource);
}

static void fe_overlay_start_random(FrontEnd* fe, const FeSubmenu* sub)
{
    if (sub->overlay_count <= 0 || fe_overlay_random(fe) % 400u != 0) {
        return;
    }

    int picked = (int)(fe_overlay_random(fe) % (uint32_t)sub->overlay_count);
    FeOverlay* o = (FeOverlay*)&sub->overlays[picked];
    o->flic = flic_open(o->path);
    if (o->flic == NULL) {
        return;
    }
    fe->overlay_active = picked;
    fe_overlay_play_sfx(fe, o);
}

/* MenuOverlayFlicPlaybackController (FUN_004121b0): ensure the current
 * submenu's overlays are open (re-opening on menu switches), start random
 * playback when configured, and advance each active flic one frame. The
 * port's FrontEnd owns the former global menu/Flic state. */
// FUNCTION: LEGORR 0x004121b0
static void fe_overlay_step(FrontEnd* fe)
{
    if (fe == NULL || !fe->open) {
        return;
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    if (sub == NULL || sub->overlay_count == 0) {
        if (fe->overlays_for != NULL) {
            fe_overlay_close_all(fe);
        }
        return;
    }
    if (fe->overlays_for != sub) {
        fe_overlay_close_all(fe);
        fe->overlays_for = sub;
        if (!sub->play_random) {
            fe_overlay_open_all(fe, sub);
        }
    }
    if (sub->play_random) {
        if (fe->overlay_active >= 0
            && fe->overlay_active < sub->overlay_count) {
            FeOverlay* o = (FeOverlay*)&sub->overlays[fe->overlay_active];
            flic_tick_loop(o->flic, 0);
            if (flic_eof(o->flic)) {
                flic_close(o->flic);
                o->flic = NULL;
                o->tried = 0;
                fe->overlay_active = -1;
            }
        } else {
            fe_overlay_start_random(fe, sub);
        }
        return;
    }
    for (int i = 0; i < sub->overlay_count; ++i) {
        if (sub->overlays[i].flic != NULL) {
            flic_tick_loop(sub->overlays[i].flic, 1);
        }
    }
}

void frontend_init(FrontEnd* fe)
{
    if (fe == NULL) {
        return;
    }
    s_main_menu_text_item_pending = 0;
    memset(fe, 0, sizeof(*fe));
    fe->pointer_x = fe->pointer_y = -1;
    fe->overlay_active = -1;
    fe->overlay_rng = (uint32_t)time(NULL) ^ (uint32_t)(uintptr_t)fe;
    if (fe->overlay_rng == 0) {
        fe->overlay_rng = 1;
    }
}

static int fe_parse_int_tuple(const char* text, int* out, int count)
{
    if (text == NULL || out == NULL || count <= 0) {
        return 0;
    }
    const char* p = text;
    for (int i = 0; i < count; ++i) {
        char* end = NULL;
        long value = strtol(p, &end, 10);
        if (end == p || value < -32768 || value > 32767) {
            return 0;
        }
        out[i] = (int)value;
        if (i + 1 < count) {
            if (*end != ':') {
                return 0;
            }
            p = end + 1;
        }
    }
    return 1;
}

/* InterfaceBackButtonConfigLoad (FUN_0041b1b0): the Lego.cfg record is
 * `width:height:highlight-image:pressed-image:action`. The corpus also
 * initializes its legacy UI text entry 5; the port's menu model has no such
 * text-entry record, so retain the action token for the future UI consumer. */
// FUNCTION: LEGORR 0x0041b1b0
int interface_back_button_config_load(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    char key[FE_PATH_MAX * 2];
    snprintf(key, sizeof(key), "%s InterfaceBackButton", util_root_name(cfg));
    const char* value = block_get_strptr(cfg, key);
    if (value == NULL) {
        return 0;
    }

    int width = 0, height = 0;
    char* end = NULL;
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != ':' || parsed <= 0 || parsed > 32767) {
        return 0;
    }
    width = (int)parsed;
    const char* p = end + 1;
    parsed = strtol(p, &end, 10);
    if (end == p || *end != ':' || parsed <= 0 || parsed > 32767) {
        return 0;
    }
    height = (int)parsed;
    p = end + 1;

    const char* highlight_end = strchr(p, ':');
    if (highlight_end == NULL || highlight_end == p) {
        return 0;
    }
    const char* pressed = highlight_end + 1;
    const char* pressed_end = strchr(pressed, ':');
    if (pressed_end == NULL || pressed_end == pressed) {
        return 0;
    }
    const char* action = pressed_end + 1;
    if (*action == '\0' || strchr(action, ':') != NULL) {
        return 0;
    }

    char highlight_path[FE_PATH_MAX];
    char pressed_path[FE_PATH_MAX];
    size_t highlight_len = (size_t)(highlight_end - p);
    size_t pressed_len = (size_t)(pressed_end - pressed);
    size_t action_len = strlen(action);
    if (highlight_len >= sizeof(highlight_path)
        || pressed_len >= sizeof(pressed_path)
        || action_len >= sizeof(s_back_button_action)) {
        return 0;
    }
    memcpy(highlight_path, p, highlight_len);
    highlight_path[highlight_len] = '\0';
    memcpy(pressed_path, pressed, pressed_len);
    pressed_path[pressed_len] = '\0';

    s_back_button_width = width;
    s_back_button_height = height;
    s_back_button_highlight = texture_get_or_load(NULL, highlight_path);
    s_back_button_pressed = texture_get_or_load(NULL, pressed_path);
    memcpy(s_back_button_action, action, action_len + 1);
    return 1;
}

int interface_back_button_dimensions(int* width, int* height)
{
    if (width != NULL) *width = s_back_button_width;
    if (height != NULL) *height = s_back_button_height;
    return s_back_button_width > 0 && s_back_button_height > 0;
}

TextureSurface* interface_back_button_image(int pressed)
{
    return pressed ? s_back_button_pressed : s_back_button_highlight;
}

const char* interface_back_button_action(void)
{
    return s_back_button_action[0] != '\0' ? s_back_button_action : NULL;
}

static int fe_token_is(const char* token, size_t length, const char* expected)
{
    size_t i = 0;
    while (expected[i] != '\0') {
        if (i >= length
            || s_lower((unsigned char)token[i])
                   != s_lower((unsigned char)expected[i])) {
            return 0;
        }
        ++i;
    }
    return i == length;
}

static int fe_token_copy(char* out, size_t out_size,
                         const char* token, size_t token_size)
{
    if (out == NULL || out_size == 0 || token_size >= out_size) {
        return 0;
    }
    memcpy(out, token, token_size);
    out[token_size] = '\0';
    return 1;
}

/* A tooltip token is either text or text|sound-alias. */
static void fe_interface_tooltip_parse(const char* token, size_t length,
                                       char* text, size_t text_size,
                                       int* sound, int* has_sound)
{
    const char* pipe = (const char*)memchr(token, '|', length);
    const size_t text_length = pipe != NULL ? (size_t)(pipe - token) : length;
    (void)fe_token_copy(text, text_size, token, text_length);
    *sound = -1;
    *has_sound = 0;
    if (pipe != NULL && (size_t)(token + length - pipe - 1) > 0) {
        char alias[FE_NAME_MAX];
        const size_t alias_length = (size_t)(token + length - pipe - 1);
        if (fe_token_copy(alias, sizeof(alias), pipe + 1, alias_length)
            && sound_alias_lookup(alias, sound)) {
            *has_sound = 1;
        }
    }
}

/* InterfaceImagesConfigLoad (FUN_0041ada0): load three bitmap states and
 * tooltip/sound/shortcut metadata for each of the 74 named action slots. */
// FUNCTION: LEGORR 0x0041ada0
int interface_images_config_load(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    keyboard_key_names_init();
    char section_path[FE_PATH_MAX * 2];
    snprintf(section_path, sizeof(section_path), "%s InterfaceImages",
             util_root_name(cfg));
    if (block_subgroup(cfg, section_path) == NULL) {
        /* Corpus fallback when the generic action-image section is absent. */
        (void)interface_build_images_config_load(cfg);
        (void)interface_back_button_config_load(cfg);
        return 0;
    }
    int loaded = 0;
    for (int i = 0; i < FE_INTERFACE_ACTION_COUNT; ++i) {
        char key[FE_PATH_MAX * 2];
        snprintf(key, sizeof(key), "%s InterfaceImages %s",
                 util_root_name(cfg), s_interface_menu_action_labels[i]);
        const char* value = block_get_strptr(cfg, key);
        if (value == NULL) {
            continue;
        }

        const char* fields[7];
        size_t lengths[7];
        int count = 0;
        const char* p = value;
        int valid = 1;
        while (count < 7) {
            fields[count] = p;
            const char* colon = strchr(p, ':');
            lengths[count] = colon != NULL
                ? (size_t)(colon - p) : strlen(p);
            if (lengths[count] == 0) {
                valid = 0;
                break;
            }
            ++count;
            if (colon == NULL) {
                break;
            }
            if (count == 7) {
                valid = 0;
                break;
            }
            p = colon + 1;
        }
        if (!valid || count < 6) {
            continue;
        }

        FeInterfaceImage* image = &s_interface_images[i];
        memset(image, 0, sizeof(*image));
        image->normal_sound = -1;
        image->disabled_sound = -1;
        if (!fe_token_copy(image->shortcut_name,
                           sizeof(image->shortcut_name),
                           fields[5], lengths[5])) {
            continue;
        }
        image->has_shortcut_code = named_option_index_find_case_insensitive(
            image->shortcut_name, &image->shortcut_code);
        TextureSurface** states[3] = {
            &image->normal, &image->disabled, &image->pressed
        };
        for (int state = 0; state < 3; ++state) {
            if (fe_token_is(fields[state], lengths[state], "NULL")) {
                continue;
            }
            char path[FE_PATH_MAX];
            if (!fe_token_copy(path, sizeof(path), fields[state],
                               lengths[state])) {
                valid = 0;
                break;
            }
            /* The corpus calls BitmapSurfaceLoad directly (FUN_0047d9d0),
             * not the port's name-keyed cache. This keeps the call site on
             * the annotated twin; surfaces in this table therefore follow
             * the corpus's independent-load identity rather than cache
             * sharing. */
            *states[state] = image_surface_load(path, 0u, 0u);
        }
        if (!valid) {
            for (int state = 0; state < 3; ++state) {
                texture_release(*states[state]);
            }
            memset(image, 0, sizeof(*image));
            continue;
        }

        int has_sound = 0;
        fe_interface_tooltip_parse(fields[3], lengths[3],
                                   image->normal_tooltip,
                                   sizeof(image->normal_tooltip),
                                   &image->normal_sound,
                                   &has_sound);
        fe_interface_tooltip_parse(fields[4], lengths[4],
                                   image->disabled_tooltip,
                                   sizeof(image->disabled_tooltip),
                                   &image->disabled_sound,
                                   &has_sound);

        if (count == 7) {
            if (fe_token_is(fields[6], lengths[6], "TRUE")
                || fe_token_is(fields[6], lengths[6], "YES")
                || fe_token_is(fields[6], lengths[6], "1")) {
                image->optional_flag = 1;
                image->has_optional_flag = 1;
            }
            else if (fe_token_is(fields[6], lengths[6], "FALSE")
                     || fe_token_is(fields[6], lengths[6], "NO")
                     || fe_token_is(fields[6], lengths[6], "0")) {
                image->optional_flag = 0;
                image->has_optional_flag = 1;
            }
        }
        ++loaded;
    }
    return loaded;
}

const FeInterfaceImage* interface_image_peek(int action_index)
{
    return (action_index >= 0 && action_index < FE_INTERFACE_ACTION_COUNT)
        ? &s_interface_images[action_index] : NULL;
}

/* InterfaceBuildImagesConfigLoad (FUN_0041ab00): the two groups are keyed
 * by the VehicleTypes and BuildingTypes catalog names, in that order. */
// FUNCTION: LEGORR 0x0041ab00
int interface_build_images_config_load(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    keyboard_key_names_init();
    static const char* const type_sections[2] = {
        "VehicleTypes", "BuildingTypes"
    };
    int loaded = 0;
    for (int group = 0; group < 2; ++group) {
        if (s_interface_build_images[group] != NULL) {
            for (uint32_t i = 0; i < s_interface_build_image_counts[group];
                 ++i) {
                FeInterfaceBuildImage* old = &s_interface_build_images[group][i];
                texture_release(old->normal);
                texture_release(old->disabled);
                texture_release(old->pressed);
            }
            engine_heap_free(s_interface_build_images[group]);
        }
        char type_path[FE_PATH_MAX * 2];
        snprintf(type_path, sizeof(type_path), "%s %s", util_root_name(cfg),
                 type_sections[group]);
        BlockRec* type_section = block_subgroup(cfg, type_path);
        uint32_t count = 0;
        for (BlockRec* row = block_first_child(type_section); row != NULL;
             row = block_next_sibling(row)) {
            ++count;
        }
        s_interface_build_image_counts[group] = count;
        s_interface_build_images[group] = (count != 0)
            ? (FeInterfaceBuildImage*)engine_heap_alloc(
                  (long)count * (long)sizeof(FeInterfaceBuildImage))
            : NULL;
        FeInterfaceBuildImage* images = s_interface_build_images[group];
        if (images == NULL) {
            s_interface_build_image_counts[group] = 0;
            continue;
        }
        memset(images, 0, (size_t)count * sizeof(*images));
        if (type_section == NULL) {
            continue;
        }
        BlockRec* type_row = block_first_child(type_section);
        for (uint32_t i = 0; i < count; ++i) {
            if (type_row == NULL) {
                break;
            }
            const char* type_name = block_name(type_row);
            type_row = block_next_sibling(type_row);
            if (type_name == NULL) {
                continue;
            }
            char key[FE_PATH_MAX * 2];
            snprintf(key, sizeof(key), "%s InterfaceBuildImages %s",
                     util_root_name(cfg), type_name);
            const char* value = block_get_strptr(cfg, key);
            if (value == NULL) {
                continue;
            }
            const char* fields[4];
            size_t lengths[4];
            const char* p = value;
            int valid = 1;
            for (int field = 0; field < 4; ++field) {
                fields[field] = p;
                const char* colon = field < 3 ? strchr(p, ':') : NULL;
                if (field < 3 && colon == NULL) {
                    valid = 0;
                    break;
                }
                lengths[field] = colon != NULL
                    ? (size_t)(colon - p) : strlen(p);
                if (lengths[field] == 0) {
                    valid = 0;
                    break;
                }
                p = colon != NULL ? colon + 1 : p + lengths[field];
            }
            if (!valid || *p != '\0') {
                continue;
            }
            FeInterfaceBuildImage* image = &images[i];
            if (!fe_token_copy(image->shortcut_name,
                               sizeof(image->shortcut_name),
                               fields[3], lengths[3])) {
                continue;
            }
            image->has_shortcut_code =
                named_option_index_find_case_insensitive(
                    image->shortcut_name, &image->shortcut_code);
            TextureSurface** states[3] = {
                &image->normal, &image->disabled, &image->pressed
            };
            for (int state = 0; state < 3; ++state) {
                if (fe_token_is(fields[state], lengths[state], "NULL")) {
                    continue;
                }
                char path[FE_PATH_MAX];
                if (!fe_token_copy(path, sizeof(path), fields[state],
                                   lengths[state])) {
                    valid = 0;
                    break;
                }
                *states[state] = texture_get_or_load(NULL, path);
            }
            if (!valid) {
                texture_release(image->normal);
                texture_release(image->disabled);
                texture_release(image->pressed);
                memset(image, 0, sizeof(*image));
                continue;
            }
            ++loaded;
        }
    }
    return loaded;
}

uint32_t interface_build_image_count(int group)
{
    return group >= 0 && group < 2
        ? s_interface_build_image_counts[group] : 0u;
}

const FeInterfaceBuildImage* interface_build_image_peek(int group, int index)
{
    if (group < 0 || group >= 2 || index < 0
        || (uint32_t)index >= s_interface_build_image_counts[group]
        || s_interface_build_images[group] == NULL) {
        return NULL;
    }
    return &s_interface_build_images[group][index];
}

/* InputBindingValueGet (FUN_0041caf0): resolve a catalog image from the
 * input binding's pressed state, then its flag-8 state, else the normal
 * image. Corpus kind values 1 and 4 select the two image catalogs. */
// FUNCTION: LEGORR 0x0041CAF0
TextureSurface* interface_build_image_binding_value_get(
    const EngineContext* engine, int kind, int index)
{
    const int group = (kind == 1) ? 0 : (kind == 4) ? 1 : -1;
    const FeInterfaceBuildImage* const image =
        interface_build_image_peek(group, index);
    if (image == NULL) {
        return NULL;
    }
    if (input_binding_pressed_get(engine, kind, index, 0) != 0) {
        return image->disabled;
    }
    if (player_input_flag8_get(kind, index) != 0u) {
        return image->pressed;
    }
    return image->normal;
}

/* InputBindingDefaultValueGet (FUN_0041cb90): return the catalog's default
 * image field for kinds 1 and 4. Kinds 2, 11, and 15 read zero-initialized
 * scalar globals in the corpus image; no writer is present in the ledger. */
// FUNCTION: LEGORR 0x0041CB90
void* input_binding_default_value_get(int kind, int index, int mode)
{
    const int group = (kind == 1) ? 0 : (kind == 4) ? 1 : -1;
    if (group >= 0) {
        const FeInterfaceBuildImage* const image =
            interface_build_image_peek(group, index);
        if (image == NULL) {
            return 0u;
        }
        const TextureSurface* const value = (mode == 0)
            ? image->disabled : image->normal;
        return (void*)value;
    }
    if ((kind == 2 || kind == 11 || kind == 15)) {
        return 0u;
    }
    return 0u;
}

/* InterfaceSurroundImagesConfigLoad (FUN_0041afd0): load the eleven
 * numbered surround records. Each record is
 * `image:x:y:w:h:image-without-back:x:y`; either image may be `NULL`. */
// FUNCTION: LEGORR 0x0041afd0
int interface_surround_images_config_load(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    int loaded = 0;
    const char* root = util_root_name(cfg);
    for (int i = 0; i < FE_SURROUND_IMAGES_MAX; ++i) {
        char key[FE_PATH_MAX * 2];
        snprintf(key, sizeof(key), "%s InterfaceSurroundImages %d", root,
                 i + 1);
        const char* value = block_get_strptr(cfg, key);
        FeSurroundImage* record = &s_interface_surround_images[i];
        if (value == NULL) {
            record->image = NULL;
            continue;
        }

        const char* fields[8];
        size_t lengths[8];
        const char* p = value;
        int valid = 1;
        for (int field = 0; field < 8; ++field) {
            fields[field] = p;
            const char* colon = (field < 7) ? strchr(p, ':') : NULL;
            if (field < 7 && colon == NULL) {
                valid = 0;
                break;
            }
            lengths[field] = colon != NULL ? (size_t)(colon - p) : strlen(p);
            if (lengths[field] == 0) {
                valid = 0;
                break;
            }
            p = colon != NULL ? colon + 1 : p + lengths[field];
        }
        if (!valid || *p != '\0' || lengths[0] >= FE_PATH_MAX
            || lengths[5] >= FE_PATH_MAX) {
            continue;
        }

        int numbers[6];
        const int number_fields[6] = { 1, 2, 3, 4, 6, 7 };
        for (int n = 0; n < 6; ++n) {
            const int field = number_fields[n];
            char token[32];
            if (lengths[field] >= sizeof(token)) {
                valid = 0;
                break;
            }
            memcpy(token, fields[field], lengths[field]);
            token[lengths[field]] = '\0';
            /* The corpus uses its annotated atoi twin (FUN_0048db70):
             * numeric prefixes are accepted and the parsed value is not
             * narrowed to the port's earlier signed-16-bit safety range. */
            numbers[n] = (int)util_atoi(token);
        }
        if (!valid) {
            continue;
        }

        const int first_is_null = lengths[0] == 4
            && s_lower((unsigned char)fields[0][0]) == 'n'
            && s_lower((unsigned char)fields[0][1]) == 'u'
            && s_lower((unsigned char)fields[0][2]) == 'l'
            && s_lower((unsigned char)fields[0][3]) == 'l';
        const int second_is_null = lengths[5] == 4
            && s_lower((unsigned char)fields[5][0]) == 'n'
            && s_lower((unsigned char)fields[5][1]) == 'u'
            && s_lower((unsigned char)fields[5][2]) == 'l'
            && s_lower((unsigned char)fields[5][3]) == 'l';
        if (!first_is_null) {
            char image_path[FE_PATH_MAX];
            memcpy(image_path, fields[0], lengths[0]);
            image_path[lengths[0]] = '\0';
            /* FUN_0041afd0 calls BitmapSurfaceLoad (FUN_0047d9d0), rather
             * than the port's name-keyed cache. */
            record->image = image_surface_load(image_path, 0u, 0u);
            record->x = numbers[0];
            record->y = numbers[1];
            record->width = numbers[2];
            record->height = numbers[3];
        }
        if (!second_is_null) {
            char image_path[FE_PATH_MAX];
            memcpy(image_path, fields[5], lengths[5]);
            image_path[lengths[5]] = '\0';
            record->image_without_back = image_surface_load(image_path,
                                                            0u, 0u);
            record->without_back_x = numbers[4];
            record->without_back_y = numbers[5];
        }
        ++loaded;
    }
    return loaded;
}

const FeSurroundImage* interface_surround_image_peek(int index)
{
    return (index >= 0 && index < FE_SURROUND_IMAGES_MAX)
        ? &s_interface_surround_images[index] : NULL;
}

/* SaveMenuLayoutLoad (FUN_00415d00): resolve the seven named resource paths
 * plus the Window rectangle and Panel resource tuple. The corpus writes these
 * into fixed offsets in its resource record; FeSaveMenuLayout keeps the same
 * data in bounded, typed fields. */
// FUNCTION: LEGORR 0x00415d00
int save_menu_layout_load(FrontEnd* fe, const char* group,
                          FeSaveMenuLayout* layout)
{
    if (fe == NULL || fe->cfg == NULL || group == NULL || layout == NULL) {
        return 0;
    }
    memset(layout, 0, sizeof(*layout));
    static const char* const keys[7] = {
        "Load", "Save", "Slot", "SaveSel", "LoadSel", "Level", "Tutorial"
    };
    char* const fields[7] = {
        layout->load, layout->save, layout->slot, layout->save_sel,
        layout->load_sel, layout->level, layout->tutorial
    };
    char path[FE_PATH_MAX * 2];
    int found = 0;
    for (int i = 0; i < 7; ++i) {
        snprintf(path, sizeof(path), "%s %s %s", util_root_name(fe->cfg),
                 group, keys[i]);
        const char* value = block_get_strptr(fe->cfg, path);
        if (value != NULL) {
            snprintf(fields[i], FE_PATH_MAX, "%s", value);
            found = 1;
        }
    }

    snprintf(path, sizeof(path), "%s %s Window", util_root_name(fe->cfg),
             group);
    int window[4];
    if (fe_parse_int_tuple(block_get_strptr(fe->cfg, path), window, 4)) {
        layout->window_x = window[0];
        layout->window_y = window[1];
        layout->window_w = window[2];
        layout->window_h = window[3];
        layout->has_window = 1;
        found = 1;
    }

    snprintf(path, sizeof(path), "%s %s Panel", util_root_name(fe->cfg),
             group);
    const char* panel = block_get_strptr(fe->cfg, path);
    if (panel != NULL) {
        const char* colon = strchr(panel, ':');
        if (colon != NULL && colon != panel) {
            size_t name_len = (size_t)(colon - panel);
            if (name_len < sizeof(layout->panel_path)) {
                memcpy(layout->panel_path, panel, name_len);
                layout->panel_path[name_len] = '\0';
                int rect[4];
                if (fe_parse_int_tuple(colon + 1, rect, 4)) {
                    layout->panel_x = rect[0];
                    layout->panel_y = rect[1];
                    layout->panel_w = rect[2];
                    layout->panel_h = rect[3];
                    layout->has_panel = 1;
                    found = 1;
                }
            }
        }
    }
    return found;
}

int frontend_build(FrontEnd* fe, BlockFile* cfg)
{
    if (fe == NULL) {
        return 0;
    }
    frontend_init(fe);
    fe->cfg = cfg;
    if (cfg == NULL) {
        return 0;
    }

    /* GameConfigInit loads the action and surround image tables alongside
     * the menu definitions; keep those menu-facing resources on the same
     * Lego.cfg startup path in the port. */
    (void)interface_images_config_load(cfg);
    (void)interface_surround_images_config_load(cfg);

    fe->campaign_n = fe_build_chain(cfg, "StartLevel",
                                    fe->campaign, fe->campaign_label,
                                    FE_CHAIN_MAX);
    fe->tutorial_n = fe_build_chain(cfg, "TutorialStartLevel",
                                    fe->tutorial, fe->tutorial_label,
                                    FE_CHAIN_MAX);

    /* SaveMenuBuild loads the SaveText and LevelText layouts separately
     * (FUN_00415d00); they drive the save-slot/level preview panels. */
    (void)save_menu_layout_load(fe, "Menu::SaveText", &fe->save_text_layout);
    (void)save_menu_layout_load(fe, "Menu::LevelText", &fe->level_text_layout);

    int main_ok = fe_parse_menu(fe, FE_MENU_MAIN, "MainMenuFull");
    fe_parse_menu(fe, FE_MENU_SAVE, "SaveMenu");
    fe_parse_menu(fe, FE_MENU_PAUSED, "PausedMenu");
    fe_parse_menu(fe, FE_MENU_OPTIONS, "OptionsMenu");

    /* graft the level pickers onto the main menu's picker sub-menus */
    fe_graft_picker(fe, &fe->menus[FE_MENU_MAIN], 0);
    fe_graft_picker(fe, &fe->menus[FE_MENU_MAIN], 1);

    fe->built = main_ok;
    /* SaveMenuBuild's two SavMenuList calls consume campaign/tutorial trees.
     * Build those from the configured LevelLinks graphs for the typed row
     * snapshots used by the level-preview lists. */
    if (fe->built) {
        (void)fe_sav_menu_list_from_config(
            fe, 4, fe->campaign_n > 0 ? fe->campaign[0] : NULL);
        (void)fe_sav_menu_list_from_config(
            fe, 5, fe->tutorial_n > 0 ? fe->tutorial[0] : NULL);
    }
    /* Options widgets seed from the store when one is already attached. */
    frontend_options_sync(fe);
    return main_ok;
}

int fe_record_for_level(const FrontEnd* fe, const char* level)
{
    if (fe == NULL || !fe->built || level == NULL || level[0] == '\0') {
        return -1;
    }
    for (int tutorial_index = 0; tutorial_index < fe->tutorial_n;
         tutorial_index++) {
        if (s_strcaseeq(level, fe->tutorial[tutorial_index])) {
            return tutorial_index;
        }
    }
    for (int campaign_index =  0; campaign_index < fe->campaign_n;
         campaign_index++) {
        if (s_strcaseeq(level, fe->campaign[campaign_index])) {
            return fe->tutorial_n + campaign_index;
        }
    }
    return -1;
}

int fe_level_for_record(const FrontEnd* fe, int rec, char* out, size_t sz)
{
    if (fe == NULL || !fe->built || out == NULL || rec < 0) {
        return 0;
    }
    if (rec < fe->tutorial_n) {
        snprintf(out, sz, "%s", fe->tutorial[rec]);
        return 1;
    }
    rec -= fe->tutorial_n;
    if (rec < fe->campaign_n) {
        snprintf(out, sz, "%s", fe->campaign[rec]);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00416d00
int frontend_enabled(BlockFile* cfg)
{    if (cfg == NULL) {
        return 0;
    }
    /* FrontEndActive @ 0x416d00: the cfg flag (loader-mode <3 is Win32). */
    return block_get_bool(cfg, "Lego* Main FrontEnd") == 1;
}

/* SaveMenuListEntryAppend (FUN_00410740), adapted to FeItem instead of the
 * corpus's packed 0x20-byte resource row. */
// FUNCTION: LEGORR 0x00410740
static int fe_save_menu_list_entry_append(FeSubmenu* submenu,
                                          const FeSlot* slot, int row)
{
    if (submenu == NULL || slot == NULL || submenu->item_count < 0
        || submenu->item_count >= FE_ITEMS_MAX || row < 0) {
        return 0;
    }
    FeItem* item = &submenu->items[submenu->item_count];
    memset(item, 0, sizeof(*item));
    item->kind = FE_ITEM_NEXT;
    item->load_slot = slot->id;
    snprintf(item->label, FE_NAME_MAX, "%s", slot->label);
    snprintf(item->status, FE_NAME_MAX, "%s", slot->status);
    item->x = 48;
    item->y = 96 + row * 60;
    ++submenu->item_count;
    return 1;
}

/* Partial SaveMenuBuild (FUN_00415a10): populate save/load menus with
 * runtime slot rows; the corpus also resolves config keys and slot summaries. */
// FUNCTION: LEGORR 0x00415a10
int frontend_build_slots(FrontEnd* fe, const FeSlot* slots, int n)
{
    if (fe == NULL || slots == NULL || n <= 0) {
        return 0;
    }
    if (n > FE_SLOT_COUNT) {
        n = FE_SLOT_COUNT;
    }
    fe->slot_count = n;
    for (int i = 0; i < n; i++) {
        fe->slots[i] = slots[i];
    }
    int grafted = 0;
    /* both the main menu's Load_A_Saved_Game screen and the SaveMenu carry
     * the slot rows (SaveMenuBuild @ 0x415a10's runtime rows; the screens
     * are identified by their SaveLoad art). */
    for (int mi = 0; mi < FE_MENU_COUNT; mi++) {
        FeMenu* m = &fe->menus[mi];
        for (int si = 0; si < m->submenu_count; si++) {
            FeSubmenu* sub = &m->submenus[si];
            if (!s_stristr(sub->image, "SaveLoad")) {
                continue;
            }
            int base = sub->item_count;
            for (int k = 0; k < n && base + k < FE_ITEMS_MAX; k++) {
                /* SaveMenuBuild constructs the slot list with a 0x3c-pixel
                 * row interval; MenuTick advances each list row by it. */
                (void)fe_save_menu_list_entry_append(sub, &slots[k], k);
            }
            grafted = 1;
        }
    }
    return grafted;
}

/* Refresh a save/load row after its backing slot changes. The corpus
 * rebuilds the selected save descriptor from the slot tree; this updates the
 * frontend's cached row and status text without reconstructing its menus. */
int frontend_update_slot(FrontEnd* fe, const FeSlot* slot)
{
    if (fe == NULL || slot == NULL || slot->id <= 0
        || slot->id >= SAVE_SLOTS) {
        return 0;
    }
    int found = 0;
    for (int i = 0; i < fe->slot_count; ++i) {
        if (fe->slots[i].id == slot->id) {
            fe->slots[i] = *slot;
            found = 1;
        }
    }
    for (int mi = 0; mi < FE_MENU_COUNT; ++mi) {
        FeMenu* menu = &fe->menus[mi];
        for (int si = 0; si < menu->submenu_count; ++si) {
            FeSubmenu* sub = &menu->submenus[si];
            for (int ii = 0; ii < sub->item_count; ++ii) {
                FeItem* item = &sub->items[ii];
                if (item->load_slot != slot->id) {
                    continue;
                }
                snprintf(item->label, sizeof(item->label), "%s",
                         slot->label);
                snprintf(item->status, sizeof(item->status), "%s",
                         slot->status);
                found = 1;
            }
        }
    }
    return found;
}

typedef struct FeSavMenuListBuild {
    FrontEnd* fe;
    FeLevelMenuList* list;
    const SavListWindow* registry;
    const SavTreeNode* seen[FE_LEVEL_MENU_ROWS * 4];
    int seen_count;
} FeSavMenuListBuild;

static void fe_sav_menu_list_walk(FeSavMenuListBuild* build,
                                  const SavTreeNode* node)
{
    if (build == NULL || node == NULL || build->list == NULL
        || build->registry == NULL || node->window >= (uint32_t)build->registry->count
        || build->registry->entries == NULL
        || build->list->row_count >= FE_LEVEL_MENU_ROWS) {
        return;
    }
    for (int i = 0; i < build->seen_count; ++i) {
        if (build->seen[i] == node) {
            return;
        }
    }
    if (build->seen_count >= FE_LEVEL_MENU_ROWS * 4) {
        return;
    }
    build->seen[build->seen_count++] = node;

    const char* name = (const char*)build->registry->entries[node->window];
    if (name == NULL || name[0] == '\0') {
        return;
    }
    const char* group = fe_level_group_name(name);
    FeLevelMenuRow* row = &build->list->rows[build->list->row_count++];
    memset(row, 0, sizeof(*row));
    snprintf(row->level, sizeof(row->level), "%s", group);
    char label[FE_NAME_MAX];
    if (fe_cfg_str(build->fe->cfg, group, "FullName", label,
                   sizeof(label)) != NULL) {
        snprintf(row->full_name, sizeof(row->full_name), "%s",
                 ui_text_underscores_to_spaces(label));
    } else {
        snprintf(row->full_name, sizeof(row->full_name), "%s", group);
    }
    row->x = fe_cfg_int(build->fe->cfg, group, "FrontEndX", 0);
    row->y = fe_cfg_int(build->fe->cfg, group, "FrontEndY", 0);
    (void)fe_cfg_str(build->fe->cfg, group, "MenuBMP", row->menu_bmp,
                     sizeof(row->menu_bmp));
    char path[FE_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s Levels %s Open",
             util_root_name(build->fe->cfg), group);
    int open = block_get_bool(build->fe->cfg, path);
    if (open == 2) {
        snprintf(path, sizeof(path), "%s %s Open",
                 util_root_name(build->fe->cfg), group);
        open = block_get_bool(build->fe->cfg, path);
    }
    row->open = open == 1;

    const SavTreeNode* const* children =
        (const SavTreeNode* const*)node->children;
    for (uint32_t child_index = 0;
         children != NULL && child_index < node->count; ++child_index) {
        fe_sav_menu_list_walk(build, children[child_index]);
    }
}

/* SavMenuList (FUN_004179b0): snapshot the selected campaign/tutorial tree
 * into the port's typed level-row list. The native function emits packed
 * menu records and loads MenuBMP resources; this model preserves tree order,
 * level coordinates, Open state, and image keys for the frontend renderer. */
// FUNCTION: LEGORR 0x004179b0
int frontend_sav_menu_list(FrontEnd* fe, int list_id,
                           const SavTreeNode* root,
                           const SavListWindow* registry)
{
    if (fe == NULL || !fe->built || root == NULL || registry == NULL
        || (list_id != 4 && list_id != 5)) {
        return 0;
    }
    FeLevelMenuList* list = &fe->level_menu_lists[list_id - 4];
    memset(list, 0, sizeof(*list));
    list->native_id = list_id;
    snprintf(list->title, sizeof(list->title), "Levels...");
    FeSavMenuListBuild build;
    memset(&build, 0, sizeof(build));
    build.fe = fe;
    build.list = list;
    build.registry = registry;
    fe_sav_menu_list_walk(&build, root);
    return list->row_count;
}

const FeLevelMenuList* frontend_level_menu_list_peek(const FrontEnd* fe,
                                                      int list_id)
{
    if (fe == NULL || (list_id != 4 && list_id != 5)) {
        return NULL;
    }
    return &fe->level_menu_lists[list_id - 4];
}

static int fe_sav_menu_list_from_config(FrontEnd* fe, int list_id,
                                        const char* start_level)
{
    enum { LEVEL_REGISTRY_MAX = 256 };
    if (fe == NULL || fe->cfg == NULL || start_level == NULL
        || start_level[0] == '\0') {
        return 0;
    }
    char path[FE_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s Levels", util_root_name(fe->cfg));
    BlockRec* levels = block_subgroup(fe->cfg, path);
    if (levels == NULL) {
        return 0;
    }

    void* entries[LEVEL_REGISTRY_MAX];
    void* values_a[LEVEL_REGISTRY_MAX];
    void* values_b[LEVEL_REGISTRY_MAX];
    memset(entries, 0, sizeof(entries));
    memset(values_a, 0, sizeof(values_a));
    memset(values_b, 0, sizeof(values_b));
    int count = 0;
    for (BlockRec* rec = block_first_child(levels);
         rec != NULL && count < LEVEL_REGISTRY_MAX;
         rec = block_next_sibling(rec)) {
        entries[count++] = (void*)block_name(rec);
    }
    if (count == 0) {
        return 0;
    }

    SavListWindow registry;
    registry.count = count;
    registry.entries = entries;
    registry.values_a = values_a;
    registry.values_b = values_b;
    SavTreeNode* root = level_node_get(
        &registry, fe->cfg, fe_level_group_name(start_level));
    int rows = frontend_sav_menu_list(fe, list_id, root, &registry);

    /* The native globals keep these trees for the process lifetime. The
     * frontend stores a flattened snapshot, so release the temporary graph. */
    for (int i = 0; i < count; ++i) {
        SavTreeNode* node = (SavTreeNode*)values_a[i];
        if (node != NULL) {
            engine_heap_free(node->children);
            engine_heap_free(node);
        }
    }
    return rows;
}

/* MenuShow (FUN_00413e20): enter a menu at submenu zero. The legacy routine
 * runs its menu loop synchronously; FrontEnd leaves the same state open for
 * the application's frame loop to advance. */
// FUNCTION: LEGORR 0x00413e20
int frontend_open(FrontEnd* fe, int menu)
{
    if (fe == NULL || !fe->built || menu < 0 || menu >= FE_MENU_COUNT) {
        return 0;
    }
    fe_overlay_close_all(fe);
    fe->menu = menu;
    fe->save_action = 0;
    fe->sub = 0;
    fe->sel = 0;
    fe->open = 1;
    fe->quit = 0;
    fe->pending_level[0] = '\0';
    /* Issue #85: re-entry shows the effective option values, never the
     * build-time defaults — sync every widget from the store. */
    frontend_options_sync(fe);
    fe_overlay_step(fe); /* open the active submenu's flic overlays */
    /* corpus DAT_005574e0 reset @ 0x413d29: the plane returns to the top
     * whenever the menu (re-)enters. */
    for (int i = 0; i < FE_SUBMENUS_MAX; i++) {
        fe->menus[menu].submenus[i].scroll = 0;
    }
    return 1;
}

/* FrontEndStateChange (FUN_004169c0): prepare state before MenuEnter chooses
 * a menu. The model records the selector and refreshes option widgets;
 * native save-tree/current-context mutations remain in the savegame APIs. */
// FUNCTION: LEGORR 0x004169c0
int frontend_state_change(FrontEnd* fe, int state)
{
    if (fe == NULL || !fe->built) {
        return 0;
    }
    if (state < FE_STATE_MAIN || state > FE_STATE_SAVE_REWARD) {
        return 0;
    }
    fe->flow_state = state;
    frontend_options_sync(fe);
    return 1;
}

/* MenuEnter (FUN_00416c80): FrontEndStateChange prepares the flow, then the
 * native routine selects submenu 0/1/2 in MainMenuFull for modes 0/1/2;
 * mode 4 selects submenu 0 in the separate save-menu set. Mode 3 performs
 * cleanup without showing a menu. */
// FUNCTION: LEGORR 0x00416c80
int menu_enter(FrontEnd* fe, int mode)
{
    if (!frontend_state_change(fe, mode)) {
        return 0;
    }
    if (mode == FE_STATE_OPTIONS) {
        frontend_close(fe);
        return 1;
    }

    const int menu = mode == FE_STATE_SAVE_REWARD ? FE_MENU_SAVE : FE_MENU_MAIN;
    const int submenu = mode == FE_STATE_SAVE_REWARD ? 0 : mode;
    if (submenu >= fe->menus[menu].submenu_count
        || !frontend_open(fe, menu)) {
        return 0;
    }
    if (submenu != 0) {
        fe->sub = submenu;
        fe->sel = 0;
        fe_overlay_step(fe);
    }
    if (mode == FE_STATE_SAVE_REWARD) {
        fe->save_action = 1;
    }
    return 1;
}

/* SaveMenuOpenFromReward (FUN_00462030): the original runs a synchronous
 * save-slot dialog. The frame loop owns input/rendering here, so open that
 * same menu in save-selection mode and let it advance one frame at a time. */
// FUNCTION: LEGORR 0x00462030
int save_menu_open_from_reward(FrontEnd* fe)
{
    return menu_enter(fe, 4);
}

void frontend_set_audio(FrontEnd* fe, struct AudioPlayer* audio)
{
    if (fe != NULL) {
        fe->audio = audio;
    }
}

void frontend_close(FrontEnd* fe)
{
    if (fe != NULL) {
        fe->open = 0;
        fe_overlay_close_all(fe);
        interface_resource_handles_release(fe);
    }
}

/* InterfaceResourceHandlesRelease (FUN_00413b50): release the five cached
 * save-thumbnail surfaces when the menu loop exits. The corpus stores them
 * in a fixed global handle band; the frontend keeps those handles per instance. */
// FUNCTION: LEGORR 0x00413b50
void interface_resource_handles_release(FrontEnd* fe)
{
    if (fe == NULL) {
        return;
    }
    for (int i = 0; i < FE_SAVE_THUMBNAIL_COUNT; ++i) {
        if (fe->save_thumbnails[i] != NULL) {
            texture_release(fe->save_thumbnails[i]);
            fe->save_thumbnails[i] = NULL;
        }
    }
}

void frontend_save_thumbnail_invalidate(FrontEnd* fe, int slot)
{
    const int index = slot - 1;
    if (fe == NULL || index < 0 || index >= FE_SAVE_THUMBNAIL_COUNT) {
        return;
    }
    if (fe->save_thumbnails[index] != NULL) {
        texture_release(fe->save_thumbnails[index]);
        fe->save_thumbnails[index] = NULL;
    }
}

/* The menu callback enters the scrolling-text path using the resources
 * captured while the front end is initialized. The frame loop consumes the
 * request on its next tick. */
// FUNCTION: LEGORR 0x0040a0c0
static void scrolling_text_display(void* text, void* font, void* background)
{
    (void)text;
    (void)font;
    (void)background;
    s_main_menu_text_item_pending = 1;
}

// FUNCTION: LEGORR 0x004103d0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void main_menu_text_item(void)
{
    __asm {
        mov eax, DAT_00556f54
        mov ecx, DAT_0055613c
        mov edx, DAT_00556f50
        push eax
        push ecx
        push edx
        call scrolling_text_display
        add esp, 0x0c
        ret
    }
}
#else
void main_menu_text_item(void)
{
    scrolling_text_display(DAT_00556f50, DAT_0055613c, DAT_00556f54);
}
#endif

/* Pointer hit-testing (defined with the render helpers below) uses the same
 * placement arithmetic + measure-font widths as drawing. */
static int fe_item_pick(const FrontEnd* fe, int mx, int my,
                        const TextureSurface* measure, int* out_idx);
static int fe_scroll_measure(const FeSubmenu* sub);
static void fe_item_pos(const FrontEnd* fe, const FeItem* it, int i,
                        int w, const TextureSurface* measure, int* x, int* y);

/* Partial MenuSliderPointerHandler (FUN_00411870): the base slider-track
 * drag branch routed by MenuInputTick. The alternate arrow regions in the
 * original handler are not implemented by this port. */
// FUNCTION: LEGORR 0x00411870
static void fe_slider_pointer_handle(FrontEnd* fe, FeSubmenu* sub,
                                     const FeInput* in,
                                     const TextureSurface* measure)
{
    if (!in->mdown || fe->sel >= sub->item_count) {
        return;
    }
    FeItem* dit = &sub->items[fe->sel];
    if (dit->kind != FE_ITEM_SLIDER && dit->kind != FE_ITEM_REALSLIDER) {
        return;
    }
    int bx = 0, by = 0;
    fe_item_pos(fe, dit, fe->sel, 0, measure, &bx, &by);
    if (in->mouse_x < bx || in->mouse_x > bx + 100
        || in->mouse_y < by || in->mouse_y > by + 20) {
        return;
    }
    float f = (float)(in->mouse_x - bx) * 0.01f;
    if (f < 0.0f) {
        f = 0.0f;
    }
    if (f > 1.0f) {
        f = 1.0f;
    }
    int v = (int)(f * (float)(dit->max - dit->min) + 0.5f);
    if (v < dit->min) {
        v = dit->min;
    }
    if (v > dit->max) {
        v = dit->max;
    }
    if (v != dit->value) {
        dit->value = v;
        fe_options_write(fe, dit);
        fe->option_changed = 1;
    }
}

/* --- activation (Enter or a pointer click resolves to this) -------------- */

/* Partial FrontEndSelectedAction (FUN_00416d70): resolve the selected
 * campaign/tutorial picker row to its level key. Save-slot list resolution
 * remains in the frame-loop load path. */
// FUNCTION: LEGORR 0x00416d70
static const char* fe_selected_level_action(const FrontEnd* fe)
{
    const FeItem* it = frontend_active_item(fe);
    return it->level_start ? it->target : NULL;
}

/* FrontEndStartLevel (FUN_00415220): the original routine performs the
 * engine-global level transition. In this port the frame loop owns that
 * transition, so this helper records the selected level and emits the
 * existing start-level event after closing the menu. */
// FUNCTION: LEGORR 0x00415220
static FeEvent frontend_start_level(FrontEnd* fe, const char* level)
{
    if (fe == NULL || level == NULL || level[0] == '\0') {
        return FE_EVT_NONE;
    }
    snprintf(fe->pending_level, FE_LEVEL_NAME_MAX, "%s", level);
    frontend_close(fe);
    return FE_EVT_START_LEVEL;
}

static FeEvent fe_activate(FrontEnd* fe)
{
    FeMenu* m = &fe->menus[fe->menu];
    FeSubmenu* sub = &m->submenus[fe->sub];
    FeItem* it = &sub->items[fe->sel];

    /* save-slot row: load the slot (FE_EVT_LOAD_GAME, fe.load_slot set) */
    if (it->load_slot) {
        fe->load_slot = it->load_slot;
        int save_action = fe->save_action;
        frontend_close(fe);
        return save_action ? FE_EVT_SAVE_GAME : FE_EVT_LOAD_GAME;
    }

    /* picker row: start the selected level */
    const char* level_action = fe_selected_level_action(fe);
    if (level_action != NULL) {
        return frontend_start_level(fe, level_action);
    }

    /* positive Trigger flags resolve by menu context (the shipped buttons):
     * on the pause overlay they are Continue / the quit-confirm / the
     * restart-confirm; on the main menu the only one is the Quit confirm. */
    if (it->kind == FE_ITEM_TRIGGER && it->flag != 0) {
        if (fe->menu == FE_MENU_SAVE) {
            /* SaveMenu's configured image trigger is the back button. */
            frontend_close(fe);
            return FE_EVT_NONE;
        }
        if (fe->menu == FE_MENU_PAUSED) {
            switch (fe->sub) {
            case 0:   /* pause Menu1: Continue_Game / Replay_Objective */
                if (s_stristr(it->label, "continue")) {
                    frontend_close(fe);
                    return FE_EVT_RESUME;
                }
                return FE_EVT_NONE;   /* Replay_Objective: stub */
            case 2:   /* pause Menu3 = quit confirm ("Yes") */
                fe->quit = 1;
                frontend_close(fe);
                return FE_EVT_QUIT;
            case 3:   /* pause Menu4 = restart confirm ("Yes") */
                return frontend_start_level(
                    fe, fe->resume_level[0] != '\0' ? fe->resume_level
                                                     : it->target);
            default:  /* options submenus: Replay_Objective etc. — stub */
                return FE_EVT_NONE;
            }
        }
        fe->quit = 1;
        frontend_close(fe);
        return FE_EVT_QUIT;
    }

    /* MainMenuFull's Credits text row is a trigger with flag 0. In the
     * original menu builder it has a callback that opens the scrolling
     * credits display; the other plain triggers remain no-ops here. */
    if (it->kind == FE_ITEM_TRIGGER && fe->menu == FE_MENU_MAIN
        && s_stristr(it->label, "credits")) {
        return FE_EVT_CREDITS;
    }

    /* Next -> navigate submenus ("Menu%i" target) */
    if (it->kind == FE_ITEM_NEXT && it->target[0] != '\0'
        && s_stristr(it->target, "menu") != 0) {
        int idx = (int)strtol(it->target + 4, NULL, 10) - 1; /* "MenuN" */
        if (idx >= 0 && idx < m->submenu_count) {
            fe->sub = idx;
            fe->sel = 0;
            /* corpus resets DAT_005574e0 (the plane offset) when a menu
             * (re-)enters — the port keeps it per submenu. */
            m->submenus[idx].scroll = 0;
        }
        return FE_EVT_NONE;
    }

    /* Slider / Cycle / TextInput / plain Trigger (flag 0): no-op this
     * milestone (parsed + rendered, not editable). */
    return FE_EVT_NONE;
}

FeEvent frontend_tick(FrontEnd* fe, const FeInput* in,
                      const TextureSurface* measure)
{
    if (fe == NULL || in == NULL || !fe->open) {
        return FE_EVT_NONE;
    }
    if (s_main_menu_text_item_pending) {
        s_main_menu_text_item_pending = 0;
        return FE_EVT_CREDITS;
    }
    FeMenu* m = &fe->menus[fe->menu];
    if (fe->sub < 0 || fe->sub >= m->submenu_count) {
        fe->sub = 0;
    }
    FeSubmenu* sub = &m->submenus[fe->sub];
    if (sub->item_count <= 0) {
        fe_overlay_step(fe); /* still advance any overlay animations */
        return FE_EVT_NONE;
    }
    if (fe->sel >= sub->item_count) {
        fe->sel = sub->item_count - 1;
    }
    if (fe->sel < 0) {
        fe->sel = 0;
    }
    /* .flh overlay lane (finding 23): advance the active clip, or let
     * PlayRandom's per-tick gate start one configured overlay. */
    fe_overlay_step(fe);

    if (in->up) {
        if (fe->sel > 0) {
            --fe->sel;
        }
    }
    if (in->down) {
        if (fe->sel < sub->item_count - 1) {
            ++fe->sel;
        }
    }

    /* Slider / Cycle adjustment: left/right steps the value within
     * [min,max] and flags option_changed for the frame loop to apply. */
    {
        FeItem* dit = &sub->items[fe->sel];
        if (dit->kind == FE_ITEM_SLIDER || dit->kind == FE_ITEM_REALSLIDER
            || dit->kind == FE_ITEM_CYCLE) {
            if (in->right && dit->value < dit->max) {
                dit->value++;
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
            if (in->left && dit->value > dit->min) {
                dit->value--;
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
        }
    }

    /* CanScroll edge pan (corpus FUN_00413970 @ 0x413970 + DAT_005574e0
     * in FUN_00412c00's item placement): while the cursor is in the outer
     * bands past the deadzone the item plane scrolls, velocity proportional
     * to distance from the band edge, clamped to [view_h - content, 0].
     * The corpus gates y against absolute bands for a 480-high screen
     * (< 200 scrolls, 200..280 deadzone) and velocity ~(50 - y/4)^2; this
     * port keeps the 5/12-h + h/12 band shape and linear distance velocity
     * capped at the corpus's ~50 px/frame. */
    if (sub->can_scroll) {
        if (sub->scroll_extent <= 0) {
            sub->scroll_extent = fe_scroll_measure(sub);
        }
        int vh = (fe->view_h > 0) ? fe->view_h : 480;
        int band = vh / 12;
        int cy = vh / 2;
        int lo = vh - sub->scroll_extent;
        if (lo > 0) {
            lo = 0;
        }
        if (sub->scroll < lo) {
            sub->scroll = lo;
        }
        if (sub->scroll > 0) {
            sub->scroll = 0;
        }
        if (in->mouse_y >= 0) {
            int d = 0, dir = 0;
            if (in->mouse_y < cy - band) {
                d = (cy - band) - in->mouse_y;
                dir = 1;                       /* show earlier rows */
            } else if (in->mouse_y > cy + band) {
                d = in->mouse_y - (cy + band);
                dir = -1;                      /* show later rows */
            }
            if (d > 0) {
                sub->scroll += dir * (d > 50 ? 50 : d);
                if (sub->scroll > 0) {
                    sub->scroll = 0;
                }
                if (sub->scroll < lo) {
                    sub->scroll = lo;
                }
            }
        }
    }

    /* pointer: hover selects; a click activates the hovered row (Enter).
     * When the pointer is in use but outside every item, a click is a
     * no-op (the menu stays up). */
    if (in->mouse_x >= 0 && in->mouse_y >= 0) {
        fe->pointer_x = in->mouse_x;
        fe->pointer_y = in->mouse_y;
        int idx = 0;
        int hit = fe_item_pick(fe, in->mouse_x, in->mouse_y, measure, &idx);
        if (hit) {
            fe->sel = idx;
        }

        /* Slider pointer drag (corpus FUN_00411a00 case 3 routes a held
         * button through FUN_00411870 every frame): with the pointer held
         * inside the bar's 100x20 rect anchored at the selected item's
         * origin, the value continuously tracks pointer x —
         *   v = round(clamp((mx - x) * 0.01, 0.0, 1.0) * (max - min))
         * (constants 0x49e170/174/178 = 0.0 / 1.0 / 0.01, rect w/h =
         * 0x64/0x14). The corpus mouse path omits the +min offset that the
         * keyboard path keeps; every shipped slider is min 0, and the port
         * clamps to [min,max] so a hypothetical min>0 row cannot sink below
         * its floor. */
        fe_slider_pointer_handle(fe, sub, in, measure);

        if (in->click) {
            if (hit) {
                return fe_activate(fe);
            }
            return FE_EVT_NONE;
        }
    }

    if (in->esc) {
        if (fe->sub != 0) {
            fe->sub = 0;
            fe->sel = 0;
        } else if (fe->menu == FE_MENU_PAUSED) {
            frontend_close(fe);       /* pause Esc = resume */
            return FE_EVT_RESUME;
        } else {
            fe->quit = 1;
            frontend_close(fe);
            return FE_EVT_QUIT;
        }
        return FE_EVT_NONE;
    }
    if (!in->enter) {
        return FE_EVT_NONE;
    }
    return fe_activate(fe);
}

const FeMenu* frontend_menu(const FrontEnd* fe, int menu)
{
    static const FeMenu s_empty = { 0 };
    if (fe == NULL || menu < 0 || menu >= FE_MENU_COUNT) {
        return &s_empty;
    }
    return &fe->menus[menu];
}

const FeSubmenu* frontend_active_submenu(const FrontEnd* fe)
{
    static const FeSubmenu s_empty = { 0 };
    if (fe == NULL || !fe->open || fe->menu < 0 || fe->menu >= FE_MENU_COUNT) {
        return &s_empty;
    }
    const FeMenu* m = &fe->menus[fe->menu];
    if (fe->sub < 0 || fe->sub >= m->submenu_count) {
        return &s_empty;
    }
    return &m->submenus[fe->sub];
}

const FeItem* frontend_active_item(const FrontEnd* fe)
{
    static const FeItem s_empty = { 0 };
    const FeSubmenu* s = frontend_active_submenu(fe);
    if (s == NULL || fe->sel < 0 || fe->sel >= s->item_count) {
        return &s_empty;
    }
    return &s->items[fe->sel];
}

const char* frontend_active_image(const FrontEnd* fe)
{
    const FeSubmenu* s = frontend_active_submenu(fe);
    return (s != NULL && s->image[0] != '\0') ? s->image : NULL;
}

/* --- render -----------------------------------------------------------------
 * Software blit of the active submenu (finding 22 §3 lane). Background and
 * MenuImage blits are the caller's; this draws the item layer with the
 * shipped bitmap fonts (HiFont selected / LoFont unselected, FUN_0047a470
 * 10x19 grid), falling back to the port's own 3x5 typeface when the frame
 * loop supplies none. Selection = the classic highlight bar + bright face.
 * ------------------------------------------------------------------------- */

/* 3x5 glyphs (5 rows, bit2 = left column). Uppercase + digits + the
 * punctuation the shipped labels use. Our typeface ("palette is ours" —
 * README "Software level renderer"), not a clone. */
static const int8_t s_font_caps[26][5] = {
    {5,7,5,5,5}, {7,5,7,5,7}, {7,4,4,4,7}, {6,5,5,5,6},
    {7,4,7,4,7}, {7,4,7,4,4}, {7,5,4,5,7}, {5,5,7,5,5},
    {2,2,7,2,2}, {1,1,2,3,7}, {5,6,4,6,5}, {4,4,4,4,7},
    {7,5,5,5,7}, {6,5,5,5,6}, {7,5,5,5,7}, {7,5,7,4,4},
    {7,5,4,5,7}, {7,5,7,4,7}, {7,4,1,2,7}, {7,4,7,4,4},
    {7,5,5,5,7}, {5,5,5,5,7}, {5,5,5,5,7}, {5,2,0,2,5},
    {5,5,5,5,7}, {7,1,2,4,7},
};
static const int8_t s_font_digits[10][5] = {
    {7,5,5,5,7}, {2,6,2,2,7}, {7,4,2,1,7}, {7,1,2,3,7},
    {6,6,7,2,2}, {7,4,1,2,7}, {7,4,6,4,7}, {7,1,1,1,7},
    {7,5,7,5,7}, {7,5,7,4,7},
};
static const int8_t s_font_fallback[5] = { 7, 7, 7, 7, 7 };

static const int8_t* fe_glyph(int ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        return s_font_caps[ch - 'A'];
    }
    if (ch >= 'a' && ch <= 'z') {
        return s_font_caps[s_toupper(ch) - 'A'];
    }
    if (ch >= '0' && ch <= '9') {
        return s_font_digits[ch - '0'];
    }
    switch (ch) {
    case ' ':  return NULL;              /* zero-width */
    case '!':  { static const int8_t g[5] = {2,2,0,2,2}; return g; }
    case '?':  { static const int8_t g[5] = {3,1,2,5,7}; return g; }
    case '-':  { static const int8_t g[5] = {0,0,7,0,0}; return g; }
    case '_':  { static const int8_t g[5] = {0,0,0,0,7}; return g; }
    case '.':  { static const int8_t g[5] = {0,0,0,0,1}; return g; }
    case ',':  { static const int8_t g[5] = {0,0,0,1,1}; return g; }
    case ':':  { static const int8_t g[5] = {0,2,0,2,0}; return g; }
    case '\'': { static const int8_t g[5] = {1,1,0,0,0}; return g; }
    case '/':  { static const int8_t g[5] = {1,2,2,2,4}; return g; }
    case '"':  { static const int8_t g[5] = {5,5,0,0,0}; return g; }
    default:
        return s_font_fallback;
    }
}

#define FE_FONT_SCALE 2

static void fe_set_px(void* px, int pitch, int w, int h, int x, int y,
                      uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || y < 0 || x >= w || y >= h) {
        return;
    }
    uint8_t* row = (uint8_t*)px + (size_t)y * (size_t)pitch + (size_t)x * 4;
    row[0] = r; row[1] = g; row[2] = b; row[3] = 255;
}

static int fe_text_width(const char* s)
{
    return (int)strlen(s) * (3 * FE_FONT_SCALE + 1);
}

static void fe_draw_text(void* px, int pitch, int w, int h, int x, int y,
                         const char* s, int r, int g, int b)
{
    for (int i = 0; s[i] != '\0' && x < w; ++i) {
        int c = (unsigned char)s[i];
        int sc = (c == '_') ? ' ' : s_toupper(c);
        const int8_t* glyph = fe_glyph(sc);
        if (glyph == NULL) {              /* space: no ink, just advance */
            x += 3 * FE_FONT_SCALE + 1;
            continue;
        }
        for (int row = 0; row < 5; row++) {
            int bits = glyph[row];
            for (int col = 0; col < 3; col++) {
                if (bits & (1 << (2 - col))) {
                    for (int sy = 0; sy < FE_FONT_SCALE; sy++) {
                        for (int sx = 0; sx < FE_FONT_SCALE; sx++) {
                            fe_set_px(px, pitch, w, h,
                                      x + col * FE_FONT_SCALE + sx,
                                      y + row * FE_FONT_SCALE + sy,
                                      (uint8_t)r, (uint8_t)g, (uint8_t)b);
                        }
                    }
                }
            }
        }
        x += 3 * FE_FONT_SCALE + 1;
    }
}

/* RenderUiFrameCell @ 0x412450 selects either a text row or an image frame.
 * The port's software menu framebuffer uses the same 3x5 text fallback as
 * other menu overlays and the shared clipped texture blitter for images. */
// FUNCTION: LEGORR 0x00412450
void menu_render_ui_frame_cell(void* px, int pitch, int w, int h,
                               int x, int y, int text_row,
                               const char* text,
                               const TextureSurface* image)
{
    if (px == NULL || w <= 0 || h <= 0) {
        return;
    }
    if (text_row != 0) {
        if (text != NULL) {
            fe_draw_text(px, pitch, w, h, x, y, text, 220, 220, 220);
        }
        return;
    }
    if (image == NULL || image->width <= 0 || image->height <= 0 ||
        x >= 640 || x <= -image->width ||
        y <= -image->height || y >= 480) {
        return;
    }
    surface_blit_texture(px, pitch, w, h, image, x, y);
}

static void fe_blit_texture_crop(void* px, int pitch, int w, int h,
                                 const TextureSurface* image,
                                 int source_x, int source_width,
                                 int source_height, int x, int y)
{
    if (px == NULL || image == NULL || image->pixels == NULL ||
        source_width <= 0 || image->height <= 0) {
        return;
    }
    int sx0 = source_x > 0 ? source_x : 0;
    int sx1 = source_x + source_width;
    if (sx1 > image->width) sx1 = image->width;
    int sy0 = 0;
    int sy1 = source_height < image->height ? source_height : image->height;
    int dx = x + (sx0 - source_x);
    int dy = y;
    if (dx < 0) {
        sx0 -= dx;
        dx = 0;
    }
    if (dy < 0) {
        sy0 -= dy;
        dy = 0;
    }
    int copy_width = sx1 - sx0;
    int copy_height = sy1 - sy0;
    if (copy_width > w - dx) copy_width = w - dx;
    if (copy_height > h - dy) copy_height = h - dy;
    if (copy_width <= 0 || copy_height <= 0 || dx >= w || dy >= h) {
        return;
    }
    for (int row_index = 0; row_index < copy_height; ++row_index) {
        unsigned char* dst = (unsigned char*)px +
            (size_t)(dy + row_index) * (size_t)pitch + (size_t)dx * 4;
        const uint32_t* src = image->pixels +
            (size_t)(sy0 + row_index) * (size_t)image->width + sx0;
        for (int col = 0; col < copy_width; ++col) {
            uint32_t pixel = src[col];
            dst[col * 4 + 0] = (unsigned char)(pixel >> 16);
            dst[col * 4 + 1] = (unsigned char)(pixel >> 8);
            dst[col * 4 + 2] = (unsigned char)pixel;
            dst[col * 4 + 3] = (unsigned char)(pixel >> 24);
        }
    }
}

/* PercentSplitSurfaceBlit (FUN_00411f10): two ten-pixel-high source lanes
 * are split at (numerator * 100) / denominator. The original packed record
 * stores the images in the same order represented by FePercentSplitSurface;
 * optional overlays and two independently hoverable handles are layered on
 * top. The handle hit rectangles use the normal image dimensions, matching
 * MenuPointerRectangleHitTest's inclusive edges. */
// FUNCTION: LEGORR 0x00411F10
void menu_percent_split_surface_blit(void* px, int pitch, int w, int h,
                                     const FePercentSplitSurface* surface,
                                     int x, int y, int numerator,
                                     int denominator, int mouse_x,
                                     int mouse_y)
{
    if (px == NULL || surface == NULL || denominator == 0) {
        return;
    }
    const int percent = (int)(((int64_t)numerator * 100) / denominator);
    fe_blit_texture_crop(px, pitch, w, h, surface->left_fill,
                         0, percent, 10, x, y);
    fe_blit_texture_crop(px, pitch, w, h, surface->right_fill,
                         percent, 100 - percent, 10, x + percent, y);
    if (surface->left_overlay != NULL) {
        surface_blit_texture(px, pitch, w, h, surface->left_overlay, x, y);
    }
    if (surface->right_overlay != NULL) {
        surface_blit_texture(px, pitch, w, h, surface->right_overlay,
                             x + 100, y);
    }

    if (surface->left_idle != NULL && surface->left_hover != NULL) {
        const int handle_x = x - (surface->left_idle->width + 4);
        const int hot = menu_pointer_rectangle_hit_test(
            mouse_x, mouse_y, handle_x, y, surface->left_idle->width,
            surface->left_idle->height);
        surface_blit_texture(px, pitch, w, h,
                             hot ? surface->left_hover : surface->left_idle,
                             handle_x, y);
    }
    if (surface->right_idle != NULL && surface->right_hover != NULL) {
        const int right_overlay_width = surface->right_overlay != NULL
                                            ? surface->right_overlay->width : 0;
        const int handle_x = x + right_overlay_width + 104;
        const int hot = menu_pointer_rectangle_hit_test(
            mouse_x, mouse_y, handle_x, y, surface->right_idle->width,
            surface->right_idle->height);
        surface_blit_texture(px, pitch, w, h,
                             hot ? surface->right_hover : surface->right_idle,
                             handle_x, y);
    }
}

static void fe_fill_rect(void* px, int pitch, int w, int h,
                         int x, int y, int rw, int rh, int r, int g, int b)
{
    for (int yy = 0; yy < rh; yy++) {
        for (int xx = 0; xx < rw; xx++) {
            fe_set_px(px, pitch, w, h, x + xx, y + yy,
                      (uint8_t)r, (uint8_t)g, (uint8_t)b);
        }
    }
}

/* The in-game Help Window panel (issue #85). Corpus model: HelpWindow's
 * BIG-screen backdrop with info text; the Help_Window option is the
 * display gate — it lives here, so the frame loop and every caller share
 * one gate and the option can never draw without the store saying On.
 * The port's face is a bottom-left panel: title line plus `text` (the demo
 * key lines when NULL). */
void frontend_help_overlay(void* px, int pitch, int w, int h,
                           const char* text, const GameOptions* opts)
{
    if (opts != NULL && opts->help_window == 0) {
        return; /* Help_Window Off: the option gates the display. */
    }
    if (px == NULL || w <= 0 || h <= 0) {
        return;
    }
    int x0 = 16, y0 = (h > 96) ? h - 92 : 4;
    int pw = (w > 360) ? 360 : (w > 40 ? w - 32 : w);
    int ph = 80;
    if (y0 + ph > h) {
        ph = h - y0 - 4;
    }
    if (ph <= 0) {
        return;
    }
    fe_fill_rect(px, pitch, w, h, x0, y0, pw, ph, 10, 14, 34);
    /* 1-px frame */
    fe_fill_rect(px, pitch, w, h, x0, y0, pw, 1, 150, 160, 200);
    fe_fill_rect(px, pitch, w, h, x0, y0 + ph - 1, pw, 1, 150, 160, 200);
    fe_fill_rect(px, pitch, w, h, x0, y0, 1, ph, 150, 160, 200);
    fe_fill_rect(px, pitch, w, h, x0 + pw - 1, y0, 1, ph, 150, 160, 200);
    fe_draw_text(px, pitch, w, h, x0 + 8, y0 + 6, "HELP", 235, 225, 150);
    fe_draw_text(px, pitch, w, h, x0 + 8, y0 + 26,
                 (text != NULL && text[0] != '\0')
                     ? text : "ESC PAUSE  ENTER SELECT  SPACE SFX",
                 200, 205, 220);
    fe_draw_text(px, pitch, w, h, x0 + 8, y0 + 46,
                 "C ORE   CRYSTALS   B SAVE   Y REWARD",
                 170, 175, 195);
}

/* --- shipped bitmap-font lane (FUN_0047a470) ------------------------------ */

/* Advance is measured from each cell's ink bounding box (the engine stores
 * per-glyph float advances in the font object at load; we re-derive the
 * same numbers from the image). */

static int fe_font_cell(const TextureSurface* f, int* cw, int* ch)
{
    if (f == NULL || f->pixels == NULL || f->width < FE_FONT_GRID_COLS
        || f->height < FE_FONT_GRID_ROWS) {
        return 0;
    }
    *cw = f->width / FE_FONT_GRID_COLS;
    *ch = f->height / FE_FONT_GRID_ROWS;
    return 1;
}

/* Fallback key derivation (port-side stand-in, documented deviation): when
 * a surface carries no palette key info — synthetic fonts, 24-bit sources
 * — the key is sampled as the dominant colour of the grid (glyph 0's cell
 * is background in every observed font). FE_FONT_KEY stays as the generic
 * default for a fully blank sample. */
static uint32_t fe_font_key(const TextureSurface* f, int cw, int ch)
{
    uint32_t colors[8];
    unsigned count[8] = { 0 };
    int n = 0;
    for (int yy = 0; yy < ch; yy += (ch > 4 ? 2 : 1)) {
        for (int xx = 0; xx < cw; xx += (cw > 4 ? 2 : 1)) {
            uint32_t p = f->pixels[(size_t)yy * (size_t)f->width + xx];
            int i = 0;
            for (; i < n; i++) {
                if (colors[i] == p) {
                    break;
                }
            }
            if (i == n) {
                if (n == 8) {
                    continue;
                }
                colors[n] = p;
                count[n] = 1;
                n++;
            } else {
                count[i]++;
            }
        }
    }
    if (n == 0) {
        return FE_FONT_KEY;
    }
    unsigned bestc = 0;
    uint32_t best = colors[0];
    for (int i = 1; i < n; i++) {
        if (count[i] > bestc) {
            bestc = count[i];
            best = colors[i];
        }
    }
    return best;
}

/* Face keys — the corpus path (FUN_0047d9d0 + FUN_0047e050): every 8bpp
 * font surface carries palette[0] and palette[255] as colour keys, both
 * blitted transparent. Surfaces without a decoded palette (key_count 0)
 * keep the sampled stand-in for both slots. */
static void fe_font_keys(const TextureSurface* f, int cw, int ch,
                         uint32_t keys[2])
{
    if (f->key_count == 2) {
        keys[0] = f->keys[0];
        keys[1] = f->keys[1];
        return;
    }
    keys[0] = keys[1] = fe_font_key(f, cw, ch);
}

static int fe_font_key_hit(uint32_t p, const uint32_t keys[2])
{
    return p == keys[0] || p == keys[1];
}

/* Ink bounding box of glyph g — the non-key pixels only. The engine blits
 * exactly this box (FUN_00479ee0's right-edge scan / per-glyph rect), which
 * keeps each face's background AND frame strips out while retaining the
 * glyph's dark outline when it is inside the box. */
// FUNCTION: LEGORR 0x00479ee0
static int fe_font_ink_box(const TextureSurface* f, int cw, int ch, int g,
                           const uint32_t keys[2], int* minx, int* miny,
                           int* maxx, int* maxy)
{
    int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
    int x0 = col * cw, y0 = row * ch;
    *minx = cw; *miny = ch; *maxx = -1; *maxy = -1;
    for (int yy = 0; yy < ch; yy++) {
        for (int xx = 0; xx < cw; xx++) {
            if (fe_font_key_hit(
                    f->pixels[(size_t)(y0 + yy) * (size_t)f->width + (x0 + xx)],
                    keys)) {
                continue;
            }
            if (xx < *minx) { *minx = xx; }
            if (xx > *maxx) { *maxx = xx; }
            if (yy < *miny) { *miny = yy; }
            if (yy > *maxy) { *maxy = yy; }
        }
    }
    return *maxx >= 0;
}

/* Advance width of glyph g = its ink width from the cell's left edge
 * (the engine's right-edge scan), 0 when blank. */
static int fe_font_advance(const TextureSurface* f, int cw, int ch, int g,
                           const uint32_t keys[2])
{
    int mx, my, ax, ay;
    if (!fe_font_ink_box(f, cw, ch, g, keys, &mx, &my, &ax, &ay)) {
        return 0;
    }
    return ax + 1;
}

static void fe_blit_glyph(void* px, int pitch, int w, int h,
                          int dx, int dy, const TextureSurface* f,
                          int cw, int ch, int g, const uint32_t keys[2])
{
    int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
    int x0 = col * cw, y0 = row * ch;
    int minx, miny, maxx, maxy;
    if (!fe_font_ink_box(f, cw, ch, g, keys, &minx, &miny, &maxx, &maxy)) {
        return;
    }
    for (int yy = miny; yy <= maxy; yy++) {
        int dy2 = dy + yy;
        if (dy2 < 0 || dy2 >= h) {
            continue;
        }
        for (int xx = minx; xx <= maxx; xx++) {
            uint32_t p = f->pixels[(size_t)(y0 + yy) * (size_t)f->width
                                   + (x0 + xx)];
            if (fe_font_key_hit(p, keys)) {
                continue;               /* key *inside* the box */
            }
            int dx2 = dx + xx;
            if (dx2 < 0 || dx2 >= w) {
                continue;
            }
            uint8_t* o = (uint8_t*)px + (size_t)dy2 * (size_t)pitch
                         + (size_t)dx2 * 4;
            o[0] = (uint8_t)(p >> 16);
            o[1] = (uint8_t)(p >> 8);
            o[2] = (uint8_t)(p);
            o[3] = (uint8_t)(p >> 24);
        }
    }
}

/* Partial BitmapFontGlyphDrawAndMeasure (FUN_0047a470): combine the port's
 * software glyph blit with the glyph advance calculation. The corpus uses
 * its DirectDraw surface and stored float advances; this path clips RGBA
 * pixels and derives the advance from the glyph's ink bounds. */
// FUNCTION: LEGORR 0x0047a470
static int fe_font_glyph_draw_and_measure(void* px, int pitch, int w, int h,
                                          int x, int y,
                                          const TextureSurface* f,
                                          int cw, int ch, int g,
                                          const uint32_t keys[2], int draw)
{
    int advance = fe_font_advance(f, cw, ch, g, keys);
    if (draw != 0) {
        fe_blit_glyph(px, pitch, w, h, x, y, f, cw, ch, g, keys);
    }
    return advance;
}

/* Draw `s` with a bitmap font at (x,y). `_` renders as a space, matching
 * the shipped menu labels. Returns the advance. */
static int fe_draw_font_text(void* px, int pitch, int w, int h,
                             int x, int y, const char* s,
                             const TextureSurface* f)
{
    int cw = 0, chh = 0;
    if (!fe_font_cell(f, &cw, &chh)) {
        return 0;
    }
    uint32_t keys[2];
    fe_font_keys(f, cw, chh, keys);
    int cx = x;
    for (int i = 0; s[i] != '\0' && cx < w; ++i) {
        int c = (unsigned char)s[i];
        int g = c - 0x20;
        if (g < 0 || g >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS) {
            cx += cw / 2;
            continue;
        }
        int adv = fe_font_glyph_draw_and_measure(
            px, pitch, w, h, cx, y, f, cw, chh, g, keys, 1);
        if (adv <= 0) {
            adv = (c == ' ') ? cw / 2 : cw;        /* blank cell fallback */
        }
        cx += adv;
    }
    return cx - x;
}

/* FormattedTextDraw (FUN_0047a1b0): software-framebuffer draw of an already
 * resolved text value using the selected bitmap face. Format expansion is
 * performed at the caller boundary in this port. */
// FUNCTION: LEGORR 0x0047A1B0
void formatted_text_draw(const TextureSurface* font, void* px, int pitch,
                         int w, int h, int x, int y, const char* text)
{
    if (font == NULL || px == NULL || text == NULL || w <= 0 || h <= 0) {
        return;
    }
    formatted_text_draw_and_measure(font, px, pitch, w, h, x, y, text, NULL);
}

static int fe_font_text_width(const char* s, const TextureSurface* f)
{
    int cw = 0, chh = 0;
    if (!fe_font_cell(f, &cw, &chh)) {
        return 0;
    }
    uint32_t keys[2];
    fe_font_keys(f, cw, chh, keys);
    int cx = 0;
    for (int i = 0; s[i] != '\0'; ++i) {
        int c = (unsigned char)s[i];
        int g = c - 0x20;
        if (g < 0 || g >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS) {
            cx += cw / 2;
            continue;
        }
        int adv = fe_font_advance(f, cw, chh, g, keys);
        cx += adv > 0 ? adv : (c == ' ' ? cw / 2 : cw);
    }
    return cx;
}

static FeFormattedTextTextureResolve s_formatted_text_texture_resolve;

void formatted_text_texture_resolver_set(FeFormattedTextTextureResolve resolve)
{
    s_formatted_text_texture_resolve = resolve;
}

/* FormattedTextDrawAndMeasure (FUN_0047a1e0): draw decoded bitmap-font text,
 * returning its widest line and optionally the number of lines. Newlines
 * reset the pen and advance one glyph cell; tabs advance to the next eight-
 * cell stop. Inline @[0x........] handles are resolved through the port-side
 * hook, then blitted through the software surface path. */
// FUNCTION: LEGORR 0x0047A1E0
uint32_t formatted_text_draw_and_measure(const TextureSurface* font,
                                         void* px, int pitch, int w, int h,
                                         int x, int y, const char* text,
                                         int* line_count)
{
    if (line_count != NULL) {
        *line_count = 0;
    }
    if (font == NULL || text == NULL) {
        return 0;
    }

    int cell_w = 0;
    int cell_h = 0;
    if (!fe_font_cell(font, &cell_w, &cell_h)) {
        return 0;
    }
    uint32_t keys[2];
    fe_font_keys(font, cell_w, cell_h, keys);

    int pen_x = 0;
    int max_width = 0;
    int line_h = cell_h;
    int lines = 1;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n') {
            if (pen_x > max_width) max_width = pen_x;
            pen_x = 0;
            y += line_h;
            line_h = cell_h;
            ++lines;
            continue;
        }
        if (text[i] == '\t') {
            const int tab_width = cell_w * 8;
            if (tab_width > 0) {
                pen_x += tab_width - (pen_x % tab_width);
            }
            continue;
        }
        if (text[i] == '@' && text[i + 1] == '[' && text[i + 2] == '0'
            && text[i + 3] == 'x' && strlen(text + i) >= 13
            && text[i + 12] == ']') {
            uint32_t handle = 0;
            int valid = 1;
            for (int digit = 0; digit < 8; ++digit) {
                const char c = text[i + 4 + digit];
                uint32_t value;
                if (c >= '0' && c <= '9') value = (uint32_t)(c - '0');
                else if (c >= 'a' && c <= 'f') value = (uint32_t)(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') value = (uint32_t)(c - 'A' + 10);
                else { valid = 0; break; }
                handle = (handle << 4) | value;
            }
            const TextureSurface* image = valid && handle != 0
                && s_formatted_text_texture_resolve != NULL
                ? s_formatted_text_texture_resolve(handle) : NULL;
            if (image != NULL) {
                if (px != NULL && w > 0 && h > 0) {
                    surface_blit_texture(px, pitch, w, h, image,
                                         x + pen_x, y);
                }
                pen_x += image->width;
                if (image->height > line_h) line_h = image->height;
            }
            i += 12;
            continue;
        }

        const int c = (unsigned char)text[i];
        const int glyph = c - 0x20;
        int advance;
        if (glyph < 0 || glyph >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS) {
            advance = cell_w / 2;
        } else {
            advance = fe_font_glyph_draw_and_measure(
                px, pitch, w, h, x + pen_x, y, font, cell_w, cell_h, glyph,
                keys, px != NULL && w > 0 && h > 0);
            if (advance <= 0) {
                advance = (c == ' ') ? cell_w / 2 : cell_w;
            }
        }
        pen_x += advance;
    }
    if (pen_x > max_width) max_width = pen_x;
    if (line_count != NULL) {
        *line_count = lines;
    }
    return (uint32_t)max_width;
}

/* FormattedTextFormatDraw (FUN_0047a150): expand the engine's `%b` inline
 * texture token, format into bounded storage, then delegate drawing and
 * measurement to the decoded-text path. `%b` values are 32-bit handles
 * resolved by the software renderer hook. */
// FUNCTION: LEGORR 0x0047A150
void formatted_text_format_draw(const TextureSurface* font,
                                uint32_t* width_out, int* line_count,
                                void* px, int pitch, int w, int h,
                                int x, int y, const char* format,
                                va_list args)
{
    if (width_out != NULL) *width_out = 0;
    if (line_count != NULL) *line_count = 0;
    if (font == NULL || format == NULL) return;

    char expanded[1024];
    size_t used = 0;
    for (size_t i = 0; format[i] != '\0' && used + 1 < sizeof(expanded); ++i) {
        if (format[i] == '%' && format[i + 1] == 'b') {
            static const char texture_format[] = "@[0x%0.8x]";
            const size_t n = sizeof(texture_format) - 1;
            if (used + n >= sizeof(expanded)) break;
            memcpy(expanded + used, texture_format, n);
            used += n;
            ++i;
        } else {
            expanded[used++] = format[i];
        }
    }
    expanded[used] = '\0';

    char text[1024];
    formatted_text_buffer_build(text, sizeof(text), expanded, args);
    const uint32_t width = formatted_text_draw_and_measure(
        font, px, pitch, w, h, x, y, text, line_count);
    if (width_out != NULL) *width_out = width;
}

/* FormattedTextPrintf (FUN_00401c80): variadic front-end to the format
 * renderer. The original forwards its stack varargs to FormattedTextFormatDraw. */
// FUNCTION: LEGORR 0x00401C80
void formatted_text_printf(const TextureSurface* font,
                           uint32_t* width_out, int* line_count,
                           void* px, int pitch, int w, int h,
                           int x, int y, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    formatted_text_format_draw(font, width_out, line_count, px, pitch, w, h,
                               x, y, format, args);
    va_end(args);
}

/* RewardTextDraw (FUN_0047a180): positioned variadic convenience wrapper for
 * formatted UI text. The legacy routine forwards its format and argument
 * list to FormattedTextFormatDraw; the port supplies its software target. */
// FUNCTION: LEGORR 0x0047A180
void reward_text_draw(const TextureSurface* font, void* px, int pitch,
                      int w, int h, int x, int y, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    formatted_text_format_draw(font, NULL, NULL, px, pitch, w, h,
                               x, y, format, args);
    va_end(args);
}

/* FormattedTextEscapeDecodeAndEmit (FUN_00477590): format into the legacy
 * 256-byte scratch window, decode `\\n` to newline and `_` to space, then
 * duplicate the resulting text. The port returns the allocated copy to make
 * ownership usable by callers; the original engine heap retained it. */
// FUNCTION: LEGORR 0x00477590
char* formatted_text_escape_decode_and_emit(const char* format, ...)
{
    if (format == NULL) return NULL;
    char formatted[256];
    va_list args;
    va_start(args, format);
    const uint32_t result = formatted_text_buffer_build(
        formatted, sizeof(formatted), format, args);
    va_end(args);

    char decoded[256];
    size_t read = 0;
    size_t write = 0;
    while (read < result && formatted[read] != '\0'
           && write + 1 < sizeof(decoded)) {
        if (formatted[read] == '\\' && formatted[read + 1] == 'n') {
            decoded[write++] = '\n';
            read += 2;
        } else {
            decoded[write++] = formatted[read] == '_'
                                   ? ' ' : formatted[read];
            ++read;
        }
    }
    decoded[write] = '\0';
    return string_duplicate(decoded);
}

/* FormattedTextPrintfWidth (FUN_00401c50): measure a literal using the
 * selected menu bitmap face. Format expansion is performed by its draw
 * caller in the original engine; this boundary receives the resolved text. */
// FUNCTION: LEGORR 0x00401c50
uint32_t formatted_text_printf_width(const TextureSurface* font,
                                     const char* text)
{
    if (font == NULL || text == NULL) return 0;
    return (uint32_t)fe_font_text_width(text, font);
}

static void fe_menu_panel_text_draw(void* px, int pitch, int w, int h,
                                    const FeMenuPanelText* item,
                                    const TextureSurface* font)
{
    if (item == NULL || item->text == NULL) return;
    int text_x = item->x;
    if (item->centered) {
        text_x += (item->width - fe_font_text_width(item->text, font)) / 2;
    }
    formatted_text_draw(font, px, pitch, w, h, text_x, item->y, item->text);
}

/* MenuOptionPanelTextureUpdate (FUN_004355c0): software-framebuffer face of
 * the dialog panel updater. `FeMenuPanelText` carries the position and
 * centering data that the original text records hold in their packed words.
 * The two return values preserve the native OK/Cancel hover results. */
// FUNCTION: LEGORR 0x004355C0
int menu_option_panel_texture_update(const FeMenuOptionPanel* panel,
                                    void* px, int pitch, int w, int h,
                                    int mouse_x, int mouse_y)
{
    if (panel == NULL || px == NULL || panel->image == NULL ||
        panel->font == NULL || panel->title.text == NULL ||
        panel->body.text == NULL || w <= 0 || h <= 0) {
        return -1;
    }

    if (panel->contrast != NULL) {
        surface_blit_texture(px, pitch, w, h, panel->contrast,
                             panel->contrast_x, panel->contrast_y);
    }
    surface_blit_texture(px, pitch, w, h, panel->image,
                         (w - panel->image->width) / 2,
                         (h - panel->image->height) / 2);
    fe_menu_panel_text_draw(px, pitch, w, h, &panel->title, panel->font);
    fe_menu_panel_text_draw(px, pitch, w, h, &panel->body, panel->font);

    int result = -1;
    int cell_width = 0, cell_height = 0;
    fe_font_cell(panel->font, &cell_width, &cell_height);
    (void)cell_width;
    const FeMenuPanelText* buttons[2] = { &panel->ok, &panel->cancel };
    for (int i = 0; i < 2; ++i) {
        const FeMenuPanelText* button = buttons[i];
        if (button->text == NULL) continue;
        const int text_width = fe_font_text_width(button->text, panel->font);
        int text_x = button->x;
        if (button->centered) {
            text_x += (button->width - text_width) / 2;
        }
        const int hover = mouse_x >= text_x && mouse_x <= text_x + text_width &&
                          mouse_y >= button->y && mouse_y <= button->y + cell_height;
        const TextureSurface* font = hover && panel->hover_font != NULL
                                         ? panel->hover_font : panel->font;
        formatted_text_draw(font, px, pitch, w, h, text_x, button->y,
                            button->text);
        if (hover) result = (i == 0) ? 1 : 0;
    }
    return result;
}

/* OverlayMenuGate (FUN_00415370): keep the native open transition separate
 * from steady-state ticking. Entry slots 0 and 2 carry the action latch that
 * MenuTick sets; the adapter returns and clears those latches in the same
 * mode-specific order as the original routine. */
// FUNCTION: LEGORR 0x00415370
int overlay_menu_gate(FeOverlayMenuGateState* state, int mode,
                      FeOverlayMenuScrollRefresh refresh_scroll,
                      FeOverlayMenuTick menu_tick, void* context)
{
    if (refresh_scroll != NULL) {
        refresh_scroll(context);
    }
    if (state == NULL || (mode != 0 && mode != 1)) {
        return mode != 0 && mode != 1;
    }

    FeOverlayMenuSet* menu = &state->menus[mode];
    if (state->active_item != 0 && state->options_flag == 0 &&
        state->level_start_flag == 0 && state->action_flag == 0 &&
        state->cached_menu == mode && state->blocked == 0) {
        if (menu_tick != NULL) {
            state->active_item = menu_tick(context, state->active_item, 0);
        }
        if (mode == 1) {
            const int activated = menu->entries[0].action_state;
            menu->entries[0].action_state = 0;
            return activated == 1;
        }
        const int activated = menu->entries[0].action_state == 1 ||
                              menu->entries[2].action_state == 1;
        menu->entries[0].action_state = 0;
        menu->entries[2].action_state = 0;
        return activated;
    }

    state->level_start_flag = 0;
    state->active_item = menu->entries[0].handle;
    state->cached_menu = mode;
    if (state->action_flag != 0) {
        if (menu->entries[0].handle != 0) {
            menu->entries[0].action_state = 1;
        }
        if (menu->entries[2].handle != 0) {
            menu->entries[2].action_state = 1;
        }
        if (mode == 0) {
            state->action_flag = 0;
            return 0;
        }
        return 0;
    }
    if (state->options_flag == 1) {
        return 0;
    }

    if (state->blocked == 0 && menu_tick != NULL) {
        state->active_item = menu_tick(context, state->active_item, 0);
    }
    if (mode == 1) {
        const int activated = menu->entries[0].action_state;
        menu->entries[0].action_state = 0;
        return activated == 1;
    }
    const int activated = menu->entries[0].action_state == 1 ||
                          menu->entries[2].action_state == 1;
    menu->entries[0].action_state = 0;
    menu->entries[2].action_state = 0;
    return activated;
}

static void fe_menu_record_put_word(void* record, int field, uint32_t value)
{
    __builtin_memcpy((char*)record + (size_t)field * sizeof(void*), &value,
                     sizeof(value));
}

/* MenuTextRecordMake (FUN_00410d50): stores a copied label, layout words,
 * font handles and optional negative half-width offsets. */
// FUNCTION: LEGORR 0x00410d50
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_text_record_make(
    const char* text, const TextureSurface* font_a,
    const TextureSurface* font_b, uint32_t f4, uint32_t f5,
    uint32_t f6, int center, uint32_t f8, uint32_t f9)
{
    __asm {
        push ebx
        mov ebx, dword ptr [esp + 8]
        push esi
        push edi
        test ebx, ebx
        jne text_record_allocate
        xor eax, eax
        pop edi
        pop esi
        pop ebx
        ret
    text_record_allocate:
        push 0x3c
        call engine_heap_alloc
        mov esi, eax
        add esp, 4
        test esi, esi
        jne text_record_initialize
        pop edi
        pop esi
        pop ebx
        ret
    text_record_initialize:
        mov ecx, 0x0f
        xor eax, eax
        mov edi, esi
        push ebx
        rep stosd
        mov edi, ebx
        or ecx, 0xffffffff
        repnz scasb
        not ecx
        dec ecx
        mov dword ptr [esi + 4], ecx
        call campaign_level_fallback_label
        mov ecx, dword ptr [esp + 0x28]
        mov edx, dword ptr [esp + 0x30]
        mov edi, dword ptr [esp + 0x1c]
        mov dword ptr [esi + 0x14], ecx
        mov ecx, dword ptr [esp + 0x20]
        mov dword ptr [esi], eax
        mov eax, dword ptr [esp + 0x18]
        mov dword ptr [esi + 0x18], ecx
        mov ecx, dword ptr [esp + 0x34]
        mov dword ptr [esi + 0x10], edx
        mov edx, dword ptr [esp + 0x24]
        mov dword ptr [esi + 0x38], ecx
        mov ecx, dword ptr [esp + 0x2c]
        add esp, 4
        test ecx, ecx
        mov dword ptr [esi + 0x0c], eax
        mov dword ptr [esi + 8], edi
        mov dword ptr [esi + 0x1c], edx
        je text_record_zero_offsets
        test eax, eax
        je text_record_zero_offsets
        test edi, edi
        je text_record_zero_offsets
        push ebx
        push eax
        call formatted_text_printf_width
        add esp, 8
        shr eax, 1
        neg eax
        push ebx
        push edi
        mov dword ptr [esi + 0x20], eax
        call formatted_text_printf_width
        shr eax, 1
        add esp, 8
        neg eax
        mov dword ptr [esi + 0x24], eax
        mov eax, esi
        pop edi
        pop esi
        pop ebx
        ret
    text_record_zero_offsets:
        mov dword ptr [esi + 0x20], 0
        mov dword ptr [esi + 0x24], 0
        mov eax, esi
        pop edi
        pop esi
        pop ebx
        ret
    }
}
#else
void* menu_text_record_make(const char* text, const TextureSurface* font_a,
                            const TextureSurface* font_b, uint32_t f4,
                            uint32_t f5, uint32_t f6, int center,
                            uint32_t f8, uint32_t f9)
{
    if (text == NULL) return NULL;
    void* record = engine_heap_alloc(15 * sizeof(void*));
    if (record == NULL) return NULL;
    memset(record, 0, 15 * sizeof(void*));
    char* copy = campaign_level_fallback_label(text);
    if (copy == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    const size_t length = strlen(text);
    void* font_a_slot = (void*)font_a;
    void* font_b_slot = (void*)font_b;
    __builtin_memcpy((char*)record, &copy, sizeof(copy));
    fe_menu_record_put_word(record, 1, (uint32_t)length);
    __builtin_memcpy((char*)record + 2 * sizeof(void*), &font_b_slot,
                     sizeof(font_b_slot));
    __builtin_memcpy((char*)record + 3 * sizeof(void*), &font_a_slot,
                     sizeof(font_a_slot));
    fe_menu_record_put_word(record, 4, f8);
    fe_menu_record_put_word(record, 5, f6);
    fe_menu_record_put_word(record, 6, f4);
    fe_menu_record_put_word(record, 7, f5);
    fe_menu_record_put_word(record, 14, f9);
    if (center != 0 && font_a != NULL && font_b != NULL) {
        fe_menu_record_put_word(record, 8,
            0u - (formatted_text_printf_width(font_a, text) >> 1));
        fe_menu_record_put_word(record, 9,
            0u - (formatted_text_printf_width(font_b, text) >> 1));
    }
    return record;
}
#endif

/* CaseInsensitiveNameTableLookup (FUN_0046b4d0): TooltipOverlayStateInitialize
 * installs these 38 names consecutively at DAT_0054bf68. Results are 1-based. */
// FUNCTION: LEGORR 0x0046b4d0
int case_insensitive_name_table_lookup(const char* name, int* out_index)
{
    /* Corpus shape: the walk strides the name slots by four with a
     * one-based counter, feeds each entry to strcasecmp_lite untested,
     * and answers through the raw out pointer. Deviation: the name NULL
     * gate stays (the port's menu caller can pass a null optional name;
     * the corpus strides it blindly). */
    static const char* const names[] = {
        "ToolTip_RadarBlock", "ToolTip_UnitSelect", "ToolTip_Construction",
        "ToolTip_InterfaceMenu", "ToolTip_InterfaceMenuBackButton",
        "ToolTip_MapBlock", "ToolTip_Priority", "ToolTip_InfoMenuContinue",
        "ToolTip_InfoMenuDisableFuture", "ToolTip_RadarToggle",
        "ToolTip_RadarObjectView", "ToolTip_RadarZoomIn",
        "ToolTip_RadarZoomOut", "ToolTip_RadarMapView",
        "ToolTip_InfoDockGoto", "ToolTip_InfoDockClose",
        "ToolTip_TopPanelCallToArms", "ToolTip_TopPanelInfo",
        "ToolTip_TopPanelOptions", "ToolTip_TopPanelPriorities",
        "ToolTip_PriorityDisable", "ToolTip_PriorityUpOne",
        "ToolTip_PriorityReset", "ToolTip_CamControlZoomIn",
        "ToolTip_CamControlZoomOut", "ToolTip_CamControlCycle",
        "ToolTip_CamControlRotate", "ToolTip_SideBar_Ore",
        "ToolTip_SideBar_Crystals", "ToolTip_Close",
        "ToolTip_PreviousMessage", "ToolTip_NextMessage", "ToolTip_More",
        "ToolTip_Back", "ToolTip_CamControlCycleMinifigur",
        "ToolTip_Reward_Save", "ToolTip_Reward_Advance",
        "ToolTip_FrontEnd_Back",
    };
    if (name == NULL) {
        return 0;
    }
    const char* const* p = names;
    int index = 1;
    while (p < names + (sizeof(names) / sizeof(names[0]))) {
        if (strcasecmp_lite(name, *p) == 0) {
            *out_index = index;
            return 1;
        }
        ++p;
        ++index;
    }
    return 0;
}

/* MenuButtonRecordMake (FUN_00410e20): create a 15-slot image-backed menu
 * item, including its optional tooltip-name index. */
// FUNCTION: LEGORR 0x00410e20
void* menu_button_record_make(const char* text,
                              const TextureSurface* normal_font,
                              const TextureSurface* selected_font,
                              const char* normal_image,
                              const char* selected_image, uint32_t x,
                              uint32_t y, uint32_t flags, int centered,
                              const char* optional_name, uint32_t extra)
{
    if (text == NULL || normal_image == NULL) return NULL;
    void* record = engine_heap_alloc(15 * sizeof(void*));
    if (record == NULL) return NULL;
    memset(record, 0, 15 * sizeof(void*));

    char* label = campaign_level_fallback_label(text);
    if (label == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    const TextureSurface* normal = image_surface_load(normal_image, 0, 0);
    if (normal == NULL) {
        engine_heap_free(label);
        engine_heap_free(record);
        return NULL;
    }
    const TextureSurface* selected = selected_image != NULL
        ? image_surface_load(selected_image, 0, 0) : NULL;

    void* label_slot = label;
    void* selected_slot = (void*)selected;
    void* normal_slot = (void*)normal;
    __builtin_memcpy((char*)record, &label_slot, sizeof(label_slot));
    fe_menu_record_put_word(record, 1, (uint32_t)strlen(text));
    void* selected_font_slot = (void*)selected_font;
    void* normal_font_slot = (void*)normal_font;
    __builtin_memcpy((char*)record + 2 * sizeof(void*), &selected_font_slot,
                     sizeof(selected_font_slot));
    __builtin_memcpy((char*)record + 3 * sizeof(void*), &normal_font_slot,
                     sizeof(normal_font_slot));
    __builtin_memcpy((char*)record + 11 * sizeof(void*), &normal_slot,
                     sizeof(normal_slot));
    __builtin_memcpy((char*)record + 12 * sizeof(void*), &selected_slot,
                     sizeof(selected_slot));
    if (selected != NULL) fe_menu_record_put_word(record, 10, 1);
    fe_menu_record_put_word(record, 4, extra);
    fe_menu_record_put_word(record, 5, flags);
    fe_menu_record_put_word(record, 6, x);
    fe_menu_record_put_word(record, 7, y);
    if (centered && selected != NULL) {
        fe_menu_record_put_word(record, 8,
            0u - ((uint32_t)normal->width >> 1));
        fe_menu_record_put_word(record, 9,
            0u - ((uint32_t)selected->width >> 1));
    }
    int name_index = 0;
    if (case_insensitive_name_table_lookup(optional_name, &name_index)) {
        fe_menu_record_put_word(record, 13, (uint32_t)name_index);
    }
    return record;
}

static uint32_t fe_legacy_dword(const void* record, size_t field)
{
    uint32_t value = 0;
    if (record != NULL) {
        __builtin_memcpy(&value,
            (const char*)record + field * sizeof(void*), sizeof(value));
    }
    return value;
}

static void* fe_legacy_pointer(const void* record, size_t field)
{
    void* value = NULL;
    if (record != NULL) {
        __builtin_memcpy(&value,
            (const char*)record + field * sizeof(void*), sizeof(value));
    }
    return value;
}

static int fe_legacy_hit(uint32_t mouse_x, uint32_t mouse_y,
                         int32_t x, int32_t y, int32_t width, int32_t height)
{
    return menu_pointer_rectangle_hit_test((int32_t)mouse_x,
        (int32_t)mouse_y, x, y, width, height);
}

/* MenuMouseCoordinateUpdate (FUN_00411360): test the two row labels first,
 * then the visible type-6 resource entries. */
// FUNCTION: LEGORR 0x00411360
int menu_mouse_coordinate_update(const void* root, const void* item,
                                 const void* row)
{
    if (root == NULL || item == NULL || row == NULL) return -1;
    const TextureSurface* face = (const TextureSurface*)
        fe_legacy_pointer(item, 2);
    const char* label = (const char*)fe_legacy_pointer(row, 8);
    uint32_t first_width = 0, first_height = 0;
    if (face != NULL) {
        first_width = formatted_text_printf_width(face, label);
        first_height = (uint32_t)face->height;
    }
    const int32_t mouse_x = (int32_t)renderer_word_a_get();
    const int32_t mouse_y = (int32_t)renderer_word_b_get();
    int32_t x = (int32_t)fe_legacy_dword(root, 11)
        + (int32_t)fe_legacy_dword(row, 15);
    int32_t y = (int32_t)fe_legacy_dword(root, 12)
        + (int32_t)fe_legacy_dword(row, 16);
    if (fe_legacy_hit((uint32_t)mouse_x, (uint32_t)mouse_y, x, y,
                      (int32_t)first_width, (int32_t)first_height)) {
        return -2;
    }

    label = (const char*)fe_legacy_pointer(row, 9);
    uint32_t second_width = 0, second_height = 0;
    if (face != NULL) {
        second_width = formatted_text_printf_width(face, label);
        second_height = (uint32_t)face->height;
    }
    x = (int32_t)fe_legacy_dword(root, 11)
        + (int32_t)fe_legacy_dword(row, 17);
    y = (int32_t)fe_legacy_dword(root, 12)
        + (int32_t)fe_legacy_dword(row, 18);
    if (fe_legacy_hit((uint32_t)mouse_x, (uint32_t)mouse_y, x, y,
                      (int32_t)second_width, (int32_t)second_height)) {
        return -3;
    }

    const uint32_t visible = fe_legacy_dword(row, 14);
    const uint32_t start = fe_legacy_dword(row, 19);
    const uint32_t total = fe_legacy_dword(row, 7);
    const uint8_t* entries = (const uint8_t*)fe_legacy_pointer(row, 0);
    const uint32_t* widths = (const uint32_t*)fe_legacy_pointer(row, 1);
    const uint32_t* heights = (const uint32_t*)fe_legacy_pointer(row, 4);
    if (entries == NULL || widths == NULL || heights == NULL) return -1;
    for (uint32_t i = 0; i < visible; ++i) {
        const uint32_t index = start + i;
        if (index >= total) break;
        const uint8_t* entry = entries + (size_t)index * 0x20;
        uint32_t flags = 0, offset_x = 0, offset_y = 0;
        __builtin_memcpy(&flags, entry, sizeof(flags));
        if ((flags & 4u) == 0) continue;
        __builtin_memcpy(&offset_x, entry + 0x14, sizeof(offset_x));
        __builtin_memcpy(&offset_y, entry + 0x18, sizeof(offset_y));
        x = (int32_t)fe_legacy_dword(root, 11)
            + (int32_t)fe_legacy_dword(item, 6)
            + (int32_t)fe_legacy_dword(row, 11)
            + (int32_t)offset_x + (int32_t)s_menu_plane_pan[0] + mouse_x;
        y = (int32_t)fe_legacy_dword(root, 12)
            + (int32_t)fe_legacy_dword(item, 7)
            + (int32_t)fe_legacy_dword(row, 12)
            + (int32_t)offset_y + (int32_t)s_menu_plane_pan[1]
            + (int32_t)fe_legacy_dword(row, 13) * (int32_t)i + mouse_y;
        if (text_layout_offset_for_centering(mouse_x, mouse_y, x, y,
                (int32_t)widths[index], (int32_t)heights[index],
                (int)fe_legacy_dword(root, 17))) {
            return (int)(start + i);
        }
    }
    return -1;
}

/* HasType6ItemAtIndex (FUN_004114f0): the item is active when its row
 * renderer reports any hit result other than the no-hit sentinel. */
// FUNCTION: LEGORR 0x004114f0
int has_type6_item_at_index(const void* menu, int index)
{
    if (menu == NULL || index < 0
        || (uint32_t)index >= fe_legacy_dword(menu, 7)) {
        return 0;
    }
    void* items = fe_legacy_pointer(menu, 6);
    if (items == NULL) return 0;
    void* item = NULL;
    __builtin_memcpy(&item, (char*)items + (size_t)index * sizeof(void*),
                     sizeof(item));
    if (item == NULL || fe_legacy_dword(item, 5) != 6) return 0;
    return menu_mouse_coordinate_update(menu, item,
        fe_legacy_pointer(item, 4)) != -1;
}

/* MenuWidgetItemScreenRect (FUN_00411530): calculate a menu item's clickable
 * screen rectangle. Legacy menu records store scalar values in the low dword
 * of each native-width slot, which keeps this layout usable in 64-bit builds. */
// FUNCTION: LEGORR 0x00411530
int menu_widget_item_screen_rect(const void* menu, int index, int* x, int* y,
                                 int* width, int* height)
{
    if (menu == NULL || index < 0
        || (uint32_t)index >= fe_legacy_dword(menu, 7)) return 0;
    void* items = fe_legacy_pointer(menu, 6);
    if (items == NULL) return 0;
    void* item = NULL;
    __builtin_memcpy(&item, (char*)items + (size_t)index * sizeof(void*),
                     sizeof(item));
    if (item == NULL) return 0;

    const uint32_t type = fe_legacy_dword(item, 5);
    if (type == 6) return 0;
    const int32_t origin_x = (int32_t)fe_legacy_dword(menu, 11);
    const int32_t origin_y = (int32_t)fe_legacy_dword(menu, 12);
    const int32_t item_x = (int32_t)fe_legacy_dword(item, 6);
    const int32_t item_y = (int32_t)fe_legacy_dword(item, 7);
    const bool selected = (int32_t)fe_legacy_dword(menu, 9) == index;

    int32_t left = origin_x + (int32_t)fe_legacy_dword(item, 9) + item_x;
    int32_t top = origin_y + item_y;
    int32_t rect_width = 0;
    int32_t rect_height = 0;
    if (type == 3) {
        const char* label = (const char*)fe_legacy_pointer(item, 0);
        const TextureSurface* font = (const TextureSurface*)
            fe_legacy_pointer(item, selected ? 2 : 3);
        const int32_t label_width = (int32_t)
            formatted_text_printf_width(font, label);
        const int32_t label_height = font != NULL ? font->height : 0;
        const int32_t label_x = left;
        const int32_t label_y = top;

        const void* slider = fe_legacy_pointer(item, 4);
        const int32_t slider_x = origin_x + item_x
            + (int32_t)fe_legacy_dword(slider, 3);
        const int32_t slider_y = origin_y + item_y
            + (int32_t)fe_legacy_dword(slider, 4);
        const TextureSurface* left_cap = (const TextureSurface*)
            fe_legacy_pointer(slider, 9);
        const TextureSurface* right_cap = (const TextureSurface*)
            fe_legacy_pointer(slider, 10);
        const int32_t extra_left = left_cap != NULL ? left_cap->width : 0;
        const int32_t extra_right = right_cap != NULL ? right_cap->width : 0;

        left = label_x < slider_x ? label_x : slider_x;
        top = label_y < slider_y ? label_y : slider_y;
        const int32_t label_right = label_x + label_width;
        const int32_t slider_right = slider_x + extra_left + 0x68 + extra_right;
        const int32_t bottom_label = label_y + label_height;
        const int32_t bottom_slider = slider_y + 0x14;
        const int32_t right = label_right > slider_right ? label_right : slider_right;
        const int32_t bottom = bottom_label > bottom_slider
            ? bottom_label : bottom_slider;
        rect_width = right - left;
        rect_height = bottom - top;
    }
    else if (fe_legacy_dword(item, 10) != 0) {
        const TextureSurface* surface = (const TextureSurface*)
            fe_legacy_pointer(item, selected ? 12 : 11);
        rect_width = surface != NULL ? surface->width : 0;
        rect_height = surface != NULL ? surface->height : 0;
    }
    else {
        const char* label = (const char*)fe_legacy_pointer(item, 0);
        const TextureSurface* font = (const TextureSurface*)
            fe_legacy_pointer(item, selected ? 2 : 3);
        rect_width = (int32_t)formatted_text_printf_width(font, label);
        rect_height = font != NULL ? font->height : 0;
    }

    if (x != NULL) *x = left;
    if (y != NULL) *y = top;
    if (width != NULL) *width = rect_width;
    if (height != NULL) *height = rect_height;
    return 1;
}

/* MenuPickWidgetUnderPointer (FUN_00411790): the regular-rect helper gates
 * the walk; a failed rect returns unless the type-6 helper hits. */
// FUNCTION: LEGORR 0x00411790
int menu_pick_widget_under_pointer(const void* menu, int* picked_index)
{
    int x = 0, y = 0, width = 0, height = 0;
    int index = 0;
    int type6_hit = 0;
    for (;;) {
        const int has_rect = menu_widget_item_screen_rect(menu, index, &x, &y,
                                                          &width, &height);
        if (!has_rect) {
            type6_hit = has_type6_item_at_index(menu, index);
            if (!type6_hit) return 0;
        }
        if (type6_hit) break;
        if (menu_pointer_rectangle_hit_test(
                (int32_t)renderer_word_a_get(),
                (int32_t)renderer_word_b_get(), x, y, width, height)) {
            break;
        }
        ++index;
    }
    if (picked_index != NULL) *picked_index = index;
    return 1;
}

/* The previous-renderer-word memory is corpus-owned (DAT_004db8d4, the
 * zero-filled tail); the current word is crt's g_renderer_word_c — the
 * corpus loads both directly, so the port does too (the getter call was
 * the only operand mismatch). */
// GLOBAL: LEGORR 0x004db8d4
static uint32_t s_progress_previous;

/* FrontendProgressValueChanged (FUN_00411840): detect the transition from
 * progress-active (1) back to idle (0) in the renderer's third word. */
// FUNCTION: LEGORR 0x00411840
int frontend_progress_value_changed(void)
{
    const uint32_t current = g_renderer_word_c;
    const int changed_to_idle = current == 0u && s_progress_previous == 1u;
    s_progress_previous = current;
    return changed_to_idle;
}

/* Item display position — the MenuTick @ 0x412c00 arithmetic: each item
 * draws at (menu.Position.x + item.x [+ centering], menu.Position.y +
 * item.y); AutoCenter centres the label on the Position column (base_x -
 * text_width/2), otherwise the item's own x anchor is added (the shipped
 * Back buttons are e.g. x=-314 on Position 320:200 -> left edge). Picker
 * rows carry absolute FrontEndX/Y scatter coords (their submenu Position is
 * 0:0). */
// FUNCTION: LEGORR 0x00412c00
static void fe_item_pos(const FrontEnd* fe, const FeItem* it, int i,
                        int w, const TextureSurface* measure, int* x, int* y)
{
    (void)i;
    (void)w;
    const FeSubmenu* sub = frontend_active_submenu(fe);
    /* Runtime save rows use absolute coordinates just like level picker
     * rows. Their SaveLoad panel is already positioned by the submenu, so
     * adding its centered menu anchor would push the list off the panel. */
    if (it->level_start || it->load_slot) {
        *x = it->x;
        *y = it->y;
    } else {
        int bx = (sub != NULL) ? sub->base_x : 0;
        int by = (sub != NULL) ? sub->base_y : 0;
        if (sub != NULL && sub->auto_center) {
            char label[FE_NAME_MAX];
            fe_display_label(it->label, label, sizeof(label));
            int tw = measure != NULL ? fe_font_text_width(label, measure)
                                     : fe_text_width(label);
            *x = bx + it->x - tw / 2;
        } else {
            *x = bx + it->x;
        }
        *y = by + it->y;
    }
    /* CanScroll plane pan (FUN_00412c00 adds DAT_005574e0 to the item y
     * in BOTH draw and hit-test paths). */
    if (sub != NULL && sub->can_scroll) {
        *y += sub->scroll;
    }
}

/* CanScroll content extent (the corpus's menu+8 "plane height"): the
 * deepest item row of the submenu plus one row height. */
static int fe_scroll_measure(const FeSubmenu* sub)
{
    int maxy = 0;
    for (int i = 0; i < sub->item_count; i++) {
        int top = sub->items[i].level_start
                      ? sub->items[i].y
                      : sub->base_y + sub->items[i].y;
        if (top > maxy) {
            maxy = top;
        }
    }
    return maxy + 16;               /* last row's height (port chrome) */
}

/* Pointer hit-test over the active submenu's items, using the same
 * placement arithmetic (fe_item_pos) and measure-font widths as drawing.
 * Returns 1 and writes the hovered index. */
static int fe_item_pick(const FrontEnd* fe, int mx, int my,
                        const TextureSurface* measure, int* out_idx)
{
    const FeSubmenu* sub = frontend_active_submenu(fe);
    if (sub == NULL || sub->item_count <= 0) {
        return 0;
    }
    int cw = 0, chh = 0;
    if (measure != NULL) {
        fe_font_cell(measure, &cw, &chh);
    }
    int rh = measure != NULL ? chh + 8 : 12;
    for (int i = 0; i < sub->item_count; i++) {
        const FeItem* it = &sub->items[i];
        int x, y;
        fe_item_pos(fe, it, i, 0, measure, &x, &y);
        char label[FE_NAME_MAX];
        fe_display_label(it->label, label, sizeof(label));
        int tw = measure != NULL ? fe_font_text_width(label, measure)
                                 : fe_text_width(label);
        int hit_h = rh;
        if (it->image_normal[0] != '\0') {
            TextureSurface* image = texture_get_or_load(NULL,
                                                        it->image_normal);
            if (image != NULL) {
                tw = image->width;
                hit_h = image->height;
                texture_release(image);
            }
        }
        if (mx >= x - 4 && mx <= x + tw + 4
            && my >= y - 2 && my <= y + hit_h) {
            if (out_idx != NULL) {
                *out_idx = i;
            }
            return 1;
        }
    }
    return 0;
}

/* MenuHoverStatusTextDraw (FUN_00412750): refresh the menu's hover status
 * from the currently selected runtime save row. The corpus rotates resource
 * records and rewrites localized labels; the frontend keeps the derived slot
 * summary alongside its FeItem and draws it in the status band. */
// FUNCTION: LEGORR 0x00412750
void menu_hover_status_text_draw(FrontEnd* fe, const TextureSurface* font,
                                 int w, int h, void* px, int pitch)
{
    if (fe == NULL || px == NULL || fe->menu < 0 || fe->menu >= FE_MENU_COUNT
        || h < 16) {
        return;
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    const char* image = frontend_active_image(fe);
    const FeItem* item = frontend_active_item(fe);
    if (sub == NULL || image == NULL || item == NULL
        || !s_stristr(image, "SaveLoad")) {
        return;
    }

    /* SaveMenuLayoutLoad reads these strings and the Window rect from the
     * shipped Menu::SaveText config. The reference draws an instruction and
     * a completion line in this lower panel, independently of the selected
     * row's optional save summary. */
    const FeSaveMenuLayout* layout = &fe->save_text_layout;
    const char* prompt = fe->save_action ? layout->save : layout->load;
    if (prompt[0] != '\0' && layout->slot[0] != '\0') {
        /* SaveText uses the small green briefing font, not the large cyan
         * FrontEnd button font passed to this renderer. */
        TextureSurface* save_text_font = texture_get_or_load(
            NULL, "Interface\\Fonts\\MBriefFONT2.bmp");
        const TextureSurface* text_font = save_text_font != NULL
                                              ? save_text_font : font;
        char instruction[FE_NAME_MAX];
        char progress[FE_NAME_MAX];
        snprintf(instruction, sizeof(instruction), "%s",
                 ui_text_underscores_to_spaces(prompt));
        /* The config's Slot format is e.g. "%d%%_of_game_completed.". */
        snprintf(progress, sizeof(progress), layout->slot, 0);
        char normalized_progress[FE_NAME_MAX];
        snprintf(normalized_progress, sizeof(normalized_progress), "%s",
                 ui_text_underscores_to_spaces(progress));
        int wx = layout->has_window ? layout->window_x : 0;
        int wy = layout->has_window ? layout->window_y : h - 45;
        int ww = layout->has_window ? layout->window_w : w;
        int line_h = 12;
        if (text_font != NULL) {
            int cell_w = 0;
            (void)fe_font_cell(text_font, &cell_w, &line_h);
        }
        const char* lines[2] = { instruction, normalized_progress };
        for (int line = 0; line < 2; ++line) {
            int text_w = text_font != NULL
                             ? fe_font_text_width(lines[line], text_font)
                             : fe_text_width(lines[line]);
            int x = wx + (ww - text_w) / 2;
            int y = wy + 4 + line * line_h;
            if (text_font != NULL) {
                formatted_text_draw(text_font, px, pitch, w, h, x, y,
                                    lines[line]);
            } else {
                fe_draw_text(px, pitch, w, h, x, y, lines[line],
                             150, 255, 80);
            }
        }
        if (save_text_font != NULL) {
            texture_release(save_text_font);
        }
    }
}

/* InterfaceMessageTabInfoTextLoad (FUN_00419430): load the scrolling
 * message-tab panel surface and apply the shared resource-record setup. */
// FUNCTION: LEGORR 0x00419430
void interface_message_tab_info_text_load(const char* path)
{
    s_interface_message_tab_info_text =
        (path != NULL && path[0] != '\0')
            ? image_surface_load(path, 0u, 0u) : NULL;
    resource_record_add_ref(s_interface_message_tab_info_text);
}

TextureSurface* interface_message_tab_info_text(void)
{
    return s_interface_message_tab_info_text;
}

void frontend_render(FrontEnd* fe, const TextureSurface* hi_font,
                     const TextureSurface* lo_font,
                     int w, int h, void* px, int pitch)
{
    if (fe == NULL || px == NULL || !fe->open) {
        return;
    }
    /* viewport bookkeeping for the CanScroll bands/clamp (the corpus keeps
     * its own screen-absolute band constants; we track the window here). */
    if (fe->view_w != w || fe->view_h != h) {
        fe->view_w = w;
        fe->view_h = h;
        FeSubmenu* act = &fe->menus[fe->menu].submenus[fe->sub];
        if (act->can_scroll) {
            act->scroll_extent = 0;            /* re-measure next tick */
        }
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    if (sub == NULL || sub->item_count <= 0) {
        return;
    }

    /* .flh overlay lane (finding 23): blit each open overlay's current
     * frame at its configured (x,y), native size, over the menu backdrop
     * and under the items. */
    for (int overlay_index = 0; overlay_index < sub->overlay_count;
         ++overlay_index) {
        const FeOverlay* o = &sub->overlays[overlay_index];
        if (o == NULL || o->flic == NULL) {
            continue;
        }
        const uint16_t* frm = flic_frame555(o->flic);
        if (frm == NULL) {
            continue;
        }
        int fw = flic_width(o->flic), fh = flic_height(o->flic);
        for (int yy = 0; yy < fh; ++yy) {
            int dy = o->y + yy;
            if (dy < 0 || dy >= h) {
                continue;
            }
            int sx = o->x < 0 ? -o->x : 0;
            int ex = fw;
            if (o->x + ex > w) {
                ex = w - o->x;
            }
            if (sx >= ex) {
                continue;
            }
            uint8_t* d = (uint8_t*)px + (size_t)dy * (size_t)pitch
                         + (size_t)(o->x + sx) * 4;
            for (int xx = sx; xx < ex; ++xx) {
                uint16_t c = frm[(size_t)yy * fw + xx];
                /* FLIC-backed sprites use RGB555 zero as the transparent
                 * key. Treating every pixel as opaque paints the decoder's
                 * empty canvas over the menu backdrop. */
                if (c != 0) {
                    uint32_t rgba = flic_rgb555_to_rgba(c);
                    d[0] = (uint8_t)rgba;
                    d[1] = (uint8_t)(rgba >> 8);
                    d[2] = (uint8_t)(rgba >> 16);
                    d[3] = 0xff;
                }
                d += 4;
            }
        }
    }

    if (sub->display_title && sub->title[0] != '\0') {
        char title[FE_NAME_MAX];
        fe_display_label(sub->title, title, sizeof(title));
        int title_w = hi_font != NULL ? fe_font_text_width(title, hi_font)
                                      : fe_text_width(title);
        int title_x = (w - title_w) / 2;
        int title_y = sub->base_y + sub->title_anchor_y;
        if (hi_font != NULL) {
            formatted_text_draw(hi_font, px, pitch, w, h, title_x, title_y,
                                title);
        } else {
            fe_draw_text(px, pitch, w, h, title_x, title_y, title,
                         230, 210, 120);
        }
    }

    for (int item_index =  0; item_index < sub->item_count; item_index++) {
        const FeItem* it = &sub->items[item_index];
        int x, y;
        fe_item_pos(fe, it, item_index, w, hi_font, &x, &y);
        if (x < 0) {
            x = 8;
        }
        if (y < 0 && !(sub->can_scroll)) {
            y = 8;
        }
        if (sub->can_scroll && (y + 16 < 0 || y >= h)) {
            continue;   /* scrolled row: outside the plane window */
        }

        int selected = (item_index == fe->sel);
        const TextureSurface* font = selected ? hi_font : lo_font;
        char label[FE_NAME_MAX];
        fe_display_label(it->label, label, sizeof(label));
        int lw = font != NULL ? fe_font_text_width(label, font)
                              : fe_text_width(label);
        if (it->image_normal[0] != '\0') {
            const char* image_path = selected && it->image_selected[0] != '\0'
                                         ? it->image_selected
                                         : it->image_normal;
            TextureSurface* image = texture_get_or_load(NULL, image_path);
            if (image != NULL) {
                surface_blit_texture(px, pitch, w, h, image, x, y);
                texture_release(image);
            } else if (font != NULL) {
                formatted_text_draw(font, px, pitch, w, h, x, y, label);
            } else {
                fe_draw_text(px, pitch, w, h, x, y, label,
                             selected ? 255 : 200, selected ? 255 : 200,
                             selected ? 255 : 160);
            }
        } else if (font != NULL) {
            formatted_text_draw(font, px, pitch, w, h, x, y, label);
        } else {
            fe_draw_text(px, pitch, w, h, x, y, label,
                         selected ? 255 : 200, selected ? 255 : 200,
                         selected ? 255 : 160);
        }
        /* Slider / Cycle widget: a bar + knob for sliders, the current
         * option text for cycles (the port's own chrome, not the shipped
         * Vol_* bar art). */
        if (it->kind == FE_ITEM_SLIDER || it->kind == FE_ITEM_REALSLIDER) {
            int chh2 = 12;
            if (font != NULL) {
                int cw2 = 0;
                fe_font_cell(font, &cw2, &chh2);
            }
            int bx = x + lw + 10, by = y + chh2 / 2 - 2;
            int bw = 72;
            int frac = bw;
            if (it->max > it->min) {
                frac = (it->value - it->min) * bw / (it->max - it->min);
            }
            fe_fill_rect(px, pitch, w, h, bx, by, bw, 6, 36, 36, 40);
            fe_fill_rect(px, pitch, w, h, bx, by, frac, 6, 210, 180, 70);
            fe_fill_rect(px, pitch, w, h, bx + frac, by - 1, 3, 8, 255, 255, 255);
        } else if (it->kind == FE_ITEM_CYCLE && it->cycle_count > 0) {
            int oi = it->value < it->cycle_count ? it->value : 0;
            const char* opt = it->cycle_opts[oi];
            if (font != NULL) {
                formatted_text_draw(font, px, pitch, w, h, x + lw + 10, y,
                                    opt);
            } else {
                fe_draw_text(px, pitch, w, h, x + lw + 10, y, opt,
                             selected ? 255 : 200, selected ? 255 : 200,
                             selected ? 255 : 160);
            }
        }
    }

    /* Pointer_Standard is the shipped Aclosed.bmp cursor from Lego.cfg's
     * Pointers block. Its red tip is the top-left hotspot. */
    if (fe->pointer_x >= 0 && fe->pointer_y >= 0
        && fe->pointer_x < w && fe->pointer_y < h) {
        char key[128];
        snprintf(key, sizeof(key), "%s Pointers Pointer_Standard",
                 util_root_name(fe->cfg));
        const char* pointer = fe->cfg != NULL
                                  ? block_get_strptr(fe->cfg, key) : NULL;
        if (pointer == NULL || pointer[0] == '\0') {
            pointer = "Interface\\Pointers\\Aclosed.bmp";
        }
        TextureSurface* cursor = texture_get_or_load(NULL, pointer);
        if (cursor != NULL) {
            int ox = fe->pointer_x, oy = fe->pointer_y;
            for (int yy = 0; yy < cursor->height; ++yy) {
                int dy = oy + yy;
                if (dy < 0 || dy >= h) {
                    continue;
                }
                uint8_t* d = (uint8_t*)px + (size_t)dy * pitch;
                for (int xx = 0; xx < cursor->width; ++xx) {
                    int dx = ox + xx;
                    if (dx < 0 || dx >= w) {
                        continue;
                    }
                    uint32_t c = cursor->pixels[(size_t)yy * cursor->width + xx];
                    int transparent = 0;
                    for (int k = 0; k < cursor->key_count; ++k) {
                        if (c == cursor->keys[k]) {
                            transparent = 1;
                            break;
                        }
                    }
                    if (!transparent) {
                        uint8_t* p = d + (size_t)dx * 4;
                        p[0] = (uint8_t)(c >> 16);
                        p[1] = (uint8_t)(c >> 8);
                        p[2] = (uint8_t)c;
                        p[3] = 0xff;
                    }
                }
            }
            texture_release(cursor);
        }
    }
    menu_hover_status_text_draw(fe, hi_font != NULL ? hi_font : lo_font,
                                w, h, px, pitch);
}

void frontend_render_credits(const TextureSurface* font,
                             const char* const* lines, int line_count,
                             int scroll_px, int w, int h, void* px, int pitch)
{
    if (px == NULL || lines == NULL || line_count <= 0 || w <= 0 || h <= 0) {
        return;
    }
    int cw = 0, ch = 0;
    if (font != NULL && fe_font_cell(font, &cw, &ch)) {
        /* use the shipped RSFont bitmap grid */
    } else {
        font = NULL;
        cw = 4;
        ch = 8;
    }
    const int line_h = ch;
    const int visible_rows = h / line_h + 1;
    const int period = (line_count + visible_rows) * line_h;
    int phase = scroll_px % period;
    if (phase < 0) {
        phase += period;
    }
    const int first_row = -visible_rows + phase / line_h;
    const int partial = phase % line_h;
    for (int row = first_row < 0 ? 0 : first_row;
         row < line_count && row < first_row + visible_rows + 1; ++row) {
        const char* text = lines[row] != NULL ? lines[row] : "";
        int tw = font != NULL ? fe_font_text_width(text, font)
                              : fe_text_width(text);
        int x = (w - tw) / 2;
        int y = (row - first_row) * line_h - partial;
        if (font != NULL) {
            formatted_text_draw(font, px, pitch, w, h, x, y, text);
        } else {
            fe_draw_text(px, pitch, w, h, x, y, text, 230, 230, 220);
        }
    }
}

/* --- current selection context (see frontend.hpp) -------------------------- */
// GLOBAL: LEGORR 0x00557500
static void* s_current_selection_context;

// FUNCTION: LEGORR 0x00417fe0
void* current_selection_context_get(void)
{
    return s_current_selection_context;
}

// FUNCTION: LEGORR 0x00417ff0
void current_selection_context_set(void* context)
{
    s_current_selection_context = context;
}

/* --- selection-state band (see frontend.hpp) -------------------------------
 * Zero-initialized module statics. The resource-position and help-window
 * button hit-test routines are ported; the help-window layout loader and
 * other state writers remain unported, so their inputs land at zero until
 * installed through the port hooks. The corpus's 0x1c-stride handle tables
 * index blindly; the port
 * guards a 16-entry window per table (index * 7 dwords). */
#define SEL_HANDLE_ENTRIES 16
#define SEL_HANDLE_STRIDE 7 /* dwords per 0x1c-stride entry */

// GLOBAL: LEGORR 0x004dc650
static uint32_t s_sel_state;                    /* DAT_004dc650 */
static int      s_sel_hit_index;                /* DAT_004dc648 */
static uint32_t s_sel_handles_a[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
static uint32_t s_sel_handles_b[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
static uint32_t s_sel_handle_extra;             /* DAT_004db934 */

// FUNCTION: LEGORR 0x004181a0
void selection_state_flag_clear_bit0(void)
{
    s_sel_state = s_sel_state & ~1u;
}

// FUNCTION: LEGORR 0x00419150
uint32_t selection_state_handle_get(int index)
{
    if (index == 1 && (s_sel_state & 4u) == 0) {
        return s_sel_handle_extra;
    }
    if ((s_sel_state & 0x10u) != 0 && s_sel_hit_index == index) {
        if (index < 0 || index >= SEL_HANDLE_ENTRIES) {
            return 0u; /* deviation: the corpus indexes blindly */
        }
        const size_t off = (size_t)index * SEL_HANDLE_STRIDE;
        if ((s_sel_state & 0x20u) != 0) {
            return s_sel_handles_b[off];
        }
        return s_sel_handles_a[off];
    }
    return 0u;
}

uint32_t selection_state_flags_peek(void) { return s_sel_state; }

// FUNCTION: LEGORR 0x004191f0
uint32_t ui_mode_resource_bit40_get(void)
{
    return s_sel_state & 0x40u;
}
void selection_state_flags_set(uint32_t flags) { s_sel_state = flags; }
int selection_state_hit_index_peek(void) { return s_sel_hit_index; }
void selection_state_hit_index_set(int index) { s_sel_hit_index = index; }
uint32_t selection_state_handle_extra_peek(void) { return s_sel_handle_extra; }
void selection_state_handle_extra_set(uint32_t value)
{
    s_sel_handle_extra = value;
}

uint32_t selection_state_handle_slot_peek(int table, int index)
{
    if (index < 0 || index >= SEL_HANDLE_ENTRIES) {
        return 0u;
    }
    const size_t off = (size_t)index * SEL_HANDLE_STRIDE;
    return (table != 0) ? s_sel_handles_b[off] : s_sel_handles_a[off];
}

void selection_state_handle_slot_set(int table, int index, uint32_t value)
{
    if (index < 0 || index >= SEL_HANDLE_ENTRIES) {
        return;
    }
    const size_t off = (size_t)index * SEL_HANDLE_STRIDE;
    if (table != 0) {
        s_sel_handles_b[off] = value;
    } else {
        s_sel_handles_a[off] = value;
    }
}

/* --- band-mode / timed-effect / positions (see frontend.hpp) -----------------
 * The band mode word joins the selection-state statics (the corpus's
 * DAT_004dc64c, read by the transition leaf and raised in passing by the
 * state-bit gates). The timed-effect flag words land as a guarded window:
 * the corpus's 0x27c-stride band entries live with the unported
 * update dispatch; the flag and time fields land as guarded windows. */
#define TIMED_EFFECT_ENTRIES 16
#define LAYOUT_POSITION_ENTRIES 2   /* corpus: two help-window buttons */
#define LAYOUT_POSITION_STRIDE 7    /* corpus advances 0x1c bytes per entry */

static int      s_band_mode;                    /* DAT_004dc64c */
static uint32_t s_timed_effect_flags[TIMED_EFFECT_ENTRIES];
static uint32_t s_timed_effect_time[TIMED_EFFECT_ENTRIES];
static uint32_t s_timed_effect_tick; /* DAT_0054bf3c role: unowned */
static int32_t  s_layout_position[LAYOUT_POSITION_ENTRIES]
                                  [LAYOUT_POSITION_STRIDE];
static float s_help_window_button_rects[LAYOUT_POSITION_ENTRIES][4];
static int (*s_ui_resource_reposition_hook)(uint32_t handle, int x, int y);
static float s_help_background_offset_x;
static float s_help_background_offset_y;
static uint32_t s_help_background_handle;
static uint32_t s_help_text_area_handle;
static char s_help_window_labels[3][256];
static float s_help_window_feedback_position;
static float s_help_window_feedback_limit;
static void (*s_help_window_feedback_reset)(int, int);
static void (*s_help_window_feedback_cleanup)(void);
static void (*s_help_window_feedback_finish)(int);
static void (*s_help_window_sound_clear)(int);
static void (*s_help_render_transition_clear)(int, int);

// FUNCTION: LEGORR 0x0046b770
void timed_effect_update_request_set(int index)
{
    if (index >= 0 && index < TIMED_EFFECT_ENTRIES) {
        s_timed_effect_flags[index] |= 4u;
    }
    /* deviation: the corpus raises blindly at any index. */
}

/* TimedEffectCueRequestSet (FUN_0046b700) — see frontend.hpp. The band's
 * cue word lands as own storage next to the flag/time windows. */
static uint32_t s_timed_effect_cue[TIMED_EFFECT_ENTRIES];

// FUNCTION: LEGORR 0x0046b700
void timed_effect_cue_request_set(int index, uint32_t value)
{
    if (index < 0 || index >= TIMED_EFFECT_ENTRIES) {
        return; /* deviation: the corpus strides any index blindly */
    }
    s_timed_effect_cue[index] = value;
    s_timed_effect_flags[index] |= 3u;
}

uint32_t timed_effect_cue_peek(int index)
{
    return (index >= 0 && index < TIMED_EFFECT_ENTRIES)
               ? s_timed_effect_cue[index] : 0u;
}

uint32_t timed_effect_flags_peek(int index)
{
    if (index < 0 || index >= TIMED_EFFECT_ENTRIES) {
        return 0u;
    }
    return s_timed_effect_flags[index];
}

// FUNCTION: LEGORR 0x00418fc0
bool ui_mode_transition_request(int index, int p2, int p3)
{
    const int mode = s_band_mode;
    if (p2 == 0) {
        s_band_mode = 4;
    } else if (s_band_mode == 4) {
        if (p3 == 0) {
            s_band_mode = index;
            s_sel_state |= 0x20u;
            return index == 4;
        }
    } else if (s_band_mode == index) {
        s_sel_state |= 0x20u;
        return true;
    }
    return mode == index;
}

int band_mode_peek(void) { return s_band_mode; }
void band_mode_set(int mode) { s_band_mode = mode; }

// FUNCTION: LEGORR 0x00418f80
void selection_mode_set_by_index(int index)
{
    switch (index) {
    case 0:
        timed_effect_update_request_set(8);
        break;
    case 1:
        timed_effect_update_request_set(9);
        break;
    case 2:
    case 3:
        break;
    default:
        break;
    }
}

// FUNCTION: LEGORR 0x00419100
void ui_mode_resource_positions_apply(void)
{
    for (int i = 0; i < LAYOUT_POSITION_ENTRIES; ++i) {
        const uint32_t handle = selection_state_handle_get(i);
        if (handle != 0u) {
            if (s_ui_resource_reposition_hook != NULL) {
                (void)s_ui_resource_reposition_hook(
                    handle, s_layout_position[i][0], s_layout_position[i][1]);
            }
        }
    }
}

void ui_mode_resource_reposition_hook_set(
    int (*hook)(uint32_t handle, int x, int y))
{
    s_ui_resource_reposition_hook = hook;
}

// FUNCTION: LEGORR 0x00419030
void help_window_button_feedback_update(int action, float amount)
{
    switch (action) {
    case 0:
        if (s_help_window_feedback_cleanup != NULL) {
            s_help_window_feedback_cleanup();
        }
        return;
    case 1:
        if (s_help_window_feedback_reset != NULL) {
            s_help_window_feedback_reset(1, 0);
        }
        if (s_help_window_feedback_cleanup != NULL) {
            s_help_window_feedback_cleanup();
        }
        return;
    case 2:
        if (0.0f < s_help_window_feedback_position) {
            s_help_window_feedback_position -= amount * 0.2f;
            if (s_help_window_feedback_position < 0.0f) {
                s_help_window_feedback_position = 0.0f;
                return;
            }
        }
        break;
    case 3:
        if (s_help_window_feedback_position < s_help_window_feedback_limit) {
            s_help_window_feedback_position += amount * 0.2f;
            if (s_help_window_feedback_limit <= s_help_window_feedback_position) {
                s_help_window_feedback_position = s_help_window_feedback_limit;
                if (s_help_window_feedback_finish != NULL) {
                    s_help_window_feedback_finish(0);
                }
            }
        }
        break;
    default:
        break;
    }
}

void help_window_feedback_state_set(float position, float limit)
{
    s_help_window_feedback_position = position;
    s_help_window_feedback_limit = limit;
}

float help_window_feedback_position_get(void)
{
    return s_help_window_feedback_position;
}

void help_window_feedback_hooks_set(void (*reset)(int, int),
                                    void (*cleanup)(void),
                                    void (*finish)(int))
{
    s_help_window_feedback_reset = reset;
    s_help_window_feedback_cleanup = cleanup;
    s_help_window_feedback_finish = finish;
}

/* Install API for the transition-clear hook. Deviation: the corpus's
 * RenderTransitionCleanupIfActive calls its two cleanup callees directly, so
 * this static has no reader left — the setter stays as dead install API. */
void help_render_transition_clear_hook_set(void (*hook)(int, int))
{
    s_help_render_transition_clear = hook;
}

/* TransitionControl (FUN_00425c30): the corpus's opaque transition leaf. Like
 * flic.cpp's 0x00484c90 fallback, the port carries a file-local no-op so the
 * call site lands the corpus's two pushes; the ledger_stubs entry stays the
 * link-time no-op for the unported callers. */
// STUB: LEGORR 0x00425c30
static void help_transition_control_noop(int a, int b)
{
    (void)a;
    (void)b;
    /* FUN_00425c30: pure return — faithful. */
}

/* RenderTransitionCleanupIfActive (FUN_004189d0): the corpus gates two byte
 * tests of the selection-state DAT, calls TransitionControl(0, 0) and
 * ObjectiveScoreTextOriginSet unconditionally (no hook indirection), then
 * re-reads the DAT and clears bit 0. ObjectiveScoreTextOriginSet arrives with
 * a null pair and a null context, so it writes nothing and drops no debug bit
 * (the corpus's own push of a single 0 reads the same way). */
// FUNCTION: LEGORR 0x004189d0
void help_render_transition_cleanup_if_active(void)
{
    if (((unsigned char)s_sel_state & 0x2u) != 0u) {
        if (((unsigned char)s_sel_state & 0x1u) != 0u) {
            help_transition_control_noop(0, 0);
            objective_score_text_origin_set(NULL);
            s_sel_state &= ~1u;
        }
    }
}

void help_window_button_rect_set(int index, float x, float y,
                                 float width, float height)
{
    if (index < 0 || index >= LAYOUT_POSITION_ENTRIES) {
        return;
    }
    s_help_window_button_rects[index][0] = x;
    s_help_window_button_rects[index][1] = y;
    s_help_window_button_rects[index][2] = width;
    s_help_window_button_rects[index][3] = height;
}

static int help_window_button_record_parse(const char* value, int* x, int* y,
                                           int* width, int* height,
                                           const char** resource_a,
                                           const char** resource_b,
                                           const char** text)
{
    if (value == NULL || x == NULL || y == NULL || width == NULL ||
        height == NULL || resource_a == NULL || resource_b == NULL ||
        text == NULL) {
        return 0;
    }
    const char* cursor = value;
    int* dimensions[4] = { x, y, width, height };
    for (int i = 0; i < 4; ++i) {
        char* end = NULL;
        const long parsed = strtol(cursor, &end, 10);
        if (end == cursor || parsed < -32768 || parsed > 32767 ||
            (i < 3 && *end != ':')) {
            return 0;
        }
        *dimensions[i] = (int)parsed;
        cursor = i < 3 ? end + 1 : end;
    }
    if (*cursor != ':') {
        return 0;
    }
    *resource_a = ++cursor;
    const char* separator = strchr(cursor, ':');
    if (separator == NULL || separator == cursor) {
        return 0;
    }
    cursor = separator + 1;
    *resource_b = cursor;
    separator = strchr(cursor, ':');
    if (separator == NULL || separator == cursor || separator[1] == '\0') {
        return 0;
    }
    *text = separator + 1;
    return 1;
}

static void help_window_label_copy(char* out, size_t out_size,
                                   const char* value)
{
    if (out == NULL || out_size == 0) {
        return;
    }
    if (value == NULL) {
        out[0] = '\0';
        return;
    }
    size_t i = 0;
    for (; value[i] != '\0' && i + 1 < out_size; ++i) {
        out[i] = value[i] == '_' ? ' ' : value[i];
    }
    out[i] = '\0';
}

// FUNCTION: LEGORR 0x00418450
void help_window_info_config_strings_load(
    BlockFile* cfg, const char* const* const group_names[3],
    const size_t group_counts[3],
    void (*store)(void* context, int group, size_t catalog_index,
                  int line_index, const char* text),
    void* context)
{
    if (group_names == NULL || group_counts == NULL || store == NULL) {
        return;
    }
    for (int group = 0; group < 3; ++group) {
        if (group_names[group] == NULL) {
            continue;
        }
        for (size_t catalog_index = 0;
             catalog_index < group_counts[group]; ++catalog_index) {
            const char* const catalog = group_names[group][catalog_index];
            for (int line_index = 0; line_index < 16; ++line_index) {
                const char* value = NULL;
                char path[1024];
                if (catalog != NULL) {
                    const int written = snprintf(
                        path, sizeof(path),
                        "HelpWindow::HelpWindowInfo::%sL%d",
                        catalog, line_index);
                    if (written >= 0 && written < (int)sizeof(path)) {
                        value = block_get_strptr(cfg, path);
                    }
                }

                char text[1024];
                if (value != NULL) {
                    help_window_label_copy(text, sizeof(text), value);
                    store(context, group, catalog_index, line_index, text);
                } else {
                    store(context, group, catalog_index, line_index, NULL);
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x004181b0
void help_window_resources_init(BlockFile* cfg,
                                const FeHelpWindowResourceOps* ops)
{
    if (ops != NULL && ops->info_strings_load != NULL) {
        ops->info_strings_load(ops->context, cfg);
    } else if (ops != NULL && ops->info_string_store != NULL) {
        help_window_info_config_strings_load(
            cfg, ops->info_group_names, ops->info_group_counts,
            ops->info_string_store, ops->context);
    }

    const char* background = block_get_strptr(cfg,
        "HelpWindow::BackGroundImage");
    if (background != NULL) {
        const char* first_separator = strchr(background, ':');
        const char* second_separator = first_separator != NULL
            ? strchr(first_separator + 1, ':') : NULL;
        if (first_separator != NULL && second_separator != NULL &&
            first_separator != background && second_separator[1] != '\0') {
            char name[256];
            const size_t length = (size_t)(first_separator - background);
            char* end_x = NULL;
            char* end_y = NULL;
            const long offset_x = strtol(first_separator + 1, &end_x, 10);
            const long offset_y = strtol(second_separator + 1, &end_y, 10);
            if (length < sizeof(name) && end_x == second_separator &&
                end_y != second_separator + 1 && *end_y == '\0' &&
                offset_x >= -32768 && offset_x <= 32767 &&
                offset_y >= -32768 && offset_y <= 32767) {
                memcpy(name, background, length);
                name[length] = '\0';
                s_help_background_offset_x = (float)offset_x;
                s_help_background_offset_y = (float)offset_y;
                s_help_background_handle = ops != NULL &&
                    ops->resource_load != NULL
                    ? ops->resource_load(ops->context, name, 0, 0) : 0u;
                if (s_help_background_handle != 0u && ops != NULL &&
                    ops->background_apply != NULL) {
                    ops->background_apply(ops->context,
                                          s_help_background_handle);
                }
            }
        }
    }

    help_window_button_layout_load(
        cfg, s_help_background_offset_x, s_help_background_offset_y,
        ops != NULL ? ops->resource_load : NULL,
        ops != NULL ? ops->text_load : NULL,
        ops != NULL ? ops->context : NULL);

    const char* text_area = block_get_strptr(cfg,
        "HelpWindow::TextWindowArea");
    int area[4];
    if (fe_parse_int_tuple(text_area, area, 4)) {
        s_help_text_area_handle = ops != NULL && ops->text_area_create != NULL
            ? ops->text_area_create(ops->context,
                                    selection_mode_state_peek(), area, 0x400u)
            : 0u;
    }

    static const char* const label_keys[3] = {
        "HelpWindow::CanBuildMessage",
        "HelpWindow::CanTransportMessage",
        "HelpWindow::CanBuildAndTransportMessage"
    };
    for (int i = 0; i < 3; ++i) {
        help_window_label_copy(s_help_window_labels[i],
                               sizeof(s_help_window_labels[i]),
                               block_get_strptr(cfg, label_keys[i]));
        if (ops != NULL && ops->label_set != NULL) {
            ops->label_set(ops->context, i, s_help_window_labels[i]);
        }
    }
    s_sel_state |= 6u;
    if (ops != NULL && ops->refresh != NULL) {
        ops->refresh(ops->context, 0, 0);
    }
}

// FUNCTION: LEGORR 0x004185f0
void help_window_button_layout_load(
    BlockFile* cfg, float offset_x, float offset_y,
    uint32_t (*resource_load)(void* context, const char* name, int x, int y),
    void (*text_load)(void* context, int slot, const char* name),
    void* context)
{
    static const char* const keys[LAYOUT_POSITION_ENTRIES] = {
        "HelpWindow::ContinueButton", "HelpWindow::CancelFutureButton"
    };
    for (int i = 0; i < LAYOUT_POSITION_ENTRIES; ++i) {
        const char* const value = block_get_strptr(cfg, keys[i]);
        int x, y, width, height;
        const char *resource_a, *resource_b, *text;
        if (!help_window_button_record_parse(value, &x, &y, &width, &height,
                                             &resource_a, &resource_b, &text)) {
            continue;
        }
        const float final_x = (float)x + offset_x;
        const float final_y = (float)y + offset_y;

        char first[256], second[256];
        const char* end = strchr(resource_a, ':');
        size_t length = end != NULL ? (size_t)(end - resource_a) : 0;
        if (length == 0 || length >= sizeof(first)) {
            continue;
        }
        memcpy(first, resource_a, length);
        first[length] = '\0';
        end = strchr(resource_b, ':');
        length = end != NULL ? (size_t)(end - resource_b) : 0;
        if (length == 0 || length >= sizeof(second)) {
            continue;
        }
        memcpy(second, resource_b, length);
        second[length] = '\0';

        help_window_button_rect_set(i, final_x, final_y,
                                    (float)width, (float)height);
        layout_position_set(i, (int)final_x, (int)final_y);
        const uint32_t handle_a = resource_load != NULL
            ? resource_load(context, first, 0, 0) : 0u;
        const uint32_t handle_b = resource_load != NULL
            ? resource_load(context, second, 0, 0) : 0u;
        selection_state_handle_slot_set(0, i, handle_a);
        selection_state_handle_slot_set(1, i, handle_b);
        if (i == 1) {
            selection_state_handle_extra_set(handle_b);
        }
        if (text_load != NULL) {
            text_load(context, i == 0 ? 8 : 9, text);
        }
    }
}

void help_window_sound_clear_hook_set(void (*hook)(int))
{
    s_help_window_sound_clear = hook;
}

// FUNCTION: LEGORR 0x00418da0
int help_window_button_hit_test(uint32_t x, uint32_t y, int mode,
                                int transition_flags, int active,
                                float feedback_amount)
{
    if ((s_sel_state & 3u) != 3u) {
        return 0;
    }

    s_sel_state &= ~0x30u;
    const float pointer_x = (float)x;
    const float pointer_y = (float)y;
    if (pointer_x >= 345.0f &&
        pointer_x <= (float)display_width_get() + 345.0f &&
        pointer_y >= 350.0f &&
        pointer_y <= (float)display_height_get() + 350.0f && active != 0) {
        timed_effect_update_request_set(0x21);
        help_window_button_feedback_update(3, 5.0f);
        if (s_help_window_sound_clear != NULL) {
            s_help_window_sound_clear(0);
        }
    }

    for (int i = 0; i < LAYOUT_POSITION_ENTRIES; ++i) {
        const float* const rect = s_help_window_button_rects[i];
        if (pointer_x < rect[0] || pointer_x > rect[0] + rect[2] ||
            pointer_y < rect[1] || pointer_y > rect[1] + rect[3]) {
            continue;
        }

        s_sel_state |= 0x10u;
        s_sel_hit_index = i;
        const int transitioned = ui_mode_transition_request(
            i, mode, transition_flags);
        if (transitioned != 0 && active != 0) {
            help_window_button_feedback_update(i, feedback_amount);
            if (s_help_window_sound_clear != NULL) {
                s_help_window_sound_clear(0);
            }
        }
        if (mode == 0 && active == 0) {
            selection_mode_set_by_index(i);
        }
        return 1;
    }

    (void)ui_mode_transition_request(4, mode, transition_flags);
    return 0;
}

void layout_position_set(int entry, int x, int y)
{
    if (entry >= 0 && entry < LAYOUT_POSITION_ENTRIES) {
        s_layout_position[entry][0] = x;
        s_layout_position[entry][1] = y;
    }
}

int layout_position_get(int entry, int* x, int* y)
{
    if (entry < 0 || entry >= LAYOUT_POSITION_ENTRIES) {
        return 0;
    }
    if (x != NULL) {
        *x = s_layout_position[entry][0];
    }
    if (y != NULL) {
        *y = s_layout_position[entry][1];
    }
    return 1;
}

/* --- the front-end flow words (see frontend.hpp) ----------------------------
 * Each word is its own zero-initialized static; the winapi build annotates
 * them individually at their corpus DAT addresses, whose zero-filled .data
 * tail makes that the faithful initial state (finding 23/438). */
// FUNCTION: LEGORR 0x00416d50
int save_slot_open(void)
{
    return FE_FLOW_SAVE_SLOT != -1;
}

// FUNCTION: LEGORR 0x00416d60
int is_in_game(void)
{
    return FE_FLOW_INGAME_SLOT != -1;
}

// FUNCTION: LEGORR 0x00416dd0
int quit_menu_probe(void)
{
    return FE_FLOW_QUIT_WORD;
}

// FUNCTION: LEGORR 0x00416de0
int level_start_flag(void)
{
    return FE_FLOW_LEVEL_START != 0;
}

// FUNCTION: LEGORR 0x00416df0
int menu_options_flag(void)
{
    return FE_FLOW_MENU_OPTIONS != 0;
}

/* OptionMusicDefault (FUN_00414e10) queries a WinMM audio-CD mixer source
 * and returns its current value, or zero when no such source is available.
 * The port's streamed music has no CD mixer source, so it follows that
 * unavailable-source result instead of treating the DirectSound/WAV gain as
 * a CD volume. */
// FUNCTION: LEGORR 0x00414e10
int option_music_default(void)
{
    return 0;
}

void frontend_flow_word_set(int which, int value)
{
    if (which >= 0 && which < 5) {
        switch (which) {
        case 0:  FE_FLOW_QUIT_WORD = value;      break;
        case 1:  FE_FLOW_SAVE_SLOT = value;      break;
        case 2:  FE_FLOW_INGAME_SLOT = value;    break;
        case 3:  FE_FLOW_LEVEL_START = value;    break;
        default: FE_FLOW_MENU_OPTIONS = value;   break;
        }
    }
}

/* MainSimStateAdmissionPredicate (FUN_004191a0) — see frontend.hpp. The
 * corpus reads bits 1/2 of the selection word directly and the 0x40 bit
 * through its own leaf; all three reduce to the same modeled word. */
// FUNCTION: LEGORR 0x4191a0
int main_sim_state_admission_predicate(void)
{
    const uint32_t sel = s_sel_state;
    return ui_mode_resource_bit40_get() != 0u
        && (sel & 2u) != 0u
        && (sel & 1u) != 0u
        && system_state_flags_get() == 0u;
}

/* SelectionModeStateSet (FUN_00418190) — zero-init module static (the
 * corpus's DAT_004db8e8 word; its corpus readers are unported). */
// GLOBAL: LEGORR 0x004db8e8
static uint32_t s_sel_mode_state;

// FUNCTION: LEGORR 0x418190
void selection_mode_state_set(uint32_t value)
{
    s_sel_mode_state = value;
}

uint32_t selection_mode_state_peek(void)
{
    return s_sel_mode_state;
}

/* MenuWipeEnd (FUN_00410440) — DAT_005574f0 as a zero-init static; the
 * corpus's MenuWipeStart-side setter is unported. */
// FUNCTION: LEGORR 0x410440
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void menu_wipe_end(void)
{
    /* Symbolic operands (like menu_wipe_start): __emit-encoded operand
     * dwords carry no relocation, so reccmp resolved them as an OFFSET
     * into s_reference_menu_data_pad0 and scored the two movs as
     * mismatches (measured 50.0%); the symbolic form resolves through
     * the 0x005574f0 annotation as DATA. */
    __asm {
        mov eax, s_menu_wipe
        and al, 0xfc
        mov s_menu_wipe, eax
        ret
    }
}
#else
void menu_wipe_end(void)
{
    s_menu_wipe &= ~3u;
}
#endif

uint32_t menu_wipe_flags_peek(void)
{
    return s_menu_wipe;
}

void menu_wipe_flags_set(uint32_t flags)
{
    s_menu_wipe = flags;
}

/* --- the selection-region band (see frontend.hpp) ---------------------------
 * Zero-initialized statics per the unowned-global convention: the corpus's
 * words have no ported owner (its UI/hit-test writers are unported), so
 * the zero load image is the faithful state (deviation). */
static uint32_t s_sel_c0c;        /* DAT_004ddd0c */
static uint32_t s_sel_c10;        /* DAT_004ddd10 */
static float    s_sel_c14;        /* DAT_004ddd14 */
static float    s_sel_c18;        /* DAT_004ddd18 */
/* The corpus's parameter register quintet, DAT_004ddce0..f0 at stride 4.
 * Each float carries its own GLOBAL annotation: Reccmp only resolves
 * whole-object operands, so a shared-array base never matches the corpus's
 * absolute accesses in the FUN_0041a990 and FUN_0041e9d0 readers. */
// GLOBAL: LEGORR 0x004ddce0
static float s_reg_e0;            /* DAT_004ddce0 */
// GLOBAL: LEGORR 0x004ddce4
static float s_reg_e4;            /* DAT_004ddce4 */
// GLOBAL: LEGORR 0x004ddce8
static float s_reg_e8;            /* DAT_004ddce8 */
// GLOBAL: LEGORR 0x004ddcec
static float s_reg_ec;            /* DAT_004ddcec */
// GLOBAL: LEGORR 0x004ddcf0
static float s_reg_f0;            /* DAT_004ddcf0 */
// GLOBAL: LEGORR 0x004ddccc
static uint32_t s_obj_sel_word;   /* DAT_004ddccc */
// GLOBAL: LEGORR 0x004ddcd0
static uint32_t s_obj_track_word; /* DAT_004ddcd0 */
// GLOBAL: LEGORR 0x004de1ec
static float    s_inter_timer = 0.0f; /* DAT_004de1ec */
// GLOBAL: LEGORR 0x004de1f8
static uint32_t s_inter_flags;    /* DAT_004de1f8 */
// GLOBAL: LEGORR 0x004ddd00
static uint32_t s_pointer_action_type; /* DAT_004ddd00 */
// GLOBAL: LEGORR 0x004ddd04
static uint32_t s_pointer_action_kind; /* DAT_004ddd04 */
// GLOBAL: LEGORR 0x004ddd08
static uint32_t s_pointer_action_entry;/* DAT_004ddd08 */

/* PointerActionStateDispatch (FUN_0041c6e0): update the current pointer
 * action latch and its input flags. The interaction lock at +0x1f8 bit
 * 0x1000 suppresses state changes while preserving the prior-state result. */
// FUNCTION: LEGORR 0x0041C6E0
int pointer_action_state_dispatch(int action, int kind, int entry,
                                  int active, int suppress_initial)
{
    const uint32_t previous_action = s_pointer_action_type;
    if (active != 0) {
        if ((s_inter_flags & 0x1000u) == 0u) {
            if (s_pointer_action_type == 0x4au) {
                if (suppress_initial != 0) {
                    goto finish;
                }
                s_pointer_action_type = (uint32_t)action;
                s_pointer_action_kind = (uint32_t)kind;
                s_pointer_action_entry = (uint32_t)entry;
                if (action == 0x49) {
                    player_input_flag8_set_clear(kind, entry, 1);
                    goto finish;
                }
            }
            else {
                if (s_pointer_action_type != (uint32_t)action) {
                    goto finish;
                }
                if (action == 0x49) {
                    if (s_pointer_action_kind == (uint32_t)kind
                        && s_pointer_action_entry == (uint32_t)entry) {
                        player_input_flag8_set_clear(kind, entry, 1);
                    }
                    goto finish;
                }
            }
            input_entry_flag8_set_clear(action, 1);
        }
    } else {
        s_pointer_action_type = 0x4au;
    }

finish:
    if (previous_action != (uint32_t)action) {
        return 0;
    }
    /* FUN_0041c6e0 has distinct true exits for the non-0x49 and pair arms. */
    if (action == 0x49) {
        if (s_pointer_action_kind == (uint32_t)kind
            && s_pointer_action_entry == (uint32_t)entry) {
            return 1;
        }
    }
    else {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041cce0
void selection_coordinates_set(const uint32_t* pair, uint32_t x, uint32_t y)
{
    if (pair == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    s_sel_c0c = pair[0];
    s_sel_c10 = pair[1];
    s_sel_c14 = (float)x; /* the corpus's int->float cast */
    s_sel_c18 = (float)y;
}

/* SelectionRegionParametersSet (FUN_0041e9d0): four integer words and one
 * float. Each word is zero-extended onto the x87 stack (fild of a zeroed
 * qword, an unsigned conversion) and the slots rotate: arg0 -> +08,
 * arg1 -> +0c, arg2 -> +00, arg3 -> +04; the float multiplies the .rdata
 * 0.04 constant in single precision into the +10 slot. */
// GLOBAL: LEGORR 0x0049e298
static const float s_region_scale_004 = 0.04f; /* DAT_0049e298, verified */

// FUNCTION: LEGORR 0x0041e9d0
void selection_region_parameters_set(uint32_t a, uint32_t b, uint32_t c,
                                     uint32_t d, float e)
{
    s_reg_e0 = (float)c;
    s_reg_e4 = (float)d;
    s_reg_f0 = e * s_region_scale_004;
    s_reg_e8 = (float)a;
    s_reg_ec = (float)b;
}

// FUNCTION: LEGORR 0x0041a990
void objective_selection_state_reset(void)
{
    s_obj_sel_word = 0u;
    __builtin_memcpy(&s_obj_track_word, &s_reg_e8, sizeof(s_obj_track_word));
    s_inter_flags |= 0x84u;
}

// FUNCTION: LEGORR 0x0041ee60
void interaction_timer_reset_and_flag_clear(void)
{
    s_inter_timer = 25.0f; /* corpus 0x41c80000 */
    s_inter_flags &= ~0x800u;
}

/* Corpus shape: one register copy; the true arm ORs bit 1 then masks
 * bit 0, the false arm masks bit 1 then bit 0 as two separate ands; each
 * arm stores and exits on its own. MSVC5 tail-merges the two C stores and
 * folds the false arm's masks into 0xfc, so the body is emitted in naked
 * asm to pin the original's line-for-line sequence. */
// FUNCTION: LEGORR 0x00418800
__declspec(naked) void mode_flag_pair_apply_if_bit2(int value)
{
    __asm {
        mov eax, s_sel_state
        test al, 4
        je   done
        mov ecx, dword ptr [esp + 4]
        test ecx, ecx
        je   clear_both
        or   al, 2
        and  al, 0xfe
        mov  s_sel_state, eax
        ret
    clear_both:
        and  al, 0xfd
        and  al, 0xfe
        mov  s_sel_state, eax
    done:
        ret
    }
}

float selection_coordinates_peek(int which)
{
    switch (which) {
    case 0: return (float)s_sel_c0c;
    case 1: return (float)s_sel_c10;
    case 2: return s_sel_c14;
    case 3: return s_sel_c18;
    }
    return 0.0f;
}

void selection_region_parameters_peek(float out[5])
{
    if (out == NULL) {
        return;
    }
    out[0] = s_reg_e0;
    out[1] = s_reg_e4;
    out[2] = s_reg_e8;
    out[3] = s_reg_ec;
    out[4] = s_reg_f0;
}

uint32_t objective_selection_word_peek(void)
{
    return s_obj_sel_word;
}

/* --- the selection-coordinate store (2026-10-24) ---------------------------
 * Corpus band DAT_004ddcc4/cc8/cfc/dcf4/f8; the other owners of these words
 * are unported seams, so the port models them as own storage (the
 * unowned-global deviation). Each word carries its own GLOBAL annotation:
 * Reccmp only resolves whole-object operands, so accesses at an offset
 * inside a shared array never match. */
// GLOBAL: LEGORR 0x004ddcc4
static uint32_t s_map_store_pair_x;   /* DAT_004ddcc4 */
// GLOBAL: LEGORR 0x004ddcc8
static uint32_t s_map_store_pair_y;   /* DAT_004ddcc8 */
// GLOBAL: LEGORR 0x004ddcf4
static uint32_t s_map_store_last_x;   /* DAT_004ddcf4 */
// GLOBAL: LEGORR 0x004ddcf8
static uint32_t s_map_store_last_y;   /* DAT_004ddcf8 */
// GLOBAL: LEGORR 0x004ddcfc
static uint32_t s_map_store_kind;     /* DAT_004ddcfc */

// FUNCTION: LEGORR 0x0041ea50
void store_selected_map_coordinate(int kind, const uint32_t coord[2])
{
    if ((s_inter_flags & 0x80u) == 0u
        || s_obj_sel_word != (uint32_t)kind) {
        s_inter_flags = (s_inter_flags & ~0x80u) | 0x100u;
        sound_alias_play(0x16, 0);
    } else {
        s_obj_sel_word = (uint32_t)kind;
        if (coord == NULL) {
            s_map_store_kind = (uint32_t)kind;
            return;
        }
        s_map_store_pair_x = coord[0];
        s_map_store_pair_y = coord[1];
    }
    if (coord != NULL) {
        s_map_store_last_x = coord[0];
        s_map_store_last_y = coord[1];
    }
    s_map_store_kind = (uint32_t)kind;
}

uint32_t map_store_pair_word(int which)
{
    if (which == 0) {
        return s_map_store_pair_x;
    }
    if (which == 1) {
        return s_map_store_pair_y;
    }
    return 0u;
}

uint32_t map_store_last_word(int which)
{
    if (which == 0) {
        return s_map_store_last_x;
    }
    if (which == 1) {
        return s_map_store_last_y;
    }
    return 0u;
}

uint32_t map_store_kind_peek(void)
{
    return s_map_store_kind;
}

float interaction_timer_peek(void)
{
    return s_inter_timer;
}

uint32_t interaction_flags_peek(void)
{
    return s_inter_flags;
}

/* AdvisorCallbackScheduleGate (FUN_0041c440) reads DAT_0076abbc as a global
 * enable byte; its writer is outside the port, so preserve the zero-init
 * default and provide an installation hook. FUN_0041b680's list traversal
 * and LAB_0041c470 callback are represented by the supplied scheduler seam. */
static int s_advisor_callback_schedule_enabled;

void advisor_callback_schedule_enabled_set(int enabled)
{
    s_advisor_callback_schedule_enabled = enabled != 0;
}

// FUNCTION: LEGORR 0x0041C440
void advisor_callback_schedule_gate(FeAdvisorCallbackSchedule schedule,
                                    void* context)
{
    if ((s_inter_flags & 0x84u) != 0x84u
        || s_advisor_callback_schedule_enabled == 0
        || schedule == NULL) {
        return;
    }
    schedule(s_obj_sel_word, context);
}

/* AdvisorAnimationSlotIdResolve (FUN_0045c6b0): table-dispatch the supported
 * animation-kind/position pairs to their request slot. The shipped jump table
 * has entries for kinds 0..9; kind 10 passes the range check but indexes past
 * that table, so the port returns the initialized invalid slot (deviation). */
// FUNCTION: LEGORR 0x0045C6B0
int advisor_animation_slot_id_resolve(uint32_t animation_kind,
                                      uint32_t position_index,
                                      uint32_t* slot_id)
{
    if (slot_id == NULL) return 0;
    *slot_id = 0x15u;
    if (animation_kind >= 10u) return 0;

    static const uint8_t dispatch_group[10] = {
        0, 4, 4, 4, 4, 1, 4, 4, 2, 3,
    };
    const uint32_t group = dispatch_group[animation_kind];
    switch (group) {
    case 0:
    case 4:
        if (position_index <= 4u) *slot_id = 6u + position_index;
        break;
    case 1:
        if (position_index <= 1u) *slot_id = 0x0bu + position_index;
        break;
    case 2:
        if (position_index <= 2u) *slot_id = 0x0eu + position_index;
        break;
    case 3:
        if (position_index <= 1u) *slot_id = 0x12u + position_index;
        break;
    }
    return *slot_id != 0x15u;
}

/* --- the cell-action permission window (see frontend.hpp) ------------------- */
#define CELL_PERMISSION_ENTRIES 300 /* corpus's 300-dword clear loop */
static uint32_t s_cell_perm[CELL_PERMISSION_ENTRIES];

// FUNCTION: LEGORR 0x0041e990
int cell_action_permission_get(int column, int row)
{
    if ((system_state_flags_get() & 4u) == 0u) {
        return 1;
    }
    const int index = row + column * 15; /* the corpus's 0xf stride */
    if (index < 0 || index >= CELL_PERMISSION_ENTRIES) {
        return 0; /* deviation: the corpus strides blindly */
    }
    return s_cell_perm[index] != 0u;
}

void cell_action_permission_table_set(int index, uint32_t value)
{
    if (index >= 0 && index < CELL_PERMISSION_ENTRIES) {
        s_cell_perm[index] = value;
    }
}

uint32_t cell_action_permission_table_peek(int index)
{
    if (index < 0 || index >= CELL_PERMISSION_ENTRIES) {
        return 0u;
    }
    return s_cell_perm[index];
}

/* --- the menu-tick gate + status banner word (see frontend.hpp) ------------- */
// GLOBAL: LEGORR 0x004ddd1c
static uint32_t s_status_banner_word;   /* DAT_004ddd1c */

// FUNCTION: LEGORR 0x00412190
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) bool menu_audio_tick_interval_reached(void)
{
    __asm {
        call level_rand_u16
        cdq
        mov ecx, 0x190
        idiv ecx
        xor eax, eax
        test edx, edx
        sete al
        ret
    }
}
#else
bool menu_audio_tick_interval_reached(void)
{
    return level_rand_u16() % 400 == 0;
}
#endif

// FUNCTION: LEGORR 0x0041b9b0
uint32_t sim_status_banner_word_get(void)
{
    return s_status_banner_word;
}

void sim_status_banner_word_set(uint32_t value)
{
    s_status_banner_word = value;
}

// FUNCTION: LEGORR 0x00419200
uint32_t ui_mode_resource_bit40_set_clear(int probe, uint32_t value)
{
    uint32_t v;
    if (probe != 0) {
        v = s_sel_state & 0x40u;
    }
    else {
        /* FUN_00419200 fetches the value argument only after the probe test. */
        v = value;
    }
    /* Corpus layout: the nonzero answer falls through to the clear, the
     * zero answer jumps to the set tail; the word answers after the
     * store in both arms. */
    if (v != 0u) {
        s_sel_state = s_sel_state & ~0x40u; /* plain form: keeps the
            * corpus's mov/and al,0xbf/mov register round-trip (b45
            * global_flag_bit0_clear lesson) */
    } else {
        s_sel_state = s_sel_state | 0x40u;
    }
    return s_sel_state;
}

/* MenuWipeStart (FUN_004103f0) — see frontend.hpp. */
static uint32_t s_menu_clock_millis; /* timeGetTime model */

// FUNCTION: LEGORR 0x004103f0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void menu_wipe_start(void)
{
    __asm {
        mov eax, s_menu_wipe_arm
        sub esp, 8
        test eax, eax
        je menu_wipe_start_done
        mov eax, s_menu_wipe
        mov dword ptr s_menu_wipe_stop, 0
        or al, 1
        or al, 2
        mov s_menu_wipe, eax
        call timeGetTime
        mov dword ptr [esp], eax
        mov dword ptr [esp + 4], 0
        fild qword ptr [esp]
        push 0
        push 0x27
        fstp dword ptr s_menu_wipe_clock
        call sound_request_queue_submit
        add esp, 8
    menu_wipe_start_done:
        add esp, 8
        ret
    }
}
#else
void menu_wipe_start(void)
{
    if (s_menu_wipe_arm == 0u) {
        return;
    }
    s_menu_wipe_stop = 0u;
    s_menu_wipe |= 3u;
    s_menu_wipe_clock = (float)s_menu_clock_millis;
    sound_request_queue_submit(0x27, 0);
}
#endif

void menu_wipe_arm_set(uint32_t value) { s_menu_wipe_arm = value; }
uint32_t menu_wipe_arm_peek(void) { return s_menu_wipe_arm; }
float menu_wipe_clock_peek(void) { return s_menu_wipe_clock; }

void menu_clock_millis_set(uint32_t millis)
{
    s_menu_clock_millis = millis;
}

/* --- the panel texture-slot blit (see frontend.hpp) --------------------------
 * The record's flag word is the float view's +0x28 dword; the texture
 * handles are the +0x10/+0x14/+0x18 float views carried bit-for-bit. A
 * missing draw-record block answers nothing (deviation: the corpus walks
 * the window unconditionally once its count word reads non-null). */
// FUNCTION: LEGORR 0x0045a9c0
void panel_texture_slots_blit(int panel)
{
    const int count = panel_draw_count_peek(panel);
    if (count <= 0) {
        return; /* the corpus's count gate */
    }
    const char* block = (const char*)panel_draw_records_peek(panel);
    if (block == NULL) {
        return; /* deviation: the corpus strides a null block */
    }
    uint32_t oa = 0u, ob = 0u;
    indexed_pair_get(panel, &oa, &ob);
    const float ox = (float)(int)oa;
    const float oy = (float)(int)ob;

    for (int i = 0; i < count; ++i) {
        const float* rec = (const float*)(block + (size_t)i * 0x2c);
        uint32_t flags = 0u;
        __builtin_memcpy(&flags, rec + 10, sizeof(flags));
        if ((flags & 0x10u) != 0u) {
            continue; /* the corpus hides bit-0x10 items */
        }
        const int has40 = (flags & 0x40u) != 0u;
        int banner = 0;
        float pick = 0.0f;
        if ((flags & 8u) == 0u && (flags & 2u) != 0u) {
            if (!has40) {
                pick = rec[4];
            } else {
                banner = (int)sim_status_banner_word_get();
                if (banner == 0) {
                    continue; /* the corpus skips the blit */
                }
                pick = rec[4];
            }
        } else if ((flags & 4u) == 0u && (flags & 8u) == 0u) {
            if (!has40) {
                pick = rec[6];
            } else {
                banner = (int)sim_status_banner_word_get();
                pick = banner == 0 ? rec[6] : rec[4];
            }
        } else {
            if (has40) {
                banner = (int)sim_status_banner_word_get();
                if (banner != 0) {
                    pick = rec[4];
                } else {
                    pick = rec[5];
                }
            } else {
                pick = rec[5];
            }
        }
        uint32_t handle = 0u;
        __builtin_memcpy(&handle, &pick, sizeof(handle));
        if (handle != 0u) {
            crt_surface_blit_record(handle, rec[0] + ox, rec[1] + oy);
        }
    }
}

/* TimedEffectTimestampReset (FUN_0046b7a0) — see frontend.hpp. */
// FUNCTION: LEGORR 0x46b7a0
void timed_effect_timestamp_reset(int index)
{
    if (index >= 0 && index < TIMED_EFFECT_ENTRIES) {
        s_timed_effect_time[index] = s_timed_effect_tick;
    }
    /* deviation: the corpus strides the table blindly at any index. */
}

void timed_effect_tick_word_set(uint32_t tick) { s_timed_effect_tick = tick; }
uint32_t timed_effect_tick_word_peek(void) { return s_timed_effect_tick; }

uint32_t timed_effect_time_peek(int index)
{
    if (index < 0 || index >= TIMED_EFFECT_ENTRIES) {
        return 0u;
    }
    return s_timed_effect_time[index];
}

/* ScreenRightEdgeCacheRefresh (FUN_0041a290) — see frontend.hpp. */
static uint32_t s_screen_right_edge; /* DAT_004dcd44 role: unowned */

// FUNCTION: LEGORR 0x41a290
void screen_right_edge_cache_refresh(void)
{
    uint32_t x = 0, y = 0;
    indexed_text_metrics_pair_get(0, &x, &y);
    s_screen_right_edge = x - y;
}

uint32_t screen_right_edge_cache_peek(void) { return s_screen_right_edge; }

/* BitmapFontGlyphMeasureWidth (FUN_0047a520) — see frontend.hpp. */
// FUNCTION: LEGORR 0x47a520
void bitmap_font_glyph_measure_width(void* font, unsigned char glyph)
{
    (void)font; /* deviation: no corpus font record is addressed */
    const unsigned row = ((unsigned)(glyph - 0x20u)) / FE_FONT_GRID_COLS;
    if (row < FE_FONT_GRID_ROWS) {
        /* the corpus's flag-clear measure lane keeps its recomputed floats
         * in locals — nothing observable here, faithful to the wrapper. */
    }
}

/* --- the panel resource + scroll leaf band (see frontend.hpp) --------------
 * The corpus's unowned panel words (DAT_004ffe68 enable gate, DAT_004ffed8/
 * edc bubble offsets, DAT_00500810..81c handle/dimensions, DAT_005007d8 UI
 * record slot) land here as zero-init statics; both corpus seams (surface
 * blit FUN_0047dcb0, surface load FUN_0047d9d0) are hook boundaries with
 * no ported body — the seam calls record and the load answers NULL (the
 * view object-layer precedent). */
static uint32_t s_panel_enable_word; /* DAT_004ffe68 role */
static float    s_bubble_offset_x;   /* DAT_004ffed8 role */
static float    s_bubble_offset_y;   /* DAT_004ffedc role */
static void*    s_panel_surface_handle; /* DAT_00500810 role */
static float    s_panel_width;       /* DAT_00500814 role */
static float    s_panel_height;      /* DAT_00500818 role */
static uint32_t s_panel_extra;       /* DAT_0050081c role */
// GLOBAL: LEGORR 0x005007d8
static void*    s_panel_ui_record;   /* DAT_005007d8 role */
static void*    s_panel_text_slot;   /* DAT_005007dc role: holder-slot address */

static void*    s_panel_position_slots[5]; /* DAT_004ffec4 role: panel slots */
static float    s_panel_position_offset_x; /* DAT_004ffebc role */
static float    s_panel_position_offset_y; /* DAT_004ffec0 role */

static int    s_panel_seam_calls;
static void*  s_panel_seam_last_res;
static float  s_panel_seam_last_x;
static float  s_panel_seam_last_y;

static void panel_blit_seam_call(void* res, float x, float y)
{
    ++s_panel_seam_calls;
    s_panel_seam_last_res = res;
    s_panel_seam_last_x = x;
    s_panel_seam_last_y = y;
}

static void* panel_load_seam_call(void* res)
{
    panel_blit_seam_call(res, 0.0f, 0.0f);
    return NULL; /* the port has no surface layer */
}

// FUNCTION: LEGORR 0x00459fd0
void panel_resource_position_apply_if_enabled(void* resource, int x, int y)
{
    /* The corpus tests the enable byte, then FILD/FADD-stores x before y
     * into its local coordinate pair. FUN_0047dcb0 is an unported hook
     * boundary here, so preserve the observable test seam. */
    if (((unsigned char)s_panel_enable_word & 0x10u) != 0u) {
        float position[2];
        position[0] = (float)x + s_bubble_offset_x;
        position[1] = (float)y + s_bubble_offset_y;
        panel_blit_seam_call(resource, position[0], position[1]);
    }
}

/* IndexedPanelResourcePositionApply (FUN_00459f50) — see frontend.hpp.
 * The corpus reads the slot selector off the record's screen-position
 * float (+0x344), multiplied by its two float constants in the x87
 * register's extended precision (modeled as exact double products) and
 * truncated toward zero, then clamped at the last slot by the unsigned
 * compare (a negative conversion clamps to 4 too); the slot table and
 * offsets are own storage (the position-config loads are recorded config
 * seams), and the position rides the surface blit seam. Deviations: the
 * port takes the record float's address instead of the corpus's blind
 * record+0x344 read, and a NULL float reads zero. */
// FUNCTION: LEGORR 0x00459f50
void indexed_panel_resource_position_apply(const float* record_position,
                                           int x, int y)
{
    if ((s_panel_enable_word & 1u) == 0u) {
        return;
    }
    const float v = (record_position != NULL ? *record_position : 0.0f);
    const int sel = (int)(((double)v * (double)0.01f) * (double)5.0f);
    uint32_t slot = (uint32_t)sel;
    if (slot > 4u) {
        slot = 4u; /* the corpus jbe: every out-of-range index, negative
                    * conversions included, rides the last slot */
    }
    void* res = s_panel_position_slots[slot];
    if (res == NULL) {
        return;
    }
    panel_blit_seam_call(res, (float)x + s_panel_position_offset_x,
                         (float)y + s_panel_position_offset_y);
}

// FUNCTION: LEGORR 0x0045c520
void panel_resource_handle_and_dimensions_store(void* resource, uint32_t w,
                                                uint32_t h, uint32_t extra)
{
    /* corpus order: load, +0x14 width, +0x1c extra, +0x18 height */
    s_panel_surface_handle = panel_load_seam_call(resource);
    s_panel_width = (float)w;
    s_panel_extra = extra;
    s_panel_height = (float)h;
}

// FUNCTION: LEGORR 0x0041a250
void scroll_offset_from_cursor_update(void)
{
    uint32_t lo = 0, hi = 0;
    /* the corpus pushes the DAT_005007d8 record value itself, whose +0
     * field the forward reads; the unowned-global deviation keeps the
     * slot NULL-defaulted and the callee's guards answer zeros */
    ui_record_callback_forward((const void* const*)s_panel_ui_record,
                               &lo, &hi);
    int diff = (int)(lo - hi);
    if (diff < 0) {
        diff = 0;
    }
    indexed_text_metrics_pair_set(0, (uint32_t)diff, 0u);
    screen_right_edge_cache_refresh();
}

void     panel_enable_word_set(uint32_t word) { s_panel_enable_word = word; }
uint32_t panel_enable_word_peek(void) { return s_panel_enable_word; }

/* --- the panel layout band (2026-10-25; frontend.hpp) ----------------------
 * DAT_005007f0/7f4 handle slots, DAT_005007f8/7fc size floats,
 * DAT_00500800 extra word and DAT_00500804/808 offset floats have no
 * ported corpus owner — zero-init own storage (the unowned-global
 * deviation). The corpus surface loads (FUN_0047d9d0) stay hook boundaries
 * answering NULL (the panel-load precedent), and the blits ride the seam. */
static void* s_panel_layout_surface;     /* DAT_005007f0 role */
static void* s_panel_layout_glyph;       /* DAT_005007f4 role */
static float s_panel_layout_size[2];     /* DAT_005007f8/7fc role */
static uint32_t s_panel_layout_extra;    /* DAT_00500800 role */
static float s_panel_layout_offset[2];   /* DAT_00500804/808 role */

// FUNCTION: LEGORR 0x0045c340
void panel_resource_layout_store(void* face_resource, uint32_t width,
                                 uint32_t height, uint32_t extra,
                                 void* glyph_resource, uint32_t position_x,
                                 uint32_t position_y)
{
    s_panel_layout_surface = panel_load_seam_call(face_resource);
    s_panel_layout_size[0] = (float)width;
    s_panel_layout_size[1] = (float)height;
    s_panel_layout_extra = extra;
    s_panel_layout_glyph = panel_load_seam_call(glyph_resource);
    if (s_panel_layout_glyph != NULL) {
        panel_blit_seam_call(s_panel_layout_glyph, 0.0f, 0.0f);
    }
    s_panel_layout_offset[0] = (float)position_x;
    s_panel_layout_offset[1] = (float)position_y;
}

// FUNCTION: LEGORR 0x0045c4b0
void panel_resource_position_update(int index)
{
    if (s_panel_layout_glyph == NULL) {
        return;
    }
    uint32_t x = 0, y = 0;
    indexed_pair_get(index, &x, &y);
    const float px = (float)(int32_t)x + s_panel_layout_offset[0];
    const float py = (float)(int32_t)y + s_panel_layout_offset[1];
    if (global_value_0050080c_peek() == 0u) {
        return;
    }
    if (sim_status_banner_word_get() != 0u) {
        panel_blit_seam_call(s_panel_layout_glyph, px, py);
    }
}

void* panel_layout_surface_peek(void) { return s_panel_layout_surface; }
void* panel_layout_glyph_peek(void) { return s_panel_layout_glyph; }
uint32_t panel_layout_extra_peek(void) { return s_panel_layout_extra; }
float panel_layout_size_peek(int which)
{
    if (which < 0 || which > 1) {
        return 0.0f; /* deviation: the corpus strides the pair blindly */
    }
    return s_panel_layout_size[which];
}
void panel_layout_glyph_install(void* surface)
{
    s_panel_layout_glyph = surface;
    /* deviation: the corpus glyph word lands only through the surface-load
     * seam (NULL answers uninstalled); the hook stands in for the seam. */
}

/* --- the load band + advisor progress bar (2026-10-25) ---------------------
 * InterfaceButtonImagesLoad (FUN_0041b270) and ThreeSurfaceResourcesLoad
 * AndInitialize (FUN_0045bfb0) load their paths through the surface-load
 * seam (the string cache) and, for every non-zero handle, call the surface
 * transform seam — the seam fold records the corpus's (handle, 0, 0) call.
 * Quirk pinned: the three-way takes param_2 into the THIRD window slot and
 * param_3 into the second (the corpus stores 7e0, 7e8, 7e4 in call order).
 * Deviations: the corpus keeps loaded surface records; the port carries the
 * cache value's pointer bits as the handle bits (LP64), and the DAT windows
 * are own storage (their corpus owners are unported).
 *
 * AdvisorProgressBarFillDraw (FUN_0045c3f0): the fill blit — gate on the
 * layout face surface and a strictly positive percent, clamp the percent at
 * the corpus's 100.0 constant, then ride the seam with the row's pair (read
 * as the corpus reads it, float bits) offset by the layout size pair and a
 * size witness {extra * 0.01 * percent, face+8 int->float}. Deviation: the
 * face+8 read needs an installed face buffer (the load seam answers NULL).
 *
 * MenuSurfaceBlitScaled (FUN_00412af0): the side-selectable menu face blit.
 * A set flag byte (low bit of record +0x34) takes the plain destination
 * blit from record +0x38; otherwise the scaled lane rides the seam with the
 * source rect {-pan_x, -pan_y, 640, 480} onto the origin. Deviations: the
 * corpus record and the plane-pan globals ride own storage (the corpus's
 * per-submenu pan writer lands in FeSubmenu.scroll, apart from this
 * window). */
void panel_layout_surface_install(void* surface)
{
    s_panel_layout_surface = surface;
    /* test hook: the load seam's face word (deviation as in glyph_install) */
}

static uint32_t s_interface_button_surfaces[2]; /* DAT_004de1e4/e8 roles */
static uint32_t s_advisor_bar_surfaces[3];      /* DAT_005007e0/e8/e4 roles */
static uint32_t s_menu_blit_record[16];         /* corpus record window */

static uint32_t surface_load_handle(const char* path)
{
    return (uint32_t)(uintptr_t)string_cache_lookup_or_load(path);
}

// FUNCTION: LEGORR 0x0041b270
void interface_button_images_load(const char* left_path,
                                  const char* right_path)
{
    const char* paths[2] = { left_path, right_path };
    for (int i = 0; i != 2; ++i) {
        const uint32_t handle = surface_load_handle(paths[i]);
        s_interface_button_surfaces[i] = handle;
        if (handle != 0u) {
            crt_surface_blit_record(handle, 0.0f, 0.0f); /* transform */
        }
    }
}

uint32_t interface_button_surface_peek(int which)
{
    return (which >= 0 && which < 2) ? s_interface_button_surfaces[which] : 0u;
}

// FUNCTION: LEGORR 0x0045bfb0
void three_surface_resources_load(const char* first, const char* second,
                                  const char* third)
{
    const char* paths[3] = { first, second, third };
    /* corpus store order: slot 0 (DAT_005007e0), slot 1 lands at the
     * THIRD peek index (DAT_005007e8), slot 2 at the second (DAT_005007e4)
     */
    uint32_t* const slots[3] = { &s_advisor_bar_surfaces[0],
                                 &s_advisor_bar_surfaces[2],
                                 &s_advisor_bar_surfaces[1] };
    for (int i = 0; i != 3; ++i) {
        const uint32_t handle = surface_load_handle(paths[i]);
        *slots[i] = handle;
        if (handle != 0u) {
            crt_surface_blit_record(handle, 0.0f, 0.0f); /* transform */
        }
    }
}

uint32_t advisor_bar_surface_peek(int which)
{
    return (which >= 0 && which < 3) ? s_advisor_bar_surfaces[which] : 0u;
}

// FUNCTION: LEGORR 0x0045c3f0
void advisor_progress_bar_fill_draw(int row, float percent)
{
    if (s_panel_layout_surface == NULL || !(0.0f < percent)) {
        return;
    }
    float pct = percent;
    if (100.0f < pct) {
        pct = 100.0f; /* the corpus constant */
    }
    int field8 = 0;
    __builtin_memcpy(&field8, (const char*)s_panel_layout_surface + 8,
                     sizeof field8);
    uint32_t pair_x = 0u, pair_y = 0u;
    indexed_pair_get(row, &pair_x, &pair_y);
    const float dest_x
        = crt_float_bits(pair_x) + panel_layout_size_peek(0);
    const float dest_y
        = crt_float_bits(pair_y) + panel_layout_size_peek(1);
    const float size[2] = {
        (float)((double)(float)panel_layout_extra_peek()
                * (double)(float)(1.0 / 100.0) * (double)pct),
        (float)field8
    };
    crt_surface_blit_record_rect(
        (uint32_t)(uintptr_t)s_panel_layout_surface, dest_x, dest_y, NULL,
        size);
}

void menu_blit_record_write(int index, uint32_t value)
{
    if (index >= 0 && index < 16) {
        s_menu_blit_record[index] = value;
    }
}

uint32_t menu_blit_record_read(int index)
{
    return (index >= 0 && index < 16) ? s_menu_blit_record[index] : 0u;
}

void menu_blit_record_clear(void)
{
    __builtin_memset(s_menu_blit_record, 0, sizeof s_menu_blit_record);
}

void menu_plane_pan_set(float x, float y)
{
    s_menu_plane_pan[0] = x;
    s_menu_plane_pan[1] = y;
}

float menu_plane_pan_peek(int axis)
{
    return (axis >= 0 && axis < 2) ? s_menu_plane_pan[axis] : 0.0f;
}

// FUNCTION: LEGORR 0x00412af0
void menu_surface_blit_scaled(int percent_side)
{
    const uint32_t handle
        = s_menu_blit_record[percent_side == 0 ? 5 : 4]; /* +0x14 / +0x10 */
    if (handle == 0u) {
        return;
    }
    if ((s_menu_blit_record[13] & 1u) != 0u) { /* record +0x34 flag byte */
        crt_surface_blit_record(handle,
                                crt_float_bits(s_menu_blit_record[14]),
                                crt_float_bits(s_menu_blit_record[15]));
        return;
    }
    const float rect1[4] = { -s_menu_plane_pan[0], -s_menu_plane_pan[1],
                             640.0f, 480.0f };
    crt_surface_blit_record_rect(handle, 0.0f, 0.0f, rect1, NULL);
}

float panel_layout_offset_peek(int which)
{
    if (which < 0 || which > 1) {
        return 0.0f; /* deviation: the corpus strides the pair blindly */
    }
    return s_panel_layout_offset[which];
}
void     bubble_offsets_set(float x, float y)
{
    s_bubble_offset_x = x;
    s_bubble_offset_y = y;
}
void  panel_surface_handle_set(void* h) { s_panel_surface_handle = h; }
void panel_dimensions_peek(float* w, float* h, uint32_t* extra)
{
    if (w != NULL) {
        *w = s_panel_width;
    }
    if (h != NULL) {
        *h = s_panel_height;
    }
    if (extra != NULL) {
        *extra = s_panel_extra;
    }
}
void* panel_surface_handle_peek(void) { return s_panel_surface_handle; }
void  panel_ui_record_set(void* record) { s_panel_ui_record = record; }
void* panel_ui_record_peek(void) { return s_panel_ui_record; }
void  panel_text_slot_set(void* slot) { s_panel_text_slot = slot; }
void* panel_text_slot_peek(void) { return s_panel_text_slot; }

void panel_position_slot_set(int which, void* res)
{
    if (which >= 0 && which < 5) {
        s_panel_position_slots[which] = res;
    }
}
void* panel_position_slot_peek(int which)
{
    return (which >= 0 && which < 5) ? s_panel_position_slots[which] : NULL;
}
void panel_position_offsets_set(float x, float y)
{
    s_panel_position_offset_x = x;
    s_panel_position_offset_y = y;
}

/* ScreenCoordNormalizeInit (FUN_0045d8b0) — see frontend.hpp. The corpus's
 * screen-normalization band spans one data-section block: 90 color
 * constants (scaled *1/255), two stride-16 lanes of 20 unit cells, 14
 * coordinate pairs as seven rows of two mirrors, a row-count word, and
 * two sentinel words. The constants ride the corpus's static data (the
 * colors are pre-scaled values there, the rest zero), so the port keeps
 * the band as own storage seeded by the reset hook; the scaling rides
 * the port's display-dimension pair through the corpus's exact float
 * constants (each lane is one exact x87 product modeled as a double).
 * Quirks pinned: the last primary pair is never written (rides zero)
 * while its mirror takes the first row's constants, and the first
 * mirror row carries a different pair than its primary. */
static const float s_snorm_colors_init[90] = {
    255.0f, 204.0f, 0.0f, 45.0f, 0.0f, 75.0f, 41.0f, 96.0f, 255.0f, 255.0f,
    90.0f, 0.0f, 164.0f, 227.0f, 149.0f, 97.0f, 203.0f, 86.0f, 0.0f, 175.0f,
    12.0f, 0.0f, 93.0f, 0.0f, 0.0f, 47.0f, 0.0f, 199.0f, 123.0f, 255.0f,
    174.0f, 89.0f, 233.0f, 144.0f, 60.0f, 200.0f, 115.0f, 30.0f, 168.0f,
    86.0f, 2.0f, 136.0f, 255.0f, 204.0f, 0.0f, 255.0f, 0.0f, 0.0f, 0.0f,
    255.0f, 0.0f, 255.0f, 123.0f, 251.0f, 233.0f, 89.0f, 226.0f, 200.0f,
    60.0f, 192.0f, 168.0f, 30.0f, 158.0f, 136.0f, 2.0f, 125.0f, 255.0f,
    255.0f, 0.0f, 157.0f, 155.0f, 0.0f, 155.0f, 65.0f, 14.0f, 180.0f,
    255.0f, 0.0f, 255.0f, 255.0f, 0.0f, 0.0f, 47.0f, 181.0f, 191.0f,
    191.0f, 191.0f, 108.0f, 0.0f, 0.0f
};
static float s_snorm_colors[90];                /* DAT band role */
static float s_snorm_cells[2][20];              /* stride-16 unit lanes */
static float s_snorm_pairs[2][7][2];            /* seven rows of mirrors */
static uint32_t s_snorm_rows;                   /* DAT_004a9318 role */
static uint32_t s_snorm_sentinels[2];           /* DAT band sentinels */

static const float s_snorm_pair_consts[2][7][2] = {
    {{320.0f, 40.0f},  {360.0f, 180.0f}, {360.0f, 180.0f}, {400.0f, 180.0f},
     {280.0f, 180.0f}, {220.0f, 180.0f}, {0.0f, 0.0f}},
    {{420.0f, 180.0f}, {360.0f, 180.0f}, {360.0f, 180.0f}, {400.0f, 180.0f},
     {280.0f, 180.0f}, {220.0f, 180.0f}, {320.0f, 40.0f}}
};

void screen_coord_normalize_reset(void)
{
    __builtin_memcpy(s_snorm_colors, s_snorm_colors_init,
                     sizeof s_snorm_colors);
    __builtin_memset(s_snorm_cells, 0, sizeof s_snorm_cells);
    __builtin_memset(s_snorm_pairs, 0, sizeof s_snorm_pairs);
    s_snorm_rows = 0u;
    s_snorm_sentinels[0] = 0u;
    s_snorm_sentinels[1] = 0u;
}

// FUNCTION: LEGORR 0x0045d8b0
void screen_coord_normalize_init(void)
{
    const double color_k = (double)((float)(1.0 / 255.0));
    int i;
    int mirror;
    int row;
    for (i = 0; i < 90; ++i) {
        s_snorm_colors[i] = (float)((double)s_snorm_colors[i] * color_k);
    }
    for (i = 0; i < 20; ++i) {
        s_snorm_cells[0][i] = 1.0f;
        s_snorm_cells[1][i] = 1.0f;
    }
    for (mirror = 0; mirror < 2; ++mirror) {
        for (row = 0; row < 7; ++row) {
            s_snorm_pairs[mirror][row][0] = s_snorm_pair_consts[mirror][row][0];
            s_snorm_pairs[mirror][row][1] = s_snorm_pair_consts[mirror][row][1];
        }
    }
    s_snorm_rows = 7u;
    const double w = (double)((float)display_width_get());
    const double h = (double)((float)display_height_get());
    const double kx = (double)((float)(1.0 / 640.0));
    const double ky = (double)((float)(1.0 / 480.0));
    for (mirror = 0; mirror < 2; ++mirror) {
        for (row = 0; row < 7; ++row) {
            s_snorm_pairs[mirror][row][0]
                = (float)((w * kx) * (double)s_snorm_pairs[mirror][row][0]);
            s_snorm_pairs[mirror][row][1]
                = (float)((h * ky) * (double)s_snorm_pairs[mirror][row][1]);
        }
    }
    s_snorm_sentinels[0] = 0xffffffffu;
    s_snorm_sentinels[1] = 0xffffffffu;
}

float screen_coord_normalize_color_peek(int index)
{
    return (index >= 0 && index < 90) ? s_snorm_colors[index] : 0.0f;
}

float screen_coord_normalize_pair_peek(int mirror, int row, int axis)
{
    if (mirror < 0 || mirror > 1 || row < 0 || row > 6 || axis < 0
        || axis > 1) {
        return 0.0f;
    }
    return s_snorm_pairs[mirror][row][axis];
}

float screen_coord_normalize_cell_peek(int mirror, int index)
{
    if (mirror < 0 || mirror > 1 || index < 0 || index >= 20) {
        return 0.0f;
    }
    return s_snorm_cells[mirror][index];
}

uint32_t screen_coord_normalize_rows_peek(void) { return s_snorm_rows; }

void panel_seam_reset(void)
{
    s_panel_seam_calls = 0;
    s_panel_seam_last_res = NULL;
    s_panel_seam_last_x = 0.0f;
    s_panel_seam_last_y = 0.0f;
}
int panel_seam_calls(void) { return s_panel_seam_calls; }
void panel_seam_last(void** out_res, float* out_x, float* out_y)
{
    if (out_res != NULL) {
        *out_res = s_panel_seam_last_res;
    }
    if (out_x != NULL) {
        *out_x = s_panel_seam_last_x;
    }
    if (out_y != NULL) {
        *out_y = s_panel_seam_last_y;
    }
}

/* --- the active-object word selector (see frontend.hpp) --------------------
 * DAT_00556fd0 (in-game) / DAT_00556fe4 (out) are unowned: zero-init
 * statics here, addressed through the ported in-game test. */
// GLOBAL: LEGORR 0x00556fd0
static int s_active_object_ingame;  /* DAT_00556fd0 role */
// GLOBAL: LEGORR 0x00556fe4
static int s_active_object_outgame; /* DAT_00556fe4 role */

// FUNCTION: LEGORR 0x00416e00
int* active_object_list_word(void)
{
    return is_in_game() != 0 ? &s_active_object_ingame : &s_active_object_outgame;
}

/* --- the screen-effect release (see frontend.hpp) --------------------------
 * The corpus table runs ten entries from DAT_004b8aa0 at stride 0x20: the
 * handle sits at entry +0, the flag dword twenty-eight bytes above the
 * corpus's walk pointer. Unowned: zero-init statics here. */
#define SCREEN_EFFECT_COUNT 10
static void*    s_screen_effect_handle[SCREEN_EFFECT_COUNT];
static uint32_t s_screen_effect_flags[SCREEN_EFFECT_COUNT];
static int     s_screen_effect_seam_calls;
static void*   s_screen_effect_seam_last_handle;
static int     s_screen_effect_seam_last_flag;

// FUNCTION: LEGORR 0x0040a3d0
void screen_effect_flags_clear_and_release(void)
{
    for (int i = 0; i < SCREEN_EFFECT_COUNT; ++i) {
        if ((s_screen_effect_flags[i] & 1u) != 0u) {
            s_screen_effect_flags[i] &= ~1u;
            ++s_screen_effect_seam_calls;
            s_screen_effect_seam_last_handle = s_screen_effect_handle[i];
            s_screen_effect_seam_last_flag = 1;
        }
    }
}

void screen_effect_entry_set(int index, void* handle, uint32_t flags)
{
    if (index >= 0 && index < SCREEN_EFFECT_COUNT) {
        s_screen_effect_handle[index] = handle;
        s_screen_effect_flags[index] = flags;
    }
}
void* screen_effect_handle_peek(int index)
{
    return (index >= 0 && index < SCREEN_EFFECT_COUNT)
               ? s_screen_effect_handle[index] : NULL;
}
uint32_t screen_effect_flag_peek(int index)
{
    return (index >= 0 && index < SCREEN_EFFECT_COUNT)
               ? s_screen_effect_flags[index] : 0u;
}
void screen_effect_seam_reset(void)
{
    s_screen_effect_seam_calls = 0;
    s_screen_effect_seam_last_handle = NULL;
    s_screen_effect_seam_last_flag = 0;
}
int screen_effect_seam_calls(void) { return s_screen_effect_seam_calls; }
void screen_effect_seam_last(void** out_handle, int* out_flag)
{
    if (out_handle != NULL) {
        *out_handle = s_screen_effect_seam_last_handle;
    }
    if (out_flag != NULL) {
        *out_flag = s_screen_effect_seam_last_flag;
    }
}

/* TextLayoutOffsetForCentering (FUN_00411320) — see frontend.hpp. */
// FUNCTION: LEGORR 0x411320
int text_layout_offset_for_centering(int x, int y, int rect_x, int rect_y,
                                     int rect_w, int rect_h, int center)
{
    const int xoff = (center != 0) ? -(rect_w / 2) : 0;
    return menu_pointer_rectangle_hit_test(x, y, xoff + rect_x, rect_y,
                                           rect_w, rect_h);
}

/* SelectionEntityCodeResolve (FUN_0041b9c0) — see frontend.hpp. The corpus
 * reads the first selection first and branches afterwards; the port's
 * order is equivalent since the read is side-effect-free. */
// FUNCTION: LEGORR 0x41b9c0
int selection_entity_code_resolve(const GameContext* ctx, int* code_out)
{
    if ((s_inter_flags & 0x80u) == 0u && (s_inter_flags & 0x100u) != 0u) {
        return (code_out != NULL) ? *code_out : 0;
    }
    const EntitySlot* first = selection_first(ctx);
    const int code = ((first != NULL
                       && (first->flags_3e8 & 0x80000000u) != 0u)
                          ? 0x22 : 0x23);
    if (code_out != NULL) {
        *code_out = code;
    }
    return code;
}

/* ScrollRectResourceRecordCreate (FUN_00463770) — see frontend.hpp. The
 * view-window store preserves the window's other fields (peek/set), and
 * the surface-load seam (FUN_0047d9d0 with the handle and zero positions)
 * answers NULL (the panel-load precedent), so the dim halves stay zero
 * (deviation). */
// FUNCTION: LEGORR 0x463770
void scroll_rect_resource_record_create(int view, int mode,
                                        const float* quad, int tex_a,
                                        int tex_b)
{
    float* pf = (float*)engine_heap_alloc(0x1c);
    if (pf == NULL) {
        return; /* deviation: the corpus strides the store blindly */
    }
    if (quad != NULL) {
        pf[0] = quad[0];
        pf[1] = quad[1];
        pf[2] = quad[2] + quad[0];
        pf[3] = quad[3] + quad[1];
    } else {
        pf[0] = 0.0f;
        pf[1] = 0.0f;
        pf[2] = 0.0f;
        pf[3] = 0.0f;
    }
    pf[4] = 0.0f; /* (float)FUN_0047d9d0(tex_a,0,0): seam answers NULL */
    pf[5] = 0.0f; /* (float)FUN_0047d9d0(tex_b,0,0): seam answers NULL */
    pf[6] = 0.0f;

    void* rect_a = NULL;
    void* rect_b = NULL;
    uint32_t handle = 0u;
    int region = 0;
    view_blit_window_peek(view, &handle, &rect_a, &rect_b, &region);
    if (mode == 0) {
        rect_a = pf;
    } else {
        rect_b = pf;
    }
    view_blit_window_set(view, handle, rect_a, rect_b, region);
}

/* --- overlay animation float (2026-10-25) ---------------------------------
 * The DAT_00500820 animation float is reference state with no port owner,
 * so it lands as a guarded static (the unowned-global deviation) and the
 * corpus x87 float10 lands as plain float. */
static float s_overlay_anim_float; /* DAT_00500820 */

float overlay_anim_float_peek(void)
{
    return s_overlay_anim_float;
}

void overlay_anim_float_install(float value)
{
    s_overlay_anim_float = value;
}

// FUNCTION: LEGORR 0x0045c580
void ui_overlay_animation_advance(int mode, int steps)
{
    /* Deviation: the corpus loops `param_2 != 0` down by one, so a
     * negative count runs away; the port treats it as no steps. */
    for (int i = steps > 0 ? steps : 0; i != 0; --i) {
        if (mode == 0) {
            s_overlay_anim_float =
                (s_overlay_anim_float - 0.05f) * 20.0f * 0.05263158f;
        } else {
            s_overlay_anim_float =
                s_overlay_anim_float - (1.0f - s_overlay_anim_float) * -0.05f;
        }
    }
    if (s_overlay_anim_float < 0.0f) {
        s_overlay_anim_float = 0.0f; /* the after-loop negative guard */
    }
}

/* TimedEffectPersistenceSet (FUN_0046b730) — see frontend.hpp. */
// FUNCTION: LEGORR 0x0046b730
void timed_effect_persistence_set(int index, int set)
{
    if (index < 0 || index >= TIMED_EFFECT_ENTRIES) {
        return; /* deviation: the corpus strides the band blindly */
    }
    s_timed_effect_flags[index] =
        (set != 0) ? (s_timed_effect_flags[index] | 0x10u)
                   : (s_timed_effect_flags[index] & ~0x10u);
}

/* CursorTileSelectionUpdateByType (FUN_0041cc80) — see frontend.hpp. */
/* The corpus addresses the pair as two individual dwords; carve annotated
 * statics (whole-object resolution, the s_fe_flow_* precedent). Own
 * storage, zero-init like the corpus's tail. */
// GLOBAL: LEGORR 0x004ddcd8
static uint32_t s_cursor_tile_first;  /* DAT_004ddcd8 role */
// GLOBAL: LEGORR 0x004ddcdc
static uint32_t s_cursor_tile_second; /* DAT_004ddcdc role */

// FUNCTION: LEGORR 0x0041cc80
void cursor_tile_selection_update_by_type(int kind, uint32_t first,
                                          uint32_t second)
{
    if (kind == 0x49) {
        s_cursor_tile_first = first;
        s_cursor_tile_second = second;
        return;
    }
    if (kind == 5) {
        s_cursor_tile_first = 0xFu;
        s_cursor_tile_second = 0u;
        return;
    }
    if (kind == 0xe) {
        s_cursor_tile_first = 0xBu;
        s_cursor_tile_second = 0u;
    }
    /* the corpus's else branch rides */
}

uint32_t cursor_tile_pair_peek(int which)
{
    if (which < 0 || which > 1) {
        return 0u; /* deviation: the corpus strides the pair blindly */
    }
    return (which == 0) ? s_cursor_tile_first : s_cursor_tile_second;
}

/* --- the UI/panel category-bounds band (2026-10-25) ------------------------
 * The corpus's DAT_00500110 record band (stride 0x30, eighteen windows —
 * the DAT-window convention) carries the panel category rectangles. Only
 * the record+0x18/+0x1c pair and the +0x2c flag byte were modeled earlier
 * (the pair/flag windows in crt.*); the record's own float fields at
 * +0x08..+0x14 and the zeroed word at +0x20 have no ported owner, so they
 * land as own storage (the unowned-global convention). */

#define PANEL_CATEGORY_WINDOWS 18

typedef struct PanelCategoryRecord {
    float    x1;    /* corpus DAT_00500118 role (record+0x08) */
    float    y1;    /* corpus DAT_0050011c role (record+0x0c) */
    float    x2;    /* corpus DAT_00500120 role (record+0x10) */
    float    y2;    /* corpus DAT_00500124 role (record+0x14) */
    uint32_t word;  /* corpus DAT_00500130 word (record+0x20) */
} PanelCategoryRecord;

static PanelCategoryRecord
    s_panel_categories[PANEL_CATEGORY_WINDOWS];

static uint32_t float_bits(float value)
{
    uint32_t bits;
    __builtin_memcpy(&bits, &value, sizeof bits);
    return bits;
}

/* PanelCategoryBoundsSet (FUN_0045a650): store the category rectangle as
 * ints-cast-to-floats, clear the record's word, and route the pair at
 * record+0x18/+0x1c — the first corner while the flag byte's bit 2 rides,
 * the second corner otherwise. Deviations: the window is guarded (the
 * corpus strides blindly) and the corpus's byte-wide bit-2 read is the
 * word's bit 1 on the modeled flag. */
// FUNCTION: LEGORR 0x0045a650
void panel_category_bounds_set(int panel, int x1, int y1, int x2, int y2)
{
    if (panel < 0 || panel >= PANEL_CATEGORY_WINDOWS) {
        return; /* deviation: the corpus strides blindly */
    }
    const float fx1 = (float)x1, fy1 = (float)y1;
    const float fx2 = (float)x2, fy2 = (float)y2;
    s_panel_categories[panel].x1 = fx1;
    s_panel_categories[panel].y1 = fy1;
    s_panel_categories[panel].x2 = fx2;
    s_panel_categories[panel].y2 = fy2;
    s_panel_categories[panel].word = 0u;
    if ((request_record_flags_peek(panel) & 2u) != 0u) {
        indexed_pair_set(panel, float_bits(fx1), float_bits(fy1));
    } else {
        indexed_pair_set(panel, float_bits(fx2), float_bits(fy2));
    }
}

void panel_category_bounds_peek(int panel, float* out_x1, float* out_y1,
                                float* out_x2, float* out_y2)
{
    if (panel < 0 || panel >= PANEL_CATEGORY_WINDOWS) {
        return;
    }
    const PanelCategoryRecord* r = &s_panel_categories[panel];
    if (out_x1 != NULL) {
        *out_x1 = r->x1;
    }
    if (out_y1 != NULL) {
        *out_y1 = r->y1;
    }
    if (out_x2 != NULL) {
        *out_x2 = r->x2;
    }
    if (out_y2 != NULL) {
        *out_y2 = r->y2;
    }
}

/* UiEntryBoundsGet (FUN_0045bf70): read the entry's record out of the
 * panel's installed draw-record block (stride 0x2c) and answer the
 * position plus the derived size — the position dwords ride as-is, the
 * size is the far-corner minus the near one. Deviations: the panel window
 * and the null block are guarded (the corpus strides both). */
// FUNCTION: LEGORR 0x0045bf70
void ui_entry_bounds_get(int panel, int item, float* out_bounds)
{
    if (panel < 0 || panel >= PANEL_CATEGORY_WINDOWS || out_bounds == NULL
        || item < 0) {
        return; /* deviation: the corpus strides blindly */
    }
    const char* block = (const char*)panel_draw_records_peek(panel);
    if (block == NULL) {
        return; /* deviation: the corpus strides a null block */
    }
    const float* rec = (const float*)(block + (size_t)item * 0x2c);
    out_bounds[0] = rec[0];
    out_bounds[1] = rec[1];
    out_bounds[2] = rec[2] - rec[0];
    out_bounds[3] = rec[3] - rec[1];
}

/* PriorityImageSelectionStateUpdate (FUN_0045d300): the corpus's
 * priority-image selection state pair (DAT_005013b8/13bc — no ported
 * owner, own storage). A cleared gate parks the sentinel 0x1b; while the
 * sentinel rides, a cleared third argument installs the candidate and
 * raises flag bit 2; otherwise an equal state just raises the flag. The
 * answer is the pre-call state against the candidate (the corpus threads
 * the stale word through the fall-through). */
static int32_t  s_priority_image_state;  /* DAT_005013b8 */
static uint32_t s_priority_image_flags;  /* DAT_005013bc */

// FUNCTION: LEGORR 0x0045d300
int priority_image_selection_state_update(int candidate, int gate, int skip)
{
    const int32_t prior = s_priority_image_state;
    if (gate == 0) {
        s_priority_image_state = 0x1b;
    } else if (s_priority_image_state == 0x1b) {
        if (skip == 0) {
            s_priority_image_state = candidate;
            s_priority_image_flags |= 2u;
            return candidate == 0x1b ? 1 : 0;
        }
    } else if (s_priority_image_state == candidate) {
        s_priority_image_flags |= 2u;
        return 1;
    }
    return prior == candidate ? 1 : 0;
}

void priority_image_state_set(int32_t state, uint32_t flags)
{
    s_priority_image_state = state;
    s_priority_image_flags = flags;
}

void priority_image_state_peek(int32_t* state, uint32_t* flags)
{
    if (state != NULL) {
        *state = s_priority_image_state;
    }
    if (flags != NULL) {
        *flags = s_priority_image_flags;
    }
}

/* UiRenderRecordCreate (FUN_0045da90): build the 0x3c corpus record —
 * the source, its four copied payload words, the extra argument, and the
 * source's float read at byte offset +0x10 (the corpus threads that read
 * through its float10 widening — a float -> float10 -> float round trip
 * is exact, so the port copies). Deviations: the record fields widen
 * LP64-safe, the payload array and source are guarded, and the corpus's
 * uninitialized record tail stays absent from the modeled record. */
// FUNCTION: LEGORR 0x0045da90
UiRenderRecord* ui_render_record_create(void* source,
                                        const uint32_t* payload,
                                        uint32_t extra)
{
    if (source == NULL || payload == NULL) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    UiRenderRecord* rec = (UiRenderRecord*)engine_heap_alloc(
        (long)sizeof(UiRenderRecord));
    if (rec == NULL) {
        return NULL;
    }
    rec->source = source;
    rec->payload[0] = payload[0];
    rec->payload[1] = payload[1];
    rec->payload[2] = payload[2];
    rec->payload[3] = payload[3];
    rec->extra = extra;
    __builtin_memcpy(&rec->float_at_10,
                     (const unsigned char*)source + 0x10, sizeof rec->float_at_10);
    return rec;
}

/* --- the sav visited-table band (2026-10-25) ------------------------------
 * The corpus's two "visited" tables sit parallel to the modeled
 * active-object count words (out: DAT_00556fe4 count, DAT_00556ff0/ff4
 * item/word arrays; in: DAT_00556fd0 count, DAT_00556fdc/e0 arrays).
 * Their installers sit in the unported sav-tree loaders (the level-node /
 * tree-load / menu-list paths), so the tables land as zero-init own
 * storage behind install/peek hooks (the unowned-global convention). */
#define SAV_VISITED_ENTRIES 16
static void*    s_sav_visited_items_out[SAV_VISITED_ENTRIES]; /* DAT_00556ff0 */
static uint32_t s_sav_visited_words_out[SAV_VISITED_ENTRIES]; /* DAT_00556ff4 */
static void*    s_sav_visited_items_in[SAV_VISITED_ENTRIES];  /* DAT_00556fdc */
static uint32_t s_sav_visited_words_in[SAV_VISITED_ENTRIES];  /* DAT_00556fe0 */

/* SavClearVisited (FUN_00416e70): the corpus walks the out-of-game band
 * first (DAT_00556fe4 / DAT_00556ff0 / DAT_00556ff4), then the in-game
 * band (DAT_00556fd0 / DAT_00556fdc / DAT_00556fe0). A non-null record
 * has its +0x10 dword and paired table word cleared. Keep pointer-array
 * indexing typed: it emits corpus dword strides on x86 and scales pointer
 * slots for the LP64 host. The port-only upper-bound checks stay because
 * the oversized-count test pins the whole band unchanged. */
// FUNCTION: LEGORR 0x00416e70
void sav_clear_visited(void)
{
    int i;
    if (s_active_object_outgame > 0 &&
        s_active_object_outgame <= SAV_VISITED_ENTRIES) {
        for (i = 0; i < s_active_object_outgame; ++i) {
            void* rec = s_sav_visited_items_out[i];
            if (rec != NULL) {
                ((uint32_t*)rec)[4] = 0u; /* corpus record + 0x10 */
                s_sav_visited_words_out[i] = 0u;
            }
        }
    }
    if (s_active_object_ingame > 0 &&
        s_active_object_ingame <= SAV_VISITED_ENTRIES) {
        for (i = 0; i < s_active_object_ingame; ++i) {
            void* rec = s_sav_visited_items_in[i];
            if (rec != NULL) {
                ((uint32_t*)rec)[4] = 0u; /* corpus record + 0x10 */
                s_sav_visited_words_in[i] = 0u;
            }
        }
    }
}

void sav_visited_entry_set(int which, int index, void* record,
                           uint32_t word)
{
    void** items = which != 0 ? s_sav_visited_items_in
                              : s_sav_visited_items_out;
    uint32_t* words = which != 0 ? s_sav_visited_words_in
                                 : s_sav_visited_words_out;
    if (index < 0 || index >= SAV_VISITED_ENTRIES) {
        return; /* deviation: the corpus strides blindly */
    }
    items[index] = record;
    words[index] = word;
}

void sav_visited_entry_peek(int which, int index, void** out_record,
                            uint32_t* out_word)
{
    const void* const* items = which != 0 ? s_sav_visited_items_in
                                          : s_sav_visited_items_out;
    const uint32_t* words = which != 0 ? s_sav_visited_words_in
                                       : s_sav_visited_words_out;
    if (index < 0 || index >= SAV_VISITED_ENTRIES) {
        return; /* deviation: the corpus strides blindly */
    }
    if (out_record != NULL) {
        *out_record = (void*)items[index];
    }
    if (out_word != NULL) {
        *out_word = words[index];
    }
}

/* --- the menu-list find (2026-10-25) ---------------------------------------
 * MenuListFindName (FUN_00414c90): walks the corpus's list header
 * {record-array pointer @+0, count @+4}, answering the index of the first
 * non-NULL entry whose name case-fold-equals the target (the corpus's
 * __strcmpi shape); misses and non-positive counts answer -1. Deviations:
 * the header rides explicit arguments (the header struct has no ported
 * owner) and the entry name pointers are guarded (the corpus strides). */
static int s_name_fold_equal(const char* a, const char* b)
{
    for (;; ++a, ++b) {
        unsigned char ca = (unsigned char)*a;
        unsigned char cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') {
            ca += (unsigned char)('a' - 'A');
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb += (unsigned char)('a' - 'A');
        }
        if (ca != cb) {
            return 0;
        }
        if (ca == 0) {
            return 1;
        }
    }
}

// FUNCTION: LEGORR 0x00414c90
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) int menu_list_find_name(const void* list, const char* name)
{
    __asm {
        push ebx
        push esi
        push edi
        mov edi, dword ptr [esp + 0x10]
        xor esi, esi
        mov eax, dword ptr [edi + 4]
        test eax, eax
        jle menu_list_find_name_no_entries
        mov ebx, dword ptr [esp + 0x14]
    menu_list_find_name_loop:
        mov eax, dword ptr [edi]
        mov eax, dword ptr [eax + esi*4]
        test eax, eax
        je menu_list_find_name_next
        mov ecx, dword ptr [eax]
        push ebx
        push ecx
        call strcasecmp_lite
        add esp, 8
        test eax, eax
        je menu_list_find_name_found
    menu_list_find_name_next:
        mov eax, dword ptr [edi + 4]
        inc esi
        cmp esi, eax
        jl menu_list_find_name_loop
        or eax, 0xffffffff
        pop edi
        pop esi
        pop ebx
        ret
    menu_list_find_name_found:
        mov eax, esi
        pop edi
        pop esi
        pop ebx
        ret
    menu_list_find_name_no_entries:
        pop edi
        pop esi
        or eax, 0xffffffff
        pop ebx
        ret
    }
}
#else
int menu_list_find_name(const void* list, const char* name)
{
    if (list == NULL) return -1;
    void* const* entries = *(void* const* const*)list;
    const int count = *(const int*)((const char*)list + sizeof(void*));
    for (int i = 0; i < count; ++i) {
        void* entry = entries[i];
        if (entry != NULL
            && strcasecmp_lite(*(const char**)entry, name) == 0) {
            return i;
        }
    }
    return -1;
}
#endif

/* --- the encyclopedia config-file window (2026-10-25) --------------------
 * EncyclopediaConfigFilesOpen (FUN_0040e490) — see frontend.hpp. The group
 * pairs (count window, name table) ride the corpus's address pairing:
 * 0x27c/0x1e8, 0x280/0x1ec, 0x284/0x1f0, 0x288/0x1f4. */
static void* s_ency_files[4];   /* DAT_004c7e88/ec/90/94 roles */
static void* s_ency_crystal;    /* DAT_004c7e98 role */
static void* s_ency_ore;        /* DAT_004c7e9c role */

/* The corpus joins `<root>::Encyclopedia::<name>` through the path builder
 * (FUN_00478f50), reads the record's +8 value token (FUN_004790b0's
 * record+8), and dispatches the file open (FUN_0047f6e0 in "r" mode — the
 * port's AUTO open). NULL answers ride where the corpus strides. */
static void encyclopedia_value_open(BlockFile* cfg, const char* root_path,
                                    const char* name, void** slot)
{
    const char* path = string_path_join(root_path, "Encyclopedia", name,
                                        (char*)NULL);
    const char* value = block_get_strptr(cfg, path);
    if (value != NULL) {
        *slot = resource_open(value, RESOURCE_AUTO);
    }
}

// FUNCTION: LEGORR 0x0040e490
void encyclopedia_config_files_open(BlockFile* cfg, const char* root_path)
{
    static const int count_slot[4] = { 0, 2, 3, 1 }; /* 0x27c/80/84/88 */
    for (int kind = 0; kind != 4; ++kind) {
        const uint32_t count = catalog_count_peek(count_slot[kind]);
        /* The corpus strides count 4-byte handle slots (its windows are
         * undefined4 arrays sized count * 4); on the pointer-sized port the
         * window is count void* slots, so the allocation and zeroing stride
         * sizeof(void*) to cover every handle (deviation: slot width). */
        void* array = engine_heap_alloc((long)count * (long)sizeof(void*));
        s_ency_files[kind] = array;
        if (array == NULL) {
            continue; /* deviation: the corpus strides the store blindly */
        }
        memset(array, 0, (size_t)count * sizeof(void*));
        const char* const* table =
            (const char* const*)catalog_name_table_peek(kind);
        for (uint32_t i = 0; i != count; ++i) {
            const char* name = (table != NULL) ? table[i] : NULL;
            encyclopedia_value_open(cfg, root_path, name,
                                    &((void**)array)[i]);
        }
    }
    encyclopedia_value_open(cfg, root_path, "PowerCrystal", &s_ency_crystal);
    encyclopedia_value_open(cfg, root_path, "Ore", &s_ency_ore);
}

void* encyclopedia_file_window_peek(int group)
{
    return (group >= 0 && group < 4) ? s_ency_files[group] : NULL;
}

void* encyclopedia_crystal_file_peek(void)
{
    return s_ency_crystal;
}

void* encyclopedia_ore_file_peek(void)
{
    return s_ency_ore;
}
