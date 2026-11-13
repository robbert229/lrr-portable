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
#include "frontend_for_interface.hpp"

static void advisor_text_display_production_install(void);
void frontend_status_flash_tick(float delta);
// The corpus keeps this 74-pointer label table in the runtime-filled band at
// 0x4dced0 (zero-init at image load), so it takes no // GLOBAL annotation:
// static initializers against a zero-filled corpus tail break reccmp-datacmp.
const char* const s_interface_menu_action_labels[] = {
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
/* main_menu_text_item (FUN_004103d0) reads these three dwords individually;
 * dedicated annotated statics carry them (whole-object resolution, the same
 * precedent as the s_fe_flow_* carve-outs). Own storage, zero-init like the
 * corpus's tail. */
// GLOBAL: LEGORR 0x0055613c
extern "C" { void* DAT_0055613c; }
// GLOBAL: LEGORR 0x00556f50
extern "C" { void* DAT_00556f50; }
// GLOBAL: LEGORR 0x00556f54
extern "C" { void* DAT_00556f54; }
/* The five flow words land as own storage: the corpus addresses them as
 * individual dwords, and Reccmp only resolves whole-object operands. */
#if defined(LEGORRR_BACKEND_WINAPI)
// GLOBAL: LEGORR 0x0055701c
int s_fe_flow_quit;
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
// GLOBAL: LEGORR 0x00557020
int s_fe_flow_save_slot;
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
// GLOBAL: LEGORR 0x00557024
int s_fe_flow_ingame;
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
// GLOBAL: LEGORR 0x00557048
int s_fe_flow_level_start;
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
// GLOBAL: LEGORR 0x0055704c
int s_fe_flow_menu_options;
#endif
#if !defined(LEGORRR_BACKEND_WINAPI)
int s_fe_flow[5];
#endif
/* The wipe quad lands as own storage: the corpus addresses each word as
 * an individual dword (objdump-verified 0x5574ec/f0/f4/f8) and Reccmp
 * only resolves whole-object operands. */
// GLOBAL: LEGORR 0x005574ec
uint32_t s_menu_wipe_arm;
// GLOBAL: LEGORR 0x005574f0
uint32_t s_menu_wipe;
// GLOBAL: LEGORR 0x005574f4
uint32_t s_menu_wipe_stop;
// GLOBAL: LEGORR 0x005574f8
float s_menu_wipe_clock;
// GLOBAL: LEGORR 0x005574dc
float s_menu_plane_pan[2];
int s_main_menu_text_item_pending;
/* The original reward-save entry point has no explicit front-end argument.
 * Keep its active menu owner in the module so the mapped entry retains that
 * ABI while the portable frame loop can still use its instance-taking
 * adapter below. */
FrontEnd* s_reward_save_frontend;
static void fe_picker_order(FrontEnd* fe, int tree_index);
static int fe_sav_menu_list_from_config(FrontEnd* fe, int list_id,
                                        int tree_index,
                                        const char* start_level);
char s_back_button_action[FE_NAME_MAX];
FeSurroundImage s_interface_surround_images[FE_SURROUND_IMAGES_MAX];
FeInterfaceImage s_interface_images[FE_INTERFACE_ACTION_COUNT];
FeInterfaceBuildImage* s_interface_build_images[2];
uint32_t s_interface_build_image_counts[2];
#include "gods98/resource.hpp"
#include "entity.hpp"
#include "level.hpp"
/* C linkage for ToolTip_SetSFX, which GAME-021's lego_unit.c calls (issue
 * #544). The frozen header below redeclares it and inherits it. */
extern "C" void timed_effect_cue_request_set_corpus(int index, uint32_t value);
#include "frontend_shared.hpp"
#include "frontend_for_helpwindow.hpp"
#include "frontend_from_helpwindow.hpp"
#include "frontend_for_reward.hpp"
#include "frontend_from_reward.hpp"
#include "frontend_for_panel_unit.hpp"
#include "frontend_from_panel_unit.hpp"
#include "frontend_for_lego.hpp"
#include "frontend_from_lego.hpp"
#include "frontend_from_interface.hpp"
#include "frontend_for_frontend_unit.hpp"
#include "frontend_from_frontend_unit.hpp"
#include "frontend_for_legoobject.hpp"

static int s_toupper(int c)
{
    return (c >= 'a' && c <= 'z') ? toupper((unsigned char)c) : c;
}
int s_strcaseeq(const char* a, const char* b);

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
void fe_display_label(const char* label, char* out, size_t out_size)
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

float s_interface_menu_action_xy[2];
const void* s_interface_menu_action_context;

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

int s_strcaseeq(const char* a, const char* b)
{
    if (a == NULL || b == NULL) {
        return a == b;
    }
    return strcasecmp_lite(a, b) == 0;
}

/* Case-insensitive substring search ("LevelPick" in "Interface\LevelPicker\…"). */
int s_stristr(const char* haystack, const char* needle)
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

/* --- config value helpers (Levels-container aware, like #37) ------------- */

const char* fe_cfg_str(BlockFile* cfg, const char* level,
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

int fe_cfg_int(BlockFile* cfg, const char* level, const char* key,
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

/* --- Item%i value grammar ------------------------------------------------- */

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
void fe_options_write(FrontEnd* fe, FeItem* it)
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
void fe_overlay_close_all(FrontEnd* fe)
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

const FeInterfaceImage* interface_image_peek(int action_index)
{
    return (action_index >= 0 && action_index < FE_INTERFACE_ACTION_COUNT)
        ? &s_interface_images[action_index] : NULL;
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

const FeSurroundImage* interface_surround_image_peek(int index)
{
    return (index >= 0 && index < FE_SURROUND_IMAGES_MAX)
        ? &s_interface_surround_images[index] : NULL;
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

/* Port convenience entry for callers that open submenu zero. */
int frontend_open(FrontEnd* fe, int menu)
{
    return frontend_open_submenu(fe, menu, 0);
}

/* SaveMenuOpenFromReward (FUN_00462030) has no explicit front-end argument;
 * it uses the active menu/global state and enters the reward save picker.
 * The portable frame loop owns input/rendering, so menu_enter opens that same
 * picker and the frame loop advances it one frame at a time. */
namespace reference_target {
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

/* Pointer hit-testing (defined with the render helpers below) uses the same
 * placement arithmetic + measure-font widths as drawing. */
static int fe_item_pick(const FrontEnd* fe, int mx, int my,
                        const TextureSurface* measure, int* out_idx);
static int fe_scroll_measure(const FeSubmenu* sub);

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

/* --- activation -------------------------------------------------------- */

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

/* The zeroed menu that out-of-range lookups return, allocated on first use
 * (#641). As two static sentinels, it and the empty submenu took 431,660
 * bytes of the matching build's .bss. Its first submenu is the empty
 * submenu: all zero, like the menu. */
static const FeMenu* frontend_empty_menu(void)
{
    static FeMenu* s_empty = NULL;
    if (s_empty == NULL) {
        s_empty = (FeMenu*)calloc(1, sizeof(*s_empty));
    }
    return s_empty;
}

static const FeSubmenu* frontend_empty_submenu(void)
{
    const FeMenu* empty = frontend_empty_menu();
    return empty != NULL ? &empty->submenus[0] : NULL;
}

const FeMenu* frontend_menu(const FrontEnd* fe, int menu)
{
    if (fe == NULL || menu < 0 || menu >= FE_MENU_COUNT) {
        return frontend_empty_menu();
    }
    return &fe->menus[menu];
}

const FeSubmenu* frontend_active_submenu(const FrontEnd* fe)
{
    if (fe == NULL || !fe->open || fe->menu < 0 || fe->menu >= FE_MENU_COUNT) {
        return frontend_empty_submenu();
    }
    const FeMenu* m = &fe->menus[fe->menu];
    if (fe->sub < 0 || fe->sub >= m->submenu_count) {
        return frontend_empty_submenu();
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

int fe_text_width(const char* s)
{
    return (int)strlen(s) * (3 * FE_FONT_SCALE + 1);
}

void fe_draw_text(void* px, int pitch, int w, int h, int x, int y,
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

void fe_blit_texture_crop(void* px, int pitch, int w, int h,
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

int fe_font_cell(const TextureSurface* f, int* cw, int* ch)
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
void fe_font_keys(const TextureSurface* f, int cw, int ch,
                         uint32_t keys[2])
{
    if (f->key_count == 2) {
        keys[0] = f->keys[0];
        keys[1] = f->keys[1];
        return;
    }
    keys[0] = keys[1] = fe_font_key(f, cw, ch);
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
int fe_font_advance(const TextureSurface* f, int cw, int ch, int g,
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

int fe_font_text_width(const char* s, const TextureSurface* f)
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

FeFormattedTextTextureResolve s_formatted_text_texture_resolve;

void formatted_text_texture_resolver_set(FeFormattedTextTextureResolve resolve)
{
    s_formatted_text_texture_resolve = resolve;
}

/* The native initializer publishes this list to DAT_0054bf68. */
const char* const s_tooltip_names[38] = {
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

int ui_panel_string_equal_case_insensitive(const char* first,
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

static ToolTipIconConfigHooks s_tooltip_icon_config_hooks;
const char* s_tooltip_primary_names[11] = {
    "LegoObject_ToolType_Drill", "LegoObject_ToolType_Spade",
    "LegoObject_ToolType_Hammer", "LegoObject_ToolType_Spanner",
    "LegoObject_ToolType_Laser", "LegoObject_ToolType_PusherGun",
    "LegoObject_ToolType_BirdScarer", "LegoObject_ToolType_FreezerGun",
    NULL, NULL, NULL
};
const char* s_tooltip_secondary_names[6] = {
    "LegoObject_AbilityType_Pilot", "LegoObject_AbilityType_Sailor",
    "LegoObject_AbilityType_Driver", "LegoObject_AbilityType_Dynamite",
    "LegoObject_AbilityType_Repair", "LegoObject_AbilityType_Scanner"
};
void* s_tooltip_primary_handles[11];
void* s_tooltip_secondary_handles[6];
void* s_tooltip_blank_handle;
void* s_tooltip_cursor_handle;

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

const TextureSurface* s_advisor_tooltip_font;

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

/* Item display position — the MenuTick @ 0x412c00 arithmetic: each item
 * draws at (menu.Position.x + item.x [+ centering], menu.Position.y +
 * item.y); AutoCenter centres the label on the Position column (base_x -
 * text_width/2), otherwise the item's own x anchor is added (the shipped
 * Back buttons are e.g. x=-314 on Position 320:200 -> left edge). Picker
 * rows carry absolute FrontEndX/Y scatter coords (their submenu Position is
 * 0:0). */
int fe_picker_submenu(const FeSubmenu* sub)
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
void* s_current_selection_context;

uint32_t selection_state_flags_peek(void) { return s_sel_state; }

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

/* Corpus records span DAT_0054c000..DAT_0055235f at 0x27c-byte strides;
 * the update walk's exclusive flag bound is DAT_0055235c (record 39's
 * +0x278 field), so the timestamp band has 40 records. Tooltip overlay
 * initialization clears 0x1839 dwords: 39 full records plus the first
 * three words of record 39, leaving its update-bound flag intact. */
/* The port's copy: ToolTip.c's records (0x0054c000) are in tooltip.c's
 * toolTipGlobs (finding 597). */
TimedEffectRecord s_timed_effect_records[TIMED_EFFECT_RECORD_COUNT];
/* DAT_0054bf3c is the tooltip duration threshold (scale * 25). Timed-effect
 * timestamp reset uses these same float bits as its fallback time value. */
float s_timed_effect_tick; /* the port's; 0x0054bf3c is toolTipGlobs.hoverTime */
static int32_t s_layout_position_debug[LAYOUT_POSITION_ENTRIES][2];
float s_help_window_button_rects[LAYOUT_POSITION_ENTRIES][4];
UiModeResourceBlitRectHook s_ui_resource_reposition_hook;
char s_tooltip_info_strings[9][256];
TooltipOverlayState s_tooltip_overlay_state;
AdvisorTutorialIconLoadFn s_tutorial_icon_load;
AdvisorTutorialIconInitializeFn s_tutorial_icon_initialize;
static void (*s_tutorial_icon_release)(void* icon, void* context);
void* s_tutorial_icon_context;
void* s_tutorial_icon;
static AdvisorTextRequestActionFn s_advisor_text_request_action;
static void* s_advisor_text_request_context;
uint32_t s_advisor_text_state_flags;
int s_advisor_text_current_slot = 0x1a;
float s_advisor_text_animation_timer;
float s_advisor_text_animation_reset_timer;
const char* s_advisor_text_request_source;
AdvisorTextAnimationOps s_advisor_text_animation_ops;
static AdvisorTextBoxExtentFn s_advisor_text_box_extent_hook;
static void* s_advisor_text_box_extent_context;
AdvisorTextDisplayOps s_advisor_text_display_ops;
AdvisorInlineTokenOps s_advisor_inline_token_ops;
uint32_t s_advisor_inline_last_sound = 0xffffffffu;
uint32_t s_advisor_inline_token_generation = 0xffffffffu;
float s_advisor_inline_duration;
SelectedGroundMarkerOps s_selected_ground_marker_ops;
PositionTrackerBoxDrawOps s_position_tracker_box_draw_ops;
OverlayTextBufferOps s_overlay_text_buffer_ops;
AdvisorDialogStackOps s_advisor_dialog_stack_ops;
uint32_t s_advisor_dialog_stack_width = 0xffffffffu;
uint32_t s_advisor_dialog_stack_height = 0xffffffffu;
DebugPanelTextOps s_debug_panel_text_ops;
InterfaceSpawnRequestOps s_interface_spawn_request_ops;
InterfaceSpawnRequestState s_interface_spawn_request_state;
void (*s_help_window_sound_clear)(int);
static void (*s_help_render_transition_clear)(int, int);

void timed_effect_update_request_set(int index)
{
    if (index >= 0 && index < TIMED_EFFECT_ENTRIES)
        timed_effect_update_request_set_corpus(index);
}

/* In the matching build tooltip.c's ToolTip_SetContent (0046b520) serves
 * this name (tooltip_link_names.h, finding 597). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
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
#endif

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

int band_mode_peek(void) { return s_band_mode; }
void band_mode_set(int mode) { s_band_mode = mode; }

void ui_mode_resource_reposition_hook_set(
    UiModeResourceBlitRectHook hook)
{
    s_ui_resource_reposition_hook = hook;
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
void* s_message_font;
void* s_message_text_buffers[2];
static AdvisorMessagePointerCapture s_message_pointer_capture;
static int s_message_gameplay_active;
float s_message_panel_y = 409.0f;
float s_message_text_y = 435.0f;
float s_message_text_height = 42.0f;

/* 0x46ad00 grows/shrinks by the actual font line after wrapped output. */
void advisor_message_layout_commit(void)
{
    if (s_message_text_buffers[0] == NULL) return;
    float_pair_at_offset4_set_from_ints(s_message_text_buffers[0],
                                       55, (int)s_message_text_y);
    float_pair_at_offsetc_set_from_uints(s_message_text_buffers[0],
                                        325u, (uint32_t)s_message_text_height);
}

void advisor_message_layout_reset(void)
{
    s_message_panel_y = 409.0f;
    s_message_text_y = 435.0f;
    s_message_text_height = 42.0f;
    s_advisor_text_state_flags &= ~1u;
    advisor_message_layout_commit();
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
    indexed_pair_set(3, position[0], position[1]);
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

void advisor_dialog_stack_ops_set(const AdvisorDialogStackOps* ops)
{
    if (ops != NULL) s_advisor_dialog_stack_ops = *ops;
    else memset(&s_advisor_dialog_stack_ops, 0,
                sizeof(s_advisor_dialog_stack_ops));
    s_advisor_dialog_stack_width = 0xffffffffu;
    s_advisor_dialog_stack_height = 0xffffffffu;
}

void advisor_text_animation_ops_set(const AdvisorTextAnimationOps* ops,
                                    float reset_timer)
{
    if (ops != NULL) s_advisor_text_animation_ops = *ops;
    else memset(&s_advisor_text_animation_ops, 0,
                sizeof(s_advisor_text_animation_ops));
    s_advisor_text_animation_reset_timer = reset_timer;
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

float advisor_inline_token_duration_get(void)
{
    return s_advisor_inline_duration;
}

void selected_ground_marker_ops_set(const SelectedGroundMarkerOps* ops)
{
    if (ops != NULL) s_selected_ground_marker_ops = *ops;
    else memset(&s_selected_ground_marker_ops, 0,
                sizeof(s_selected_ground_marker_ops));
}

void position_tracker_box_draw_ops_set(const PositionTrackerBoxDrawOps* ops)
{
    if (ops != NULL) s_position_tracker_box_draw_ops = *ops;
    else memset(&s_position_tracker_box_draw_ops, 0,
                sizeof(s_position_tracker_box_draw_ops));
}

void overlay_text_buffer_ops_set(const OverlayTextBufferOps* ops)
{
    if (ops != NULL) s_overlay_text_buffer_ops = *ops;
    else memset(&s_overlay_text_buffer_ops, 0,
                sizeof(s_overlay_text_buffer_ops));
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

void tooltip_overlay_state_fill(
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

uint32_t surface_load_handle(const char* path);

void help_window_sound_clear_hook_set(void (*hook)(int))
{
    s_help_window_sound_clear = hook;
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

uint32_t selection_mode_state_peek(void)
{
    return s_sel_mode_state;
}

uint32_t menu_wipe_flags_peek(void)
{
    return s_menu_wipe;
}

void menu_wipe_flags_set(uint32_t flags)
{
    s_menu_wipe = flags;
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

void frontend_status_flash_tick(float delta)
{
    s_status_blink_timer -= delta;
    if (s_status_blink_timer < 0.0f) {
        s_status_banner_word = (uint32_t)(s_status_banner_word == 0u);
        s_status_blink_timer = 15.0f;
    }
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
FeAdvisorCatalogCallback s_advisor_callback_callback;

void advisor_callback_schedule_callback_set(
    FeAdvisorCatalogCallback callback)
{
    s_advisor_callback_callback = callback;
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

void sim_status_banner_word_set(uint32_t value)
{
    s_status_banner_word = value;
}

/* MenuWipeStart (FUN_004103f0) — see frontend.hpp. */
uint32_t s_menu_clock_millis; /* timeGetTime model */

void menu_wipe_arm_set(uint32_t value) { s_menu_wipe_arm = value; }
uint32_t menu_wipe_arm_peek(void) { return s_menu_wipe_arm; }
float menu_wipe_clock_peek(void) { return s_menu_wipe_clock; }

void menu_clock_millis_set(uint32_t millis)
{
    s_menu_clock_millis = millis;
}

void panel_texture_slots_blit(int panel)
{
    panel_texture_slots_render(panel, NULL, 0, 0, 0);
}

/* The native HUD's software draw of one panel's buttons, with the state
 * choice of Panel_Button_DisplayAll (panel_texture_slots_blit_to,
 * FUN_0045a9c0, game/panel_unit.c). The original draws to the back buffer
 * through Image_DisplayScaled, which the port's renderer does not own. */
void panel_texture_slots_render(int panel, void* pixels, int pitch,
                                int width, int height)
{
    const PanelData* data;
    unsigned int i;
    if (panel < 0 || panel >= PANEL_TYPE_COUNT || pixels == NULL ||
        pitch <= 0 || width <= 0 || height <= 0) return;
    data = &panelGlobs.panelTable[panel];
    for (i = 0; i < data->buttonCount && data->buttonList != NULL; ++i) {
        const PanelButton* button = &data->buttonList[i];
        const unsigned int flags = button->flags;
        const int flash = (flags & PANEL_BUTTON_FLAG_BANNER) != 0u &&
                          sim_status_banner_word_get() != 0u;
        const Image* image;
        if ((flags & PANEL_BUTTON_FLAG_HIDDEN) != 0u) continue;
        if ((flags & PANEL_BUTTON_FLAG_TOGGLED) == 0u &&
            (flags & PANEL_BUTTON_FLAG_HIGHLIGHT) != 0u) {
            if ((flags & PANEL_BUTTON_FLAG_BANNER) != 0u && !flash) continue;
            image = button->highlightImage;
        } else if ((flags & (PANEL_BUTTON_FLAG_PRESSED |
                             PANEL_BUTTON_FLAG_TOGGLED)) == 0u) {
            image = flash ? button->highlightImage : button->normalImage;
        } else {
            image = flash ? button->highlightImage : button->pressedImage;
        }
        if (image != NULL && image->surface != NULL)
            surface_blit_texture(pixels, pitch, width, height,
                (const TextureSurface*)image->surface,
                (int)floor((double)(button->x1 + data->xCurr)),
                (int)floor((double)(button->y1 + data->yCurr)));
    }
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
        float ox_bits = 0.0f, oy_bits = 0.0f;
        int i, px, py, count;
        PanelButton* block;
        if ((request_record_flags_peek(panel) & 1u) == 0u) continue;
        count = panel_draw_count_peek(panel);
        block = (PanelButton*)panel_draw_records_peek(panel);
        if (block == NULL) continue;
        indexed_pair_get(panel, &ox_bits, &oy_bits);
        px = x - (int)ox_bits;
        py = y - (int)oy_bits;
        for (i = 0; i < count; ++i) {
            PanelButton* rec = &block[i];
            uint32_t flags;
            int inside = 0;
            flags = rec->flags;
            if (!found) {
                if (panel == 9 && i == 3) {
                    const double dx = (double)rotation.center_x - x;
                    const double dy = (double)rotation.center_y - y;
                    inside = sqrt(dx * dx + dy * dy) < rotation.radius;
                } else {
                    inside = rec->x1 <= (float)px && (float)px < rec->x2 &&
                             rec->y1 <= (float)py && (float)py < rec->y2;
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
            rec->flags = flags;
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
        PanelButton* block = (PanelButton*)panel_draw_records_peek(capture->category);
        unsigned int* word = &block[capture->item].flags;
        uint32_t flags;
        flags = *word;
        flags = (flags & ~6u) | ((flags & 8u) ? 4u : 0u);
        *word = flags;
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
    PanelButton* block;
    uint32_t count, flags, blocked = 0u;
    LevelState* level;
    if (category < 0 || category >= 12 || item < 0 ||
        item >= panel_draw_count_peek(category)) return 0;
    block = (PanelButton*)panel_draw_records_peek(category);
    if (block == NULL) return 0;
    count = (uint32_t)block[item].clickCount;
    ++count;
    block[item].clickCount = (int)count;
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
    float x = 0.0f, y = 0.0f;
    TextureSurface* image = legacy_panel_texture_get(panel);
    indexed_pair_get(panel, &x, &y);
    if (image != NULL)
        surface_blit_texture(pixels, pitch, width, height, image,
            (int)floor((double)x),
            (int)floor((double)y));
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
        if (!panelGlobs.panelTable[panel].isFlic &&
            panelGlobs.panelTable[panel].image != NULL) {
            Image* image = (Image*)panelGlobs.panelTable[panel].image;
            texture_free((TextureSurface*)image->surface);
            free(image);
        }
        panelGlobs.panelTable[panel].image = NULL;
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

uint32_t screen_right_edge_cache_peek(void) { return s_screen_right_edge; }

float    s_bubble_offset_x;
float    s_bubble_offset_y;
static void*    s_panel_text_slot;   /* DAT_005007dc role: holder-slot address */

static int    s_panel_seam_calls;
static void*  s_panel_seam_last_res;
static float  s_panel_seam_last_x;
static float  s_panel_seam_last_y;

void panel_blit_seam_call(void* res, float x, float y)
{
    ++s_panel_seam_calls;
    s_panel_seam_last_res = res;
    s_panel_seam_last_x = x;
    s_panel_seam_last_y = y;
}

void     panel_enable_word_set(uint32_t word) { s_panel_enable_word = word; }
uint32_t panel_enable_word_peek(void) { return s_panel_enable_word; }

float s_panel_layout_size[2];     /* DAT_005007f8/7fc role */

void* panel_layout_surface_peek(void) { return s_panel_layout_surface; }
void* panel_layout_glyph_peek(void) { return s_panel_layout_glyph; }
uint32_t panel_layout_extra_peek(void) { return s_panel_layout_extra; }
float panel_layout_size_peek(int which)
{
    if (which < 0 || which > 1) {
        return 0.0f; /* deviation: the corpus strides the pair blindly */
    }
    return (&panelGlobs.airMeterJuiceOffset.x)[which];
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

uint32_t s_menu_blit_record[16];         /* corpus record window */

uint32_t surface_load_handle(const char* path)
{
    return (uint32_t)(uintptr_t)string_cache_lookup_or_load(path);
}

uint32_t interface_button_surface_peek(int which)
{
    return (which >= 0 && which < 2) ? s_interface_button_surfaces[which] : 0u;
}

uint32_t advisor_bar_surface_peek(int which)
{
    return (which >= 0 && which < 3)
        ? (uint32_t)(uintptr_t)(&panelGlobs.crystalSmallImage)[which] : 0u;
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
float s_snorm_colors[90];                /* DAT band role */
float s_snorm_cells[2][20];              /* stride-16 unit lanes */
float s_snorm_pairs[2][7][2];            /* seven rows of mirrors */
uint32_t s_snorm_rows;
uint32_t s_snorm_sentinels[2];           /* DAT band sentinels */

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

void active_object_list_count_set(int which, int count)
{
    if (which == 0) s_active_object_outgame = count;
    else if (which == 1) s_active_object_ingame = count;
}

void*    s_screen_effect_handle[SCREEN_EFFECT_COUNT];
uint32_t s_screen_effect_flags[SCREEN_EFFECT_COUNT];
int     s_screen_effect_seam_calls;
void*   s_screen_effect_seam_last_handle;
int     s_screen_effect_seam_last_flag;

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

float overlay_anim_float_peek(void)
{
    return s_overlay_anim_float;
}

void overlay_anim_float_install(float value)
{
    s_overlay_anim_float = value;
}

void timed_effect_persistence_set(int index, int set)
{
    if (index >= 0 && index < TIMED_EFFECT_ENTRIES)
        timed_effect_persistence_set_corpus(index, set);
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

void panel_category_bounds_peek(int panel, float* out_x1, float* out_y1,
                                float* out_x2, float* out_y2)
{
    if (panel < 0 || panel >= PANEL_CATEGORY_WINDOWS) {
        return;
    }
    request_record_panel_bounds_peek(panel, out_x1, out_y1, out_x2, out_y2);
}

int frontend_config_fields(const char* value, char* storage,
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

/* --- the encyclopedia config-file window (2026-10-25) --------------------
 * EncyclopediaConfigFilesOpen (FUN_0040e490) — see frontend.hpp. The group
 * pairs (count window, name table) ride the corpus's address pairing:
 * 0x27c/0x1e8, 0x280/0x1ec, 0x284/0x1f0, 0x288/0x1f4. */
void* s_ency_files[4];   /* DAT_004c7e88/ec/90/94 roles */
void* s_ency_crystal;    /* DAT_004c7e98 role */
void* s_ency_ore;        /* DAT_004c7e9c role */

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

InputDeviceReleaseHooks s_input_device_release_hooks;
uint32_t s_input_device_release_flags;
uint32_t s_input_device_system_flags;
int s_input_device_active_track = -1;

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

CellGridPointHitTestState* s_cell_grid_point_hit_test_state;

void cell_grid_point_hit_test_state_bind(CellGridPointHitTestState* state)
{
    s_cell_grid_point_hit_test_state = state;
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

UiEntryHitTestState* s_ui_entry_hit_test_state;

void ui_entry_hit_test_state_bind(
    unported_ledger_stubs::UiEntryHitTestState* state)
{
    s_ui_entry_hit_test_state = state;
}

} // namespace unported_ledger_stubs

UiActionDispatchState* s_ui_action_dispatch_state;

void ui_action_dispatch_state_install(UiActionDispatchState* state)
{
    s_ui_action_dispatch_state = state;
}

namespace unported_ledger_stubs {
int ledger_stub_0041dca0_UiActionDispatch(int action)
{
    return ::ui_action_dispatch(action);
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

UiTextOutputPointerBinding s_ui_text_output_bindings[64];
float s_ui_text_output_panel_offsets[18][8];

int ui_text_output_binding_install(void* record, void* text_buffer,
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
    indexed_pair_set(panel, x_offset, y_offset);
    s_ui_text_output_panel_offsets[panel][6] = y_offset;
    s_ui_text_output_panel_offsets[panel][7] = x_offset;
    return 1;
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
    if (text_buffer == NULL) {
        text_buffer = ((const PanelTextWindow*)opaque)->textWindow;
        owns_text_buffer = 1;
    }
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

void ui_text_output_record_slot_draw(void* const* holder,
                                     uint32_t start_line, float line_scale)
{
    if (holder != NULL && *holder != NULL)
        ui_text_output_position_and_draw(*holder, start_line, line_scale);
}

} // namespace unported_ledger_stubs

FrontendPanelRotationControlState s_panel_rotation_control;

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

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

} // namespace unported_ledger_stubs

PointerCursorState s_pointer_cursor_runtime;

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

// OverlayTextBufferRefresh
int ledger_stub_00418a00_OverlayTextBufferRefresh(void)
{
    return overlay_text_buffer_refresh();
}

SimulationPointerDispatchOps s_simulation_pointer_dispatch_ops;

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

} // namespace unported_ledger_stubs

