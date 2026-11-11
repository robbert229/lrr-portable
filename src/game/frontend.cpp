// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — front end / main menu (finding 23). See frontend.hpp.
 *
 * Reading the menu groups follows the shipped chip query shape exactly:
 * "<root>::Menu::<name>::<Menu%i>::<key>" with the `::` path separator
 * (DAT_004aab18 — Config.c splits paths on "::" only). The level
 * pickers graft the StartLevel / TutorialStartLevel + NextLevel chains
 * (LevelChainBuild @ 0x4154b0 semantics) onto the picker sub-menus; each
 * row carries the level's FrontEndX/Y scatter position (the config's
 * per-level cavern coords) so the picker looks like a map.
 *
 * Main menu selection follows the pointer. Main and pause rows activate on
 * clicks, not Enter/Space. Save menu keyboard behavior remains separate.
 * Slider and cycle controls write attached options.
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include "frontend.hpp"
#include "frontend.h"
#include "objective.hpp"
#include "level.hpp"
#include "entity.hpp"
#include "terrain_feedback.hpp"
#include "port_loop_ports.hpp"
#include "effects.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/engine.hpp"
#include "gods98/simtick.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "crt/crt.hpp"

namespace frontend_config {
void panels_config_entry_apply(BlockFile* config, int display_width,
                               int display_height);
void panel_button_layout_config_load(BlockFile* config, int display_width,
                                     int display_height);
void text_messages_config_load(BlockFile* config);
}
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/block.hpp"
#include "gods98/input.hpp"
#include "crt/crt.hpp"
#include "util.hpp"
#include "select.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "simulation_ui_ports.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
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

static void advisor_text_display_production_install(void);
static void frontend_status_flash_tick(float delta);
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
    "Interface_MenuItem_RemovePath",
    "Interface_MenuItem_RepairLava",
    "Interface_MenuItem_GeologistTest",
    "Interface_MenuItem_ClearRubble",
    "Interface_MenuItem_Dam",
    "Interface_MenuItem_Dig",
    "Interface_MenuItem_Reinforce",
    "Interface_MenuItem_Dynamite",
    "Interface_MenuItem_PlaceFence",
    "Interface_MenuItem_DeselectDig",
    "Interface_MenuItem_CancelConstruction",
    "Interface_MenuItem_SelectMan",
    "Interface_MenuItem_SelectVehicle",
    "Interface_MenuItem_SelectBuilding",
    "Interface_MenuItem_VehiclePickUp",
    "Interface_MenuItem_MinifigurePickUp",
    "Interface_MenuItem_UnLoadVehicle",
    "Interface_MenuItem_UnLoadMinifigure",
    "Interface_MenuItem_GetIn",
    "Interface_MenuItem_GetOut",
    "Interface_MenuItem_GotoDock",
    "Interface_MenuItem_LegoManGoto",
    "Interface_MenuItem_VehicleGoto",
    "Interface_MenuItem_VehicleDig",
    "Interface_MenuItem_LegoManDig",
    "Interface_MenuItem_GoFeed",
    "Interface_MenuItem_DeleteMan",
    "Interface_MenuItem_DropBirdScarer",
    "Interface_MenuItem_PowerOn",
    "Interface_MenuItem_PowerOff",
    "Interface_MenuItem_Repair",
    "Interface_MenuItem_MakeTeleporterPrimary",
    "Interface_MenuItem_EjectCrystal",
    "Interface_MenuItem_EjectOre",
    "Interface_MenuItem_DeleteBuilding",
    "Interface_MenuItem_DeleteVehicle",
    "Interface_MenuItem_DeleteElectricFence",
    "Interface_MenuItem_Attack",
    "Interface_MenuItem_GotoTopView",
    "Interface_MenuItem_GotoFirstPerson",
    "Interface_MenuItem_GotoSecondPerson",
    "Interface_MenuItem_TrackObject",
    "Interface_MenuItem_GetTool",
    "Interface_MenuItem_GetDrill",
    "Interface_MenuItem_GetSpade",
    "Interface_MenuItem_GetHammer",
    "Interface_MenuItem_GetSpanner",
    "Interface_MenuItem_GetLaser",
    "Interface_MenuItem_GetPusherGun",
    "Interface_MenuItem_GetFreezerGun",
    "Interface_MenuItem_GetBirdScarer",
    "Interface_MenuItem_TrainSkill",
    "Interface_MenuItem_TrainDriver",
    "Interface_MenuItem_TrainEngineer",
    "Interface_MenuItem_TrainGeologist",
    "Interface_MenuItem_TrainPilot",
    "Interface_MenuItem_TrainSailor",
    "Interface_MenuItem_TrainDynamite",
    "Interface_MenuItem_UpgradeMan",
    "Interface_MenuItem_UpgradeBuilding",
    "Interface_MenuItem_UpgradeVehicle",
    "Interface_MenuItem_UpgradeEngine",
    "Interface_MenuItem_UpgardeDrill",
    "Interface_MenuItem_UpgardeScan",
    "Interface_MenuItem_UpgardeCarry",
    "Interface_MenuItem_Encyclopedia",
    "Interface_MenuItem_ClearSelection",
    "Interface_MenuItem_Build",
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
// GLOBAL: LEGORR 0x005574dc
static float s_menu_plane_pan[2];
static int s_main_menu_text_item_pending;
/* The original reward-save entry point has no explicit front-end argument.
 * Keep its active menu owner in the module so the mapped entry retains that
 * ABI while the portable frame loop can still use its instance-taking
 * adapter below. */
static FrontEnd* s_reward_save_frontend;
// GLOBAL: LEGORR 0x004dcd40
static TextureSurface* s_interface_message_tab_info_text;
// GLOBAL: LEGORR 0x004dcec4
static int s_back_button_width;
static void fe_picker_order(FrontEnd* fe, int tree_index);
static int fe_sav_menu_list_from_config(FrontEnd* fe, int list_id,
                                        int tree_index,
                                        const char* start_level);
// GLOBAL: LEGORR 0x004dcec8
static int s_back_button_height;
// GLOBAL: LEGORR 0x004dce64
static TextureSurface* s_back_button_highlight;
// GLOBAL: LEGORR 0x004dce68
static TextureSurface* s_back_button_pressed;
static char s_back_button_action[FE_NAME_MAX];
static FeSurroundImage s_interface_surround_images[FE_SURROUND_IMAGES_MAX];
static FeInterfaceImage s_interface_images[FE_INTERFACE_ACTION_COUNT];
static FeInterfaceBuildImage* s_interface_build_images[2];
static uint32_t s_interface_build_image_counts[2];
#include "gods98/resource.hpp"
#include "entity.hpp"
#include "level.hpp"

static int s_toupper(int c)
{
    return (c >= 'a' && c <= 'z') ? toupper((unsigned char)c) : c;
}
static int s_strcaseeq(const char* a, const char* b);

// GLOBAL: LEGORR 0x004db748
static char s_ui_text_underscore_buffer[256];

/* The mapped target assumes a valid mutable input pointer, returns it
 * unchanged at 256 bytes or more, and otherwise copies to shared storage. */
// FUNCTION: LEGORR 0x004102b0
char* ui_text_underscores_to_spaces_target(char* text)
{
    char* const display = s_ui_text_underscore_buffer;
    const size_t length = strlen(text);
    if (length >= 0x100u) {
        return text;
    }
    strcpy(display, text);
    /* The original scans the full 256-byte scratch region after copying;
     * bytes after the terminator are not visible as part of the returned
     * C string, but retaining the full scan also matches its loop shape. */
    for (size_t i = 0; i < 0x100u; ++i) {
        if (display[i] == '_') {
            display[i] = ' ';
        }
    }
    return display;
}

/* Portable callers keep the null-safe interface; the retail entry point
 * itself dereferences its argument without checking it. */
const char* ui_text_underscores_to_spaces(const char* text)
{
    return text != NULL
        ? ui_text_underscores_to_spaces_target((char*)text) : NULL;
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
    size_t copy_length = strlen(display);
    if (copy_length >= out_size) copy_length = out_size - 1;
    if (display != out) strncpy(out, display, copy_length);
    out[copy_length] = '\0';
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
        call _strcmpi
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
    return strcasecmp_lite(a, b) == 0;
}

/* Case-insensitive substring search ("LevelPick" in "Interface\LevelPicker\…"). */
static int s_stristr(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    if (haystack[0] == '\0') {
        return 0;
    }
    if (needle[0] == '\0') {
        return 1;
    }
    return case_insensitive_substring_find(haystack, needle) != NULL;
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
    snprintf(path, sizeof(path), "%s::Levels::%s::%s", root, level, key);
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        snprintf(path, sizeof(path), "%s::%s::%s", root, level, key);
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
    /* FrontEndX/FrontEndY are plain config integers, like the other menu
     * fields parsed by the reference's VC5 _atoi wrapper. */
    return atoi(v);
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
    snprintf(path, sizeof(path), "%s::Main::%s", root, start_key);
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
    it->state_gated = 0;
    it->min = 0; it->max = 0; it->value = 0; it->cycle_count = 0;
    it->slider_x = 0; it->slider_y = 0;
    it->slider_config = NULL;
    memset(it->slider_images, 0, sizeof(it->slider_images));
    it->label[0] = '\0'; it->target[0] = '\0';
    it->image_normal[0] = it->image_selected[0] = it->image_disabled[0] = '\0';
    if (value == NULL) {
        return;
    }
    /* A Slider has eight resource paths after its seven numeric/text
     * fields. Preserve the complete retail tuple (16 fields total). */
    char fields[16][FE_PATH_MAX];
    int n = 0;
    const char* p = value;
    while (n < 16) {
        p += strspn(p, ":");
        if (*p == '\0') {
            break;
        }
        const char* s = p;
        size_t length = strcspn(p, ":");
        p += length;
        size_t copy_length = length;
        if (copy_length >= FE_PATH_MAX) {
            copy_length = FE_PATH_MAX - 1;
        }
        memcpy(fields[n], s, copy_length);
        fields[n][copy_length] = '\0';
        ++n;
    }
    if (n == 0) {
        return;
    }
    it->kind = fe_item_kind(fields[0]);
    int atoi1 = (n > 1) ? atoi(fields[1]) : 0;
    int atoi2 = (n > 2) ? atoi(fields[2]) : 0;
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
            it->state_gated = (n == 6);
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
                it->flag = atoi(fields[7]);
            }
        } else {
            if (n > 3) {
                snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
            }
            if (n > 4) {
                it->flag = atoi(fields[4]);
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
            /* Slider:x1:y1:x2:y2:banner:lo:hi:off:on:leftcap:rightcap:
             * plus:minus:plus-highlight:minus-highlight. Resource paths are
             * still owned by the BlockFile; runtime handles are cached here. */
            it->slider_x = (n > 3) ? atoi(fields[3]) : 0;
            it->slider_y = (n > 4) ? atoi(fields[4]) : 0;
            if (n > 8) {
                it->slider_config = value;
            }
            it->min = (n > 6) ? atoi(fields[6]) : 0;
            it->max = (n > 7) ? atoi(fields[7]) : 100;
            it->flag = it->min;
            it->value = fe_option_default(it->label, it->min, it->max);
        } else if (it->kind == FE_ITEM_REALSLIDER) {
            /* RealSlider:type:x1:y1:x2:y2:banner:lo:hi:step */
            it->min = (n > 6) ? atoi(fields[6]) : 0;
            it->max = (n > 7) ? atoi(fields[7]) : 100;
            it->value = fe_option_default(it->label, it->min, it->max);
        } else if (it->kind == FE_ITEM_CYCLE) {
            /* Cycle:x1:y1:x2:y2:banner:count:opt1:opt2:… */
            it->cycle_x = (n > 3) ? atoi(fields[3]) : 0;
            it->cycle_y = (n > 4) ? atoi(fields[4]) : 0;
            int count = (n > 6) ? atoi(fields[6]) : 0;
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
        int id = options_id_for_label(it->label);
        options_set_label(fe->opts, it->label, it->value);
        /* These callbacks belong to actual user adjustments, not widget sync
         * or save loading. Ordinal zero is High/On in the shipped menu. */
        if (id == OPT_WALL_DETAIL) {
            uint32_t* state = engine_shared_state_word();
            if (it->value == 0) *state |= 0x20u;
            else if (it->value == 1) *state &= ~0x20u;
        } else if (id == OPT_MUSIC_ONOFF) {
            media_cd_watchdog_update(it->value == 0);
        } else if (id == OPT_SFX_ONOFF) {
            media_audio_playback_toggle(it->value == 0);
        } else if (id == OPT_AUTO_SPEED) {
            selection_flag10_set_clear(it->value == 0);
        }
    }
}

/* --- menu group building -------------------------------------------------- */

/* MenuImageLoad (0x410fb0): normal and dark images share the optional
 * floating-point origin. Only a four-field tuple ending in 1 enables a key. */
static void fe_parse_menu_image(FeSubmenu* sub, const char* value, int dark)
{
    char fields[5][FE_PATH_MAX];
    const char* p = value;
    int count = 0;
    if (value == NULL) return;
    while (count < 5) {
        p += strspn(p, ":");
        if (*p == '\0') break;
        size_t length = strcspn(p, ":");
        size_t copied = length < FE_PATH_MAX ? length : FE_PATH_MAX - 1;
        memcpy(fields[count], p, copied);
        fields[count][copied] = '\0';
        ++count;
        p += length;
    }
    if (count == 0) return;
    snprintf(dark ? sub->image_dark : sub->image, FE_PATH_MAX, "%s", fields[0]);
    if (count >= 3) {
        sub->image_has_origin = 1;
        sub->image_x = (float)atof(fields[1]);
        sub->image_y = (float)atof(fields[2]);
    }
    int keyed = count == 4 && atoi(fields[3]) == 1;
    if (dark) sub->image_dark_keyed = keyed;
    else sub->image_keyed = keyed;
}

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
    sub->image_x = sub->image_y = 0.0f;
    sub->image_has_origin = sub->image_keyed = sub->image_dark_keyed = 0;
    sub->lofont[0] = sub->hifont[0] = '\0';
    memset(sub->items, 0, sizeof(sub->items));
    memset(sub->overlays, 0, sizeof(sub->overlays));
    sub->overlay_count = 0;
    char pre[FE_PATH_MAX * 2];
    snprintf(pre, sizeof(pre), "%s::Menu::%s::%s", util_root_name(fe->cfg),
             name, fe_menu_name_idx(idx));
    char buf[FE_PATH_MAX * 2];

    snprintf(buf, sizeof(buf), "%s::Title", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->title, FE_NAME_MAX, "%s", block_get_strptr(fe->cfg, buf));
    }
    snprintf(buf, sizeof(buf), "%s::FullName", pre);
    const char* v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        snprintf(sub->full_name, sizeof(sub->full_name), "%s", v);
        char* underscore = sub->full_name;
        while ((underscore = crt_strchr(underscore, '_')) != NULL) {
            *underscore++ = ' ';
        }
    }

    /* Position x:y */
    snprintf(buf, sizeof(buf), "%s::Position", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        const char* c = strchr(v, ':');
        if (c != NULL) {
            sub->base_x = atoi(v);
            sub->base_y = atoi(c + 1);
        }
    }

    snprintf(buf, sizeof(buf), "%s::AutoCenter", pre);
    sub->auto_center = block_get_bool_path(fe->cfg, buf) == 1;
    snprintf(buf, sizeof(buf), "%s::DisplayTitle", pre);
    sub->display_title = block_get_bool_path(fe->cfg, buf) != 0;
    /* Anchored is stored as x:y; title placement currently uses its y value. */
    snprintf(buf, sizeof(buf), "%s::Anchored", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        const char* c = strchr(v, ':');
        if (c != NULL) {
            char xbuf[16];
            size_t xlen = (size_t)(c - v);
            if (xlen >= sizeof(xbuf)) xlen = sizeof(xbuf) - 1;
            memcpy(xbuf, v, xlen);
            xbuf[xlen] = '\0';
            sub->title_anchor_x = atoi(xbuf);
            sub->title_anchor_y = atoi(c + 1);
        }
    }
    snprintf(buf, sizeof(buf), "%s::CanScroll", pre);
    sub->can_scroll = block_get_bool_path(fe->cfg, buf) == 1;
    snprintf(buf, sizeof(buf), "%s::MenuImage", pre);
    v = block_get_strptr(fe->cfg, buf);
    fe_parse_menu_image(sub, v, 0);
    snprintf(buf, sizeof(buf), "%s::MenuImageDark", pre);
    v = block_get_strptr(fe->cfg, buf);
    fe_parse_menu_image(sub, v, 1);

    /* Bitmap fonts: LoFont = unselected, HiFont = selected (MenuFont is
     * the measurement face — fall back to it for both when the pair is
     * absent). */
    snprintf(buf, sizeof(buf), "%s::LoFont", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->lofont, FE_PATH_MAX, "%s",
                 block_get_strptr(fe->cfg, buf));
    }
    snprintf(buf, sizeof(buf), "%s::HiFont", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->hifont, FE_PATH_MAX, "%s",
                 block_get_strptr(fe->cfg, buf));
    }
    if (sub->lofont[0] == '\0' || sub->hifont[0] == '\0') {
        snprintf(buf, sizeof(buf), "%s::MenuFont", pre);
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

    snprintf(buf, sizeof(buf), "%s::ItemCount", pre);
    const char* item_count_value = block_get_strptr(fe->cfg, buf);
    int ic = atoi(item_count_value != NULL ? item_count_value : "");
    if (ic > FE_ITEMS_MAX) {
        ic = FE_ITEMS_MAX;
    }
    sub->item_count = ic;
    for (int item_index = 0; item_index < ic; item_index++) {
        snprintf(buf, sizeof(buf), "%s::Item%d", pre, item_index + 1);
        const char* item = block_get_strptr(fe->cfg, buf);
        if (item == NULL) {
            continue;
        }
        fe_parse_item(item, &sub->items[item_index]);
    }

    /* FLIC overlays (`!Overlay%i <avi\X.flh>:<SFX>:x:y`) + PlayRandom —
     * finding 23's decoration lane; the `.flh` players open lazily at
     * display time (the paths may be loose Data/AVI copies). */
    snprintf(buf, sizeof(buf), "%s::PlayRandom", pre);
    sub->play_random = block_get_bool_path(fe->cfg, buf) == 1;
    sub->overlay_count = 0;
    for (int overlay_index =  1;
         overlay_index <= FE_OVERLAYS_MAX &&
             sub->overlay_count < FE_OVERLAYS_MAX;
         ++overlay_index) {
        snprintf(buf, sizeof(buf), "%s::!Overlay%d", pre, overlay_index);
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
                o->x = atoi(c2 + 1);
                const char* c3 = strchr(c2 + 1, ':');
                if (c3 != NULL && c3[1] != '\0') {
                    o->y = atoi(c3 + 1);
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
    snprintf(buf, sizeof(buf), "%s::Menu::%s::MenuCount", util_root_name(fe->cfg),
             name);
    const char* menu_count_value = block_get_strptr(fe->cfg, buf);
    int count = atoi(menu_count_value != NULL ? menu_count_value : "");
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
            it->level_enabled = 1;
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
            }
        }
    }
    fe->overlays_for = NULL;
    fe->overlay_active = -1;
}

const FeSubmenu* frontend_active_submenu(const FrontEnd* fe);

/* VC5 rand.obj owns the shared target stream in the classic x86 build. Other
 * builds keep the frontend's isolated LCG model. */
static uint32_t fe_overlay_random(FrontEnd* fe)
{
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
    (void)fe;
    return (uint32_t)rand();
#else
    fe->overlay_rng = fe->overlay_rng * 0x343fdu + 0x269ec3u;
    return (fe->overlay_rng >> 16) & 0x7fffu;
#endif
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
    snprintf(key, sizeof(key), "%s::Samples::!%s", util_root_name(fe->cfg),
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

/* MenuOverlayFlicPlaybackController (FUN_004121b0): the retail path has one
 * active FLIC player. PlayRandom starts one configured entry on the rand()%400
 * gate; non-random hover FLIC selection needs the renderer's type-6 menu rows. */
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
    }
    if (sub->play_random && fe->overlay_active < 0) {
        fe_overlay_start_random(fe, sub);
        return;
    }
    if (fe->overlay_active >= 0 &&
        fe->overlay_active < sub->overlay_count) {
        FeOverlay* o = (FeOverlay*)&sub->overlays[fe->overlay_active];
        if (o->flic == NULL) {
            fe->overlay_active = -1;
            return;
        }
        flic_tick_loop(o->flic, 0);
        if (flic_eof(o->flic)) {
            flic_close(o->flic);
            o->flic = NULL;
            fe->overlay_active = -1;
        }
    }
}

void frontend_init(FrontEnd* fe)
{
    if (fe == NULL) {
        return;
    }
    s_reward_save_frontend = fe;
    ui_panel_name_tables_init();
    s_main_menu_text_item_pending = 0;
    memset(fe, 0, sizeof(*fe));
    fe->selected_save_slot = -1;
    fe->origin_picker_sub = -1;
    fe->pointer_x = fe->pointer_y = -1;
    fe->overlay_active = -1;
    advisor_text_display_production_install();
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
    fe->overlay_rng = (uint32_t)time(NULL) ^ (uint32_t)(uintptr_t)fe;
    if (fe->overlay_rng == 0) {
        fe->overlay_rng = 1;
    }
#endif
}

static int fe_parse_int_tuple(const char* text, int* out, int count)
{
    if (text == NULL || out == NULL || count <= 0) {
        return 0;
    }
    const char* p = text;
    for (int i = 0; i < count; ++i) {
        const char* separator = strchr(p, ':');
        if (p[0] == '\0' || separator == p
            || (i + 1 < count && separator == NULL)) {
            return 0;
        }
        out[i] = atoi(p);
        if (i + 1 < count) {
            p = separator + 1;
        }
    }
    return 1;
}

/* BlockTextTokenSplit-backed config records used by SaveMenuLayoutLoad
 * separate fields with the vertical bar DAT string. Preserve empty fields:
 * the original integer parser turns an empty numeric token into zero. */
static int fe_parse_int_tuple_bar(const char* text, int* out, int count)
{
    const char* p;
    int i;
    if (text == NULL || out == NULL || count <= 0) {
        return 0;
    }
    p = text;
    for (i = 0; i < count; ++i) {
        const char* separator = strchr(p, '|');
        if (i + 1 < count && separator == NULL) {
            return 0;
        }
        if (i + 1 == count && separator != NULL) {
            return 0;
        }
        out[i] = atoi(p);
        if (separator != NULL) {
            p = separator + 1;
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
    snprintf(key, sizeof(key), "%s::InterfaceBackButton", util_root_name(cfg));
    const char* value = block_get_strptr(cfg, key);
    if (value == NULL) {
        return 0;
    }

    int width = 0, height = 0;
    const char* first_separator = strchr(value, ':');
    if (first_separator == NULL) {
        return 0;
    }
    width = atoi(value);
    if (width <= 0 || width > 32767) {
        return 0;
    }
    const char* p = first_separator + 1;
    const char* second_separator = strchr(p, ':');
    if (second_separator == NULL) {
        return 0;
    }
    height = atoi(p);
    if (height <= 0 || height > 32767) {
        return 0;
    }
    p = second_separator + 1;

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
    /* InterfaceBackButtonConfigLoad calls BitmapSurfaceLoad for both
     * resources. Keep these as independent loads instead of sharing the
     * name-keyed texture cache used by ordinary menu assets. */
    s_back_button_highlight = image_surface_load(highlight_path, 0u, 0u);
    s_back_button_pressed = image_surface_load(pressed_path, 0u, 0u);
    strncpy(s_back_button_action, action, action_len + 1);
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
    if (strlen(expected) != length) {
        return 0;
    }
    return strnicmp_lite(token, expected, (int)length) == 0;
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
    BlockRec* section;
    BlockRec* entry;
    if (cfg == NULL) {
        return 0;
    }
    keyboard_key_names_init();
    char section_path[FE_PATH_MAX * 2];
    snprintf(section_path, sizeof(section_path), "%s::InterfaceImages",
             util_root_name(cfg));
    section = block_subgroup(cfg, section_path);
    entry = section;
    if (entry == NULL) {
        /* These two loaders run after the generic loop, including when
         * the section is absent (FUN_0041ada0 + 0x209). */
        (void)interface_build_images_config_load(cfg);
        (void)interface_back_button_config_load(cfg);
        return 0;
    }
    int loaded = 0;
    for (; entry != NULL; entry = block_next_sibling(entry)) {
        const char* label = block_name(entry);
        const char* value = block_value(entry);
        int image_index;
        if (label == NULL || value == NULL ||
            !interface_menu_action_label_find(label, &image_index)) {
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

        FeInterfaceImage* image = &s_interface_images[image_index];
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
    (void)interface_build_images_config_load(cfg);
    (void)interface_back_button_config_load(cfg);
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
        snprintf(type_path, sizeof(type_path), "%s::%s", util_root_name(cfg),
                 type_sections[group]);
        BlockRec* type_section = block_subgroup(cfg, type_path);
        uint32_t count = 0;
        for (BlockRec* row = type_section; row != NULL;
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
        BlockRec* type_row = type_section;
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
            snprintf(key, sizeof(key), "%s::InterfaceBuildImages::%s",
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
                /* FUN_0041ab00 uses BitmapSurfaceLoad for each state. These
                 * catalog records own independent surface instances. */
                *states[state] = image_surface_load(path, 0u, 0u);
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

/* Typed counterpart of IndexedEntityValueGet (0x0041cab0). The legacy
 * DWORD windows cannot carry TextureSurface pointers on a 64-bit host. */
TextureSurface* interface_action_image_value_get(
    const EngineContext* engine, int action)
{
    const FeInterfaceImage* image = interface_image_peek(action);
    if (image == NULL) return NULL;
    if (input_action_pressed_get(engine, action, 0) != 0)
        return image->disabled;
    if (input_entry_flag8_get(action) != 0u)
        return image->pressed;
    return image->normal;
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
        snprintf(key, sizeof(key), "%s::InterfaceSurroundImages::%d", root,
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
            numbers[n] = atoi(token);
        }
        if (!valid) {
            continue;
        }

        int first_is_null = 0;
        int second_is_null = 0;
        if (lengths[0] == 4) {
            char token[5];
            memcpy(token, fields[0], 4);
            token[4] = '\0';
            first_is_null = strcasecmp_lite(token, "NULL") == 0;
        }
        if (lengths[5] == 4) {
            char token[5];
            memcpy(token, fields[5], 4);
            token[4] = '\0';
            second_is_null = strcasecmp_lite(token, "NULL") == 0;
        }
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
 * data in bounded, typed fields. LoadMenuBuild zeroes the record before these
 * calls; this helper only overwrites fields whose config entries are present. */
// FUNCTION: LEGORR 0x00415d00
int save_menu_layout_load(FrontEnd* fe, const char* group,
                          FeSaveMenuLayout* layout)
{
    if (fe == NULL || fe->cfg == NULL || group == NULL || layout == NULL) {
        return 0;
    }
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
        snprintf(path, sizeof(path), "%s::%s::%s", util_root_name(fe->cfg),
                 group, keys[i]);
        const char* value = block_get_strptr(fe->cfg, path);
        if (value != NULL) {
            snprintf(fields[i], FE_SAVE_LAYOUT_TEXT_MAX, "%s", value);
            found = 1;
        }
    }

    snprintf(path, sizeof(path), "%s::%s::Window", util_root_name(fe->cfg),
             group);
    int window[4];
    if (fe_parse_int_tuple_bar(block_get_strptr(fe->cfg, path), window, 4)) {
        layout->window_x = window[0];
        layout->window_y = window[1];
        layout->window_w = window[2];
        layout->window_h = window[3];
        layout->has_window = 1;
        found = 1;
    }

    snprintf(path, sizeof(path), "%s::%s::Panel", util_root_name(fe->cfg),
             group);
    const char* panel = block_get_strptr(fe->cfg, path);
    if (panel != NULL) {
        const char* separator = strchr(panel, '|');
        const char* end = panel;
        int rect[4];
        int i;
        if (separator != NULL && separator != panel) {
            size_t name_len = (size_t)(separator - panel);
            if (name_len < sizeof(layout->panel_path)) {
                for (i = 0; i < 4; ++i) {
                    end = strchr(end, '|');
                    if (end == NULL) {
                        break;
                    }
                    ++end;
                }
                if (i == 4 && end != NULL && strchr(end, '|') == NULL
                    && fe_parse_int_tuple_bar(separator + 1, rect, 4)) {
                    memcpy(layout->panel_path, panel, name_len);
                    layout->panel_path[name_len] = '\0';
                    /* The reference loads the configured bitmap directly,
                     * then applies a black source color key before storing
                     * its four panel coordinates. */
                    /* The frame renderer's SaveLoad panel path uses the
                     * shared texture cache; retain that same cached surface
                     * here so layout loading and drawing share one resource. */
                    layout->panel_surface = texture_get_or_load(
                        NULL, layout->panel_path);
                    if (layout->panel_surface != NULL) {
                        texture_source_color_key_set(layout->panel_surface,
                                                     0u, 0u);
                        layout->panel_x = rect[0];
                        layout->panel_y = rect[1];
                        layout->panel_w = rect[2];
                        layout->panel_h = rect[3];
                        layout->has_panel = 1;
                    }
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
    /* SaveMenuBuild owns the two campaign/tutorial graphs for as long as
     * save reads and refreshes can consult their node counts. */
    sav_tree_roots_clear();
    frontend_init(fe);
    fe->cfg = cfg;
    if (cfg == NULL) {
        return 0;
    }

    {
        char key[256];
        uint32_t* state = engine_shared_state_word();
        snprintf(key, sizeof(key), "%s::Main::Panels", util_root_name(cfg));
        *state |= ENGINE_STATE_INDEXED_SUBSYS;
        if (block_get_bool_path(cfg, key) == 0)
            *state &= ~ENGINE_STATE_INDEXED_SUBSYS;
    }
    frontend_panels_release();
    advisor_message_ui_initialize(cfg, display_width_get(), display_height_get());

    /* GameConfigInit applies message and display-sized panel data after the
     * panel-name tables have been initialized. The portable startup reaches
     * the same work through FrontEndBuild. */
    frontend_config::text_messages_config_load(cfg);
    frontend_config::panels_config_entry_apply(
        cfg, display_width_get(), display_height_get());
    frontend_config::panel_button_layout_config_load(
        cfg, display_width_get(), display_height_get());
    frontend_pointer_config_load(cfg);

    /* GameConfigInit loads the action and surround image tables alongside
     * the menu definitions; keep those menu-facing resources on the same
     * Lego.cfg startup path in the port. */
    frontend_panel_rotation_control_config_load(cfg, util_root_name(cfg));
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
            fe, 4, 0, fe->campaign_n > 0 ? fe->campaign[0] : NULL);
        (void)fe_sav_menu_list_from_config(
            fe, 5, 1, fe->tutorial_n > 0 ? fe->tutorial[0] : NULL);
    }
    if (fe->built) {
        fe_picker_order(fe, 0);
        fe_picker_order(fe, 1);
        (void)frontend_apply_campaign_progress(fe, NULL, 0);
    }
    /* Options widgets seed from the store when one is already attached. */
    frontend_options_sync(fe);
    return main_ok;
}

/* Separate registries: campaign records use the campaign NextLevel order.
 * Training has its own index space in the reference (0x416641/0x41665a). */
int fe_record_for_level(const FrontEnd* fe, const char* level)
{
    int i;
    if (fe == NULL || !fe->built || level == NULL) return -1;
    for (i = 0; i < fe->campaign_n; ++i)
        if (s_strcaseeq(level, fe->campaign[i])) return i;
    for (i = 0; i < fe->tutorial_n; ++i)
        if (s_strcaseeq(level, fe->tutorial[i])) return i;
    return -1;
}

int fe_level_for_record(const FrontEnd* fe, int rec, char* out, size_t sz)
{
    if (fe == NULL || !fe->built || out == NULL || rec < 0 ||
        rec >= fe->campaign_n) return 0;
    snprintf(out, sz, "%s", fe->campaign[rec]);
    return 1;
}

// FUNCTION: LEGORR 0x00416d00
int frontend_enabled(BlockFile* cfg)
{    if (cfg == NULL) {
        return 0;
    }
    /* FrontEndActive @ 0x416d00: the cfg flag (loader-mode <3 is Win32). */
    return block_get_bool_path(cfg, "Lego*::Main::FrontEnd") == 1;
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
    /* SaveMenuBuild supplies list offsets relative to the submenu anchor. */
    item->x = submenu->base_x - 200;
    item->y = submenu->base_y - 100 + row * 60;
    ++submenu->item_count;
    return 1;
}

/* Partial SaveMenuBuild (FUN_00415a10): populate save/load menus with
 * runtime slot rows; the corpus also resolves config keys and slot summaries. */
// FUNCTION: LEGORR 0x00415a10
int frontend_slot_raw_index(int ordinal)
{
    return ordinal >= 1 && ordinal < SAVE_SLOTS ? ordinal - 1 : -1;
}

int frontend_build_slots(FrontEnd* fe, const FeSlot* slots, int n)
{
    if (fe == NULL || slots == NULL || n <= 0) {
        return 0;
    }
    if (n > FE_SLOT_COUNT) {
        n = FE_SLOT_COUNT;
    }
    fe->slot_count = n;
    FeSlot configured_slots[FE_SLOT_COUNT];
    for (int i = 0; i < n; i++) {
        configured_slots[i] = slots[i];
        /* SaveMenuBuild reads Menu::Save_Game_<slot>, appends the numeric
         * slot to that configured stem, then replaces underscores with
         * spaces before adding the same label to the save and load lists.
         * Keep the caller's label as a fallback when this optional key is
         * absent. */
        if (fe->cfg != NULL && configured_slots[i].id > 0) {
            char key[FE_PATH_MAX * 2];
            snprintf(key, sizeof(key), "%s::Menu::Save_Game_%d",
                     util_root_name(fe->cfg), configured_slots[i].id);
            const char* stem = block_get_strptr(fe->cfg, key);
            if (stem != NULL && stem[0] != '\0') {
                char label[FE_NAME_MAX];
                snprintf(label, sizeof(label), "%s %d", stem,
                         configured_slots[i].id);
                for (char* p = label; *p != '\0'; ++p) {
                    if (*p == '_') {
                        *p = ' ';
                    }
                }
                snprintf(configured_slots[i].label,
                         sizeof(configured_slots[i].label), "%s", label);
            }
        }
        fe->slots[i] = configured_slots[i];
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
                (void)fe_save_menu_list_entry_append(
                    sub, &configured_slots[k], k);
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
    snprintf(path, sizeof(path), "%s::Levels::%s::FrontEndOpen",
             util_root_name(build->fe->cfg), group);
    int open = block_get_bool_path(build->fe->cfg, path);
    if (open == 2) {
        snprintf(path, sizeof(path), "%s::%s::FrontEndOpen",
                 util_root_name(build->fe->cfg), group);
        open = block_get_bool_path(build->fe->cfg, path);
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
                                        int tree_index,
                                        const char* start_level)
{
    enum { LEVEL_REGISTRY_MAX = 256 };
    if (fe == NULL || fe->cfg == NULL || start_level == NULL
        || start_level[0] == '\0') {
        return 0;
    }
    char names[FE_CHAIN_MAX][FE_LEVEL_NAME_MAX + 16];
    void* entries[FE_CHAIN_MAX];
    void* values_a[FE_CHAIN_MAX];
    void* values_b[FE_CHAIN_MAX];
    int count = tree_index == 0 ? fe->campaign_n : fe->tutorial_n;
    int i;
    memset(entries, 0, sizeof(entries));
    memset(values_a, 0, sizeof(values_a));
    memset(values_b, 0, sizeof(values_b));
    for (i = 0; i < count; ++i) {
        snprintf(names[i], sizeof(names[i]), "Levels::%s",
                 tree_index == 0 ? fe->campaign[i] : fe->tutorial[i]);
        entries[i] = names[i];
    }
    if (count == 0) return 0;

    SavListWindow registry;
    registry.count = count;
    registry.entries = entries;
    registry.values_a = values_a;
    registry.values_b = values_b;
    SavTreeNode* root = level_node_get(&registry, fe->cfg, names[0]);
    int rows = frontend_sav_menu_list(fe, list_id, root, &registry);

    /* Retail SaveMenuBuild retains both roots for SavReadSlot's campaign
     * record-count validation and SavRefreshSlots. Transfer graph ownership
     * to savegame; the row snapshot above no longer depends on the registry. */
    if (!sav_tree_root_install_owned(tree_index, root,
                                     (void* const*)values_a,
                                     (uint32_t)count)) {
        for (int i = 0; i < count; ++i) {
            SavTreeNode* node = (SavTreeNode*)values_a[i];
            if (node != NULL && (uintptr_t)node > 1u) {
                engine_heap_free(node->children);
                engine_heap_free(node);
            }
        }
    }
    return rows;
}

typedef struct FePickerOrder {
    const SavTreeNode* nodes[FE_CHAIN_MAX];
    int count;
} FePickerOrder;

static void fe_picker_order_walk(FePickerOrder* order, const SavTreeNode* node)
{
    int i;
    if (node == NULL) return;
    for (i = 0; i < order->count; ++i) if (order->nodes[i] == node) return;
    if (order->count >= FE_CHAIN_MAX) return;
    order->nodes[order->count++] = node;
    for (i = 0; i < (int)node->count; ++i)
        fe_picker_order_walk(order, ((SavTreeNode**)node->children)[i]);
}

/* SaveMenuList rows and FrontEndSelectedAction use first-visit DFS order. */
static void fe_picker_order(FrontEnd* fe, int tree_index)
{
    FePickerOrder order;
    FeItem ordered[FE_CHAIN_MAX];
    FeMenu* menu = &fe->menus[FE_MENU_MAIN];
    int i;
    memset(&order, 0, sizeof(order));
    fe_picker_order_walk(&order, (SavTreeNode*)sav_tree_root_peek(tree_index));
    for (i = 0; i < menu->submenu_count; ++i) {
        FeSubmenu* sub = &menu->submenus[i];
        int first = -1, n = 0, j, k;
        if (!s_stristr(sub->image, "LevelPick") ||
            (!!s_stristr(sub->image, "LevelPickT") != (tree_index != 0))) continue;
        for (j = 0; j < sub->item_count; ++j)
            if (sub->items[j].level_start) { first = j; break; }
        if (first < 0) continue;
        for (j = 0; j < order.count; ++j) {
            uint32_t index = order.nodes[j]->window;
            int max = tree_index ? fe->tutorial_n : fe->campaign_n;
            const char* name;
            if (index >= (uint32_t)max) continue;
            name = tree_index ? fe->tutorial[index] : fe->campaign[index];
            for (k = first; k < sub->item_count; ++k) {
                if (sub->items[k].level_start && s_strcaseeq(name, sub->items[k].target)) {
                    ordered[n++] = sub->items[k];
                    break;
                }
            }
        }
        for (j = 0; j < n; ++j) {
            const FeLevelMenuList* list = &fe->level_menu_lists[tree_index];
            FeItem* item = &ordered[j];
            int r;
            for (r = 0; r < list->row_count; ++r) {
                const FeLevelMenuRow* row = &list->rows[r];
                if (s_strcaseeq(row->level, item->target)) {
                    char tuple[sizeof(row->menu_bmp)];
                    char* part;
                    char* next;
                    char* dest[3] = { item->image_selected, item->image_normal,
                                      item->image_disabled };
                    int variant;
                    item->x = row->x; item->y = row->y;
                    snprintf(item->label, sizeof(item->label), "%s", row->full_name);
                    snprintf(tuple, sizeof(tuple), "%s", row->menu_bmp);
                    part = tuple;
                    for (variant = 0; variant < 3 && part != NULL; ++variant) {
                        next = strchr(part, ',');
                        if (next != NULL) *next++ = '\0';
                        snprintf(dest[variant], FE_PATH_MAX, "%s", part);
                        part = next;
                    }
                    break;
                }
            }
            sub->items[first + j] = *item;
        }
        sub->item_count = first + n;
    }
}

typedef struct FeProgressWalk {
    FrontEnd* fe;
    const uint8_t* records;
    int count;
    const SavTreeNode* seen[FE_CHAIN_MAX];
    int seen_count;
} FeProgressWalk;

/* Reference SavTreeWalk2 (0x4172f0): the first DFS visit owns the row;
 * children inherit this node's record bit, not accumulated ancestry. */
static void fe_progress_walk(FeProgressWalk* walk, const SavTreeNode* node,
                             int parent_complete)
{
    int i, complete = 0, authored_open = 0;
    char path[FE_PATH_MAX * 2];
    const char* level;
    FeMenu* menu;
    if (node == NULL || node->window >= (uint32_t)walk->fe->campaign_n) return;
    for (i = 0; i < walk->seen_count; ++i)
        if (walk->seen[i] == node) return;
    if (walk->seen_count >= FE_CHAIN_MAX) return;
    walk->seen[walk->seen_count++] = node;
    level = walk->fe->campaign[node->window];
    if (walk->records != NULL && node->window < (uint32_t)walk->count) {
        uint32_t flags;
        memcpy(&flags, walk->records + (size_t)node->window * SAVE_RECORD_SIZE,
               sizeof(flags));
        complete = (flags & 1u) != 0;
    }
    snprintf(path, sizeof(path), "%s::Levels::%s::FrontEndOpen",
             util_root_name(walk->fe->cfg), level);
    authored_open = block_get_bool_path(walk->fe->cfg, path) == 1;
    menu = &walk->fe->menus[FE_MENU_MAIN];
    for (i = 0; i < menu->submenu_count; ++i) {
        int j;
        FeSubmenu* sub = &menu->submenus[i];
        if (!s_stristr(sub->image, "LevelPick") ||
            s_stristr(sub->image, "LevelPickT")) continue;
        for (j = 0; j < sub->item_count; ++j) {
            FeItem* item = &sub->items[j];
            if (item->level_start && s_strcaseeq(item->target, level))
                item->level_enabled = complete || parent_complete || authored_open;
        }
    }
    for (i = 0; i < (int)node->count; ++i)
        fe_progress_walk(walk, ((SavTreeNode**)node->children)[i], complete);
}

int frontend_apply_campaign_progress(FrontEnd* fe, const uint8_t* records,
                                     int count)
{
    FeProgressWalk walk;
    int i, j;
    SavTreeNode* root = (SavTreeNode*)sav_tree_root_peek(0);
    if (fe == NULL || !fe->built || root == NULL || count < 0) return 0;
    for (i = 0; i < fe->menus[FE_MENU_MAIN].submenu_count; ++i) {
        FeSubmenu* sub = &fe->menus[FE_MENU_MAIN].submenus[i];
        if (!s_stristr(sub->image, "LevelPick") ||
            s_stristr(sub->image, "LevelPickT")) continue;
        for (j = 0; j < sub->item_count; ++j)
            if (sub->items[j].level_start) sub->items[j].level_enabled = 0;
    }
    memset(&walk, 0, sizeof(walk));
    walk.fe = fe; walk.records = records; walk.count = count;
    fe_progress_walk(&walk, root, 1);
    return walk.seen_count != 0;
}

/* The slot chooses a campaign context; gameplay starts only on a later
 * enabled level selection. Slot inspection owns its immutable record view. */
int frontend_open_saved_campaign(FrontEnd* fe, GameContext* game, int slot)
{
    int count = 0;
    const uint8_t* records = NULL;
    if (!save_slot_preview(game, slot, &records, &count) ||
        !frontend_apply_campaign_progress(fe, records, count)) return 0;
    /* A selected slot needs its context, not just the immutable preview. */
    if (!save_read_slot_target((uint32_t)slot, NULL, 1) ||
        !frontend_open_submenu(fe, FE_MENU_MAIN, 1)) return 0;
    fe->selected_save_slot = slot;
    return 1;
}

/* Prepare the portable modal state after MenuShow has reset run flags. */
static int frontend_open_prepare(FrontEnd* fe, int menu, int submenu_index)
{
    fe_overlay_close_all(fe);
    fe->menu = menu;
    fe->save_action = 0;
    fe->sub = submenu_index;
    fe->sel = 0;
    fe->open = 1;
    fe->quit = 0;
    fe->pending_level[0] = '\0';
    /* MenuBuild's Slider records retain eight loaded surface handles. Open
     * these with the menu and let InterfaceResourceHandlesRelease balance
     * them when the modal menu exits. */
    for (int si = 0; si < fe->menus[menu].submenu_count; ++si) {
        FeSubmenu* sub = &fe->menus[menu].submenus[si];
        for (int ii = 0; ii < sub->item_count; ++ii) {
            FeItem* item = &sub->items[ii];
            if (item->kind != FE_ITEM_SLIDER || item->slider_config == NULL)
                continue;
            const char* p = item->slider_config;
            int field = 0;
            while (field < 8 && *p != '\0') {
                const char* colon = strchr(p, ':');
                if (colon == NULL) { p += strlen(p); break; }
                p = colon + 1;
                ++field;
            }
            for (int ri = 0; ri < 8 && *p != '\0'; ++ri) {
                const char* end = strchr(p, ':');
                size_t len = end != NULL ? (size_t)(end - p) : strlen(p);
                char path[FE_PATH_MAX];
                if (len >= sizeof(path)) len = sizeof(path) - 1;
                memcpy(path, p, len);
                path[len] = '\0';
                if (item->slider_images[ri] == NULL && path[0] != '\0')
                    item->slider_images[ri] = texture_get_or_load(NULL, path);
                if (end == NULL) break;
                p = end + 1;
            }
        }
    }
    /* Issue #85: re-entry shows the effective option values, never the
     * build-time defaults — sync every widget from the store. */
    frontend_options_sync(fe);
    /* corpus DAT_005574e0 reset @ 0x413d29: the plane returns to the top
     * whenever the menu (re-)enters. MenuShow does this before entering the
     * selected submenu's run loop. */
    for (int i = 0; i < fe->menus[menu].submenu_count; i++) {
        fe->menus[menu].submenus[i].scroll = 0;
    }
    return 1;
}

/* MenuShow (FUN_00413e20): clear every submenu's run flag, then enter the
 * requested submenu. The native call blocks in MenuRunLoop; the portable
 * frontend leaves that selected state open for the frame loop to advance. */
// FUNCTION: LEGORR 0x00413e20
int frontend_open_submenu(FrontEnd* fe, int menu, int submenu_index)
{
    if (fe == NULL || !fe->built || menu < 0 || menu >= FE_MENU_COUNT ||
        submenu_index < 0 ||
        submenu_index >= fe->menus[menu].submenu_count) {
        return 0;
    }
    for (int i = 0; i < fe->menus[menu].submenu_count; ++i) {
        fe->menus[menu].submenus[i].run_flags = 0u;
    }
    return frontend_open_prepare(fe, menu, submenu_index);
}

/* Port convenience entry for callers that open submenu zero. */
int frontend_open(FrontEnd* fe, int menu)
{
    return frontend_open_submenu(fe, menu, 0);
}

/* FrontEndStateChange (FUN_004169c0): prepare state before MenuEnter chooses
 * a menu. The live option and save-event words are applied here; the
 * corpus's tree walks still need the EngineContext passed to its callbacks. */
// FUNCTION: LEGORR 0x004169c0
int frontend_state_change(FrontEnd* fe, int state)
{
    if (fe == NULL || !fe->built) {
        return 0;
    }
    if (state < FE_STATE_MAIN || state > FE_STATE_SAVE_REWARD) {
        return 0;
    }
    /* FrontEndStateChange calls OptionsLoadOrDefault for every valid flow.
     * Main/new-game/load retain the active save context's option triple;
     * options/quit only clear the frontend selection words. */
    options_frontend_selection_set(state == FE_STATE_MAIN
                                       ? -1 : fe->selected_save_slot);
    options_load_or_default(save_current_context_get(),
        state <= FE_STATE_TRAINING ? 1 : 0, 1);
    /* Save flow case 4 raises DAT_00557500 only when there is no active
     * save-slot context and the save-state event flag is already set. */
    if (state == FE_STATE_SAVE_REWARD && sav_build_ctx_target() == NULL &&
        save_state_event_context_peek() != 0u) {
        current_selection_context_set((void*)(uintptr_t)1u);
    }
    /* Every switch arm ends by raising the save-state event context. */
    save_state_event_context_set(1u);
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
    if (submenu >= fe->menus[menu].submenu_count ||
        !frontend_open_submenu(fe, menu, submenu)) {
        return 0;
    }
    if (mode == FE_STATE_SAVE_REWARD) {
        fe->save_action = 1;
    }
    return 1;
}

/* SaveMenuOpenFromReward (FUN_00462030) has no explicit front-end argument;
 * it uses the active menu/global state and enters the reward save picker.
 * The portable frame loop owns input/rendering, so menu_enter opens that same
 * picker and the frame loop advances it one frame at a time. */
namespace reference_target {
// FUNCTION: LEGORR 0x00462030
void save_menu_open_from_reward(void)
{
    FrontEnd* fe = s_reward_save_frontend;
    if (fe == NULL)
        return;
    options_write_to_slot_target();
    if (!frontend_state_change(fe, FE_STATE_SAVE_REWARD))
        return;
    /* SaveMenuOpenFromReward selects submenu zero in the separate save-menu
     * set. The legacy routine then runs a blocking input loop; the portable
     * frame loop owns that loop, so this entry prepares the same live state
     * and returns to its caller. */
    if (fe->menus[FE_MENU_SAVE].submenu_count <= 0 ||
        !frontend_open(fe, FE_MENU_SAVE))
        return;
    fe->save_action = 1;
}
}

int save_menu_open_from_reward(FrontEnd* fe)
{
    if (fe == NULL) return 0;
    s_reward_save_frontend = fe;
    reference_target::save_menu_open_from_reward();
    return fe->open && fe->save_action;
}

int save_menu_open_from_reward_bound(void)
{
    if (s_reward_save_frontend == NULL) return 0;
    return save_menu_open_from_reward(s_reward_save_frontend);
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
    for (int mi = 0; mi < FE_MENU_COUNT; ++mi) {
        FeMenu* menu = &fe->menus[mi];
        for (int si = 0; si < menu->submenu_count; ++si) {
            FeSubmenu* sub = &menu->submenus[si];
            for (int ii = 0; ii < sub->item_count; ++ii) {
                FeItem* item = &sub->items[ii];
                for (int ri = 0; ri < 8; ++ri) {
                    if (item->slider_images[ri] != NULL) {
                        texture_release(item->slider_images[ri]);
                        item->slider_images[ri] = NULL;
                    }
                }
            }
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

/* ScrollingTextDisplay (FUN_0040a0c0) enters the credits display. The
 * portable frontend is frame-driven, so its body raises the event consumed by
 * frontend_tick; the frame loop then loads the configured text and backdrop,
 * advances the 33 ms scroll clock, draws the centered rows, pumps audio/video,
 * and exits on keyboard input or mouse release. Those operations are split
 * across frontend_credits_lines_load/render_credits and frameloop_credits
 * because this module does not own the platform window or render surface. */
static void scrolling_text_display(void* text, void* font, void* background)
{
    /* The retail arguments are renderer handles. In the portable boot path,
     * the configured CreditsTextFile, RSFont, and CreditsBackAVI are resolved
     * by the frame loop that consumes this request. Keep the ABI parameters
     * explicit even though that owner supplies their portable equivalents. */
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

/* Hit admission must include the same track and resource-sized arrows as
 * the slider consumer, even after MenuUpdate clears the hovered row. */
static int fe_slider_pointer_contains(const FeSubmenu* sub, const FeItem* item,
                                       int mx, int my)
{
    int bx = sub->base_x + item->x + item->slider_x;
    int by = sub->base_y + item->y + item->slider_y;
    TextureSurface* anchor = item->slider_images[3];
    TextureSurface* right = item->slider_images[4];
    TextureSurface* left = item->slider_images[5];
    if (sub->can_scroll) by += sub->scroll;
    if (mx >= bx && mx <= bx + 100 && my >= by && my <= by + 20)
        return 1;
    if (left == NULL || right == NULL) return 0;
    if (menu_pointer_rectangle_hit_test(mx, my, bx - left->width - 4, by,
                                         left->width, left->height)) return 1;
    return anchor != NULL && menu_pointer_rectangle_hit_test(mx, my,
        bx + anchor->width + 0x68, by, right->width, right->height);
}

/* MenuSliderPointerHandler (FUN_00411870): the track drag plus the two
 * resource-sized arrow hit zones from the retail numeric-range record. */
// FUNCTION: LEGORR 0x00411870
static void fe_slider_pointer_handle(FrontEnd* fe, FeSubmenu* sub,
                                     const FeInput* in,
                                     const TextureSurface* measure)
{
    if (!in->mdown || fe->sel < 0 || fe->sel >= sub->item_count) {
        return;
    }
    FeItem* dit = &sub->items[fe->sel];
    if (dit->kind != FE_ITEM_SLIDER && dit->kind != FE_ITEM_REALSLIDER) {
        return;
    }
    (void)measure;
    int bx = sub->base_x + dit->x + dit->slider_x;
    int by = sub->base_y + dit->y + dit->slider_y;
    if (sub->can_scroll) {
        by += sub->scroll;
    }
    if (in->mouse_x < bx || in->mouse_x > bx + 100
        || in->mouse_y < by || in->mouse_y > by + 20) {
        TextureSurface* right_anchor = dit->slider_images[3];
        TextureSurface* right_arrow = dit->slider_images[4];
        TextureSurface* left_arrow = dit->slider_images[5];
        int hit = 0;
        int changed = 0;
        if (right_arrow != NULL && left_arrow != NULL) {
            int left_w = left_arrow->width;
            int left_h = left_arrow->height;
            hit = menu_pointer_rectangle_hit_test(
                in->mouse_x, in->mouse_y, bx - left_w - 4, by,
                left_w, left_h);
            if (!hit && right_anchor != NULL) {
                hit = menu_pointer_rectangle_hit_test(
                    in->mouse_x, in->mouse_y,
                    bx + right_anchor->width + 0x68, by,
                    right_arrow->width, right_arrow->height);
                if (hit && dit->value < dit->max) {
                    ++dit->value;
                    changed = 1;
                }
            } else if (hit && dit->value > dit->min) {
                --dit->value;
                changed = 1;
            }
            if (changed) {
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
        }
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
    /* Retail writes the scaled delta directly; unlike the keyboard path, it
     * does not add the configured minimum. */
    if (v != dit->value) {
        dit->value = v;
        fe_options_write(fe, dit);
        fe->option_changed = 1;
    }
}

/* --- activation -------------------------------------------------------- */

/* Partial FrontEndSelectedAction (FUN_00416d70): resolve the selected
 * campaign/tutorial picker row to its level key. Save-slot list resolution
 * remains in the frame-loop load path. */
// FUNCTION: LEGORR 0x00416d70
static const char* fe_selected_level_action(const FrontEnd* fe)
{
    const FeItem* it = frontend_active_item(fe);
    return it->level_start ? it->target : NULL;
}

/* FrontEndStartLevel (FUN_00415220): prepare transition state and record the
 * selected level; the frame loop owns the pending-level teardown and load. */
// FUNCTION: LEGORR 0x00415220
static FeEvent frontend_start_level(FrontEnd* fe, const char* level)
{
    if (fe == NULL || level == NULL || level[0] == '\0') {
        return FE_EVT_NONE;
    }
    snprintf(fe->pending_level, FE_LEVEL_NAME_MAX, "%s", level);
    /* The reference clears transition state before it tears down the active
     * level and begins LevelLoad. The frame loop owns that load/teardown. */
    unported_ledger_stubs::ledger_stub_00425c30_TransitionControl(0, 0u);
    frontend_flow_word_set(4, 0);
    frontend_close(fe);
    return FE_EVT_START_LEVEL;
}

static FeEvent fe_activate(FrontEnd* fe)
{
    FeMenu* m = &fe->menus[fe->menu];
    FeSubmenu* sub = &m->submenus[fe->sub];
    FeItem* it = &sub->items[fe->sel];

    if (it->state_gated && system_state_flags_get() != 0u) return FE_EVT_NONE;

    /* save-slot row: load the slot (FE_EVT_LOAD_GAME, fe.load_slot set) */
    if (it->load_slot) {
        fe->load_slot = it->load_slot;
        int save_action = fe->save_action;
        frontend_close(fe);
        return save_action ? FE_EVT_SAVE_GAME : FE_EVT_LOAD_GAME;
    }

    /* SaveTreeLoad bit 4 gates both pointer and keyboard activation. */
    if (it->level_start && !it->level_enabled) return FE_EVT_NONE;

    /* picker row: start the selected level */
    const char* level_action = fe_selected_level_action(fe);
    if (level_action != NULL) {
        if (fe->menu == FE_MENU_MAIN) fe->origin_picker_sub = fe->sub;
        return frontend_start_level(fe, level_action);
    }

    /* MenuInputTick's cycle case advances the selected option on the
     * activation edge and wraps from the last choice to the first. The
     * portable keyboard path already handles left/right; activation must do
     * the same for mouse clicks and supported keyboard activation. */
    if (it->kind == FE_ITEM_CYCLE && it->cycle_count > 0) {
        ++it->value;
        if (it->value >= it->cycle_count) {
            it->value = 0;
        }
        fe_options_write(fe, it);
        fe->option_changed = 1;
        return FE_EVT_NONE;
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
        if (fe->menu == FE_MENU_OPTIONS) {
            if (s_stristr(it->label, "replay"))
                objective_announce_current(0);
            else if (!s_stristr(it->label, "continue"))
                return FE_EVT_NONE;
            frontend_close(fe);
            return FE_EVT_RESUME;
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
                frontend_close(fe);
                return FE_EVT_LEAVE_LEVEL;
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
        main_menu_text_item();
        return FE_EVT_NONE;
    }

    /* Next -> navigate submenus ("Menu%i" target) */
    if (it->kind == FE_ITEM_NEXT && it->target[0] != '\0'
        && s_stristr(it->target, "menu") != 0) {
        int idx = atoi(it->target + 4) - 1; /* "MenuN" */
        if (idx >= 0 && idx < m->submenu_count) {
            if (fe->menu == FE_MENU_MAIN && fe->sub == 0 &&
                s_stristr(m->submenus[idx].image, "LevelPick")) {
                fe->selected_save_slot = -1;
                if (!s_stristr(m->submenus[idx].image, "LevelPickT"))
                    (void)frontend_apply_campaign_progress(fe, NULL, 0);
            }
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

/* MenuScrollStateUpdate runs after the current menu tick and render. */
void frontend_scroll_update(FrontEnd* fe, const FeInput* in)
{
    union { uint32_t bits; float value; } scale, curve;
    FeSubmenu* sub;
    double q, amount;
    float y;
    if (fe == NULL || in == NULL || !fe->open) return;
    if (fe->menu < 0 || fe->menu >= FE_MENU_COUNT) return;
    if (fe->sub < 0 || fe->sub >= fe->menus[fe->menu].submenu_count) return;
    sub = &fe->menus[fe->menu].submenus[fe->sub];
    if (!sub->can_scroll) {
        sub->scroll = 0;
        return;
    }
    if (sub->scroll_extent <= 0) sub->scroll_extent = fe_scroll_measure(sub);
    y = (float)in->mouse_y;
    if (in->mouse_y >= 200 && in->mouse_y <= 280) return;
    if (y < 480.0f && !in->mdown) {
        scale.bits = 0x3e555555u;
        curve.bits = 0x3ca3d70au;
        q = (double)y * scale.value - 50.0;
        amount = q * q * curve.value;
        if (y < 240.0f) amount = -amount;
        sub->scroll += (int)(amount * -0.25);
    }
    if (sub->scroll > 0) sub->scroll = 0;
    if (sub->scroll - 480 < -sub->scroll_extent)
        sub->scroll = 480 - sub->scroll_extent;
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
    const int pointer_selection = fe->menu == FE_MENU_MAIN ||
        (fe->menu == FE_MENU_PAUSED &&
         in->mouse_x >= 0 && in->mouse_y >= 0);
    if (fe->sel < 0 && !pointer_selection) {
        fe->sel = 0;
    }

    /* MenuUpdate clears its current item before hit-testing. Pause menus
     * use that same field for option input and selected drawing, so a miss
     * must not retain the previous row or the submenu's initial item. */
    if (pointer_selection) {
        int idx = -1;
        if (in->mouse_x >= 0 && in->mouse_y >= 0) {
            fe_item_pick(fe, in->mouse_x, in->mouse_y, measure, &idx);
        }
        fe->sel = idx;
    }

    if (!pointer_selection && in->up) {
        if (fe->sel > 0) {
            --fe->sel;
        }
    }
    if (!pointer_selection && in->down) {
        if (fe->sel < sub->item_count - 1) {
            ++fe->sel;
        }
    }

    /* Resolve the current pointer target before applying horizontal input.
     * Motion and a key edge can arrive in the same frame. */
    if ((in->left || in->right) && in->mouse_x >= 0 && in->mouse_y >= 0) {
        int hovered = 0;
        if (fe_item_pick(fe, in->mouse_x, in->mouse_y, measure, &hovered)) {
            fe->sel = hovered;
        }
    }

    /* Slider adjustment clamps to [min,max]. Cycle input wraps because the
     * reference's kind-0 path applies (value + 1) % option_count and wraps
     * decrements from zero to the last option. */
    if (fe->sel >= 0) {
        FeItem* dit = &sub->items[fe->sel];
        if (dit->kind == FE_ITEM_CYCLE && dit->cycle_count > 0) {
            if (in->right) {
                dit->value = (dit->value + 1) % dit->cycle_count;
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
            if (in->left) {
                --dit->value;
                if (dit->value < 0) {
                    dit->value = dit->cycle_count - 1;
                }
                /* Retail's left-cycle branch stores the wrapped ordinal but
                 * does not invoke the cycle callback. Slider left still does. */
                if (fe->opts != NULL)
                    options_set_label(fe->opts, dit->label, dit->value);
                fe->option_changed = 1;
            }
        } else if (dit->kind == FE_ITEM_SLIDER
                   || dit->kind == FE_ITEM_REALSLIDER) {
            if (in->right && dit->value < dit->max) {
                ++dit->value;
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
            if (in->left && dit->value > dit->min) {
                --dit->value;
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
        }
    }

    /* pointer: hover selects; a click activates the hovered row.
     * When the pointer is in use but outside every item, a click is a
     * no-op (the menu stays up). */
    if (in->mouse_x >= 0 && in->mouse_y >= 0) {
        fe->pointer_x = in->mouse_x;
        fe->pointer_y = in->mouse_y;
        int idx = 0;
        int hit = fe_item_pick(fe, in->mouse_x, in->mouse_y, measure, &idx);
        if (hit) {
            fe->sel = idx;
        } else if (fe->menu == FE_MENU_MAIN || fe->menu == FE_MENU_PAUSED) {
            /* These menus select only the current pointer target. */
            fe->sel = -1;
        }

        /* Slider pointer drag (corpus FUN_00411a00 case 3 routes a held
         * button through FUN_00411870 every frame): with the pointer held
         * inside the bar's 100x20 rect anchored at the selected item's
         * origin, the value continuously tracks pointer x —
         *   v = round(clamp((mx - x) * 0.01, 0.0, 1.0) * (max - min))
         * (constants 0x49e170/174/178 = 0.0 / 1.0 / 0.01, rect w/h =
         * 0x64/0x14). The corpus mouse path omits the +min offset that the
         * keyboard path keeps. */
        fe_slider_pointer_handle(fe, sub, in, measure);

        if (in->click) {
            FeEvent event = hit ? fe_activate(fe) : FE_EVT_NONE;
            fe_overlay_step(fe);
            return event;
        }
    }

    if (in->esc) {
        /* Main and pause menus leave navigation to their configured items. */
        if (fe->menu == FE_MENU_PAUSED || fe->menu == FE_MENU_MAIN) {
            fe_overlay_step(fe);
        } else if (fe->sub != 0) {
            fe->sub = 0;
            fe->sel = 0;
            fe_overlay_step(fe);
        } else {
            fe->quit = 1;
            frontend_close(fe);
            return FE_EVT_QUIT;
        }
        return FE_EVT_NONE;
    }
    /* Main and pause menu rows activate on pointer clicks, not Enter/Space. */
    if (!in->enter || fe->menu == FE_MENU_MAIN || fe->menu == FE_MENU_PAUSED) {
        /* MenuTick updates input and selection before its overlay controller. */
        fe_overlay_step(fe);
        return FE_EVT_NONE;
    }
    FeEvent event = fe_activate(fe);
    fe_overlay_step(fe);
    return event;
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

void frontend_background_render(const FrontEnd* fe, void* pixels, int pitch,
                                 int width, int height)
{
    const FeSubmenu* sub = frontend_active_submenu(fe);
    const char* path = frontend_active_image(fe);
    if (sub == NULL || path == NULL) return;
    TextureSurface* image = texture_get_or_load(NULL, path);
    if (image == NULL) return;
    TextureSurface view = *image;
    if (sub->image_keyed) {
        uint32_t key = image->key_count == 2 ? image->keys[0] : 0u;
        texture_source_color_key_set(&view, key, key);
    }
    /* MenuImageDraw's explicit-origin lane ignores the scrolling plane. */
    int x = sub->image_has_origin ? (int)sub->image_x : 0;
    int y = sub->image_has_origin ? (int)sub->image_y : sub->scroll;
    surface_blit_texture(pixels, pitch, width, height, &view, x, y);
    texture_release(image);
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

/* RenderUiFrameCell @ 0x412450 selects either the supplied direct surface
 * or one of eight animation frames in the renderer-owned frame table. */
// FUNCTION: LEGORR 0x00412450
void menu_render_ui_frame_cell(void* px, int pitch, int w, int h,
                               int x, int y,
                               const TextureSurface* direct_surface,
                               const FeUiFrameCellTable* table,
                               int item_index, int frame_index)
{
    if (px == NULL || w <= 0 || h <= 0 || table == NULL ||
        table->entries == NULL || item_index < 0 || frame_index < 0 ||
        (size_t)item_index >= table->entry_count) {
        return;
    }
    if ((table->entries[item_index].flags & 1u) != 0u) {
        if (direct_surface != NULL && direct_surface->pixels != NULL &&
            direct_surface->width > 0 && direct_surface->height > 0) {
            surface_blit_texture(px, pitch, w, h, direct_surface, x, y);
        }
        return;
    }
    if (frame_index >= 8 || table->frame_surfaces == NULL) {
        return;
    }
    size_t surface_index = (size_t)item_index * 8u +
                           (size_t)frame_index;
    if (surface_index >= table->frame_surface_count) return;
    const TextureSurface* image = table->frame_surfaces[surface_index];
    if (image == NULL || image->width <= 0 || image->height <= 0 ||
        x >= 640 || -image->width >= x ||
        -image->height >= y || y >= 480) return;
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

/* Load a font bitmap for the frontend's CPU text path. Task/reward records
 * that need the retail glyph atlas are created by font_atlas_record_create. */
TextureSurface* bitmap_font_resource_load(const char* name)
{
    TextureSurface* surface;
    int cell_width;
    int cell_height;
    if (name == NULL || name[0] == '\0') return NULL;
    surface = image_surface_load(name, 0u, 0u);
    if (surface == NULL) return NULL;
    if (!fe_font_cell(surface, &cell_width, &cell_height)) {
        texture_free(surface);
        return NULL;
    }
    return surface;
}

/* FontResourceLoad measures the top row against the second palette key. */
static int fe_font_advance(const TextureSurface* f, int cw, int ch, int g,
                           const uint32_t keys[2])
{
    int col = g % FE_FONT_GRID_COLS;
    int row = g / FE_FONT_GRID_COLS;
    int x;
    for (x = cw - 1; x > 0; --x) {
        uint32_t pixel = f->pixels[(size_t)(row * ch) *
                                    (size_t)f->width + col * cw + x];
        if (pixel != keys[1]) return x + 1;
    }
    return 1;
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
            char* end;
            unsigned long parsed = strtoul(text + i + 4, &end, 16);
            int valid = end == text + i + 12 && *end == ']';
            uint32_t handle = (uint32_t)parsed;
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
            advance = 0;
        } else {
            advance = fe_font_glyph_draw_and_measure(
                px, pitch, w, h, x + pen_x, y, font, cell_w, cell_h, glyph,
                keys, px != NULL && w > 0 && h > 0);
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
        call malloc
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

/* The native initializer publishes this list to DAT_0054bf68. */
static const char* const s_tooltip_names[38] = {
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
    "ToolTip_FrontEnd_Back"
};
static const char* s_tooltip_name_table[38];

/* CaseInsensitiveNameTableLookup (FUN_0046b4d0) searches the table published
 * by TooltipOverlayStateInitialize. Results are one-based. The fallback to
 * canonical labels keeps config-only callers usable before GameConfigInit's
 * renderer setup path has initialized the global table in this port. */
// FUNCTION: LEGORR 0x0046b4d0
int case_insensitive_name_table_lookup(const char* name, int* out_index)
{
    /* Corpus shape: the walk strides the name slots by four with a
     * one-based counter, feeds each entry to strcasecmp_lite untested,
     * and answers through the raw out pointer. Deviation: the name NULL
     * gate stays (the port's menu caller can pass a null optional name;
     * the corpus strides it blindly). */
    if (name == NULL) {
        return 0;
    }
    int index = 1;
    while (index <= 38) {
        const char* candidate = s_tooltip_name_table[index - 1];
        if (candidate == NULL) candidate = s_tooltip_names[index - 1];
        if (strcasecmp_lite(name, candidate) == 0) {
            *out_index = index;
            return 1;
        }
        ++index;
    }
    return 0;
}

/* UiPanelNameTablesInit publishes these exact pointer-table bands. The
 * button table is twelve 24-pointer rows and ends at DAT_005007d0. */
// GLOBAL: LEGORR 0x004a8924
static const char s_panel_name_radar[] = "Panel_Radar";
// GLOBAL: LEGORR 0x004a8914
static const char s_panel_name_radar_fill[] = "Panel_RadarFill";
// GLOBAL: LEGORR 0x004a8900
static const char s_panel_name_radar_overlay[] = "Panel_RadarOverlay";
// GLOBAL: LEGORR 0x004a88f0
static const char s_panel_name_messages[] = "Panel_Messages";
// GLOBAL: LEGORR 0x004a88dc
static const char s_panel_name_messages_side[] = "Panel_MessagesSide";
// GLOBAL: LEGORR 0x004a88c4
static const char s_panel_name_crystal_side_bar[] = "Panel_CrystalSideBar";
// GLOBAL: LEGORR 0x004a88b4
static const char s_panel_name_top_panel[] = "Panel_TopPanel";
// GLOBAL: LEGORR 0x004a88a0
static const char s_panel_name_information[] = "Panel_Information";
// GLOBAL: LEGORR 0x004a888c
static const char s_panel_name_priority_list[] = "Panel_PriorityList";
// GLOBAL: LEGORR 0x004a8878
static const char s_panel_name_camera_control[] = "Panel_CameraControl";
// GLOBAL: LEGORR 0x004a8868
static const char s_panel_name_info_dock[] = "Panel_InfoDock";
// GLOBAL: LEGORR 0x004a8930
static const char s_panel_name_encyclopedia[] = "Panel_Encyclopedia";
// GLOBAL: LEGORR 0x004a884c
static const char s_panel_button_radar_toggle[] = "PanelButton_Radar_Toggle";
// GLOBAL: LEGORR 0x004a8828
static const char s_panel_button_radar_tagged[] = "PanelButton_Radar_TaggedObjectView";
// GLOBAL: LEGORR 0x004a880c
static const char s_panel_button_radar_zoom_in[] = "PanelButton_Radar_ZoomIn";
// GLOBAL: LEGORR 0x004a87f0
static const char s_panel_button_radar_zoom_out[] = "PanelButton_Radar_ZoomOut";
// GLOBAL: LEGORR 0x004a87d4
static const char s_panel_button_radar_map_view[] = "PanelButton_Radar_MapView";
// GLOBAL: LEGORR 0x004a87b8
static const char s_panel_button_info_dock_goto[] = "PanelButton_InfoDock_Goto";
// GLOBAL: LEGORR 0x004a879c
static const char s_panel_button_info_dock_close[] = "PanelButton_InfoDock_Close";
// GLOBAL: LEGORR 0x004a877c
static const char s_panel_button_top_options[] = "PanelButton_TopPanel_Options";
// GLOBAL: LEGORR 0x004a875c
static const char s_panel_button_top_priorities[] = "PanelButton_TopPanel_Priorities";
// GLOBAL: LEGORR 0x004a873c
static const char s_panel_button_top_call_to_arms[] = "PanelButton_TopPanel_CallToArms";
// GLOBAL: LEGORR 0x004a871c
static const char s_panel_button_information_toggle[] = "PanelButton_Information_Toggle";
// GLOBAL: LEGORR 0x004a86f8
static const char s_panel_button_information_function[] = "PanelButton_Information_Function";
// GLOBAL: LEGORR 0x004a86d4
static const char s_panel_button_priority_disable1[] = "PanelButton_PriorityList_Disable1";
// GLOBAL: LEGORR 0x004a86b0
static const char s_panel_button_priority_disable2[] = "PanelButton_PriorityList_Disable2";
// GLOBAL: LEGORR 0x004a868c
static const char s_panel_button_priority_disable3[] = "PanelButton_PriorityList_Disable3";
// GLOBAL: LEGORR 0x004a8668
static const char s_panel_button_priority_disable4[] = "PanelButton_PriorityList_Disable4";
// GLOBAL: LEGORR 0x004a8644
static const char s_panel_button_priority_disable5[] = "PanelButton_PriorityList_Disable5";
// GLOBAL: LEGORR 0x004a8620
static const char s_panel_button_priority_disable6[] = "PanelButton_PriorityList_Disable6";
// GLOBAL: LEGORR 0x004a85fc
static const char s_panel_button_priority_disable7[] = "PanelButton_PriorityList_Disable7";
// GLOBAL: LEGORR 0x004a85d8
static const char s_panel_button_priority_disable8[] = "PanelButton_PriorityList_Disable8";
// GLOBAL: LEGORR 0x004a85b4
static const char s_panel_button_priority_disable9[] = "PanelButton_PriorityList_Disable9";
// GLOBAL: LEGORR 0x004a8594
static const char s_panel_button_priority_up1[] = "PanelButton_PriorityList_UpOne1";
// GLOBAL: LEGORR 0x004a8574
static const char s_panel_button_priority_up2[] = "PanelButton_PriorityList_UpOne2";
// GLOBAL: LEGORR 0x004a8554
static const char s_panel_button_priority_up3[] = "PanelButton_PriorityList_UpOne3";
// GLOBAL: LEGORR 0x004a8534
static const char s_panel_button_priority_up4[] = "PanelButton_PriorityList_UpOne4";
// GLOBAL: LEGORR 0x004a8514
static const char s_panel_button_priority_up5[] = "PanelButton_PriorityList_UpOne5";
// GLOBAL: LEGORR 0x004a84f4
static const char s_panel_button_priority_up6[] = "PanelButton_PriorityList_UpOne6";
// GLOBAL: LEGORR 0x004a84d4
static const char s_panel_button_priority_up7[] = "PanelButton_PriorityList_UpOne7";
// GLOBAL: LEGORR 0x004a84b4
static const char s_panel_button_priority_up8[] = "PanelButton_PriorityList_UpOne8";
// GLOBAL: LEGORR 0x004a8494
static const char s_panel_button_priority_close[] = "PanelButton_PriorityList_Close";
// GLOBAL: LEGORR 0x004a8474
static const char s_panel_button_priority_reset[] = "PanelButton_PriorityList_Reset";
// GLOBAL: LEGORR 0x004a8454
static const char s_panel_button_encyclopedia_close[] = "PanelButton_Encyclopedia_Close";
// GLOBAL: LEGORR 0x004a8430
static const char s_panel_button_camera_zoom_in[] = "PanelButton_CameraControl_ZoomIn";
// GLOBAL: LEGORR 0x004a840c
static const char s_panel_button_camera_zoom_out[] = "PanelButton_CameraControl_ZoomOut";
// GLOBAL: LEGORR 0x004a83e0
static const char s_panel_button_camera_cycle_buildings[] = "PanelButton_CameraControl_CycleBuildings";
// GLOBAL: LEGORR 0x004a83bc
static const char s_panel_button_camera_rotate[] = "PanelButton_CameraControl_Rotate";
// GLOBAL: LEGORR 0x004a839c
static const char s_panel_button_crystal_ore[] = "PanelButton_CrystalSideBar_Ore";
// GLOBAL: LEGORR 0x004a8378
static const char s_panel_button_crystal_crystals[] = "PanelButton_CrystalSideBar_Crystals";
// GLOBAL: LEGORR 0x005000e0
static const char* s_ui_panel_names[12];
// GLOBAL: LEGORR 0x00500350
static const char* s_ui_panel_indexed_names[12][24];
/* These words are also the selected category-cell coordinates. The table
 * initializer sets the no-selection sentinel used by SimCategoryCellHitTest. */
// GLOBAL: LEGORR 0x005007d0
static int s_category_cell_selected_x;
// GLOBAL: LEGORR 0x005007d4
static int s_category_cell_selected_y;

void ui_panel_name_tables_install(const char* const names[12],
                                  const char* const indexed_names[][24],
                                  unsigned int indexed_count)
{
    unsigned int i;
    unsigned int j;
    if (indexed_count > 12u) indexed_count = 12u;
    for (i = 0; i < 12u; ++i) {
        s_ui_panel_names[i] = names != NULL ? names[i] : NULL;
    }
    for (i = 0; i < 12u; ++i) {
        for (j = 0; j < 24u; ++j) {
            s_ui_panel_indexed_names[i][j] =
                indexed_names != NULL && i < indexed_count
                    ? indexed_names[i][j] : NULL;
        }
    }
}

static int ui_panel_string_equal_case_insensitive(const char* first,
                                                   const char* second)
{
    if (first == NULL || second == NULL) return 0;
    while (*first != '\0' && *second != '\0') {
        if (tolower((unsigned char)*first) !=
            tolower((unsigned char)*second)) return 0;
        ++first;
        ++second;
    }
    return *first == '\0' && *second == '\0';
}

/* UiPanelNameTableLookup (FUN_0045a370): the corpus do-while-walks the twelve
 * panel-name dwords at DAT_005000e0..DAT_00500110, __strcmpi-comparing each
 * against the caller's name; the first case-insensitive match writes the
 * zero-based slot and returns 1, the exhausted scan returns 0. Deviations:
 * the port guards null arguments (the corpus dereferences blindly) and the
 * null-name check inside the comparator stands in for the corpus's direct
 * __strcmpi against the installed pointer. */
// FUNCTION: LEGORR 0x0045a370
int ui_panel_name_table_lookup(const char* name, int* out_index)
{
    const char** slot;
    int index;
    if (name == NULL || out_index == NULL) return 0;
    slot = s_ui_panel_names;
    index = 0;
    do {
        if (ui_panel_string_equal_case_insensitive(name, *slot)) {
            *out_index = index;
            return 1;
        }
        ++slot;
        ++index;
    } while (slot < s_ui_panel_names + 12);
    return 0;
}

/* UiPanelIndexedNameLookup (FUN_0045a410): the corpus strides the category's
 * 24 name dwords at DAT_00500350 + table * 0x60, skipping null slots, and
 * __strcmpi-compares the rest; the first match writes the zero-based slot and
 * returns 1. Deviations: the port bounds the table index to the 64 installed
 * tables (the corpus strides the DAT band blindly) and null-guards its
 * arguments. */
// FUNCTION: LEGORR 0x0045a410
int ui_panel_indexed_name_lookup(unsigned int table_index, const char* name,
                                 unsigned int* out_index)
{
    const char** slot;
    unsigned int index;
    if (name == NULL || out_index == NULL || table_index >= 12u) return 0;
    slot = s_ui_panel_indexed_names[table_index];
    index = 0;
    do {
        if (*slot != NULL &&
            ui_panel_string_equal_case_insensitive(name, *slot)) {
            *out_index = index;
            return 1;
        }
        ++slot;
        ++index;
    } while (index < 24u);
    return 0;
}

/* UiPanelNameTablesInit (FUN_0045a030) stores twelve panel-name pointers,
 * thirty-eight button-name pointers at their original 24-slot row offsets,
 * then initializes the category-cell sentinel to (12, 24). */
// FUNCTION: LEGORR 0x0045a030
void ui_panel_name_tables_init(void)
{
    s_category_cell_selected_x = 0x0c;
    s_category_cell_selected_y = 0x18;
    /* Keep each string pointer at the exact panel/category/slot offset from
     * the reference tables. The target leaves all unlisted slots untouched. */
    s_ui_panel_names[11] = s_panel_name_encyclopedia;
    s_ui_panel_names[0] = s_panel_name_radar;
    s_ui_panel_names[1] = s_panel_name_radar_fill;
    s_ui_panel_names[2] = s_panel_name_radar_overlay;
    s_ui_panel_names[3] = s_panel_name_messages;
    s_ui_panel_names[4] = s_panel_name_messages_side;
    s_ui_panel_names[5] = s_panel_name_crystal_side_bar;
    s_ui_panel_names[6] = s_panel_name_top_panel;
    s_ui_panel_names[7] = s_panel_name_information;
    s_ui_panel_names[8] = s_panel_name_priority_list;
    s_ui_panel_names[9] = s_panel_name_camera_control;
    s_ui_panel_names[10] = s_panel_name_info_dock;

    s_ui_panel_indexed_names[0][0] = s_panel_button_radar_toggle;
    s_ui_panel_indexed_names[0][1] = s_panel_button_radar_tagged;
    s_ui_panel_indexed_names[0][2] = s_panel_button_radar_zoom_in;
    s_ui_panel_indexed_names[0][3] = s_panel_button_radar_zoom_out;
    s_ui_panel_indexed_names[0][4] = s_panel_button_radar_map_view;
    s_ui_panel_indexed_names[10][0] = s_panel_button_info_dock_goto;
    s_ui_panel_indexed_names[10][1] = s_panel_button_info_dock_close;
    s_ui_panel_indexed_names[6][0] = s_panel_button_top_options;
    s_ui_panel_indexed_names[6][1] = s_panel_button_top_priorities;
    s_ui_panel_indexed_names[6][2] = s_panel_button_top_call_to_arms;
    s_ui_panel_indexed_names[7][0] = s_panel_button_information_toggle;
    s_ui_panel_indexed_names[7][1] = s_panel_button_information_function;
    s_ui_panel_indexed_names[8][0] = s_panel_button_priority_disable1;
    s_ui_panel_indexed_names[8][1] = s_panel_button_priority_disable2;
    s_ui_panel_indexed_names[8][2] = s_panel_button_priority_disable3;
    s_ui_panel_indexed_names[8][3] = s_panel_button_priority_disable4;
    s_ui_panel_indexed_names[8][4] = s_panel_button_priority_disable5;
    s_ui_panel_indexed_names[8][5] = s_panel_button_priority_disable6;
    s_ui_panel_indexed_names[8][6] = s_panel_button_priority_disable7;
    s_ui_panel_indexed_names[8][7] = s_panel_button_priority_disable8;
    s_ui_panel_indexed_names[8][8] = s_panel_button_priority_disable9;
    s_ui_panel_indexed_names[8][9] = s_panel_button_priority_up1;
    s_ui_panel_indexed_names[8][10] = s_panel_button_priority_up2;
    s_ui_panel_indexed_names[8][11] = s_panel_button_priority_up3;
    s_ui_panel_indexed_names[8][12] = s_panel_button_priority_up4;
    s_ui_panel_indexed_names[8][13] = s_panel_button_priority_up5;
    s_ui_panel_indexed_names[8][14] = s_panel_button_priority_up6;
    s_ui_panel_indexed_names[8][15] = s_panel_button_priority_up7;
    s_ui_panel_indexed_names[8][16] = s_panel_button_priority_up8;
    s_ui_panel_indexed_names[8][17] = s_panel_button_priority_close;
    s_ui_panel_indexed_names[8][18] = s_panel_button_priority_reset;
    s_ui_panel_indexed_names[11][0] = s_panel_button_encyclopedia_close;
    s_ui_panel_indexed_names[9][0] = s_panel_button_camera_zoom_in;
    s_ui_panel_indexed_names[9][1] = s_panel_button_camera_zoom_out;
    s_ui_panel_indexed_names[9][2] = s_panel_button_camera_cycle_buildings;
    s_ui_panel_indexed_names[9][3] = s_panel_button_camera_rotate;
    s_ui_panel_indexed_names[5][0] = s_panel_button_crystal_ore;
    s_ui_panel_indexed_names[5][1] = s_panel_button_crystal_crystals;
}

static ToolTipIconConfigHooks s_tooltip_icon_config_hooks;
static const char* s_tooltip_primary_names[11] = {
    "LegoObject_ToolType_Drill", "LegoObject_ToolType_Spade",
    "LegoObject_ToolType_Hammer", "LegoObject_ToolType_Spanner",
    "LegoObject_ToolType_Laser", "LegoObject_ToolType_PusherGun",
    "LegoObject_ToolType_BirdScarer", "LegoObject_ToolType_FreezerGun",
    NULL, NULL, NULL
};
static const char* s_tooltip_secondary_names[6] = {
    "LegoObject_AbilityType_Pilot", "LegoObject_AbilityType_Sailor",
    "LegoObject_AbilityType_Driver", "LegoObject_AbilityType_Dynamite",
    "LegoObject_AbilityType_Repair", "LegoObject_AbilityType_Scanner"
};
static void* s_tooltip_primary_handles[11];
static void* s_tooltip_secondary_handles[6];
static void* s_tooltip_blank_handle;
static void* s_tooltip_cursor_handle;

void tooltip_icon_config_hooks_set(const ToolTipIconConfigHooks* hooks,
                                   const char* const primary_names[11],
                                   const char* const secondary_names[6])
{
    unsigned int i;
    if (hooks == NULL) {
        memset(&s_tooltip_icon_config_hooks, 0,
               sizeof(s_tooltip_icon_config_hooks));
    }
    else {
        s_tooltip_icon_config_hooks = *hooks;
    }
    for (i = 0; i < 11u; ++i)
        s_tooltip_primary_names[i] =
            primary_names != NULL ? primary_names[i] : NULL;
    for (i = 0; i < 6u; ++i)
        s_tooltip_secondary_names[i] =
            secondary_names != NULL ? secondary_names[i] : NULL;
}

void* tooltip_icon_handle_get(unsigned int bank, unsigned int index)
{
    if (bank == 0u && index < 11u) return s_tooltip_primary_handles[index];
    if (bank == 1u && index < 6u) return s_tooltip_secondary_handles[index];
    if (bank == 2u && index == 0u) return s_tooltip_blank_handle;
    if (bank == 2u && index == 1u) return s_tooltip_cursor_handle;
    return NULL;
}

static const TextureSurface* s_advisor_tooltip_font;

const char* advisor_tooltip_text_get(int index)
{
    TimedEffectRecord* record = index >= 0 && index < 38
        ? timed_effect_record_get((unsigned int)index + 1u) : NULL;
    return record != NULL && record->payload[0] != 0
        ? (const char*)record->payload : NULL;
}

int advisor_tooltip_sound_get(int index)
{
    TimedEffectRecord* record = index >= 0 && index < 38
        ? timed_effect_record_get((unsigned int)index + 1u) : NULL;
    return record != NULL ? (int)record->cue : 0;
}

/* The text half of ToolTip_SetContent (FUN_0046b520) for the advisor
 * tool tips: copy the value into the slot's record, turning "\n" into a
 * newline and '_' into a space, measure it and arm the record. Image
 * entries ('@') require the indexed resource owner; do not turn their file
 * names into visible tooltip text. */
static void advisor_tooltip_text_apply(int index, const char* value)
{
    TimedEffectRecord* record;
    char* out;
    size_t used = 0;
    int lines = 0;
    uint32_t width;
    if (value == NULL || *value == '@') return;
    record = timed_effect_record_get((unsigned int)index);
    out = (char*)record->payload;
    while (*value != '\0' && used < 511u) {
        if (*value == '\\' && value[1] == 'n') {
            out[used++] = '\n'; value += 2;
        } else {
            out[used++] = *value == '_' ? ' ' : *value;
            ++value;
        }
    }
    out[used] = '\0';
    width = formatted_text_draw_and_measure(s_advisor_tooltip_font,
        NULL, 0, 0, 0, 0, 0, out, &lines);
    memcpy((char*)record + 0x204, &width, sizeof(width));
    memcpy((char*)record + 0x208, &lines, sizeof(lines));
    memset((char*)record + 0x20c, 0, 0x64);
    record->flags = (record->flags & ~8u) | 3u;
}

/* AdvisorTooltipAndTooltipSoundConfigLoad (FUN_00434da0): the two config
 * groups are keyed by the 38 named tooltip slots. Keep their text and sound
 * alias in the same slot used by case_insensitive_name_table_lookup. */
// FUNCTION: LEGORR 0x00434DA0
void advisor_tooltip_and_sound_config_load(BlockFile* config)
{
    BlockRec* entry;
    int index;
    int alias;
    for (entry = block_subgroup(config, string_path_join(
             util_root_name(config), "ToolTips", (const char*)NULL));
         entry != NULL; entry = block_next_equal_key(entry)) {
        if (case_insensitive_name_table_lookup(entry->key, &index))
            advisor_tooltip_text_apply(index, entry->value);
    }
    for (entry = block_subgroup(config, string_path_join(
             util_root_name(config), "ToolTipsSFXs", (const char*)NULL));
         entry != NULL; entry = block_next_equal_key(entry)) {
        if (case_insensitive_name_table_lookup(entry->key, &index) &&
            sound_alias_lookup(entry->value, &alias)) {
            timed_effect_cue_request_set(index, (uint32_t)alias);
        }
    }
}

/* ToolTipIconsConfigLoad (FUN_00437150): load matching images from the
 * ToolTipIcons config section into the two named icon banks and special
 * Blank/Cursor slots. */
// FUNCTION: LEGORR 0x00437150
void tooltip_icons_config_load(void* config)
{
    BlockRec* section = block_subgroup((BlockFile*)config,
        string_path_join(util_root_name((BlockFile*)config), "ToolTipIcons",
                         (const char*)NULL));
    BlockRec* entry;
    if (section == NULL) {
        if (s_tooltip_cursor_handle != NULL) {
            texture_release((TextureSurface*)s_tooltip_cursor_handle);
        }
        s_tooltip_cursor_handle = NULL;
        return;
    }
    entry = section;
    while (entry != NULL) {
        const char* name = block_name(entry);
        const char* value = block_value(entry);
        unsigned int i;
        int matched = 0;
        for (i = 0; i < 11u; ++i) {
            if (ui_panel_string_equal_case_insensitive(
                    name, s_tooltip_primary_names[i])) {
                s_tooltip_primary_handles[i] = image_surface_load(value, 0u, 0u);
                matched = 1;
                break;
            }
        }
        if (!matched) {
            for (i = 0; i < 6u; ++i) {
                if (ui_panel_string_equal_case_insensitive(
                        name, s_tooltip_secondary_names[i])) {
                    s_tooltip_secondary_handles[i] = image_surface_load(
                        value, 0u, 0u);
                    matched = 1;
                    break;
                }
            }
        }
        if (!matched && ui_panel_string_equal_case_insensitive(name, "Blank"))
            s_tooltip_blank_handle = image_surface_load(value, 0u, 0u);
        else if (!matched && ui_panel_string_equal_case_insensitive(
                                 name, "Cursor"))
            s_tooltip_cursor_handle = image_surface_load(value, 0u, 0u);
        entry = block_next_sibling(entry);
    }
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
static int fe_picker_submenu(const FeSubmenu* sub)
{
    int i;
    if (sub == NULL) return 0;
    for (i = 0; i < sub->item_count; ++i)
        if (sub->items[i].level_start) return 1;
    return 0;
}

/* MenuBMP records have an exact source key enabled by the retail list
 * constructor. A local view avoids changing the shared cached texture. */
static void fe_blit_menu_bitmap(void* px, int pitch, int w, int h,
                                const TextureSurface* image, int x, int y)
{
    TextureSurface keyed = *image;
    uint32_t key = image->key_count == 2 ? image->keys[0] : 0;
    texture_source_color_key_set(&keyed, key, key);
    surface_blit_texture(px, pitch, w, h, &keyed, x, y);
}

static const char* fe_item_image(const FeItem* item, int selected)
{
    if (item->level_start && !item->level_enabled && item->image_disabled[0])
        return item->image_disabled;
    if (selected && item->image_selected[0]) return item->image_selected;
    return item->image_normal;
}

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
    if (sub != NULL && sub->can_scroll &&
        (it->level_start || !fe_picker_submenu(sub))) {
        *y += sub->scroll;
    }
}

/* CanScroll content extent (the corpus's menu+8 "plane height"): the
 * deepest item row of the submenu plus one row height. */
static int fe_scroll_measure(const FeSubmenu* sub)
{
    int maxy = 0;
    int picker = fe_picker_submenu(sub);
    if (picker && sub->image[0]) {
        TextureSurface* background = texture_get_or_load(NULL, sub->image);
        if (background != NULL) {
            int height = background->height;
            texture_release(background);
            if (height > 0) return height;
        }
    }
    for (int i = 0; i < sub->item_count; i++) {
        if (picker && !sub->items[i].level_start) continue;
        int top = sub->items[i].level_start ? sub->items[i].y
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
    int rh = measure != NULL ? chh : 12;
    for (int i = 0; i < sub->item_count; i++) {
        const FeItem* it = &sub->items[i];
        int x, y;
        if (it->level_start && !it->level_enabled) continue;
        fe_item_pos(fe, it, i, 0, measure, &x, &y);
        char label[FE_NAME_MAX];
        fe_display_label(it->label, label, sizeof(label));
        int tw = measure != NULL ? fe_font_text_width(label, measure)
                                 : fe_text_width(label);
        int hit_h = rh;
        if (it->image_normal[0] != '\0') {
            TextureSurface* image = texture_get_or_load(NULL,
                it->level_start && it->image_selected[0] != '\0'
                    ? it->image_selected : it->image_normal);
            if (image != NULL) {
                tw = image->width;
                hit_h = image->height;
                texture_release(image);
            }
        }
        if ((it->kind == FE_ITEM_SLIDER || it->kind == FE_ITEM_REALSLIDER) &&
            fe_slider_pointer_contains(sub, it, mx, my)) {
            if (out_idx != NULL) *out_idx = i;
            return 1;
        }
        /* MenuItemArea uses the label's measured width and font height.
         * Extra padding makes a short cycle label active outside its ink
         * column; its separately drawn value does not extend the area. */
        if ((it->level_start && mx >= x && mx <= x + tw &&
             my >= y && my <= y + hit_h) ||
            (!it->level_start && mx >= x && mx <= x + tw &&
             my >= y && my <= y + hit_h)) {
            if (out_idx != NULL) {
                *out_idx = i;
            }
            return 1;
        }
    }
    return 0;
}

int frontend_picker_active(const FrontEnd* fe)
{
    return fe != NULL && fe_picker_submenu(frontend_active_submenu(fe));
}

void frontend_picker_panel_text(FrontEnd* fe, const TextureSurface* measure,
                                void* pixels, int pitch, int width, int height)
{
    const FeSubmenu* sub;
    const FeSaveMenuLayout* layout;
    const char* prompt;
    void* font;
    void* buffer;
    float rect[4];
    uint32_t header[4];
    TextBufferDrawOps saved_ops;
    int hover = -1;
    int tutorial;
    int i;
    if (!frontend_picker_active(fe) || pixels == NULL) {
        if (fe != NULL) fe->level_panel_name[0] = 0;
        return;
    }
    sub = frontend_active_submenu(fe);
    layout = &fe->level_text_layout;
    tutorial = s_stristr(sub->image, "LevelPickT") != NULL;
    prompt = tutorial ? layout->tutorial : layout->level;
    if (fe->level_panel_menu != fe->menu || fe->level_panel_sub != fe->sub)
        fe->level_panel_name[0] = 0;
    fe->level_panel_menu = fe->menu;
    fe->level_panel_sub = fe->sub;
    if (layout->has_window) {
        rect[0] = (float)layout->window_x;
        rect[1] = (float)layout->window_y;
        rect[2] = (float)layout->window_w;
        rect[3] = (float)layout->window_h;
        memcpy(header, rect, sizeof(header));
        font = font_atlas_record_create("Interface\\Fonts\\FONT5_HI.bmp");
        buffer = font != NULL ? text_buffer_create((uintptr_t)font, header, 1024) : NULL;
        if (buffer != NULL) {
            text_buffer_append_format(buffer, "\n%s\n%s",
                                      ui_text_underscores_to_spaces(prompt),
                                      fe->level_panel_name);
            text_buffer_draw_ops_get(&saved_ops);
            text_buffer_draw_framebuffer_install(pixels, pitch, width, height);
            text_buffer_draw_wrapped(buffer, 0u, 0.0f, NULL);
            text_buffer_draw_ops_set(&saved_ops);
            text_buffer_destroy(buffer);
        }
        if (font != NULL) font_atlas_record_release(font);
    }
    fe->level_panel_name[0] = 0;
    if (fe_item_pick(fe, fe->pointer_x, fe->pointer_y, measure, &hover) &&
        hover >= 0 && sub->items[hover].level_start && sub->items[hover].level_enabled) {
        const char* target = sub->items[hover].target;
        int count = tutorial ? fe->tutorial_n : fe->campaign_n;
        for (i = 0; i < count; ++i) {
            const char* registry = tutorial ? fe->tutorial[i] : fe->campaign[i];
            const char* label = tutorial ? fe->tutorial_label[i] : fe->campaign_label[i];
            if (s_strcaseeq(registry, target)) {
                snprintf(fe->level_panel_name, sizeof(fe->level_panel_name), "%s",
                         label[0] != 0 ? ui_text_underscores_to_spaces(label)
                                       : registry);
                break;
            }
        }
    }
}

/* MenuHoverStatusTextDraw (FUN_00412750) runs once per MenuTick after input
 * selection has been updated. In this single-window software frontend the
 * active FeItem is the focused row; its status is the port-side text payload
 * where the native menu's type-6 child records are not available. */
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
    if (sub == NULL || item == NULL) return;
    const int save_load_menu = image != NULL &&
                               s_stristr(image, "SaveLoad") != NULL;

    if (save_load_menu && fe->save_text_layout.has_window) {
        const FeSaveMenuLayout* layout = &fe->save_text_layout;
        const char* prompt = fe->save_action ? layout->save : layout->load;
        char instruction[FE_SAVE_LAYOUT_TEXT_MAX];
        char progress[FE_SAVE_LAYOUT_TEXT_MAX];
        char normalized_progress[FE_SAVE_LAYOUT_TEXT_MAX];
        float rect[4];
        uint32_t header[4];
        TextBufferDrawOps saved_ops;
        void* text_font;
        void* buffer;
        int completed = 0;
        int total_levels = fe->campaign_n + fe->tutorial_n;
        int percent;
        int slot_index;
        for (slot_index = 0; slot_index < fe->slot_count; ++slot_index) {
            if (fe->slots[slot_index].id == item->load_slot) {
                completed = fe->slots[slot_index].completed;
                break;
            }
        }
        percent = total_levels > 0 ? (completed * 100) / total_levels : 0;
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        snprintf(instruction, sizeof(instruction), "%s",
                 ui_text_underscores_to_spaces(prompt));
        snprintf(progress, sizeof(progress), layout->slot, percent);
        snprintf(normalized_progress, sizeof(normalized_progress), "%s",
                 ui_text_underscores_to_spaces(progress));
        rect[0] = (float)layout->window_x;
        rect[1] = (float)layout->window_y;
        rect[2] = (float)layout->window_w;
        rect[3] = (float)layout->window_h;
        memcpy(header, rect, sizeof(header));
        /* SaveMenuLayoutLoad creates this window with FONT5_HI. */
        text_font = font_atlas_record_create("Interface\\Fonts\\FONT5_HI.bmp");
        buffer = text_font != NULL
                     ? text_buffer_create((uintptr_t)text_font, header, 512)
                     : NULL;
        if (buffer != NULL) {
            text_buffer_append_format(buffer, "\n%s\n%s", instruction,
                                      normalized_progress);
            text_buffer_draw_ops_get(&saved_ops);
            text_buffer_draw_framebuffer_install(px, pitch, w, h);
            text_buffer_draw_wrapped(buffer, 0u, 1.0f, NULL);
            text_buffer_draw_ops_set(&saved_ops);
            text_buffer_destroy(buffer);
        }
        if (text_font != NULL) font_atlas_record_release(text_font);
        return;
    }
    if (item->status[0] != '\0') {
        const char* status = ui_text_underscores_to_spaces(item->status);
        TextureSurface* status_font = texture_get_or_load(
            NULL, "Interface\\Fonts\\MBriefFONT2.bmp");
        const TextureSurface* text_font = status_font != NULL ? status_font : font;
        int text_w = text_font != NULL ? fe_font_text_width(status, text_font)
                                       : fe_text_width(status);
        if (text_font != NULL)
            formatted_text_draw(text_font, px, pitch, w, h, (w - text_w) / 2,
                                h - 41, status);
        else
            fe_draw_text(px, pitch, w, h, (w - text_w) / 2, h - 41,
                         status, 150, 255, 80);
        if (status_font != NULL) texture_release(status_font);
    }

}

/* InterfaceMessageTabInfoTextLoad (FUN_00419430): load the scrolling
 * message-tab panel surface and apply the shared resource-record setup. */
// FUNCTION: LEGORR 0x00419430
void interface_message_tab_info_text_load(const char* path)
{
    s_interface_message_tab_info_text = image_surface_load(path, 0u, 0u);
    resource_record_add_ref(s_interface_message_tab_info_text);
}

TextureSurface* interface_message_tab_info_text(void)
{
    return s_interface_message_tab_info_text;
}

static void fe_render_pass(FrontEnd* fe, const TextureSurface* hi_font,
                           const TextureSurface* lo_font,
                           int w, int h, void* px, int pitch, int pass,
                           int draw_cursor)
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
    for (int overlay_index = 0; pass != 2 && overlay_index < sub->overlay_count;
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

    if (pass != 2 && sub->display_title &&
        (sub->full_name[0] != '\0' || sub->title[0] != '\0')) {
        char title[FE_NAME_MAX];
        fe_display_label(sub->full_name[0] != '\0' ? sub->full_name
                                                    : sub->title,
                         title, sizeof(title));
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
        if (pass == 1 && !it->level_start) continue;
        if (pass == 2 && it->level_start) continue;
        int x, y;
        fe_item_pos(fe, it, item_index, w, hi_font, &x, &y);
        if (x < 0) {
            x = 8;
        }
        if (y < 0 && !(sub->can_scroll)) {
            y = 8;
        }
        if (sub->can_scroll && !fe_picker_submenu(sub) &&
            (y + 16 < 0 || y >= h)) {
            continue;   /* scrolled row: outside the plane window */
        }

        int selected = (item_index == fe->sel) &&
                       !(it->state_gated && system_state_flags_get() != 0u) &&
                       (!it->level_start || it->level_enabled);
        const TextureSurface* font = selected ? hi_font : lo_font;
        char label[FE_NAME_MAX];
        fe_display_label(it->label, label, sizeof(label));
        if (it->image_normal[0] != '\0') {
            const char* image_path = fe_item_image(it, selected);
            TextureSurface* image = texture_get_or_load(NULL, image_path);
            if (image != NULL) {
                if (it->level_start && fe_picker_submenu(sub)) {
                    fe_blit_menu_bitmap(px, pitch, w, h, image, x, y);
                } else {
                    surface_blit_texture(px, pitch, w, h, image, x, y);
                }
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
        /* Slider / Cycle widget. Slider artwork comes from the eight
         * MenuBuild resources retained by this item, in config order. */
        if (it->kind == FE_ITEM_SLIDER || it->kind == FE_ITEM_REALSLIDER) {
            int bx = sub->base_x + it->x + it->slider_x;
            int by = sub->base_y + it->y + it->slider_y;
            if (sub->can_scroll) {
                by += sub->scroll;
            }
            TextureSurface* off = it->slider_images[0];
            TextureSurface* on = it->slider_images[1];
            TextureSurface* left_cap = it->slider_images[2];
            TextureSurface* right_cap = it->slider_images[3];
            TextureSurface* plus = it->slider_images[4];
            TextureSurface* minus = it->slider_images[5];
            TextureSurface* plus_hi = it->slider_images[6];
            TextureSurface* minus_hi = it->slider_images[7];
            if (off != NULL) {
                surface_blit_texture(px, pitch, w, h, off, bx, by);
            }
            int frac = 0;
            if (it->max > it->min && off != NULL) {
                frac = (it->value - it->min) * off->width /
                       (it->max - it->min);
                if (frac < 0) frac = 0;
                if (frac > off->width) frac = off->width;
            }
            if (on != NULL && frac > 0) {
                fe_blit_texture_crop(px, pitch, w, h, on, 0, frac,
                                     on->height, bx, by);
            }
            if (left_cap != NULL) {
                surface_blit_texture(px, pitch, w, h, left_cap,
                                     bx - left_cap->width, by);
            }
            if (right_cap != NULL) {
                surface_blit_texture(px, pitch, w, h, right_cap,
                                     bx + 100, by);
            }
            int left_x = bx - (minus != NULL ? minus->width : 0) - 4;
            int right_x = bx + (right_cap != NULL ? right_cap->width : 0)
                          + 0x68;
            int arrow_y = by;
            int left_hot = menu_pointer_rectangle_hit_test(
                fe->pointer_x, fe->pointer_y, left_x, arrow_y,
                minus != NULL ? minus->width : 0,
                minus != NULL ? minus->height : 0);
            int right_hot = menu_pointer_rectangle_hit_test(
                fe->pointer_x, fe->pointer_y, right_x, arrow_y,
                plus != NULL ? plus->width : 0,
                plus != NULL ? plus->height : 0);
            if (minus != NULL) {
                TextureSurface* image = left_hot && minus_hi != NULL
                                           ? minus_hi : minus;
                surface_blit_texture(px, pitch, w, h, image, left_x,
                                     arrow_y);
            }
            if (plus != NULL) {
                TextureSurface* image = right_hot && plus_hi != NULL
                                           ? plus_hi : plus;
                surface_blit_texture(px, pitch, w, h, image, right_x,
                                     arrow_y);
            }
            if (off == NULL) {
                /* RealSlider and legacy configs without bitmap tuples keep
                 * the portable chrome they used before image-backed sliders. */
                int bw = 100;
                int fallback_by = by + 7;
                int frac = bw;
                if (it->max > it->min) {
                    frac = (it->value - it->min) * bw /
                           (it->max - it->min);
                }
                if (frac < 0) frac = 0;
                if (frac > bw) frac = bw;
                fe_fill_rect(px, pitch, w, h, bx, fallback_by, bw, 6,
                             36, 36, 40);
                fe_fill_rect(px, pitch, w, h, bx, fallback_by, frac, 6,
                             210, 180, 70);
                fe_fill_rect(px, pitch, w, h, bx + frac, fallback_by - 1,
                             3, 8, 255, 255, 255);
            }
        } else if (it->kind == FE_ITEM_CYCLE && it->cycle_count > 0) {
            int oi = it->value < it->cycle_count ? it->value : 0;
            const char* opt = it->cycle_opts[oi];
            if (font != NULL) {
                formatted_text_draw(font, px, pitch, w, h, x + it->cycle_x,
                                    y + it->cycle_y, opt);
            } else {
                fe_draw_text(px, pitch, w, h, x + it->cycle_x,
                             y + it->cycle_y, opt,
                             selected ? 255 : 200, selected ? 255 : 200,
                             selected ? 255 : 160);
            }
        }
    }

    if (pass == 1) return;

    /* Pointer_Standard is the shipped Aclosed.bmp cursor from Lego.cfg's
     * Pointers block. Its red tip is the top-left hotspot. */
    if (draw_cursor && fe->pointer_x >= 0 && fe->pointer_y >= 0
        && fe->pointer_x < w && fe->pointer_y < h) {
        char key[128];
        snprintf(key, sizeof(key), "%s::Pointers::Pointer_Standard",
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

void frontend_render(FrontEnd* fe, const TextureSurface* hi_font,
                     const TextureSurface* lo_font,
                     int w, int h, void* px, int pitch)
{
    fe_render_pass(fe, hi_font, lo_font, w, h, px, pitch, 0, 1);
}

void frontend_render_picker_pass(FrontEnd* fe, const TextureSurface* hi_font,
                                 const TextureSurface* lo_font,
                                 int w, int h, void* px, int pitch, int foreground)
{
    fe_render_pass(fe, hi_font, lo_font, w, h, px, pitch,
                   foreground ? 2 : 1, 1);
}

void frontend_render_menu_pass(FrontEnd* fe, const TextureSurface* hi_font,
                               const TextureSurface* lo_font, int w, int h,
                               void* px, int pitch, int pass, int draw_cursor)
{
    fe_render_pass(fe, hi_font, lo_font, w, h, px, pitch, pass, draw_cursor);
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

/* The corpus formats `<CreditsTextFile>.cconv`, falls back to the configured
 * file only when that open fails, counts CRLF pairs, and uses the shared
 * delimiter splitter. Keeping the source buffer alive also keeps each row a
 * zero-copy pointer into the single allocation, as in the original. */
int frontend_credits_lines_load(const char* path, char*** lines_out,
                                int* count_out)
{
    if (lines_out == NULL || count_out == NULL) {
        return 0;
    }
    *lines_out = NULL;
    *count_out = 0;
    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    char sidecar[1024];
    Resource* resource = NULL;
    const int sidecar_length = snprintf(sidecar, sizeof(sidecar), "%s.cconv",
                                        path);
    if (sidecar_length > 0 && (size_t)sidecar_length < sizeof(sidecar)) {
        resource = resource_open(sidecar, RESOURCE_AUTO);
    }
    if (resource == NULL) {
        resource = resource_open(path, RESOURCE_AUTO);
    }
    if (resource == NULL) {
        return 0;
    }

    int32_t size = resource_read(resource);
    if (size <= 0) {
        resource_free(resource);
        return 0;
    }
    char* text = (char*)malloc((size_t)size + 1);
    if (text == NULL) {
        resource_free(resource);
        return 0;
    }
    memcpy(text, resource->data, (size_t)size);
    text[size] = '\0';
    resource_free(resource);

    int count = 1;
    int32_t i;
    for (i = 0; i + 1 < size; ++i) {
        if (text[i] == '\r' && text[i + 1] == '\n') {
            ++count;
        }
    }
    char** lines = (char**)malloc((size_t)count * sizeof(*lines));
    if (lines == NULL) {
        free(text);
        return 0;
    }
    int line = 0;
    lines[line++] = text;
    for (i = 0; i + 1 < size; ++i) {
        if (text[i] == '\r' && text[i + 1] == '\n') {
            text[i] = '\0';
            text[i + 1] = '\0';
            lines[line++] = text + i + 2;
            ++i;
        }
    }
    *lines_out = lines;
    *count_out = line;
    return 1;
}

void frontend_credits_lines_free(char** lines, int count)
{
    (void)count;
    if (lines != NULL) {
        free(lines[0]);
        free(lines);
    }
}

/* Advance the credits' 33 ms scroll clock by at most one pixel. */
void frontend_credits_scroll_step(uint32_t now, uint32_t* clock,
                                  int* scroll_px)
{
    if (clock == NULL || scroll_px == NULL || now - *clock <= 0x21u) {
        return;
    }
    ++*scroll_px;
    *clock += 0x21u;
    if (*clock < now) {
        *clock = now;
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
 * index blindly; the explicit slot setters and diagnostics guard their
 * 16-entry windows (index * 7 dwords). */
#define SEL_HANDLE_ENTRIES 16
#define SEL_HANDLE_STRIDE 7 /* dwords per 0x1c-stride entry */

// GLOBAL: LEGORR 0x004dc650
static uint32_t s_sel_state;                    /* DAT_004dc650 */
// GLOBAL: LEGORR 0x004dc648
static int      s_sel_hit_index;                /* DAT_004dc648 */
// GLOBAL: LEGORR 0x004db914
static uint32_t s_sel_handles_a[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
// GLOBAL: LEGORR 0x004db918
static uint32_t s_sel_handles_b[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
// GLOBAL: LEGORR 0x004db934
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
        if ((s_sel_state & 0x20u) != 0) {
            return s_sel_handles_b[index * SEL_HANDLE_STRIDE];
        }
        return s_sel_handles_a[index * SEL_HANDLE_STRIDE];
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
 * state-bit gates). The timed-effect state uses the full 0x27c-stride
 * records: the timestamp, cue, flags, attached object, and image list share
 * the corpus table at DAT_0054c000. */
#define TIMED_EFFECT_ENTRIES 40
#define TIMED_EFFECT_RECORD_COUNT 40
#define LAYOUT_POSITION_ENTRIES 2   /* corpus: two help-window buttons */
#define LAYOUT_POSITION_STRIDE 7    /* corpus advances 0x1c bytes per entry */

// GLOBAL: LEGORR 0x004dc64c
static int      s_band_mode;
/* Corpus records span DAT_0054c000..DAT_0055235f at 0x27c-byte strides;
 * the update walk's exclusive flag bound is DAT_0055235c (record 39's
 * +0x278 field), so the timestamp band has 40 records. Tooltip overlay
 * initialization clears 0x1839 dwords: 39 full records plus the first
 * three words of record 39, leaving its update-bound flag intact. */
// GLOBAL: LEGORR 0x0054c000
static TimedEffectRecord s_timed_effect_records[TIMED_EFFECT_RECORD_COUNT];
/* DAT_0054bf3c is the tooltip duration threshold (scale * 25). Timed-effect
 * timestamp reset uses these same float bits as its fallback time value. */
// GLOBAL: LEGORR 0x0054bf3c
static float s_timed_effect_tick;
typedef union HelpWindowLayoutWord {
    uint32_t u;
    int32_t i;
    float f;
} HelpWindowLayoutWord;
// GLOBAL: LEGORR 0x004db904
static HelpWindowLayoutWord s_layout_position[LAYOUT_POSITION_ENTRIES *
                                              LAYOUT_POSITION_STRIDE];
static const char* s_layout_position_text[LAYOUT_POSITION_ENTRIES];
static char s_layout_position_text_storage[LAYOUT_POSITION_ENTRIES][256];
static int32_t s_layout_position_debug[LAYOUT_POSITION_ENTRIES][2];
static float s_help_window_button_rects[LAYOUT_POSITION_ENTRIES][4];
static UiModeResourceBlitRectHook s_ui_resource_reposition_hook;
static float s_help_background_offset_x;
static float s_help_background_offset_y;
// GLOBAL: LEGORR 0x004db8f8
static uint32_t s_help_background_handle;
// GLOBAL: LEGORR 0x004db974
static uintptr_t s_help_text_area_handle;
static void* s_help_text_area_native_record;
static char s_help_window_labels[3][256];
static char s_tooltip_info_strings[9][256];
/* HelpWindowInfoConfigStringsLoad fills three adjacent 15-by-16 pointer
 * tables. The catalog loader owns the name tables/counts; module-owned text
 * storage below supplies the mutable copies for these pointer rows. */
// GLOBAL: LEGORR 0x004db978
static const char* s_help_info_strings[3][15 * 16];
/* ConfigStringCopy in the retail loader gives each HelpWindowInfo line its
 * own mutable string before replacing underscores. Keep those copies in
 * module-owned storage so this loader does not modify BlockFile-owned text. */
static char s_help_info_string_storage[3][15 * 16][1024];
static TooltipOverlayState s_tooltip_overlay_state;
static AdvisorTutorialIconLoadFn s_tutorial_icon_load;
static AdvisorTutorialIconInitializeFn s_tutorial_icon_initialize;
static void (*s_tutorial_icon_release)(void* icon, void* context);
static void* s_tutorial_icon_context;
static void* s_tutorial_icon;
static AdvisorTextRequestActionFn s_advisor_text_request_action;
static void* s_advisor_text_request_context;
static uint32_t s_advisor_text_state_flags;
static int s_advisor_text_current_slot = 0x1a;
static float s_advisor_text_animation_timer;
static float s_advisor_text_animation_reset_timer;
static const char* s_advisor_text_request_source;
static AdvisorTextAnimationOps s_advisor_text_animation_ops;
static AdvisorTextBoxExtentFn s_advisor_text_box_extent_hook;
static void* s_advisor_text_box_extent_context;
static AdvisorTextDisplayOps s_advisor_text_display_ops;
static AdvisorInlineTokenOps s_advisor_inline_token_ops;
static uint32_t s_advisor_inline_last_sound = 0xffffffffu;
static uint32_t s_advisor_inline_token_generation = 0xffffffffu;
static float s_advisor_inline_duration;
static SelectedGroundMarkerOps s_selected_ground_marker_ops;
static PositionTrackerBoxDrawOps s_position_tracker_box_draw_ops;
static OverlayTextBufferOps s_overlay_text_buffer_ops;
static AdvisorDialogStackOps s_advisor_dialog_stack_ops;
static uint32_t s_advisor_dialog_stack_width = 0xffffffffu;
static uint32_t s_advisor_dialog_stack_height = 0xffffffffu;
static DebugPanelTextOps s_debug_panel_text_ops;
static float s_debug_panel_text_timer;
static int s_debug_panel_text_alternate;
static InterfaceSpawnRequestOps s_interface_spawn_request_ops;
static InterfaceSpawnRequestState s_interface_spawn_request_state;
// GLOBAL: LEGORR 0x004dc640
static float s_help_window_feedback_position;
// GLOBAL: LEGORR 0x004dc644
static float s_help_window_feedback_limit;
static void (*s_help_window_sound_clear)(int);
static void (*s_help_render_transition_clear)(int, int);

// FUNCTION: LEGORR 0x0046b770
void timed_effect_update_request_set_corpus(int index)
{
    s_timed_effect_records[index].flags |= 4u;
}

void timed_effect_update_request_set(int index)
{
    if (index >= 0 && index < TIMED_EFFECT_ENTRIES)
        timed_effect_update_request_set_corpus(index);
}

/* ToolTip_SetContent adapter (frontend.h). The original (FUN_0046b520)
 * formats into a 512-byte buffer and either loads an image ('@' prefix) or
 * copies the text into the record, turning "\n" into a newline and '_'
 * into a space, then measures it and arms the record (flags |= 3). This
 * port keeps the text path only: the image pointer slot is 32-bit in the
 * record layout, and the text measurement (FUN_00401c80) has no owner. */
void tooltip_set_content(int index, const char* format, ...)
{
    TimedEffectRecord* record;
    char text[512];
    const char* in;
    char* out;
    char* end;
    va_list args;
    if (index < 0 || index >= TIMED_EFFECT_RECORD_COUNT || format == NULL)
        return;
    record = &s_timed_effect_records[index];
    va_start(args, format);
    formatted_text_buffer_build(text, sizeof(text), format, args);
    va_end(args);
    if (text[0] == '@') return;
    out = (char*)record->payload;
    end = out + 0x1ff;
    for (in = text; *in != '\0' && out < end; ++in, ++out) {
        if (in[0] == '\\' && in[1] == 'n') {
            *out = '\n';
            ++in;
        }
        else if (*in == '_') *out = ' ';
        else *out = *in;
    }
    *out = '\0';
    memset(record->payload + 0x258, 0, 0x14);
    record->flags = (record->flags & ~8u) | 3u;
}

// FUNCTION: LEGORR 0x0046b700
static void timed_effect_cue_request_set_corpus(int index, uint32_t value)
{
    s_timed_effect_records[index].cue = value;
    s_timed_effect_records[index].flags |= 3u;
}

void timed_effect_cue_request_set(int index, uint32_t value)
{
    if (index >= 0 && index < TIMED_EFFECT_ENTRIES)
        timed_effect_cue_request_set_corpus(index, value);
}

uint32_t timed_effect_cue_peek(int index)
{
    return (index >= 0 && index < TIMED_EFFECT_ENTRIES)
        ? s_timed_effect_records[index].cue : 0u;
}

uint32_t timed_effect_flags_peek(int index)
{
    if (index < 0 || index >= TIMED_EFFECT_ENTRIES) {
        return 0u;
    }
    return s_timed_effect_records[index].flags;
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
        return s_band_mode == index;
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
    float* position_ptr = (float*)((char*)s_layout_position + sizeof(uint32_t));
    int i = 0;
    do {
        float position[2];
        uint32_t handle;
        position[0] = position_ptr[-1];
        position[1] = position_ptr[0];
        handle = selection_state_handle_get(i);
        if (handle != 0u) {
            if (s_ui_resource_reposition_hook != NULL) {
                /* SurfaceBlitRect consumes the raw pair and performs the
                 * floor/__ftol conversions itself. */
                (void)s_ui_resource_reposition_hook(handle, NULL, position,
                                                    NULL);
            }
        }
        position_ptr += LAYOUT_POSITION_STRIDE;
        ++i;
    } while ((char*)position_ptr <
             (char*)s_layout_position + sizeof(s_layout_position) +
                 sizeof(uint32_t));
}

void ui_mode_resource_reposition_hook_set(
    UiModeResourceBlitRectHook hook)
{
    s_ui_resource_reposition_hook = hook;
}

// FUNCTION: LEGORR 0x00419030
void help_window_button_feedback_update(int action, float amount)
{
    switch (action) {
    case 0:
        help_render_transition_cleanup_if_active();
        return;
    case 1:
        ui_mode_resource_bit40_set_clear(1, 0u);
        help_render_transition_cleanup_if_active();
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
                objective_score_text_origin_set(NULL);
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
        const char* separator = strchr(cursor, ':');
        if (separator == NULL || separator == cursor) {
            return 0;
        }
        const int parsed = atoi(cursor);
        *dimensions[i] = parsed;
        cursor = i < 3 ? separator + 1 : separator;
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
    const size_t copy_limit = out_size - 1;
    const char* terminator = (const char*)memchr(value, '\0', copy_limit);
    size_t length = terminator != NULL
        ? (size_t)(terminator - value) : copy_limit;
    strncpy(out, value, length);
    out[length] = '\0';
    char* underscore = out;
    while ((underscore = crt_strchr(underscore, '_')) != NULL) {
        *underscore++ = ' ';
    }
}

/* The HelpWindowInfo key has a fixed prefix/suffix and a one- or two-digit
 * line number. Build it with bounded copies so this path remains usable by
 * the VC5 toolchain as well as the native build. */
static int help_window_info_path_build(char* out, size_t out_size,
                                       const char* catalog, int line_index)
{
    static const char prefix[] = "HelpWindow::HelpWindowInfo::";
    static const char suffix[] = "L";
    size_t prefix_size = sizeof(prefix) - 1u;
    size_t catalog_size;
    size_t number_size;
    size_t needed;
    size_t cursor;
    if (out == NULL || catalog == NULL || line_index < 0 ||
        line_index > 99) return 0;
    catalog_size = strlen(catalog);
    number_size = line_index >= 10 ? 2u : 1u;
    needed = prefix_size + catalog_size + 1u + number_size + 1u;
    if (needed > out_size) return 0;
    memcpy(out, prefix, prefix_size);
    cursor = prefix_size;
    memcpy(out + cursor, catalog, catalog_size);
    cursor += catalog_size;
    memcpy(out + cursor, suffix, 1u);
    cursor += 1u;
    if (number_size == 2u) {
        out[cursor++] = (char)('0' + line_index / 10);
        line_index %= 10;
    }
    out[cursor++] = (char)('0' + line_index);
    out[cursor] = '\0';
    return 1;
}

static void help_window_info_config_strings_load_native(BlockFile* cfg)
{
    static const int catalog_kinds[3] = { 0, 1, 3 };
    int group;
    memset(s_help_info_strings, 0, sizeof(s_help_info_strings));
    for (group = 0; group < 3; ++group) {
        const int kind = catalog_kinds[group];
        const char* const* names = (const char* const*)
            catalog_name_table_peek(kind);
        uint32_t count = catalog_count_peek(kind);
        uint32_t catalog_index;
        if (count > 15u) count = 15u;
        if (names == NULL) continue;
        for (catalog_index = 0; catalog_index < count; ++catalog_index) {
            int line_index;
            for (line_index = 0; line_index < 16; ++line_index) {
                char path[1024];
                const char* value = NULL;
                const char* catalog = names[catalog_index];
                if (help_window_info_path_build(path, sizeof(path), catalog,
                                                line_index)) {
                    value = block_get_strptr(cfg, path);
                }
                if (value != NULL) {
                    char* copy = s_help_info_string_storage[group]
                        [catalog_index * 16u + (uint32_t)line_index];
                    help_window_label_copy(copy, 1024u, value);
                    s_help_info_strings[group][catalog_index * 16u +
                        (uint32_t)line_index] = copy;
                }
            }
        }
    }
}

/* ToolTipInfoStringsLoad (FUN_00427e00) loads the nine fixed labels from
 * the ToolTipInfo group. The corpus heap-copies each value into a global;
 * this model keeps bounded module-owned copies so they survive block_close. */
// FUNCTION: LEGORR 0x00427e00
void tooltip_info_strings_load(BlockFile* cfg)
{
    static const char* const paths[9] = {
        "ToolTipInfo::HealthText",
        "ToolTipInfo::EnergyText",
        "ToolTipInfo::CrystalsText",
        "ToolTipInfo::OreText",
        "ToolTipInfo::StudsText",
        "ToolTipInfo::ToolsText",
        "ToolTipInfo::CarryObjectText",
        "ToolTipInfo::DrivenByText",
        "ToolTipInfo::OreRequiredText"
    };
    int i;
    for (i = 0; i < 9; ++i) {
        s_tooltip_info_strings[i][0] = '\0';
        (void)block_get_str(cfg, paths[i], s_tooltip_info_strings[i],
                            sizeof(s_tooltip_info_strings[i]));
    }
}

void advisor_tutorial_icon_hooks_set(AdvisorTutorialIconLoadFn load,
                                     AdvisorTutorialIconInitializeFn initialize,
                                     void* context)
{
    advisor_tutorial_icon_image_release();
    s_tutorial_icon_load = load;
    s_tutorial_icon_initialize = initialize;
    s_tutorial_icon_release = NULL; /* Legacy hooks lend their images. */
    s_tutorial_icon_context = context;
}

void advisor_tutorial_icon_image_release(void)
{
    if (s_tutorial_icon != NULL && s_tutorial_icon_release != NULL)
        s_tutorial_icon_release(s_tutorial_icon, s_tutorial_icon_context);
    s_tutorial_icon = NULL;
}

static void* tutorial_icon_surface_load(const char* path, void* context)
{
    (void)context;
    return image_surface_load(path, 0u, 0u);
}

static void tutorial_icon_surface_initialize(void* icon, void* context)
{
    (void)context;
    image_surface_blit_transformed((TextureSurface*)icon,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

static void tutorial_icon_surface_release(void* icon, void* context)
{
    (void)context;
    texture_release((TextureSurface*)icon);
}

void advisor_tutorial_icon_runtime_initialize(BlockFile* cfg)
{
    advisor_tutorial_icon_hooks_set(tutorial_icon_surface_load,
                                    tutorial_icon_surface_initialize, NULL);
    s_tutorial_icon_release = tutorial_icon_surface_release;
    advisor_tutorial_icon_image_load(cfg);
}

/* AdvisorTutorialIconImageLoad (FUN_00434a00): root/Main/TutorialIcon
 * selects the authored bitmap. Production owns one texture reference. */
// FUNCTION: LEGORR 0x00434a00
void advisor_tutorial_icon_image_load(BlockFile* cfg)
{
    const char* icon_path;
    void* icon;
    advisor_tutorial_icon_image_release();
    if (cfg == NULL || s_tutorial_icon_load == NULL) return;
    icon_path = block_get_strptr(cfg, string_path_join(util_root_name(cfg),
        "Main", "TutorialIcon", (char*)NULL));
    if (icon_path == NULL) return;
    icon = s_tutorial_icon_load(icon_path, s_tutorial_icon_context);
    if (icon == NULL) return;
    s_tutorial_icon = icon;
    if (s_tutorial_icon_initialize != NULL) {
        s_tutorial_icon_initialize(icon, s_tutorial_icon_context);
    }
}

void* advisor_tutorial_icon_get(void)
{
    return s_tutorial_icon;
}

/* GameConfigInit (0x4209e0..0x420cc2) owns these configured surfaces;
 * the reference message widget geometry is installed at 0x4201f5. */
static TextureSurface* s_message_next_image;
static TextureSurface* s_message_back_image;
static TextureSurface* s_message_panel_image;
static TextureSurface* s_message_air_image;
static TextureSurface* s_message_noair_image;
static float s_message_next_position[2];
static float s_message_back_position[2];
static void* s_message_font;
static void* s_message_text_buffers[2];
static AdvisorMessagePointerCapture s_message_pointer_capture;
static int s_message_gameplay_active;
static float s_message_panel_y = 409.0f;
static float s_message_text_y = 435.0f;
static float s_message_text_height = 42.0f;

/* 0x46ad00 grows/shrinks by the actual font line after wrapped output. */
static void advisor_message_layout_commit(void)
{
    if (s_message_text_buffers[0] == NULL) return;
    float_pair_at_offset4_set_from_ints(s_message_text_buffers[0],
                                       55, (int)s_message_text_y);
    float_pair_at_offsetc_set_from_uints(s_message_text_buffers[0],
                                        325u, (uint32_t)s_message_text_height);
}

static void advisor_message_layout_reset(void)
{
    s_message_panel_y = 409.0f;
    s_message_text_y = 435.0f;
    s_message_text_height = 42.0f;
    s_advisor_text_state_flags &= ~1u;
    advisor_message_layout_commit();
}

static void advisor_message_layout_result(void* output, int overflow,
                                          int bottom)
{
    int line;
    if (output == NULL || output != s_message_text_buffers[0] ||
        s_message_font == NULL) return;
    line = font_atlas_record_line_height_get(s_message_font);
    if (line <= 0) return;
    if (overflow) {
        s_message_panel_y -= (float)line;
        s_message_text_y -= (float)line;
        s_message_text_height += (float)line;
        s_advisor_text_state_flags &= ~1u;
        advisor_message_layout_commit();
    } else if ((s_advisor_text_state_flags & 1u) != 0u) {
        /* The reference compares the last drawn baseline with the original
         * text bottom minus one font line before contracting a new message. */
        if (435.0f + 42.0f - (float)line <= (float)bottom) {
            s_advisor_text_state_flags &= ~1u;
        } else {
            s_message_panel_y += (float)line;
            s_message_text_y += (float)line;
            s_message_text_height -= (float)line;
            if (s_message_panel_y >= 409.0f) {
                advisor_message_layout_reset();
                s_advisor_text_state_flags &= ~1u;
            } else advisor_message_layout_commit();
        }
    }
}

/* Return a transition so the real input owner can discard inherited holds
 * once; reward controls retain their subsequent fresh input. */
int advisor_message_ui_gameplay_set(int active)
{
    int changed;
    active = active != 0;
    changed = active != s_message_gameplay_active;
    if (changed) advisor_message_pointer_cancel(0);
    s_message_gameplay_active = active;
    return changed;
}

void advisor_message_source_clear(void)
{
    s_advisor_text_request_source = NULL;
    s_advisor_text_state_flags = 0u;
    advisor_message_layout_reset();
    engine_overlay_words_set(0u, 0u);
    engine_global_state_flags_bits6_and_9_set_target(0, 0);
    engine_global_state_flags_bit7_set_target(0);
    advisor_message_pointer_cancel(0);
}

void advisor_message_pointer_cancel(int held)
{
    /* A modal/lifetime transition cannot turn an inherited hold into NEXT. */
    memset(&s_message_pointer_capture, 0, sizeof(s_message_pointer_capture));
    s_message_pointer_capture.down = held != 0;
}

void advisor_message_ui_release(void)
{
    int i;
    advisor_message_source_clear();
    s_message_gameplay_active = 0;
    ui_message_strings_initialize(NULL, NULL, NULL, NULL, 0.0f);
    for (i = 0; i < 2; ++i) {
        if (s_message_text_buffers[i] != NULL)
            text_buffer_destroy(s_message_text_buffers[i]);
        s_message_text_buffers[i] = NULL;
    }
    if (s_message_font != NULL) font_atlas_record_release(s_message_font);
    s_message_font = NULL;
    texture_release(s_message_next_image);
    texture_release(s_message_back_image);
    texture_release(s_message_panel_image);
    texture_release(s_message_air_image);
    texture_release(s_message_noair_image);
    s_message_next_image = NULL;
    s_message_back_image = NULL;
    s_message_panel_image = NULL;
    s_message_air_image = NULL;
    s_message_noair_image = NULL;
    deferred_callback_set(0u);
}

void advisor_message_ui_initialize(BlockFile* cfg, int width, int height)
{
    char key[256];
    const char* path;
    const char* position;
    int i;
    /* GameConfigInit uses (55,435,325,42) and (55,435,260,42),
     * FONT5_HI, capacity 0x400. These are shared UI geometry, not level text. */
    float rect[4] = {55.0f, 435.0f, 325.0f, 42.0f};
    uint32_t header[4];
    advisor_message_ui_release();
    if (cfg == NULL) return;
    s_message_font = font_atlas_record_create("Interface\\Fonts\\font5_HI.bmp");
    for (i = 0; i < 2; ++i) {
        rect[2] = i == 0 ? 325.0f : 260.0f;
        memcpy(header, rect, sizeof(header));
        if (s_message_font != NULL)
            s_message_text_buffers[i] = text_buffer_create(
                (uintptr_t)s_message_font, header, 0x400);
    }
    ui_message_strings_initialize(s_message_text_buffers[0],
        s_message_text_buffers[1], (void*)(uintptr_t)316,
        (void*)(uintptr_t)425, 0.0f);
    text_buffer_draw_software_install();
    s_message_panel_image = image_surface_load(
        "Interface\\MessagePanel\\MsgPanel.bmp", 0u, 0u);
    image_surface_blit_transformed(s_message_panel_image,
                                  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    /* GameConfigInit 0x4225a3 owns the real air-meter resources. */
    s_message_air_image = image_surface_load(
        "Interface\\AirMeter\\msgpanel_air_juice.bmp", 0u, 0u);
    s_message_noair_image = image_surface_load(
        "Interface\\AirMeter\\msgpanel_noair.bmp", 0u, 0u);
    image_surface_blit_transformed(s_message_noair_image,
                                  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    snprintf(key, sizeof(key), "%s::Main::NextButton%ix%i",
             util_root_name(cfg), width, height);
    path = block_get_strptr(cfg, key);
    snprintf(key, sizeof(key), "%s::Main::NextButtonPos%ix%i",
             util_root_name(cfg), width, height);
    position = block_get_strptr(cfg, key);
    if (path != NULL && position != NULL &&
        sscanf(position, "%f,%f", &s_message_next_position[0],
               &s_message_next_position[1]) == 2) {
        s_message_next_image = image_surface_load(path, 0u, 0u);
        texture_source_color_key_set(s_message_next_image, 0u, 0u);
    }
    snprintf(key, sizeof(key), "%s::Main::BackButton%ix%i",
             util_root_name(cfg), width, height);
    path = block_get_strptr(cfg, key);
    snprintf(key, sizeof(key), "%s::Main::BackButtonPos%ix%i",
             util_root_name(cfg), width, height);
    position = block_get_strptr(cfg, key);
    if (path != NULL && position != NULL &&
        sscanf(position, "%f,%f", &s_message_back_position[0],
               &s_message_back_position[1]) == 2) {
        s_message_back_image = image_surface_load(path, 0u, 0u);
        texture_source_color_key_set(s_message_back_image, 0u, 0u);
    }
    deferred_callback_set(s_message_next_image != NULL);
}

/* MainSim's local_2c comes from 0x45b310 -> 0x45a3b0 -> 0x47dfa0:
 * an enabled bitmap panel must contain a nonzero pixel at the current pose.
 * Test the SAME decoded resource and placement used by this message draw,
 * including its black source key, rather than claiming an empty rectangle. */
static int advisor_message_panel_hit(int x, int y)
{
    int image_x = x - 42;
    int image_y = y - (int)s_message_panel_y;
    tutorial_message_generation_sync();
    if (!s_message_gameplay_active || s_advisor_text_request_source == NULL ||
        engine_overlay_word_peek(0) == 0u || s_message_panel_image == NULL ||
        s_message_panel_image->pixels == NULL || image_x < 0 || image_y < 0 ||
        image_x >= s_message_panel_image->width ||
        image_y >= s_message_panel_image->height) return 0;
    return (s_message_panel_image->pixels[
        image_y * s_message_panel_image->width + image_x] & 0x00ffffffu) != 0u;
}

static int advisor_message_button_visible(int button)
{
    uint32_t flags;
    tutorial_message_generation_sync();
    flags = engine_debug_word_peek();
    return button == 1 ? s_message_next_image != NULL && (flags & 0x40u) != 0u
                       : s_message_back_image != NULL && (flags & 0x80u) != 0u;
}

static int advisor_message_button_hit(int button, int x, int y)
{
    TextureSurface* image = button == 1 ? s_message_next_image : s_message_back_image;
    const float* position = button == 1 ? s_message_next_position : s_message_back_position;
    if (!advisor_message_panel_hit(x, y) ||
        !advisor_message_button_visible(button)) return 0;
    /* 0x427c2a..0x427c8e uses strict bounds and loaded image dimensions. */
    return position[0] < (float)x && position[1] < (float)y &&
        (float)x < position[0] + (float)image->width &&
        (float)y < position[1] + (float)image->height;
}

/* Port adapter for the reference release-edge caller at 0x427ca2.
 * A real control claims DOWN through UP; motion onto UI during a world
 * hold cannot acquire it. No rectangle exists without the configured image. */
int advisor_message_pointer_capture_dispatch(AdvisorMessagePointerCapture* capture,
    int x, int y, int down)
{
    int consumed;
    int hit;
    int panel_hit = advisor_message_panel_hit(x, y);
    uint32_t generation = objective_advisor_token_generation_get();
    if (capture == NULL) return 0;
    if (!s_message_gameplay_active) {
        memset(capture, 0, sizeof(*capture));
        capture->down = down != 0;
        return 0;
    }
    consumed = capture->owner != 0;
    hit = advisor_message_button_hit(1, x, y) ? 1 :
              (advisor_message_button_hit(2, x, y) ? 2 : 0);
    if (!capture->down && down) {
        capture->owner = hit != 0 ? hit : (panel_hit ? 3 : 0);
        capture->generation = generation;
        consumed = capture->owner != 0;
    } else if (capture->down && !down) {
        int owner = capture->owner;
        capture->owner = 0;
        if (capture->generation == generation) {
            if (owner == 1 && hit == 1) tutorial_advisor_scroll_step_advance();
            else if (owner == 2 && hit == 2 && deferred_callback_peek() != 0u)
                advisor_sound_alias_play_global(); /* 0x453910 -> 0x456810 */
            else if (owner != 0 && panel_hit)
                media_action_feedback_sound_dispatch(1);
        }
    }
    if (hit != 0) timed_effect_update_request_set(hit == 1 ? 0x20 : 0x1f);
    capture->down = down != 0;
    return consumed;
}

int advisor_message_pointer_sample(int x, int y, int down)
{
    return advisor_message_pointer_capture_dispatch(&s_message_pointer_capture,
                                                     x, y, down);
}

/* The air texture is an 8-pixel-wide authored strip. SurfaceBlitRect
 * stretches it to the requested meter width, preserving its own height. */
static void advisor_message_air_blit(void* pixels, int pitch, int width,
    int height, const TextureSurface* image, int x, int y, int draw_width)
{
    int yy;
    if (pixels == NULL || image == NULL || image->pixels == NULL ||
        image->width <= 0 || image->height <= 0 || draw_width <= 0) return;
    for (yy = 0; yy < image->height; ++yy) {
        int dy = y + yy;
        int xx;
        unsigned char* row;
        if (dy < 0 || dy >= height) continue;
        row = (unsigned char*)pixels + (size_t)dy * pitch;
        for (xx = 0; xx < draw_width; ++xx) {
            int dx = x + xx;
            uint32_t color;
            if (dx < 0 || dx >= width) continue;
            color = image->pixels[yy * image->width +
                                  xx * image->width / draw_width];
            row[dx * 4] = (unsigned char)(color >> 16);
            row[dx * 4 + 1] = (unsigned char)(color >> 8);
            row[dx * 4 + 2] = (unsigned char)color;
            row[dx * 4 + 3] = (unsigned char)(color >> 24);
        }
    }
}

/* AdvisorTextDisplayWorker owns category 3 presentation after any layout
 * reset and before text. The shipped category has no button records. These
 * are the actual GameConfigInit surfaces, not the diagnostic blit ledger. */
static void advisor_message_interface_finish(void* context,
    const AdvisorTextDisplayState* state, float delta)
{
    DisplaySurface* target = surface_active_render_target_get();
    LevelState* level = level_current_owner_get();
    void* pixels = NULL;
    int pitch = 0;
    float position[2];
    uint32_t position_bits[2];
    (void)context;
    (void)delta;
    if (state == NULL) return;
    if (((state->interface_flags & 0x100000u) != 0u &&
         state->current_slot != 0x16) ||
        state->hidden_slot)
        advisor_message_layout_reset();
    position[0] = 42.0f;
    position[1] = s_message_panel_y;
    memcpy(position_bits, position, sizeof(position_bits));
    indexed_pair_set(3, position_bits[0], position_bits[1]);
    if (target == NULL || target->w <= 0 || target->h <= 0 ||
        surface_lock(target, &pixels, &pitch) != 0) return;
    surface_blit_texture(pixels, pitch, target->w, target->h,
                         s_message_panel_image, (int)position[0],
                         (int)position[1]);
    if (level != NULL && s_message_air_image != NULL) {
        float percent = level_float(level, LEVEL_OFF_AIR_SUPPLY);
        if (percent > 0.0f) {
            int x = (int)position[0] + 85;
            int y = (int)position[1] + 6;
            int draw_width;
            if (percent > 100.0f) percent = 100.0f;
            draw_width = (int)crt_float_to_long((float)
                ((double)236.0f * (double)(float)0.01f * (double)percent));
            advisor_message_air_blit(pixels, pitch, target->w, target->h,
                                    s_message_air_image, x, y, draw_width);
        }
    }
    if (s_message_noair_image != NULL &&
        global_value_0050080c_peek() != 0u &&
        sim_status_banner_word_get() != 0u)
        surface_blit_texture(pixels, pitch, target->w, target->h,
            s_message_noair_image, (int)position[0] + 21, (int)position[1]);
    surface_unlock(target, pixels);
}

void advisor_message_ui_draw(float delta)
{
    DisplaySurface* target = surface_active_render_target_get();
    const uint32_t* engine_state = engine_current_state_word();
    void* pixels = NULL;
    int pitch = 0;
    tutorial_message_generation_sync();
    if (!s_message_gameplay_active || target == NULL) return;
    /* The portable frame draws messages without the indexed manager block
     * that normally calls 0x41b930. Advance only its shared flash clock here
     * when that owner is absent; otherwise NEXT stays in the off phase.
     * delta is already in the reference's 25-Hz units at this draw seam. */
    if (engine_state == NULL ||
        (*engine_state & ENGINE_STATE_INDEXED_SUBSYS) == 0u) {
        if (engine_state == NULL ||
            (*engine_state & ENGINE_STATE_TIME_FROZEN) == 0u)
            frontend_status_flash_tick(delta);
    }
    advisor_text_display_worker(delta);
    if (surface_lock(target, &pixels, &pitch) == 0) {
        if (advisor_message_button_visible(1) &&
            ((engine_debug_word_peek() & 0x200u) == 0u ||
             sim_status_banner_word_get() != 0u))
            surface_blit_texture(pixels, pitch, target->w, target->h,
                s_message_next_image, (int)s_message_next_position[0],
                (int)s_message_next_position[1]);
        if (advisor_message_button_visible(2))
            surface_blit_texture(pixels, pitch, target->w, target->h,
                s_message_back_image, (int)s_message_back_position[0],
                (int)s_message_back_position[1]);
        surface_unlock(target, pixels);
    }
}

void advisor_text_request_action_hook_set(AdvisorTextRequestActionFn hook,
                                          void* context)
{
    s_advisor_text_request_action = hook;
    s_advisor_text_request_context = context;
}

/* AdvisorTextRequestSetup (FUN_0046ac20): normalize the request mask,
 * publish the two overlay globals, notify the actor, and set pending bit 0. */
// FUNCTION: LEGORR 0x0046ac20
void advisor_text_request_setup(uint32_t request, uint32_t flags)
{
    if (flags == 0u) flags = 0x8000u;
    flags |= 0x10000u;
    engine_overlay_words_set(flags, request);
    s_advisor_text_request_source = NULL;
    advisor_inline_token_render((char*)(uintptr_t)request, NULL, 1);
    s_advisor_text_state_flags |= 1u;
}

/* Typed source path for callers that already own the token text. On 64-bit
 * hosts the retail request dword cannot preserve that pointer, but inline
 * image/sound processing still runs against the live source before the
 * request is published. */
void advisor_text_request_setup_text(const char* source, uint32_t flags)
{
    uint32_t request = (uint32_t)(uintptr_t)source;
    if (flags == 0u) flags = 0x8000u;
    flags |= 0x10000u;
    engine_overlay_words_set(flags, request);
    s_advisor_text_request_source = source;
    if (source != NULL)
        advisor_inline_token_render((char*)source, NULL, 1);
    if (s_advisor_text_request_action != NULL)
        s_advisor_text_request_action(request, 0, 1,
                                     s_advisor_text_request_context);
    s_advisor_text_state_flags |= 1u;
}

uint32_t advisor_text_request_flags_get(void)
{
    return s_advisor_text_state_flags;
}

void debug_panel_text_ops_set(const DebugPanelTextOps* ops)
{
    if (ops != NULL) s_debug_panel_text_ops = *ops;
    else memset(&s_debug_panel_text_ops, 0,
                sizeof(s_debug_panel_text_ops));
}

/* DebugPanelTextDrawUpdate (FUN_004230c0): pulse the alternate palette every
 * 12.5 seconds, resolve an empty string from the active selection, clamp its
 * horizontal placement, and draw it centered vertically on its font row. */
// FUNCTION: LEGORR 0x004230c0
void debug_panel_text_draw_update(float elapsed, int screen_width,
                                  const char* text)
{
    int x = 0;
    int y = 0;
    int font_height = 0;
    int text_width = 0;
    int scroll_width = 0;
    s_debug_panel_text_timer -= elapsed;
    if (s_debug_panel_text_timer <= 0.0f) {
        s_debug_panel_text_timer += 12.5f;
        s_debug_panel_text_alternate =
            (s_debug_panel_text_alternate == 0) ? 1 : 0;
    }
    if (text == NULL) return;
    if (text[0] == '\0' &&
        s_debug_panel_text_ops.fallback_text_get != NULL) {
        text = s_debug_panel_text_ops.fallback_text_get(
            s_debug_panel_text_ops.context);
        if (text == NULL) return;
    }
    if (s_debug_panel_text_ops.origin_get != NULL) {
        s_debug_panel_text_ops.origin_get(s_debug_panel_text_ops.context,
                                          &x, &y);
    }
    if (s_debug_panel_text_ops.font_line_height != NULL) {
        font_height = s_debug_panel_text_ops.font_line_height(
            s_debug_panel_text_ops.context);
    }
    if (s_debug_panel_text_ops.text_width != NULL) {
        text_width = s_debug_panel_text_ops.text_width(
            s_debug_panel_text_ops.context, text);
    }
    if (s_debug_panel_text_ops.scroll_width != NULL) {
        scroll_width = s_debug_panel_text_ops.scroll_width(
            s_debug_panel_text_ops.context, 0x22u);
    }
    if (screen_width - 40 < x + text_width) x -= text_width;
    if (s_debug_panel_text_ops.text_draw != NULL) {
        int offset = (s_debug_panel_text_alternate == 0) ? scroll_width : 0;
        s_debug_panel_text_ops.text_draw(
            s_debug_panel_text_ops.context, x + offset,
            y - font_height / 2, s_debug_panel_text_alternate, text);
    }
}

void interface_spawn_request_ops_set(const InterfaceSpawnRequestOps* ops)
{
    if (ops != NULL) s_interface_spawn_request_ops = *ops;
    else memset(&s_interface_spawn_request_ops, 0,
                sizeof(s_interface_spawn_request_ops));
}

void interface_spawn_request_state_get(InterfaceSpawnRequestState* state_out)
{
    if (state_out != NULL) *state_out = s_interface_spawn_request_state;
}

/* InterfaceSpawnRequestEnqueue (FUN_00434e80): clear the interface pending
 * bit, update the directional request state for the two special types, or
 * dispatch to the first matching category from the object flags. */
// FUNCTION: LEGORR 0x00434e80
int interface_spawn_request_enqueue(int type, uint32_t parameter)
{
    uint32_t flags;
    uint32_t category;
    uint32_t state_flags = global_state_low_word_get();
    global_state_low_word_set(state_flags & ~0x80u);
    if (s_interface_spawn_request_ops.prepare != NULL) {
        s_interface_spawn_request_ops.prepare(
            s_interface_spawn_request_ops.context, 1);
    }
    if (type == 4 || type == 0xb) {
        float direction[2] = { 0.0f, 0.0f };
        state_flags = global_state_low_word_get();
        global_state_low_word_set(state_flags | 0x80u);
        s_interface_spawn_request_state.type = type;
        s_interface_spawn_request_state.parameter = parameter;
        if (s_interface_spawn_request_ops.camera_direction_get != NULL) {
            s_interface_spawn_request_ops.camera_direction_get(
                s_interface_spawn_request_ops.context, direction);
        }
        if (fabs((double)direction[0]) <= fabs((double)direction[1])) {
            s_interface_spawn_request_state.facing =
                (direction[1] > 0.0f) ? 2 : 0;
        }
        else {
            s_interface_spawn_request_state.facing =
                (direction[0] > 0.0f) ? 3 : 1;
        }
        return 1;
    }
    if (s_interface_spawn_request_ops.request_flags_get == NULL ||
        s_interface_spawn_request_ops.request_dispatch == NULL) return 0;
    flags = s_interface_spawn_request_ops.request_flags_get(
        s_interface_spawn_request_ops.context, type, parameter);
    if ((flags & 0x1000u) != 0u) category = 0u;
    else if ((flags & 0x800u) != 0u) category = 1u;
    else if ((flags & 0x2000u) != 0u) category = 2u;
    else if ((flags & 0x400000u) != 0u) category = 3u;
    else return 0;
    return s_interface_spawn_request_ops.request_dispatch(
        s_interface_spawn_request_ops.context, category,
        category < 3u && s_interface_spawn_request_ops.saved_slot_get != NULL
            ? s_interface_spawn_request_ops.saved_slot_get(
                s_interface_spawn_request_ops.context, category)
            : NULL,
        type, parameter);
}

void advisor_dialog_stack_ops_set(const AdvisorDialogStackOps* ops)
{
    if (ops != NULL) s_advisor_dialog_stack_ops = *ops;
    else memset(&s_advisor_dialog_stack_ops, 0,
                sizeof(s_advisor_dialog_stack_ops));
    s_advisor_dialog_stack_width = 0xffffffffu;
    s_advisor_dialog_stack_height = 0xffffffffu;
}

/* AdvisorDialogStackLayout (FUN_0041eca0): refresh geometry when dimensions
 * change, then lay out icons and queued entries upward from the top point. */
// FUNCTION: LEGORR 0x0041eca0
void advisor_dialog_stack_layout(float y, uint32_t parameter,
                                 uint32_t width, uint32_t height)
{
    AdvisorDialogStackEntry* entries = NULL;
    uint32_t count = 0;
    uint32_t index;
    (void)parameter;
    if (width != s_advisor_dialog_stack_width ||
        height != s_advisor_dialog_stack_height) {
        if (s_advisor_dialog_stack_ops.layout_reset != NULL) {
            s_advisor_dialog_stack_ops.layout_reset(
                s_advisor_dialog_stack_ops.context, width, height);
        }
        s_advisor_dialog_stack_width = width;
        s_advisor_dialog_stack_height = height;
    }
    if (s_advisor_dialog_stack_ops.active == NULL ||
        !s_advisor_dialog_stack_ops.active(
            s_advisor_dialog_stack_ops.context) ||
        s_advisor_dialog_stack_ops.entries_get == NULL ||
        !s_advisor_dialog_stack_ops.entries_get(
            s_advisor_dialog_stack_ops.context, width, height,
            &entries, &count) || entries == NULL) return;

    y -= 15.0f;
    for (index = 0; index < count; ++index) {
        void* icon = NULL;
        void* item = NULL;
        int resource_height = 0;
        if (s_advisor_dialog_stack_ops.icon_get != NULL) {
            icon = s_advisor_dialog_stack_ops.icon_get(
                s_advisor_dialog_stack_ops.context, index == 0 ? 0u : 1u);
        }
        if (icon == NULL) y -= 20.0f;
        else {
            if (s_advisor_dialog_stack_ops.icon_height != NULL) {
                resource_height = s_advisor_dialog_stack_ops.icon_height(
                    s_advisor_dialog_stack_ops.context, icon);
            }
            y -= (float)resource_height;
            if (s_advisor_dialog_stack_ops.icon_place != NULL) {
                s_advisor_dialog_stack_ops.icon_place(
                    s_advisor_dialog_stack_ops.context, icon, y);
            }
        }
        if (s_advisor_dialog_stack_ops.item_resolve != NULL) {
            int eligible = 0;
            if (s_advisor_dialog_stack_ops.entry_eligible != NULL) {
                eligible = s_advisor_dialog_stack_ops.entry_eligible(
                    s_advisor_dialog_stack_ops.context, entries[index].record);
            }
            item = s_advisor_dialog_stack_ops.item_resolve(
                s_advisor_dialog_stack_ops.context, entries[index].record,
                eligible);
        }
        if (item == NULL) y -= 40.0f;
        else {
            resource_height = 0;
            if (s_advisor_dialog_stack_ops.item_height != NULL) {
                resource_height = s_advisor_dialog_stack_ops.item_height(
                    s_advisor_dialog_stack_ops.context, item);
            }
            y -= (float)resource_height;
            if (s_advisor_dialog_stack_ops.item_place != NULL) {
                s_advisor_dialog_stack_ops.item_place(
                    s_advisor_dialog_stack_ops.context, item, y);
            }
            if (entries[index].has_special && entries[index].item_id != 0u &&
                s_advisor_dialog_stack_ops.special_item_apply != NULL) {
                s_advisor_dialog_stack_ops.special_item_apply(
                    s_advisor_dialog_stack_ops.context,
                    entries[index].item_id);
            }
        }
    }
}

/* AdvisorOverlayHitResolve (FUN_0041c1c0): first hit-test the active overlay
 * rectangle; if it misses, defer to the configurable control-tree search. */
// FUNCTION: LEGORR 0x0041c1c0
int advisor_overlay_hit_resolve(
    float x, float y, const AdvisorOverlayRect* focused_rect,
    float offset_x, float offset_y, AdvisorOverlayFallbackHitFn fallback,
    void* context, uint32_t* result0, uint32_t* result1,
    uint32_t* result2)
{
    uint32_t hit0 = 0u;
    uint32_t hit1 = 0u;
    uint32_t hit2 = 0u;
    if (focused_rect != NULL && focused_rect->enabled != 0 &&
        x >= focused_rect->x + offset_x &&
        x <= focused_rect->x + focused_rect->width + offset_x &&
        y >= focused_rect->y + offset_y &&
        y <= focused_rect->y + focused_rect->height + offset_y) {
        if (result0 != NULL) *result0 = 0u;
        return 1;
    }
    if (fallback == NULL ||
        fallback(context, x, y, offset_x, offset_y,
                 &hit0, &hit1, &hit2) == 0) {
        return 0;
    }
    if (result0 != NULL) *result0 = hit0;
    if (result1 != NULL) *result1 = hit1;
    if (result2 != NULL) *result2 = hit2;
    return 1;
}

void advisor_text_animation_ops_set(const AdvisorTextAnimationOps* ops,
                                    float reset_timer)
{
    if (ops != NULL) s_advisor_text_animation_ops = *ops;
    else memset(&s_advisor_text_animation_ops, 0,
                sizeof(s_advisor_text_animation_ops));
    s_advisor_text_animation_reset_timer = reset_timer;
}

/* AdvisorTextAnimationUpdate (FUN_0046ac60): update the active text slot,
 * optionally restart its timeout, advance an available animation, and
 * maintain the slot-selected flag. */
// FUNCTION: LEGORR 0x0046ac60
void advisor_text_animation_update(int slot, int force, int selected)
{
    if (force == 0) {
        if (s_advisor_text_animation_timer <= 0.0f) {
            s_advisor_text_current_slot = slot;
        }
    }
    else {
        s_advisor_text_current_slot = slot;
        s_advisor_text_animation_timer = s_advisor_text_animation_reset_timer;
    }
    if (s_advisor_text_animation_ops.is_paused == NULL ||
        s_advisor_text_animation_ops.is_paused(
            s_advisor_text_animation_ops.context) == 0) {
        int request = 0;
        if (s_advisor_text_animation_ops.animation_find != NULL &&
            s_advisor_text_animation_ops.animation_find(
                s_advisor_text_animation_ops.context, slot, &request) != 0 &&
            s_advisor_text_animation_ops.animation_play != NULL) {
            s_advisor_text_animation_ops.animation_play(
                s_advisor_text_animation_ops.context, request, 0);
        }
    }
    if (s_advisor_text_current_slot == slot && selected != 0) {
        s_advisor_text_state_flags |= 4u;
    }
    else {
        s_advisor_text_state_flags &= ~4u;
    }
}

void advisor_text_display_ops_set(const AdvisorTextDisplayOps* ops)
{
    if (ops != NULL) s_advisor_text_display_ops = *ops;
    else memset(&s_advisor_text_display_ops, 0,
                sizeof(s_advisor_text_display_ops));
}

/* Production binding for the display worker. The retail worker reads these
 * values from message/advisor globals directly; keep that ownership in the
 * existing portable message and engine state instead of requiring a caller
 * to install an adapter. */
static int advisor_text_display_state_get(void* context,
                                          AdvisorTextDisplayState* state)
{
    uint32_t request;
    (void)context;
    if (state == NULL) return 0;
    tutorial_message_generation_sync();
    request = engine_overlay_word_peek(1);
    state->current_slot = s_advisor_text_current_slot;
    state->slot_image = state->current_slot >= 0 ?
        ui_message_image_slot_get((unsigned int)state->current_slot) : NULL;
    state->hidden_slot = state->slot_image != NULL;
    ui_message_image_position_get(&state->x, &state->y);
    state->message_id = (int)request;
    state->message_flags = engine_overlay_word_peek(0);
    {
        const uint32_t* engine_state = engine_current_state_word();
        state->interface_flags = engine_state != NULL ? *engine_state : 0u;
    }
    state->message_timer = s_advisor_text_animation_timer;
    state->primary_text_output = ui_message_text_output_get(0);
    state->alternate_text_output = ui_message_text_output_get(1);
    state->message_text = s_advisor_text_request_source;
    state->message_token_index = request;
    state->message_token_index_valid =
        request != 0u && s_advisor_text_request_source == NULL;
    state->advisor_mode = global_flag_004ff978_get() != 0u;
    state->advisor_mode_valid = 1;
    return 1;
}

static int advisor_text_display_slot_available(void* context, int slot)
{
    (void)context;
    return slot >= 0 && slot < 26 &&
        ui_message_text_slot_get((unsigned int)slot) != NULL;
}

static void advisor_text_display_current_slot_set(void* context, int slot)
{
    (void)context;
    s_advisor_text_current_slot = slot;
}

static void advisor_text_display_message_flags_set(void* context,
                                                    uint32_t flags)
{
    (void)context;
    engine_overlay_words_set(flags, engine_overlay_word_peek(1));
    if (flags == 0u) s_advisor_text_request_source = NULL;
}

static void advisor_text_display_message_timer_set(void* context, float timer)
{
    (void)context;
    s_advisor_text_animation_timer = timer;
}

static void advisor_text_display_production_install(void)
{
    AdvisorTextDisplayOps ops;
    memset(&ops, 0, sizeof(ops));
    ops.state_get = advisor_text_display_state_get;
    ops.slot_is_available = advisor_text_display_slot_available;
    ops.current_slot_set = advisor_text_display_current_slot_set;
    ops.message_flags_set = advisor_text_display_message_flags_set;
    ops.message_timer_set = advisor_text_display_message_timer_set;
    ops.interface_finish = advisor_message_interface_finish;
    advisor_text_display_ops_set(&ops);
}

static int advisor_inline_name_equal(const char* a, const char* b)
{
    if (a == NULL || b == NULL) return a == b;
    while (*a != '\0' && *b != '\0') {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return 0;
    }
    return *a == *b;
}

void advisor_inline_token_ops_set(const AdvisorInlineTokenOps* ops)
{
    if (ops != NULL) s_advisor_inline_token_ops = *ops;
    else memset(&s_advisor_inline_token_ops, 0,
                sizeof(s_advisor_inline_token_ops));
    s_advisor_inline_last_sound = 0xffffffffu;
    s_advisor_inline_token_generation = 0xffffffffu;
    s_advisor_inline_duration = 0.0f;
}

uint32_t advisor_inline_last_sound_get(void)
{
    uint32_t generation = objective_advisor_token_generation_get();
    if (generation != s_advisor_inline_token_generation) {
        s_advisor_inline_token_generation = generation;
        s_advisor_inline_last_sound = objective_advisor_sound_token_count();
    }
    return s_advisor_inline_last_sound;
}

static void advisor_inline_output_char(char* output, uint32_t output_size,
                                       uint32_t* used, char value)
{
    if (output == NULL || output_size == 0u || used == NULL ||
        *used + 1u >= output_size) return;
    output[(*used)++] = value;
    output[*used] = '\0';
}

// FUNCTION: LEGORR 0x00456830
void advisor_inline_token_render(char* source, char* output, int draw)
{
    const uint32_t output_size = 512u;
    uint32_t used = 0u;
    void* image_handles[20];
    uint32_t image_token_count = 0u;
    uint32_t i = 0u;
    uint32_t image_table_count = objective_advisor_image_token_count();
    uint32_t sound_table_count = objective_advisor_sound_token_count();
    uint32_t token_generation = objective_advisor_token_generation_get();
    float timer_values[3];
    float final_duration;
    tutorial_message_timer_values_get(timer_values);
    final_duration = timer_values[2];
    memset(image_handles, 0, sizeof(image_handles));
    if (output != NULL && output_size != 0u) output[0] = '\0';
    if (source == NULL) return;
    if (token_generation != s_advisor_inline_token_generation) {
        s_advisor_inline_token_generation = token_generation;
        s_advisor_inline_last_sound = sound_table_count;
    }

    /* With neither resource table initialized the retail helper takes its
     * memcpy path and leaves marker characters untouched. */
    if (image_table_count == 0u && sound_table_count == 0u) {
        while (source[i] != '\0')
            advisor_inline_output_char(output, output_size, &used,
                                       source[i++]);
        if (draw != 0) {
            s_advisor_inline_duration = final_duration;
            tutorial_message_duration_set(final_duration);
        }
        return;
    }

    while (source[i] != '\0') {
        char marker = source[i];
        char close = marker == '<' ? '>' : (marker == '#' ? '#' : '\0');
        if (close != '\0' && source[i + 1u] != '\0') {
            char name[256];
            uint32_t start = i + 1u;
            uint32_t end = start;
            uint32_t name_length;
            uint32_t entry;
            int matched = 0;
            while (source[end] != '\0' && source[end] != close &&
                   end - start < sizeof(name) - 1u) ++end;
            if (source[end] == close) {
                name_length = end - start;
                memcpy(name, source + start, name_length);
                name[name_length] = '\0';
                if (marker == '<') {
                    for (entry = 0u; entry < image_table_count; ++entry) {
                        const char* token_name =
                            objective_advisor_image_token_name_get(entry);
                        if (advisor_inline_name_equal(name, token_name)) {
                            void* handle =
                                objective_advisor_image_token_resource_get(entry);
                            if (image_token_count <
                                sizeof(image_handles) / sizeof(image_handles[0]))
                                image_handles[image_token_count] = handle;
                            if (image_token_count < 255u) ++image_token_count;
                            matched = 1;
                            break;
                        }
                    }
                } else {
                    for (entry = 0u; entry < sound_table_count; ++entry) {
                        const char* token_name =
                            objective_advisor_sound_token_name_get(entry);
                        if (advisor_inline_name_equal(name, token_name)) {
                            uintptr_t encoded_handle =
                                objective_advisor_sound_token_handle_get(entry);
                            if (entry != s_advisor_inline_last_sound) {
                                float duration = 0.0f;
                                int sound_slot = encoded_handle != 0u
                                    ? (int)(uint32_t)encoded_handle - 1 : -1;
                                s_advisor_inline_last_sound = entry;
                                if (s_advisor_inline_token_ops.sound_duration != NULL)
                                    duration =
                                        s_advisor_inline_token_ops.sound_duration(
                                            s_advisor_inline_token_ops.context,
                                            (uint32_t)sound_slot);
                                else if (sound_slot >= 0)
                                    duration = (float)
                                        sound_streamed_wave_buffer_step_rate(
                                            sound_slot);
                                final_duration = duration * timer_values[0] + timer_values[1];
                                if (sound_slot >= 0 && sound_bank_sound_on()) {
                                    sound_deferred_play_expire();
                                    (void)sound_deferred_timer_set_if_idle(
                                        duration * 25.0f);
                                    advisor_sound_alias_set(sound_slot);
                                    advisor_sound_alias_play_global();
                                    if (system_state_flags_get() == 0u)
                                        (void)advisor_request_admit(0x14, 1);
                                    tutorial_message_sound_active_set();
                                    if (s_advisor_inline_token_ops.sound_transition != NULL)
                                        s_advisor_inline_token_ops.sound_transition(
                                            s_advisor_inline_token_ops.context,
                                            (uint32_t)sound_slot,
                                            final_duration);
                                }
                            }
                            matched = 1;
                            break;
                        }
                    }
                }
                /* Both recognized and unrecognized markup spans are consumed
                 * by the retail scanner; only ordinary text is copied. */
                (void)matched;
                i = end + 1u;
                continue;
            }
            /* An unterminated span consumes the rest of the input. */
            break;
        }
        advisor_inline_output_char(output, output_size, &used, marker);
        ++i;
    }
    if (s_advisor_inline_token_ops.image_draw != NULL) {
        uint32_t draw_count = image_token_count;
        uint32_t draw_index;
        float center = s_advisor_inline_token_ops.image_x != 0.0f
            ? s_advisor_inline_token_ops.image_x : 260.0f;
        float y = s_advisor_inline_token_ops.image_y != 0.0f
            ? s_advisor_inline_token_ops.image_y : 386.0f;
        float image_step = s_advisor_inline_token_ops.image_step_x != 0.0f
            ? s_advisor_inline_token_ops.image_step_x : 40.0f;
        if (draw_count > sizeof(image_handles) / sizeof(image_handles[0]))
            draw_count = sizeof(image_handles) / sizeof(image_handles[0]);
        for (draw_index = 0u; draw_index < draw_count; ++draw_index) {
            if (image_handles[draw_index] != NULL)
                s_advisor_inline_token_ops.image_draw(
                    s_advisor_inline_token_ops.context,
                    image_handles[draw_index], draw_index,
                    center - (float)image_token_count * 20.0f +
                        (float)draw_index * image_step,
                    y);
        }
    }
    if (draw != 0) {
        s_advisor_inline_duration = final_duration;
        tutorial_message_duration_set(final_duration);
    }
}

float advisor_inline_token_duration_get(void)
{
    return s_advisor_inline_duration;
}

/* The engine text-output record already owns its font, position, wrapping,
 * and scroll state. Use the shared record writer and wrapped renderer when a
 * display-state provider supplies that record but no backend-specific draw
 * callback. */
static void advisor_text_output_copy(void* output, const char* text, int mode)
{
    if (output == NULL || text == NULL) return;
    objective_advisor_text_copy(output, mode, "%s", text);
}

static int advisor_text_output_draw(void* output, float delta)
{
    int bottom = 0;
    int overflow;
    if (output == NULL) return 0;
    overflow = text_buffer_draw_wrapped(output, 0u, delta, &bottom);
    advisor_message_layout_result(output, overflow, bottom);
    return overflow;
}

/* AdvisorTextDisplayWorker (FUN_0046ad00): update the active advisor slot
 * or message timer, render the selected text path, then close or finish the
 * interface overlay according to the global display state. */
// FUNCTION: LEGORR 0x0046ad00
void advisor_text_display_worker(float delta)
{
    AdvisorTextDisplayOps* ops = &s_advisor_text_display_ops;
    AdvisorTextDisplayState state;
    int old_slot;
    int alternate;
    int slot_available = 0;
    int message_pending;
    int update_enabled;
    int image_path;
    const char* request_text = NULL;
    char rendered_text[512];
    if (ops->state_get == NULL) return;
    memset(&state, 0, sizeof(state));
    if (!ops->state_get(ops->context, &state)) return;
    old_slot = state.current_slot;
    message_pending = state.message_flags != 0u;
    update_enabled = !state.advisor_mode_valid || state.advisor_mode != 0;
    image_path = !message_pending && update_enabled && old_slot != 0x1a &&
                 state.slot_image != NULL;
    if (state.message_flags == 0u) {
        if (update_enabled) {
            slot_available = ops->slot_is_available != NULL ?
                ops->slot_is_available(ops->context, state.current_slot) : 0;
            if (slot_available && state.current_slot != 0x1a) {
                alternate = state.hidden_slot != 0;
                if (ops->slot_text_draw == NULL) {
                    const char* text = ui_message_text_slot_get(
                        (unsigned int)state.current_slot);
                    void* output = alternate != 0 ? state.alternate_text_output :
                                                    state.primary_text_output;
                    advisor_text_output_copy(output, text, 1);
                }
                if (state.message_timer <= 0.0f) {
                    state.current_slot = 0x1a;
                    if (ops->current_slot_set != NULL)
                        ops->current_slot_set(ops->context, state.current_slot);
                }
                state.message_timer -= delta;
                if (ops->message_timer_set != NULL)
                    ops->message_timer_set(ops->context, state.message_timer);
            }
        }
    }
    else if (state.message_id == 0) {
        state.message_flags = 0u;
        message_pending = 0;
        if (ops->message_flags_set != NULL)
            ops->message_flags_set(ops->context, 0u);
    }
    else {
        request_text = state.message_text;
        if (request_text == NULL && state.message_token_index_valid != 0u)
            request_text = objective_advisor_token_text_get(
                state.message_token_index);
        if (request_text != NULL) {
            advisor_inline_token_render((char*)request_text, rendered_text,
                                        0);
            advisor_text_output_copy(state.primary_text_output,
                                     rendered_text, 0);
        }
        if ((state.message_flags & 0x8000u) == 0u) {
            state.message_flags--;
            if (ops->message_flags_set != NULL)
                ops->message_flags_set(ops->context, state.message_flags);
        }
    }
    if (((state.interface_flags & 0x100000u) != 0u ||
         (ops->overlay_active != NULL &&
          ops->overlay_active(ops->context))) && old_slot != 0x16) {
        if (ops->interface_finish != NULL) {
            AdvisorTextDisplayState presentation = state;
            presentation.current_slot = old_slot;
            presentation.hidden_slot = image_path;
            ops->interface_finish(ops->context, &presentation, delta);
        }
        if (ops->overlay_close != NULL) ops->overlay_close(ops->context);
        return;
    }
    if (ops->interface_finish != NULL) {
        AdvisorTextDisplayState presentation = state;
        presentation.current_slot = old_slot;
        presentation.hidden_slot = image_path;
        ops->interface_finish(ops->context, &presentation, delta);
    }
    if (message_pending) {
        if (request_text != NULL) {
            (void)advisor_text_output_draw(state.primary_text_output, delta);
        }
        else if (ops->message_text_draw != NULL) {
            ops->message_text_draw(ops->context, state.message_id,
                                   state.x, state.y);
        }
        else {
            (void)advisor_text_output_draw(state.primary_text_output, delta);
        }
    }
    else {
        if (slot_available && old_slot != 0x1a) {
            alternate = state.hidden_slot != 0;
            if (ops->slot_text_draw != NULL) {
                ops->slot_text_draw(ops->context, old_slot, alternate,
                                    state.x, state.y);
            }
            else {
                void* output = alternate != 0 ? state.alternate_text_output :
                                                state.primary_text_output;
                (void)advisor_text_output_draw(output, delta);
            }
            if (state.slot_image != NULL) {
                DisplaySurface* target = surface_active_render_target_get();
                void* pixels = NULL;
                int pitch = 0;
                if (target != NULL && surface_lock(target, &pixels, &pitch) == 0) {
                    surface_blit_texture(pixels, pitch, target->w, target->h,
                        (TextureSurface*)state.slot_image,
                        (int)state.x, (int)state.y);
                    surface_unlock(target, pixels);
                }
            }
        }
        else {
            (void)advisor_text_output_draw(state.primary_text_output, delta);
        }
    }
}

void selected_ground_marker_ops_set(const SelectedGroundMarkerOps* ops)
{
    if (ops != NULL) s_selected_ground_marker_ops = *ops;
    else memset(&s_selected_ground_marker_ops, 0,
                sizeof(s_selected_ground_marker_ops));
}

/* SelectedEntityGroundMarkerRender (FUN_004248b0): project the entity's
 * ground point, reject off-screen markers, build the beveled eight-point
 * ground outline, then update its optional label tint. */
// FUNCTION: LEGORR 0x004248B0
int selected_entity_ground_marker_render(void* entity, void* camera,
                                         float red, float green, float blue)
{
    SelectedGroundMarkerOps* ops = &s_selected_ground_marker_ops;
    float world[3];
    float screen[3];
    float edge_world[3];
    float edge_screen[3];
    float width = 0.0f;
    float viewport_height = 0.0f;
    float entity_height = 0.0f;
    float radius = 0.0f;
    float pixel_radius;
    float bevel;
    float points[16];
    const char* label = NULL;
    uint32_t global_flags;
    float dx;
    float dy;
    float dz;
    if (entity == NULL ||
        entity_type_operation_release((const EntitySlot*)entity) != 0u ||
        ops->entity_renderable == NULL ||
        !ops->entity_renderable(ops->context, entity) ||
        ops->marker_data_get == NULL || ops->world_to_screen == NULL ||
        ops->viewport_get == NULL) return 0;
    global_flags = ops->global_flags_get != NULL ?
        ops->global_flags_get(ops->context) : 0u;
    if ((global_flags & 0x100000u) != 0u) {
        red *= 0.3f;
        green *= 0.3f;
        blue *= 0.3f;
    }
    if (!ops->marker_data_get(ops->context, entity, world, &radius,
                              &entity_height, &label)) return 0;
    world[2] -= entity_height * 0.5f;
    if (!ops->world_to_screen(ops->context, camera, world, screen)) return 0;
    ops->viewport_get(ops->context, &width, &viewport_height);
    if (screen[0] < 0.0f || screen[1] < 0.0f ||
        screen[0] > width || screen[1] > viewport_height) return 0;
    edge_world[0] = world[0] + radius;
    edge_world[1] = world[1];
    edge_world[2] = world[2];
    if (!ops->world_to_screen(ops->context, camera, edge_world, edge_screen))
        return 0;
    dx = screen[0] - edge_screen[0];
    dy = screen[1] - edge_screen[1];
    dz = screen[2] - edge_screen[2];
    pixel_radius = (float)sqrt(dx * dx + dy * dy + dz * dz) / 2.5f;
    bevel = pixel_radius / 5.0f + 3.0f;
    points[0] = screen[0] - pixel_radius;
    points[1] = screen[1] - pixel_radius + bevel;
    points[2] = screen[0] - pixel_radius + 1.0f;
    points[3] = screen[1] - pixel_radius;
    points[4] = screen[0] - pixel_radius + bevel;
    points[5] = screen[1] - pixel_radius;
    points[6] = screen[0] + pixel_radius - bevel;
    points[7] = screen[1] - pixel_radius;
    points[8] = screen[0] + pixel_radius;
    points[9] = screen[1] - pixel_radius + bevel;
    points[10] = screen[0] + pixel_radius;
    points[11] = screen[1] + pixel_radius - bevel;
    points[12] = screen[0] + pixel_radius - bevel;
    points[13] = screen[1] + pixel_radius;
    points[14] = screen[0] - pixel_radius + bevel;
    points[15] = screen[1] + pixel_radius;
    if (ops->polyline_draw != NULL)
        ops->polyline_draw(ops->context, points, red, green, blue);
    if ((global_flags & 0x100000u) == 0u && label != NULL && *label != '\0' &&
        ops->label_update != NULL) {
        ops->label_update(ops->context, label, red, green, blue);
    }
    return 1;
}

void position_tracker_box_draw_ops_set(const PositionTrackerBoxDrawOps* ops)
{
    if (ops != NULL) s_position_tracker_box_draw_ops = *ops;
    else memset(&s_position_tracker_box_draw_ops, 0,
                sizeof(s_position_tracker_box_draw_ops));
}

/* PositionTrackerBoxDraw (FUN_0041cd30): draw the two nested outline quads
 * in green unless the interface's transition flag is active. Each closed
 * four-point contour is one Draw_LineListEx call (draw.c) whose to-list is
 * the from-list shifted by one point. */
// FUNCTION: LEGORR 0x0041cd30
void position_tracker_box_draw(const float bounds[4])
{
    PositionTrackerBoxDrawOps* ops = &s_position_tracker_box_draw_ops;
    uint32_t flags;
    float points[10];
    int inset;
    if (bounds == NULL) return;
    flags = ops->interface_flags_get != NULL ?
        ops->interface_flags_get(ops->context) : 0u;
    if ((flags & 0x100000u) != 0u) return;
    for (inset = 1; inset <= 2; ++inset) {
        const float left = bounds[0] - (float)inset;
        const float top = bounds[1] - (float)inset;
        const float right = bounds[0] + bounds[2] + (float)inset;
        const float bottom = bounds[1] + bounds[3] + (float)inset;
        points[0] = left;
        points[1] = top;
        points[2] = right;
        points[3] = top;
        points[4] = right;
        points[5] = bottom;
        points[6] = left;
        points[7] = bottom;
        points[8] = left;
        points[9] = top;
        surface_draw_polyline((struct Point2F*)points,
                              (struct Point2F*)(points + 2), 4, 0.0f, 1.0f,
                              0.0f, 0);
    }
}

void overlay_text_buffer_ops_set(const OverlayTextBufferOps* ops)
{
    if (ops != NULL) s_overlay_text_buffer_ops = *ops;
    else memset(&s_overlay_text_buffer_ops, 0,
                sizeof(s_overlay_text_buffer_ops));
}

static void help_transition_control_noop(int a, int b);

/* OverlayTextBufferRefresh (FUN_00418a00): refresh the help overlay,
 * validate interface/input gates, render its header and scrolled entries,
 * then finish the overlay frame. */
// FUNCTION: LEGORR 0x00418a00
int overlay_text_buffer_refresh(void)
{
    OverlayTextBufferOps* ops = &s_overlay_text_buffer_ops;
    uint32_t flags;
    uint32_t input_flags;
    int count;
    int start;
    int index;
    float bottom = 0.0f;
    float y;
    float clip_y;
    int primary_drawn;
    /* The reference re-arms the transition cleanup when its bit-0x40 mode
     * handler is absent, then checks that mode again. That state is owned by
     * this module, so it does not need a host callback. */
    if (ops->ensure_initialized != NULL) {
        if (!ops->ensure_initialized(ops->context)) return 0;
    } else {
        if (ui_mode_resource_bit40_get() == 0u) {
            help_render_transition_cleanup_if_active();
        }
        if (ui_mode_resource_bit40_get() == 0u) return 0;
    }
    flags = ops->interface_flags_get != NULL ?
        ops->interface_flags_get(ops->context) : s_sel_state;
    if ((flags & 2u) == 0u || (flags & 1u) == 0u) return 0;
    input_flags = ops->input_flags_get != NULL ?
        ops->input_flags_get(ops->context) : system_state_flags_get();
    if ((input_flags & 0x800u) != 0u) return 0;
    if (ops->transition_begin != NULL)
        ops->transition_begin(ops->context);
    else
        help_transition_control_noop(0, 1);
    count = ops->entry_count_get != NULL ?
        ops->entry_count_get(ops->context) : 0;
    if (count <= 0) {
        if (ops->reset != NULL) ops->reset(ops->context);
        else help_render_transition_cleanup_if_active();
        return 1;
    }
    /* SurfaceBlitRect has a repository binding through the resource-record
     * renderer. The per-frame message/list source that decides whether this
     * background is needed is still supplied by the caller's row provider. */
    if (s_help_background_handle != 0u) {
        crt_surface_blit_record(s_help_background_handle,
                                s_help_background_offset_x,
                                s_help_background_offset_y);
    }
    primary_drawn = ops->primary_text_draw != NULL ?
        ops->primary_text_draw(ops->context, &bottom) : 0;
    if ((flags & 8u) != 0u) {
        if (primary_drawn && ops->header_draw != NULL)
            ops->header_draw(ops->context);
        if (ops->header_dirty_clear != NULL)
            ops->header_dirty_clear(ops->context);
    }
    y = bottom + 10.0f;
    if (ops->scroll_position_get != NULL)
        y += ops->scroll_position_get(ops->context);
    start = ops->scroll_start_index_get != NULL ?
        ops->scroll_start_index_get(ops->context, count) : 0;
    if (start < 0) start = 0;
    if (start >= count) start = count - 1;
    clip_y = ops->scroll_clip_y_get != NULL ?
        ops->scroll_clip_y_get(ops->context) : 0.0f;
    for (index = start; index < count; ++index) {
        OverlayTextLine line;
        memset(&line, 0, sizeof(line));
        if (ops->entry_get == NULL ||
            !ops->entry_get(ops->context, index, &line)) continue;
        if (clip_y != 0.0f && y + line.height >= clip_y) {
            if (ops->line_focus != NULL)
                ops->line_focus(ops->context, &line);
            if (ops->scroll_limit_reached != NULL)
                ops->scroll_limit_reached(ops->context);
            break;
        }
        line.y = y;
        if (ops->line_draw != NULL &&
            !ops->line_draw(ops->context, &line, line.x, y)) {
            if (ops->line_focus != NULL)
                ops->line_focus(ops->context, &line);
            break;
        }
        y += line.height + 10.0f;
    }
    if (ops->finish != NULL) ops->finish(ops->context);
    else ui_mode_resource_positions_apply();
    return 1;
}

void advisor_text_box_extent_hook_set(AdvisorTextBoxExtentFn hook,
                                      void* context)
{
    s_advisor_text_box_extent_hook = hook;
    s_advisor_text_box_extent_context = context;
}

/* AdvisorTextBoxRecordCommit (FUN_0046a9e0): apply the integer extents,
 * retain the last floating geometry, and clear the pending-layout bit. */
void advisor_text_box_record_commit_state(AdvisorTextBoxLayoutState* state)
{
    if (state == NULL) return;
    if (s_advisor_text_box_extent_hook != NULL) {
        s_advisor_text_box_extent_hook(s_advisor_text_box_extent_context,
            float_to_long(state->value), float_to_long(state->bottom));
    }
    state->flags &= ~1u;
}

/* TextLayoutGeometryStoreAndApply (FUN_0046a950): save the requested box,
 * derive its lower bound from the font line height, then commit it. */
void text_layout_geometry_store_and_apply_state(
    AdvisorTextBoxLayoutState* state, uint32_t object_id,
    uint32_t request_id, float value, const float geometry[4],
    uint32_t mode)
{
    if (state == NULL || geometry == NULL) return;
    state->object_id = object_id;
    state->request_id = request_id;
    state->value = value;
    state->geometry[0] = geometry[0];
    state->geometry[1] = geometry[1];
    state->geometry[2] = geometry[2];
    state->geometry[3] = geometry[3];
    state->bottom = geometry[1] + geometry[3] - state->font_line_height;
    state->mode = mode;
    advisor_text_box_record_commit_state(state);
}

/* UiTextEntryStateLoad is the portable state adapter for the UI text slot.
 * The target at 0x0046b520 takes a slot index and uses renderer-owned global
 * text buffers, font state, and draw context, none of which this record owns.
 * Keep this helper unmapped until that runtime owner is available. */
void ui_text_entry_state_load(UiTextEntryState* entry, const char* source,
                              UiTextImageLoadFn image_load, void* context)
{
    const char* input;
    size_t out = 0;
    if (entry == NULL) return;
    entry->image = NULL;
    if (source == NULL) source = "";
    if (source[0] == '@') {
        input = source + 1;
        while (*input != '\0' && out + 1 < sizeof(entry->text)) {
            entry->text[out++] = *input++;
        }
        entry->text[out] = '\0';
        if (image_load != NULL) {
            entry->image = image_load(entry->text, context);
        }
        entry->flags |= 8u;
    }
    else {
        input = source;
        while (*input != '\0' && out + 1 < sizeof(entry->text)) {
            if (*input == '\\' && input[1] == 'n') {
                entry->text[out++] = '\n';
                input += 2;
            }
            else if (*input == '_') {
                entry->text[out++] = ' ';
                ++input;
            }
            else {
                entry->text[out++] = *input++;
            }
        }
        entry->text[out] = '\0';
        entry->flags &= ~8u;
    }
    memset(entry->counters, 0, sizeof(entry->counters));
    entry->flags |= 3u;
}

const char* tooltip_info_string_peek(int index)
{
    return (index >= 0 && index < 9) ? s_tooltip_info_strings[index] : NULL;
}

static void tooltip_overlay_state_fill(
    TooltipOverlayState* state, void* font,
    uint32_t value2, uint32_t value3, float scale,
    uint32_t value5, uint32_t value6, uint32_t value7,
    float color_r, float color_g, float color_b,
    uint32_t font_metric)
{
    int i;
    if (state == NULL) return;
    for (i = 0; i < 38; ++i) state->names[i] = s_tooltip_names[i];
    state->font = font;
    state->font_metric = font_metric;
    state->values[0] = value2;
    state->values[1] = value3;
    state->values[2] = value5;
    state->values[3] = value6;
    state->values[4] = value7;
    state->scale = scale * 25.0f;
    state->color_base[0] = color_r;
    state->color_base[1] = color_g;
    state->color_base[2] = color_b;
    state->color_high[0] = color_r + color_r * 0.4f;
    state->color_high[1] = color_g + color_g * 0.4f;
    state->color_high[2] = color_b + color_b * 0.4f;
    state->color_low[0] = color_r - color_r * 0.4f;
    state->color_low[1] = color_g - color_g * 0.4f;
    state->color_low[2] = color_b - color_b * 0.4f;
    for (i = 0; i < 3; ++i) {
        if (state->color_base[i] > 1.0f) state->color_base[i] = 1.0f;
        if (state->color_base[i] < 0.0f) state->color_base[i] = 0.0f;
    }
}

/* Adapter retained for callers that own a separate state object. This is not
 * the mapped target ABI; the native entry below writes the module globals. */
void tooltip_overlay_state_initialize_state(
    TooltipOverlayState* state, void* font,
    uint32_t value2, uint32_t value3, float scale,
    uint32_t value5, uint32_t value6, uint32_t value7,
    float color_r, float color_g, float color_b,
    TooltipFontMetricQueryFn metric_query, void* context)
{
    const uint32_t metric = metric_query != NULL
        ? metric_query(font, context) : object_field_be4_dword_get(font);
    tooltip_overlay_state_fill(state, font, value2, value3, scale,
                               value5, value6, value7,
                               color_r, color_g, color_b, metric);
}

/* TooltipOverlayStateInitialize @ 0x46b1d0: the target's ten stack-word entry
 * publishes its fixed labels and geometry/color globals, sets the timed
 * tooltip duration in 25-unit ticks, and clears 0x1839 dwords at the start
 * of the 0x27c-byte overlay-record table. This includes the first 12 bytes
 * of record 39 but leaves its +0x278 traversal-bound flag intact. */
// FUNCTION: LEGORR 0x0046b1d0
void tooltip_overlay_state_initialize(
    void* font, uint32_t value2, uint32_t value3, float scale,
    uint32_t value5, uint32_t value6, uint32_t value7,
    float color_r, float color_g, float color_b)
{
    float threshold = scale * 25.0f;
    int i;
    tooltip_overlay_state_fill(&s_tooltip_overlay_state, font,
                               value2, value3, scale,
                               value5, value6, value7,
                               color_r, color_g, color_b,
                               object_field_be4_dword_get(font));
    for (i = 0; i < 38; ++i) s_tooltip_name_table[i] = s_tooltip_names[i];
    s_timed_effect_tick = threshold;
    memset(s_timed_effect_records, 0,
           (size_t)0x1839u * sizeof(uint32_t));
}

void tooltip_overlay_state_current(TooltipOverlayState* state)
{
    if (state != NULL) *state = s_tooltip_overlay_state;
}

void tooltip_software_initialize(const TextureSurface* font, int width,
                                 int height, float r, float g, float b)
{
    s_advisor_tooltip_font = font;
    tooltip_overlay_state_fill(&s_tooltip_overlay_state, (void*)font,
        2u, 1u, 1.0f, (uint32_t)width, (uint32_t)height, 32u,
        r, g, b, font != NULL ? (uint32_t)(font->height / FE_FONT_GRID_ROWS) : 0u);
    s_timed_effect_tick = 25.0f;
    memset(s_timed_effect_records, 0, sizeof(s_timed_effect_records));
}

void tooltip_software_shutdown(void)
{
    if (s_advisor_tooltip_font != NULL)
        texture_free((TextureSurface*)s_advisor_tooltip_font);
    s_advisor_tooltip_font = NULL;
    memset(&s_tooltip_overlay_state, 0, sizeof(s_tooltip_overlay_state));
    memset(s_timed_effect_records, 0, sizeof(s_timed_effect_records));
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
                if (help_window_info_path_build(path, sizeof(path), catalog,
                                                line_index))
                    value = block_get_strptr(cfg, path);

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

const char* help_window_info_string_get(int group, size_t catalog_index,
                                        int line_index)
{
    if (group < 0 || group >= 3 || catalog_index >= 15u ||
        line_index < 0 || line_index >= 16) return NULL;
    return s_help_info_strings[group][catalog_index * 16u +
                                     (size_t)line_index];
}

uintptr_t help_window_text_area_handle_peek(void)
{
    return s_help_text_area_handle;
}

static void help_window_button_layout_load_with_ops(
    BlockFile* cfg, const FeHelpWindowResourceOps* ops);
static uint32_t surface_load_handle(const char* path);

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
    } else {
        help_window_info_config_strings_load_native(cfg);
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
            if (length < sizeof(name)) {
                memcpy(name, background, length);
                name[length] = '\0';
                s_help_background_offset_x = (float)atoi(first_separator + 1);
                s_help_background_offset_y = (float)atoi(second_separator + 1);
                s_help_background_handle = ops != NULL &&
                    ops->resource_load != NULL
                    ? ops->resource_load(ops->context, name, 0, 0)
                    : surface_load_handle(name);
                if (s_help_background_handle != 0u && ops != NULL &&
                    ops->background_apply != NULL) {
                    ops->background_apply(ops->context,
                                          s_help_background_handle);
                }
            }
        }
    }

    help_window_button_layout_load_with_ops(cfg, ops);

    const char* text_area = block_get_strptr(cfg,
        "HelpWindow::TextWindowArea");
    const uintptr_t help_font_handle = (uintptr_t)
        resource_name_cache_get_or_load("Interface\\Fonts\\FONT5_HI.bmp");
    int area[4];
    if (fe_parse_int_tuple(text_area, area, 4)) {
        if (ops != NULL && ops->text_area_create != NULL) {
            s_help_text_area_handle = (uintptr_t)ops->text_area_create(
                ops->context, help_font_handle, area, 0x400u);
            s_help_text_area_native_record = NULL;
        } else {
            float area_f[4];
            uint32_t area_words[4];
            int i;
            for (i = 0; i < 4; ++i) {
                area_f[i] = (float)area[i];
                memcpy(&area_words[i], &area_f[i], sizeof(area_words[i]));
            }
            s_help_text_area_native_record = text_buffer_create(
                help_font_handle, area_words, 0x400);
            s_help_text_area_handle =
                (uintptr_t)s_help_text_area_native_record;
        }
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
    ui_mode_resource_bit40_set_clear(0, 0u);
    if (ops != NULL && ops->refresh != NULL) {
        ops->refresh(ops->context, 0, 0);
    }
}

static void help_window_button_layout_load_with_ops(
    BlockFile* cfg, const FeHelpWindowResourceOps* ops)
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
        const float final_x = (float)x + s_help_background_offset_x;
        const float final_y = (float)y + s_help_background_offset_y;

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
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 0].f = final_x;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 1].f = final_y;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 2].f = (float)width;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 3].f = (float)height;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 4].u = ops != NULL &&
            ops->resource_load != NULL
            ? ops->resource_load(ops->context, first, 0, 0) : 0u;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 5].u = ops != NULL &&
            ops->resource_load != NULL
            ? ops->resource_load(ops->context, second, 0, 0) : 0u;
        if (ops == NULL || ops->resource_load == NULL) {
            s_layout_position[i * LAYOUT_POSITION_STRIDE + 4].u =
                surface_load_handle(first);
            s_layout_position[i * LAYOUT_POSITION_STRIDE + 5].u =
                surface_load_handle(second);
        }
        help_window_label_copy(s_layout_position_text_storage[i],
                               sizeof(s_layout_position_text_storage[i]),
                               text);
        s_layout_position_text[i] = s_layout_position_text_storage[i];
        if (ops != NULL && ops->text_load != NULL) {
            ops->text_load(ops->context, i == 0 ? 8 : 9,
                           s_layout_position_text[i]);
        }
    }
}

// FUNCTION: LEGORR 0x004185f0
void help_window_button_layout_load(BlockFile* cfg, BlockFile* config_root)
{
    /* The root is folded into the port's BlockFile path lookup. Native
     * surfaces are loaded through the frontend resource cache. */
    (void)config_root;
    help_window_button_layout_load_with_ops(cfg, NULL);
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
        s_layout_position_debug[entry][0] = x;
        s_layout_position_debug[entry][1] = y;
    }
}

int layout_position_get(int entry, int* x, int* y)
{
    if (entry < 0 || entry >= LAYOUT_POSITION_ENTRIES) {
        return 0;
    }
    if (x != NULL) {
        *x = s_layout_position_debug[entry][0];
    }
    if (y != NULL) {
        *y = s_layout_position_debug[entry][1];
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
 * and returns the rounded mean channel level on the original 0..9 scale,
 * or zero when no such source is available. Backends without a hardware CD
 * mixer return unavailable rather than substituting streamed-output gain. */
// FUNCTION: LEGORR 0x00414e10
int option_music_default(void)
{
    float left;
    float right;
    if (!audio_cd_mixer_volume_get(&left, &right)) return 0;
    return crt_float_to_long((left + right) * 4.5f);
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
    const uint32_t ui_mode = ui_mode_resource_bit40_get();
    const uint32_t bit1 = s_sel_state & 1u;
    const uint32_t bit2 = s_sel_state & 2u;
    const uint32_t system_state = system_state_flags_get();
    if (ui_mode != 0u && bit2 != 0u && bit1 != 0u
        && system_state == 0u) {
        return 1;
    }
    return 0;
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
// GLOBAL: LEGORR 0x004ddd0c
static uint32_t s_sel_c0c;
// GLOBAL: LEGORR 0x004ddd10
static uint32_t s_sel_c10;
// GLOBAL: LEGORR 0x004ddd14
static float    s_sel_c14;
// GLOBAL: LEGORR 0x004ddd18
static float    s_sel_c18;
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
uint32_t s_obj_sel_word;          /* DAT_004ddccc */
// GLOBAL: LEGORR 0x004ddcd0
static float s_obj_track_word;     /* _DAT_004ddcd0 */
// GLOBAL: LEGORR 0x004de1ec
static float    s_inter_timer = 0.0f; /* DAT_004de1ec */
// GLOBAL: LEGORR 0x004de1f8
static uint32_t s_inter_flags;    /* DAT_004de1f8 */
// GLOBAL: LEGORR 0x004ddd1c
static uint32_t s_status_banner_word;
// GLOBAL: LEGORR 0x004ddd20
static float s_status_blink_timer;
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
    s_reg_e8 = (float)a;
    s_reg_ec = (float)b;
    s_reg_f0 = e * s_region_scale_004;
}

// FUNCTION: LEGORR 0x0041a990
void objective_selection_state_reset(void)
{
    s_obj_sel_word = 0u;
    s_obj_track_word = s_reg_e8;
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
#if defined(_MSC_VER) && defined(_M_IX86)
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
#else
void mode_flag_pair_apply_if_bit2(int value)
{
    if ((s_sel_state & 4u) == 0u) return;
    if (value != 0) {
        s_sel_state |= 2u;
        s_sel_state &= ~1u;
    }
    else {
        s_sel_state &= ~2u;
        s_sel_state &= ~1u;
    }
}
#endif

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

void frontend_event_timer_state_get(EventTimerState* state)
{
    if (state == NULL) return;
    state->flags = s_inter_flags;
    /* DAT_005002bc is supplied only by its unported input producer. */
    state->external_flags = event_timer_interaction_flags_get();
    state->current_event = (int)s_obj_sel_word;
    state->next_event = (int)s_map_store_kind;
    state->timer = s_obj_track_word;
    state->rate = s_reg_f0;
    state->lower = s_reg_e8;
    state->upper = s_reg_e0;
    state->current_x = s_map_store_pair_x;
    state->current_y = s_map_store_pair_y;
    state->next_x = s_map_store_last_x;
    state->next_y = s_map_store_last_y;
}

void frontend_event_timer_state_set(const EventTimerState* state)
{
    if (state == NULL) return;
    s_inter_flags = state->flags;
    s_obj_sel_word = (uint32_t)state->current_event;
    s_obj_track_word = state->timer;
    s_map_store_pair_x = state->current_x;
    s_map_store_pair_y = state->current_y;
}

static void frontend_status_flash_tick(float delta)
{
    s_status_blink_timer -= delta;
    if (s_status_blink_timer < 0.0f) {
        s_status_banner_word = (uint32_t)(s_status_banner_word == 0u);
        s_status_blink_timer = 15.0f;
    }
}

// FUNCTION: LEGORR 0x0041B930
void timed_status_and_event_tick(float delta)
{
    frontend_status_flash_tick(delta);
    event_timer_countdown_advance(delta);
    if (s_inter_timer > 0.0f) {
        s_inter_timer -= delta;
        return;
    }
    s_inter_flags |= 0x800u;
}

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

void interaction_flags_set(uint32_t flags)
{
    s_inter_flags = flags;
}

/* AdvisorCallbackScheduleGate reads the held scan-code byte at DAT_0076abbc
 * (keyboard offset 0x3c) before walking the current catalog. The anonymous
 * row callback remains an explicit binding because its body is unresolved. */
static FeAdvisorCatalogCallback s_advisor_callback_callback;

void advisor_callback_schedule_callback_set(
    FeAdvisorCatalogCallback callback)
{
    s_advisor_callback_callback = callback;
}

/* AdvisorCallbackScheduleGate @ 0x0041c440 is a no-argument global-state
 * entry. The catalog walk is available; LAB_0041c470 remains an explicit
 * callback binding because its body is unresolved. */
// FUNCTION: LEGORR 0x0041C440
void advisor_callback_schedule_gate(void)
{
    if ((s_inter_flags & 0x04u) == 0u || (s_inter_flags & 0x80u) == 0u ||
        !input_keyboard_key_held(0x3c) ||
        s_advisor_callback_callback == NULL) {
        return;
    }
    (void)catalog_callback_by_id_dispatch(save_current_context_get(),
        (int)s_obj_sel_word, s_advisor_callback_callback, NULL);
}

/* AdvisorAnimationSlotIdResolve (FUN_0045c6b0): only kinds 0, 6, 9, and 10
 * have cases; each accepts its own position-index range and maps to a fixed
 * advisor animation slot. */
// FUNCTION: LEGORR 0x0045C6B0
int advisor_animation_slot_id_resolve(uint32_t animation_kind,
                                      uint32_t position_index,
                                      uint32_t* slot_id)
{
    if (slot_id == NULL) return 0;
    *slot_id = 0x15u;
    switch (animation_kind) {
    case 0:
        if (position_index <= 4u) *slot_id = 6u + position_index;
        break;
    case 6:
        if (position_index <= 2u) *slot_id = 0x0eu + position_index;
        break;
    case 9:
        if (position_index <= 1u) *slot_id = 0x12u + position_index;
        break;
    case 10:
        if (position_index <= 1u) *slot_id = 0x0bu + position_index;
        break;
    }
    return *slot_id != 0x15u;
}

/* --- the cell-action permission window (see frontend.hpp) ------------------- */
#define CELL_PERMISSION_ENTRIES 300 /* corpus's 300-dword clear loop */
// GLOBAL: LEGORR 0x004ddd34
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
    /* The retail helper returns the table dword itself, not a normalized
     * boolean. Keep callers that use its value semantics intact. */
    return (int)s_cell_perm[index];
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

// FUNCTION: LEGORR 0x00412190
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) bool menu_audio_tick_interval_reached(void)
{
    __asm {
        call rand
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
void ui_mode_resource_bit40_set_clear(int probe, uint32_t value)
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
static void panel_texture_slots_blit_to(int panel, void* pixels, int pitch,
                                        int width, int height)
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
    const float ox = crt_float_bits(oa);
    const float oy = crt_float_bits(ob);

    for (int i = 0; i < count; ++i) {
        const float* rec = (const float*)(block + (size_t)i * 0x2c);
        uint32_t flags = 0u;
        __builtin_memcpy(&flags, rec + 10, sizeof(flags));
        if ((flags & 0x10u) != 0u) {
            continue; /* the corpus hides bit-0x10 items */
        }
        const int has40 = (flags & 0x40u) != 0u;
        int banner = 0;
        int field = 0;
        if ((flags & 8u) == 0u && (flags & 2u) != 0u) {
            if (!has40) {
                field = 4;
            } else {
                banner = (int)sim_status_banner_word_get();
                if (banner == 0) {
                    continue; /* the corpus skips the blit */
                }
                field = 4;
            }
        } else if ((flags & 4u) == 0u && (flags & 8u) == 0u) {
            if (!has40) {
                field = 6;
            } else {
                banner = (int)sim_status_banner_word_get();
                field = banner == 0 ? 6 : 4;
            }
        } else {
            if (has40) {
                banner = (int)sim_status_banner_word_get();
                if (banner != 0) {
                    field = 4;
                } else {
                    field = 5;
                }
            } else {
                field = 5;
            }
        }
        uint32_t handle = 0u;
        __builtin_memcpy(&handle, rec + field, sizeof(handle));
        if (handle != 0u) {
            float x = rec[0] + ox;
            float y = rec[1] + oy;
            TextureSurface* surface = panel_named_resource_get(
                panel, block, i, field, handle);
            crt_surface_blit_record(handle, x, y);
            if (surface != NULL && pixels != NULL)
                surface_blit_texture(pixels, pitch, width, height, surface,
                                     (int)crt_float_to_long(x),
                                     (int)crt_float_to_long(y));
        }
    }
}

void panel_texture_slots_blit(int panel)
{
    panel_texture_slots_blit_to(panel, NULL, 0, 0, 0);
}

void panel_texture_slots_render(int panel, void* pixels, int pitch,
                                int width, int height)
{
    if (pixels == NULL || pitch <= 0 || width <= 0 || height <= 0) return;
    panel_texture_slots_blit_to(panel, pixels, pitch, width, height);
}

/* The physical gesture caller supplies mapped button levels; the scan owns
 * only category/item capture and feedback, separately from action dispatch. */
/* Ordinary 45b310 scan. 500860 bit 1 forced-camera mode is unowned here.
 * Return means geometric ownership; item 24 means no action-capable item. */
int frontend_panels_pointer_update(FrontendPanelPointerCapture* capture,
                                  int x, int y, int current, int previous,
                                  FrontendPanelPointerHit* hit)
{
    FrontendPanelRotationControlState rotation;
    int found = 0, panel;
    if (capture == NULL || hit == NULL) return 0;
    hit->category = 12; hit->item = 24; hit->released = 0;
    frontend_panel_rotation_control_state_get(&rotation);
    for (panel = 0; panel < 12; ++panel) {
        uint32_t ox_bits = 0u, oy_bits = 0u;
        int i, px, py, count;
        char* block;
        if ((request_record_flags_peek(panel) & 1u) == 0u) continue;
        count = panel_draw_count_peek(panel);
        block = (char*)panel_draw_records_peek(panel);
        if (block == NULL) continue;
        indexed_pair_get(panel, &ox_bits, &oy_bits);
        px = x - (int)crt_float_bits(ox_bits);
        py = y - (int)crt_float_bits(oy_bits);
        for (i = 0; i < count; ++i) {
            float* rec = (float*)(block + (size_t)i * 0x2c);
            uint32_t flags;
            int inside = 0;
            memcpy(&flags, rec + 10, sizeof(flags));
            if (!found) {
                if (panel == 9 && i == 3) {
                    const double dx = (double)rotation.center_x - x;
                    const double dy = (double)rotation.center_y - y;
                    inside = sqrt(dx * dx + dy * dy) < rotation.radius;
                } else {
                    inside = rec[0] <= (float)px && (float)px < rec[2] &&
                             rec[1] <= (float)py && (float)py < rec[3];
                }
            }
            if (inside) {
                int same;
                found = 1;
                hit->category = panel;
                same = capture->active && capture->category == panel &&
                       capture->item == i && capture->records == block;
                if (current) {
                    if (!capture->active && !previous) {
                        capture->active = 1; capture->category = panel;
                        capture->item = i; capture->records = block;
                        same = 1;
                    }
                    if (same) {
                        hit->item = i;
                        flags = (flags & ~2u) | 4u;
                    }
                } else if (!capture->active || same) {
                    hit->item = i;
                    if (same) {
                        if ((flags & 0x20u) != 0u) flags ^= 8u;
                        hit->released = 1;
                    }
                    flags = (flags & ~4u) | 2u;
                }
            } else {
                flags = (flags & ~6u) | ((flags & 8u) ? 4u : 0u);
            }
            memcpy(rec + 10, &flags, sizeof(flags));
        }
    }
    if (!current) {
        capture->active = 0; capture->records = NULL;
        capture->category = 12; capture->item = 24;
    }
    return found;
}

/* Explicit cancellation does not release. Only clean the captured record if
 * its original block is still installed, category enabled, and index valid. */
void frontend_panels_pointer_capture_cancel(FrontendPanelPointerCapture* capture)
{
    if (capture == NULL) return;
    if (capture->active && capture->category >= 0 && capture->category < 12 &&
        (request_record_flags_peek(capture->category) & 1u) != 0u &&
        capture->records != NULL &&
        capture->records == panel_draw_records_peek(capture->category) &&
        capture->item >= 0 && capture->item < panel_draw_count_peek(capture->category)) {
        char* block = (char*)panel_draw_records_peek(capture->category);
        char* word = block + (size_t)capture->item * 0x2c + 0x28;
        uint32_t flags;
        memcpy(&flags, word, sizeof(flags));
        flags = (flags & ~6u) | ((flags & 8u) ? 4u : 0u);
        memcpy(word, &flags, sizeof(flags));
    }
    capture->active = 0; capture->records = NULL;
    capture->category = 12; capture->item = 24;
}

int frontend_panels_pointer_hover(int x, int y)
{
    FrontendPanelPointerCapture capture;
    FrontendPanelPointerHit hit;
    memset(&capture, 0, sizeof(capture));
    return frontend_panels_pointer_update(&capture, x, y, 0, 0, &hit);
}

/* Release counts precede the tutorial/result gates in 0x45adb0. A hit and a
 * performed action are separate results; unported action owners return zero. */
int frontend_panels_pointer_activate(int category, int item)
{
    char* block;
    uint32_t count, flags, blocked = 0u;
    LevelState* level;
    if (category < 0 || category >= 12 || item < 0 ||
        item >= panel_draw_count_peek(category)) return 0;
    block = (char*)panel_draw_records_peek(category);
    if (block == NULL) return 0;
    memcpy(&count, block + (size_t)item * 0x2c + 0x24, sizeof(count));
    ++count;
    memcpy(block + (size_t)item * 0x2c + 0x24, &count, sizeof(count));
    flags = system_state_flags_get();
    if (category == 0) blocked = flags & 0x10u;
    else if (category == 6 && item < 3) blocked = flags & (0x20u << item);
    else if (category == 9) {
        if (item == 2) blocked = flags & 0x400u;
        else if (item == 0 || item == 1 || item == 3)
            blocked = flags & 0x1000u;
    }
    if (blocked != 0u) {
        if (category == 6 && item < 3)
            panel_item_flag8_set_if_enabled(category, item, 0);
        media_action_feedback_sound_dispatch(1);
        return 0;
    }
    level = level_current_owner_get();
    if (level == NULL || level_int(level, LEVEL_OFF_OBJ_RESULT) != 0) {
        media_action_feedback_sound_dispatch(1);
        return 0;
    }
    if (category == 6 && item == 0) {
        media_action_feedback_sound_dispatch(0);
        unported_ledger_stubs::ledger_stub_00425c30_TransitionControl(1, 0u);
        engine_debug_word_bit_set(0x100u,
            (engine_debug_word_peek() & 0x100u) == 0u);
        return 1;
    }
    /* Other category actions retain their own renderer/game owners. */
    return 0;
}

/* The software presentation pass consumes live resources and category state.
 * Simulation advances these positions once in EngineManagerDispatch; drawing
 * a frozen pause frame must not advance the manager or replay its ledger. */
static void frontend_panel_render(int panel, void* pixels, int pitch,
                                   int width, int height)
{
    uint32_t x = 0u, y = 0u;
    TextureSurface* image = legacy_panel_texture_get(panel);
    indexed_pair_get(panel, &x, &y);
    if (image != NULL)
        surface_blit_texture(pixels, pitch, width, height, image,
            (int)floor((double)crt_float_bits(x)),
            (int)floor((double)crt_float_bits(y)));
    panel_texture_slots_render(panel, pixels, pitch, width, height);
}

void frontend_panels_render(uint32_t state, void* pixels, int pitch,
                            int width, int height)
{
    static const int order[] = {4, 9, 6, 8, 11, 5, 7, 10};
    unsigned int i;
    if ((state & ENGINE_STATE_INDEXED_SUBSYS) == 0u || pixels == NULL) return;
    if ((state & 2u) == 0u)
        frontend_panel_render(1, pixels, pitch, width, height);
    frontend_panel_render(0, pixels, pitch, width, height);
    if ((state & 2u) != 0u && (state & 0x2000u) != 0u &&
        (state & 0x20000u) == 0u)
        frontend_panel_render(2, pixels, pitch, width, height);
    for (i = 0; i < sizeof(order) / sizeof(order[0]); ++i)
        frontend_panel_render(order[i], pixels, pitch, width, height);
}

void frontend_panels_release(void)
{
    int panel;
    for (panel = 0; panel < 12; ++panel) {
        panel_draw_records_set(panel, NULL);
        panel_draw_count_set(panel, 0);
        legacy_overlay_image_record_load(NULL, panel, 0u);
    }
}

/* The 0x41c310 row callback uses the same image owners and catalog traversal
 * as composition. A missing image retains the reference's 40x40 extent. */
typedef struct FeActionCatalogHitPass {
    const EngineContext* engine;
    float x, y;
    float position[2];
    uint32_t action, kind, entry;
} FeActionCatalogHitPass;

static int fe_action_catalog_hit_row(int action, int kind, int entry,
                                    void* payload)
{
    FeActionCatalogHitPass* pass = (FeActionCatalogHitPass*)payload;
    TextureSurface* image = action == 0x49
        ? interface_build_image_binding_value_get(pass->engine, kind, entry)
        : interface_action_image_value_get(pass->engine, action);
    const int width = image != NULL ? image->width : 40;
    const int height = image != NULL ? image->height : 40;
    if (pass->x >= pass->position[0] &&
        pass->x <= pass->position[0] + (float)width &&
        pass->y >= pass->position[1] &&
        pass->y <= pass->position[1] + (float)height) {
        uint32_t position[2];
        pass->action = (uint32_t)action;
        if (action == 0x49) {
            pass->kind = (uint32_t)kind;
            pass->entry = (uint32_t)entry;
        }
        cursor_tile_selection_update_by_type(action, (uint32_t)kind,
                                               (uint32_t)entry);
        memcpy(position, pass->position, sizeof(position));
        selection_coordinates_set(position, (uint32_t)(width - 1),
                                   (uint32_t)(height - 1));
        return 1;
    }
    pass->position[1] += (float)height;
    return 0;
}

void frontend_action_catalog_pointer_setup(SimulationPointerDispatchOps* ops)
{
    EventTimerState state;
    float origin[2];
    const FeSurroundImage* surround;
    int width, height;
    if (ops == NULL) return;
    frontend_event_timer_state_get(&state);
    interface_menu_action_layout_peek(origin);
    ops->overlay_offset_x = state.timer;
    ops->overlay_offset_y = origin[1];
    memset(&ops->focused_rect, 0, sizeof(ops->focused_rect));
    surround = interface_surround_image_peek(
        entity_query_flag_count(state.current_event) - 1);
    if (state.current_event != 0 && state.current_event != 15 &&
        surround != NULL && interface_back_button_dimensions(&width, &height)) {
        ops->focused_rect.x = (float)surround->width;
        ops->focused_rect.y = (float)surround->height;
        ops->focused_rect.width = (float)width;
        ops->focused_rect.height = (float)height;
        ops->focused_rect.enabled = 1;
    }
}

int frontend_action_catalog_hit(const EngineContext* engine,
    const GameContext* game, float x, float y, float offset_x, float offset_y,
    uint32_t* action_out, uint32_t* kind_out, uint32_t* entry_out)
{
    FeActionCatalogHitPass pass;
    memset(&pass, 0, sizeof(pass));
    pass.engine = engine;
    pass.x = x;
    pass.y = y;
    pass.position[0] = offset_x;
    pass.position[1] = offset_y;
    if (!catalog_callback_by_id_dispatch(game,
            (int)objective_selection_word_peek(),
            fe_action_catalog_hit_row, &pass)) return 0;
    if (action_out != NULL) *action_out = pass.action;
    if (kind_out != NULL) *kind_out = pass.kind;
    if (entry_out != NULL) *entry_out = pass.entry;
    return 1;
}

/* CPU image composition from AdvisorOverlayUpdate and its 0x41b800 row
 * callback. Mode, sliding origin, images and action state remain owned by
 * their existing producers. The count-text overlay and tracker/dialog tail
 * have separate owners; this adapter does not stand in for those paths. */
typedef struct FeActionCatalogImagePass {
    const EngineContext* engine;
    void* pixels;
    int pitch, width, height;
    float position[2];
} FeActionCatalogImagePass;

static int fe_action_catalog_image_row(int action, int kind, int entry,
                                      void* payload)
{
    FeActionCatalogImagePass* pass = (FeActionCatalogImagePass*)payload;
    TextureSurface* image;
    uint32_t flags;
    if (action == 73) {
        catalog_slot_flag_update(kind, entry);
        image = interface_build_image_binding_value_get(pass->engine,
                                                         kind, entry);
        flags = input_binding_state_word_get(kind, entry);
        player_input_flag8_set_clear(kind, entry, 0);
    }
    else {
        input_action_state_update(action);
        image = interface_action_image_value_get(pass->engine, action);
        flags = input_action_state_word_get(action);
        input_entry_flag8_set_clear(action, 0);
    }
    /* The reference resolves the current image again after consuming flag
     * 8. Pass its typed dimensions; never truncate a host image pointer. */
    advisor_animation_position_and_request_update_image(action, kind, entry,
        pass->position, action == 73
            ? interface_build_image_binding_value_get(pass->engine, kind, entry)
            : interface_action_image_value_get(pass->engine, action));
    if (image != NULL &&
        ((flags & 4u) == 0u || sim_status_banner_word_get() != 0u)) {
        surface_blit_texture(pass->pixels, pass->pitch, pass->width,
            pass->height, image, (int)pass->position[0],
            (int)pass->position[1]);
    }
    pass->position[1] += image != NULL ? (float)image->height : 40.0f;
    return 0;
}

void frontend_action_catalog_images_render(const EngineContext* engine,
    const GameContext* game, void* pixels, int pitch, int width, int height)
{
    EventTimerState state;
    float origin[2];
    int count;
    int has_back;
    const FeSurroundImage* surround;
    FeActionCatalogImagePass pass;
    if (pixels == NULL || width <= 0 || height <= 0) return;
    frontend_event_timer_state_get(&state);
    if ((state.flags & 4u) == 0u) return;
    interface_menu_action_layout_peek(origin);
    origin[0] = state.timer;
    count = entity_query_flag_count(state.current_event);
    /* Config row 1 is stored at source index 0. In the executable the
     * renderer bases its arrays one entry before that first stored row. */
    surround = interface_surround_image_peek(count - 1);
    has_back = state.current_event != 0 && state.current_event != 15;
    if (surround != NULL) {
        TextureSurface* image = has_back ? surround->image
                                         : surround->image_without_back;
        const int x = has_back ? surround->x : surround->without_back_x;
        const int y = has_back ? surround->y : surround->without_back_y;
        if (image != NULL)
            surface_blit_texture(pixels, pitch, width, height, image,
                (int)(origin[0] + (float)x), (int)(origin[1] + (float)y));
    }
    if (has_back) {
        const int flash = (input_action_state_word_get(0) & 4u) != 0u;
        const int blink = sim_status_banner_word_get() != 0u;
        const int pressed = input_entry_flag8_get(0) != 0u;
        TextureSurface* back = NULL;
        if (pressed)
            back = interface_back_button_image(!(flash && blink));
        else if ((flash && blink) ||
                 (!flash && (state.flags & 0x4000u) != 0u))
            back = interface_back_button_image(0);
        if (back != NULL && surround != NULL)
            surface_blit_texture(pixels, pitch, width, height, back,
                (int)(origin[0] + (float)surround->width),
                (int)(origin[1] + (float)surround->height));
        advisor_animation_position_and_request_update_image(0, 0, 0, NULL,
                                                            NULL);
        input_entry_flag8_set_clear(0, 0);
        input_action_state_update(0);
    }
    pass.engine = engine;
    pass.pixels = pixels;
    pass.pitch = pitch;
    pass.width = width;
    pass.height = height;
    pass.position[0] = origin[0];
    pass.position[1] = origin[1];
    (void)catalog_callback_by_id_dispatch(game, state.current_event,
                                         fe_action_catalog_image_row, &pass);
}

/* Keep the address-mapped leaf blind like the corpus; the public wrapper
 * retains the bounds check exercised by the portable API tests. */
// FUNCTION: LEGORR 0x0046b7a0
static void timed_effect_timestamp_reset_corpus(int index)
{
    s_timed_effect_records[index].timestamp = s_timed_effect_tick;
}

void timed_effect_timestamp_reset(int index)
{
    if (index >= 0 && index < TIMED_EFFECT_RECORD_COUNT) {
        timed_effect_timestamp_reset_corpus(index);
    }
    /* deviation: the corpus strides the table blindly at any index. */
}

void* timed_effect_record_storage_get(unsigned int index)
{
    if (index >= TIMED_EFFECT_RECORD_COUNT) return NULL;
    return &s_timed_effect_records[index];
}

void timed_effect_tick_word_set(uint32_t tick)
{
    memcpy(&s_timed_effect_tick, &tick, sizeof(tick));
}
uint32_t timed_effect_tick_word_peek(void)
{
    uint32_t bits;
    memcpy(&bits, &s_timed_effect_tick, sizeof(bits));
    return bits;
}

uint32_t timed_effect_time_peek(int index)
{
    if (index < 0 || index >= TIMED_EFFECT_RECORD_COUNT) {
        return 0u;
    }
    uint32_t bits;
    memcpy(&bits, &s_timed_effect_records[index].timestamp, sizeof(bits));
    return bits;
}

/* ScreenRightEdgeCacheRefresh (FUN_0041a290) — see frontend.hpp. */
// GLOBAL: LEGORR 0x004dcd44
static uint32_t s_screen_right_edge;

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
int bitmap_font_glyph_measure_width(void* font, unsigned char glyph)
{
    const TextureSurface* surface = (const TextureSurface*)font;
    const int glyph_index = (int)glyph - 0x20;
    int cell_width = 0;
    int cell_height = 0;
    uint32_t keys[2];
    if (glyph_index < 0 ||
        glyph_index >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS ||
        !fe_font_cell(surface, &cell_width, &cell_height)) {
        return 0;
    }
    fe_font_keys(surface, cell_width, cell_height, keys);
    return fe_font_advance(surface, cell_width, cell_height,
                           glyph_index, keys);
}

/* --- the panel resource + scroll leaf band (see frontend.hpp) --------------
 * The corpus's unowned panel words (DAT_004ffe68 enable gate, DAT_004ffed8/
 * edc bubble offsets, DAT_00500810..81c handle/dimensions, DAT_005007d8 UI
 * record slot) land here as zero-init statics; both corpus seams (surface
 * blit FUN_0047dcb0, surface load FUN_0047d9d0) are hook boundaries with
 * no ported body — the seam calls record and the load answers NULL (the
 * view object-layer precedent). */
// GLOBAL: LEGORR 0x004ffe68
uint32_t s_panel_enable_word;
static float    s_bubble_offset_x;
static float    s_bubble_offset_y;
// GLOBAL: LEGORR 0x00500810
static void*    s_panel_surface_handle;
// GLOBAL: LEGORR 0x00500814
static float    s_panel_width;
// GLOBAL: LEGORR 0x00500818
static float    s_panel_height;
// GLOBAL: LEGORR 0x0050081c
static uint32_t s_panel_extra;
// GLOBAL: LEGORR 0x005007d8
static void*    s_panel_ui_record;   /* DAT_005007d8 role */
static void*    s_panel_text_slot;   /* DAT_005007dc role: holder-slot address */

// GLOBAL: LEGORR 0x004ffec4
static void*    s_panel_position_slots[5];
// GLOBAL: LEGORR 0x004ffebc
static float    s_panel_position_offset_x;
// GLOBAL: LEGORR 0x004ffec0
static float    s_panel_position_offset_y;

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

/* FUN_0047dcb0's native device path is renderer-owned. The portable HUD
 * loader supplies TextureSurface records, so use the active software render
 * target when one is bound and retain the seam state for callers without a
 * framebuffer. */
static void panel_texture_draw_at_active_target(void* resource, float x,
                                               float y)
{
    DisplaySurface* target = surface_active_render_target_get();
    void* pixels = NULL;
    int pitch = 0;
    if (resource == NULL || target == NULL || target->w <= 0 ||
        target->h <= 0) return;
    if (surface_lock(target, &pixels, &pitch) != 0) return;
    surface_blit_texture(pixels, pitch, target->w, target->h,
        (const TextureSurface*)resource, (int)crt_float_to_long(x),
        (int)crt_float_to_long(y));
    surface_unlock(target, pixels);
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
     * into its local coordinate pair. Its native SurfaceBlitRect path uses
     * the renderer-owned DirectDraw device; portable HUD images are
     * TextureSurface records and draw through the active software target. */
    if (((unsigned char)s_panel_enable_word & 0x10u) != 0u) {
        float position[2];
        position[0] = (float)x + s_bubble_offset_x;
        position[1] = (float)y + s_bubble_offset_y;
        panel_blit_seam_call(resource, position[0], position[1]);
        panel_texture_draw_at_active_target(resource, position[0],
                                             position[1]);
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
// GLOBAL: LEGORR 0x005007f0
static void* s_panel_layout_surface;
// GLOBAL: LEGORR 0x005007f4
static void* s_panel_layout_glyph;
static float s_panel_layout_size[2];     /* DAT_005007f8/7fc role */
// GLOBAL: LEGORR 0x00500800
static uint32_t s_panel_layout_extra;
// GLOBAL: LEGORR 0x00500804
static float s_panel_layout_offset[2];

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

// GLOBAL: LEGORR 0x004de1e4
static uint32_t s_interface_button_surfaces[2];
static TextureSurface* s_interface_button_surface_native[2];
// GLOBAL: LEGORR 0x005007e0
static uint32_t s_advisor_bar_surfaces[3];
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
        TextureSurface* surface = (TextureSurface*)
            string_cache_lookup_or_load(paths[i]);
        const uint32_t handle = (uint32_t)(uintptr_t)surface;
        s_interface_button_surface_native[i] = surface;
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
static uint32_t s_snorm_rows;
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
 * state words in savegame.cpp, addressed through the in-game test. */
extern int s_active_object_ingame;
extern int s_active_object_outgame;

// FUNCTION: LEGORR 0x00416e00
int* active_object_list_word(void)
{
    return is_in_game() != 0 ? &s_active_object_ingame : &s_active_object_outgame;
}

void active_object_list_count_set(int which, int count)
{
    if (which == 0) s_active_object_outgame = count;
    else if (which == 1) s_active_object_ingame = count;
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
    const EntitySlot* first = selection_first(ctx);
    if ((s_inter_flags & 0x80u) == 0u && (s_inter_flags & 0x100u) != 0u) {
        return (code_out != NULL) ? *code_out : 0;
    }
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
// GLOBAL: LEGORR 0x00500820
static float s_overlay_anim_float;

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

// FUNCTION: LEGORR 0x0046b730
static void timed_effect_persistence_set_corpus(int index, int set)
{
    uint32_t* flags = &s_timed_effect_records[index].flags;
    *flags = (set != 0) ? (*flags | 0x10u) : (*flags & ~0x10u);
}

void timed_effect_persistence_set(int index, int set)
{
    if (index >= 0 && index < TIMED_EFFECT_ENTRIES)
        timed_effect_persistence_set_corpus(index, set);
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

void panel_category_bounds_peek(int panel, float* out_x1, float* out_y1,
                                float* out_x2, float* out_y2)
{
    if (panel < 0 || panel >= PANEL_CATEGORY_WINDOWS) {
        return;
    }
    request_record_panel_bounds_peek(panel, out_x1, out_y1, out_x2, out_y2);
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



static int frontend_config_fields(const char* value, char* storage,
                                  unsigned int storage_size,
                                  char** fields, unsigned int field_count,
                                  char separator)
{
    unsigned int i;
    unsigned int found = 1;
    size_t length;
    if (value == NULL || storage == NULL || fields == NULL ||
        storage_size == 0 || field_count == 0) return 0;
    length = strlen(value);
    if (length >= storage_size) return 0;
    memcpy(storage, value, length + 1);
    fields[0] = storage;
    for (i = 0; i < (unsigned int)length; ++i) {
        if (storage[i] == separator) {
            storage[i] = '\0';
            if (found < field_count) fields[found] = storage + i + 1;
            ++found;
        }
    }
    return found == field_count;
}

static int frontend_config_path(char* out, size_t out_size,
                                const char* prefix, const char* group,
                                const char* key)
{
    size_t length = 0;
    const char* parts[3];
    int p;
    parts[0] = prefix;
    parts[1] = group;
    parts[2] = key;
    if (out == NULL || out_size == 0) return 0;
    out[0] = '\0';
    for (p = 0; p < 3; ++p) {
        size_t part_length;
        if (parts[p] == NULL || parts[p][0] == '\0') continue;
        part_length = strlen(parts[p]);
        if (length != 0) {
            if (length + 2 >= out_size) return 0;
            out[length++] = ':';
            out[length++] = ':';
        }
        if (length + part_length >= out_size) return 0;
        memcpy(out + length, parts[p], part_length);
        length += part_length;
        out[length] = '\0';
    }
    return length != 0;
}

static int frontend_config_boolean(const char* value)
{
    if (value == NULL) return 2;
    if (strcasecmp_lite(value, "TRUE") == 0 ||
        strcasecmp_lite(value, "YES") == 0 ||
        strcasecmp_lite(value, "ON") == 0) return 1;
    if (strcasecmp_lite(value, "FALSE") == 0 ||
        strcasecmp_lite(value, "NO") == 0 ||
        strcasecmp_lite(value, "OFF") == 0) return 0;
    return 2;
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

void sav_visited_entry_set(int which, int index, void* record,
                           uint32_t word)
{
    if (index < 0) return;
    sav_tree_visited_entry_set(which != 0 ? 1 : 0, (uint32_t)index,
                               record, word);
}

void sav_visited_entry_peek(int which, int index, void** out_record,
                            uint32_t* out_word)
{
    if (index < 0) return;
    sav_tree_visited_entry_peek(which != 0 ? 1 : 0, (uint32_t)index,
                                out_record, out_word);
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
    return strcasecmp_lite(a, b) == 0;
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
        call _strcmpi
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

namespace unported_ledger_stubs {

static InputDeviceReleaseHooks s_input_device_release_hooks;
static uint32_t s_input_device_release_flags;
static uint32_t s_input_device_system_flags;
static int s_input_device_active_track = -1;

void input_device_release_state_set(uint32_t input_flags,
                                    uint32_t system_flags,
                                    int active_track)
{
    s_input_device_release_flags = input_flags;
    s_input_device_system_flags = system_flags;
    s_input_device_active_track = active_track;
}

void input_device_release_hooks_set(const InputDeviceReleaseHooks* hooks)
{
    if (hooks == NULL) memset(&s_input_device_release_hooks, 0,
                              sizeof(s_input_device_release_hooks));
    else s_input_device_release_hooks = *hooks;
}

// FUNCTION: LEGORR 0x004588e0
void input_device_release(void)
{
    InputDeviceReleaseHooks* hooks = &s_input_device_release_hooks;
    uint32_t flags = s_input_device_release_flags;
    if ((flags & 1u) != 0u) {
        if (hooks->primary_release != NULL) {
            hooks->primary_release(hooks->context);
        }
        s_input_device_release_flags &= ~1u;
        if ((s_input_device_system_flags & 0x800u) != 0u) {
            if (hooks->secondary_reenable != NULL) {
                hooks->secondary_reenable(hooks->context, 1);
            }
            s_input_device_system_flags &= ~0x800u;
        }
    }
    else if ((flags & 2u) != 0u) {
        s_input_device_release_flags &= ~2u;
    }
    else if ((flags & 4u) != 0u) {
        s_input_device_release_flags &= ~0x104u;
    }

    if (hooks->advisor_clear != NULL) hooks->advisor_clear(hooks->context);
    if (s_input_device_active_track != -1) {
        if (hooks->stream_stop != NULL) {
            hooks->stream_stop(hooks->context, 0);
        }
        s_input_device_active_track = -1;
    }
    if (hooks->transition_control != NULL) {
        hooks->transition_control(hooks->context, 0, 0);
    }
    if (hooks->sound_cleanup != NULL) {
        hooks->sound_cleanup(hooks->context, 0);
    }
    if (hooks->system_state_get != NULL &&
        hooks->system_state_get(hooks->context, 0) == 0) {
        s_input_device_system_flags &= ~0x1000u;
    }
}

uint32_t input_device_release_flags_get(void)
{
    return s_input_device_release_flags;
}

uint32_t input_device_release_system_flags_get(void)
{
    return s_input_device_system_flags;
}

int input_device_release_active_track_get(void)
{
    return s_input_device_active_track;
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

static CellGridPointHitTestState* s_cell_grid_point_hit_test_state;

void cell_grid_point_hit_test_state_bind(CellGridPointHitTestState* state)
{
    s_cell_grid_point_hit_test_state = state;
}

static int cell_grid_point_hit_record_size_get(void* context,
                                              uint32_t type,
                                              uint32_t row,
                                              float* width, float* height)
{
    TextureSurface* image;
    (void)context;
    image = (TextureSurface*)input_binding_default_value_get(
        (int)type, (int)row, 0);
    if (image == NULL) return 0;
    if (width != NULL) *width = (float)image->width;
    if (height != NULL) *height = (float)image->height;
    return 1;
}

static int cell_grid_point_hit_live_state_build(
    CellGridPointHitTestState* live_state,
    CellGridPointHitRecord* live_records)
{
    GameContext* game;
    const unported_ledger_stubs::DependencyGridTuple* tuples = NULL;
    uint32_t tuple_count = 0u;
    uint32_t index;
    uint32_t row = cursor_tile_pair_peek(0);
    uint32_t column = cursor_tile_pair_peek(1);
    if (live_state == NULL || live_records == NULL) return 0;
    game = entity_pool_callback_context_get();
    if (game == NULL || game->level == NULL ||
        !unported_ledger_stubs::dependency_grid_cell_record_get_native(
            game->level, (int)row, (int)column, 0, &tuples,
            &tuple_count)) return 0;
    if (tuple_count > DEPENDENCY_GRID_MATCH_CAPACITY)
        tuple_count = DEPENDENCY_GRID_MATCH_CAPACITY;
    for (index = 0u; index < tuple_count; ++index) {
        live_records[index].action = tuples[index].first;
        live_records[index].x = tuples[index].second;
        live_records[index].y = tuples[index].selector;
        live_records[index].reserved = tuples[index].wildcard;
    }
    memset(live_state, 0, sizeof(*live_state));
    live_state->flags = s_inter_flags;
    live_state->records = live_records;
    live_state->record_count = tuple_count;
    live_state->context = NULL;
    live_state->record_size_get = cell_grid_point_hit_record_size_get;
    return 1;
}

// FUNCTION: LEGORR 0x0041ee80
int ledger_stub_0041ee80_CellGridPointHitTest(
    uint32_t x, uint32_t y, float region_x, float region_y,
    uint32_t* action_out, uint32_t* cell_x_out, uint32_t* cell_y_out)
{
    CellGridPointHitTestState* state = s_cell_grid_point_hit_test_state;
    CellGridPointHitTestState live_state;
    CellGridPointHitRecord live_records[DEPENDENCY_GRID_MATCH_CAPACITY];
    float point_x = (float)x;
    const float point_y = (float)y;
    uint32_t index;

    if (action_out == NULL || (s_inter_flags & 0x800u) == 0u) return 0;
    /* The retail caller obtains its 16-byte {type,row,selector,wildcard}
     * list directly from TerrainCellRecordGet using the current cursor pair.
     * LevelState now owns that same list, so build the target's three-word
     * hit-test view from the native-width records for this synchronous call. */
    if (state == NULL) {
        if (!cell_grid_point_hit_live_state_build(&live_state, live_records))
            return 0;
        state = &live_state;
    }
    if (state == NULL || state->record_count == 0u ||
        state->records == NULL) return 0;

    region_x -= 15.0f;
    if (region_x <= point_x && point_x <= region_x + 15.0f &&
        region_y <= point_y && point_y <= region_y + 40.0f) {
        *action_out = 0x14u;
        return 1;
    }

    for (index = 0u; index < state->record_count; ++index) {
        float width;
        float height;
        float left;
        /* The reference selects DAT_004de1e8 for record zero, then
         * DAT_004de1e4 for every later record.  The adapter keeps those two
         * source slots in rectangle_width/height order. */
        const int rectangle = index == 0u ? 1 : 0;
        const CellGridPointHitRecord* record = &state->records[index];

        if (state->rectangles_present[rectangle]) {
            width = state->rectangle_width[rectangle];
            height = state->rectangle_height[rectangle];
        }
        else if (s_interface_button_surface_native[rectangle == 0 ? 1 : 0]
                 != NULL) {
            TextureSurface* image =
                s_interface_button_surface_native[rectangle == 0 ? 1 : 0];
            width = (float)image->width;
            height = (float)image->height;
        }
        else {
            width = 20.0f;
            height = 40.0f;
        }
        left = region_x - width;
        if (left <= point_x && point_x <= left + width &&
            region_y <= point_y && point_y <= region_y + height) {
            *action_out = 0x14u;
            return 1;
        }

        width = 40.0f;
        height = 40.0f;
        /* FUN_0041cb90 receives the first two dwords of the 16-byte
         * record; the third dword is only copied to the caller on a hit. */
        if (!(state->record_size_get != NULL
                  ? state->record_size_get(state->context, record->action,
                                           record->x, &width, &height)
                  : cell_grid_point_hit_record_size_get(
                        NULL, record->action, record->x, &width, &height))) {
            width = 40.0f;
            height = 40.0f;
        }
        region_x = left - width;
        if (region_x <= point_x && point_x <= region_x + width &&
            region_y <= point_y && point_y <= region_y + height) {
            *action_out = record->action;
            if (cell_x_out != NULL) *cell_x_out = record->x;
            if (cell_y_out != NULL) *cell_y_out = record->y;
            return 1;
        }
    }
    return 0;
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

static UiEntryHitTestState* s_ui_entry_hit_test_state;

void ui_entry_hit_test_state_bind(
    unported_ledger_stubs::UiEntryHitTestState* state)
{
    s_ui_entry_hit_test_state = state;
}

// FUNCTION: LEGORR 0x0041a1a0
int ui_entry_hit_test_and_dispatch(
    int x, int y, int click)
{
    UiEntryHitTestState* state = s_ui_entry_hit_test_state;
    uint32_t index, entry_count;
    if (state == NULL || state->entries == NULL || state->count == 0u ||
        state->object_bounds_get == NULL) return 0;
    entry_count = state->count;
    if (entry_count > UI_HIT_ENTRY_CAPACITY)
        entry_count = UI_HIT_ENTRY_CAPACITY;

    for (index = 0u; index < entry_count; ++index) {
        UiEntryHitRecord* entry = &state->entries[index];
        int width = 0;
        int height = 0;
        int top = crt_float_to_long(entry->vertical_offset);
        if (!state->object_bounds_get(state->context, entry->fields[2],
                                      &width, &height)) continue;
        if (x < 0 || x > width || y < top || y > top + height) continue;
        if (click != 0) {
            if ((state->flags & 1u) != 0u && index == 0u) {
                panel_state_flags_toggle(7);
                state->flags &= ~1u;
                return 1;
            }
            if (index != 0u) {
                UiEntryHitRecord selected = *entry;
                uint32_t move;
                for (move = index; move != 0u; --move)
                    state->entries[move] = state->entries[move - 1u];
                state->entries[0] = selected;
            }
            /* RecordListEntryMoveToFront sets both state bits even when the
             * clicked row is already at index zero. */
            state->flags |= 3u;
        }
        return 1;
    }
    return 0;
}

} // namespace unported_ledger_stubs

static UiActionDispatchState* s_ui_action_dispatch_state;

void ui_action_dispatch_state_install(UiActionDispatchState* state)
{
    s_ui_action_dispatch_state = state;
}

static void ui_action_dispatch_mode_request(UiActionDispatchState* state,
                                            int mode, int argument)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->ui_mode_request != NULL)
        ops->ui_mode_request(ops->context, mode, argument);
}

static void ui_action_dispatch_selection_clear(UiActionDispatchState* state)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->selection_clear_request_and_hud_reset != NULL)
        ops->selection_clear_request_and_hud_reset(ops->context);
}

static void ui_action_dispatch_global_low_byte_clear(
    UiActionDispatchState* state)
{
    const UiActionDispatchOps* ops = state->ops;
    if (ops != NULL && ops->global_state_low_byte_clear_and_refresh != NULL)
        ops->global_state_low_byte_clear_and_refresh(ops->context);
    state->action_flags &= ~0xffu;
}

// FUNCTION: LEGORR 0x0041dca0
int ui_action_dispatch(int action)
{
    UiActionDispatchState* state = s_ui_action_dispatch_state;
    const UiActionDispatchOps* ops;
    void* entity = NULL;
    uint32_t flags;
    int type;
    if (state == NULL) return 0;
    ops = state->ops;
    if (ops != NULL && ops->current_selection_get != NULL)
        entity = ops->current_selection_get(ops->context);
    if (ops != NULL && ops->action_already_held != NULL &&
        ops->action_already_held(ops->context, action, 1)) return 0;

    switch (action) {
    case 0:
        if (state->current_mode > 0xf && state->current_mode < 0x12) {
            ui_action_dispatch_mode_request(state, 9, 0);
            return 1;
        }
        if (state->current_mode == 0x12) {
            flags = ops != NULL && ops->entity_flags_get != NULL
                ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
            if ((flags & 0x02000000u) == 0u &&
                (ops == NULL || ops->entity_byte_flags_get == NULL ||
                 (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
                  & 0x40u) == 0u))
                ui_action_dispatch_mode_request(state, 0xb, 0);
            else
                ui_action_dispatch_mode_request(state, 10, 0);
            return 1;
        }
        break;
    case 1:
        if (ops != NULL && ops->cursor_action_pair_request != NULL)
            ops->cursor_action_pair_request(ops->context, 2, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 2: ui_action_dispatch_mode_request(state, 0x13, 0); return 1;
    case 3: ui_action_dispatch_mode_request(state, 0x14, 0); return 1;
    case 4: ui_action_dispatch_mode_request(state, 0x15, 0); return 1;
    case 5:
    case 7:
        if (ops != NULL && ops->tutorial_path_descriptor_create != NULL)
            ops->tutorial_path_descriptor_create(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 6:
        {
            int cell_xy[2];
            cell_xy[0] = state->selected_cell_x;
            cell_xy[1] = state->selected_cell_y;
            /* FUN_00430320 runs before the UI's explicit 0x20000000 clear
             * and CellInitPass. It also owns the work gates and queued
             * coordinate events, so preserve the retail call order here. */
            terrain_cell_work_flag_clear_current(cell_xy, 0x3fu);
        }
        if (ops != NULL && ops->selected_cell_reveal_mask_clear != NULL)
            ops->selected_cell_reveal_mask_clear(
                ops->context, state->selected_cell_x, state->selected_cell_y,
                0x20000000u);
        if (ops != NULL && ops->terrain_cell_update != NULL)
            ops->terrain_cell_update(ops->context, state->selected_cell_x,
                                     state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 9:
        if (ops != NULL && ops->selected_map_action_enqueue != NULL)
            ops->selected_map_action_enqueue(ops->context, 0xc, 0,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xb:
        if (ops != NULL && ops->terrain_cell_command_02510 != NULL)
            ops->terrain_cell_command_02510(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        goto cell_action_finalize;
    case 0xc:
        if (ops != NULL && ops->terrain_cell_command_03030 != NULL)
            ops->terrain_cell_command_03030(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        goto cell_action_finalize;
    case 0xd:
        if (ops != NULL && ops->terrain_cell_command_02d50 != NULL)
            ops->terrain_cell_command_02d50(ops->context,
                state->selected_cell_x, state->selected_cell_y);
cell_action_finalize:
        if (ops != NULL && ops->terrain_cell_action_mode_refresh != NULL)
            ops->terrain_cell_action_mode_refresh(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xe:
        if (ops != NULL && ops->terrain_cell_command_02fb0 != NULL)
            ops->terrain_cell_command_02fb0(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0xf:
        if (ops != NULL && ops->terrain_cell_command_02ae0 != NULL)
            ops->terrain_cell_command_02ae0(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        if (ops != NULL && ops->terrain_cell_action_mode_refresh != NULL)
            ops->terrain_cell_action_mode_refresh(
                ops->context, state->selected_cell_x, state->selected_cell_y);
        if (ops != NULL && ops->terrain_cell_command_complete != NULL)
            ops->terrain_cell_command_complete(ops->context,
                state->selected_cell_x, state->selected_cell_y);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x10:
        flags = ops != NULL && ops->selected_cell_flags_get != NULL
            ? ops->selected_cell_flags_get(ops->context,
                state->selected_cell_x, state->selected_cell_y) : 0u;
        if ((flags & 0x40000000u) == 0u) {
            if (ops != NULL &&
                ops->terrain_descriptor_tiles_reset_and_free != NULL)
                ops->terrain_descriptor_tiles_reset_and_free(ops->context,
                    state->selected_cell_x, state->selected_cell_y);
        }
        else {
            /* ClearSelectionGroup keys the operation by the selected cell. */
            if (ops != NULL && ops->selection_group_clear != NULL)
                ops->selection_group_clear(ops->context,
                    state->selected_cell_x, state->selected_cell_y);
        }
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x11:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        ui_action_dispatch_mode_request(state, 9, 0);
        return 1;
    case 0x12:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        flags = ops != NULL && ops->entity_flags_get != NULL
            ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
        if ((flags & 0x02000000u) == 0u &&
            (ops == NULL || ops->entity_byte_flags_get == NULL ||
             (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
              & 0x40u) == 0u))
            ui_action_dispatch_mode_request(state, 0xb, 0);
        else
            ui_action_dispatch_mode_request(state, 10, 0);
        return 1;
    case 0x13:
        if (ops != NULL && ops->input_request_enqueue != NULL) {
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
            ops->input_request_enqueue(ops->context, 1, entity, 0, 0);
        }
        ui_action_dispatch_mode_request(state, 0xd, 0);
        return 1;
    case 0x14:
        flags = entity != NULL
            ? entity_capability_flags_get((const EntitySlot*)entity) : 0u;
        if ((flags & 1u) != 0u) {
            if (ops != NULL && ops->selected_entity_action_02ec0 != NULL)
                ops->selected_entity_action_02ec0(ops->context, entity);
            return 1;
        }
        if ((state->action_flags & 0x10u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x15:
        if ((state->action_flags & 8u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x16:
    case 0x17:
        if (ops != NULL && ops->selected_entities_detach_or_pose != NULL)
            ops->selected_entities_detach_or_pose(ops->context);
        return 1;
    case 0x18:
        if (ops != NULL && ops->ui_action_helper_00452be0 != NULL)
            ops->ui_action_helper_00452be0(ops->context);
        if (ops != NULL && ops->entity_kind_command_issue != NULL)
            ops->entity_kind_command_issue(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x19:
    case 0x1a:
        if (ops != NULL && ops->terrain_cell_to_world_position != NULL)
            ops->terrain_cell_to_world_position(ops->context, entity,
                                               action == 0x19 ? 0 : 1);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x1b:
        if ((state->action_flags & 2u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x1c:
        if ((state->action_flags & 4u) != 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            return 1;
        }
        goto action_flag_set;
    case 0x1d:
        flags = 0x40u;
        goto action_flag_toggle;
    case 0x1e:
        flags = 0x20u;
        goto action_flag_toggle;
    case 0x1f:
        if (ops != NULL && ops->selected_entity_action_1f != NULL)
            ops->selected_entity_action_1f(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x20:
    case 0x28:
    case 0x29:
    case 0x2a:
        if (ops != NULL && ops->selected_entity_action_20_group != NULL)
            ops->selected_entity_action_20_group(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x21:
        if (ops != NULL && ops->selected_entity_action_21 != NULL)
            ops->selected_entity_action_21(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x22:
        if (ops != NULL && ops->entity_flag_80000000_set_clear != NULL)
            ops->entity_flag_80000000_set_clear(ops->context, entity, 1);
        break;
    case 0x23:
        if (ops != NULL && ops->entity_flag_80000000_set_clear != NULL)
            ops->entity_flag_80000000_set_clear(ops->context, entity, 0);
        break;
    case 0x24:
        if (ops != NULL && ops->selected_entity_action_24 != NULL)
            ops->selected_entity_action_24(ops->context);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x25:
        if (ops != NULL && ops->selected_entity_predicate != NULL) {
            if (ops->selected_entity_predicate(ops->context, entity, 0x39cf0)) {
                state->selected_entity_slots[0] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 0, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
            if (ops->selected_entity_predicate(ops->context, entity, 0x39d10)) {
                state->selected_entity_slots[1] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 1, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
            if (ops->selected_entity_predicate(ops->context, entity, 0x39d30)) {
                state->selected_entity_slots[2] = entity;
                if (ops->selected_entity_slot_set != NULL)
                    ops->selected_entity_slot_set(ops->context, 2, entity);
                ui_action_dispatch_selection_clear(state);
                return 1;
            }
        }
        break;
    case 0x26:
    case 0x27:
        if (ops != NULL && ops->entity_action_kind_create != NULL)
            ops->entity_action_kind_create(ops->context, entity,
                                           action == 0x26 ? 6 : 7);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x2c:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 0xb, NULL, 0, 0);
        type = ops != NULL && ops->entity_type_get != NULL
            ? ops->entity_type_get(ops->context, entity) : 0;
        if (type == 1) {
            flags = ops != NULL && ops->entity_flags_get != NULL
                ? ops->entity_flags_get(ops->context, entity, 0x3e8) : 0u;
            if ((flags & 0x02000000u) == 0u &&
                (ops == NULL || ops->entity_byte_flags_get == NULL ||
                 (ops->entity_byte_flags_get(ops->context, entity, 0x3ec)
                  & 0x40u) == 0u))
                ui_action_dispatch_mode_request(state, 0xb, 0);
            else
                ui_action_dispatch_mode_request(state, 10, 0);
            return 1;
        }
        if (type == 2) {
            ui_action_dispatch_mode_request(state, 9, 0);
            return 1;
        }
        if (type == 4) {
            ui_action_dispatch_mode_request(state, 0xd, 0);
            return 1;
        }
        break;
    case 0x2d:
    case 0x2e:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 9, NULL,
                                       action == 0x2d ? 0 : 1, 0);
        ui_action_dispatch_mode_request(state, 0xf, 0);
        return 1;
    case 0x2f:
        if (ops != NULL && ops->selected_entity_action_2f != NULL)
            ops->selected_entity_action_2f(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x30:
        ui_action_dispatch_mode_request(state, 0x10, 0);
        return 1;
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
        if (ops != NULL && ops->selected_entity_action_31_group != NULL) {
            int callback_argument = action == 0x31 ? 0 :
                action == 0x32 ? 1 : action == 0x33 ? 2 :
                action == 0x34 ? 3 : action == 0x35 ? 4 :
                action == 0x36 ? 5 : action == 0x38 ? 6 : 7;
            ops->selected_entity_action_31_group(ops->context,
                                                 callback_argument);
        }
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x39:
        ui_action_dispatch_mode_request(state, 0x11, 0);
        return 1;
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f:
        flags = action == 0x3a ? 4u : action == 0x3b ? 0x10u :
                action == 0x3c ? 0x20u : action == 0x3d ? 1u :
                action == 0x3e ? 2u : 8u;
        if (ops == NULL || ops->entity_action_record_find == NULL)
            return 0;
        {
            void* action_record = ops->entity_action_record_find(
                ops->context, entity, 0, (int)flags);
            if (action_record == NULL) return 0;
            if (ops->entity_action_record_apply != NULL)
                ops->entity_action_record_apply(ops->context, action_record,
                                                (int)flags, 1);
        }
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x40:
        if (ops != NULL && ops->object_command_task_create != NULL)
            ops->object_command_task_create(ops->context, entity, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x41:
        if (ops != NULL && ops->building_upgrade_commit != NULL)
            ops->building_upgrade_commit(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x42:
        ui_action_dispatch_mode_request(state, 0x12, 0);
        return 1;
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
        if (ops != NULL && ops->object_command_task_create != NULL)
            ops->object_command_task_create(ops->context, entity,
                action == 0x43 ? 1 : action == 0x44 ? 0 :
                action == 0x45 ? 2 : 3);
        if (ops != NULL && ops->selected_entity_field_f6_clear != NULL)
            ops->selected_entity_field_f6_clear(ops->context, entity);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x47:
        if ((state->action_flags & 1u) == 0u) {
            ui_action_dispatch_global_low_byte_clear(state);
            state->action_flags |= 1u;
            global_flag_bit0_clear();
            state->cursor_flags &= ~1u;
            ui_action_dispatch_selection_clear(state);
            return 1;
        }
        ui_action_dispatch_global_low_byte_clear(state);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x48:
        if (ops != NULL && ops->input_request_enqueue != NULL)
            ops->input_request_enqueue(ops->context, 3, NULL, 0, 0);
        ui_action_dispatch_selection_clear(state);
        return 1;
    case 0x49:
        if (ops != NULL && ops->cursor_action_rejected != NULL &&
            ops->cursor_action_rejected(ops->context, state->cursor_action_x,
                state->cursor_action_y, 1)) return 0;
        if (ops != NULL && ops->cursor_action_pair_request != NULL)
            ops->cursor_action_pair_request(ops->context,
                state->cursor_action_x, state->cursor_action_y);
        if (state->cursor_action_x == 4) return 1;
        break;
    default:
        break;
    }
    ui_action_dispatch_selection_clear(state);
    return 1;

action_flag_set:
    if (action == 0x14) flags = 0x10u;
    else if (action == 0x15) flags = 8u;
    else if (action == 0x1b) flags = 2u;
    else flags = 4u;
action_flag_toggle:
    ui_action_dispatch_global_low_byte_clear(state);
    state->action_flags |= flags;
    return 1;
}

namespace unported_ledger_stubs {
int ledger_stub_0041dca0_UiActionDispatch(int action)
{
    return ::ui_action_dispatch(action);
}

// FUNCTION: LEGORR 0x00425c30
void ledger_stub_00425c30_TransitionControl(int enabled, uint32_t parameter)
{
    uint32_t* engine_state = engine_current_state_word();

    if ((engine_debug_word_peek() & 2u) != 0u || engine_state == NULL)
        return;

    if (enabled != 0)
        parameter = (~*engine_state >> 20) & 1u;

    if (parameter != 0u) {
        if ((*engine_state & 0x100000u) == 0u) {
            sound_buffer_association_stop_all();
        }
        engine_state = engine_current_state_word();
        if (engine_state != NULL) *engine_state |= 0x100000u;
        engine_debug_word_bit_set(0x1000u, 1);
        return;
    }

    periodic_entity_callback_dispatch(NULL);
    sound_alias_play(7, 1);
    engine_state = engine_current_state_word();
    if (engine_state != NULL) *engine_state &= ~0x100000u;
    engine_debug_word_bit_set(0x1000u, 0);
}

/* Adapter only: GlobalRenderRecordSetFromInput (FUN_00425d50) takes one
 * input entity and operates on DAT_005560e8; this port seam takes explicit
 * render-state dependencies and is not the corpus ABI. */
void ledger_stub_00425d50_GlobalRenderRecordSetFromInput(
    void* record, void* view, const GlobalRenderRecordHooks* hooks)
{
    global_render_record_set_from_input(record, view, hooks);
}

/* Adapter only: UpdateRenderViewportState (FUN_00426250) is a zero-argument
 * global-state routine. The explicit state/hooks API is not its ABI. */
void ledger_stub_00426250_UpdateRenderViewportState(
    RenderViewportState* state, const RenderViewportHooks* hooks)
{
    render_viewport_state_update(state, hooks);
}

} // namespace unported_ledger_stubs

namespace frontend_config {

static int ledger_config_split_limited(char* text, char** fields,
                                       unsigned int field_limit,
                                       const char* separator)
{
    size_t separator_length;
    unsigned int count;
    char* p;
    if (text == NULL || fields == NULL || field_limit == 0u ||
        separator == NULL || separator[0] == '\0' || text[0] == '\0')
        return 0;
    separator_length = strlen(separator);
    fields[0] = text;
    count = 1u;
    /* BlockTextTokenSplit tests the character after the candidate before
     * comparing the separator, so a trailing delimiter stays in the token. */
    for (p = text; p[1] != '\0' && count < field_limit; ++p) {
        if (strncmp(separator, p, separator_length) == 0) {
            *p = '\0';
            fields[count++] = p + separator_length;
        }
    }
    return (int)count;
}

// FUNCTION: LEGORR 0x004345f0
void panels_config_entry_apply(BlockFile* config, int display_width,
                               int display_height)
{
    char path[256];
    BlockRec* section;
    BlockRec* entry;
    if (config == NULL) return;
    sprintf(path, "Panels%ix%i", display_width, display_height);
    section = block_subgroup(config,
        string_path_join(util_root_name(config), path, (const char*)NULL));
    if (section == NULL) return;
    for (entry = section; entry != NULL;
         entry = block_next_sibling(entry)) {
        int panel;
        const char* value = block_value(entry);
        char copy[256];
        char* fields[6];
        int count;
        if (!ui_panel_name_table_lookup(block_name(entry), &panel) ||
            value == NULL) continue;
        strncpy(copy, value, sizeof(copy) - 1u);
        copy[sizeof(copy) - 1u] = '\0';
        count = ledger_config_split_limited(copy, fields, 6u, ",");
        if (count != 5) continue;
        legacy_overlay_image_record_load(fields[0], panel, 4u);
        panel_category_bounds_set(panel, atoi(fields[1]), atoi(fields[2]),
                                  atoi(fields[3]), atoi(fields[4]));
    }
}

// FUNCTION: LEGORR 0x00434710
void panel_button_layout_config_load(BlockFile* config, int display_width,
                                     int display_height)
{
    enum { PANEL_COUNT = 12, BUTTONS_PER_PANEL = 24, LAYOUT_FIELDS = 9 };
    int counts[PANEL_COUNT];
    int slots[PANEL_COUNT][BUTTONS_PER_PANEL];
    char* textures[3][PANEL_COUNT][BUTTONS_PER_PANEL];
    int ids[PANEL_COUNT][BUTTONS_PER_PANEL];
    float geometry[PANEL_COUNT][BUTTONS_PER_PANEL][4];
    char path[256];
    BlockRec* group;
    BlockRec* entry;
    int panel;

    if (config == NULL) return;
    sprintf(path, "Buttons%ix%i", display_width, display_height);
    group = block_subgroup(config,
        string_path_join(util_root_name(config), path, (const char*)NULL));
    if (group == NULL) return;
    memset(counts, 0, sizeof(counts));
    memset(textures, 0, sizeof(textures));
    memset(ids, 0, sizeof(ids));

    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* panel_name = block_name(entry);
        const char* value = block_value(entry);
        char value_copy[512];
        char* fields[LAYOUT_FIELDS];
        int count;
        unsigned int button;
        int row;

        if (panel_name == NULL || value == NULL ||
            !ui_panel_name_table_lookup(panel_name, &panel) ||
            panel < 0 || panel >= PANEL_COUNT ||
            counts[panel] >= BUTTONS_PER_PANEL) continue;
        strncpy(value_copy, value, sizeof(value_copy) - 1u);
        value_copy[sizeof(value_copy) - 1u] = '\0';
        count = ledger_config_split_limited(value_copy, fields,
                                            LAYOUT_FIELDS, ",");
        if (count != LAYOUT_FIELDS ||
            !ui_panel_indexed_name_lookup((unsigned int)panel, fields[0],
                                          &button) ||
            button >= BUTTONS_PER_PANEL) continue;

        row = counts[panel];
        slots[panel][row] = (int)button;
        if (strcasecmp_lite(fields[1], "NULL") != 0)
            textures[0][panel][row] = string_duplicate(fields[1]);
        if (strcasecmp_lite(fields[2], "NULL") != 0)
            textures[1][panel][row] = string_duplicate(fields[2]);
        if (strcasecmp_lite(fields[3], "NULL") != 0)
            textures[2][panel][row] = string_duplicate(fields[3]);
        geometry[panel][row][0] = (float)atoi(fields[4]);
        geometry[panel][row][1] = (float)atoi(fields[5]);
        geometry[panel][row][2] = (float)atoi(fields[6]);
        geometry[panel][row][3] = (float)atoi(fields[7]);
        if (!case_insensitive_name_table_lookup(fields[8], &ids[panel][row]))
            ids[panel][row] = 0;
        ++counts[panel];
    }

    for (panel = 0; panel < PANEL_COUNT; ++panel) {
        panel_texture_slots_allocate_named(
            panel, counts[panel], slots[panel], &geometry[panel][0][0],
            (const char* const*)textures[0][panel],
            (const char* const*)textures[1][panel],
            (const char* const*)textures[2][panel],
            ids[panel]);
    }
    for (panel = 0; panel < PANEL_COUNT; ++panel) {
        int row;
        for (row = 0; row < counts[panel]; ++row) {
            int texture;
            for (texture = 0; texture < 3; ++texture) {
                engine_heap_free(textures[texture][panel][row]);
                textures[texture][panel][row] = NULL;
            }
        }
    }
}

} // namespace frontend_config

namespace frontend_config {

// TextMessagesConfigLoad
// FUNCTION: LEGORR 0x00434af0
void text_messages_config_load(BlockFile* config)
{
    BlockRec* group;
    BlockRec* entry;
    char path[256];
    if (config == NULL) return;
    group = block_subgroup(config, string_path_join(util_root_name(config),
        "TextMessages", (const char*)NULL));
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        int slot;
        const char* label = block_name(entry);
        const char* text = block_value(entry);
        if (ui_message_label_index_find(label, &slot))
            ui_message_text_slot_assign(slot, text != NULL ? text : "");
    }
    group = block_subgroup(config, string_path_join(util_root_name(config),
        "TextMessagesWithImages", (const char*)NULL));
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        int slot;
        const char* label = block_name(entry);
        const char* value = block_value(entry);
        char copy[512];
        char* fields[3];
        int count;
        if (!ui_message_label_index_find(label, &slot) || value == NULL)
            continue;
        strncpy(copy, value, sizeof(copy) - 1u);
        copy[sizeof(copy) - 1u] = '\0';
        fields[0] = copy;
        fields[1] = NULL;
        fields[2] = NULL;
        /* TextMessagesWithImages uses the colon delimiter at DAT_004a1cd4;
         * Panels_* and Buttons* use the separate comma delimiter. */
        count = ledger_config_split_limited(copy, fields, 3u, ":");
        (void)count;
        ui_message_text_slot_assign_with_image(slot, fields[0], fields[1],
                                                fields[2]);
    }
}

} // namespace frontend_config

namespace unported_ledger_stubs {

void ledger_stub_00434da0_AdvisorTooltipAndTooltipSoundConfigLoad(
    BlockFile* config)
{
    advisor_tooltip_and_sound_config_load(config);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// ToolTipIconsConfigLoad
void ledger_stub_00437150_ToolTipIconsConfigLoad(void* config)
{
    tooltip_icons_config_load(config);
}

} // namespace unported_ledger_stubs

static void* frontend_hud_surface_load(BlockFile* cfg, const char* path,
                                       int x, int y, void* context)
{
    (void)cfg;
    (void)x;
    (void)y;
    (void)context;
    return image_surface_load(path, 0u, 0u);
}

static TextureSurface* frontend_hud_panel_texture_load(const char* path,
                                                        void* context)
{
    (void)context;
    return image_surface_load(path, 0u, 0u);
}

static void frontend_hud_surface_release(void* surface, void* context)
{
    (void)context;
    texture_release((TextureSurface*)surface);
}

static void frontend_hud_surface_initialize(void* surface, void* context)
{
    (void)context;
    texture_source_color_key_set((TextureSurface*)surface, 0u, 0u);
}

static uint32_t frontend_hud_config_resource(BlockFile* cfg,
                                              const char* path, char* out,
                                              uint32_t out_size,
                                              void* context)
{
    (void)context;
    if (cfg == NULL || path == NULL || out == NULL || out_size == 0u)
        return 0u;
    if (!block_get_str(cfg, path, out, (size_t)out_size)) {
        out[0] = '\0';
        return 0u;
    }
    return (uint32_t)strlen(out);
}

void frontend_hud_runtime_install(void)
{
    LegacyHudOps ops;
    memset(&ops, 0, sizeof(ops));
    ops.surface_load = frontend_hud_surface_load;
    ops.surface_release = frontend_hud_surface_release;
    ops.panel_texture_load = frontend_hud_panel_texture_load;
    ops.surface_initialize = frontend_hud_surface_initialize;
    ops.config_resource = frontend_hud_config_resource;
    legacy_hud_ops_set(&ops);
}

// FUNCTION: LEGORR 0x00459530
void frontend_render_three_pass_update(BlockFile* values,
                                        const char* config_root)
{
    legacy_health_bar_config_load(values, config_root);
    legacy_hunger_images_position_config_load(values, config_root);
    (void)frontend_bubble_images_position_config_load(values, config_root);
}

// FUNCTION: LEGORR 0x00459a50
int frontend_bubble_images_position_config_load(BlockFile* values,
                                                const char* config_root)
{
    const char* const path = string_path_join(config_root, "ObjInfo",
        "BubbleImagesPosition", (char*)NULL);
    const char* value;
    char storage[128];
    char* fields[2];
    if (values == NULL) return 0;
    value = block_get_strptr(values, path);
    if (value == NULL ||
        !frontend_config_fields(value, storage, sizeof(storage), fields, 2,
                                ':')) {
        return 0;
    }
    bubble_offsets_set((float)atoi(fields[0]), (float)atoi(fields[1]));
    panel_enable_word_set(panel_enable_word_peek() | 0x10u);
    return 1;
}

namespace unported_ledger_stubs {

// RenderThreePassUpdate
void ledger_stub_00459530_RenderThreePassUpdate(BlockFile* values,
                                                 BlockFile* names)
{
    frontend_render_three_pass_update(values,
        util_root_name(names != NULL ? names : values));
}

// HealthBarConfigLoadAndShadeDerive
int ledger_stub_00459560_HealthBarConfigLoadAndShadeDerive(
    BlockFile* values, BlockFile* names)
{
    return legacy_health_bar_config_load(values,
        util_root_name(names != NULL ? names : values));
}

// HungerImagesPositionConfigLoad
int ledger_stub_00459900_HungerImagesPositionConfigLoad(
    BlockFile* values, BlockFile* names)
{
    return legacy_hunger_images_position_config_load(values,
        util_root_name(names != NULL ? names : values));
}

// LoadObjectHungerMeterImages
void ledger_stub_004599c0_LoadObjectHungerMeterImages(BlockFile* values,
                                                       BlockFile* names)
{
    legacy_hunger_images_load(values,
        util_root_name(names != NULL ? names : values));
}

int ledger_stub_00459a50_BubbleImagesPositionConfigLoad(
    BlockFile* values, BlockFile* names)
{
    return frontend_bubble_images_position_config_load(values,
        util_root_name(names != NULL ? names : values));
}

// RenderEntityScreenRing
void ledger_stub_00459b00_RenderEntityScreenRing(void* entity, int x, int y)
{
    legacy_entity_screen_ring_render(entity, x, y);
}

// UiPanelNameTablesInit
void ledger_stub_0045a030_UiPanelNameTablesInit()
{
    ui_panel_name_tables_init();
}

// ScrollPanelControlsInitialize
void ledger_stub_0045a240_ScrollPanelControlsInitialize()
{
    legacy_scroll_panel_controls_initialize();
}

// PanelCategoryInteractionStateReset
void ledger_stub_0045a270_PanelCategoryInteractionStateReset()
{
    legacy_panel_category_interaction_state_reset();
}

// OverlayImageRecordLoad
void ledger_stub_0045a2e0_OverlayImageRecordLoad(const char* path, int panel,
                                                 uint32_t state)
{
    legacy_overlay_image_record_load(path, panel, state);
}

// CaseInsensitiveStringTableLookup
int ledger_stub_0045a370_CaseInsensitiveStringTableLookup(
    const char* name, int* out_index)
{
    return ui_panel_name_table_lookup(name, out_index);
}

// PanelSlotCoordinateHitTest
int ledger_stub_0045a3b0_PanelSlotCoordinateHitTest(int panel, int x, int y)
{
    return legacy_panel_slot_coordinate_hit_test(panel, x, y);
}

// CaseInsensitiveIndexedStringLookup
int ledger_stub_0045a410_CaseInsensitiveIndexedStringLookup(
    unsigned int table_index, const char* name, unsigned int* out_index)
{
    return ui_panel_indexed_name_lookup(table_index, name, out_index);
}

/* Panel-image rendering routes to the image-specific entry implementation;
 * UI text output has its separate 16-byte record adapter below. */
void* frontend_panel_image_slot_entry_allocate(
    int panel, const char* path, const int rect[4], uint32_t flags)
{
    return legacy_panel_image_slot_entry_allocate(panel, path, rect, flags);
}

// ScrollingAdvisorTextAppend
// FUNCTION: LEGORR 0x0045a590
void ledger_stub_0045a590_ScrollingAdvisorTextAppend(void* record,
                                                      const char* format, ...)
{
    va_list args;
    va_start(args, format);
    text_buffer_append_vformat(*(void**)record, 0, format, args);
    va_end(args);
}

// UITextOutputPositionAndDraw
typedef struct UiTextOutputPointerBinding {
    void* record;
    void* text_buffer;
    const float* offsets;
    int owns_text_buffer;
    int panel_index;
} UiTextOutputPointerBinding;

static UiTextOutputPointerBinding s_ui_text_output_bindings[64];
static float s_ui_text_output_panel_offsets[18][8];

static int ui_text_output_binding_install(void* record, void* text_buffer,
                                          const float* offsets, int owns,
                                          int panel_index)
{
    unsigned int i;
    unsigned int free_slot = 64;
    if (record == NULL || text_buffer == NULL || offsets == NULL) return 0;
    for (i = 0; i < 64; ++i) {
        if (s_ui_text_output_bindings[i].record == record) {
            s_ui_text_output_bindings[i].text_buffer = text_buffer;
            s_ui_text_output_bindings[i].offsets = offsets;
            s_ui_text_output_bindings[i].owns_text_buffer = owns;
            s_ui_text_output_bindings[i].panel_index = panel_index;
            return 1;
        }
        if (free_slot == 64 && s_ui_text_output_bindings[i].record == NULL)
            free_slot = i;
    }
    if (free_slot == 64) return 0;
    s_ui_text_output_bindings[free_slot].record = record;
    s_ui_text_output_bindings[free_slot].text_buffer = text_buffer;
    s_ui_text_output_bindings[free_slot].offsets = offsets;
    s_ui_text_output_bindings[free_slot].owns_text_buffer = owns;
    s_ui_text_output_bindings[free_slot].panel_index = panel_index;
    return 1;
}

int ui_text_output_pointer_binding_set(void* record, void* text_buffer,
                                       const float* offsets)
{
    return ui_text_output_binding_install(record, text_buffer, offsets, 0,
                                          -1);
}

int ui_text_output_panel_coordinates_set(int panel, float y_offset,
                                         float x_offset)
{
    uint32_t y_bits, x_bits;
    if (panel < 0 || panel >= 18) return 0;
    memcpy(&y_bits, &y_offset, sizeof(y_bits));
    memcpy(&x_bits, &x_offset, sizeof(x_bits));
    indexed_pair_set(panel, x_bits, y_bits);
    s_ui_text_output_panel_offsets[panel][6] = y_offset;
    s_ui_text_output_panel_offsets[panel][7] = x_offset;
    return 1;
}

/* PanelImageSlotEntryAllocate (FUN_0045a500): allocate the four-word wrapper
 * first, then create its text buffer with the supplied font handle, header
 * rectangle, and capacity. The target record order is text buffer, base Y,
 * base X, and category-coordinate pointer. */
// FUNCTION: LEGORR 0x0045a500
void* ui_text_output_record_allocate(int panel, uintptr_t font_handle,
                                     const float rect[4],
                                     uint32_t buffer_size)
{
    UiTextOutputRecord32* record;
    uint32_t header[4] = { 0u, 0u, 0u, 0u };
    void* text_buffer;
    uint32_t text_token = 0u;
    uint32_t offsets_token = 0u;
    uintptr_t pointer_value;
    if (rect == NULL || panel < 0 || panel >= 18) return NULL;
    memcpy(&header[2], &rect[2], sizeof(rect[2]));
    memcpy(&header[3], &rect[3], sizeof(rect[3]));

    /* The retail order allocates the 16-byte wrapper before its text buffer. */
    record = (UiTextOutputRecord32*)engine_heap_alloc(
        (long)sizeof(UiTextOutputRecord32));
    if (record == NULL) return NULL;
    memset(record, 0, sizeof(*record));
    text_buffer = text_buffer_create(font_handle, header, (long)buffer_size);
    record->base_x = crt_float_to_long(rect[0]);
    record->base_y = crt_float_to_long(rect[1]);

#if defined(_M_IX86) || defined(__i386__)
    text_token = (uint32_t)(uintptr_t)text_buffer;
    pointer_value = (uintptr_t)&s_ui_text_output_panel_offsets[panel][0];
    offsets_token = (uint32_t)pointer_value;
#else
    /* A 32-bit target record cannot carry native pointers on this host;
     * the sidecar is authoritative, so leave the ABI token words empty. */
    (void)pointer_value;
#endif
    record->text_buffer_pointer = text_token;
    record->panel_coordinates_pointer = offsets_token;
    if (text_buffer != NULL &&
        !ui_text_output_binding_install(record, text_buffer,
            s_ui_text_output_panel_offsets[panel], 1, panel)) {
        text_buffer_destroy(text_buffer);
        engine_heap_free(record);
        return NULL;
    }
    return record;
}

/* Adapter for UiOverlayInitSink.element_create: its panel index, geometry,
 * and buffer size feed the retail allocator, while mode comes from the
 * caller-owned typed context. */
void* ui_text_output_element_create(void* context, int panel, int kind,
                                    float x, float y, float width,
                                    float height, int buffer_size)
{
    const UiTextOutputElementCreateContext* create_context =
        (const UiTextOutputElementCreateContext*)context;
    const float rect[4] = { x, y, width, height };
    (void)kind;
    return ui_text_output_record_allocate(
        panel, create_context != NULL ? create_context->font_handle : 0u, rect,
        (uint32_t)buffer_size);
}

/* Explicit port teardown; retail FUN_0045a620 resets the text widget but
 * leaves its wrapper in the owning slot. */
void ui_text_output_record_destroy(void* opaque)
{
    unsigned int i;
    void* text_buffer = NULL;
    int owns_text_buffer = 0;
    if (opaque == NULL) return;
    if (panel_ui_record_peek() == opaque)
        panel_ui_record_set(NULL);
    for (i = 0; i < 64; ++i) {
        if (s_ui_text_output_bindings[i].record == opaque) {
            text_buffer = s_ui_text_output_bindings[i].text_buffer;
            owns_text_buffer = s_ui_text_output_bindings[i].owns_text_buffer;
            memset(&s_ui_text_output_bindings[i], 0,
                   sizeof(s_ui_text_output_bindings[i]));
            break;
        }
    }
#if defined(_M_IX86) || defined(__i386__)
    if (text_buffer == NULL) {
        const UiTextOutputRecord32* record =
            (const UiTextOutputRecord32*)opaque;
        text_buffer = (void*)(uintptr_t)record->text_buffer_pointer;
        owns_text_buffer = 1;
    }
#endif
    if (owns_text_buffer && text_buffer != NULL)
        text_buffer_destroy(text_buffer);
    engine_heap_free(opaque);
}

int ui_text_output_record_slot_allocate(void** holder, int panel,
                                        uintptr_t font_handle,
                                        const float rect[4],
                                        uint32_t buffer_size)
{
    void* record;
    if (holder == NULL) return 0;
    record = ui_text_output_record_allocate(panel, font_handle, rect,
                                            buffer_size);
    if (record == NULL) return 0;
    if (*holder != NULL) ui_text_output_record_destroy(*holder);
    *holder = record;
    panel_text_slot_set(holder);
    return 1;
}

void ui_text_output_record_slot_destroy(void** holder)
{
    if (holder == NULL) return;
    if (*holder != NULL) {
        ui_text_output_record_destroy(*holder);
        *holder = NULL;
    }
    if (panel_text_slot_peek() == holder) panel_text_slot_set(NULL);
}

void ui_text_output_pointer_binding_clear(void* record)
{
    unsigned int i;
    for (i = 0; i < 64; ++i) {
        if (s_ui_text_output_bindings[i].record == record) {
            memset(&s_ui_text_output_bindings[i], 0,
                   sizeof(s_ui_text_output_bindings[i]));
            return;
        }
    }
}

// FUNCTION: LEGORR 0x0045a5b0
void ui_text_output_position_and_draw(void* record, uint32_t start_line,
                                     float line_scale)
{
    const UiTextOutputRecord32* target_record =
        (const UiTextOutputRecord32*)record;
    void* text_buffer = NULL;
    const float* offsets = NULL;
    int panel_index = -1;
    unsigned int binding_index;
    if (record == NULL) return;
    for (binding_index = 0; binding_index < 64; ++binding_index) {
        if (s_ui_text_output_bindings[binding_index].record == record) {
            text_buffer = s_ui_text_output_bindings[binding_index].text_buffer;
            offsets = s_ui_text_output_bindings[binding_index].offsets;
            panel_index = s_ui_text_output_bindings[binding_index].panel_index;
            break;
        }
    }
    if (panel_index >= 0 && panel_index < 18) {
        uint32_t y_bits, x_bits;
        indexed_pair_get(panel_index, &x_bits, &y_bits);
        memcpy(&s_ui_text_output_panel_offsets[panel_index][6], &y_bits,
               sizeof(y_bits));
        memcpy(&s_ui_text_output_panel_offsets[panel_index][7], &x_bits,
               sizeof(x_bits));
    }
#if defined(_M_IX86) || defined(__i386__)
    if (text_buffer == NULL)
        text_buffer = (void*)(uintptr_t)target_record->text_buffer_pointer;
    if (offsets == NULL)
        offsets = (const float*)(uintptr_t)
            target_record->panel_coordinates_pointer;
#else
    if (text_buffer == NULL || offsets == NULL) return;
#endif
    {
        int x, y;
    /* The retail path converts x (+0x1c) before y (+0x18), calling the CRT
     * floor helper before __ftol so coordinates round downward. */
        x = (int)crt_round_to_integral_with_math_error_handling(
            (double)((float)target_record->base_x + offsets[7]));
        y = (int)crt_round_to_integral_with_math_error_handling(
            (double)((float)target_record->base_y + offsets[6]));
        float_pair_at_offset4_set_from_ints(text_buffer, x, y);
    }
    (void)text_buffer_draw_wrapped(text_buffer, start_line, line_scale, NULL);
}

void ui_text_output_record_slot_draw(void* const* holder,
                                     uint32_t start_line, float line_scale)
{
    if (holder != NULL && *holder != NULL)
        ui_text_output_position_and_draw(*holder, start_line, line_scale);
}

// SimCursorInteractionTick
// FUNCTION: LEGORR 0x0045adb0
int ledger_stub_0045adb0_SimCursorInteractionTick(
    const SimCursorInteractionHooks* hooks, int x, int y,
    unsigned int action)
{
    return sim_cursor_interaction_tick(hooks, x, y, action);
}

// SimCategoryCellHitTest
// FUNCTION: LEGORR 0x0045b310
int ledger_stub_0045b310_SimCategoryCellHitTest(
    const SimCategoryHitEntry* entries, uint32_t count, int x, int y,
    int* category_out, int* item_out)
{
    return sim_category_cell_hit_test(entries, count, x, y,
                                      category_out, item_out);
}

} // namespace unported_ledger_stubs

static FrontendPanelRotationControlState s_panel_rotation_control;

static void frontend_panel_rotation_image_load(BlockFile* values,
                                                const char* config_root,
                                                const char* key, int index)
{
    const char* const path = string_path_join(config_root,
        "PanelRotationControl", key, (char*)NULL);
    const char* value = values != NULL ? block_get_strptr(values, path) : NULL;
    TextureSurface* old_surface = s_panel_rotation_control.images[index];
    if (value == NULL) {
        s_panel_rotation_control.images[index] = NULL;
        if (old_surface != NULL) texture_release(old_surface);
        return;
    }

    char storage[FE_PATH_MAX * 2];
    char* fields[3];
    TextureSurface* surface = NULL;
    if (frontend_config_fields(value, storage, sizeof(storage), fields, 3,
                               ',')) {
        surface = image_surface_load(fields[0], 0u, 0u);
        if (surface != NULL) {
            s_panel_rotation_control.image_sizes[index][0] =
                (float)atof(fields[1]);
            s_panel_rotation_control.image_sizes[index][1] =
                (float)atof(fields[2]);
        }
    }
    s_panel_rotation_control.images[index] = surface;
    if (old_surface != NULL) texture_release(old_surface);
}

// FUNCTION: LEGORR 0x0045b9d0
void frontend_panel_rotation_control_config_load(BlockFile* values,
                                                   const char* config_root)
{
    static const char* const image_keys[4] = {
        "LeftImage", "RightImage", "UpImage", "DownImage"
    };
    static const char* const numeric_keys[3] = {
        "CenterPositionX", "CenterPositionY", "Radius"
    };
    int* numeric_values[3];
    int i;
    numeric_values[0] = &s_panel_rotation_control.center_x;
    numeric_values[1] = &s_panel_rotation_control.center_y;
    numeric_values[2] = &s_panel_rotation_control.radius;
    for (i = 0; i < 3; ++i) {
        const char* const path = string_path_join(config_root,
            "PanelRotationControl", numeric_keys[i], (char*)NULL);
        const char* const value = values != NULL
            ? block_get_strptr(values, path) : NULL;
        *numeric_values[i] = value != NULL ? atoi(value) : 1;
    }
    for (i = 0; i < 4; ++i)
        frontend_panel_rotation_image_load(values, config_root,
                                            image_keys[i], i);
}

void frontend_panel_rotation_control_state_get(
    FrontendPanelRotationControlState* state)
{
    if (state != NULL) *state = s_panel_rotation_control;
}

namespace unported_ledger_stubs {

/* Retained adapter entry for callers that provide test/config hooks. The
 * target mapping above is the BlockFile-backed production implementation. */
void ledger_stub_0045b9d0_PanelRotationControlConfigLoad(
    PanelRotationControlState* state,
    const PanelRotationControlHooks* hooks)
{
    if (state == NULL || hooks == NULL) return;
    (void)panel_rotation_control_config_load(state, hooks);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// CameraDragFromCursorApply
// FUNCTION: LEGORR 0x0045bcd0
int ledger_stub_0045bcd0_CameraDragFromCursorApply(
    CameraDragState* state, const CameraDragHooks* hooks, int x, int y,
    float delta)
{
    return camera_drag_from_cursor_apply(state, hooks, x, y, delta);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// UiOverlayAnimationApply
// FUNCTION: LEGORR 0x0045c5f0
void ledger_stub_0045c5f0_UiOverlayAnimationApply(
    const UiOverlayAnimationState* state,
    void (*draw)(void*, void*, int, const float*, const float*),
    void* context)
{
    ui_overlay_animation_apply(state, draw, context);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

} // namespace unported_ledger_stubs

static PointerCursorState s_pointer_cursor_runtime;

void frontend_pointer_config_load(BlockFile* cfg)
{
    char key[256];
    BlockRec* section;
    snprintf(key, sizeof(key), "%s::Pointers", util_root_name(cfg));
    section = cfg != NULL ? block_subgroup(cfg, key) : NULL;
    if (section == NULL && cfg != NULL && block_name(cfg) != NULL &&
        strcasecmp_lite(block_name(cfg), "Pointers") == 0)
        section = block_first_child(cfg);
    pointer_cursor_config_load_target(
        section);
}

static void pointer_cursor_slot_release(PointerCursorSlot* slot)
{
    if (slot == NULL || slot->resource == NULL) return;
    if (slot->animated)
        (void)flic_close((Flic*)slot->resource);
    else
        texture_release((TextureSurface*)slot->resource);
    slot->resource = NULL;
    slot->animated = 0;
    slot->hot_x = 0;
    slot->hot_y = 0;
}

static int pointer_cursor_value_split(char* value, char** fields,
                                      unsigned int capacity)
{
    unsigned int count = 0u;
    char* p;
    if (value == NULL || fields == NULL || capacity == 0u || value[0] == '\0')
        return 0;
    fields[count++] = value;
    for (p = value; *p != '\0' && count < capacity; ++p) {
        if (*p == ',') {
            *p = '\0';
            fields[count++] = p + 1;
        }
    }
    return (int)count;
}

// FUNCTION: LEGORR 0x0045cbd0
int pointer_cursor_config_parse_target(const char* name, int* index_out)
{
    if (s_pointer_cursor_runtime.names[0] == NULL)
        pointer_cursor_registry_init(&s_pointer_cursor_runtime);
    return pointer_cursor_name_resolve(&s_pointer_cursor_runtime, name,
                                       index_out);
}

// FUNCTION: LEGORR 0x0045ca70
void pointer_cursor_config_load_target(BlockRec* first_entry)
{
    BlockRec* entry;
    uint32_t* feature_flags;
    if (s_pointer_cursor_runtime.names[0] == NULL)
        pointer_cursor_registry_init(&s_pointer_cursor_runtime);
    feature_flags = engine_current_feature_word();
    for (entry = first_entry; entry != NULL; entry = block_next_sibling(entry)) {
        const char* entry_name = block_name(entry);
        const char* entry_value = block_value(entry);
        char value_copy[512];
        char* fields[3];
        int field_count;
        int slot_index;
        int animated;
        PointerCursorSlot* slot;
        if (entry_name == NULL || entry_value == NULL) continue;
        if (entry_name[0] == '!') {
            if (feature_flags != NULL &&
                (*feature_flags & 0x02000000u) != 0u) continue;
            ++entry_name;
        }
        if (!pointer_cursor_config_parse_target(entry_name, &slot_index) ||
            slot_index < 0 || slot_index >= POINTER_CURSOR_COUNT)
            continue;
        strncpy(value_copy, entry_value, sizeof(value_copy) - 1u);
        value_copy[sizeof(value_copy) - 1u] = '\0';
        field_count = pointer_cursor_value_split(value_copy, fields, 3u);
        if (field_count < 1) continue;
        slot = &s_pointer_cursor_runtime.slots[slot_index];
        animated = field_count != 1;
        if (animated) {
            Flic* animation = flic_open(fields[0]);
            if (animation == NULL) continue;
            pointer_cursor_slot_release(slot);
            slot->resource = animation;
            slot->animated = 1;
            slot->hot_x = field_count > 1 ? atoi(fields[1]) : 0;
            slot->hot_y = field_count > 2 ? atoi(fields[2]) : 0;
        }
        else {
            TextureSurface* bitmap = image_surface_load(fields[0], 0u, 0u);
            if (bitmap == NULL) continue;
            image_surface_blit_transformed(bitmap, 0.0f, 0.0f, 0.0f,
                                           0.0f, 0.0f, 0.0f);
            pointer_cursor_slot_release(slot);
            slot->resource = bitmap;
            slot->animated = 0;
            slot->hot_x = 0;
            slot->hot_y = 0;
        }
    }
}

static void pointer_cursor_flic_blit(void* pixels, int pitch, int width,
                                     int height, const Flic* animation,
                                     int x, int y, int source_key_enabled)
{
    const uint16_t* frame = flic_frame555(animation);
    const int frame_width = flic_width(animation);
    const int frame_height = flic_height(animation);
    int row;
    if (pixels == NULL || frame == NULL || pitch <= 0 || width <= 0 ||
        height <= 0 || frame_width <= 0 || frame_height <= 0) return;
    for (row = 0; row < frame_height; ++row) {
        const int destination_y = y + row;
        int column;
        if (destination_y < 0 || destination_y >= height) continue;
        for (column = 0; column < frame_width; ++column) {
            const int destination_x = x + column;
            const uint16_t source = frame[(size_t)row * frame_width + column];
            uint8_t* destination;
            uint32_t rgba;
            if (destination_x < 0 || destination_x >= width ||
                (source_key_enabled && source == 0u))
                continue;
            destination = (uint8_t*)pixels +
                (size_t)destination_y * (size_t)pitch +
                (size_t)destination_x * 4u;
            rgba = flic_rgb555_to_rgba(source);
            destination[0] = (uint8_t)(rgba >> 16);
            destination[1] = (uint8_t)(rgba >> 8);
            destination[2] = (uint8_t)rgba;
            destination[3] = 0xffu;
        }
    }
}

/* The software frame already owns its target lock. Keep resource selection
 * and animated/bitmap drawing shared with the legacy target entry point. */
void pointer_cursor_draw_pixels(uint32_t x, uint32_t y, void* pixels,
                                 int pitch, int width, int height)
{
    const int selected = media_pointer_cursor_slot();
    PointerCursorSlot* slot;
    if (pixels == NULL || pitch <= 0 || width <= 0 || height <= 0 ||
        selected < 0 || selected >= POINTER_CURSOR_COUNT)
        return;
    slot = &s_pointer_cursor_runtime.slots[selected];
    if (!slot->animated) {
        TextureSurface* bitmap = (TextureSurface*)slot->resource;
        if (bitmap == NULL)
            bitmap = (TextureSurface*)s_pointer_cursor_runtime.slots[0].resource;
        if (bitmap != NULL)
            surface_blit_texture(pixels, pitch, width, height, bitmap,
                                 (int)x, (int)y);
    }
    else if (slot->resource != NULL) {
        Flic* animation = (Flic*)slot->resource;
        const uint32_t* flags = engine_current_state_word();
        const int source_key_enabled = flags != NULL
            ? (int)((~*flags >> 20) & 1u) : 0;
        if (s_pointer_cursor_runtime.slots[1].resource != NULL)
            surface_blit_texture(pixels, pitch, width, height,
                (TextureSurface*)s_pointer_cursor_runtime.slots[1].resource,
                (int)x, (int)y);
        (void)flic_tick_loop(animation, 1);
        pointer_cursor_flic_blit(pixels, pitch, width, height,
            animation, (int)x + slot->hot_x, (int)y + slot->hot_y,
            source_key_enabled);
    }
}

// FUNCTION: LEGORR 0x0045cc70
void pointer_cursor_draw_target(uint32_t x, uint32_t y)
{
    DisplaySurface* target = surface_active_render_target_get();
    void* pixels = NULL;
    int pitch = 0;
    if (target == NULL || surface_lock(target, &pixels, &pitch) < 0) return;
    pointer_cursor_draw_pixels(x, y, pixels, pitch, target->w, target->h);
    surface_unlock(target, pixels);
}

namespace unported_ledger_stubs {

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// PriorityImagesConfigLoad
uint32_t ledger_stub_0045cdc0_PriorityImagesConfigLoad(
    PriorityConfigState* state, const PriorityImageConfigRow* rows,
    uint32_t count)
{
    return priority_images_config_load(state, rows, count);
}

// PrioritiesConfigLoad
uint32_t ledger_stub_0045cf50_PrioritiesConfigLoad(
    PriorityConfigState* state, const PriorityConfigRow* rows,
    uint32_t count, const PriorityConfigHooks* hooks)
{
    return priorities_config_load(state, rows, count, hooks);
}

// PrioritiesImagePositionsConfigLoad
uint32_t ledger_stub_0045d060_PrioritiesImagePositionsConfigLoad(
    PriorityConfigState* state, const PriorityImagePositionRow* rows,
    uint32_t count)
{
    return priorities_image_positions_config_load(state, rows, count);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

/* Adapter only: InfoPanelStringEntriesInitialize (FUN_004636b0) takes four
 * fixed arguments followed by a variable stack record list and writes a
 * global 0x20-byte table. This explicit table/array resolver has a different
 * ABI and no live global table owner. */
uint32_t ledger_stub_004636b0_InfoPanelStringEntriesInitialize(
    UiInfoPanelStringTable* table, const UiInfoPanelStringInit* entries,
    uint32_t count, void* context, UiInfoPanelStringResolveHook resolve)
{
    return ui_info_panel_string_entries_initialize(table, entries, count,
                                                   context, resolve);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// SimTrackCursorHitTest
// FUNCTION: LEGORR 0x004638a0
int ledger_stub_004638a0_SimTrackCursorHitTest(
    UiTrackCursorState* state, int x, int y, int mode, int allow_mode_change,
    const UiTrackCursorHitHooks* hooks, unsigned int* slot_index_out)
{
    return ui_sim_track_cursor_hit_test(state, x, y, mode,
                                        allow_mode_change, hooks,
                                        slot_index_out);
}

// OverlayTextBufferRefresh
int ledger_stub_00418a00_OverlayTextBufferRefresh(void)
{
    return overlay_text_buffer_refresh();
}

static SimulationPointerDispatchOps s_simulation_pointer_dispatch_ops;

void simulation_pointer_dispatch_ops_set_impl(
    const SimulationPointerDispatchOps* ops)
{
    if (ops == NULL) {
        memset(&s_simulation_pointer_dispatch_ops, 0,
               sizeof(s_simulation_pointer_dispatch_ops));
    }
    else {
        s_simulation_pointer_dispatch_ops = *ops;
    }
}

int ledger_stub_0041ee80_CellGridPointHitTest(
    uint32_t x, uint32_t y, float region_x, float region_y,
    uint32_t* action_out, uint32_t* cell_x_out, uint32_t* cell_y_out);

/* SimulationPointerActionDispatch (FUN_0041baa0): keep the retail gate,
 * clear-on-entry interaction bits, cell-action branch, overlay result flags,
 * and pointer-state transition here. Config text, action-word mapping,
 * renderer feedback, and the later action-specific statistics are supplied
 * by the frontend owner because their tables are not yet owned in this
 * module. A terrain ray test covers the available 3D miss path. */
// FUNCTION: LEGORR 0x0041baa0
int simulation_pointer_action_dispatch_impl(int x, int y, int active,
                                            int suppress_initial,
                                            int count_action)
{
    SimulationPointerDispatchOps* ops =
        &s_simulation_pointer_dispatch_ops;
    uint32_t action = 0u;
    uint32_t cell_x = 0u;
    uint32_t cell_y = 0u;
    uint32_t global_word;
    int hit;
    const char* label;

    if ((s_inter_flags & 0x84u) != 0x84u) {
        s_inter_flags &= ~0x7600u;
        return 0;
    }
    s_inter_flags &= ~0x7600u;

    hit = ledger_stub_0041ee80_CellGridPointHitTest(
        (uint32_t)x, (uint32_t)y, ops->cell_region_x,
        ops->cell_region_y, &action, &cell_x, &cell_y);
    if (hit != 0) {
        s_inter_flags |= 0x1000u;
        label = (ops->action_label_get != NULL) ?
            ops->action_label_get(ops->context, (int)action,
                                  (int)cell_x, (int)cell_y) : NULL;
        if (active == 0 && count_action == 0 && action != 0x14u &&
            ops->feedback != NULL) {
            ops->feedback(ops->context, 4, (int)action, (int)cell_x,
                          (int)cell_y, label, 1u);
        }
        if ((ops->feature_word & 0x100000u) == 0u &&
            ops->global_word_resolve != NULL &&
            ops->global_word_resolve(ops->context, (int)action,
                                     (int)cell_x, (int)cell_y,
                                     &global_word)) {
            /* DAT_004de1f4 is updated by SimulationGlobalWordSet. */
            simtick_set_mute_word(global_word);
        }
        (void)pointer_action_state_dispatch(0, 0, 0, active,
                                            suppress_initial);
        return 1;
    }

    hit = advisor_overlay_hit_resolve(
        (float)x, (float)y,
        ops->focused_rect.enabled != 0 ? &ops->focused_rect : NULL,
        ops->overlay_offset_x, ops->overlay_offset_y,
        ops->overlay_fallback, ops->context, &action, &cell_x, &cell_y);
    if (hit == 0) {
        if (ops->terrain_grid != NULL && ops->ray_hooks != NULL) {
            int terrain_x;
            int terrain_y;
            float hit_xyz[3];
            if (screen_ray_terrain_cell_pick(
                    ops->terrain_grid, ops->ray_hooks, (float)x, (float)y,
                    0u, &terrain_x, &terrain_y, hit_xyz)) {
                if (count_action != 0 && ops->world_hit_apply != NULL) {

                    ops->world_hit_apply(ops->context, terrain_x,
                                         terrain_y, hit_xyz, count_action);
                }
                return 1;
            }
        }
        return 0;
    }

    if (action == 0u) {
        s_inter_flags |= 0x4000u;
        if (active == 0 && count_action == 0 && ops->feedback != NULL)
            ops->feedback(ops->context, 5, 0, (int)cell_x, (int)cell_y,
                          NULL, 1u);
    }
    else {
        if (action == 0x49u) s_inter_flags |= 0x2600u;
        else if (action == 5u || action == 0xeu)
            s_inter_flags |= 0x2200u;
        else s_inter_flags |= 0x0200u;

        label = (ops->action_label_get != NULL) ?
            ops->action_label_get(ops->context, (int)action,
                                  (int)cell_x, (int)cell_y) : NULL;
        if (active == 0 && count_action == 0 && ops->feedback != NULL)
            ops->feedback(ops->context, 4, (int)action, (int)cell_x,
                          (int)cell_y, label, 1u);
        if ((ops->feature_word & 0x100000u) == 0u &&
            ops->global_word_resolve != NULL &&
            ops->global_word_resolve(ops->context, (int)action,
                                     (int)cell_x, (int)cell_y,
                                     &global_word)) {
            simtick_set_mute_word(global_word);
        }
    }

    hit = pointer_action_state_dispatch((int)action, (int)cell_x,
                                        (int)cell_y, active,
                                        suppress_initial);
    if (hit != 0 && count_action != 0) {
        if (ops->action_count != NULL)
            ops->action_count(ops->context, (int)action, (int)cell_y);
        if (ops->action_complete != NULL)
            ops->action_complete(ops->context, (int)action, (int)cell_x,
                                 (int)cell_y);
    }
    return 1;
}

int ledger_stub_0041baa0_SimulationPointerActionDispatch(
    int x, int y, int active, int suppress_initial, int count_action,
    void* main_sim_context)
{
    (void)main_sim_context; /* sixth raw caller argument is unused in the body */
    return simulation_pointer_action_dispatch_impl(
        x, y, active, suppress_initial, count_action);
}

} // namespace unported_ledger_stubs

void simulation_pointer_dispatch_ops_set(
    const SimulationPointerDispatchOps* ops)
{
    unported_ledger_stubs::simulation_pointer_dispatch_ops_set_impl(ops);
}

int simulation_pointer_action_dispatch(int x, int y, int active,
                                       int suppress_initial,
                                       int count_action,
                                       void* main_sim_context)
{
    return unported_ledger_stubs::ledger_stub_0041baa0_SimulationPointerActionDispatch(
        x, y, active, suppress_initial, count_action, main_sim_context);
}

void simulation_pointer_capture_reset(SimulationPointerCapture* capture)
{
    if (capture == NULL) return;
    capture->action = 0x4au;
    capture->kind = capture->entry = 0u;
    capture->begun = 0;
}

int simulation_pointer_capture_dispatch(SimulationPointerCapture* capture,
    int x, int y, int active, int count_action, void* main_sim_context)
{
    uint32_t saved_action = s_pointer_action_type;
    uint32_t saved_kind = s_pointer_action_kind;
    uint32_t saved_entry = s_pointer_action_entry;
    int hit;
    if (capture == NULL) return 0;
    s_pointer_action_type = capture->action;
    s_pointer_action_kind = capture->kind;
    s_pointer_action_entry = capture->entry;
    /* A dependency hit can own the gesture without setting an action latch.
     * Do not let a later hold over another UI target begin a new capture. */
    hit = simulation_pointer_action_dispatch(x, y, active, capture->begun,
                                             count_action, main_sim_context);
    if (active && hit) capture->begun = 1;
    capture->action = s_pointer_action_type;
    capture->kind = s_pointer_action_kind;
    capture->entry = s_pointer_action_entry;
    /* The mapped miss path does not touch its latch. Physical release must
     * still retire our capture, including release off every UI target. */
    if (!active) simulation_pointer_capture_reset(capture);
    s_pointer_action_type = saved_action;
    s_pointer_action_kind = saved_kind;
    s_pointer_action_entry = saved_entry;
    return hit;
}

namespace unported_ledger_stubs {

void ledger_stub_0041c4f0_AdvisorAnimationPositionAndRequestUpdate(
    int position_mode, int input_a, int input_b, float* position)
{
    advisor_animation_position_and_request_update(
        position_mode, input_a, input_b, position);
}

int ledger_stub_0041c7b0_InputActionStateUpdate(const EngineContext* context,
                                                int action, int alternate)
{
    (void)context;
    (void)alternate;
    input_action_state_update(action);
    return 0;
}

// CatalogSlotFlagUpdate
void ledger_stub_0041c800_CatalogSlotFlagUpdate(int category, int slot)
{
    catalog_slot_flag_update(category, slot);
}

void ledger_stub_0041eac0_EventTimerCountdownAdvance(float delta)
{
    event_timer_countdown_advance(delta);
}

// TerrainCommandFeedbackDispatch
static uint32_t terrain_feedback_message_cue_get(void* context,
                                                  int message_id)
{
    (void)context;
    return surface_type_configured_value_at_index(message_id);
}

static void terrain_feedback_message_cue_set(void* context, int message_group,
                                              uint32_t cue)
{
    (void)context;
    timed_effect_cue_request_set(message_group, cue);
}

static void terrain_feedback_message_update(void* context, int message_group)
{
    (void)context;
    timed_effect_update_request_set(message_group);
}

// FUNCTION: LEGORR 0x00428330
void ledger_stub_00428330_TerrainCommandFeedbackDispatch(
    TerrainCommandFeedbackState* state, int details_requested,
    int parameter_three, int parameter_four,
    const TerrainCommandFeedbackHooks* hooks)
{
    TerrainCommandFeedbackHooks runtime_hooks;
    if (hooks != NULL) runtime_hooks = *hooks;
    else memset(&runtime_hooks, 0, sizeof(runtime_hooks));
    if (runtime_hooks.message_cue_get == NULL)
        runtime_hooks.message_cue_get = terrain_feedback_message_cue_get;
    if (runtime_hooks.message_cue_set == NULL)
        runtime_hooks.message_cue_set = terrain_feedback_message_cue_set;
    if (runtime_hooks.message_update == NULL)
        runtime_hooks.message_update = terrain_feedback_message_update;
    terrain_command_feedback_dispatch(state, details_requested,
                                      parameter_three, parameter_four,
                                      &runtime_hooks);
}

// TutorialNerpScriptFileLoad
int ledger_stub_00452df0_TutorialNerpScriptFileLoad(const char* path)
{
    return tutorial_script_file_load(path);
}

// ObjectiveAdvisorTokenLoad
int ledger_stub_00452e70_ObjectiveAdvisorTokenLoad(const char* path)
{
    return tutorial_objective_token_load(path);
}

// AdvisorDisplayStateResourcesRelease
int ledger_stub_00453220_AdvisorDisplayStateResourcesRelease()
{
    return tutorial_display_resources_release();
}

// IndirectHandleResolveInPlace
void ledger_stub_004532e0_IndirectHandleResolveInPlace(uint32_t* value)
{
    tutorial_indirect_handle_resolve(value);
}

// AdvisorScrollStepAdvance
void ledger_stub_00453920_AdvisorScrollStepAdvance()
{
    tutorial_advisor_scroll_step_advance();
}

// CameraFocusTweenUpdate
void ledger_stub_00453da0_CameraFocusTweenUpdate(float delta)
{
    tutorial_camera_focus_tween_update(delta);
}

// SelectedEntityGroundMarkerRender
int ledger_stub_004248b0_SelectedEntityGroundMarkerRender(
    void* entity, void* camera, float red, float green, float blue)
{
    return selected_entity_ground_marker_render(entity, camera, red, green,
                                                blue);
}

// AdvisorInlineTokenRender
void ledger_stub_00456830_AdvisorInlineTokenRender(
    char* source, char* output, int draw)
{
    advisor_inline_token_render(source, output, draw);
}

// InputDeviceRelease
void ledger_stub_004588e0_InputDeviceRelease()
{
    input_device_release();
}

/* PairCountHistogramBuild rolls the last six pairs into a two-row history
 * before producing its local histogram. The original table is owned by the
 * simulation HUD; retain the same 2x6 window here rather than accepting an
 * invented caller-provided array. */
// GLOBAL: LEGORR 0x00500868
static uint32_t s_pair_count_history[2][6];
static uint32_t s_pair_count_histogram[2][6];
static uint32_t s_pair_count_dominant[2];

// PairCountHistogramBuild
// FUNCTION: LEGORR 0x0045c0d0
void ledger_stub_0045c0d0_PairCountHistogramBuild(uint32_t first,
                                                   uint32_t second)
{
    int group;
    int column;
    for (group = 0; group != 2; ++group) {
        for (column = 0; column != 5; ++column)
            s_pair_count_history[group][column] =
                s_pair_count_history[group][column + 1];
    }
    s_pair_count_history[0][5] = first;
    s_pair_count_history[1][5] = second;
    pair_count_histogram_build(&s_pair_count_history[0][0],
                               s_pair_count_histogram,
                               s_pair_count_dominant);
}
} // namespace unported_ledger_stubs
