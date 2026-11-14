// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-021 -- original translation unit (text 0041f870-00435b20; finding
 * 505): the original's Lego.c, the game's hub (start-up, configuration, the
 * main loop, level loading and the per-frame world update).
 *
 * Rebuilt in the original's shape (ADR 0005) in three parts by address. This
 * file holds part 1 (issue #544): the functions 0041f870-0042b500, in
 * original order, in the C-compatible subset, so the matching build compiles
 * it as C. The native port compiles it as C++. Parts 2 and 3 move the rest
 * of the unit here from lego.cpp.
 *
 * The port's own model of Lego.c (the EngineContext, LevelState, hook tables
 * and the C++ level loader) is a different contract. Its implementations
 * stay, unannotated and under the port's names, in lego.cpp, which becomes
 * the unit's port tail once parts 2 and 3 have drained it. Native callers
 * keep using them. The functions here that keep a port name are the ones
 * whose callers already use the original's contract.
 *
 * The original keeps its state in one Lego_Globs struct (legoGlobs,
 * 005560c0-00556fb8), defined here as that struct (lego_globs.h, finding
 * 653). In the matching build every field is the struct's; natively the
 * fields a C++ port module owns stay that module's globals, and the unit
 * reads them by the port's names. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
#include "legoobject_port_aliases.h"
#include "aitask_port_aliases.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "lego_unit.h"
#include "lego_unit_imports.h"
#include "lego_globs.h"

/* ------------------------------------------------------------------ */
/* State: the unit's .bss and legoGlobs.                               */
/* ------------------------------------------------------------------ */

/* ---- state, group M ---- */

/* legoGlobs (005560c0, 0xef8 bytes): the original's one Lego_Globs struct
 * (lego_globs.h, finding 653). */
// GLOBAL: LEGORR 0x005560c0
struct Lego_Globs legoGlobs;

/* ---- state, group A ---- */

/* Three factors set to 0.2, 0.7 and 1.0 at start-up. */


/* ---- state, group B ---- */

/* Lego.c .data: the rename-text cursor blink timer, the two ambient sound
 * timers and the 3D sound roll-off factor the debug keys adjust. */
// GLOBAL: LEGORR 0x004a3558
static float s_lego_renameCursorTimer = 12.5f;

// GLOBAL: LEGORR 0x004a355c
static float s_lego_ambientSoundTimer5 = 75.0f;

// GLOBAL: LEGORR 0x004a3560
static float s_lego_ambientSoundTimer6 = 250.0f;

// GLOBAL: LEGORR 0x004a3564
static float s_lego_soundRollOff = 1.0f;

/* Lego.c .bss. */
// GLOBAL: LEGORR 0x004de428
static int s_lego_renameCursorOn;

/* The previous frame's key map, for the rename input. */
// GLOBAL: LEGORR 0x004de430
static unsigned char s_lego_renamePrevKeyMap[256];

// GLOBAL: LEGORR 0x004de530
static float s_lego_upgradeTextTimer;

// GLOBAL: LEGORR 0x004de534
static float s_lego_gameSpeedTextTimer;

// GLOBAL: LEGORR 0x004de538
static float s_lego_rollOffTextTimer;

/* legoGlobs fields. */

/* gods98 Input's key maps (Input.c; the port's are not annotated). */
/* ---- state, group C ---- */

/* Lego_HandleWorld's pointer state (Lego.c .bss): the accumulated drag
 * distance, the drag flag, the previous button states, the time the
 * buttons have been up, and the request to forget the previous states. */
// GLOBAL: LEGORR 0x004de418
float s_lego_drag_x;

// GLOBAL: LEGORR 0x004de41c
float s_lego_drag_y;

// GLOBAL: LEGORR 0x004de560
int s_lego_dragging;

// GLOBAL: LEGORR 0x004de564
int s_lego_prev_left_button;

// GLOBAL: LEGORR 0x004de568
int s_lego_prev_right_button;

// GLOBAL: LEGORR 0x004de56c
float s_lego_hold_time;

// GLOBAL: LEGORR 0x004de570
int s_lego_reset_buttons;

/* legoGlobs.viewMain (005560dc). */

/* legoGlobs.cameraTrack (005560e8). */

/* The block a vehicle placement points at, and whether it is valid. */

/* The screen border (pixels) that scrolls the top-down view. */

/* The two objective-panel images and their positions. */

/* The block under the pointer (mouseBlockPos). It follows legoGlobs, outside
 * its 0xef8 bytes, so it keeps its own definition. */
// GLOBAL: LEGORR 0x00556fb8
int legoGlobs_f_556fb8[2];
/* ---- state, group D ---- */

/* legoGlobs.config (005560c0): the loaded Lego.cfg. */

/* The object manager LoadTileMeshes registers the tile meshes with. */

/* The objective tick's first argument. */

/* Not noFallins. */

/* Gates the AllowRename key. */

/* The first mini-figure the object list loads (OListEntityRecordParse sets
 * it); the StartFP view follows it. It is objectGlobs +0x9cb8
 * (legoobject_globs.h, finding 662). Kept, unreferenced, so the symbol
 * counter does not move. Deviation (scoring only). */
extern void* s_lego_f_4e8448;
/* ---- state, group E ---- */

/* legoGlobs.config (005560c0): the game's Lego.cfg block root. */

/* legoGlobs.rootCont (005560d8). */

/* legoGlobs.viewMain (005560dc). */

/* legoGlobs.cameraTrack (005560e8). */

/* legoGlobs.bmpFONT5_HI (00556124). */

/* legoGlobs.bmpToolTipFont (00556128). */

/* legoGlobs.textOnlyWindow (00556140). */

/* legoGlobs.miniFigureData (005561d8): 0x74-byte records. */

/* legoGlobs.FogColourRGB (0055640c). */

/* legoGlobs.HighFogColourRGB (00556418). */

/* legoGlobs+0xe38 (00556ef8): the high-fog colour cycle's period. */

/* legoGlobs.renameInput (00556fa0): the name being typed for a unit. */

/* legoGlobs.renamePosition (00556fa4): its screen position. */

/* gods98 Input's key maps (finding 505's brief): this frame's and the
 * previous frame's, one byte per DirectInput key. */

/* The spotlight's penumbra (Lego.c .data, 004a3568), stepped by the debug
 * keys in lego_main_sim_step. */
// GLOBAL: LEGORR 0x004a3568
float s_lego_spotPenumbra = 0.7f;

/* The high-fog colour cycle's phase. */
// GLOBAL: LEGORR 0x004de53c
float s_lego_highFogPhase;

/* Input's mouse-button word (0076ab3c) as it was last frame. */
// GLOBAL: LEGORR 0x004de540
uint32_t s_lego_prevMouseWord;

/* The spotlight's umbra, stepped by the debug keys. */
// GLOBAL: LEGORR 0x004de544
float s_lego_spotUmbra;

/* ---- state, group F ---- */

/* legoGlobs.rootCont (005560d8): the scene root container. */

/* legoGlobs' ToolTipInfo texts (loaded by 00427e00). */

/* legoGlobs' UpgradeNames texts, one per building level (00427f80). */

/* gods98 Input's key maps (Input.c): the current and the previous frame.
 * The port's s_global_current_keys (input.cpp) is not annotated. */

/* legoGlobs.rockMonsterData (005561dc): the creature types, 0x74 bytes
 * each. */

/* legoGlobs 005560f0: the light container the debug keys move. */

/* legoGlobs.cameraTrack (005560e8): the radar's tracking camera. */

/* legoGlobs 005563f8: the world point the radar was last clicked at. */

/* Lego.c .bss: the radar map's mouse state. */
// GLOBAL: LEGORR 0x004de410
uint32_t s_lego_mouseLastY;
// GLOBAL: LEGORR 0x004de414
uint32_t s_lego_mouseLastX;
// GLOBAL: LEGORR 0x004de54c
float s_lego_radarDragDistance;
// GLOBAL: LEGORR 0x004de550
uint32_t s_lego_radarPrevButton;
// GLOBAL: LEGORR 0x004de554
unsigned int s_lego_radarBlockX;
// GLOBAL: LEGORR 0x004de558
unsigned int s_lego_radarBlockY;
// GLOBAL: LEGORR 0x004de55c
int s_lego_radarDragging;

/* Lego.c .bss: the object and the block the tool tips last hovered. */
// GLOBAL: LEGORR 0x004de208
int s_lego_toolTipBlockX;
// GLOBAL: LEGORR 0x004de20c
int s_lego_toolTipBlockY;
// GLOBAL: LEGORR 0x004de574
struct EntitySlot* s_lego_toolTipObject;

/* Lego.c .bss: the debug keys' toggles and the unit the K key picked. */
// GLOBAL: LEGORR 0x004de578
int s_lego_debugLightToggle;
// GLOBAL: LEGORR 0x004de57c
int s_lego_debugTransformToggle;
// GLOBAL: LEGORR 0x004de584
struct EntitySlot* s_lego_debugRetargetObject;

/* LegoObject.c's state: objectGlobs (004de790), the original's one
 * struct, which legoobject_unit.c defines (legoobject_globs.h, finding
 * 662). Its tool tip icons (004ead78-004eadbc), 004e8448 and the two
 * words Level_Free clears (004eabc4, 004eabc8) are its fields. The
 * include costs three symbols, the three icon definitions that were here;
 * the two words' definitions were redeclarations (lego_unit.h) and cost
 * none. */
#include "legoobject_globs.h"
/* ---- state, group G ---- */

/* legoGlobs.config (005560c0), the struct's first field: see
 * LEGO_GLOBS_SIZE. */

/* A text window GameConfigInit creates (legoGlobs +0x80). */

/* The world position the drag box started at. */

/* Text.c's message timer (00503618). In the matching build it is text.c's
 * textGlobs.timer (lego_unit_imports.h, finding 607); natively the port has
 * no owner for it. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
uint32_t textGlobs_f_503618;
#endif


/* ---- GAME-021 part 2 (issue #574) state ---- */

/* ---- part 2 state, group 2A (0042b510-0042c440): begin ---- */

/* ---- part 2 state, group 2A: end ---- */

/* ---- part 2 state, group 2B (0042c480-0042d460): begin ---- */

/* ---- part 2 state, group 2B: end ---- */

/* ---- part 2 state, group 2C (0042d600-0042dff0): begin ---- */

/* ---- part 2 state, group 2C: end ---- */

/* ---- part 2 state, group 2D (0042e020-0042ef40): begin ---- */

/* ---- part 2 state, group 2D: end ---- */

/* ---- part 2 state, group 2E (0042efc0-0042f6f0): begin ---- */

/* LegoObject.c's 004eabc4 and 004eabc8, two words Level_Free clears, are
 * objectGlobs fields (legoobject_globs.h). The port's ai.cpp keeps an
 * unannotated copy of the second (s_lego_object_tool_tip_gate). */

/* ---- part 2 state, group 2E: end ---- */

/* ---- part 2 state, group 2F (004302b0-00431570): begin ---- */

/* Lego.c's blockValue(level, bx, by): one block of a level's map. */
#define LEGO_2F_BLOCK(level, bx, by) \
    (LEGO_LEVEL_PTR(Lego_Block, (level)->blocks)[(level)->width * (by) + (bx)])
/* A 32-bit pointer word of an original record (Lego_Level +0x88, Lego_Block
 * +0x18) read or written as a pointer. Only the Win32 builds run the code
 * that uses it (see LEGO_LEVEL_PTR). */
#define LEGO_2F_PTR(T, word) ((T*)(uintptr_t)(word))
#define LEGO_2F_PTR_SET(word, value) ((word) = (uint32_t)(uintptr_t)(value))

/* ---- part 2 state, group 2F: end ---- */

/* ---- part 2 state, group 2G (00431580-00432490): begin ---- */

/* ---- part 2 state, group 2G: end ---- */


/* ---- GAME-021 part 3 (issue #587) state ---- */

/* ---- part 3 state, group 3A (004324d0-00433330): begin ---- */

/* ---- part 3 state, group 3A: end ---- */

/* ---- part 3 state, group 3B (00433380-00434530): begin ---- */

/* ---- part 3 state, group 3B: end ---- */

/* ---- part 3 state, group 3C (00434570-00435aa0): begin ---- */

/* ---- part 3 state, group 3C: end ---- */


/* ------------------------------------------------------------------ */
/* Functions, in original address order.                              */
/* ------------------------------------------------------------------ */

/* GlobalObjectField8CIncrement [group M] */
// FUNCTION: LEGORR 0x0041f870
void global_object_field8c_increment(void)
{
    if (legoGlobs_currLevel != NULL) legoGlobs_currLevel->f_08c++;
}

/* GlobalObjectField8CSubtract [group M] */
// FUNCTION: LEGORR 0x0041f880
void global_object_field8c_subtract(int amount)
{
    if (legoGlobs_currLevel != NULL) legoGlobs_currLevel->f_08c -= amount;
}

/* GlobalObjectField98Add [group M] */
// FUNCTION: LEGORR 0x0041f8a0
void global_object_field98_add(int amount)
{
    if (legoGlobs_currLevel != NULL) legoGlobs_currLevel->f_098 += amount;
}

/* GlobalObjectField98Clear [group M] */
// FUNCTION: LEGORR 0x0041f8c0
void global_object_field98_clear_current(void)
{
    if (legoGlobs_currLevel != NULL) legoGlobs_currLevel->f_098 = 0;
}

/* NoOpCallback [group M] */
// FUNCTION: LEGORR 0x0041f8e0
void noop_callback(void)
{
}

/* UpgradeResourceCheck [group M] */
// FUNCTION: LEGORR 0x0041f900
int lego_upgrade_resource_check(int ore)
{
    if (legoGlobs_currLevel != NULL) {
        if (ore) return legoGlobs_currLevel->ore;
        return legoGlobs_currLevel->crystals;
    }
    /* Like the original, no value: eax still holds the null level. */
}

/* LevelStateField9CAdd [group M] */
// FUNCTION: LEGORR 0x0041f920
void level_state_field9c_add(int amount)
{
    if (legoGlobs_currLevel != NULL) legoGlobs_currLevel->f_09c += amount;
}

/* FUN_0041f940 [group M] */
// FUNCTION: LEGORR 0x0041f940
void lego_flags1_bit17_set(int on)
{
    if (on) legoGlobs_flags1 |= 0x20000;
    else legoGlobs_flags1 &= ~0x20000;
}

/* RuntimeFlag40000000Get [group M] */
// FUNCTION: LEGORR 0x0041f970
uint32_t lego_runtime_flag40000000_get(void)
{
    return legoGlobs_flags1 & 0x40000000;
}

/* GlobalObjectField94Increment [group M] */
// FUNCTION: LEGORR 0x0041f980
void global_object_field94_increment(void)
{
    if (legoGlobs_currLevel != NULL) legoGlobs_currLevel->f_094++;
}

/* GlobalObjectFieldA8Increment [group M] */
// FUNCTION: LEGORR 0x0041f990
void global_object_fielda8_increment(void)
{
    if (legoGlobs_currLevel != NULL) legoGlobs_currLevel->f_0a8++;
}

/* LevelResourceBalanceAddAndSound [group M] */
// FUNCTION: LEGORR 0x0041f9a0
void lego_level_resource_balance_add_and_sound(int ore)
{
    if (legoGlobs_currLevel != NULL) {
        if (ore) legoGlobs_currLevel->ore++;
        else legoGlobs_currLevel->crystals++;
    }
    if (ore) ui_overlay_animation_advance(1, 5);
    else ui_overlay_animation_advance(1, 1);
}

/* UpgradeResourceDebit [group M] */
// FUNCTION: LEGORR 0x0041f9e0
void lego_upgrade_resource_debit(int ore, int amount)
{
    if (legoGlobs_currLevel != NULL) {
        if (ore) legoGlobs_currLevel->ore -= amount;
        else legoGlobs_currLevel->crystals -= amount;
    }
    if (ore) ui_overlay_animation_advance(0, 5);
    else ui_overlay_animation_advance(0, amount);
}

/* GameConfigCallbackRegistryInit [group G] */
// FUNCTION: LEGORR 0x0041fa20
void lego_game_config_callback_registry_init(const char* gameName)
{
    LegoMainStateG mainState;

    memset(&legoGlobs_config, 0, LEGO_GLOBS_SIZE);
    s_engine_config_root = gameName;
    window_title_set(gameName);
    mainState.initFunc = lego_game_config_init;
    mainState.mainLoopFunc = lego_engine_tick;
    mainState.shutdownFunc = lego_engine_shutdown;
    if (s_application_mode != 10) {
        mainState.shutdownFunc = lego_engine_shutdown_alt;
    }
    window_runtime_callback_triplet_set((const size_t*)&mainState);
}

/* StartLevelPilots [group G] */
// FUNCTION: LEGORR 0x0041fa80
void lego_start_level_pilots(void)
{
    level_current_owner_get();
    if (!(legoGlobs_flags1 & 0x8000000)) {
        s_start_level_pilot_grid_count =
            // STRING: LEGORR 0x004a356c
            entity_type_grid_record_count_get("Pilot", 0);
        legoGlobs_flags1 |= 0x8000000;
        level_state_stored_value_set(1);
        scatter_spawn_gate_set(1);
        scene_object_state_dispatch_original(1, 1, 1);
        scene_object_state_dispatch_original(2, 1, 1);
        scene_object_state_dispatch_original(8, 1, 1);
        scene_object_state_dispatch_original(0x400, 1, 1);
        lego_transition_control(0, 0);
        selection_clear_request_and_hud_reset();
        if (LEGO_FIELD(&panelGlobs, uint32_t, 0x5c) & 2) {
            panel_state_flags_toggle(0);
            panel_state_flags_toggle(1);
        }
    }
}

/* TimeScaleGet [group M] */
// FUNCTION: LEGORR 0x0041fb40
float EngineTimeScale(void)
{
    return s_scroll_scale;
}

/* GameConfigInit [group A] */
/* Lego_Initialise: the game's init callback (installed by 0x0041fa20).
 * Reads Lego.cfg and sets up the fonts, the views and cameras, the
 * interface, panels, tool tips, sounds and dialogs, then starts the front
 * end or the start level.
 *
 * The original compiled this function (and Lego_LoadMiscObjects after it)
 * without optimisation: every local lives in its stack slot. */
#if defined(_MSC_VER)
#pragma optimize("", off)
#endif
// FUNCTION: LEGORR 0x0041fb50
int lego_game_config_init(void)
{
    float minTiltStr;
    float maxdist;
    float mindist;
    BlockFile* cfgStr;
    unsigned int countStr;
    float tiltMax;
    float gVal;
    float r;
    float b;
    Area2F textRectName;
    unsigned int textureUsageE;
    unsigned int availMemF;
    unsigned int totalMemValue;
    unsigned char* s;
    const char* barName;
    unsigned char* progressWindowStr;
    const char* loadTextVal;
    const char* shutdownScreenName;
    int barDirA;
    Area2F barRect;
    char* argvVal[10];
    char* startMessageI;
    const char* versionName;
    Area2F panelWindow;
    Area2F panelImageWindow;
    float textPauseTimeStr;
    Area2F textImageWnd;
    const char* sharedDir;
    const char* loadScreen;
    char* posArgvStr[10];
    char buff[128];
    const char* imageNameStr;
    char* posString;
    float minDist3D;
    float rollOff3D;
    int indent;
    float cameraAcceleration;
    const char* startLevel;
    Point2F dialogPosValue;
    char* dialogStringC;
    char* dialogTokens[5];
    unsigned int numPartsC;
    Area2F dialogWindow;
    BlockRec* pointerArrayB;
    float camDropOff;
    float cameraSpeedD;
    float maxDist3DName;

    legoGlobs_dragBoxRGB[0] = 0.2f;
    legoGlobs_dragBoxRGB[1] = 0.7f;
    legoGlobs_dragBoxRGB[2] = 1.0f;

    // STRING: LEGORR 0x004a3ee4
    legoGlobs_langSurface_name[0] = "Lego_SurfaceType_Tunnel";
    // STRING: LEGORR 0x004a3ec8
    legoGlobs_langSurface_name[1] = "Lego_SurfaceType_Immovable";
    // STRING: LEGORR 0x004a3eb0
    legoGlobs_langSurface_name[2] = "Lego_SurfaceType_Hard";
    // STRING: LEGORR 0x004a3e98
    legoGlobs_langSurface_name[3] = "Lego_SurfaceType_Medium";
    // STRING: LEGORR 0x004a3e80
    legoGlobs_langSurface_name[4] = "Lego_SurfaceType_Loose";
    // STRING: LEGORR 0x004a3e68
    legoGlobs_langSurface_name[5] = "Lego_SurfaceType_Soil";
    // STRING: LEGORR 0x004a3e50
    legoGlobs_langSurface_name[6] = "Lego_SurfaceType_Lava";
    // STRING: LEGORR 0x004a3e38
    legoGlobs_langSurface_name[7] = "Lego_SurfaceType_Water";
    // STRING: LEGORR 0x004a3e1c
    legoGlobs_langSurface_name[8] = "Lego_SurfaceType_OreSeam";
    // STRING: LEGORR 0x004a3e04
    legoGlobs_langSurface_name[9] = "Lego_SurfaceType_Lake";
    // STRING: LEGORR 0x004a3de4
    legoGlobs_langSurface_name[10] = "Lego_SurfaceType_CrystalSeam";
    // STRING: LEGORR 0x004a3dc4
    legoGlobs_langSurface_name[11] = "Lego_SurfaceType_RechargeSeam";
    // STRING: LEGORR 0x004a3dac
    legoGlobs_langSurface_name[12] = "Lego_SurfaceType_Rubble";
    // STRING: LEGORR 0x004a3d8c
    legoGlobs_langSurface_name[13] = "Lego_SurfaceType_Reinforcement";
    // STRING: LEGORR 0x004a3d74
    legoGlobs_langSurface_name[14] = "Lego_SurfaceType_Path";
    // STRING: LEGORR 0x004a3d58
    legoGlobs_langSurface_name[15] = "Lego_SurfaceType_SlugHole";
    // STRING: LEGORR 0x004a3d38
    legoGlobs_langSurface_name[16] = "Lego_SurfaceType_Undiscovered";
    // STRING: LEGORR 0x004a3d20
    legoGlobs_langSurface_name[17] = "Lego_SurfaceType_Cavern";

    for (countStr = 0; countStr < 18; countStr++) {
    }

    lego_game_speed_set(1.0f);
    keyboard_key_names_init();
    scene_resource_slots_initialize();
    ai_activity_name_table_initialize();
    renderer_record_table_initialize();
    flic_opcode27_fallback_noop();
    screen_coord_normalize_init();
    ui_panel_name_tables_init();
    cmd_registry_init_target();
    message_string_table_initialize();
    scene_effect_registry_reset();
    ui_pointer_cursor_name_registry_init();
    bubble_action_strings_initialize();

    // STRING: LEGORR 0x004a3d00
    s_lego_text_window_font = (struct TextureSurface*)font_atlas_record_create("Interface\\Fonts\\FONT5_HI.bmp");
    // STRING: LEGORR 0x004a3ce0
    s_lego_crystal_count_font = (struct TextureSurface*)font_atlas_record_create("Interface\\Fonts\\ToolTipFont.bmp");
    // STRING: LEGORR 0x004a3cc0
    legoGlobs_fontDesktop = font_atlas_record_create("Interface\\Fonts\\DeskTopFont.bmp");
    // STRING: LEGORR 0x004a3ca0
    legoGlobs_fontTextWindow = font_atlas_record_create("Interface\\Fonts\\font5_HI.bmp");
    // STRING: LEGORR 0x004a3c80
    legoGlobs_fontBriefingHi = font_atlas_record_create("Interface\\Fonts\\MBriefFONT.bmp");
    // STRING: LEGORR 0x004a3c60
    legoGlobs_fontBriefingLo = font_atlas_record_create("Interface\\Fonts\\MBriefFONT2.bmp");
    // STRING: LEGORR 0x004a3c44
    DAT_0055613c = font_atlas_record_create("Interface\\Fonts\\RSFont.bmp");

    incident_message_catalog_init((uint32_t)(uintptr_t)s_lego_text_window_font);
    interface_initialise(565, 18, s_lego_crystal_count_font);

    if ((legoGlobs_rootCont = container_initialise_original(s_engine_config_root)) != NULL) {
        scene_end_hook_register((void*)lego_object_trigger_frame_callback_adapter, NULL);

        // STRING: LEGORR 0x004a3c38
        legoGlobs_config = block_open("Lego.cfg");
        cfgStr = legoGlobs_config;
        if (cfgStr != NULL) {
            if (!config_rgb_vector_read(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3c2c
                "ToolTipRGB", 0), &r, &gVal, &b)) {
                r = gVal = b = 0.3254902f;
            }
            tooltip_overlay_state_initialize(s_lego_crystal_count_font, 2, 1, 1.0f, display_width_get(), display_height_get(), 32, r, gVal, b);

            DAT_00556f50 = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3c1c
                "CreditsTextFile", 0));
            DAT_00556f54 = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3c0c
                "CreditsBackAVI", 0));

            if ((legoGlobs_BuildingUpgradeCostOre = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3bf4
                "BuildingUpgradeCostOre", 0))) == 0) legoGlobs_BuildingUpgradeCostOre = 5;
            if ((legoGlobs_BuildingUpgradeCostStuds = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3bd8
                "BuildingUpgradeCostStuds", 0))) == 0) legoGlobs_BuildingUpgradeCostStuds = 1;

            if ((legoGlobs_RenameReplace = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3bc8
                "RenameReplace", 0))) != NULL) {
                for (s = (unsigned char*)legoGlobs_RenameReplace; *s != '\0'; s++) {
                    if (*s == '_') *s = ' ';
                }
            }

            if (directdraw_get_avail_texture_mem(&totalMemValue, &availMemF)) {
                if ((textureUsageE = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3bb8
                    "TextureUsage", 0))) != 0) {
                    directdraw_adjust_texture_usage(&textureUsageE);
                    if (textureUsageE <= availMemF) low_texture_flag_set();
                }
            }

            textRectName.x = 55.0f;
            textRectName.y = 435.0f;
            textRectName.width = 325.0f;
            textRectName.height = 42.0f;
            legoGlobs_textOnlyWindow = text_buffer_create((uintptr_t)legoGlobs_fontTextWindow, (const uint32_t*)&textRectName, 1024);
            textImageWnd.x = 55.0f;
            textImageWnd.y = 435.0f;
            textImageWnd.width = 260.0f;
            textImageWnd.height = 42.0f;
            legoGlobs_textImageWindow = text_buffer_create((uintptr_t)legoGlobs_fontTextWindow, (const uint32_t*)&textImageWnd, 1024);

            textPauseTimeStr = (textPauseTimeStr = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3ba8
                "TextPauseTime", 0))) * 25.0f;
            ui_message_strings_initialize(legoGlobs_textOnlyWindow, legoGlobs_textImageWindow, (void*)316, (void*)425, textPauseTimeStr);

            panelWindow.x = 55.0f;
            panelWindow.y = 435.0f;
            panelWindow.width = 325.0f;
            panelWindow.height = 42.0f;
            panelImageWindow.x = 55.0f;
            panelImageWindow.y = 363.0f;
            panelImageWindow.width = 325.0f;
            panelImageWindow.height = 140.0f;
            // STRING: LEGORR 0x004a3b84
            text_message_panel_initialise_adapter("Interface\\MessagePanel\\MsgPanel.bmp", 42, 409, 338, &panelWindow, &panelImageWindow, 20.0f);

            if (s_engine_feature_word & 0x200000) {
                startMessageI = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3b74
                    "StartMessage", 0));
                versionName = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main", "Version", 0));
                if (startMessageI != NULL) {
                    startMessageI = mission_replace_char(startMessageI, '_', ' ');
                    if (versionName) {
                        // STRING: LEGORR 0x004a3b68
                        text_buffer_append_format(legoGlobs_textOnlyWindow, "\n%s (%s)", startMessageI, versionName);
                    } else {
                        // STRING: LEGORR 0x004a3b64
                        text_buffer_append_format(legoGlobs_textOnlyWindow, "\n%s", startMessageI);
                    }
                    engine_memory_free(startMessageI);
                }
            }

            if (frontend_enabled_original() && movie_avis_enabled_original()) {
                // STRING: LEGORR 0x004a3b5c
                front_play_intro_movie_adapter("LegoAvi", engine_app_mode_get() != 0);
                front_play_intro_splash_adapter(
                    // STRING: LEGORR 0x004a3b48
                    "DDILogo", 1,
                    // STRING: LEGORR 0x004a3b50
                    "DDILogoTime");
                // STRING: LEGORR 0x004a3b40
                front_play_intro_movie_adapter("DDIAvi", 1);
                // STRING: LEGORR 0x004a3b38
                front_play_intro_movie_adapter("RRAvi", 1);
                front_play_intro_splash_adapter(
                    // STRING: LEGORR 0x004a3b24
                    "LMILogo", engine_app_mode_get() != 0,
                    // STRING: LEGORR 0x004a3b2c
                    "LMILogoTime");
            }

            sound_alias_registry_init();

            barName = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3b18
                "ProgressBar", 0));
            loadScreen = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3b0c
                "LoadScreen", 0));
            shutdownScreenName = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3afc
                "ShutdownScreen", 0));
            progressWindowStr = (unsigned char*)block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3aec
                "ProgressWindow", 0));
            loadTextVal = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a3ae0
                "LoadingText", 0));

            if (progressWindowStr != NULL) {
                progressWindowStr[0] = (unsigned char)toupper(progressWindowStr[0]);
                switch (progressWindowStr[0]) {
                case 'U': barDirA = 0; break;
                case 'R': barDirA = 1; break;
                case 'D': barDirA = 2; break;
                case 'L': barDirA = 3; break;
                default: barDirA = 4; break;
                }
                if (sound_variant_split((char*)progressWindowStr, argvVal, ",") == 4 && progressWindowStr[1] == ':' && barDirA != 4) {
                    barRect.x = (float)atoi(argvVal[0] + 2);
                    barRect.y = (float)atoi(argvVal[1]);
                    barRect.width = (float)atoi(argvVal[2]);
                    barRect.height = (float)atoi(argvVal[3]);
                }
            } else {
                barName = NULL;
            }
            engine_memory_free(progressWindowStr);

            loader_initialise_adapter(loadScreen, shutdownScreenName, s_lego_text_window_font,
                media_cd_audio_playback_available() ?
                    // STRING: LEGORR 0x004a3acc
                    "LoaderProfile.txt"
                :
                    // STRING: LEGORR 0x004a3ab0
                    "LoaderProfileNoSound.txt",
                barDirA, barName, &barRect, loadTextVal);
            // STRING: LEGORR 0x004a3aa4
            string_table_entry_set_current("Game Data");

            if ((s_zoom_step_context = scene_object_chain_record_create(legoGlobs_rootCont, 1)) != NULL &&
                (legoGlobs_cameraFP = scene_object_chain_record_create(legoGlobs_rootCont, 3)) != NULL &&
                (legoGlobs_cameraTrack = scene_object_chain_record_create(legoGlobs_rootCont, 2)) != NULL &&
                (legoGlobs_viewMain = render_target_viewport_scale_set(0.0f, 0.0f, 1.0f, 1.0f, LEGO_FIELD(s_zoom_step_context, void*, 0x24))) != NULL &&
                (legoGlobs_viewTrack = render_target_viewport_create_original(16, 13, 151, 151, LEGO_FIELD(legoGlobs_cameraTrack, void*, 0x24))) != NULL)
            {
                if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3a90
                    "StreamNERPSSpeach", 0)) == 1) legoGlobs_flags1 |= 0x4000000;
                if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3a7c
                    "ShowDebugToolTips", 0)) == 1 || engine_app_mode_get()) s_panel_highlight_flags |= 0x8;
                if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3a6c
                    "AllowDebugKeys", 0)) == 1 || engine_app_mode_get()) s_panel_highlight_flags |= 0x10;
                if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3a5c
                    "AllowEditMode", 0)) == 1 || engine_app_mode_get()) s_panel_highlight_flags |= 0x20;

                config_rgb_vector_read(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3a4c
                    "PowerCrystalRGB", 0), &legoGlobs_PowerCrystalRGB[0], &legoGlobs_PowerCrystalRGB[1], &legoGlobs_PowerCrystalRGB[2]);
                config_rgb_vector_read(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3a38
                    "UnpoweredCrystalRGB", 0), &legoGlobs_UnpoweredCrystalRGB[0], &legoGlobs_UnpoweredCrystalRGB[1], &legoGlobs_UnpoweredCrystalRGB[2]);

                sound_samples_config_load(cfgStr, !(s_engine_feature_word & 0x100000));
                surface_type_descriptions_config_load(cfgStr, s_engine_config_root);
                priorities_load_positions(cfgStr, s_engine_config_root);
                priorities_load_images(cfgStr, s_engine_config_root);
                lego_tooltip_info_strings_load(cfgStr, s_engine_config_root);
                entity_tool_name_strings_load(cfgStr, (void*)s_engine_config_root);
                frontend_panel_rotation_control_config_load(cfgStr, s_engine_config_root);

                sharedDir = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3a28
                    "SharedObjects", 0));
                lws_initialise(sharedDir, (LwsFindSFXIDFunc)sound_alias_lookup,
                               (LwsPlaySample3DFunc)sound_positional_sound_submit,
                               (LwsSoundEnabledFunc)sound_subsystem_enabled);
                sharedDir = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3a18
                    "SharedTextures", 0));
                if (sharedDir != NULL) shared_texture_directory_set(sharedDir);
                shared_texture_root_register(sharedDir);

                // STRING: LEGORR 0x004a3a08
                sprintf(buff, "NextButton%ix%i", display_width_get(), display_height_get());
                imageNameStr = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main", buff, 0));
                if (imageNameStr != NULL && (legoGlobs_NextButtonImage = image_load_bmp_scaled(imageNameStr, 0, 0)) != NULL) {
                    image_setup_trans(legoGlobs_NextButtonImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                    // STRING: LEGORR 0x004a39f4
                    sprintf(buff, "NextButtonPos%ix%i", display_width_get(), display_height_get());
                    if ((posString = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Main", buff, 0))) != NULL) {
                        if (sound_variant_split(posString, posArgvStr, ",") == 2) {
                            legoGlobs_NextButtonPos[0] = (float)atof(posArgvStr[0]);
                            legoGlobs_NextButtonPos[1] = (float)atof(posArgvStr[1]);
                        }
                        engine_memory_free(posString);
                    }
                }
                // STRING: LEGORR 0x004a39e4
                sprintf(buff, "BackButton%ix%i", display_width_get(), display_height_get());
                imageNameStr = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main", buff, 0));
                if (imageNameStr != NULL && (legoGlobs_BackButtonImage = image_load_bmp_scaled(imageNameStr, 0, 0)) != NULL) {
                    image_setup_trans(legoGlobs_BackButtonImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                    // STRING: LEGORR 0x004a39d0
                    sprintf(buff, "BackButtonPos%ix%i", display_width_get(), display_height_get());
                    if ((posString = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Main", buff, 0))) != NULL) {
                        if (sound_variant_split(posString, posArgvStr, ",") == 2) {
                            legoGlobs_BackButtonPos[0] = (float)atof(posArgvStr[0]);
                            legoGlobs_BackButtonPos[1] = (float)atof(posArgvStr[1]);
                        }
                        engine_memory_free(posString);
                    }
                }
                imageNameStr = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a39c4
                    "BackArrow", 0));
                if (imageNameStr != NULL && (legoGlobs_BackArrowImage = image_load_bmp_scaled(imageNameStr, 0, 0)) != NULL) {
                    image_setup_trans(legoGlobs_BackArrowImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                }

                deferred_callback_set(legoGlobs_NextButtonImage != NULL);

                if (scene_lights_build_from_config_original() && render_quality_config_apply_original() &&
                    lego_upgrade_types_load() && lego_entity_vehicle_type_table_load() &&
                    lego_entity_minifigure_type_table_load() && lego_entity_rockmonster_type_table_load() &&
                    lego_entity_building_types_load())
                {
                    cameraSpeedD = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a39b8
                        "CameraSpeed", 0));
                    camDropOff = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a39a8
                        "CameraDropOff", 0));
                    cameraAcceleration = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3994
                        "CameraAcceleration", 0));
                    indent = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3980
                        "MouseScrollIndent", 0));
                    camera_view_context_set(cameraSpeedD, camDropOff, cameraAcceleration, indent);

                    if ((minDist3D = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3964
                        "MinDistFor3DSoundsOnTopView", 0))) != 0.0f) legoGlobs_MinDistFor3DSoundsOnTopView = minDist3D;
                    else legoGlobs_MinDistFor3DSoundsOnTopView = 150.0f;
                    if ((maxDist3DName = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3950
                        "MaxDistFor3DSounds", 0))) != 0.0f) sound_tracked_buffers_set_max_distance(maxDist3DName);
                    ledger_stub_0047c100_DirectSoundListenerRolloffSet(1.0f);
                    if ((rollOff3D = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a393c
                        "RollOffFor3DSounds", 0))) != 0.0f) ledger_stub_0047c100_DirectSoundListenerRolloffSet(rollOff3D);

                    s_refresh_record = select_place_create_adapter(legoGlobs_rootCont, 5.0f);
                    surface_visibility_set(s_refresh_record, 1);

                    if ((legoGlobs_MaxReturnedCrystals = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3928
                        "MaxReturnedCrystals", 0))) == 0) legoGlobs_MaxReturnedCrystals = 10;
                    if ((legoGlobs_MouseScrollBorder = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3914
                        "MouseScrollBorder", 0))) == 0) legoGlobs_MouseScrollBorder = 5;

                    lego_object_names_load(cfgStr);
                    lego_object_the_names_load(cfgStr);

                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3904
                        "HelpWindowOn", 0)) == 1) {
                        selection_mode_state_set((uint32_t)(uintptr_t)legoGlobs_fontBriefingLo);
                        help_window_initialise_adapter(cfgStr, s_engine_config_root);
                    }
                    frontend_render_three_pass_update(cfgStr, s_engine_config_root);
                    light_effect_configuration_load(cfgStr, s_engine_config_root);
                    stats_initialise(cfgStr, s_engine_config_root);
                    dependencies_initialise_adapter(cfgStr, s_engine_config_root);
                    bubble_config_texture_load(cfgStr);
                    encyclopedia_config_files_open(cfgStr, s_engine_config_root);
                    terrain_object_sfx_grid_config_load(cfgStr, s_engine_config_root);
                    weapon_type_config_load(cfgStr, (void*)s_engine_config_root);

                    s_context_mode = 1;
                    simtick_directsound_listener_view_set(*LEGO_FIELD(s_zoom_step_context, LegoA_Sound3DFrameArg*, 0x34));
                    sound_tracked_buffers_set_min_distance(legoGlobs_MinDistFor3DSoundsOnTopView);

                    mindist = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a38fc
                        "mindist", 0));
                    maxdist = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a38f4
                        "maxdist", 0));
                    minTiltStr = config_angle_radians_get(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a38ec
                        "mintilt", 0));
                    tiltMax = config_angle_radians_get(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a38e4
                        "maxtilt", 0));
                    camera_distance_bounds_set(s_zoom_step_context, mindist, maxdist);
                    camera_tilt_bounds_set(s_zoom_step_context, minTiltStr, tiltMax);

                    legoGlobs_TVClipDist = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a38d8
                        "tvclipdist", 0));
                    renderer_object_set_clip_distance(legoGlobs_viewMain, legoGlobs_TVClipDist);
                    renderer_object_set_clip_distance(legoGlobs_viewTrack, legoGlobs_TVClipDist);

                    legoGlobs_DynamiteDamageRadius = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a38c0
                        "DynamiteDamageRadius", 0));
                    legoGlobs_DynamiteMaxDamage = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a38ac
                        "DynamiteMaxDamage", 0));
                    legoGlobs_DynamiteWakeRadius = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3898
                        "DynamiteWakeRadius", 0));
                    legoGlobs_BirdScarerRadius = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3884
                        "BirdScarerRadius", 0));

                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a387c
                        "MusicOn", 0)) == 1) s_panel_highlight_flags |= 0x800;
                    if ((legoGlobs_CDStartTrack = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a386c
                        "CDStartTrack", 0))) == 0) legoGlobs_CDStartTrack = 2;
                    if ((legoGlobs_CDTracks = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3860
                        "CDTracks", 0))) == 0) {
                        s_panel_highlight_flags &= ~0x800;
                        legoGlobs_CDTracks = 1;
                    }
                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3858
                        "SoundOn", 0)) == 1) legoGlobs_flags1 |= 0x10;
                    legoGlobs_flags1 |= 0x80c20;
                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3850
                        "Clear", 0)) == 0) legoGlobs_flags1 &= ~0x400;
                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3848
                        "Panels", 0)) == 0) legoGlobs_flags1 &= ~0x800;
                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3838
                        "WallProMeshes", 0)) == 0) legoGlobs_flags1 &= ~0x20;
                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a382c
                        "DynamicPM", 0)) == 0) legoGlobs_flags1 &= ~0x80000;
                    legoGlobs_HPBlocks = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3820
                        "HPBlocks", 0));
                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a380c
                        "OnlyBuildOnPaths", 0)) == 1) legoGlobs_flags1 |= 0x400000;
                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a37fc
                        "AlwaysRockFall", 0)) == 1) legoGlobs_flags1 |= 0x800000;

                    if ((legoGlobs_MiniFigureRunAway = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a37e8
                        "MiniFigureRunAway", 0))) == 0.0f) legoGlobs_MiniFigureRunAway = 10.0f;
                    if ((legoGlobs_DrainTime = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a37dc
                        "DrainTime", 0))) == 0.0f) legoGlobs_DrainTime = 10.0f;
                    legoGlobs_DrainTime *= 25.0f;
                    if ((legoGlobs_ReinforceHits = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a37cc
                        "ReinforceHits", 0))) == 0) legoGlobs_ReinforceHits = 4;
                    legoGlobs_MedPolyRange = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a37bc
                        "MedPolyRange", 0));
                    legoGlobs_HighPolyRange = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a37ac
                        "HighPolyRange", 0));
                    legoGlobs_MinEnergyForEat = LEGO_A_CFG_FLOAT(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a379c
                        "MinEnergyForEat", 0));

                    lego_misc_object_resources_load(cfgStr);

                    // STRING: LEGORR 0x004a3790
                    pointerArrayB = block_subgroup(cfgStr, string_path_join(s_engine_config_root, "Pointers", 0));
                    pointer_cursor_config_load_target(pointerArrayB);
                    media_pointer_cursor_select(0);

                    if (s_panel_highlight_flags & 0x10) {
                        lego_key_callback_set_adapter(2, 9, 0, 0, 0);
                        lego_key_callback_set_adapter(3, 9, 0, 1, 0);
                        lego_key_callback_set_adapter(5, 10, 0, 0, 0);
                        lego_key_callback_set_adapter(4, 11, 0, 0, 0);
                    }

                    transient_effect_textures_preload(
                        // STRING: LEGORR 0x004a3778
                        "MiscAnims\\Smoke",
                        // STRING: LEGORR 0x004a3788
                        "Smoke", 3);
                    preload_numbered_bitmaps(
                        // STRING: LEGORR 0x004a3754
                        "Interface\\FONTS\\HealthFont",
                        // STRING: LEGORR 0x004a3770
                        "a000_");

                    dialogPosValue.x = 0.0f;
                    dialogPosValue.y = 0.0f;
                    dialogStringC = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root,
                        // STRING: LEGORR 0x004a3744
                        "Dialog",
                        // STRING: LEGORR 0x004a374c
                        "Image", 0));
                    if (dialogStringC != NULL && (legoGlobs_DialogImage = image_load_bmp_scaled(dialogStringC, 0, 0)) != NULL) {
                        image_setup_trans(legoGlobs_DialogImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                        dialogPosValue.x = (float)(display_width_get() / 2) - (float)(legoGlobs_DialogImage->width >> 1);
                        dialogPosValue.y = (float)(display_height_get() / 2) - (float)(legoGlobs_DialogImage->height >> 1);
                    }
                    dialogStringC = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Dialog",
                        // STRING: LEGORR 0x004a3738
                        "TitleWindow", 0));
                    if (dialogStringC != NULL && (numPartsC = sound_variant_split(dialogStringC, dialogTokens, "|")) == 4) {
                        dialogWindow.x = (float)atoi(dialogTokens[0]) + dialogPosValue.x;
                        dialogWindow.y = (float)atoi(dialogTokens[1]) + dialogPosValue.y;
                        dialogWindow.width = (float)atoi(dialogTokens[2]);
                        dialogWindow.height = (float)atoi(dialogTokens[3]);
                        legoGlobs_DialogTextWndTitle = text_buffer_create((uintptr_t)legoGlobs_fontBriefingLo, (const uint32_t*)&dialogWindow, 64);
                    }
                    dialogStringC = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Dialog",
                        // STRING: LEGORR 0x004a372c
                        "TextWindow", 0));
                    if (dialogStringC != NULL && (numPartsC = sound_variant_split(dialogStringC, dialogTokens, "|")) == 4) {
                        dialogWindow.x = (float)atoi(dialogTokens[0]) + dialogPosValue.x;
                        dialogWindow.y = (float)atoi(dialogTokens[1]) + dialogPosValue.y;
                        dialogWindow.width = (float)atoi(dialogTokens[2]);
                        dialogWindow.height = (float)atoi(dialogTokens[3]);
                        legoGlobs_DialogTextWndMessage = text_buffer_create((uintptr_t)legoGlobs_fontBriefingHi, (const uint32_t*)&dialogWindow, 256);
                    }
                    dialogStringC = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Dialog",
                        // STRING: LEGORR 0x004a3720
                        "OkWindow", 0));
                    if (dialogStringC != NULL && (numPartsC = sound_variant_split(dialogStringC, dialogTokens, "|")) == 4) {
                        dialogWindow.x = (float)atoi(dialogTokens[0]) + dialogPosValue.x;
                        dialogWindow.y = (float)atoi(dialogTokens[1]) + dialogPosValue.y;
                        dialogWindow.width = (float)atoi(dialogTokens[2]);
                        dialogWindow.height = (float)atoi(dialogTokens[3]);
                        legoGlobs_DialogTextWndOK = text_buffer_create((uintptr_t)legoGlobs_fontBriefingHi, (const uint32_t*)&dialogWindow, 256);
                    }
                    dialogStringC = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Dialog",
                        // STRING: LEGORR 0x004a3710
                        "CancelWindow", 0));
                    if (dialogStringC != NULL && (numPartsC = sound_variant_split(dialogStringC, dialogTokens, "|")) == 4) {
                        dialogWindow.x = (float)atoi(dialogTokens[0]) + dialogPosValue.x;
                        dialogWindow.y = (float)atoi(dialogTokens[1]) + dialogPosValue.y;
                        dialogWindow.width = (float)atoi(dialogTokens[2]);
                        dialogWindow.height = (float)atoi(dialogTokens[3]);
                        legoGlobs_DialogTextWndCancel = text_buffer_create((uintptr_t)legoGlobs_fontBriefingHi, (const uint32_t*)&dialogWindow, 256);
                    }
                    dialogStringC = block_get_str_copy(cfgStr, string_path_join(s_engine_config_root, "Dialog",
                        // STRING: LEGORR 0x004a3700
                        "ContrastOverlay", 0));
                    if (dialogStringC != NULL && (s_position_handle = image_load_bmp_scaled(dialogStringC, 0, 0)) != NULL) {
                        image_setup_trans((Image*)s_position_handle, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                    }

                    legoGlobs_flags1 |= 0x2002;
                    // STRING: LEGORR 0x004a36f0
                    lego_noop_load_adapter(legoGlobs_rootCont, "animtex\\dirt");

                    s_lego_radar_zoom = 15.0f;
                    legoGlobs_f_5563e4[0] = 16.0f;
                    legoGlobs_f_5563e4[1] = 13.0f;
                    legoGlobs_f_5563e4[2] = 151.0f;
                    legoGlobs_f_5563e4[3] = 151.0f;

                    rock_fall_initialise_adapter(cfgStr, s_engine_config_root, legoGlobs_rootCont);
                    advisor_tooltip_and_sound_config_load(cfgStr);
                    tooltip_icons_config_load(cfgStr);
                    lego_level_upgrade_name_strings_load(cfgStr);
                    message_load_infomessages(cfgStr);
                    // STRING: LEGORR 0x004a36c8
                    interface_message_tab_info_text_load("Interface\\MessageTabs\\infoOverFlow.bmp");
                    text_messages_config_load_original(cfgStr);
                    panels_config_entry_apply_original(cfgStr, display_width_get(), display_height_get());
                    three_surface_resources_load(
                        // STRING: LEGORR 0x004a3650
                        "Interface\\RightPanel\\SmallCrystal.bmp",
                        // STRING: LEGORR 0x004a3678
                        "Interface\\RightPanel\\UsedCrystal.bmp",
                        // STRING: LEGORR 0x004a36a0
                        "Interface\\RightPanel\\NoSmallCrystal.bmp");
                    panel_resource_layout_store(
                        // STRING: LEGORR 0x004a35fc
                        "Interface\\AirMeter\\msgpanel_air_juice.bmp", 85, 6, 236,
                        // STRING: LEGORR 0x004a3628
                        "Interface\\AirMeter\\msgpanel_noair.bmp", 21, 0);
                    // STRING: LEGORR 0x004a35d0
                    panel_resource_handle_and_dimensions_store("Interface\\RightPanel\\crystalsidebar_ore.bmp", 615, 434, 423);
                    panel_button_layout_config_load_original(cfgStr, display_width_get(), display_height_get());
                    advisor_tutorial_icon_image_load(cfgStr);
                    interface_init_object_items();
                    interface_load_menu_items(cfgStr, s_engine_config_root);
                    interface_load_item_panels(cfgStr, s_engine_config_root);
                    interface_load_dependency_images(
                        // STRING: LEGORR 0x004a3590
                        "Interface\\Dependencies\\+.bmp",
                        // STRING: LEGORR 0x004a35b0
                        "Interface\\Dependencies\\=.bmp");
                    reward_load_graphics(cfgStr, s_engine_config_root);
                    reward_initialise();
                    legacy_scroll_panel_controls_initialize();
                    panel_item_flag16_set(0, 2, 1);
                    panel_item_flag16_set(0, 3, 1);
                    save_slot_selection_clear_target();
                    if (frontend_enabled_original()) front_load_menus_adapter(legoGlobs_config);

                    advisor_scene_controller_initialize(s_engine_config_root, s_zoom_step_context, legoGlobs_viewMain);
                    advisor_scene_visual_state_initialize(200.0f);
                    advisor_config_slots_load_target(cfgStr, s_engine_config_root);
                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3580
                        "DisplayAdvisor", 0)) == 1) {
                        advisor_position_config_load(cfgStr, s_engine_config_root, display_width_get(), display_height_get());
                    }

                    if (legoGlobs_flags1 & 0x10) lego_audio_playback_toggle(1);

                    if ((unsigned int)engine_app_mode_get() <= 2 && block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main", "FrontEnd", 0)) == 1) {
                        string_table_entry_set_current(NULL);
                        front_run_screen_menu_type_adapter(0);
                        if (quit_menu_probe()) return 0;
                        startLevel = front_get_selected_level_adapter();
                    } else {
                        if ((startLevel = main_get_start_level_adapter()) == NULL) {
                            startLevel = block_get_strptr(cfgStr, string_path_join(s_engine_config_root, "Main",
                                // STRING: LEGORR 0x004a3574
                                "startlevel", 0));
                        }
                    }

                    options_load_or_default_target(1, 0);
                    if (lego_level_load(startLevel)) {
                        sound_alias_play(7, 1);
                        return 1;
                    }
                }
                block_close(cfgStr);
            }
        }
        ddiscene_object_pool_shutdown();
    }
    return 0;
}
#if defined(_MSC_VER)
#pragma optimize("", on)
#endif

/* MiscObjectResourcesLoad [group F] */
/* The original compiled Lego_LoadMiscObjects without optimisation (frame
 * pointer, every value through its stack slot). */
#ifdef _MSC_VER
#pragma optimize("", off)
#endif
// FUNCTION: LEGORR 0x00422850
void lego_misc_object_resources_load(struct BlockRec* config)
{
    const char* fileName;

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "Boulder", NULL));
    legoGlobs_contBoulder = container_load(legoGlobs_rootCont, fileName, "LWO", 1);
    if (legoGlobs_contBoulder == NULL)
        // STRING: LEGORR 0x004a4014
        legoGlobs_contBoulder = container_load(legoGlobs_rootCont, fileName, "MESH", 1);
    scene_render_lock_toggle(legoGlobs_contBoulder, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a4000
        "BoulderExplodeIce", NULL));
    legoGlobs_contBoulderExplodeIce = container_load(legoGlobs_rootCont, fileName, "LWS", 1);
    scene_render_lock_toggle(legoGlobs_contBoulderExplodeIce, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3ff0
        "BoulderExplode", NULL));
    legoGlobs_contBoulderExplode = container_load(legoGlobs_rootCont, fileName, "LWS", 1);
    scene_render_lock_toggle(legoGlobs_contBoulderExplode, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3fe4
        "SmashPath", NULL));
    legoGlobs_contSmashPath = container_load(legoGlobs_rootCont, fileName, "LWS", 1);
    scene_render_lock_toggle(legoGlobs_contSmashPath, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3fd8
        "Explosion", NULL));
    global_resource_handle_load_and_enable_original(legoGlobs_rootCont, fileName);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3fd0
        "Crystal", NULL));
    legoGlobs_contCrystal = container_load(legoGlobs_rootCont, fileName, "LWO", 1);
    if (legoGlobs_contCrystal == NULL)
        legoGlobs_contCrystal = container_load(legoGlobs_rootCont, fileName, "MESH", 1);
    scene_render_lock_toggle(legoGlobs_contCrystal, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "Dynamite", NULL));
    legoGlobs_contDynamite = container_load(legoGlobs_rootCont, fileName, "ACT", 1);
    scene_render_lock_toggle(legoGlobs_contDynamite, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "OohScary", NULL));
    legoGlobs_contOohScary = container_load(legoGlobs_rootCont, fileName, "ACT", 1);
    scene_render_lock_toggle(legoGlobs_contOohScary, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "Barrier", NULL));
    legoGlobs_contBarrier = container_load(legoGlobs_rootCont, fileName, "ACT", 1);
    scene_render_lock_toggle(legoGlobs_contBarrier, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "Ore", NULL));
    legoGlobs_contOresTable = container_load(legoGlobs_rootCont, fileName, "LWO", 1);
    if (legoGlobs_contOresTable == NULL)
        legoGlobs_contOresTable = container_load(legoGlobs_rootCont, fileName, "MESH", 1);
    scene_render_lock_toggle(legoGlobs_contOresTable, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "ProcessedOre", NULL));
    legoGlobs_contProcessedOre = container_load(legoGlobs_rootCont, fileName, "LWO", 1);
    if (legoGlobs_contProcessedOre == NULL)
        legoGlobs_contProcessedOre = container_load(legoGlobs_rootCont, fileName, "MESH", 1);
    scene_render_lock_toggle(legoGlobs_contProcessedOre, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "ElectricFence", NULL));
    legoGlobs_contElectricFence = container_load(legoGlobs_rootCont, fileName, "LWO", 1);
    scene_render_lock_toggle(legoGlobs_contElectricFence, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3f7c
        "ElectricFenceStud", NULL));
    legoGlobs_contElectricFenceStud = container_load(legoGlobs_rootCont, fileName, "LWS", 1);
    scene_render_lock_toggle(legoGlobs_contElectricFenceStud, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3f64
        "ShortElectricFenceBeam", NULL));
    indexed_resource_handle_set_create(legoGlobs_rootCont, fileName, 0);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3f4c
        "LongElectricFenceBeam", NULL));
    indexed_resource_handle_set_create(legoGlobs_rootCont, fileName, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3f40
        "SpiderWeb", NULL));
    legoGlobs_contSpiderWeb = container_load(legoGlobs_rootCont, fileName, "ACT", 1);
    scene_render_lock_toggle(legoGlobs_contSpiderWeb, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3f30
        "RechargeSparkle", NULL));
    legoGlobs_contRechargeSparkle = container_load(legoGlobs_rootCont, fileName, "LWS", 1);
    scene_render_lock_toggle(legoGlobs_contRechargeSparkle, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3f20
        "MiniTeleportUp", NULL));
    legoGlobs_contMiniTeleportUp = container_load(legoGlobs_rootCont, fileName, "LWS", 0);
    scene_render_lock_toggle(legoGlobs_contMiniTeleportUp, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3f18
        "IceCube", NULL));
    legoGlobs_contIceCube = container_load(legoGlobs_rootCont, fileName, "ACT", 1);
    scene_render_lock_toggle(legoGlobs_contIceCube, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "Pusher", NULL));
    legoGlobs_contPusher = container_load(legoGlobs_rootCont, fileName, "LWS", 1);
    scene_render_lock_toggle(legoGlobs_contPusher, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "Freezer", NULL));
    legoGlobs_contFreezer = container_load(legoGlobs_rootCont, fileName, "LWS", 1);
    scene_render_lock_toggle(legoGlobs_contFreezer, 1);

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects",
        // STRING: LEGORR 0x004a3efc
        "LaserShot", NULL));
    legoGlobs_contLaserShot = container_load(legoGlobs_rootCont, fileName, "LWS", 1);
    scene_render_lock_toggle(legoGlobs_contLaserShot, 1);

    misc_objects_effect_slots_bind_original(config, s_engine_config_root, legoGlobs_rootCont);
}
#ifdef _MSC_VER
#pragma optimize("", on)
#endif

/* ModeDependentField24Get [group M] */
/* Lego_GetCurrentCamera_Container: the camera container of the current
 * view. Like the original, it returns nothing for another view mode. */
// FUNCTION: LEGORR 0x00423080
void* lego_mode_dependent_field24_get(void)
{
    if (s_context_mode == 1) return LEGO_FIELD(s_zoom_step_context, void*, 0x24);
    else if (s_context_mode == 0) return LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24);
}

/* GlobalFloatField00556F00Get [group M] */
// FUNCTION: LEGORR 0x004230b0
float global_float_field_00556f00_get(void)
{
    return s_global_float_00556f00;
}

/* DebugPanelTextDrawUpdate [group B] */
/* Lego_DrawRenameInput: the name being typed at its position, between
 * blinking quotes, or the selected unit's name while it is empty. */
// FUNCTION: LEGORR 0x004230c0
void lego_debug_panel_text_draw_update(float elapsed)
{
    unsigned int width;
    int x;
    int y;
    unsigned int quoteWidth;
    const char* text;

    s_lego_renameCursorTimer -= elapsed;
    if (s_lego_renameCursorTimer <= 0.0f) {
        s_lego_renameCursorTimer += 12.5f;
        s_lego_renameCursorOn = !s_lego_renameCursorOn;
    }

    if (legoGlobs_renameInput != NULL) {
        x = (int)legoGlobs_renamePosition[0];
        y = (int)legoGlobs_renamePosition[1] -
            (object_field_be4_dword_get(s_lego_crystal_count_font) >> 1);
        quoteWidth = bitmap_font_glyph_measure_width(s_lego_crystal_count_font, '"');
        if (legoGlobs_renameInput[0] == '\0' && selection_active_entity_get() != NULL)
            text = entity_display_name_lookup(selection_active_entity_get());
        else
            text = legoGlobs_renameInput;
        width = formatted_text_printf_width(s_lego_crystal_count_font,
                                            // STRING: LEGORR 0x004a4030
                                            "\"%s\"",
                                            text);
        if (x + width > (unsigned int)s_display_w - 40) x -= width;
        font_print_f(s_lego_crystal_count_font,
                     x + (s_lego_renameCursorOn ? 0 : quoteWidth), y,
                     // STRING: LEGORR 0x004a4024
                     "%s%s%s",
                     // STRING: LEGORR 0x004a402c
                     s_lego_renameCursorOn ? "\"" : s_config_angle_empty,
                     text,
                     s_lego_renameCursorOn ? "\"" : s_config_angle_empty);
    }
}

/* EnginePreTick [group B] */
/* Lego_HandleRenameInput: type the pressed keys into the name being
 * entered. Backspace deletes, Return or Escape ends the input. The caller
 * passes the frame time, which it does not use. */
// FUNCTION: LEGORR 0x004231f0
void lego_engine_pre_tick(float elapsed)
{
    unsigned int len;
    unsigned int i;
    unsigned int c;

    if (legoGlobs_renameInput != NULL) {
        len = strlen(legoGlobs_renameInput);
        if (s_global_current_keys[0x0e] && (s_global_current_keys[0x0e] ^ s_lego_renamePrevKeyMap[0x0e])) {
            if (len) legoGlobs_renameInput[len - 1] = '\0';
        } else if ((!s_global_current_keys[0x1c] && s_lego_renamePrevKeyMap[0x1c]) ||
                   (!s_global_current_keys[0x01] && s_lego_renamePrevKeyMap[0x01])) {
            legoGlobs_renameInput = NULL;
            lego_transition_control(0, 0);
        } else {
            for (i = 0; i < 256; i++) {
                if (len < 11 && s_global_current_keys[i] &&
                    (s_global_current_keys[i] ^ s_lego_renamePrevKeyMap[i]) &&
                    (c = input_key_context_check(i)) != 0) {
                    legoGlobs_renameInput[len] = (char)toupper(c);
                    legoGlobs_renameInput[++len] = '\0';
                }
            }
        }
        memcpy(s_lego_renamePrevKeyMap, s_global_current_keys, 256);
        memset(s_global_current_keys, 0, 256);
    }
}

/* EngineTick [group B] */
/* Lego_MainLoop: one frame of the game, installed as the main loop's
 * callback. FALSE ends the loop. */
// FUNCTION: LEGORR 0x004232e0
int lego_engine_tick(float elapsed)
{
    float elapsedAbs;
    float elapsedWorld;
    float pos[3];
    int sfx;
    int keyDownT;
    int keyDownR;
    int keyDownShift;
    int renderTrack;
    unsigned int i;
    int total;
    int volume;
    int crystals;
    int used;

    elapsedAbs = elapsed;
    sfx = 0;

    if (s_application_mode == 10 && !keep_running_original()) return 0;

    if (legoGlobs_flags1 & 0x100000) elapsed = 0.0f;

    lego_engine_pre_tick(elapsedAbs);

    s_global_float_00556f00 = elapsed;
    if (s_context_mode == 0 && (unsigned int)s_application_mode < 2) elapsedWorld = elapsed;
    else elapsedWorld = EngineTimeScale() * elapsed;

    camera_move_pipeline_original(elapsedWorld);
    camera_registry_probe_original();
    lego_level_record_timer_countdown((Lego_Level*)level_current_owner_get(), elapsed);
    lego_camera_grid_expiry(elapsedWorld);
    lego_spawn_coordinate_pool_tick(elapsedWorld);

    if (s_panel_highlight_flags & 0x1) {
        if (legoGlobs_cameraFP != NULL && LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24) != NULL &&
            legoGlobs_dirLightCallToArms != NULL && !(legoGlobs_flags1 & 0x100000)) {
            scene_render_lock_toggle(legoGlobs_dirLightCallToArms, 0);
            render_object_rotate_axis(legoGlobs_dirLightCallToArms, 2, 0.0f, 1.0f, 0.0f, elapsed * 0.1f);
        }
    } else if (legoGlobs_cameraFP != NULL && LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24) != NULL &&
               legoGlobs_dirLightCallToArms != NULL) {
        scene_render_lock_toggle(legoGlobs_dirLightCallToArms, 1);
    }

    if (!(legoGlobs_flags1 & 0x100000) && ((legoGlobs_flags1 & 0x200000) || legoGlobs_f_556158)) {
        if (legoGlobs_f_556158) {
            float ratio;
            float delta[2];
            ratio = (1.0f / elapsed) * 5.0f;
            ddiscene_object_position_get(LEGO_FIELD(s_zoom_step_context, void*, 0x2c), NULL, pos);
            legoGlobs_f_556148 = ((ratio * pos[0]) + legoGlobs_f_556150) / (ratio + 1.0f);
            legoGlobs_f_55614c = ((ratio * pos[1]) + legoGlobs_f_556154) / (ratio + 1.0f);
            delta[0] = legoGlobs_f_556148 - pos[0];
            delta[1] = legoGlobs_f_55614c - pos[1];
            if ((float)sqrt(delta[0] * delta[0] + delta[1] * delta[1]) < 2.0f) legoGlobs_f_556158 = 0;
        }
        selection_record_point_set((struct ViewContext*)s_zoom_step_context,
                                   legoGlobs_f_556148, legoGlobs_f_55614c);
        legoGlobs_flags1 &= ~0x200000;
    }

    if (legoGlobs_flags1 & 0x4) {
        elapsedWorld = 1.0f;
        legoGlobs_flags1 &= ~0x4;
    }

    if (!lego_main_sim_step(elapsedWorld, elapsed, &keyDownT, &keyDownR, &keyDownShift)) return 0;

    if (!(legoGlobs_flags1 & 0x100000)) lego_terrain_pointer_command_update();

    cmd_tick_global_delta(elapsedWorld);
    lego_terrain_cell_flag_cleanup(legoGlobs_currLevel, elapsedWorld);
    simtick_table_dispatch_original(elapsed);
    simtick_scene_pass_original(elapsedWorld);
    simtick_sfx_bank_original(elapsedWorld);
    lava_erosion_pending_cell_tick(elapsedWorld);
    lego_block_activity_update_all(legoGlobs_currLevel, elapsedWorld);
    cmd_queue_tick();
    view_tick_original(s_zoom_step_context, legoGlobs_currLevel, elapsed, elapsedWorld);
    view_tick_original(legoGlobs_cameraTrack, legoGlobs_currLevel, elapsed, elapsedWorld);
    view_tick_original(legoGlobs_cameraFP, legoGlobs_currLevel, elapsed, elapsedWorld);
    construction_update_all(elapsedWorld);
    interface_update_hover_sfx(elapsedAbs);
    lego_scene_marker_update_and_draw(elapsed, 0);
    simtick_render_step_original();
    simtick_post_cluster_original(elapsedWorld);
    lego_main_sim_interaction_and_cell_update(elapsedAbs, elapsed, keyDownT, keyDownR, keyDownShift);
    terrain_map_update(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), elapsedWorld);
    simtick_fx_bank_tick_original(legoGlobs_currLevel, elapsedWorld);
    simtick_sound_keepalive_original();
    transient_effect_instances_tick(elapsedWorld);
    spiderweb_update(elapsedWorld);
    motion_gameplay_tick(elapsedWorld);
    message_cooldown_tick(elapsedAbs);

    if (legoGlobs_f_556ec0) kind_six_random_sweep_dispatch(elapsedWorld);

    if ((s_panel_highlight_flags & 0x10) && s_global_current_keys[0x40] &&
        (s_global_previous_keys[0x40] ^ s_global_current_keys[0x40])) {
        legoGlobs_f_556ec0 = !legoGlobs_f_556ec0;
        text_buffer_append_format(legoGlobs_textOnlyWindow,
                                  // STRING: LEGORR 0x004a40a8
                                  "\nFallin Mode %s",
                                  legoGlobs_f_556ec0 ?
                                  // STRING: LEGORR 0x004a40bc
                                  "On" :
                                  // STRING: LEGORR 0x004a40b8
                                  "Off");
    }

    scene_render_lock_toggle(legoGlobs_ambientLight, 0);
    if (s_context_mode == 1) {
        if (!(legoGlobs_flags1 & 0x100000))
            scene_render_lock_toggle((void*)(uintptr_t)legoGlobs_f_556104, 0);
        scene_render_lock_toggle(legoGlobs_spotlightTop, 0);
        ledger_stub_00477010_DDiSceneDrawBegin(legoGlobs_viewMain, 1);
        ddiscene_draw_end(legoGlobs_viewMain, legoGlobs_rootCont, elapsed);
        scene_render_lock_toggle(legoGlobs_spotlightTop, 1);
        scene_render_lock_toggle((void*)(uintptr_t)legoGlobs_f_556104, 1);
    } else if (s_context_mode == 0) {
        graphics_overlay_visibility_set(0);
        terrain_dirty_quad_normal_emit_pass_original();
        entity_type_two_positioned_callback_dispatch(
            (const struct EntitySlot*)legoGlobs_selectedObject,
            LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24),
            legoGlobs_MedPolyRange, legoGlobs_HighPolyRange, 1);
        entity_type_two_event_dispatch((const struct EntitySlot*)legoGlobs_selectedObject,
                                       record_field14_get(legoGlobs_cameraFP), 1);
        lego_sine_phase_midpoint_blend_dispatch(1, elapsedWorld);
        if (!(legoGlobs_flags1 & 0x100000)) {
            scene_render_lock_toggle((void*)(uintptr_t)legoGlobs_f_5560f8, 0);
            if (legoGlobs_dirLightFP != NULL) {
                render_object_rotate_axis(legoGlobs_dirLightFP, 2, 0.0f, 1.0f, 0.0f, elapsed * 0.01f);
                scene_render_lock_toggle(legoGlobs_dirLightFP, 0);
            }
        }
        ledger_stub_00477010_DDiSceneDrawBegin(legoGlobs_viewMain, 1);
        ddiscene_draw_end(legoGlobs_viewMain, legoGlobs_rootCont, elapsed);
        scene_render_lock_toggle((void*)(uintptr_t)legoGlobs_f_5560f8, 1);
        if (legoGlobs_dirLightFP != NULL) scene_render_lock_toggle(legoGlobs_dirLightFP, 1);
        lego_sine_phase_midpoint_blend_dispatch(0, elapsedWorld);
        entity_type_two_event_dispatch((const struct EntitySlot*)legoGlobs_selectedObject,
                                       record_field14_get(legoGlobs_cameraFP), 0);
        entity_type_two_positioned_callback_dispatch(
            (const struct EntitySlot*)legoGlobs_selectedObject, NULL, 0.0f, 0.0f, 0);
        indexed_surface_entries_restore();
        graphics_overlay_visibility_set(1);
        terrain_grid_entry_resources_release_all_current();
    }
    scene_render_lock_toggle(legoGlobs_ambientLight, 1);

    if (request_record_ready_flag2_target(0)) legoGlobs_flags1 |= 0x2;
    else legoGlobs_flags1 &= ~0x2;

    if (s_context_mode == 1) {
        lego_selected_object_outlines_draw((struct ViewProjectionAdapter*)legoGlobs_viewMain);
        lego_default_transform_apply_all(legoGlobs_viewMain);
        screen_space_effect_dispatch_original(legoGlobs_viewMain);
    }

    lego_selection_rectangle_render(legoGlobs_currLevel);

    if (!(legoGlobs_flags1 & 0x100000)) screen_effects_tick(elapsed);

    if ((legoGlobs_flags1 & 0x2) && (legoGlobs_flags1 & 0x2000)) {
        ledger_stub_00477010_DDiSceneDrawBegin(legoGlobs_viewTrack, 1);
        if (!(legoGlobs_flags1 & 0x20000)) {
            lego_scene_marker_update_and_draw(elapsedWorld, 1);
            simtick_render_step_original();
            scene_render_lock_toggle(legoGlobs_spotlightTrack, 0);
            if (legoGlobs_cameraFP != NULL && LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24) != NULL &&
                legoGlobs_dirLightCallToArms != NULL)
                scene_render_lock_toggle(legoGlobs_dirLightCallToArms, 1);
            ddiscene_draw_end(legoGlobs_viewTrack, legoGlobs_rootCont, elapsed);
            scene_render_lock_toggle(legoGlobs_spotlightTrack, 1);
            terrain_map_visible_clear(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map));
        }
    }

    renderer_begin_frame_once_retail();
    transient_effect_registry_enable_set(1);
    sound_subsystem_frame_tick(elapsedAbs);

    if (s_context_mode == 1 && !(legoGlobs_flags1 & 0x100000)) {
        world_render_pass_a_original(legoGlobs_currLevel, legoGlobs_viewMain, elapsedWorld, elapsed);
        world_render_pass_b_original(legoGlobs_currLevel, legoGlobs_viewMain, elapsedWorld, elapsed);
    }

    terrain_map_visible_clear(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map));

    renderTrack = ((legoGlobs_flags1 & 0x2) && (legoGlobs_flags1 & 0x1000)) ? 1 : 0;
    if (renderTrack) lego_update_render_viewport_state();
    periodic_timer25_advance_original(elapsedWorld, renderTrack);

    if (legoGlobs_flags1 & 0x800) {
        if (!(legoGlobs_flags1 & 0x2)) simtick_manager_tick(1, elapsed);
        simtick_manager_tick(0, elapsed);
        if ((legoGlobs_flags1 & 0x2) && (legoGlobs_flags1 & 0x2000) && !(legoGlobs_flags1 & 0x20000))
            simtick_manager_tick(2, elapsed);
        simtick_manager_tick(4, elapsed);
        simtick_manager_tick(9, elapsed);
        simtick_manager_tick(6, elapsed);
        simtick_manager_tick(8, elapsed);
        priorities_draw();
        simtick_manager_tick(11, elapsed);
        effects_update_original(elapsed);
        if (!(s_panel_highlight_flags & 0x1000)) interface_draw();
        interface_update(elapsed);
        simtick_manager_tick(5, elapsed);
        panel_crystals_draw_original(
            legoGlobs_currLevel != NULL ? legoGlobs_currLevel->f_08c : crystals,
            legoGlobs_currLevel != NULL ? legoGlobs_currLevel->f_08c - legoGlobs_currLevel->f_098 : used,
            elapsedWorld);
        panel_side_bar_draw();
        panel_print_f(5, s_lego_crystal_count_font, 16, 469, 1, "%i",
                      legoGlobs_currLevel->crystals + legoGlobs_currLevel->ore * 5);
        simtick_manager_tick(7, elapsed);
        view_blit_rect_positions_apply(0);
        incident_message_text_consumer(elapsed);
        simtick_manager_tick(10, elapsed);
        info_message_queue_tick(elapsed);
    }

    help_window_update();
    if (objective_is_ended_original()) advisor_transition_worker(elapsedAbs);
    else advisor_transition_worker(elapsed);
    media_cd_audio_watchdog_tick(legoGlobs_flags1 & 0x8);
    if (!(legoGlobs_flags1 & 0x1000000)) tutorial_script_command_dispatch(elapsed);
    objective_tick_target(legoGlobs_textOnlyWindow, (struct LevelState*)legoGlobs_currLevel,
                          elapsedWorld, elapsedAbs);

    if ((unsigned int)s_application_mode < 2) {
        if (legoGlobs_currLevel->f_278 && (unsigned int)legoGlobs_currLevel->f_09c >= legoGlobs_currLevel->f_278)
            objective_announce_current(3);
        if (grid_record_count_difference_get(15, 0, 0, 1) == 0) {
            total = 0;
            for (i = 0; i < building_upgrade_admission_check(2, 0); i++)
                total += grid_record_count_difference_get(2, 0, i, 1);
            if (total == 0 && ((Lego_Level*)level_current_owner_get())->status == 0)
                objective_announce_current(2);
        }
    }

    advisor_text_display_worker(elapsed);

    if ((s_panel_highlight_flags & 0x4000) && legoGlobs_NextButtonImage != NULL)
        image_display_scaled(legoGlobs_NextButtonImage, NULL, (const struct Point2F*)&s_score_text_origin_x, NULL);
    if ((s_panel_highlight_flags & 0x8000) && legoGlobs_BackArrowImage != NULL)
        image_display_scaled(legoGlobs_BackArrowImage, NULL, (const struct Point2F*)&s_task_text_origin_x, NULL);
    if ((s_panel_highlight_flags & 0x40) && legoGlobs_NextButtonImage != NULL &&
        (!(s_panel_highlight_flags & 0x200) || sim_status_banner_word_get()))
        image_display_scaled(legoGlobs_NextButtonImage, NULL, (const struct Point2F*)legoGlobs_NextButtonPos, NULL);
    if ((s_panel_highlight_flags & 0x80) && legoGlobs_BackButtonImage != NULL)
        image_display_scaled(legoGlobs_BackButtonImage, NULL, (const struct Point2F*)legoGlobs_BackButtonPos, NULL);

    if ((s_panel_highlight_flags & 0x100) && frontend_enabled_original() &&
        overlay_menu_gate_original(elapsed, 1)) {
        s_panel_highlight_flags &= ~0x100;
        if (!objective_is_ended_original()) lego_transition_control(0, 0);
        else legoGlobs_flags1 |= 0x2000000;
    }

    lego_debug_panel_text_draw_update(elapsedAbs);

    if (legoGlobs_flags1 & 0x2000000) {
        if (frontend_enabled_original() && overlay_menu_gate_original(elapsed, 0)) {
            legoGlobs_flags1 ^= 0x2000000;
            if (level_start_flag()) {
                lego_start_level_pilots();
                ((Lego_Level*)level_current_owner_get())->status = 3;
                reward_level_start();
                if (legoGlobs_flags1 & 0x8) {
                    s_panel_highlight_flags |= 0x800;
                    lego_cd_audio_watchdog_mode_update(0);
                }
            } else if (!objective_is_ended_original()) {
                lego_transition_control(0, 0);
            }
        } else {
            media_pointer_cursor_override_tick(elapsedAbs);
            pointer_cursor_draw_target(s_renderer_word_a, s_renderer_word_b);
        }
    } else {
        media_pointer_cursor_override_tick(elapsedAbs);
        if (legoGlobs_renameInput == NULL) pointer_cursor_draw_target(s_renderer_word_a, s_renderer_word_b);
    }

    if (menu_options_flag() == 1 || s_application_mode == 11) frontend_start_level_original();

    if (legoGlobs_flags1 & 0x20000000)
        font_print_f((const struct TextureSurface*)legoGlobs_fontDesktop, 10, 10,
                     // STRING: LEGORR 0x004a4090
                     "%s mode",
                     (legoGlobs_flags1 & 0x40000) ?
                     // STRING: LEGORR 0x004a40a0
                     "Vertex" :
                     // STRING: LEGORR 0x004a4098
                     "Block");

    if ((legoGlobs_flags1 & 0x40000) && legoGlobs_f_556188) {
        float screen[4];
        terrain_map_block_first_vertex_get(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                                           legoGlobs_f_556180, legoGlobs_f_556184,
                                           (struct TerrainMapVector3*)pos);
        render_interface_transform_point(legoGlobs_viewMain, screen, pos);
        screen[0] /= screen[3];
        screen[1] /= screen[3];
        media_pointer_cursor_select(3);
        pointer_cursor_draw_target((int)screen[0], (int)screen[1]);
        legoGlobs_flags1 &= ~0x80000000;
    }

    timed_effect_runtime_update(s_renderer_word_a, s_renderer_word_b, elapsedAbs);

    if (s_panel_highlight_flags & 0x10) {
        if ((s_global_current_keys[0x06] && (s_global_previous_keys[0x06] ^ s_global_current_keys[0x06])) ||
            (s_global_current_keys[0x07] && (s_global_previous_keys[0x07] ^ s_global_current_keys[0x07])) ||
            (s_global_current_keys[0x08] && (s_global_previous_keys[0x08] ^ s_global_current_keys[0x08])) ||
            (s_global_current_keys[0x09] && (s_global_previous_keys[0x09] ^ s_global_current_keys[0x09])))
            s_lego_upgradeTextTimer = 1.0f;
        if (s_lego_upgradeTextTimer > 0.0f) {
            int count;
            struct EntitySlot* const* units;
            void* stats;
            units = selection_active_context_record_get(&count);
            if (count) {
                if (LEGO_FIELD(units[0], int, 0x00) == 4)
                    stats = LEGO_FIELD(units[0], char*, 0x18) + 0x8c;
                else if (LEGO_FIELD(units[0], int, 0x00) == 1)
                    stats = LEGO_FIELD(units[0], char*, 0x0c) + 0x124;
                else
                    stats = NULL;
                if (stats != NULL) {
                    unsigned int upgrades;
                    upgrades = LEGO_FIELD(stats, unsigned int, 0x08);
                    font_print_f(s_lego_text_window_font, 0, 0,
                                 // STRING: LEGORR 0x004a4078
                                 "Upgrade - (%i%i%i%i)",
                                 (upgrades >> 3) & 1, (upgrades >> 2) & 1,
                                 (upgrades >> 1) & 1, upgrades & 1);
                }
            }
            s_lego_upgradeTextTimer -= 0.1f;
        }
    }

    if (s_lego_ambientSoundTimer5 < 0.0f) {
        sound_alias_play(5, 0);
        s_lego_ambientSoundTimer5 = random_float_range(75.0f, 125.0f);
    }
    if (s_lego_ambientSoundTimer6 < 0.0f) {
        sound_alias_play(6, 0);
        s_lego_ambientSoundTimer6 = random_float_range(250.0f, 750.0f);
    }
    s_lego_ambientSoundTimer5 -= elapsed;
    s_lego_ambientSoundTimer6 -= elapsed;

    if (s_panel_highlight_flags & 0x10) {
        float dir;
        if (s_global_current_keys[0x49]) dir = 1.0f;
        else if (s_global_current_keys[0x48]) dir = -1.0f;
        else dir = 0.0f;
        if (0.0f != dir) {
            s_lego_gameSpeedTextTimer = 25.0f;
            lego_game_speed_set(EngineTimeScale() + (dir * elapsed * 0.01f));
            if (EngineTimeScale() > 3.0f) lego_game_speed_set(3.0f);
            if (EngineTimeScale() < 0.0f) lego_game_speed_set(0.0f);
        }
        if (s_lego_gameSpeedTextTimer > 0.0f)
            font_print_f(s_lego_text_window_font, 10, 80,
                         // STRING: LEGORR 0x004a4064
                         "Game Speed %0.0f%%",
                         EngineTimeScale() * 100.0f);
        s_lego_gameSpeedTextTimer -= elapsed;
    }

    if (s_panel_highlight_flags & 0x10) {
        if (-1.0f == s_lego_soundRollOff)
            s_lego_soundRollOff = ledger_stub_0047c100_DirectSoundListenerRolloffSet(-1.0f);
        if (s_global_current_keys[0x0a]) {
            s_lego_soundRollOff -= 0.05f;
            s_lego_soundRollOff = ledger_stub_0047c100_DirectSoundListenerRolloffSet(s_lego_soundRollOff);
            s_lego_rollOffTextTimer = 25.0f;
        } else if (s_global_current_keys[0x0b]) {
            s_lego_soundRollOff += 0.05f;
            s_lego_soundRollOff = ledger_stub_0047c100_DirectSoundListenerRolloffSet(s_lego_soundRollOff);
            s_lego_rollOffTextTimer = 25.0f;
        }
        if (s_lego_rollOffTextTimer > 0.0f)
            font_print_f(s_lego_text_window_font, 10, 80,
                         // STRING: LEGORR 0x004a404c
                         "3D sound fall off: %f",
                         s_lego_soundRollOff);
        s_lego_rollOffTextTimer -= elapsed;
    }

    if (!(legoGlobs_flags1 & 0x1000000))
        tutorial_frame_timer_array_advance(elapsed);
    else
        font_print_f(s_lego_text_window_font, 0, 0,
                     // STRING: LEGORR 0x004a4044
                     "NoNERPS");

    if (legoGlobs_currLevel->oxygenLevel <= 0.0f) {
        message_dispatch(0x12, NULL, 0, NULL);
    } else if (legoGlobs_currLevel->oxygenLevel < (legoGlobs_currLevel->f_204 - 5.0f)) {
        if (legoGlobs_currLevel->oxygenLevel <= 10.0f) {
            message_dispatch(0x11, NULL, 0, NULL);
            advisor_text_animation_update(0x14, 1, 0);
        } else {
            message_dispatch(0x10, NULL, 0, NULL);
            advisor_text_animation_update(0x15, 1, 0);
        }
        legoGlobs_currLevel->f_204 = legoGlobs_currLevel->oxygenLevel;
    } else if (legoGlobs_currLevel->oxygenLevel > legoGlobs_currLevel->f_204) {
        if (legoGlobs_currLevel->oxygenLevel == 100.0f) message_dispatch(0x13, NULL, 0, NULL);
        legoGlobs_currLevel->f_204 = legoGlobs_currLevel->oxygenLevel;
    }

    if (legoGlobs_currLevel->oxygenLevel <= 10.0f) {
        if (!LEGO_FIELD(&panelGlobs, int, 0x72c) &&
            sound_alias_lookup(
                // STRING: LEGORR 0x004a4038
                "SND_AirBeat",
                &sfx))
            sound_alias_play(sfx, 1);
        if (sound_alias_lookup("SND_AirBeat", &sfx)) {
            volume = (int)((float)(constant_negative_10000_get() / 10.0f) * (legoGlobs_currLevel->oxygenLevel / 10.0f));
            if (volume != sound_variant_property_read_original(sfx))
                sound_variant_property_set(sfx, volume);
        }
        global_value_0050080c_set(1);
    } else {
        if (LEGO_FIELD(&panelGlobs, int, 0x72c) && sound_alias_lookup("SND_AirBeat", &sfx))
            sound_handle_stop_forward(sound_variant_select(sfx));
        global_value_0050080c_set(0);
    }

    s_panel_highlight_timer -= elapsedWorld;
    return 1;
}

/* AirSupplyAmountUpdate [group E]: Lego_UpdateAirSupply-like. Drains or
 * refills the level's oxygen by one live object's oxygen coefficient. */
// FUNCTION: LEGORR 0x00424560
void lego_air_supply_amount_update(struct EntitySlot* liveObj, float elapsed)
{
    Lego_Level* level = legoGlobs_currLevel;
    float coef = entity_stats_float_atdc(liveObj);

    if (LEGO_FIELD(liveObj, int, 0x0) != 4 ||
        (LEGO_FIELD(liveObj, uint32_t, 0x3e8) & 0x20000000)) {
        level->oxygenLevel += coef * (level->oxygenRate * (elapsed * 0.04f));
        if (level->oxygenLevel > 100.0f) level->oxygenLevel = 100.0f;
        if (level->oxygenLevel <= 0.0f) {
            level->oxygenLevel = 0.0f;
            if (!(legoGlobs_flags1 & 0x8000000)) objective_announce_current(2);
        }
    }
}

/* TerrainCellFlagCleanup [group E]: the fall-ins that finished this
 * frame clear their blocks' busy flags. */
// FUNCTION: LEGORR 0x00424600
void lego_terrain_cell_flag_cleanup(Lego_Level* level, float elapsed)
{
    uint32_t* groups;
    uint32_t* slots;
    unsigned int count;
    unsigned int i;
    uint32_t x;
    uint32_t y;
    int pos[2];

    count = scene_resource_availability_sweep(elapsed, &groups, &slots);
    for (i = 0; i < count; i++) {
        scene_cell_table_pair_read(groups[i], slots[i], &x, &y);
        pos[0] = x;
        pos[1] = y;
        LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
            [y * legoGlobs_currLevel->width + x].flags1 &= ~0x800;
        if (!(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                  [y * legoGlobs_currLevel->width + x].flags1 & 0x1000)) {
            cmd_produce(0x14, 0, 0, pos);
        }
        LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
            [y * legoGlobs_currLevel->width + x].flags1 &= ~0x1000;
        LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
            [pos[1] * legoGlobs_currLevel->width + pos[0]].flags1 &= ~0x8000000;
    }
}

/* SinePhaseMidpointBlendDispatch [group E]: fog on or off; with high fog
 * on, the fog colour swings between FogColourRGB and HighFogColourRGB. */
// FUNCTION: LEGORR 0x00424730
void lego_sine_phase_midpoint_blend_dispatch(int on, float elapsed)
{
    float rgb[3];
    unsigned int i;
    float s;
    float half;

    if (legoGlobs_flags1 & 0x8000) {
        lego_fog_enable_adapter(on);
        if (on && (legoGlobs_flags1 & 0x10000)) {
            s = (float)sin(s_lego_highFogPhase += (6.2831855f / legoGlobs_f_556ef8) * elapsed);
            for (i = 0; i < 3; i++) {
                half = (legoGlobs_highFogColourRGB[i] - legoGlobs_fogColourRGB[i]) * 0.5f;
                rgb[i] = half * s + (half + legoGlobs_fogColourRGB[i]);
            }
            lego_fog_colour_set_adapter(rgb[0], rgb[1], rgb[2]);
        }
    }
}

/* The original compiles the next four functions without optimisation. */
#ifdef _MSC_VER
#pragma optimize("", off)
#endif

/* ApplyDefaultTransformToFlaggedRenderObject [group E]: a LegoObject
 * run-all callback; draws the selection box of each flagged object. */
// FUNCTION: LEGORR 0x004247d0
int lego_default_transform_apply_if_flagged(struct EntitySlot* liveObj,
                                            void* data)
{
    struct ViewProjectionAdapter* view = (struct ViewProjectionAdapter*)data;

    if (LEGO_FIELD(liveObj, uint32_t, 0x3ec) & 1) {
        lego_selected_entity_ground_marker_render(liveObj, view, 1.0f, 0.0f, 0.0f);
    }
    return 0;
}

/* ApplyDefaultTransformToFlaggedRenderObjects [group E] */
// FUNCTION: LEGORR 0x00424810
void lego_default_transform_apply_all(void* view)
{
    entity_pool_find_by_callback_filtered_current(
        lego_default_transform_apply_if_flagged, view);
}

/* SelectedObjectOutlinesDraw [group E]: the first selected unit's box in
 * green, the others' in yellow. */
// FUNCTION: LEGORR 0x00424830
void lego_selected_object_outlines_draw(struct ViewProjectionAdapter* view)
{
    uint32_t count;
    uint32_t loop;
    struct EntitySlot* const* unitList;

    count = selection_active_count_get();
    unitList = selection_active_records_get();
    for (loop = 0; loop < count; loop++) {
        if (loop == 0) lego_selected_entity_ground_marker_render(unitList[loop], view, 0.0f, 1.0f, 0.0f);
        else lego_selected_entity_ground_marker_render(unitList[loop], view, 1.0f, 1.0f, 0.0f);
    }
}

/* SelectedEntityGroundMarkerRender [group E]: the bevelled screen-space
 * box around one object, and its custom name above it. */
// FUNCTION: LEGORR 0x004248b0
void lego_selected_entity_ground_marker_render(
    struct EntitySlot* liveObj, struct ViewProjectionAdapter* view,
    float r, float g, float b)
{
    LegoPoint2FE fromList[8];
    LegoVector3FE worldPos;
    LegoVector3FE nearWorld;
    LegoPoint2FE toList[8];
    float radius;
    LegoVector3FE up = { 0.0f, 1.0f, 0.0f };
    LegoVector4FE sPos2;
    LegoVector4FE sPos;
    LegoVector3FE side;
    LegoVector3FE direction;
    float inset;

    if (!entity_type_operation_release(liveObj)) {
        if (legoGlobs_flags1 & 0x100000) {
            r *= 0.3f;
            g *= 0.3f;
            b *= 0.3f;
        }
        entity_transform_position_get_outputs(liveObj, &worldPos.x, &worldPos.y);
        worldPos.z = terrain_map_world_z_get((struct TerrainMap*)level_current_terrain_get_original(), worldPos.x, worldPos.y);
        worldPos.z -= entity_stats_float_at74(liveObj) / 2.0f;
        camera_project_point_to_screen(view, &sPos.x, &worldPos.x);

        if (sPos.x >= 0.0f && sPos.y >= 0.0f &&
            sPos.x <= display_width_get() && sPos.y <= display_height_get()) {
            sPos.z = 0.0f;
            sPos.w = 1.0f;
            camera_screen_coordinate_project(view, &nearWorld.x, &sPos.x);
            vec_sub(&direction.x, &worldPos.x, &nearWorld.x);
            vec_normalize(&direction.x);
            vec_cross(&side.x, &direction.x, &up.x);
            vec_normalize(&side.x);
            vec_scale(&side.x, &side.x, entity_stats_float_at78(liveObj));
            vec_add(&worldPos.x, &worldPos.x, &side.x);
            camera_project_point_to_screen(view, &sPos2.x, &worldPos.x);
            radius = vec2_dist(&sPos.x, &sPos2.x);
            radius /= 2.5f;
            inset = radius / 5.0f + 3.0f;

            fromList[0].x = sPos.x - radius;
            fromList[0].y = sPos.y - radius + inset;
            toList[0].x = sPos.x - radius;
            toList[0].y = sPos.y - radius;
            fromList[1].x = sPos.x - radius + 1.0f;
            fromList[1].y = sPos.y - radius;
            toList[1].x = sPos.x - radius + inset;
            toList[1].y = sPos.y - radius;
            fromList[2].x = sPos.x + radius - inset;
            fromList[2].y = sPos.y - radius;
            toList[2].x = sPos.x + radius;
            toList[2].y = sPos.y - radius;
            fromList[3].x = sPos.x + radius;
            fromList[3].y = sPos.y - radius + 1.0f;
            toList[3].x = sPos.x + radius;
            toList[3].y = sPos.y - radius + inset;
            fromList[4].x = sPos.x + radius;
            fromList[4].y = sPos.y + radius - inset;
            toList[4].x = sPos.x + radius;
            toList[4].y = sPos.y + radius;
            fromList[5].x = sPos.x + radius - 1.0f;
            fromList[5].y = sPos.y + radius;
            toList[5].x = sPos.x + radius - inset;
            toList[5].y = sPos.y + radius;
            fromList[6].x = sPos.x - radius + inset;
            fromList[6].y = sPos.y + radius;
            toList[6].x = sPos.x - radius;
            toList[6].y = sPos.y + radius;
            fromList[7].x = sPos.x - radius;
            fromList[7].y = sPos.y + radius - 1.0f;
            toList[7].x = sPos.x - radius;
            toList[7].y = sPos.y + radius - inset;
            surface_draw_polyline((struct Point2F*)fromList,
                                  (struct Point2F*)toList, 8, r, g, b, 0);

            if (!(legoGlobs_flags1 & 0x100000) &&
                LEGO_FIELD(liveObj, unsigned char*, 0x8) != NULL &&
                LEGO_FIELD(liveObj, unsigned char*, 0x8)[0] != '\0') {
                font_print_f(s_lego_crystal_count_font,
                    (int)(sPos.x - radius),
                    (int)(sPos.y - radius) - object_field_be4_dword_get(s_lego_crystal_count_font),
                    LEGO_FIELD(liveObj, char*, 0x8));
            }
        }
    }
}

#ifdef _MSC_VER
#pragma optimize("", on)
#endif

/* EngineShutdownAlt [group E]: the shutdown callback the configuration
 * installs when the game skips its normal shutdown. */
// FUNCTION: LEGORR 0x00424cf0
void lego_engine_shutdown_alt(void)
{
    lego_exit();
}

#include "lego_maths.h"

/* EngineShutdown [group E]: Lego_Shutdown_Full. */
// FUNCTION: LEGORR 0x00424d00
void lego_engine_shutdown(void)
{
    uint32_t i;

    global_resource_handle_release();
    reward_shutdown();
    lego_cd_audio_watchdog_mode_update(0);
    lego_audio_playback_toggle(0);
    lego_level_free();
    interface_shutdown();
    block_close(legoGlobs_config);
    render_resource_track_and_retain(legoGlobs_viewMain);
    ledger_stub_00472a40_DDiSceneObjectDestroy(legoGlobs_rootCont);
    camera_view_context_close(s_zoom_step_context);
    camera_view_context_close(legoGlobs_cameraTrack);
    camera_view_context_close(legoGlobs_cameraFP);

    for (i = 0; i < legoGlobs_catalogCounts[0]; i++) {
        entity_payload_type1_release(
            (struct ScenePairedAnimationState*)&legoGlobs_vehicleData[i]);
        lego_e_mem_free(legoGlobs_vehicleName[i]);
    }
    lego_e_mem_free(legoGlobs_vehicleName);
    lego_e_mem_free((void*)legoGlobs_objectNamesKind1);
    lego_e_mem_free(legoGlobs_vehicleData);

    for (i = 0; i < legoGlobs_catalogCounts[1]; i++) {
        entity_payload_release_subslots(
            (struct EntitySlot*)&legoGlobs_miniFigureData[i]);
        lego_e_mem_free(legoGlobs_miniFigureName[i]);
    }
    lego_e_mem_free(legoGlobs_miniFigureName);
    lego_e_mem_free((void*)legoGlobs_objectNamesKind2);
    lego_e_mem_free(legoGlobs_miniFigureData);

    for (i = 0; i < legoGlobs_catalogCounts[2]; i++) {
        entity_payload_release_subslots(
            (struct EntitySlot*)&legoGlobs_rockMonsterData[i]);
        lego_e_mem_free(legoGlobs_rockMonsterName[i]);
    }
    lego_e_mem_free(legoGlobs_rockMonsterName);
    lego_e_mem_free((void*)legoGlobs_objectNamesKind3);
    lego_e_mem_free(legoGlobs_rockMonsterData);

    for (i = 0; i < legoGlobs_catalogCounts[4]; i++) {
        render_resource_binding_release(
            (struct RenderResourceBinding*)&((LegoUpgradeModelE*)s_upgrade_records)[i]);
        lego_e_mem_free(legoGlobs_upgradeName[i]);
    }
    lego_e_mem_free(legoGlobs_upgradeName);
    lego_e_mem_free((void*)legoGlobs_objectNamesKind10);
    lego_e_mem_free(s_upgrade_records);

    for (i = 0; i < legoGlobs_catalogCounts[3]; i++) {
        entity_payload_teardown(
            (struct EntitySlot*)&legoGlobs_buildingData[i]);
        lego_e_mem_free(legoGlobs_buildingName[i]);
    }
    lego_e_mem_free(legoGlobs_buildingName);
    lego_e_mem_free((void*)legoGlobs_objectNamesKind4);
    lego_e_mem_free(legoGlobs_buildingData);

    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[0]);
    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[1]);
    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[2]);
    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[3]);
    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[4]);
    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[5]);
    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[6]);
    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[7]);
    lego_e_mem_free((void*)legoGlobs_objectNamesSpecial[8]);

    text_buffer_destroy_target(legoGlobs_textOnlyWindow);
    font_atlas_record_release(s_lego_text_window_font);
    font_atlas_record_release(s_lego_crystal_count_font);
    rock_fall_style_scene_resources_release();
    handle_array_release_and_reset();
    pooled_record_pool_resources_release();
    ddiscene_object_pool_shutdown();
    scene_resource_slots_release_all();

    if (s_application_mode == 10) {
        loader_profile_entries_release_adapter(
            media_cd_audio_playback_available()
            ? "LoaderProfile.txt"
            : "LoaderProfileNoSound.txt");
    } else {
        loader_profile_entries_release_adapter(NULL);
    }

    flic_opcode27_fallback_noop();
    scene_object_registry_teardown();
    lws_shutdown();
    audio_active_shutdown();
    directdraw_clear(NULL, 0);
}

/* ResetLevelAndExit [group E]: music off, then exit(0). */
// FUNCTION: LEGORR 0x004250a0
void lego_exit(void)
{
    lego_cd_audio_watchdog_mode_update(0);
    exit(0);
}

/* MainSimStep [group E]: the original's per-frame key handling (rename,
 * pause, quit, camera keys, debug toggles). Returns FALSE to quit. */
// FUNCTION: LEGORR 0x004250c0
int lego_main_sim_step(float elapsedGame, float elapsed, int* keyDownT,
                       int* keyDownR, int* keyDownAnyShift)
{
    struct EntitySlot* liveObj;
    float sPos[2];
    float wPos[3];

    if ((s_panel_highlight_flags & 0x20000) && !s_global_current_keys[0x1c] &&
        s_global_previous_keys[0x1c] && s_context_mode == 1 &&
        !(legoGlobs_flags1 & 0x100000) &&
        (liveObj = selection_active_entity_get()) != NULL &&
        LEGO_FIELD(liveObj, int, 0x0) == 2 &&
        (LEGO_FIELD(liveObj, int, 0x3f0) || LEGO_FIELD(liveObj, int, 0x330))) {
        ddiscene_object_position_get(entity_type_component_get(liveObj), NULL, wPos);
        wPos[2] -= entity_stats_float_at74(liveObj) * 0.5f;
        camera_project_point_to_screen((struct ViewProjectionAdapter*)legoGlobs_viewMain, sPos, wPos);
        if (sPos[0] >= 40.0f && sPos[1] >= 40.0f &&
            sPos[0] <= s_display_w - 40.0f && sPos[1] <= s_display_h - 80.0f) {
            lego_transition_control(0, 1);
            if (LEGO_FIELD(liveObj, char*, 0x8) == NULL) {
                lego_object_custom_name_set_adapter(liveObj,
                    // STRING: LEGORR 0x004a4128
                    "Object");
                LEGO_FIELD(liveObj, char*, 0x8)[0] = '\0';
            }
            legoGlobs_renamePosition[1] = sPos[1];
            legoGlobs_renameInput = LEGO_FIELD(liveObj, char*, 0x8);
            legoGlobs_renamePosition[0] = sPos[0];
        }
    }

    if (s_global_current_keys[0x01] && !(s_panel_highlight_flags & 0x100) &&
        !main_sim_state_admission_predicate() &&
        !lego_level_low_bits_enabled_adapter() &&
        !(legoGlobs_flags1 & 0x8000000)) {
        if (s_panel_highlight_flags & 0x10) {
            if (s_global_current_keys[0x39]) {
                audio_active_shutdown();
                lego_exit();
            } else if (s_global_current_keys[0x1c]) {
                return 0;
            }
        }
        lego_transition_control(0, 1);
        legoGlobs_flags1 |= 0x2000000;
    } else if (s_global_current_keys[0x01] && !(s_panel_highlight_flags & 0x100) &&
               !main_sim_state_admission_predicate() &&
               !lego_level_low_bits_enabled_adapter() &&
               !(legoGlobs_flags1 & 0x8000000)) {
        audio_active_shutdown();
        lego_exit();
    }

    if (s_panel_highlight_flags & 0x2) {
        if (frontend_enabled_original() && !s_global_current_keys[0x39]) {
            if (s_global_current_keys[0x1c]) return 0;
            if (!lego_keep_running_adapter()) return 0;
        } else {
            audio_active_shutdown();
            lego_exit();
        }
    }

    *keyDownT = 0;
    *keyDownR = 0;
    *keyDownAnyShift = 0;

    if (!(legoGlobs_flags1 & 0x8100000)) {
        advisor_callback_schedule_gate();
        if (s_global_current_keys[0x39] && (s_global_previous_keys[0x39] ^ s_global_current_keys[0x39])) {
            state_flag_toggle_when_zero();
        }
        if (s_global_current_keys[0x2a] || s_global_current_keys[0x36]) *keyDownAnyShift = 1;

        if (s_context_mode == 1 && !(system_state_flags_get_original(0) & 0x1000)) {
            if (s_global_current_keys[0xd0]) camera_pan_axis_update(s_zoom_step_context, elapsed * 0.02f);
            if (s_global_current_keys[0xc8]) camera_pan_axis_update(s_zoom_step_context, elapsed * -0.02f);
            if (s_global_current_keys[0xcb]) camera_tilt_axis_update(s_zoom_step_context, elapsed * 0.02f);
            if (s_global_current_keys[0xcd]) camera_tilt_axis_update(s_zoom_step_context, elapsed * -0.02f);
            if (s_global_current_keys[0x0c]) camera_zoom_update(s_zoom_step_context, elapsed * 3.0f);
            if (s_global_current_keys[0x0d]) camera_zoom_update(s_zoom_step_context, elapsed * -3.0f);
        }

        if (s_panel_highlight_flags & 0x10) {
            if (s_global_current_keys[0x47]) lego_game_speed_set(3.0f);
            if (s_global_current_keys[0x14]) *keyDownT = 1;
            if (s_global_current_keys[0x13]) *keyDownR = 1;
            if (s_global_current_keys[0x26] && (s_global_previous_keys[0x26] ^ s_global_current_keys[0x26]) &&
                !lego_keep_running_adapter()) {
                return 0;
            }
            if (s_global_current_keys[0x1d] && s_global_current_keys[0x21] &&
                (s_global_previous_keys[0x21] ^ s_global_current_keys[0x21])) {
                if (legoGlobs_flags1 & 0x40) legoGlobs_flags1 &= ~0x40;
                else legoGlobs_flags1 |= 0x40;
                text_buffer_append_format(legoGlobs_textOnlyWindow,
                    // STRING: LEGORR 0x004a4108
                    "\nFrame Rate Monitor: %s",
                    (legoGlobs_flags1 & 0x40) ? "ON" : "OFF");
            }
            if ((legoGlobs_flags1 & 0x2) && s_global_current_keys[0x0f] &&
                (s_global_previous_keys[0x0f] ^ s_global_current_keys[0x0f])) {
                if (legoGlobs_flags1 & 0x2000) {
                    legoGlobs_flags1 &= ~0x2000;
                    legoGlobs_flags1 |= 0x1000;
                    panel_item_flag16_set(0, 2, 0);
                    panel_item_flag16_set(0, 3, 0);
                } else {
                    legoGlobs_flags1 |= 0x2000;
                    legoGlobs_flags1 &= ~0x1000;
                    panel_item_flag16_set(0, 2, 1);
                    panel_item_flag16_set(0, 3, 1);
                }
            }
            if (s_global_current_keys[0x1d]) {
                if (s_global_current_keys[0x22] && (s_global_previous_keys[0x22] ^ s_global_current_keys[0x22])) {
                    if (legoGlobs_flags1 & 0x80) legoGlobs_flags1 &= ~0x80;
                    else legoGlobs_flags1 |= 0x80;
                    text_buffer_append_format(legoGlobs_textOnlyWindow,
                        // STRING: LEGORR 0x004a40f4
                        "\nMemory Monitor: %s",
                        (legoGlobs_flags1 & 0x80) ? "ON" : "OFF");
                }
                if (s_global_current_keys[0x1d] && s_global_current_keys[0x1c] &&
                    (s_global_previous_keys[0x1c] ^ s_global_current_keys[0x1c])) {
                    if (legoGlobs_flags1 & 0x40000000) legoGlobs_flags1 &= ~0x40000000;
                    else legoGlobs_flags1 |= 0x40000000;
                }
            }
        }

        if (s_context_mode == 0 &&
            !(LEGO_FIELD(legoGlobs_selectedObject, uint32_t, 0x3e4) & 1)) {
            if (s_global_current_keys[0xc8]) lego_object_movement_input_adapter((struct EntitySlot*)legoGlobs_selectedObject, 1, 0, 0.0f);
            if (s_global_current_keys[0xd0]) lego_object_movement_input_adapter((struct EntitySlot*)legoGlobs_selectedObject, -1, 0, 0.0f);
            if (s_global_current_keys[0xcb]) lego_object_movement_input_adapter((struct EntitySlot*)legoGlobs_selectedObject, 0, 0, -0.05f);
            if (s_global_current_keys[0xcd]) lego_object_movement_input_adapter((struct EntitySlot*)legoGlobs_selectedObject, 0, 0, 0.05f);
            if (s_global_current_keys[0x2c]) lego_object_movement_input_adapter((struct EntitySlot*)legoGlobs_selectedObject, 0, -1, 0.0f);
            if (s_global_current_keys[0x2d]) lego_object_movement_input_adapter((struct EntitySlot*)legoGlobs_selectedObject, 0, 1, 0.0f);
        }

        if (s_panel_highlight_flags & 0x10) {
            if (s_global_current_keys[0x1f] && (s_global_previous_keys[0x1f] ^ s_global_current_keys[0x1f])) {
                if (legoGlobs_flags1 & 0x10) {
                    lego_audio_playback_toggle(0);
                } else {
                    lego_audio_playback_toggle(1);
                    sound_alias_play(7, 1);
                }
                text_buffer_append_format(legoGlobs_textOnlyWindow,
                    // STRING: LEGORR 0x004a40e0
                    "\nSound Effects: %s",
                    (legoGlobs_flags1 & 0x10) ? "ON" : "OFF");
            }
            if (s_global_current_keys[0x32] && (s_global_previous_keys[0x32] ^ s_global_current_keys[0x32])) {
                if (legoGlobs_flags1 & 0x8) lego_cd_audio_watchdog_mode_update(0);
                else lego_cd_audio_watchdog_mode_update(1);
                text_buffer_append_format(legoGlobs_textOnlyWindow,
                    // STRING: LEGORR 0x004a40d4
                    "\nMusic: %s",
                    (legoGlobs_flags1 & 0x8) ? "ON" : "OFF");
            }
        }

        if ((s_panel_highlight_flags & 0x20) && s_global_current_keys[0x52] &&
            (s_global_previous_keys[0x52] ^ s_global_current_keys[0x52])) {
            entity_b8_bit3_set_or_clear(s_zoom_step_context, 0);
            surface_visibility_set(s_refresh_record, 1);
        }

        if ((s_panel_highlight_flags & 0x10) && s_global_current_keys[0x2c] &&
            (s_global_previous_keys[0x2c] ^ s_global_current_keys[0x2c])) {
            entity_fields_a0_a4_store_a8_clear_original(s_zoom_step_context, 5.0f, 25.0f);
            if (selection_active_entity_get()) {
                LEGO_FIELD(selection_active_entity_get(), uint32_t, 0x3e0) |= 0x40000000;
                ai_set_activity(selection_active_entity_get(), 0x23, 0);
                activity_register_bookkeeping(selection_active_entity_get());
                ai_idle_chain_record_attach(selection_active_entity_get());
            }
        }
    }

    {
        int mouseReleased = 0;
        int quit = 0;

        if (s_lego_prevMouseWord && !g_renderer_word_c) mouseReleased = 1;
        if (!lego_quit_poll_adapter(s_global_current_keys[0x39] &&
                                        (s_global_previous_keys[0x39] ^ s_global_current_keys[0x39]),
                                    mouseReleased, &quit) &&
            !main_sim_state_admission_predicate() &&
            !lego_level_low_bits_enabled_adapter() &&
            !(legoGlobs_flags1 & 0x8000000) && !(s_panel_highlight_flags & 0x100)) {
            if (s_global_current_keys[0x19] && (s_global_previous_keys[0x19] ^ s_global_current_keys[0x19])) {
                lego_transition_control(1, 0);
                legoGlobs_flags1 ^= 0x2000000;
            }
            if (!s_window_application_active &&
                block_get_bool_path(legoGlobs_config,
                    string_path_join(s_engine_config_root, "Main",
                                     // STRING: LEGORR 0x004a40c0
                                     "LoseFocusAndPause",
                                     NULL)) == 1 &&
                !(legoGlobs_flags1 & 0x8000000)) {
                lego_transition_control(0, 1);
                if (!(legoGlobs_flags1 & 0x2000000)) legoGlobs_flags1 |= 0x2000000;
            }
        }
        s_lego_prevMouseWord = g_renderer_word_c;
        if (quit) return 0;
    }

    if (s_panel_highlight_flags & 0x10) {
        if (s_global_current_keys[0x2a]) {
            if (s_global_current_keys[0x06]) {
                s_lego_spotPenumbra -= -0.02f;
                lego_light_penumbra_set_adapter((void*)(uintptr_t)legoGlobs_f_556104, s_lego_spotPenumbra);
            }
            if (s_global_current_keys[0x07]) {
                s_lego_spotUmbra -= -0.02f;
                lego_light_umbra_set_adapter((void*)(uintptr_t)legoGlobs_f_556104, s_lego_spotUmbra);
            }
        } else {
            if (s_global_current_keys[0x06]) {
                s_lego_spotPenumbra -= 0.02f;
                lego_light_penumbra_set_adapter((void*)(uintptr_t)legoGlobs_f_556104, s_lego_spotPenumbra);
            }
            if (s_global_current_keys[0x07]) {
                s_lego_spotUmbra -= 0.02f;
                lego_light_umbra_set_adapter((void*)(uintptr_t)legoGlobs_f_556104, s_lego_spotUmbra);
            }
        }
    }

    if ((s_panel_highlight_flags & 0x10) || (s_engine_feature_word & 0x8000)) {
        if (s_global_current_keys[0x1d]) {
            if (s_global_current_keys[0x20] && (s_global_previous_keys[0x20] ^ s_global_current_keys[0x20])) {
                objective_voice_alias_raise_gate_bit27(0);
            }
            if (s_global_current_keys[0x1f] && (s_global_previous_keys[0x1f] ^ s_global_current_keys[0x1f])) {
                objective_voice_alias_raise_commit(0);
            }
        }
        if (s_global_current_keys[0x9d] && s_global_current_keys[0x1f] &&
            (s_global_previous_keys[0x1f] ^ s_global_current_keys[0x1f])) {
            objective_announce_current(3);
        }
        if (s_global_current_keys[0x18]) legoGlobs_currLevel->oxygenLevel -= 1.0f;
    }
    return 1;
}

/* PeriodicCallbackDispatch [group E]: Lego_SetSoundOn-like; stops and/or
 * restarts every object's looping sounds. */
// FUNCTION: LEGORR 0x00425b40
void lego_periodic_callback_dispatch(int stop)
{
    entity_pool_find_by_callback_filtered_current(
        lego_object_sound_restart_callback, &stop);
}

/* The LegoObject run-all callback of lego_periodic_callback_dispatch:
 * stop (if asked) and restart one object's looping sounds. [group E] */
// FUNCTION: LEGORR 0x00425b60
int lego_object_sound_restart_callback(struct EntitySlot* liveObj, void* data)
{
    void* cont = entity_type_component_get(liveObj);
    int* stop;

    if (cont) {
        stop = (int*)data;
        if (!(LEGO_FIELD(liveObj, uint32_t, 0x3ec) & 0x20000)) {
            if (*stop) sound_handle_stop_forward(LEGO_FIELD(liveObj, int, 0x350));
            if (LEGO_FIELD(liveObj, uint32_t, 0x3e0) & 0x8) {
                LEGO_FIELD(liveObj, int, 0x350) = sound_submission_route_wrapper(
                    cont, entity_stats_word_at_c0_or_c4(liveObj, 0), 1, 1, NULL);
                LEGO_FIELD(liveObj, uint32_t, 0x3ec) |= 0x100000;
            }
        }
        if (*stop) sound_handle_stop_forward(LEGO_FIELD(liveObj, int, 0x354));
        {
            uint32_t sfx = entity_stats_word_atc8(liveObj);
            if (sfx) {
                LEGO_FIELD(liveObj, int, 0x354) = sound_submission_route_wrapper(
                    cont, sfx, 1, 1, NULL);
                LEGO_FIELD(liveObj, uint32_t, 0x3ec) |= 0x80000;
            }
        }
    }
    return 0;
}

/* TransitionControl [group G] */
// FUNCTION: LEGORR 0x00425c30
void lego_transition_control(int toggle, unsigned int paused)
{
    if (!(s_panel_highlight_flags & 2)) {
        if (toggle) paused = !(legoGlobs_flags1 & 0x100000);
        if (paused) {
            if (!(legoGlobs_flags1 & 0x100000)) sound_subsystem_stop_all();
            legoGlobs_flags1 |= 0x100000;
            s_panel_highlight_flags |= 0x1000;
        } else {
            lego_periodic_callback_dispatch(0);
            sound_alias_play(7, 1);
            legoGlobs_flags1 &= ~0x100000;
            s_panel_highlight_flags &= ~0x1000;
        }
    }
}

/* Global004de548Set [group M] */
// FUNCTION: LEGORR 0x00425cd0
void global_scroll_guard_set(int value)
{
    s_scroll_guard = value;
}

/* ScrollScaleSetAndClamp [group M] */
// FUNCTION: LEGORR 0x00425ce0
void lego_game_speed_set(float speed)
{
    if (speed >= s_scroll_scale || s_scroll_guard == 0) {
        options_scroll_index_store_current(speed);
        s_scroll_scale = speed;
        if (speed < 0.0f) s_scroll_scale = 0.0f;
        if (s_scroll_scale > 3.0f) s_scroll_scale = 3.0f;
    }
}

/* GlobalRenderRecordSetFromInput [group C] */
// FUNCTION: LEGORR 0x00425d50
void lego_render_record_set_from_input(struct EntitySlot* object)
{
    screen_effect_record_set_original(legoGlobs_cameraTrack, object, 2.0f,
                                     entity_stats_float_at7c(object),
                                     0.7f, 0.01f);
}

/* CurrentEngineModeIsZero [group M] */
// FUNCTION: LEGORR 0x00425d80
int current_engine_mode_is_zero(void)
{
    return s_context_mode == 0;
}

/* TerrainPointerCommandUpdate [group F] */
// FUNCTION: LEGORR 0x00425d90
void lego_terrain_pointer_command_update(void)
{
    Lego_Level* level;
    float scroll[2];
    unsigned int bx, by;
    struct EntitySlot* liveObj;
    float objPos[2];
    unsigned int clickX, clickY;

    level = legoGlobs_currLevel;
    if ((legoGlobs_flags1 & 2) && (legoGlobs_flags1 & 0x1000) && !(legoGlobs_flags1 & 0x100)) {
        {
            float diff[2];

            diff[0] = (float)(int)s_renderer_word_a - (float)s_lego_mouseLastX;
            diff[1] = (float)(int)s_renderer_word_b - (float)s_lego_mouseLastY;
            media_pointer_cursor_select(0);
            if (!(system_state_flags_get_original(0) & 0x1000))
                s_lego_radarDragDistance += (float)sqrt(diff[0] * diff[0] + diff[1] * diff[1]);

            if (s_lego_radarDragging && g_renderer_word_c && s_lego_radarDragDistance >= 4.0f &&
                terrain_point_pick_resolve_original(
                    (const struct TerrainPointPickRegion*)LEGO_LEVEL_PTR(void, level->f_084),
                    s_renderer_word_a, s_renderer_word_b, scroll, &bx, &by)) {
                selection_record_point_get((struct ViewContext*)s_zoom_step_context, diff);
                if (bx < (unsigned int)view_grid_edge_get() ||
                    bx >= LEGO_FIELD(LEGO_LEVEL_PTR(void, level->map), unsigned int, 0x0) - view_grid_edge_get())
                    scroll[0] = diff[0];
                if (by < (unsigned int)view_grid_edge_get() ||
                    by >= LEGO_FIELD(LEGO_LEVEL_PTR(void, level->map), unsigned int, 0x4) - view_grid_edge_get())
                    scroll[1] = diff[1];
                if (s_context_mode == 1)
                    selection_record_point_set((struct ViewContext*)s_zoom_step_context, scroll[0], scroll[1]);
                media_pointer_cursor_select(0xc);
                legoGlobs_flags1 |= 0x200;
            }
        }

        if (rect_contains_point_half_open((const float*)LEGO_LEVEL_PTR(void, level->f_084),
                                          s_renderer_word_a, s_renderer_word_b) &&
            g_renderer_word_c && !s_lego_radarPrevButton)
            s_lego_radarDragging = 1;

        if (s_lego_radarDragDistance < 4.0f) {
            if (viewport_point_callback_dispatch(
                    (const struct TerrainPointPickRegion*)LEGO_LEVEL_PTR(void, level->f_084),
                    s_renderer_word_a, s_renderer_word_b, (uint32_t*)&liveObj, objPos)) {
                if (!g_renderer_word_c) {
                    if (s_lego_radarPrevButton) {
                        if (s_context_mode == 1)
                            selection_record_point_set((struct ViewContext*)s_zoom_step_context, objPos[0], objPos[1]);
                        screen_effect_record_set_original(legoGlobs_cameraTrack, liveObj, 2.0f,
                                                          entity_stats_float_at7c(liveObj), 0.7f, 0.01f);
                        legoGlobs_flags1 &= ~0x4000;
                    } else {
                        media_pointer_cursor_select(0xd);
                        tooltip_set_content(2, "%s", entity_display_name_lookup(liveObj));
                        timed_effect_cue_request_set_corpus(2, terrain_object_type_sfx_at_cell_get(liveObj));
                        timed_effect_update_request_set_corpus(2);
                    }
                }
            } else if (!g_renderer_word_c) {
                if (s_lego_radarPrevButton) {
                    if (terrain_point_pick_resolve_original(
                            (const struct TerrainPointPickRegion*)LEGO_LEVEL_PTR(void, level->f_084),
                            s_renderer_word_a, s_renderer_word_b, &legoGlobs_f_5563f8.x, &clickX, &clickY))
                        legoGlobs_flags1 |= 0x4000;
                } else if (terrain_point_pick_resolve_original(
                               (const struct TerrainPointPickRegion*)LEGO_LEVEL_PTR(void, level->f_084),
                               s_renderer_word_a, s_renderer_word_b, NULL, &bx, &by)) {
                    if (s_lego_radarBlockX == bx && s_lego_radarBlockY == by) {
                        if ((bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1)
                                ? LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                                      [by * legoGlobs_currLevel->width + bx].flags1 & 4
                                : 0) {
                            int pos[2];
                            pos[0] = bx;
                            pos[1] = by;
                            lego_terrain_command_feedback_dispatch(pos, 0, 1, 1);
                        }
                    } else {
                        s_lego_radarBlockX = bx;
                        s_lego_radarBlockY = by;
                        timed_effect_update_request_set_corpus(0);
                    }
                }
            }
        }

        if (!g_renderer_word_c) {
            s_lego_radarDragDistance = 0.0f;
            s_lego_radarDragging = 0;
        }
        s_lego_radarPrevButton = g_renderer_word_c;
        s_lego_mouseLastX = s_renderer_word_a;
        s_lego_mouseLastY = s_renderer_word_b;
    }
}

/* SpawnCoordinatePoolTick [group F] */
// FUNCTION: LEGORR 0x004261c0
void lego_spawn_coordinate_pool_tick(float elapsed)
{
    if (legoGlobs_currLevel->f_26c != 0x14) {
        g_camera_countdown -= elapsed;
        if (g_camera_countdown < 0.0f) {
            if (spawn_registry_probe_adapter(0, 0))
                predug_spawn_coordinate_select_and_create(0, legoGlobs_currLevel->f_26c);
            g_camera_countdown = legoGlobs_currLevel->f_268;
        }
    }
}

/* GlobalTripleCopyOut [group M] */
// FUNCTION: LEGORR 0x00426230
void lego_global_triple_copy_out(uint32_t* out)
{
    out[0] = s_triple_first;
    out[1] = s_triple_second;
    out[2] = s_triple_third;
}

/* UpdateRenderViewportState [group C] */
// FUNCTION: LEGORR 0x00426250
void lego_update_render_viewport_state(void)
{
    LegoPoint2FC pos;

    if (LEGO_FIELD(legoGlobs_cameraTrack, void*, 4) == NULL)
        legoGlobs_flags1 |= 0x4000;
    if (legoGlobs_flags1 & 0x4000) {
        pos = legoGlobs_f_5563f8;
    } else {
        entity_transform_position_get_outputs(
            LEGO_FIELD(legoGlobs_cameraTrack, struct EntitySlot*, 4),
            &pos.x, &pos.y);
    }
    radar_map_zoom_set_adapter((void*)(uintptr_t)legoGlobs_currLevel->f_084,
                               s_lego_radar_zoom);
    radar_map_tv_position_set_adapter(
        (void*)(uintptr_t)legoGlobs_currLevel->f_084, &pos.x);
}

/* ObjectiveScoreTextOriginSet [group M] */
// FUNCTION: LEGORR 0x004262e0
void objective_score_text_origin_set(const float* pair)
{
    if (pair != NULL) {
        s_score_text_origin_x = pair[0];
        s_score_text_origin_y = pair[1];
        s_panel_highlight_flags |= 0x4000;
    } else {
        s_panel_highlight_flags &= ~0x4000;
    }
}

/* ObjectiveTaskTextOriginSet [group M] */
// FUNCTION: LEGORR 0x00426320
void lego_objective_task_text_origin_set(const float* pair)
{
    if (pair != NULL) {
        s_task_text_origin_x = pair[0];
        s_task_text_origin_y = pair[1];
        s_panel_highlight_flags |= 0x8000;
    } else {
        s_panel_highlight_flags &= ~0x8000;
    }
}

/* GlobalStateFlagsBits6And9Set [group M] */
// FUNCTION: LEGORR 0x00426360
void engine_global_state_flags_bits6_and_9_set_target(int value,
                                                      int set_bit9)
{
    if (value) s_panel_highlight_flags |= 0x40;
    else s_panel_highlight_flags &= ~0x40;
    if (set_bit9) s_panel_highlight_flags |= 0x200;
    else s_panel_highlight_flags &= ~0x200;
}

/* GlobalStateFlagsBit7Set [group M] */
// FUNCTION: LEGORR 0x004263a0
void engine_global_state_flags_bit7_set_target(int value)
{
    if (value) s_panel_highlight_flags |= 0x80;
    else s_panel_highlight_flags &= ~0x80;
}

/* GlobalSubsystemStateReset [group G] */
// FUNCTION: LEGORR 0x004263c0
void lego_global_subsystem_state_reset(void)
{
    int mode = 1;

    s_overlay_word_03610 = 0;
    engine_mode_set_target(&mode);
    textGlobs_f_503618 = 0;
    text_buffer_reset_target(legoGlobs_textOnlyWindow);
    engine_global_state_flags_bits6_and_9_set_target(0, 0);
    engine_global_state_flags_bit7_set_target(0);
}


/* CursorRayToCameraRoute [group C] */
// FUNCTION: LEGORR 0x00426420
void lego_cursor_ray_to_camera_route(float elapsed)
{
    LegoVector3FC wPos;
    LegoVector3FC centre;
    LegoVector3FC dir;

    if (s_context_mode == 1 && !(system_state_flags_get_original(NULL) & 0x1000)) {
        if (legoInput_mouseX >= legoGlobs_MouseScrollBorder &&
            legoInput_mouseY >= legoGlobs_MouseScrollBorder &&
            legoInput_mouseX <= s_display_w - legoGlobs_MouseScrollBorder &&
            legoInput_mouseY <= s_display_h - legoGlobs_MouseScrollBorder) {
            cursor_screen_ray_context_reset((struct ViewContext*)s_zoom_step_context);
            return;
        }
        camera_topdown_position_get_adapter(
            s_zoom_step_context, LEGO_LEVEL_PTR(void, legoGlobs_currLevel->map),
            &centre.x);
        lego_camera_pointer_ray_route(legoInput_mouseX, legoInput_mouseY,
                                      &wPos.x, &wPos.y);
        wPos.z = centre.z = 0.0f;
        lego_C_vector3_subtract(&dir, &wPos, &centre);
        camera_pointer_pan_update((struct ViewContext*)s_zoom_step_context,
                                  &dir.x, elapsed);
    }
}

/* MainSimInteractionAndCellUpdate [group C] */
// FUNCTION: LEGORR 0x00426520
void lego_main_sim_interaction_and_cell_update(float elapsedGame,
                                               float elapsedAbs,
                                               int keyDownT, int keyDownR,
                                               int keyDownShift)
{
    Lego_Level* level = legoGlobs_currLevel;
    int leftReleased;
    struct EntitySlot* liveObj;
    unsigned int by;
    unsigned int bx;
    int rightReleased;
    int overPanel;
    int placed;
    LegoPoint2FC placePos;
    LegoPoint2FC screenPt;
    LegoVector3FC wPos;
    int picked;
    Lego_Block* block;
    uint32_t pointer;
    int handled;
    int done;

    s_panel_highlight_flags &= ~0x400;
    liveObj = NULL;
    placed = 0;
    overPanel = 0;
    leftReleased = 0;
    rightReleased = 0;
    surface_visibility_set(s_refresh_record, 1);

    if (!legoInput_mslb && !legoInput_msrb) s_lego_hold_time += elapsedGame;
    else s_lego_hold_time = 0.0f;

    if (s_lego_prev_left_button) {
        if (legoInput_mslb) s_lego_dragging = 1;
        else if (!(legoGlobs_flags1 & 0x100)) leftReleased = 1;
    }
    if (!legoInput_mslb) s_lego_dragging = 0;
    if (s_lego_prev_right_button && !legoInput_msrb) rightReleased = 1;

    if (s_lego_dragging) {
        s_lego_drag_x += (float)abs(s_cursor_delta_x);
        s_lego_drag_y += (float)abs(s_cursor_delta_y);
    } else {
        s_lego_drag_x = s_lego_drag_y = 0.0f;
    }

    help_window_button_hit_test(legoInput_mouseX, legoInput_mouseY,
                                legoInput_mslb, s_lego_prev_left_button,
                                leftReleased, elapsedGame);

    if (legoGlobs_flags1 & 0x100000) {
        media_pointer_cursor_select(0);
        legoGlobs_flags1 &= ~0x300;
        s_lego_prev_left_button = legoInput_mslb;
        s_lego_prev_right_button = legoInput_msrb;
        s_lego_reset_buttons = 1;
        return;
    }
    if (s_lego_reset_buttons) {
        s_lego_prev_left_button = 0;
        s_lego_prev_right_button = 0;
        s_lego_dragging = 0;
        leftReleased = 0;
        rightReleased = 0;
        s_lego_reset_buttons = 0;
    }

    if (terrain_map_screen_pick(
            LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
            legoGlobs_viewMain, legoInput_mouseX, legoInput_mouseY, &bx, &by,
            (struct TerrainMapVector3*)&wPos)) {
        picked = 1;
        block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx];
        ddiscene_object_position_set(legoGlobs_f_556108, NULL, wPos.x, wPos.y,
                                     wPos.z - 250.0f);
        simtick_route_position_blocked_state_set(0);
    } else {
        picked = 0;
        simtick_route_position_blocked_state_set(1);
    }

    if (legoGlobs_flags1 & 0x8000000) {
        lego_cursor_ray_to_camera_route(elapsedAbs);
        media_pointer_cursor_select(0);
        legoGlobs_flags1 &= ~0x300;
        return;
    }

    pointer = global_cursor_word_get();
    if (!(legoGlobs_flags1 & 0x100) && selection_active_has_any_get())
        media_pointer_cursor_select(2);
    else
        media_pointer_cursor_select(0);

    if (!(legoGlobs_flags1 & 0x100) &&
        panel_cursor_interaction_update(elapsedAbs, legoInput_mouseX,
                                        legoInput_mouseY, legoInput_mslb,
                                        s_lego_prev_left_button, &overPanel)) {
        lego_cursor_ray_to_camera_route(elapsedAbs);
        legoGlobs_flags1 |= 0x200;
        goto end;
    }
    lego_cursor_ray_to_camera_route(elapsedAbs);

    if (!(legoGlobs_flags1 & 0x100) &&
        priorities_handle_cursor(legoInput_mouseX, legoInput_mouseY,
                                 legoInput_mslb, s_lego_prev_left_button,
                                 leftReleased)) {
        legoGlobs_flags1 |= 0x200;
        goto end;
    }
    if (!(legoGlobs_flags1 & 0x100) && !(s_panel_highlight_flags & 0x1000) &&
        interface_handle_mouse_adapter(legoInput_mouseX, legoInput_mouseY,
                                       legoInput_mslb, s_lego_prev_left_button,
                                       leftReleased, elapsedAbs)) {
        legoGlobs_flags1 |= 0x200;
        goto end;
    }
    if (!(legoGlobs_flags1 & 0x100) &&
        info_check_click_adapter(legoInput_mouseX, legoInput_mouseY,
                                 leftReleased, rightReleased)) {
        legoGlobs_flags1 |= 0x200;
        if (leftReleased) media_action_feedback_sound_dispatch(0);
        goto end;
    }

    if (s_context_mode == 0) {
        legoGlobs_flags1 |= 0x200;
        goto end;
    }

    if (!(legoGlobs_flags1 & 0x100) && (legoGlobs_flags1 & 2) &&
        rect_contains_point_half_open(
            (const float*)(uintptr_t)level->f_084, legoInput_mouseX,
            legoInput_mouseY)) {
        if (leftReleased) media_action_feedback_sound_dispatch(0);
        if (!(legoGlobs_flags1 & 0x2000)) media_pointer_cursor_select(pointer);
        goto end;
    }

    if (!overPanel) {
        if (picked) {
            s_triple_first = *(uint32_t*)&wPos.x;
            s_triple_second = *(uint32_t*)&wPos.y;
            s_triple_third = *(uint32_t*)&wPos.z;
            s_panel_highlight_flags |= 0x400;
            legoGlobs_f_556fb8[0] = bx;
            legoGlobs_f_556fb8[1] = by;

            if (!(legoGlobs_flags1 & 0x100) || (s_global_low_word & 0x81)) {
                if (s_global_low_word & 0x80) {
                    placePos.x = wPos.x;
                    placePos.y = wPos.y;
                    placed = 1;
                    if (!building_placement_update_adapter(
                            legoGlobs_f_556440, legoGlobs_f_556444,
                            leftReleased, 0, &placePos.x, bx, by, 1,
                            s_refresh_record))
                        s_global_low_word &= ~0x80;
                    if (leftReleased) block->f_10[6]++;
                } else if (entity_task_at_grid_coordinate_find(
                               legoInput_mouseX, legoInput_mouseY, &liveObj)) {
                    /* An object under the pointer. */
                    if (LEGO_FIELD(liveObj, int, 0) == 2 &&
                        LEGO_FIELD(liveObj, struct EntitySlot*, 0x36c) != NULL)
                        liveObj = LEGO_FIELD(liveObj, struct EntitySlot*, 0x36c);
                    if (LEGO_FIELD(liveObj, struct EntitySlot*, 0x304) != NULL)
                        liveObj = LEGO_FIELD(liveObj, struct EntitySlot*, 0x304);

                    if (LEGO_FIELD(liveObj, int, 0) == 1 &&
                        selection_active_has_any_get() &&
                        LEGO_FIELD(selection_active_entity_get(), int, 0) == 2) {
                        struct EntitySlot* obj = liveObj;
                        struct EntitySlot* unit = selection_active_entity_get();
                        uint32_t mask = 2;
                        if (entity_capability_flags_get(obj) & 0x100) {
                            mask = 1;
                        } else if (entity_stats_word_get(obj) & 0x2000000) {
                            mask = (entity_stats_word_get(obj) & 0x1000000) ? 1 : 4;
                        }
                        if (LEGO_FIELD(unit, uint32_t, 0x3f0) & mask) {
                            if (leftReleased) {
                                entity_kind_command_issue(obj);
                                media_action_feedback_sound_dispatch(0);
                                selection_clear_request_and_hud_reset();
                            } else {
                                media_pointer_cursor_select(0x11);
                            }
                        }
                    } else if (!(s_global_low_word & 0x20) && !(s_global_low_word & 0x40)) {
                        if (!(s_global_low_word & 8) && !(s_global_low_word & 0x10)) {
                            if ((s_global_low_word & 2) || (s_global_low_word & 4)) {
                                if (s_global_low_word & 2) media_pointer_cursor_select(0x25);
                                else media_pointer_cursor_select(0x26);
                            } else if (s_global_low_word & 1) {
                                legoGlobs_flags1 |= 0x200;
                                media_pointer_cursor_select(0xe);
                                advisor_text_animation_update(8, 0, 0);
                                if (leftReleased) {
                                    media_action_feedback_sound_dispatch(0);
                                    terrain_choice_resource_select(liveObj);
                                }
                            } else if (entity_shared_eligibility_check(liveObj, 1)) {
                                int objPos[2];
                                media_pointer_cursor_select(2);
                                if (!s_lego_dragging)
                                    lego_entity_command_status_feedback_dispatch(liveObj);
                                if (leftReleased) {
                                    if ((system_state_flags_get_original(NULL) & 8) ||
                                        (system_state_flags_get_original(NULL) &&
                                         LEGO_FIELD(liveObj, int, 0x3f8) == 0)) {
                                        media_action_feedback_sound_dispatch(1);
                                    } else if (cell_action_permission_get(
                                                   LEGO_FIELD(liveObj, int, 0),
                                                   LEGO_FIELD(liveObj, int, 4)) &&
                                               !(LEGO_FIELD(liveObj, uint32_t, 0x3e4) & 2) &&
                                               (LEGO_FIELD(liveObj, int, 0) != 1 ||
                                                !(LEGO_FIELD(liveObj, uint32_t, 0x3e0) & 0x40000)) &&
                                               (!(legoGlobs_flags1 & 0x10000000) ||
                                                (LEGO_FIELD(liveObj, uint32_t, 0x3ec) & 1))) {
                                        int type = LEGO_FIELD(liveObj, int, 0);
                                        if (type == 1 || type == 4 || type == 2 ||
                                            (type == 0xb &&
                                             (LEGO_FIELD(liveObj, uint32_t, 0x3e4) & 0x2000000))) {
                                            media_action_feedback_sound_dispatch(0);
                                            if (!keyDownShift)
                                                cmd_produce(3, NULL, 0, NULL);
                                            if (!keyDownShift ||
                                                !selection_contains_entity_active_target(liveObj, NULL)) {
                                                cmd_produce(1, liveObj, 0, NULL);
                                                cmd_produce(0x2d, NULL, 0, NULL);
                                            }
                                        } else if (type == 6 || type == 7) {
                                            struct EntitySlot* unit = selection_active_entity_get();
                                            if (unit != NULL) {
                                                entity_position_to_terrain_cell_adapter(
                                                    liveObj, &objPos[0], &objPos[1]);
                                                if (lego_terrain_cell_route_and_occupancy_check(
                                                        unit, objPos[0], objPos[1],
                                                        objPos[0], objPos[1], 0)) {
                                                    cmd_produce(8, NULL, 0, objPos);
                                                    entity_deferred_work_schedule(liveObj);
                                                    cmd_mark_matching_adapter(3, liveObj, NULL);
                                                    media_action_feedback_sound_dispatch(0);
                                                    selection_clear_request_and_hud_reset();
                                                }
                                            }
                                        } else {
                                            media_action_feedback_sound_dispatch(1);
                                        }
                                    }
                                } else {
                                    struct EntitySlot* unit = selection_active_entity_get();
                                    if (unit != NULL &&
                                        (LEGO_FIELD(liveObj, int, 0) == 6 ||
                                         LEGO_FIELD(liveObj, int, 0) == 7)) {
                                        entity_position_to_terrain_cell_adapter(
                                            liveObj, &objPos[0], &objPos[1]);
                                        if (lego_terrain_cell_route_and_occupancy_check(
                                                unit, objPos[0], objPos[1], objPos[0],
                                                objPos[1], 0)) {
                                            if (LEGO_FIELD(liveObj, int, 0) == 7)
                                                media_pointer_cursor_select(0x1c);
                                            else if (LEGO_FIELD(unit, int, 0) == 2)
                                                media_pointer_cursor_select(0x23);
                                            else if (LEGO_FIELD(unit, int, 0) == 1)
                                                media_pointer_cursor_select(0x24);
                                        }
                                    }
                                }
                            }
                        } else {
                            int type;
                            if (s_global_low_word & 8) media_pointer_cursor_select(0x21);
                            else media_pointer_cursor_select(0x22);
                            type = LEGO_FIELD(liveObj, int, 0);
                            if (type == 7 || type == 6 ||
                                (type == 0xb &&
                                 !(LEGO_FIELD(liveObj, uint32_t, 0x3e4) & 0x2000000)) ||
                                (type == 8 &&
                                 !(LEGO_FIELD(liveObj, uint32_t, 0x3e8) & 0x10000))) {
                                if (lego_terrain_cell_route_and_occupancy_check(
                                        selection_active_entity_get(), bx, by, bx, by, 0)) {
                                    if (LEGO_FIELD(liveObj, int, 0) == 7)
                                        media_pointer_cursor_select(0x1c);
                                    else
                                        if (s_global_low_word & 8) media_pointer_cursor_select(0x23);
                                        else media_pointer_cursor_select(0x24);
                                    if (leftReleased) {
                                        cmd_produce(8, NULL, 0, legoGlobs_f_556fb8);
                                        entity_deferred_work_schedule(liveObj);
                                        cmd_mark_matching_adapter(3, liveObj, NULL);
                                        media_action_feedback_sound_dispatch(0);
                                        selection_clear_request_and_hud_reset();
                                    }
                                }
                            }
                        }
                    } else {
                        if (s_global_low_word & 0x20) media_pointer_cursor_select(0x1d);
                        else media_pointer_cursor_select(0x1e);
                        advisor_text_animation_update(2, 0, 0);
                    }
                } else if (legoGlobs_flags1 & 0x10000000) {
                    /* No object: the block under the pointer. */
                    if (lego_C_block_is_wall(bx, by))
                        media_action_code_to_sound_alias(block->f_02);
                } else {
                    if (leftReleased) block->f_10[6]++;
                    if ((s_global_low_word & 8) || (s_global_low_word & 0x10)) {
                        if (s_global_low_word & 8) media_pointer_cursor_select(0x21);
                        else media_pointer_cursor_select(0x22);
                    } else if ((s_global_low_word & 0x20) || (s_global_low_word & 0x40)) {
                        if (s_global_low_word & 0x20) media_pointer_cursor_select(0x1d);
                        else media_pointer_cursor_select(0x1e);
                        advisor_text_animation_update(2, 0, 0);
                        if (legoGlobs_flags1 & 0x40000) {
                            if (block->f_00 == 1 ||
                                (selection_active_has_any_get() &&
                                 !selected_entities_any_predicate_match(bx, by, 1))) {
                                legoGlobs_f_556188 = 0;
                            } else {
                                legoGlobs_f_556184 = by;
                                legoGlobs_f_556180 = bx;
                                legoGlobs_f_556188 = 1;
                                if (leftReleased &&
                                    !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                    terrain_cell_command_create_adapter(legoGlobs_f_556fb8, 1, 1);
                                    media_action_feedback_sound_dispatch(0);
                                    selection_clear_request_and_hud_reset();
                                }
                            }
                        } else if (!(block->flags1 & 8) && block->f_02 != 1 &&
                                   block->f_02 != 9 && block->f_02 != 0xb &&
                                   !(block->flags1 & 0x40) &&
                                   (!selection_active_has_any_get() ||
                                    selected_entities_any_predicate_match(bx, by, 0))) {
                            if (s_global_low_word & 0x20) media_pointer_cursor_select(0x1f);
                            else media_pointer_cursor_select(0x20);
                            advisor_text_animation_update(1, 0, 0);
                            if (leftReleased &&
                                !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                terrain_cell_command_create_adapter(legoGlobs_f_556fb8, 0, 1);
                                media_action_feedback_sound_dispatch(0);
                                selection_clear_request_and_hud_reset();
                            }
                        }
                    } else if ((s_global_low_word & 2) || (s_global_low_word & 4)) {
                        if (s_global_low_word & 2) media_pointer_cursor_select(0x25);
                        else media_pointer_cursor_select(0x26);
                        if ((block->flags1 & 8) && selection_active_has_any_get() &&
                            lego_terrain_cell_route_and_occupancy_check(
                                selection_active_entity_get(), bx, by, bx, by, 0)) {
                            if (s_global_low_word & 2) media_pointer_cursor_select(0x27);
                            else media_pointer_cursor_select(0x28);
                            advisor_text_animation_update(0, 0, 0);
                            if (leftReleased &&
                                !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                cmd_produce(8, NULL, 0, legoGlobs_f_556fb8);
                                media_action_feedback_sound_dispatch(0);
                                selection_clear_request_and_hud_reset();
                            }
                        }
                    } else if (s_global_low_word & 1) {
                        legoGlobs_flags1 |= 0x200;
                        media_pointer_cursor_select(0xf);
                        advisor_text_animation_update(8, 0, 0);
                        if (leftReleased) media_action_feedback_sound_dispatch(1);
                    } else if (lego_C_block_flag4(bx, by)) {
                        if (lego_C_block_is_wall(bx, by) &&
                            (lego_C_block_at(legoGlobs_f_556fb8[0], legoGlobs_f_556fb8[1])->f_02 == 1 ||
                             lego_C_block_at(legoGlobs_f_556fb8[0], legoGlobs_f_556fb8[1])->f_02 == 0xb)) {
                            if (!(block->flags1 & 8))
                                media_action_code_to_sound_alias(block->f_02);
                            if (!s_lego_dragging)
                                lego_terrain_command_feedback_dispatch(legoGlobs_f_556fb8, 1, 1, 0);
                            if (leftReleased) media_action_feedback_sound_dispatch(2);
                        } else {
                            handled = 0;
                            if (!(block->flags1 & 8))
                                media_action_code_to_sound_alias(block->f_02);
                            if (!s_lego_dragging)
                                lego_terrain_command_feedback_dispatch(legoGlobs_f_556fb8, 1, 1, 0);
                            if (selection_active_has_any_get()) {
                                if (legoGlobs_flags1 & 0x40000) {
                                    if (block->f_00 == 1 ||
                                        !selected_entities_any_predicate_match(bx, by, 1)) {
                                        legoGlobs_f_556188 = 0;
                                    } else {
                                        legoGlobs_f_556180 = bx;
                                        legoGlobs_f_556184 = by;
                                        handled = 1;
                                        legoGlobs_f_556188 = 1;
                                        if (leftReleased &&
                                            !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                            terrain_cell_command_create_adapter(legoGlobs_f_556fb8, 1, 1);
                                            media_action_feedback_sound_dispatch(0);
                                            selection_clear_request_and_hud_reset();
                                        }
                                    }
                                } else if (!(block->flags1 & 8) &&
                                           selected_entities_any_predicate_match(bx, by, 0)) {
                                    if (LEGO_FIELD(selection_active_entity_get(), int, 0) == 2) media_pointer_cursor_select(0x1d);
                                    else media_pointer_cursor_select(0x1e);
                                    advisor_text_animation_update(2, 0, 0);
                                    if (block->f_02 != 1 && block->f_02 != 9 &&
                                        block->f_02 != 0xb && !(block->flags1 & 0x40)) {
                                        handled = 1;
                                        if (LEGO_FIELD(selection_active_entity_get(), int, 0) == 2) media_pointer_cursor_select(0x1f);
                                        else media_pointer_cursor_select(0x20);
                                        advisor_text_animation_update(1, 0, 0);
                                        if (leftReleased &&
                                            !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                            terrain_cell_command_create_adapter(legoGlobs_f_556fb8, 0, 1);
                                            cmd_mark_matching_adapter(8, NULL, legoGlobs_f_556fb8);
                                            media_action_feedback_sound_dispatch(0);
                                            selection_clear_request_and_hud_reset();
                                        }
                                    }
                                }
                                if (!handled && (block->flags1 & 8)) {
                                    if (!lego_C_block_is_reinforced(bx, by) &&
                                        !(block->flags2 & 0x20)) {
                                        if (!(block->flags1 & 0x200000) &&
                                            (entity_stats_word_get(selection_active_entity_get()) & 0x10000) &&
                                            lego_terrain_cell_route_and_occupancy_check(
                                                selection_active_entity_get(), bx, by, bx, by, 0)) {
                                            handled = 1;
                                            if (LEGO_FIELD(selection_active_entity_get(), int, 0) == 2) media_pointer_cursor_select(0x29);
                                            else media_pointer_cursor_select(0x2a);
                                            if (leftReleased &&
                                                !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                                cmd_produce(8, NULL, 0, legoGlobs_f_556fb8);
                                                cmd_mark_matching_adapter(0xc, NULL, legoGlobs_f_556fb8);
                                                media_action_feedback_sound_dispatch(0);
                                                selection_clear_request_and_hud_reset();
                                            }
                                        }
                                        if (handled) goto tracking;
                                        if (lego_terrain_cell_route_and_occupancy_check(
                                                selection_active_entity_get(), bx, by, bx, by, 0)) {
                                            handled = 1;
                                            if (LEGO_FIELD(selection_active_entity_get(), int, 0) == 2) media_pointer_cursor_select(0x27);
                                            else media_pointer_cursor_select(0x28);
                                            advisor_text_animation_update(0, 0, 0);
                                            if (leftReleased &&
                                                !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                                cmd_produce(8, NULL, 0, legoGlobs_f_556fb8);
                                                media_action_feedback_sound_dispatch(0);
                                                selection_clear_request_and_hud_reset();
                                            }
                                        }
                                    }
                                }
                            }
                            if (!handled) {
                                if (!lego_C_block_is_reinforced(bx, by) &&
                                    block->f_02 != 6 && block->f_02 != 9 &&
                                    !(block->flags2 & 0x20)) {
                                    if (block->flags1 & 8) {
                                        if (lego_C_block_at(legoGlobs_f_556fb8[0], legoGlobs_f_556fb8[1])->flags1 & 0x80000000) {
                                            handled = 1;
                                            if (leftReleased &&
                                                !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                                cmd_produce(3, NULL, 0, NULL);
                                                media_action_feedback_sound_dispatch(5);
                                                command_context_post_to_hud(2, (const uint32_t*)legoGlobs_f_556fb8);
                                            }
                                        }
                                        if (!handled && (block->flags1 & 0x8000)) {
                                            handled = 1;
                                            if (leftReleased) media_action_feedback_sound_dispatch(1);
                                        }
                                        if (((block->flags1 & 0x100000) ||
                                             (lego_C_block_at(legoGlobs_f_556fb8[0], legoGlobs_f_556fb8[1])->flags1 & 0x40000000)) &&
                                            !(lego_C_block_at(legoGlobs_f_556fb8[0], legoGlobs_f_556fb8[1])->flags1 & 0x8000) &&
                                            !lego_C_block_flag20000000(legoGlobs_f_556fb8[0], legoGlobs_f_556fb8[1])) {
                                            handled = 1;
                                            if (leftReleased &&
                                                !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                                media_action_feedback_sound_dispatch(5);
                                                command_context_post_to_hud(4, (const uint32_t*)legoGlobs_f_556fb8);
                                            }
                                        }
                                        if (!handled) {
                                            if ((lego_C_block_at(legoGlobs_f_556fb8[0], legoGlobs_f_556fb8[1])->flags1 & 0x40000000) ||
                                                lego_C_block_flag20000000(legoGlobs_f_556fb8[0], legoGlobs_f_556fb8[1])) {
                                                handled = 1;
                                                if (leftReleased &&
                                                    !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                                    cmd_produce(3, NULL, 0, NULL);
                                                    media_action_feedback_sound_dispatch(5);
                                                    command_context_post_to_hud(1, (const uint32_t*)legoGlobs_f_556fb8);
                                                }
                                            }
                                            if (!handled) {
                                                if (block->flags1 & 0x200000) {
                                                    handled = 1;
                                                    if (leftReleased &&
                                                        !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                                        cmd_produce(3, NULL, 0, NULL);
                                                        media_action_feedback_sound_dispatch(5);
                                                        command_context_post_to_hud(1, (const uint32_t*)legoGlobs_f_556fb8);
                                                    }
                                                }
                                                if (!handled && !(block->flags1 & 0x200000)) {
                                                    handled = 1;
                                                    if (leftReleased &&
                                                        !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                                        cmd_produce(3, NULL, 0, NULL);
                                                        media_action_feedback_sound_dispatch(5);
                                                        command_context_post_to_hud(5, (const uint32_t*)legoGlobs_f_556fb8);
                                                    }
                                                }
                                            }
                                        }
                                    } else if (block->f_02 != 1 && !(block->flags1 & 0x40)) {
                                        handled = 1;
                                        if (leftReleased &&
                                            !lego_terrain_cell_command_predicate(legoGlobs_f_556fb8)) {
                                            cmd_produce(3, NULL, 0, NULL);
                                            media_action_feedback_sound_dispatch(4);
                                            command_context_post_to_hud(7, (const uint32_t*)legoGlobs_f_556fb8);
                                        }
                                    }
                                }
                                if (!handled && leftReleased)
                                    media_action_feedback_sound_dispatch(1);
                            }
                        }
                    } else if (leftReleased) {
                        media_action_feedback_sound_dispatch(1);
                    }
                }
            }

        tracking:
            if (!(s_global_low_word & 0x80) && !(legoGlobs_flags1 & 0x10000000) &&
                !(system_state_flags_get_original(NULL) & 0x200)) {
                if (!legoInput_mslb) {
                    if (legoGlobs_flags1 & 0x100) {
                        LegoPoint2FC mousePt;
                        mousePt.x = (float)legoInput_mouseX;
                        mousePt.y = (float)legoInput_mouseY;
                        if (!(system_state_flags_get_original(NULL) & 8)) {
                            if (!keyDownShift) cmd_produce(3, NULL, 0, NULL);
                            camera_project_point_to_screen(
                                (const struct ViewProjectionAdapter*)legoGlobs_viewMain,
                                &screenPt.x, legoGlobs_f_556164);
                            entity_clipped_rectangle_callback_dispatch(
                                legoGlobs_viewMain, &screenPt.x, &mousePt.x);
                            media_action_feedback_sound_dispatch(0);
                        }
                    } else {
                        legoGlobs_f_556164[1] = wPos.y;
                        legoGlobs_f_556164[2] = wPos.z;
                        legoGlobs_f_556164[0] = wPos.x;
                    }
                    legoGlobs_f_556170[0] = (float)bx;
                    legoGlobs_f_556170[1] = (float)by;
                }
                if (s_lego_dragging && !(s_panel_highlight_flags & 0x2000) &&
                    !(legoGlobs_flags1 & 0x200)) {
                    if (!(legoGlobs_flags1 & 0x100) &&
                        sqrt(s_lego_drag_x * s_lego_drag_x +
                             s_lego_drag_y * s_lego_drag_y) > 10.0f)
                        legoGlobs_flags1 |= 0x100;
                    legoGlobs_f_556178[0] = (float)bx;
                    legoGlobs_f_556178[1] = (float)by;
                }
            }
            if (s_panel_highlight_flags & 0x10)
                lego_level_debug_input_cell_mutator_dispatch(bx, by, liveObj,
                                                             s_lego_hold_time);
        }
    } else {
        done = 0;
        if (legoGlobs_NextButtonImage != NULL && (s_panel_highlight_flags & 0x40)) {
            if ((float)legoInput_mouseX > legoGlobs_NextButtonPos[0] &&
                (float)legoInput_mouseY > legoGlobs_NextButtonPos[1] &&
                (float)legoInput_mouseX < legoGlobs_NextButtonPos[0] + (float)LEGO_FIELD(legoGlobs_NextButtonImage, unsigned int, 4) &&
                (float)legoInput_mouseY < legoGlobs_NextButtonPos[1] + (float)LEGO_FIELD(legoGlobs_NextButtonImage, unsigned int, 8)) {
                timed_effect_update_request_set_corpus(0x20);
                if (leftReleased) {
                    tutorial_advisor_scroll_step_advance();
                    done = 1;
                }
            }
        }
        if (legoGlobs_BackButtonImage != NULL && (s_panel_highlight_flags & 0x80)) {
            if ((float)legoInput_mouseX > legoGlobs_BackButtonPos[0] &&
                (float)legoInput_mouseY > legoGlobs_BackButtonPos[1] &&
                (float)legoInput_mouseX < legoGlobs_BackButtonPos[0] + (float)LEGO_FIELD(legoGlobs_BackButtonImage, unsigned int, 4) &&
                (float)legoInput_mouseY < legoGlobs_BackButtonPos[1] + (float)LEGO_FIELD(legoGlobs_BackButtonImage, unsigned int, 8)) {
                timed_effect_update_request_set_corpus(0x1f);
                if (leftReleased) {
                    objective_deferred_callback_invoke_if_set();
                    done = 1;
                }
            }
        }
        if (leftReleased && !done) media_action_feedback_sound_dispatch(1);
    }

end:
    if (!placed && (s_global_low_word & 0x80) &&
        !building_placement_update_adapter(legoGlobs_f_556440, legoGlobs_f_556444,
                                           leftReleased, rightReleased, NULL, bx,
                                           by, 0, s_refresh_record))
        s_global_low_word &= ~0x80;
    if (!s_lego_dragging) legoGlobs_flags1 &= ~0x300;
    s_lego_prev_left_button = legoInput_mslb;
    s_lego_prev_right_button = legoInput_msrb;
}

/* ToolTipInfoStringsLoad [group G] */
// FUNCTION: LEGORR 0x00427e00
void lego_tooltip_info_strings_load(struct BlockRec* config,
                                    const char* gameName)
{
    legoGlobs_healthText = block_get_str_copy(config,
        string_path_join(s_engine_config_root,
            // STRING: LEGORR 0x004a419c
            "ToolTipInfo",
            // STRING: LEGORR 0x004a41a8
            "HealthText", NULL));
    legoGlobs_energyText = block_get_str_copy(config,
        string_path_join(s_engine_config_root, "ToolTipInfo",
            // STRING: LEGORR 0x004a4190
            "EnergyText", NULL));
    legoGlobs_crystalsText = block_get_str_copy(config,
        string_path_join(s_engine_config_root, "ToolTipInfo",
            // STRING: LEGORR 0x004a4180
            "CrystalsText", NULL));
    legoGlobs_oreText = block_get_str_copy(config,
        string_path_join(s_engine_config_root, "ToolTipInfo",
            // STRING: LEGORR 0x004a4178
            "OreText", NULL));
    legoGlobs_studsText = block_get_str_copy(config,
        string_path_join(s_engine_config_root, "ToolTipInfo",
            // STRING: LEGORR 0x004a416c
            "StudsText", NULL));
    legoGlobs_toolsText = block_get_str_copy(config,
        string_path_join(s_engine_config_root, "ToolTipInfo",
            // STRING: LEGORR 0x004a4160
            "ToolsText", NULL));
    legoGlobs_carryObjectText = block_get_str_copy(config,
        string_path_join(s_engine_config_root, "ToolTipInfo",
            // STRING: LEGORR 0x004a4150
            "CarryObjectText", NULL));
    legoGlobs_drivenByText = block_get_str_copy(config,
        string_path_join(s_engine_config_root, "ToolTipInfo",
            // STRING: LEGORR 0x004a4140
            "DrivenByText", NULL));
    legoGlobs_oreRequiredText = block_get_str_copy(config,
        string_path_join(s_engine_config_root, "ToolTipInfo",
            // STRING: LEGORR 0x004a4130
            "OreRequiredText", NULL));
}

/* LevelUpgradeNameStringsLoad [group G] */
// FUNCTION: LEGORR 0x00427f80
void lego_level_upgrade_name_strings_load(struct BlockRec* config)
{
    const char* prefix = "Level";
    struct BlockRec* item;
    unsigned int level;

    for (item = block_subgroup(config, string_path_join(s_engine_config_root,
             // STRING: LEGORR 0x004a41b4
             "UpgradeNames", NULL));
         item != NULL; item = block_next_equal_key(item)) {
        if (strnicmp(prefix, LEGO_FIELD(item, char*, 0x04),
                      strlen(prefix)) == 0) {
            level = atoi(LEGO_FIELD(item, char*, 0x04) + strlen(prefix));
            if (level < 16) {
                legoGlobs_upgradeNames[level] =
                    string_duplicate(LEGO_FIELD(item, char*, 0x08));
            }
        }
    }
}

/* EntityCommandStatusFeedbackDispatch [group F] */
// FUNCTION: LEGORR 0x00428020
void lego_entity_command_status_feedback_dispatch(struct EntitySlot* liveObj)
{
    char buffer[256];
    char buff2[128];
    const char* name;
    unsigned int i;
    unsigned int bit;
    void** abilityIcon;
    void* icon;

    if (entity_display_name_lookup(liveObj) != NULL && strlen(entity_display_name_lookup(liveObj)) != 0) {
        if (s_lego_toolTipObject != liveObj) {
            s_lego_toolTipObject = liveObj;
            return;
        }
        if (LEGO_FIELD(liveObj, char*, 0x8) != NULL && *LEGO_FIELD(liveObj, char*, 0x8) == '\0')
            entity_custom_name_set_adapter(liveObj, NULL);
        if (LEGO_FIELD(liveObj, char*, 0x8) != NULL) strcpy(buffer, LEGO_FIELD(liveObj, char*, 0x8));
        else strcpy(buffer, entity_display_name_lookup(liveObj));

        if (LEGO_FIELD(liveObj, int, 0x0) == 4 &&
            legoGlobs_upgradeNames[LEGO_FIELD(liveObj, int, 0x330)] != NULL) {
            sprintf(buff2,
                    // STRING: LEGORR 0x004a41d4
                    " (%s)",
                    legoGlobs_upgradeNames[LEGO_FIELD(liveObj, int, 0x330)]);
            strcat(buffer, buff2);
        }
        if ((LEGO_FIELD(liveObj, unsigned int, 0x3e8) & 0x40000) &&
            entity_quad_mesh_render_record_allowed(liveObj) &&
            LEGO_FIELD(liveObj, int, 0x0) != 2 && LEGO_FIELD(liveObj, int, 0x0) != 3) {
            sprintf(buff2,
                    // STRING: LEGORR 0x004a41cc
                    "\n%s: %i",
                    legoGlobs_energyText,
                    LEGO_FIELD(liveObj, float, 0x340) < 0.0f ? 0 : (int)LEGO_FIELD(liveObj, float, 0x340));
            strcat(buffer, buff2);
        }
        if (LEGO_FIELD(liveObj, int, 0x0) == 1 && LEGO_FIELD(liveObj, void*, 0x36c) != NULL) {
            name = LEGO_FIELD(LEGO_FIELD(liveObj, struct EntitySlot*, 0x36c), char*, 0x8);
            if (name != NULL && *name != '\0' && legoGlobs_drivenByText != NULL) {
                sprintf(buff2,
                        // STRING: LEGORR 0x004a41c4
                        "\n%s: %s",
                        legoGlobs_drivenByText, name);
                strcat(buffer, buff2);
            }
        }

        tooltip_set_content(2, buffer);
        timed_effect_cue_request_set_corpus(2, terrain_object_type_sfx_at_cell_get(liveObj));

        if (LEGO_FIELD(liveObj, int, 0x0) == 2) {
            for (i = 0; i < LEGO_FIELD(liveObj, unsigned int, 0x384); i++) {
                icon = s_object_toolTipIcons[(&LEGO_FIELD(liveObj, unsigned int, 0x370))[i]];
                if (icon != NULL) command_sample_stats_accumulate(2, icon);
            }
            for (; i < (unsigned int)ai_activity_cap(liveObj); i++) {
                if (s_object_toolTipIconBlank != NULL)
                    command_sample_stats_accumulate(2, s_object_toolTipIconBlank);
            }
            if (LEGO_FIELD(liveObj, unsigned int, 0x3f0)) {
                command_sample_stats_accumulate(2, NULL);
                for (bit = 0, abilityIcon = s_object_toolTipIconAbilities;
                     abilityIcon < s_object_toolTipIconAbilities + 6; abilityIcon++, bit++) {
                    if (LEGO_FIELD(liveObj, unsigned int, 0x3f0) & (1 << bit)) {
                        if (*abilityIcon != NULL) command_sample_stats_accumulate(2, *abilityIcon);
                    } else if (s_object_toolTipIconBlank != NULL) {
                        command_sample_stats_accumulate(2, s_object_toolTipIconBlank);
                    }
                }
            }
        }
        timed_effect_update_request_set_corpus(2);
    }
}

/* TerrainCommandFeedbackDispatch [group F] */
// FUNCTION: LEGORR 0x00428330
void lego_terrain_command_feedback_dispatch(const int* blockPos, int showConstruction,
                                            int playSound, int showCavern)
{
    char buff2[128];
    char buffer[128];
    int by, bx;
    Lego_Level* level;
    Lego_Block* block;
    unsigned int flags;
    struct EntitySlot* site;
    unsigned int usesStuds;
    int crystals, crystalsCost, ore, oreCost, studs, studsCost;
    int show;
    const char* text;
    unsigned int sfx;
    unsigned int valid;

    level = legoGlobs_currLevel;
    by = blockPos[1];
    bx = blockPos[0];
    block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx];
    if (s_lego_toolTipBlockX != bx || s_lego_toolTipBlockY != by) {
        s_lego_toolTipBlockX = blockPos[0];
        s_lego_toolTipBlockY = blockPos[1];
        return;
    }

    flags = block->flags1;
    if ((flags & 0x100000) &&
        !(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * by + bx].flags1 & 0x400) &&
        !(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * by + bx].flags2 & 4) &&
        !(flags & 0x8000)) {
        site = LEGO_FIELD(block, struct EntitySlot*, 0x10);
        usesStuds = LEGO_FIELD(site, unsigned int, 0xd0) & 8;
        strcpy(buffer, localized_object_name_get(4, LEGO_FIELD(site, int, 0x0)));
        if (showConstruction) {
            crystals = count_matching_coordinates_in_list(site, 6, 0);
            crystalsCost = entity_indexed_record_word_at_b4_get(4, LEGO_FIELD(site, int, 0x0), 0);
            ore = count_matching_coordinates_in_list(site, 7, 0);
            oreCost = entity_indexed_record_word_at_b0_get(4, LEGO_FIELD(site, int, 0x0), 0);
            studs = count_matching_coordinates_in_list(site, 7, 1);
            studsCost = stats_table_word_at_b8(4, LEGO_FIELD(site, int, 0x0), 0);
            if (crystalsCost) {
                sprintf(buff2,
                        // STRING: LEGORR 0x004a41dc
                        "\n%s: %i/%i",
                        legoGlobs_crystalsText, crystals, crystalsCost);
                strcat(buffer, buff2);
            }
            if (usesStuds) {
                if (studsCost) {
                    sprintf(buff2,
                            "\n%s: %i/%i",
                            legoGlobs_studsText, studs, studsCost);
                    strcat(buffer, buff2);
                }
            } else if (oreCost) {
                sprintf(buff2,
                        "\n%s: %i/%i",
                        legoGlobs_oreText, ore, oreCost);
                strcat(buffer, buff2);
            }
        }
        tooltip_set_content(3, buffer);
        timed_effect_cue_request_set_corpus(3, 0);
        timed_effect_update_request_set_corpus(3);
        return;
    }

    show = 0;
    if (flags & 8) {
        if (flags & 0x80000000) {
            text = lego_terrain_surface_type_name_get(0);
            sfx = lego_surface_type_configured_value_at_index(0);
            show = 1;
        } else if (!(flags & 0x200000)) {
            text = lego_terrain_surface_type_name_get(0xc);
            sfx = lego_surface_type_configured_value_at_index(0xc);
            show = 1;
        } else if (block->flags2 & 0x20) {
            text = lego_terrain_surface_type_name_get(0xf);
            sfx = lego_surface_type_configured_value_at_index(0xf);
            show = 1;
        } else {
            if (bx >= 0 && by >= 0 && bx < (int)legoGlobs_currLevel->width && by < (int)legoGlobs_currLevel->height)
                valid = LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * by + bx].flags1 & 0x20000000;
            else
                valid = 0;
            if (valid || (flags & 0x8000)) {
                text = lego_terrain_surface_type_name_get(0xe);
                sfx = lego_surface_type_configured_value_at_index(0xe);
                show = 1;
            } else if (!(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * by + bx].flags1 & 0x400) &&
                       !(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * by + bx].flags2 & 4) &&
                       !(flags & 0x100000)) {
                if (showCavern) {
                    text = lego_terrain_surface_type_name_get(0x11);
                    sfx = lego_surface_type_configured_value_at_index(0x11);
                } else {
                    text = lego_terrain_surface_type_name_get(0);
                    sfx = lego_surface_type_configured_value_at_index(0);
                }
                show = 1;
            }
        }
        if (block->f_02 == 9) {
            text = lego_terrain_surface_type_name_get(block->f_02);
            sfx = lego_surface_type_configured_value_at_index(block->f_02);
            show = 1;
        } else if (block->f_02 == 6) {
            text = lego_terrain_surface_type_name_get(block->f_02);
            sfx = lego_surface_type_configured_value_at_index(block->f_02);
            show = 1;
        }
    } else {
        if (!(flags & 0x10) && (flags & 0x20000)) {
            text = lego_terrain_surface_type_name_get(0x10);
            sfx = lego_surface_type_configured_value_at_index(0x10);
        } else if (flags & 0x20) {
            text = lego_terrain_surface_type_name_get(0xd);
            sfx = lego_surface_type_configured_value_at_index(0xd);
        } else {
            text = lego_terrain_surface_type_name_get(block->f_02);
            sfx = lego_surface_type_configured_value_at_index(block->f_02);
        }
        show = 1;
    }

    if (show) {
        tooltip_set_content(6, "%s", text);
        if (!playSound) sfx = 0;
        timed_effect_cue_request_set_corpus(6, sfx);
        timed_effect_update_request_set_corpus(6);
    }
}

/* TerrainCellCommandPredicate [group F] */
// FUNCTION: LEGORR 0x00428780
int lego_terrain_cell_command_predicate(const int* blockPos)
{
    if (system_state_flags_get_original(0) && !(system_state_flags_get_original(0) & 8) &&
        !(system_state_flags_get_original(0) & 4))
        return 1;
    if (system_state_flags_get_original(0) & 2) {
        if (LEGO_LEVEL_PTR(Lego_Block, ((Lego_Level*)level_current_owner_get())->blocks)
                [((Lego_Level*)level_current_owner_get())->width * blockPos[1] + blockPos[0]].f_04 == 0)
            return 1;
    }
    return 0;
}

/* ActionFeedbackSoundDispatch [group M] */
// FUNCTION: LEGORR 0x00428800
void media_action_feedback_sound_dispatch(int event)
{
    int saved = sound_batch_mode_get();
    sound_batch_queue_drain(0, 0);
    switch (event) {
    case 0:
        media_pointer_cursor_override_set(0x14, 12.5f);
        sound_alias_play(0x13, 0);
        break;
    case 1:
        media_pointer_cursor_override_set(0x15, 12.5f);
        sound_alias_play(0x14, 0);
        break;
    case 2:
        media_pointer_cursor_override_set(0x15, 12.5f);
        sound_alias_play(0x1f, 0);
        break;
    case 3:
        media_pointer_cursor_override_set(0x14, 12.5f);
        sound_alias_play(0x1e, 0);
        break;
    case 4:
        media_pointer_cursor_override_set(0x14, 12.5f);
        sound_alias_play(0x20, 0);
        break;
    case 5:
        media_pointer_cursor_override_set(0x14, 12.5f);
        sound_alias_play(0x21, 0);
        break;
    }
    sound_batch_queue_drain(saved, 0);
}

/* LevelDebugInputCellMutatorDispatch [group F] */
// FUNCTION: LEGORR 0x004288e0
void lego_level_debug_input_cell_mutator_dispatch(int bx, int by, struct EntitySlot* liveObj,
                                                   float elapsed)
{
    int blockPos[2];
    struct EntitySlot* unit;

    blockPos[0] = bx;
    blockPos[1] = by;

    if (LEGO_KEY_PRESSED(0x1e)) {
        lava_erosion_neighbor_mutate(blockPos, 1, 1);
        if (liveObj != NULL) entity_enter_event21_state_once(liveObj);
    }
    if (LEGO_KEY_PRESSED(0xcf) && selection_active_has_any_get())
        entity_record_flag400_toggle_and_refresh(selection_active_entity_get());
    if (LEGO_KEY_PRESSED(0x12))
        creature_spawn_from_emergence(&legoGlobs_rockMonsterData[legoGlobs_currLevel->f_0b0], 3,
                                      legoGlobs_currLevel->f_0b0, bx, by);
    if (s_global_current_keys[0x11])
        lego_terrain_neighbor_flag_set(legoGlobs_currLevel, 0, bx, by);
    if (s_global_current_keys[0x2e]) {
        if (LEGO_KEY_PRESSED(0x2e) && selection_active_has_any_get())
            entity_clear_nested_state_8_fields(selection_active_entity_get());
        if (LEGO_KEY_PRESSED(0x2e) && selection_active_has_any_get())
            entity_clear_nested_state_8_fields(selection_active_entity_get());
    }
    if (LEGO_KEY_PRESSED(0x58)) {
        if (legoGlobs_flags1 & 0x1000000) {
            legoGlobs_flags1 &= ~0x1000000;
        } else {
            uint32_t tutorialFlags = 0;
            legoGlobs_flags1 |= 0x1000000;
            system_state_flags_set_target(&tutorialFlags);
        }
    }
    if (LEGO_KEY_PRESSED(0x57))
        global_mode_zero_input_set(0);
    if (LEGO_KEY_PRESSED(0x44)) {
        s_lego_debugLightToggle = !s_lego_debugLightToggle;
        if (s_lego_debugLightToggle) {
            ddiscene_object_position_set(legoGlobs_spotlightTop, LEGO_FIELD(s_zoom_step_context, void*, 0x2c),
                                         250.0f, 190.0f, 20.0f);
            ddiscene_object_pose_method_dispatch(legoGlobs_spotlightTop, LEGO_FIELD(s_zoom_step_context, void*, 0x2c),
                                                 -1.0f, -0.8f, 0.0f, 0.0f, 1.0f, 0.0f);
        } else {
            ddiscene_object_position_set(legoGlobs_spotlightTop, LEGO_FIELD(s_zoom_step_context, void*, 0x2c),
                                         200.0f, 140.0f, -130.0f);
            ddiscene_object_pose_method_dispatch(legoGlobs_spotlightTop, LEGO_FIELD(s_zoom_step_context, void*, 0x2c),
                                                 -1.0f, -0.8f, 0.75f, 0.0f, 1.0f, 0.0f);
        }
        simtick_scene_position_reset_and_effect_motion_cancel();
    }
    if (LEGO_KEY_PRESSED(0x43)) {
        s_lego_debugTransformToggle = !s_lego_debugTransformToggle;
        scene_transform_enable_set(s_lego_debugTransformToggle);
    }

    if (selection_active_has_any_get() &&
        (LEGO_KEY_PRESSED(0x06) || LEGO_KEY_PRESSED(0x07) || LEGO_KEY_PRESSED(0x08) ||
         LEGO_KEY_PRESSED(0x09))) {
        int count;
        struct EntitySlot* const* unitList = selection_active_context_record_get(&count);
        if (count != 0) {
            unsigned int* upgrades;
            unsigned int oldFlags, newFlags;
            int canCarry, canScan, canSpeed, canDrill;
            if (LEGO_FIELD(unitList[0], int, 0x0) == 4) {
                upgrades = (unsigned int*)((char*)LEGO_FIELD(unitList[0], void*, 0x18) + 0x8c);
                canCarry = task_flag_bit_query(LEGO_FIELD(selection_active_entity_get(), void*, 0x18), 0, 0);
                canScan = task_flag_bit_query(LEGO_FIELD(selection_active_entity_get(), void*, 0x18), 1, 0);
                canSpeed = task_flag_bit_query(LEGO_FIELD(selection_active_entity_get(), void*, 0x18), 2, 0);
                canDrill = task_flag_bit_query(LEGO_FIELD(selection_active_entity_get(), void*, 0x18), 3, 0);
            } else if (LEGO_FIELD(unitList[0], int, 0x0) == 1) {
                upgrades = (unsigned int*)((char*)LEGO_FIELD(unitList[0], void*, 0xc) + 0x124);
                canCarry = record_flag_bit_test(LEGO_FIELD(selection_active_entity_get(), void*, 0xc), 0, 0);
                canScan = record_flag_bit_test(LEGO_FIELD(selection_active_entity_get(), void*, 0xc), 1, 0);
                canSpeed = record_flag_bit_test(LEGO_FIELD(selection_active_entity_get(), void*, 0xc), 2, 0);
                canDrill = record_flag_bit_test(LEGO_FIELD(selection_active_entity_get(), void*, 0xc), 3, 0);
            } else {
                upgrades = NULL;
            }
            if (upgrades != NULL) {
                oldFlags = upgrades[2];
                newFlags = oldFlags;
                if (LEGO_KEY_PRESSED(0x06) && canCarry) newFlags ^= 8;
                if (LEGO_KEY_PRESSED(0x07) && canScan) newFlags ^= 4;
                if (LEGO_KEY_PRESSED(0x08) && canSpeed) newFlags ^= 2;
                if (LEGO_KEY_PRESSED(0x09) && canDrill) newFlags ^= 1;
                if (newFlags != oldFlags) {
                    if (LEGO_FIELD(unitList[0], int, 0x0) == 4)
                        entity_payload_state_set(LEGO_FIELD(selection_active_entity_get(), struct EntitySlot*, 0x18),
                                                 newFlags);
                    else if (LEGO_FIELD(unitList[0], int, 0x0) == 1)
                        scene_record_indexed_entries_rebuild_original(
                            LEGO_FIELD(selection_active_entity_get(), void*, 0xc), newFlags);
                }
            }
        }
    }

    if (LEGO_KEY_PRESSED(0x0e))
        entity_command14_batch_issue(5);
    if (LEGO_KEY_PRESSED(0x53))
        lego_terrain_cell_mutate(legoGlobs_currLevel, bx, by, 0);
    if (s_global_current_keys[0x51])
        lego_terrain_cell_initialize(legoGlobs_currLevel, bx, by);

    if (LEGO_KEY_PRESSED(0x11) && selection_active_has_any_get()) {
        unit = selection_active_entity_get();
        if (LEGO_FIELD(unit, int, 0x0) == 3) {
            Lego_Level* level = legoGlobs_currLevel;
            int x, y;
            struct EntitySlot* boulder;
            nearest_open_cell_search_adapter(unit, &x, &y, 1, 0, 0);
            boulder = object_create_from_container_adapter(legoGlobs_contBoulder, 5, 0);
            entity_type_operation_dispatch_adapter(boulder, 1);
            entity_try_create_linked_at_position_original(unit, x, y, boulder);
            object_list_field_assign_all(boulder,
                (void*)(uintptr_t)packed_grid_cell_value_get(LEGO_LEVEL_PTR(void, level->textureSet),
                    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * y + x].texture));
        }
    }
    if (LEGO_KEY_PRESSED(0x31) && selection_active_has_any_get())
        entity_action_sequence_reset(selection_active_entity_get(), 0);
    if (s_global_current_keys[0x2a] && LEGO_KEY_PRESSED(0x1e))
        cell_command_type13_issue_original(blockPos);
    if (LEGO_KEY_PRESSED(0x30)) {
        float direction[2];
        direction[0] = 2.0f;
        direction[1] = 1.0f;
        if (liveObj != NULL) entity_pair_scalar_fields_set_original(liveObj, direction, 40.0f);
    }
    if (LEGO_KEY_PRESSED(0x23))
        spiderweb_spawn_at(bx, by);
    if (LEGO_KEY_PRESSED(0x21) && selection_active_has_any_get()) {
        unsigned int count = selection_active_count_get();
        unsigned int i;
        struct EntitySlot* const* units;
        if (count > 0) {
            units = selection_active_records_get();
            for (i = 0; i < count; i++)
                entity_damage_apply_original(units[i], 40.0f, 1, elapsed);
        }
    }
    if (LEGO_KEY_PRESSED(0x23) && selection_active_has_any_get()) {
        unit = selection_active_entity_get();
        if (LEGO_FIELD(unit, int, 0x0) == 4) electric_fence_spark_from((struct ElectricFenceObject*)unit, 0, 0);
    }
    if (LEGO_KEY_PRESSED(0x24)) {
        if (electric_fence_block_is_fence(bx, by)) electric_fence_debug_remove(bx, by);
        else electric_fence_debug_place(bx, by);
    }
    if (LEGO_KEY_PRESSED(0x15)) {
        int param[2];
        param[0] = 1;
        param[1] = 1;
        message_dispatch(0, NULL, 0, param);
    }
    if (s_global_current_keys[0x2a]) {
        if (LEGO_KEY_PRESSED(0x16)) panel_advisor_animation_play(0, 0, 1);
    } else if (LEGO_KEY_PRESSED(0x16)) {
        advisor_clear_active_state();
    }
    if (LEGO_KEY_PRESSED(0x25) && selection_active_has_any_get()) {
        if (LEGO_FIELD(selection_active_entity_get(), int, 0x0) == 1) {
            s_lego_debugRetargetObject = selection_active_entity_get();
            return;
        }
        if (LEGO_FIELD(selection_active_entity_get(), int, 0x0) == 2 && s_lego_debugRetargetObject != NULL)
            level_debug_cell_retarget_adapter(selection_active_entity_get(), s_lego_debugRetargetObject);
    }
}

/* The block-visibility callback of the top view's radius pass: switch off
 * the transient effect of a block that has one. */
// FUNCTION: LEGORR 0x00429110
static void lego_block_visible_callback(int bx, int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    uint32_t flags1 = LEGO_LEVEL_PTR(Lego_Block, level->blocks)
        [by * level->width + bx].flags1;

    if (flags1 & 0x4000) {
        transient_effect_enable_set((void*)(uintptr_t)
            LEGO_LEVEL_PTR(Lego_Block, level->blocks)
                [by * legoGlobs_currLevel->width + bx].f_10[3], 0);
    }
}

/* TerrainCellPaletteResourceAtGridPosition [group M] */
// FUNCTION: LEGORR 0x00429160
#if defined(_MSC_VER) && defined(_M_IX86)
/* The C body below compiles to the same operations except one: the index
 * multiply loads xy[1] into eax and multiplies by the level's width, where
 * the original loads the width and multiplies by xy[1], with the global and
 * parameter loads scheduled the other way round (82.35%). No source form
 * moved it: every operand order of width * y + x, signedness casts, a local
 * level pointer, a Point2I parameter declared before and after Lego_Level,
 * the permuter (no candidates) and a counter sweep over a full 512 period
 * (one code at every N, so not rule B; finding 538). As for vec_cross
 * (finding 550), the matching build keeps the port's instruction-for-
 * instruction body. */
__declspec(naked) void* lego_terrain_cell_palette_resource_at_grid_position(
    const int* xy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov edx, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [ecx + 7ch]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        mov al, byte ptr [edx + eax*8 + 1]
        push eax
        push ecx
        call packed_grid_cell_value_get
        add esp, 8
        pop esi
        ret
    }
}
#else
void* lego_terrain_cell_palette_resource_at_grid_position(const int* xy)
{
    int idx = xy[0] + xy[1] * legoGlobs_currLevel->width;
    return (void*)(uintptr_t)packed_grid_cell_value_get(
        LEGO_LEVEL_PTR(void, legoGlobs_currLevel->textureSet),
        LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[idx].texture);
}
#endif


/* SceneMarkerUpdateAndDraw [group G] */
// FUNCTION: LEGORR 0x004291a0
void lego_scene_marker_update_and_draw(float elapsed, int track)
{
    LegoVector3FG pos, dir;
    int bx, by;
    float dist;
    unsigned int count;

    if (!track) {
        if (s_context_mode == 1) {
            ddiscene_object_position_get(
                LEGO_FIELD(s_zoom_step_context, void*, 0x28), NULL, &pos.x);
            terrain_map_world_to_block(
                LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                pos.x, pos.y, &bx, &by);
            ddiscene_object_facing_pair_get(
                LEGO_FIELD(s_zoom_step_context, void*, 0x24), NULL, &dir.x,
                NULL);
            lego_G_vector3d_scale(&dir, &dir, terrain_context_scale_get(
                level_current_terrain_get_original()) * 3.0f);
            lego_G_vector3d_add(&pos, &dir, &pos);
            terrain_map_world_to_block(
                LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                pos.x, pos.y, &bx, &by);
            terrain_map_visible_radius_add(
                LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                bx, by, 8, lego_block_visible_callback);
        } else if (s_context_mode == 0) {
            dist = LEGO_FIELD(LEGO_LEVEL_PTR(void, legoGlobs_currLevel->map),
                              float, 0x10) * legoGlobs_FPClipBlocks;
            count = legoGlobs_HPBlocks;
            if ((legoGlobs_flags1 & 0x80000) && elapsed != 0.0f) {
                count = (unsigned int)(count / elapsed);
            }
            terrain_projected_cell_state_refresh_original(
                LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24),
                legoGlobs_viewMain, dist, count);
        }
    } else if ((legoGlobs_flags1 & 2) && (legoGlobs_flags1 & 0x2000)) {
        ddiscene_object_position_get(
            LEGO_FIELD(legoGlobs_cameraTrack, void*, 0x28), NULL, &pos.x);
        terrain_map_world_to_block(
            LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
            pos.x, pos.y, &bx, &by);
        terrain_map_visible_radius_add(
            LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
            bx, by, 2, NULL);
    }
}

/* SelectionRectangleRender [group G] */
// FUNCTION: LEGORR 0x004293b0
void lego_selection_rectangle_render(Lego_Level* level)
{
    LegoPoint2FG start;
    LegoPoint2FG mouse;
    LegoPoint2FG points[5];

    if (legoGlobs_flags1 & 0x100) {
        mouse.x = (float)(int)s_renderer_word_a;
        mouse.y = (float)(int)s_renderer_word_b;
        camera_project_point_to_screen(
            (const struct ViewProjectionAdapter*)legoGlobs_viewMain,
            &start.x, legoGlobs_f_556164);
        points[0].x = start.x;
        points[0].y = start.y;
        points[1].x = mouse.x;
        points[1].y = start.y;
        points[2].x = mouse.x;
        points[2].y = mouse.y;
        points[3].x = start.x;
        points[3].y = mouse.y;
        points[4].x = start.x;
        points[4].y = start.y;
        surface_draw_polyline((struct Point2F*)&points[0],
                              (struct Point2F*)&points[1], 4,
                              legoGlobs_dragBoxRGB[0],
                              legoGlobs_dragBoxRGB[1],
                              legoGlobs_dragBoxRGB[2], 2);
    }
}

/* CameraPointerRayRoute [group G] */
// FUNCTION: LEGORR 0x00429470
void lego_camera_pointer_ray_route(unsigned int mouseX, unsigned int mouseY,
                                   float* xOut, float* yOut)
{
    LegoVector3FG rayDir;
    LegoVector3FG rayOrigin;
    LegoVector3FG planeNormal;
    LegoVector3FG planePoint;
    LegoVector3FG endPoint;
    float screen[4];

    planePoint.x = 0.0f;
    planePoint.y = 0.0f;
    planePoint.z = 0.0f;
    planeNormal.x = 0.0f;
    planeNormal.y = 0.0f;
    planeNormal.z = -1.0f;
    screen[0] = (float)mouseX;
    screen[1] = (float)mouseY;
    screen[2] = 0.0f;
    screen[3] = 1.0f;
    camera_screen_coordinate_project(
        (const struct ViewProjectionAdapter*)legoGlobs_viewMain,
        &rayOrigin.x, screen);
    screen[0] = (float)mouseX;
    screen[1] = (float)mouseY;
    screen[2] = 1.0f;
    screen[3] = 1.0f;
    camera_screen_coordinate_project(
        (const struct ViewProjectionAdapter*)legoGlobs_viewMain,
        &rayDir.x, screen);
    lego_G_vector3d_subtract(&rayDir, &rayDir, &rayOrigin);
    if (view_ray_intersection_test(&endPoint.x, &rayOrigin.x, &rayDir.x,
                                   &planePoint.x, &planeNormal.x)) {
        *xOut = endPoint.x;
        *yOut = endPoint.y;
    }
}

/* ContextSelectedGlobalPointerGet [group M] */
// FUNCTION: LEGORR 0x004295a0
uint32_t lego_context_selected_global_pointer_get(void)
{
    if (s_context_mode == 0) return legoGlobs_f_5560f8;
    return legoGlobs_f_556104;
}

/* EntityMatchesCurrentSelectionOrOwner [group M] */
// FUNCTION: LEGORR 0x004295c0
int lego_entity_matches_current_selection_or_owner(const void* object)
{
    if ((s_context_mode == 0 && object == legoGlobs_selectedObject) ||
        (LEGO_FIELD(object, void*, 0x36c) != NULL &&
         LEGO_FIELD(object, void*, 0x36c) == legoGlobs_selectedObject))
        return 1;
    return 0;
}

/* SoundMinimumDistanceUpdateMode [group G] */
// FUNCTION: LEGORR 0x004295f0
void lego_sound_minimum_distance_update_mode(int viewMode, void* liveObj,
                                             int cameraFrame)
{
    float field;

    if (viewMode == 0) {
        field = 0.9f;
        if (cameraFrame) field = 0.6f;
        record_fields_4_and_14_set(legoGlobs_cameraFP,
                                   (uint32_t)(uintptr_t)liveObj,
                                   cameraFrame);
        global_entity_render_parameter_mode_set(1);
        terrain_map_wobble_emissive_set(
            LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), 1);
        legoGlobs_selectedObject = liveObj;
        if (LEGO_FIELD(liveObj, int, 0x00) == 2) {
            entity_attached_object_list_detach((struct EntitySlot*)liveObj,
                                               0);
        }
        resource_interface_call_slot48(legoGlobs_viewMain,
            LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24));
        simtick_directsound_listener_view_set(
            *LEGO_FIELD(legoGlobs_cameraFP, LegoA_Sound3DFrameArg*, 0x24));
        sound_tracked_buffers_set_min_distance(50.0f);
        scene_current_object_set_original(legoGlobs_viewMain, field);
        renderer_object_set_clip_distance(legoGlobs_viewMain,
            legoGlobs_currLevel->blockSize * legoGlobs_FPClipBlocks);
    } else if (viewMode == 1) {
        if (legoGlobs_selectedObject != NULL) {
            lego_viewport_target_position_clamp((struct EntitySlot*)legoGlobs_selectedObject,
                                                NULL, 0);
        }
        LEGO_FIELD(legoGlobs_cameraFP, void*, 0x04) = NULL;
        legoGlobs_selectedObject = NULL;
        global_entity_render_parameter_mode_set(0);
        terrain_map_wobble_emissive_set(
            LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), 0);
        resource_interface_call_slot48(legoGlobs_viewMain,
            LEGO_FIELD(s_zoom_step_context, void*, 0x24));
        simtick_directsound_listener_view_set(
            *LEGO_FIELD(s_zoom_step_context, LegoA_Sound3DFrameArg*, 0x34));
        sound_tracked_buffers_set_min_distance(
            legoGlobs_MinDistFor3DSoundsOnTopView);
        /* 0.5f: the port's entry takes the float's bits. */
        resource_interface_call_slot40_and_clear_field14(
            (struct ResourceInterfaceWrapper*)legoGlobs_viewMain,
            0x3f000000);
        renderer_object_set_clip_distance(legoGlobs_viewMain,
                                          legoGlobs_TVClipDist);
    }
    s_context_mode = viewMode;
}

/* The CD track's end callback: play another track. */
// FUNCTION: LEGORR 0x004297a0
static void lego_cd_track_play_next_callback(void)
{
    media_cd_audio_stop_and_close_if_active();
    lego_cd_audio_watchdog_mode_update(1);
}

/* CDAudioWatchdogModeUpdate [group G] */
// FUNCTION: LEGORR 0x004297b0
void lego_cd_audio_watchdog_mode_update(int on)
{
    int track;

    if (on) {
        track = (rng_thunk_draw15() % legoGlobs_CDTracks) - 1 +
                legoGlobs_CDStartTrack;
        legoGlobs_flags1 |= 8;
        media_cd_audio_track_start(track, 0,
                                   lego_cd_track_play_next_callback);
    } else {
        legoGlobs_flags1 &= ~8;
        media_cd_audio_stop_and_close_if_active();
        media_cd_audio_watchdog_tick(0);
    }
}

/* AudioPlaybackToggle [group G] */
// FUNCTION: LEGORR 0x00429810
void lego_audio_playback_toggle(int on)
{
    if (on) legoGlobs_flags1 |= 0x10;
    else legoGlobs_flags1 &= ~0x10;
    sound_subsystem_enable_set_and_stop(on);
}

/* ActiveEngineFieldB0Get [group M] */
// FUNCTION: LEGORR 0x00429850
uint32_t active_engine_field_b0_get_current(void)
{
    return legoGlobs_currLevel->f_0b0;
}

/* EngineFloatField210GetByIndex [group M] */
// FUNCTION: LEGORR 0x00429860
float engine_float_field210_get_by_index_current(int index)
{
    return legoGlobs_currLevel->upgradeTimes[index];
}

/* EngineFloatField260Get [group M] */
// FUNCTION: LEGORR 0x00429880
float engine_float_field260_get_current_target(void)
{
    return legoGlobs_currLevel->trainTime;
}

/* LevelLoad [group D] */
/* Lego_LoadLevel: read the level's config block into a new Lego_Level, load
 * its maps, objectives and object list, and make it legoGlobs.currLevel.
 * The readers below are gods98 Config.h's macros; the value ones read the
 * value twice (once to test it). */
#define LEGO_D_ID(key) string_path_join(s_engine_config_root, levelName, key, 0)
#define LEGO_D_TEMP(key) block_get_strptr(config, LEGO_D_ID(key))
#define LEGO_D_STR(key) (LEGO_D_TEMP(key) ? LEGO_D_TEMP(key) : s_config_angle_empty)
#define LEGO_D_INT(key) atoi(LEGO_D_STR(key))
#define LEGO_D_FLOAT(key) (float)atof(LEGO_D_STR(key))
#define LEGO_D_BOOL(key) block_get_bool_path(config, LEGO_D_ID(key))
#define LEGO_D_COPY(key) block_get_str_copy(config, LEGO_D_ID(key))
// FUNCTION: LEGORR 0x00429890
int lego_level_load(const char* levelName)
{
    struct BlockRec* config = legoGlobs_config;
    Lego_Level* level;
    char* surfaceMap;
    char* predugMap;
    char* terrainMap;
    char* blockPointersMap;
    char* cryOreMap;
    char* erodeMap;
    char* emergeMap;
    char* aiMap;
    char* pathMap;
    char* fallinMap;
    const char* textureSet;
    const char* objectList;
    const char* ptlName;
    const char* nerpFile;
    const char* nerpMessageFile;
    const char* emergeCreature;
    const char* introAVI;
    const char* str;
    char* copy;
    char* fullName;
    char buff[260];
    char* parts[10];
    float upgradeTime;
    float value;
    float slugValue;
    int number;
    unsigned int i;
    unsigned int rockFallStyle;

    movie_play_level_video_adapter(levelName, 1);
    global_linked_record_chain_clear();
    options_apply_from_slot_original();
    mode_flag_pair_apply_if_bit2(0);
    level_state_stored_value_set(0);
    scatter_spawn_gate_set(0);
    copy = (char*)sav_build_ctx_target();
    render_config_six_field_copy_in_or_clear(copy ? (const uint32_t*)(copy + 0x1c) : NULL);
    if (!osf_read_dirty_flag_get_original() && (number = save_slot_get_target()) != -1) {
        sprintf(buff, "%s\\%i.osf", "Saves", number);
        osf_read_original(buff);
    }

    if ((level = (Lego_Level*)malloc(sizeof(Lego_Level))) != NULL) {
        memset(level, 0, sizeof(Lego_Level));
        legoGlobs_flags1 |= 0x4;
        g_active_level_retail = (struct LevelState*)level;

        string_table_entry_set_current(levelName);
        priorities_load(config, s_engine_config_root, levelName);

        // STRING: LEGORR 0x004a45bc
        if ((number = LEGO_D_INT("Seed")) != 0) srand(number);

        // STRING: LEGORR 0x004a45a8
        if ((str = LEGO_D_TEMP("BoulderAnimation")) != NULL) {
            // STRING: LEGORR 0x004a45a0
            if (!_strcmpi(str, "Rock")) level->f_274 = 0;
            // STRING: LEGORR 0x004a4598
            else if (!_strcmpi(str, "Lava")) level->f_274 = 1;
            // STRING: LEGORR 0x004a4594
            else if (!_strcmpi(str, "Ice")) level->f_274 = 2;
            else level->f_274 = 0;
        } else level->f_274 = 0;

        // STRING: LEGORR 0x004a4580
        if ((legoGlobs_f_556f14 = LEGO_D_INT("FallinMultiplier")) == 0) legoGlobs_f_556f14 = 1;

        // STRING: LEGORR 0x004a4574
        level->blockSize = LEGO_D_FLOAT("blocksize");
        // STRING: LEGORR 0x004a4568
        level->digDepth = LEGO_D_FLOAT("digdepth");
        // STRING: LEGORR 0x004a4560
        level->useRoof = LEGO_D_BOOL("useroof");
        // STRING: LEGORR 0x004a4554
        level->roofHeight = LEGO_D_FLOAT("roofheight");
        // STRING: LEGORR 0x004a4548
        level->roughLevel = LEGO_D_FLOAT("roughlevel");

        // STRING: LEGORR 0x004a453c
        surfaceMap = LEGO_D_COPY("surfacemap");
        // STRING: LEGORR 0x004a4530
        predugMap = LEGO_D_COPY("predugmap");
        // STRING: LEGORR 0x004a4524
        terrainMap = LEGO_D_COPY("terrainmap");
        // STRING: LEGORR 0x004a4510
        blockPointersMap = LEGO_D_COPY("blockPointersMap");
        // STRING: LEGORR 0x004a4504
        cryOreMap = LEGO_D_COPY("CryOreMap");
        // STRING: LEGORR 0x004a44f8
        erodeMap = LEGO_D_COPY("ErodeMap");
        // STRING: LEGORR 0x004a44ec
        emergeMap = LEGO_D_COPY("EmergeMap");
        // STRING: LEGORR 0x004a44e4
        aiMap = LEGO_D_COPY("AIMap");
        // STRING: LEGORR 0x004a44dc
        pathMap = LEGO_D_COPY("PathMap");
        // STRING: LEGORR 0x004a44d0
        fallinMap = LEGO_D_COPY("FallinMap");
        // STRING: LEGORR 0x004a44c4
        textureSet = LEGO_D_TEMP("textureset");

        // STRING: LEGORR 0x004a44b4
        if ((level->emergeTimeOut = LEGO_D_FLOAT("EmergeTimeOut")) == 0.0f) level->emergeTimeOut = 1500.0f;

        // STRING: LEGORR 0x004a44a8
        objectList = LEGO_D_TEMP("olistfile");
        // STRING: LEGORR 0x004a449c
        LEGO_D_TEMP("elistfile");
        // STRING: LEGORR 0x004a448c
        level->selBoxHeight = LEGO_D_FLOAT("selboxheight");
        // STRING: LEGORR 0x004a4484
        ptlName = LEGO_D_TEMP("PTLFile");
        // STRING: LEGORR 0x004a4478
        nerpFile = LEGO_D_TEMP("NERPFile");
        // STRING: LEGORR 0x004a4468
        nerpMessageFile = LEGO_D_TEMP("NERPMessageFile");
        // STRING: LEGORR 0x004a4458
        emergeCreature = LEGO_D_TEMP("EmergeCreature");
        // STRING: LEGORR 0x004a444c
        level->safeCaverns = LEGO_D_BOOL("SafeCaverns");
        // STRING: LEGORR 0x004a4440
        introAVI = LEGO_D_TEMP("IntroAVI");
        // STRING: LEGORR 0x004a4438
        level->startFP = (LEGO_D_BOOL("StartFP") == 1);
        // STRING: LEGORR 0x004a4430
        level->noDrain = (LEGO_D_BOOL("NoDrain") == 1);
        // STRING: LEGORR 0x004a4424
        level->oxygenRate = LEGO_D_FLOAT("OxygenRate") * 0.001f;

        upgradeTime = LEGO_D_FLOAT("UpgradeTime") * 25.0f;
        for (i = 0; i < 20; i++) level->upgradeTimes[i] = upgradeTime;
        // STRING: LEGORR 0x004a4400
        value = LEGO_D_FLOAT("MinifigureUpgradeTime");
        level->upgradeTimes[2] = value;
        if (0.0f == value) level->upgradeTimes[2] = upgradeTime;
        else level->upgradeTimes[2] = value * 25.0f;
        // STRING: LEGORR 0x004a43ec
        value = LEGO_D_FLOAT("VehicleUpgradeTime");
        level->upgradeTimes[1] = value;
        if (0.0f == value) level->upgradeTimes[1] = upgradeTime;
        else level->upgradeTimes[1] = value * 25.0f;
        // STRING: LEGORR 0x004a43d8
        value = LEGO_D_FLOAT("BuildingUpgradeTime");
        level->upgradeTimes[4] = value;
        if (0.0f == value) level->upgradeTimes[4] = upgradeTime;
        else level->upgradeTimes[4] = value * 25.0f;
        // STRING: LEGORR 0x004a43cc
        level->trainTime = LEGO_D_FLOAT("TrainTime") * 25.0f;

        // STRING: LEGORR 0x004a43bc
        if (LEGO_D_BOOL("NoMultiSelect") == 1) s_panel_highlight_flags |= 0x2000;
        // STRING: LEGORR 0x004a43b0
        if (LEGO_D_BOOL("NoAutoEat") == 1) s_panel_highlight_flags |= 0x200000;
        else s_panel_highlight_flags &= ~0x200000;

        // STRING: LEGORR 0x004a43a4
        legoGlobs_endGameAVI1 = LEGO_D_COPY("EndGameAVI1");
        // STRING: LEGORR 0x004a4398
        legoGlobs_endGameAVI2 = LEGO_D_COPY("EndGameAVI2");

        if (legoGlobs_RenameReplace) {
            // STRING: LEGORR 0x004a438c
            if (LEGO_D_BOOL("AllowRename") == 1) s_panel_highlight_flags |= 0x20000;
            else s_panel_highlight_flags &= ~0x20000;
        }
        // STRING: LEGORR 0x004a437c
        if (LEGO_D_BOOL("RecallOLObjects") == 1) s_panel_highlight_flags |= 0x40000;
        else s_panel_highlight_flags &= ~0x40000;
        // STRING: LEGORR 0x004a436c
        if (LEGO_D_BOOL("GenerateSpiders") == 1) s_panel_highlight_flags |= 0x80000;
        else s_panel_highlight_flags &= ~0x80000;
        // STRING: LEGORR 0x004a4358
        if (LEGO_D_BOOL("DisableToolTipSound") == 1) s_panel_highlight_flags |= 0x100000;
        else s_panel_highlight_flags &= ~0x100000;
        // STRING: LEGORR 0x004a4344
        if (LEGO_D_BOOL("DisableEndTeleport") == 1) global_value_004ffe48_set(0);
        else global_value_004ffe48_set(1);

        {
            char* dragCopy;
            char* argv[10];
            // STRING: LEGORR 0x004a4338
            if ((dragCopy = LEGO_D_COPY("DragBoxRGB")) != NULL) {
                if (sound_variant_split(dragCopy, argv, s_config_rgb_separator) == 3) {
                    for (i = 0; i < 3; i++) {
                        legoGlobs_dragBoxRGB[i] = (float)atoi(argv[i]) * (1.0f / 255.0f);
                    }
                }
                free(dragCopy);
            }
        }

        // STRING: LEGORR 0x004a4324
        level->buildingTolerance = LEGO_D_FLOAT("BuildingTolerance");
        // STRING: LEGORR 0x004a430c
        level->buildingMaxVariation = LEGO_D_FLOAT("BuildingMaxVariation");
        if (0.0f == level->buildingTolerance) level->buildingTolerance = 4.0f;
        if (0.0f == level->buildingMaxVariation) level->buildingMaxVariation = 6.0f;

        // STRING: LEGORR 0x004a4300
        level->f_278 = LEGO_D_INT("MaxStolen");

        // STRING: LEGORR 0x004a42f8
        if ((str = LEGO_D_TEMP("Slug")) != NULL) {
            int slugType;
            if (lego_entity_type_pair_lookup(str, &slugType, &level->f_26c, NULL)) {
                // STRING: LEGORR 0x004a42ec
                slugValue = LEGO_D_FLOAT("SlugTime");
                level->f_268 = slugValue;
                if (0.0f == slugValue) level->f_268 = 60.0f;
                level->f_268 *= 25.0f;
                // STRING: LEGORR 0x004a42dc
                slugValue = LEGO_D_FLOAT("InitialSlugTime");
                if (0.0f == slugValue) slugValue = 60.0f;
                g_camera_countdown = slugValue * 25.0f;
            }
        } else level->f_26c = 20;

        // STRING: LEGORR 0x004a42cc
        str = LEGO_D_TEMP("RockFallStyle");
        rock_fall_style_name_lookup(str, &rockFallStyle);
        rockfall_style_table_base_set(rockFallStyle);
        // STRING: LEGORR 0x004a42c0
        legoGlobs_f_556ec0 = (LEGO_D_BOOL("noFallins") != 1);

        mission_crystal_quota_setup(config, s_engine_config_root, levelName);
        {
            int emergeType;
            lego_entity_type_pair_lookup(emergeCreature, &emergeType, &level->f_0b0, NULL);
        }

        if (config_rgb_vector_read(legoGlobs_config,
                                   // STRING: LEGORR 0x004a42b0
                                   string_path_join(s_engine_config_root, levelName, "FogColourRGB", 0),
                                   &legoGlobs_fogColourRGB[0], &legoGlobs_fogColourRGB[1],
                                   &legoGlobs_fogColourRGB[2])) {
            if (config_rgb_vector_read(legoGlobs_config,
                                       // STRING: LEGORR 0x004a429c
                                       string_path_join(s_engine_config_root, levelName, "HighFogColourRGB", 0),
                                       &legoGlobs_highFogColourRGB[0], &legoGlobs_highFogColourRGB[1],
                                       &legoGlobs_highFogColourRGB[2])) {
                legoGlobs_flags1 |= 0x10000;
                legoGlobs_f_556424 = 1.0f;
                // STRING: LEGORR 0x004a4294
                if (0.0f == (slugValue = LEGO_D_FLOAT("FogRate"))) slugValue = 60.0f;
                legoGlobs_f_556ef8 = slugValue * 25.0f;
            } else legoGlobs_flags1 &= ~0x10000;
            renderer_fog_color_set_original(legoGlobs_fogColourRGB[0], legoGlobs_fogColourRGB[1],
                                            legoGlobs_fogColourRGB[2]);
            renderer_fog_mode_set_original(2);
            renderer_fog_parameters_set_original(0.0f, legoGlobs_currLevel->blockSize * legoGlobs_FPClipBlocks,
                                                 0.0032f);
            legoGlobs_flags1 |= 0x8000;
        } else legoGlobs_flags1 &= ~0x8000;

        // STRING: LEGORR 0x004a4288
        LEGO_LEVEL_PTR_SET(level->nextLevel, LEGO_D_COPY("nextlevel"));
        fullName = LEGO_D_COPY("FullName");
        LEGO_LEVEL_PTR_SET(level->fullName, fullName);
        if (fullName) {
            for (i = 0; i < strlen(LEGO_LEVEL_PTR(char, level->fullName)); i++) {
                LEGO_LEVEL_PTR(char, level->fullName)[i] =
                    (LEGO_LEVEL_PTR(char, level->fullName)[i] == '_') ? ' ' : (unsigned char)LEGO_LEVEL_PTR(char, level->fullName)[i];
            }
        } else LEGO_LEVEL_PTR_SET(level->fullName, NULL);

        objective_load_original(config, s_engine_config_root, levelName, level, s_display_w, s_display_h);

        // STRING: LEGORR 0x004a4274
        if (0.0f == (upgradeTime = LEGO_D_FLOAT("ErodeTriggerTime"))) upgradeTime = 10.0f;
        // STRING: LEGORR 0x004a4264
        if (0.0f == (slugValue = LEGO_D_FLOAT("ErodeErodeTime"))) slugValue = 5.0f;
        // STRING: LEGORR 0x004a4254
        if (0.0f == (value = LEGO_D_FLOAT("ErodeLockTime"))) value = 60.0f;
        erosion_timing_state_init(upgradeTime, slugValue, value);
        // STRING: LEGORR 0x004a4234
        number = LEGO_D_INT("NumberOfLandSlidesTillCaveIn");
        if (number == 0) global_config_value_set(3);
        else global_config_value_set(number);

        // STRING: LEGORR 0x004a421c
        level->disableStartTeleport = (LEGO_D_BOOL("DisableStartTeleport") != 1);
        LEGO_LEVEL_PTR_SET(level->name, (char*)levelName);
        level->oxygenLevel = level->f_204 = 100.0f;
        level->f_08c = 0;
        level->crystals = 0;
        level->f_090 = 0;
        level->f_0a4 = 0;
        level->f_094 = 0;
        level->f_0a8 = 0;
        level->status = 0;
        legoGlobs_f_55644c = 0;
        legoGlobs_f_556450 = 0;

        tutorial_script_file_load_target(nerpFile);
        objective_advisor_token_file_load(nerpMessageFile);

        if (level_identifier_remap_table_load_original(ptlName, s_engine_config_root)) {
            int surfaceParam, pathParam = 0, blockPointersParam = 0, cryOreParam = 0, terrainParam = 0, predugParam = 0, fallinParam;

            if (surfaceMap && sound_variant_split(surfaceMap, parts, s_config_rgb_separator) == 2) surfaceParam = atoi(parts[1]);
            if (predugMap && sound_variant_split(predugMap, parts, s_config_rgb_separator) == 2) predugParam = atoi(parts[1]);
            if (terrainMap && sound_variant_split(terrainMap, parts, s_config_rgb_separator) == 2) terrainParam = atoi(parts[1]);
            if (blockPointersMap && sound_variant_split(blockPointersMap, parts, s_config_rgb_separator) == 2)
                blockPointersParam = atoi(parts[1]);
            if (cryOreMap && sound_variant_split(cryOreMap, parts, s_config_rgb_separator) == 2) cryOreParam = atoi(parts[1]);
            if (pathMap && sound_variant_split(pathMap, parts, s_config_rgb_separator) == 2) pathParam = atoi(parts[1]);
            if (fallinMap && sound_variant_split(fallinMap, parts, s_config_rgb_separator) == 2) fallinParam = atoi(parts[1]);
            if (emergeMap && sound_variant_split(emergeMap, parts, s_config_rgb_separator) == 2) pathParam = atoi(parts[1]);

            if (lego_map_load_pipeline(level, surfaceMap, predugMap, predugParam, terrainMap, terrainParam,
                                           blockPointersMap, blockPointersParam, cryOreMap, cryOreParam,
                                           erodeMap, pathMap, pathParam, textureSet, emergeMap, aiMap,
                                           fallinMap)) {
                free(surfaceMap);
                free(predugMap);
                free(terrainMap);
                free(blockPointersMap);
                free(cryOreMap);
                free(erodeMap);
                free(emergeMap);
                free(aiMap);
                free(fallinMap);
                if (pathMap) free(pathMap);

                electric_fence_restart((struct ElectricFenceLevel*)level);
                spiderweb_restart(level);
                objective_announce_current(0);
                objective_tick_target(legoGlobs_textOnlyWindow, (struct LevelState*)legoGlobs_currLevel, 1.0f, 1.0f);

                if (lego_object_list_load_file(level, objectList)) {
                    LegoInitialResourcesD resources;
                    float pos[2];

                    // STRING: LEGORR 0x004a420c
                    resources.crystals = LEGO_D_INT("InitialCrystals");
                    // STRING: LEGORR 0x004a4200
                    resources.ore = LEGO_D_INT("InitialOre");
                    entity_pool_find_by_callback_filtered_current_original(lego_entity_effect_counters_apply_from_record,
                                                                  &resources);

                    string_table_entry_set_current(NULL);
                    pos[0] = 350.0f;
                    pos[1] = 70.0f;
                    lego_play_movie(introAVI, pos);

                    // STRING: LEGORR 0x004a41f4
                    level_no_gather_object_setup(LEGO_D_BOOL("NoGather") == 1);
                    incident_message_flag_set(1);

                    if (level->startFP && s_lego_f_4e8448) {
                        lego_sound_minimum_distance_update_mode(0, s_lego_f_4e8448, 1);
                        cmd_produce(1, s_lego_f_4e8448, 0, NULL);
                        command_context_post_to_hud(0xf, NULL);
                    } else lego_sound_minimum_distance_update_mode(1, NULL, 0);

                    tutorial_path_coordinate_index((struct LevelState*)level);

                    // STRING: LEGORR 0x004a41e8
                    if (LEGO_D_BOOL("Precreate") == 1) {
                        void* objects[10];
                        for (i = 0; i < 10; i++) objects[i] = lego_object_create_adapter(legoGlobs_miniFigureData, 2, NULL);
                        for (i = 0; i < 10; i++) entity_teardown((struct EntitySlot*)objects[i]);
                    }
                    return 1;
                }
            }
        }
        free(level);
    }

    free(surfaceMap);
    free(predugMap);
    free(terrainMap);
    free(blockPointersMap);
    free(cryOreMap);
    free(erodeMap);
    free(emergeMap);
    string_table_entry_set_current(NULL);
    g_active_level_retail = NULL;
    return 0;
}
#undef LEGO_D_ID
#undef LEGO_D_TEMP
#undef LEGO_D_STR
#undef LEGO_D_INT
#undef LEGO_D_FLOAT
#undef LEGO_D_BOOL
#undef LEGO_D_COPY

/* EntityEffectCountersApplyFromRecord [group D] */
// FUNCTION: LEGORR 0x0042b2f0
int lego_entity_effect_counters_apply_from_record(struct EntitySlot* object, void* data)
{
    if (entity_stats_word_get(object) & 0x8) {
        simulation_update_counter_add_original(object, ((LegoInitialResourcesD*)data)->crystals);
        simulation_mode_counter_add_original(object, ((LegoInitialResourcesD*)data)->ore);
    }
    return 0;
}

/* LoadTileMeshes [group D] */
// FUNCTION: LEGORR 0x0042b330
int lego_load_tile_meshes(Lego_Level* level, const char* baseName)
{
    char names[2][256];
    unsigned int width = level->f_074;
    unsigned int height = level->f_078;
    unsigned int x, y;

    if (LEGO_LEVEL_PTR_SET(level->f_080, (void**)malloc(sizeof(void*) * width * height))) {
        if (!(s_engine_feature_word & 0x800000)) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    // STRING: LEGORR 0x004a45d4
                    sprintf(names[0], "%s%0.1x%0.1xa", baseName, x, y);
                    // STRING: LEGORR 0x004a45c4
                    sprintf(names[1], "%s%0.1x%0.1xb", baseName, x, y);
                    LEGO_LEVEL_PTR(void*, level->f_080)[(y * width) + x] =
                        tile_model_entity_register_original(legoGlobs_rootCont, names[0], names[1], level->blockSize,
                                                            LEGO_LEVEL_PTR(void, level->textureSet));
                }
            }
        } else {
            memset(LEGO_LEVEL_PTR(void*, level->f_080), 0, sizeof(void*) * width * height);
        }
        return 1;
    }
    return 0;
}

/* RectangularResourceTableDestroy [group D] */
// FUNCTION: LEGORR 0x0042b480
void lego_rectangular_resource_table_destroy(Lego_Level* level)
{
    unsigned int width = level->f_074;
    unsigned int height = level->f_078;
    unsigned int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            cell_visual_channel_table_dispose_original(LEGO_LEVEL_PTR(void*, level->f_080)[(y * width) + x]);
        }
    }
    free(LEGO_LEVEL_PTR(void, level->f_080));
    LEGO_LEVEL_PTR_SET(level->f_080, NULL);
}

/* GlobalStateField84Get [group M] */
// FUNCTION: LEGORR 0x0042b500
uint32_t global_state_field84_get_current_target(void)
{
    return legoGlobs_currLevel->f_084;
}

/* ------------------------------------------------------------------ */
/* Part 2 (issue #574): 0042b510-00432490.                             */
/* ------------------------------------------------------------------ */

/* ---- part 2 functions, group 2A (0042b510-0042c440): begin ---- */

/* Lego_Level.blocks[y * width + x]. */
#define LEGO_2A_BLOCK(level, x, y) \
    (LEGO_LEVEL_PTR(Lego_Block, (level)->blocks)[((y) * (level)->width) + (x)])
/* Lego_Level.emergeMap (+0x08) as its records. */
#define LEGO_2A_TRIGGERS(level) \
    ((LegoEmergeTrigger2A*)LEGO_LEVEL_PTR(void, (level)->emergeMap))

/* MapLoadPipeline [group 2A]: Lego_LoadMapSet. Builds the level's map and
 * blocks from the level's map files, in the order the original reads them.
 * Every failure returns the value it tested (a null map or block array, or
 * a loader's 0): the original jumps straight to the epilogue with that 0 in
 * eax, which only a result variable assigned from each test reproduces. */
// FUNCTION: LEGORR 0x0042b510
int lego_map_load_pipeline(Lego_Level* level, const char* surfaceMap,
                           const char* predugMap, int predugParam,
                           const char* terrainMap, int terrainParam,
                           const char* blockPointersMap, int blockPointersParam,
                           const char* cryOreMap, int cryOreParam,
                           const char* erodeMap, const char* pathMap,
                           int pathParam, const char* textureSet,
                           const char* emergeMap, const char* aiMap,
                           const char* fallinMap)
{
    unsigned int x, y;

    LEGO_LEVEL_PTR_SET(level->map, terrain_map_create(legoGlobs_rootCont, surfaceMap, level->blockSize,
                                                      level->roughLevel));
    if (LEGO_LEVEL_PTR(void, level->map)) {
        level->f_084 = (uint32_t)(uintptr_t)ui_render_record_create(
            LEGO_LEVEL_PTR(void, level->map), (const uint32_t*)legoGlobs_f_5563e4,
            LEGO_FIELD(&s_lego_radar_zoom, uint32_t, 0));
        terrain_map_perspective_correction_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), 1);
        terrain_map_texture_no_fade_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), 0x70);
        terrain_map_texture_no_fade_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), 0x70);
        terrain_map_texture_no_fade_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), 0x70);
        terrain_map_texture_no_fade_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), 0x70);
        terrain_map_texture_no_fade_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), 0x70);
        terrain_map_texture_no_fade_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), 0x70);

        level->width = LEGO_FIELD(LEGO_LEVEL_PTR(void, level->map), uint32_t, 0x8);
        level->height = LEGO_FIELD(LEGO_LEVEL_PTR(void, level->map), uint32_t, 0xc);

        electric_fence_restart((struct ElectricFenceLevel*)level);
        spiderweb_restart(level);

        if (level->useRoof) {
            indexed_surface_resource_allocate(legoGlobs_rootCont, level->width, level->height);
            graphics_overlay_visibility_set(1);
        }

        if (lego_texture_set_config_load(level, textureSet)) {
            LEGO_LEVEL_PTR_SET(level->blocks, (Lego_Block*)malloc(sizeof(Lego_Block) * level->width * level->height));
            if (LEGO_LEVEL_PTR(void, level->blocks)) {
                memset(LEGO_LEVEL_PTR(Lego_Block, level->blocks), 0, sizeof(Lego_Block) * level->width * level->height);
                for (y = 0; y < level->height; y++) {
                    for (x = 0; x < level->width; x++) {
                        LEGO_2A_BLOCK(level, x, y).f_00 = 0;
                        LEGO_2A_BLOCK(level, x, y).texture = 0;
                        LEGO_2A_BLOCK(level, x, y).f_02 = 0;
                        LEGO_2A_BLOCK(level, x, y).flags1 = 0;
                        LEGO_2A_BLOCK(level, x, y).f_03 = 0;
                        LEGO_2A_BLOCK(level, x, y).f_10[2] = 0;
                        LEGO_2A_BLOCK(level, x, y).f_10[1] = 0;
                        LEGO_FIELD(&LEGO_2A_BLOCK(level, x, y), short, 0x20) = rng_thunk_draw15();
                    }
                }

                if (lego_load_predug_layer(level, predugMap, predugParam)) {
                    if (lego_load_erode_cryore(level, terrainMap, terrainParam)) {
                        lego_load_path_ai(level, cryOreMap, (char)cryOreParam);
                        lego_load_emerge_layer(level, erodeMap);
                        lego_load_fallin(level, pathMap, pathParam);
                        lego_load_layer_l(level, emergeMap);
                        lego_load_layer_m(level, aiMap);
                        lego_load_block_pointers(level, fallinMap);
                        if (lego_load_terrain_map(level, blockPointersMap, blockPointersParam)) {
                            LEGO_FIELD(level, int, 0x110) = 1;
                        }

                        for (y = 0; y < level->height; y++) {
                            for (x = 0; x < level->width; x++) {
                                lego_cell_init_pass(level, x, y, 1);
                            }
                        }

                        terrain_map_vertex_normals_update_all(LEGO_LEVEL_PTR(struct TerrainMap, level->map));
                        terrain_cell_quad_run_stage_level(legoGlobs_rootCont, (struct LevelState*)level);
                        return 1;
                    }
                }
            }
        }
    }
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
    /* The original falls off the end on failure, with the failed test's
     * zero in eax; the port needs a return. */
    return 0;
#endif
}

/* SurfaceTextCharacterMapInitialize [group 2A]: Lego_InitTextureMappings.
 * Registers the surface texture pairs the map fades between. */
// FUNCTION: LEGORR 0x0042b850
void lego_surface_text_character_map_initialize(struct TerrainMap* map)
{
    unsigned char noFade[] = { 0x70, 0x70, 0x70, 0x70, 0x70, 0x70 };
    unsigned char floors[] = { 0x00, 0x00, 0x00, 0x00, 0x46, 0x47, 0x45, 0x00,
                               0x76, 0x66, 0x10, 0x11, 0x12, 0x13, 0x06, 0x16,
                               0x26, 0x36, 0x07, 0x61, 0x60, 0x62, 0x63, 0x64,
                               0x65, 0x71, 0x72, 0x73, 0x74, 0x75, 0x30 };
    unsigned char walls[] = { 0x05, 0x04, 0x03, 0x02, 0x01, 0x40, 0x20, 0x67,
                              0x05, 0x04, 0x03, 0x02, 0x01, 0x40, 0x20, 0x67 };
    unsigned char list1[] = { 0x25, 0x24, 0x23, 0x22, 0x21 };
    unsigned char list2[] = { 0x35, 0x34, 0x33, 0x32, 0x31 };
    unsigned char list3[] = { 0x35, 0x34, 0x33, 0x32, 0x31 };
    unsigned char list4[] = { 0x55, 0x54, 0x53, 0x52, 0x51 };
    unsigned char list5[] = { 0x55, 0x54, 0x53, 0x52, 0x51 };
    unsigned char list6[] = { 0x77, 0x77, 0x77, 0x77, 0x77 };
    unsigned int i, j;

    for (i = 0; i < sizeof(floors); i++) {
        for (j = 0; j < sizeof(walls); j++) terrain_map_fade_pair_add(map, floors[i], walls[j]);
        for (j = 0; j < sizeof(list1); j++) terrain_map_fade_pair_add(map, floors[i], list1[j]);
        for (j = 0; j < sizeof(list2); j++) terrain_map_fade_pair_add(map, floors[i], list2[j]);
        for (j = 0; j < sizeof(list3); j++) terrain_map_fade_pair_add(map, floors[i], list3[j]);
        for (j = 0; j < sizeof(list4); j++) terrain_map_fade_pair_add(map, floors[i], list4[j]);
        for (j = 0; j < sizeof(list5); j++) terrain_map_fade_pair_add(map, floors[i], list5[j]);
        for (j = 0; j < sizeof(list6); j++) terrain_map_fade_pair_add(map, floors[i], list6[j]);
    }
    for (i = 0; i < sizeof(noFade); i++) {
        for (j = 0; j < sizeof(walls); j++) terrain_map_fade_pair_add(map, noFade[i], walls[j]);
        for (j = 0; j < sizeof(list1); j++) terrain_map_fade_pair_add(map, noFade[i], list1[j]);
        for (j = 0; j < sizeof(list2); j++) terrain_map_fade_pair_add(map, noFade[i], list2[j]);
        for (j = 0; j < sizeof(list3); j++) terrain_map_fade_pair_add(map, noFade[i], list3[j]);
        for (j = 0; j < sizeof(list4); j++) terrain_map_fade_pair_add(map, noFade[i], list4[j]);
        for (j = 0; j < sizeof(list5); j++) terrain_map_fade_pair_add(map, noFade[i], list5[j]);
        for (j = 0; j < sizeof(list6); j++) terrain_map_fade_pair_add(map, noFade[i], list6[j]);
    }
}

/* TextureSetConfigLoad [group 2A]: Lego_LoadTextureSet. Reads the level's
 * texture set block: the surface texture grid, its meshes and textures and
 * the roof texture. */
// FUNCTION: LEGORR 0x0042bb60
int lego_texture_set_config_load(Lego_Level* level, const char* textureSet)
{
    struct BlockRec* config = legoGlobs_config;
    int ok = 0;
    unsigned int width, height;
    const char* meshBaseName;
    const char* textureBaseName;
    const char* roofTexture;

    // STRING: LEGORR 0x004a4620
    if ((width = LEGO_A_CFG_INT(config, string_path_join(s_engine_config_root, textureSet, "surftextwidth", 0)))) {
        // STRING: LEGORR 0x004a4610
        if ((height = LEGO_A_CFG_INT(config, string_path_join(s_engine_config_root, textureSet, "surftextheight", 0)))) {
            level->f_074 = width;
            level->f_078 = height;
            lego_surface_text_character_map_initialize(LEGO_LEVEL_PTR(struct TerrainMap, level->map));
            terrain_map_vertex_normals_update_all(LEGO_LEVEL_PTR(struct TerrainMap, level->map));

            // STRING: LEGORR 0x004a4600
            if ((meshBaseName = block_get_strptr(config, string_path_join(s_engine_config_root, textureSet, "meshbasename", 0)))) {
                // STRING: LEGORR 0x004a45f0
                if ((textureBaseName = block_get_strptr(config, string_path_join(s_engine_config_root, textureSet, "texturebasename", 0)))) {
                    if (LEGO_LEVEL_PTR_SET(level->textureSet, (void*)terrain_cell_resource_grid_load(textureBaseName, level->f_074, level->f_078))) {
                        terrain_map_texture_set_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map),
                                                    LEGO_LEVEL_PTR(void, level->textureSet));
                        if (lego_load_tile_meshes(level, meshBaseName)) {
                            // STRING: LEGORR 0x004a45e4
                            if ((roofTexture = block_get_strptr(config, string_path_join(s_engine_config_root, textureSet, "rooftexture", 0)))) {
                                if (animated_texture_replace(roofTexture)) ok = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return ok;
}

/* LoadPredugLayer [group 2A]: Lego_LoadPreDugMap. Opens the level's
 * pre-dug caverns (1, 3 visible; 2, 4 hidden; 3 and 4 also slug holes). */
// FUNCTION: LEGORR 0x0042bd20
int lego_load_predug_layer(Lego_Level* level, const char* filename, int modifier)
{
    uint32_t handle;
    uint32_t width, height;
    unsigned int x, y;
    int value;
    LegoPoint2I2A pos;

    if (filename && (handle = map_open2(filename, NULL)) != (uint32_t)-1) {
        level_map_dimensions_get(handle, &width, &height);
        if (width == level->width && height == level->height) {
            for (y = 1; y < height - 1; y++) {
                for (x = 1; x < width - 1; x++) {
                    value = level_map_cell_get(handle, x, y) - modifier;
                    if (value == 1 || value == 3) {
                        lego_terrain_cell_mutate(level, x, y, 1);
                        {
                            LegoPoint2I2A offsets[9] = { { -1, -1 }, { 0, -1 }, { 1, -1 },
                                                         { 1, 0 }, { 1, 1 }, { 0, 1 },
                                                         { -1, 1 }, { -1, 0 } };
                            unsigned int i;
                            for (i = 0; i < 9; i++) {
                                LEGO_2A_BLOCK(level, x + offsets[i].x, y + offsets[i].y).flags1 |= 0x4000000;
                            }
                        }
                    } else if (value == 2 || value == 4) {
                        LEGO_2A_BLOCK(level, x, y).flags1 |= 0x20000;
                    }
                    if (value == 3) {
                        pos.x = x;
                        pos.y = y;
                        LEGO_2A_BLOCK(level, x, y).flags2 |= 0x20;
                        predug_coordinate_pool_append(&pos.x);
                    }
                    if (value == 4) {
                        LEGO_2A_BLOCK(level, x, y).flags2 |= 0x200;
                    }
                }
            }
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* LoadEmergeLayer [group 2A]: Lego_LoadErodeMap. An even nonzero value
 * starts the block eroding; every value sets the block's erode speed. */
// FUNCTION: LEGORR 0x0042bf40
int lego_load_emerge_layer(Lego_Level* level, const char* filename)
{
    uint32_t handle;
    uint32_t width, height;
    unsigned int x, y;
    uint32_t value;
    int erode;
    LegoPoint2I2A pos;

    if (filename && (handle = map_open2(filename, NULL)) != (uint32_t)-1) {
        level_map_dimensions_get(handle, &width, &height);
        if (width == level->width && height == level->height) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    pos.x = x;
                    pos.y = y;
                    value = level_map_cell_get(handle, x, y);
                    erode = 0;
                    if (value == 0 || (value % 2) == 1) value++;
                    else erode = 1;
                    LEGO_2A_BLOCK(level, x, y).f_06 = (unsigned char)(value >> 1);
                    if (erode) lava_erosion_stage_register(&pos.x, rng_thunk_draw15() % 4);
                }
            }
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* LoadLayerM [group 2A]: Lego_LoadAIMap. Each block's AI node byte. */
// FUNCTION: LEGORR 0x0042c060
int lego_load_layer_m(Lego_Level* level, const char* filename)
{
    uint32_t handle;
    uint32_t width, height;
    unsigned int x, y;

    if (filename && (handle = map_open2(filename, NULL)) != (uint32_t)-1) {
        level_map_dimensions_get(handle, &width, &height);
        if (width == level->width && height == level->height) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    LEGO_FIELD(&LEGO_2A_BLOCK(level, x, y), unsigned char, 0x34) =
                        (unsigned char)level_map_cell_get(handle, x, y);
                }
            }
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* LoadLayerL [group 2A]: Lego_LoadEmergeMap. Value 6n + 1 is record n's
 * trigger block, 6n + 2..6n + 6 its emerge points. */
// FUNCTION: LEGORR 0x0042c120
int lego_load_layer_l(Lego_Level* level, const char* filename)
{
    uint32_t handle;
    uint32_t width, height;
    unsigned int x, y;
    uint32_t value, index, slot;

    if (filename) {
        handle = map_open2(filename, NULL);
        if (handle != (uint32_t)-1 &&
            (level_map_dimensions_get(handle, &width, &height), width == level->width) &&
            height == level->height) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if ((value = level_map_cell_get(handle, x, y))) {
                        value--;
                        slot = value % 6;
                        index = value / 6;
                        if (index + 1 > level->f_00c) {
                            if (!level->emergeMap) {
                                LEGO_LEVEL_PTR_SET(level->emergeMap,
                                                   malloc(sizeof(LegoEmergeTrigger2A) * (index + 1)));
                            } else {
                                LEGO_LEVEL_PTR_SET(level->emergeMap,
                                                   realloc(LEGO_LEVEL_PTR(void, level->emergeMap),
                                                           sizeof(LegoEmergeTrigger2A) * (index + 1)));
                            }
                            memset(&LEGO_2A_TRIGGERS(level)[level->f_00c], 0,
                                   sizeof(LegoEmergeTrigger2A) * ((index + 1) - level->f_00c));
                            level->f_00c = index + 1;
                        }
                        if (slot) {
                            LEGO_2A_BLOCK(level, x, y).flags2 |= 0x40;
                            LEGO_2A_TRIGGERS(level)[index].emergePoints[slot - 1].blockPos.x = x;
                            LEGO_2A_TRIGGERS(level)[index].emergePoints[slot - 1].blockPos.y = y;
                            LEGO_2A_TRIGGERS(level)[index].emergePoints[slot - 1].used = 1;
                        } else {
                            LEGO_2A_BLOCK(level, x, y).flags2 |= 0x80;
                            LEGO_2A_TRIGGERS(level)[index].blockPos.x = x;
                            LEGO_2A_TRIGGERS(level)[index].blockPos.y = y;
                        }
                    }
                }
            }
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* EmergeMapTriggerSpawn [group 2A]: Lego_EmergeTrigger? When blockPos is an
 * expired trigger block, the first emerge point that spawns a rock monster
 * of the level's emerge type restarts the trigger's timer. */
// FUNCTION: LEGORR 0x0042c330
int lego_emerge_map_trigger_spawn(Lego_Level* level, const LegoPoint2I2A* blockPos,
                                  LegoPoint2I2A* outPos)
{
    unsigned int i, j;

    for (i = 0; i < level->f_00c; i++) {
        if (blockPos->x == LEGO_2A_TRIGGERS(level)[i].blockPos.x &&
            blockPos->y == LEGO_2A_TRIGGERS(level)[i].blockPos.y &&
            LEGO_2A_TRIGGERS(level)[i].timer <= 0.0f) {
            for (j = 0; j < 5; j++) {
                if (LEGO_2A_TRIGGERS(level)[i].emergePoints[j].used) {
                    if (creature_spawn_from_emergence(&legoGlobs_rockMonsterData[level->f_0b0], 3, level->f_0b0,
                                                      LEGO_2A_TRIGGERS(level)[i].emergePoints[j].blockPos.x,
                                                      LEGO_2A_TRIGGERS(level)[i].emergePoints[j].blockPos.y)) {
                        LEGO_2A_TRIGGERS(level)[i].timer = level->emergeTimeOut;
                        if (outPos) {
                            outPos->x = LEGO_2A_TRIGGERS(level)[i].emergePoints[j].blockPos.x;
                            outPos->y = LEGO_2A_TRIGGERS(level)[i].emergePoints[j].blockPos.y;
                        }
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* LevelRecordTimerCountdown [group 2A]: Lego_UpdateEmergeTriggers? Counts
 * the running trigger timers down. */
// FUNCTION: LEGORR 0x0042c440
void lego_level_record_timer_countdown(Lego_Level* level, float elapsed)
{
    unsigned int i;

    for (i = 0; i < level->f_00c; i++) {
        if (LEGO_2A_TRIGGERS(level)[i].timer > 0.0f) {
            LEGO_2A_TRIGGERS(level)[i].timer -= elapsed;
        }
    }
}

/* ---- part 2 functions, group 2A: end ---- */

/* ---- part 2 functions, group 2B (0042c480-0042d460): begin ---- */

/* Lego_LoadErodeMap: the erosion layer, one byte per block (+0x02); type
 * 11 blocks are lava sources. */
// FUNCTION: LEGORR 0x0042c480
int lego_load_erode_cryore(Lego_Level* level, const char* filename, int modifier)
{
    int handle;
    unsigned int width, height, x, y;
    int value;
    Lego_Block* block;
    int blockPos[2];

    if ((handle = map_open2(filename, NULL)) != -1) {
        level_map_dimensions_get(handle, &width, &height);
        if (width == level->width && height == level->height) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    value = level_map_cell_get(handle, x, y) - modifier;
                    if (value == 5) value = 4;
                    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_02 = (unsigned char)value;
                    block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x];
                    if (block->f_02 == 6) {
                        block->f_07 = 4;
                    } else if (block->f_02 == 11) {
                        blockPos[0] = x;
                        blockPos[1] = y;
                        type11_cell_coordinate_append(blockPos);
                        lego_block_activity_create(level, blockPos, 1);
                    }
                }
            }
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* Level_Block_GetCrystalOreCounts? The block's crystal/ore byte (+0x05):
 * 0 for none, else (n - 1) / 4 picks the count (1, 3, 5, 10 or 25) and
 * (n - 1) % 4 which of the four outputs gets it. */
// FUNCTION: LEGORR 0x0042c5b0
int lego_terrain_cell_byte_state_classify_current(const int* blockPos,
    unsigned int* crystalsLv0, unsigned int* crystalsLv1,
    unsigned int* oreLv0, unsigned int* oreLv1)
{
    unsigned int cryOre = LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
        [blockPos[1] * legoGlobs_currLevel->width + blockPos[0]].f_05;
    unsigned int counts[5];
    unsigned int count;

    counts[0] = 1;
    counts[1] = 3;
    counts[2] = 5;
    counts[3] = 10;
    counts[4] = 25;
    *crystalsLv0 = *crystalsLv1 = *oreLv0 = *oreLv1 = 0;
    if (cryOre-- == 0) return 0;
    count = counts[cryOre / 4];
    switch (cryOre % 4) {
    case 0: *crystalsLv0 = count; return 1;
    case 1: *oreLv0 = count; return 1;
    case 2: *crystalsLv1 = count; return 1;
    case 3: *oreLv1 = count; return 1;
    }
    return 1;
}

/* Lego_LoadCryOreMap: the crystal/ore layer (+0x05). */
// FUNCTION: LEGORR 0x0042c6a0
int lego_load_path_ai(Lego_Level* level, const char* filename, char modifier)
{
    int handle;
    unsigned int width, height, x, y;

    if ((handle = map_open2(filename, NULL)) != -1) {
        level_map_dimensions_get(handle, &width, &height);
        if (width == level->width && height == level->height) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_05 =
                        (unsigned char)((char)level_map_cell_get(handle, x, y) - modifier);
                }
            }
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* Lego_LoadPathMap: 2 marks a rubble path, 1 a power path (built at once). */
// FUNCTION: LEGORR 0x0042c760
int lego_load_fallin(Lego_Level* level, const char* filename, int modifier)
{
    int handle;
    unsigned int width, height, x, y;
    int value;
    unsigned int* flags1;
    int blockPos[2];

    if (filename != NULL && (handle = map_open2(filename, NULL)) != -1) {
        level_map_dimensions_get(handle, &width, &height);
        if (width == level->width && height == level->height) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    blockPos[0] = x;
                    blockPos[1] = y;
                    value = level_map_cell_get(handle, x, y) - modifier;
                    flags1 = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1;
                    if (*flags1 & 0x8) {
                        if (value == 2) {
                            *flags1 |= 0x20000000;
                        } else if (value == 1) {
                            /* 00430320's body (group 2F), inlined: the
                             * level copy keeps the third test from reusing
                             * the block address, as in the original. */
                            Lego_Level* currLevel = legoGlobs_currLevel;
                            Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, currLevel->blocks)[blockPos[1] * currLevel->width + blockPos[0]];
                            int i;

                            if (!(block->flags1 & 0x400) &&
                                !(block->flags2 & 0x4) &&
                                !(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[blockPos[1] * legoGlobs_currLevel->width + blockPos[0]].flags1 & 0x8000) &&
                                !terrain_cell_predicate_check_current((const uint32_t*)blockPos)) {
                                LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[blockPos[1] * legoGlobs_currLevel->width + blockPos[0]].flags1 &= ~0x200000;
                                LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[blockPos[1] * legoGlobs_currLevel->width + blockPos[0]].flags1 &= ~0x20000000;
                                LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[blockPos[1] * legoGlobs_currLevel->width + blockPos[0]].flags1 &= ~0x40000000;
                                entity_activity_scene_state_update_retail();
                                coordinate_pair_callback_dispatch(blockPos);
                                lego_noop_block_adapter(blockPos);
                                for (i = 0; i < 4; i++) coordinate_pair_event_queue(blockPos, (void*)0x3d);
                                LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[blockPos[1] * legoGlobs_currLevel->width + blockPos[0]].flags1 |= 0x3;
                            }
                        }
                    }
                }
            }
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* Lego_LoadFallinMap: the fall-in layer. A value above 4 is a fall-in that
 * spreads (+0x38); the timer (+0x40) starts at a random phase. */
// FUNCTION: LEGORR 0x0042c9d0
int lego_load_block_pointers(Lego_Level* level, const char* filename)
{
    int handle;
    unsigned int width, height, x, y;
    unsigned int value;

    legoGlobs_f_556f18 = 0;
    if (filename != NULL && (handle = map_open2(filename, NULL)) != -1) {
        level_map_dimensions_get(handle, &width, &height);
        if (width == level->width && height == level->height) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    value = level_map_cell_get(handle, x, y);
                    LEGO_FIELD(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x], unsigned int, 0x3c) = 0;
                    if (value != 0) {
                        LEGO_FIELD(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x], unsigned int, 0x3c) = value;
                        LEGO_FIELD(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x], float, 0x40) =
                            (float)((unsigned int)LEGO_FIELD(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x], short, 0x20) %
                                    (value * legoGlobs_f_556f14 * 25));
                        LEGO_FIELD(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x], int, 0x38) = 0;
                        if (value > 4) {
                            LEGO_FIELD(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x], unsigned int, 0x3c) = value - 4;
                            LEGO_FIELD(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x], int, 0x38) = 1;
                        }
                    }
                }
            }
            legoGlobs_f_556f18 = 1;
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* Lego_UpdateFallins: advance each fall-in block's timer and trigger the
 * fall-in when it expires. */
// FUNCTION: LEGORR 0x0042cb70
void lego_camera_grid_expiry(float elapsed)
{
    Lego_Level* level = (Lego_Level*)level_current_owner_get();
    unsigned int width, height, x, y;
    int blockPos[2];
    Lego_Block* block;

    if (legoGlobs_f_556f18 && (width = level->width) != 0 && (height = level->height) != 0) {
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                blockPos[0] = x;
                blockPos[1] = y;
                block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x];
                if (block->f_10[11] != 0) {
                    ((float*)LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_10)[12] += elapsed;
                    if ((float)(LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_10[11] * legoGlobs_f_556f14 * 25) < ((float*)LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_10)[12]) {
                        ((float*)LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_10)[12] = 0.0f;
                        if (lava_erosion_cell_eligible_and_mutate(blockPos,
                                LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_10[10])) {
                            info_send(6, NULL, NULL, (struct InfoPoint2I*)blockPos);
                        }
                    }
                }
            }
        }
    }
}

/* Lego_LoadTerrainMap: the surface layer (+0x04). */
// FUNCTION: LEGORR 0x0042cc90
int lego_load_terrain_map(Lego_Level* level, const char* filename, int modifier)
{
    int handle;
    unsigned int width, height, x, y;
    int value;

    if (filename != NULL && (handle = map_open2(filename, NULL)) != -1) {
        level_map_dimensions_get(handle, &width, &height);
        if (width == level->width && height == level->height) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    value = level_map_cell_get(handle, x, y) - modifier;
                    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_04 = (unsigned char)value;
                }
            }
            level_map_slot_release(handle);
            return 1;
        }
        level_map_slot_release(handle);
    }
    return 0;
}

/* Lego_GetUpgradeData: the upgrade model with this name, or NULL. */
// FUNCTION: LEGORR 0x0042cd50
void* lego_upgrade_name_entry_find(const char* name)
{
    unsigned int loop;

    for (loop = 0; loop < legoGlobs_catalogCounts[4]; loop++) {
        if (_strcmpi(name, legoGlobs_upgradeName[loop]) == 0) {
            return (char*)s_upgrade_records + loop * 16;
        }
    }
    return NULL;
}

/* Lego_LoadVehicleTypes. The four type loaders and Lego_LoadUpgradeTypes
 * share this shape. The name copy is a memcpy of strlen + 1: the original
 * keeps the source in esi, which the strcpy intrinsic does not. */
// FUNCTION: LEGORR 0x0042cda0
int lego_entity_vehicle_type_table_load(void)
{
    struct BlockRec* arrayItem;
    unsigned int loop;

    legoGlobs_catalogCounts[0] = 0;
    loop = 0;
    for (arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root,
             // STRING: LEGORR 0x004a4630
             "VehicleTypes", NULL));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        legoGlobs_catalogCounts[0]++;
    }
    if (legoGlobs_catalogCounts[0] != 0) {
        if ((legoGlobs_vehicleData = (LegoVehicleModelE*)malloc(sizeof(LegoVehicleModelE) * legoGlobs_catalogCounts[0])) != NULL) {
            if ((legoGlobs_vehicleName = (char**)malloc(sizeof(char*) * legoGlobs_catalogCounts[0])) != NULL) {
                arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root, "VehicleTypes", NULL));
                while (arrayItem != NULL) {
                    if (!lego_vehicle_load_adapter(&legoGlobs_vehicleData[loop], loop, legoGlobs_rootCont,
                                                   LEGO_FIELD(arrayItem, char*, 0x08), s_engine_config_root)) break;
                    scene_resource_visibility_set(&legoGlobs_vehicleData[loop], 1);
                    legoGlobs_vehicleName[loop] = (char*)malloc(strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    memcpy(legoGlobs_vehicleName[loop], LEGO_FIELD(arrayItem, char*, 0x04), strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    loop++;
                    arrayItem = block_next_equal_key(arrayItem);
                }
                if (arrayItem == NULL) return 1;
                free(legoGlobs_vehicleName);
            }
            free(legoGlobs_vehicleData);
        }
    }
    return 0;
}

/* Lego_LoadMiniFigureTypes */
// FUNCTION: LEGORR 0x0042cf50
int lego_entity_minifigure_type_table_load(void)
{
    struct BlockRec* arrayItem;
    unsigned int loop;

    legoGlobs_catalogCounts[1] = 0;
    loop = 0;
    for (arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root,
             // STRING: LEGORR 0x004a4640
             "MiniFigureTypes", NULL));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        legoGlobs_catalogCounts[1]++;
    }
    if (legoGlobs_catalogCounts[1] != 0) {
        if ((legoGlobs_miniFigureData = (LegoCreatureModelE*)malloc(sizeof(LegoCreatureModelE) * legoGlobs_catalogCounts[1])) != NULL) {
            if ((legoGlobs_miniFigureName = (char**)malloc(sizeof(char*) * legoGlobs_catalogCounts[1])) != NULL) {
                arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root, "MiniFigureTypes", NULL));
                while (arrayItem != NULL) {
                    if (!lego_creature_load_adapter(&legoGlobs_miniFigureData[loop], loop, legoGlobs_rootCont,
                                                    LEGO_FIELD(arrayItem, char*, 0x08), s_engine_config_root)) break;
                    animation_object_visibility_set(&legoGlobs_miniFigureData[loop], 1);
                    legoGlobs_miniFigureName[loop] = (char*)malloc(strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    memcpy(legoGlobs_miniFigureName[loop], LEGO_FIELD(arrayItem, char*, 0x04), strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    loop++;
                    arrayItem = block_next_equal_key(arrayItem);
                }
                if (arrayItem == NULL) return 1;
                free(legoGlobs_miniFigureName);
            }
            free(legoGlobs_miniFigureData);
        }
    }
    return 0;
}

/* Lego_LoadRockMonsterTypes */
// FUNCTION: LEGORR 0x0042d100
int lego_entity_rockmonster_type_table_load(void)
{
    struct BlockRec* arrayItem;
    unsigned int loop;

    legoGlobs_catalogCounts[2] = 0;
    loop = 0;
    for (arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root,
             // STRING: LEGORR 0x004a4650
             "RockMonsterTypes", NULL));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        legoGlobs_catalogCounts[2]++;
    }
    if (legoGlobs_catalogCounts[2] != 0) {
        if ((legoGlobs_rockMonsterData = (LegoCreatureModelE*)malloc(sizeof(LegoCreatureModelE) * legoGlobs_catalogCounts[2])) != NULL) {
            if ((legoGlobs_rockMonsterName = (char**)malloc(sizeof(char*) * legoGlobs_catalogCounts[2])) != NULL) {
                arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root, "RockMonsterTypes", NULL));
                while (arrayItem != NULL) {
                    if (!lego_creature_load_adapter(&legoGlobs_rockMonsterData[loop], loop, legoGlobs_rootCont,
                                                    LEGO_FIELD(arrayItem, char*, 0x08), s_engine_config_root)) break;
                    animation_object_visibility_set(&legoGlobs_rockMonsterData[loop], 1);
                    legoGlobs_rockMonsterName[loop] = (char*)malloc(strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    memcpy(legoGlobs_rockMonsterName[loop], LEGO_FIELD(arrayItem, char*, 0x04), strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    loop++;
                    arrayItem = block_next_equal_key(arrayItem);
                }
                if (arrayItem == NULL) return 1;
                free(legoGlobs_rockMonsterName);
            }
            free(legoGlobs_rockMonsterData);
        }
    }
    return 0;
}

/* Lego_LoadBuildingTypes */
// FUNCTION: LEGORR 0x0042d2b0
int lego_entity_building_types_load(void)
{
    struct BlockRec* arrayItem;
    unsigned int loop;

    legoGlobs_catalogCounts[3] = 0;
    loop = 0;
    for (arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root,
             // STRING: LEGORR 0x004a4664
             "BuildingTypes", NULL));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        legoGlobs_catalogCounts[3]++;
    }
    if (legoGlobs_catalogCounts[3] != 0) {
        if ((legoGlobs_buildingData = (LegoBuildingModelE*)malloc(sizeof(LegoBuildingModelE) * legoGlobs_catalogCounts[3])) != NULL) {
            if ((legoGlobs_buildingName = (char**)malloc(sizeof(char*) * legoGlobs_catalogCounts[3])) != NULL) {
                arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root, "BuildingTypes", NULL));
                while (arrayItem != NULL) {
                    if (!entity_building_type_config_record_parse(&legoGlobs_buildingData[loop], loop, legoGlobs_rootCont,
                                                                  LEGO_FIELD(arrayItem, char*, 0x08), (void*)s_engine_config_root)) break;
                    scene_object_pair_apply_mode(&legoGlobs_buildingData[loop], 1);
                    legoGlobs_buildingName[loop] = (char*)malloc(strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    memcpy(legoGlobs_buildingName[loop], LEGO_FIELD(arrayItem, char*, 0x04), strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    loop++;
                    arrayItem = block_next_equal_key(arrayItem);
                }
                if (arrayItem == NULL) return 1;
                free(legoGlobs_buildingName);
            }
            free(legoGlobs_buildingData);
        }
    }
    return 0;
}

/* Lego_LoadUpgradeTypes: Upgrade_Load cannot fail. */
// FUNCTION: LEGORR 0x0042d460
int lego_upgrade_types_load(void)
{
    struct BlockRec* arrayItem;
    unsigned int loop;

    legoGlobs_catalogCounts[4] = 0;
    loop = 0;
    for (arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root,
             // STRING: LEGORR 0x004a4674
             "UpgradeTypes", NULL));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        legoGlobs_catalogCounts[4]++;
    }
    if (legoGlobs_catalogCounts[4] != 0) {
        if ((s_upgrade_records = malloc(16 * legoGlobs_catalogCounts[4])) != NULL) {
            if ((legoGlobs_upgradeName = (char**)malloc(sizeof(char*) * legoGlobs_catalogCounts[4])) != NULL) {
                arrayItem = block_subgroup(legoGlobs_config, string_path_join(s_engine_config_root, "UpgradeTypes", NULL));
                while (arrayItem != NULL) {
                    render_resource_binding_initialize((struct RenderResourceBinding*)((char*)s_upgrade_records + loop * 16),
                                                       (void*)(size_t)loop, legoGlobs_rootCont, LEGO_FIELD(arrayItem, char*, 0x08));
                    animation_object_visibility_set((char*)s_upgrade_records + loop * 16, 1);
                    legoGlobs_upgradeName[loop] = (char*)malloc(strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    memcpy(legoGlobs_upgradeName[loop], LEGO_FIELD(arrayItem, char*, 0x04), strlen(LEGO_FIELD(arrayItem, char*, 0x04)) + 1);
                    loop++;
                    arrayItem = block_next_equal_key(arrayItem);
                }
                if (arrayItem == NULL) return 1;
                free(legoGlobs_upgradeName);
            }
            free(s_upgrade_records);
        }
    }
    return 0;
}


/* ---- part 2 functions, group 2B: end ---- */

/* ---- part 2 functions, group 2C (0042d600-0042dff0): begin ---- */

/* ObjectNamesLoad [group 2C] */
// FUNCTION: LEGORR 0x0042d600
void lego_object_names_load(struct BlockRec* config)
{
    const char* name;
    unsigned int i;

    legoGlobs_objectNamesKind1 = (const char**)malloc(legoGlobs_catalogCounts[0] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[0]; i++) {
        legoGlobs_objectNamesKind1[i] = formatted_text_escape_decode_and_emit(block_get_strptr(config, string_path_join(s_engine_config_root,
            // STRING: LEGORR 0x004a4684
            "ObjectNames", legoGlobs_vehicleName[i], NULL)));
    }
    legoGlobs_objectNamesKind2 = (const char**)malloc(legoGlobs_catalogCounts[1] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[1]; i++) {
        legoGlobs_objectNamesKind2[i] = formatted_text_escape_decode_and_emit(block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", legoGlobs_miniFigureName[i], NULL)));
    }
    legoGlobs_objectNamesKind3 = (const char**)malloc(legoGlobs_catalogCounts[2] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[2]; i++) {
        legoGlobs_objectNamesKind3[i] = formatted_text_escape_decode_and_emit(block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", legoGlobs_rockMonsterName[i], NULL)));
    }
    legoGlobs_objectNamesKind4 = (const char**)malloc(legoGlobs_catalogCounts[3] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[3]; i++) {
        legoGlobs_objectNamesKind4[i] = formatted_text_escape_decode_and_emit(block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", legoGlobs_buildingName[i], NULL)));
    }
    legoGlobs_objectNamesKind10 = (const char**)malloc(legoGlobs_catalogCounts[4] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[4]; i++) {
        if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", legoGlobs_upgradeName[i], NULL))) != NULL)
            legoGlobs_objectNamesKind10[i] = formatted_text_escape_decode_and_emit(name);
        legoGlobs_objectNamesKind10[i] = NULL;
    }
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "PowerCrystal", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[0] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "Ore", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[1] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "ProcessedOre", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[2] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "Dynamite", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[3] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "Barrier", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[4] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "ElectricFence", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[5] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "SpiderWeb", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[6] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "OohScary", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[7] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectNames", "Path", NULL))) != NULL)
        legoGlobs_objectNamesSpecial[8] = formatted_text_escape_decode_and_emit(name);
}

/* ObjectTheNamesLoad [group 2C] */
// FUNCTION: LEGORR 0x0042da20
void lego_object_the_names_load(struct BlockRec* config)
{
    const char* name;
    unsigned int i;

    legoGlobs_objectTheNamesKind1 = (const char**)malloc(legoGlobs_catalogCounts[0] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[0]; i++) {
        legoGlobs_objectTheNamesKind1[i] = formatted_text_escape_decode_and_emit(block_get_strptr(config, string_path_join(s_engine_config_root,
            // STRING: LEGORR 0x004a4690
            "ObjectTheNames", legoGlobs_vehicleName[i], NULL)));
    }
    legoGlobs_objectTheNamesKind2 = (const char**)malloc(legoGlobs_catalogCounts[1] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[1]; i++) {
        legoGlobs_objectTheNamesKind2[i] = formatted_text_escape_decode_and_emit(block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", legoGlobs_miniFigureName[i], NULL)));
    }
    legoGlobs_objectTheNamesKind3 = (const char**)malloc(legoGlobs_catalogCounts[2] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[2]; i++) {
        legoGlobs_objectTheNamesKind3[i] = formatted_text_escape_decode_and_emit(block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", legoGlobs_rockMonsterName[i], NULL)));
    }
    legoGlobs_objectTheNamesKind4 = (const char**)malloc(legoGlobs_catalogCounts[3] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[3]; i++) {
        legoGlobs_objectTheNamesKind4[i] = formatted_text_escape_decode_and_emit(block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", legoGlobs_buildingName[i], NULL)));
    }
    legoGlobs_objectTheNamesKind10 = (const char**)malloc(legoGlobs_catalogCounts[4] * sizeof(char*));
    for (i = 0; i < legoGlobs_catalogCounts[4]; i++) {
        if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", legoGlobs_upgradeName[i], NULL))) != NULL)
            legoGlobs_objectTheNamesKind10[i] = formatted_text_escape_decode_and_emit(name);
        legoGlobs_objectTheNamesKind10[i] = NULL;
    }
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "PowerCrystal", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[0] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "Ore", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[1] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "ProcessedOre", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[2] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "Dynamite", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[3] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "Barrier", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[4] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "ElectricFence", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[5] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "SpiderWeb", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[6] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "OohScary", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[7] = formatted_text_escape_decode_and_emit(name);
    if ((name = block_get_strptr(config, string_path_join(s_engine_config_root, "ObjectTheNames", "Path", NULL))) != NULL)
        legoGlobs_objectTheNamesSpecial[8] = formatted_text_escape_decode_and_emit(name);
}

/* ViewportTargetPositionClamp [group 2C] (Lego_GotoInfo): move the view
 * to an object or a block, kept inside the map's border. With smooth, the
 * target goes to the blend position (00556150) that the main loop eases
 * the camera towards (00556158); otherwise to the camera position itself. */
// FUNCTION: LEGORR 0x0042de40
void lego_viewport_target_position_clamp(struct EntitySlot* liveObj, const int* blockPos,
                                         int smooth)
{
    Lego_Level* level = legoGlobs_currLevel;
    int pos[2];
    int x, y;

    legoGlobs_f_556158 = smooth;
    if (liveObj != NULL) {
        if (smooth)
            entity_transform_position_get_outputs(liveObj, &legoGlobs_f_556150, &legoGlobs_f_556154);
        else
            entity_transform_position_get_outputs(liveObj, &legoGlobs_f_556148, &legoGlobs_f_55614c);
        entity_position_to_terrain_cell_adapter(liveObj, &pos[0], &pos[1]);
    } else {
        if (smooth)
            terrain_map_block_to_world(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                                       blockPos[0], blockPos[1], &legoGlobs_f_556150, &legoGlobs_f_556154);
        else
            terrain_map_block_to_world(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                                       blockPos[0], blockPos[1], &legoGlobs_f_556148, &legoGlobs_f_55614c);
        pos[0] = blockPos[0];
        pos[1] = blockPos[1];
    }

    x = pos[0];
    y = pos[1];
    if (pos[0] < view_grid_edge_get()) x = view_grid_edge_get();
    if (pos[0] >= LEGO_FIELD(LEGO_LEVEL_PTR(void, level->map), int, 0x0) - view_grid_edge_get())
        x = LEGO_FIELD(LEGO_LEVEL_PTR(void, level->map), int, 0x0) - view_grid_edge_get() - 1;
    if (pos[1] < view_grid_edge_get()) y = view_grid_edge_get();
    if (pos[1] >= LEGO_FIELD(LEGO_LEVEL_PTR(void, level->map), int, 0x4) - view_grid_edge_get())
        y = LEGO_FIELD(LEGO_LEVEL_PTR(void, level->map), int, 0x4) - view_grid_edge_get() - 1;

    if (pos[0] != x || pos[1] != y) {
        if (smooth)
            terrain_map_block_to_world(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                                       x, y, &legoGlobs_f_556150, &legoGlobs_f_556154);
        else
            terrain_map_block_to_world(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                                       x, y, &legoGlobs_f_556148, &legoGlobs_f_55614c);
    }
    legoGlobs_flags1 |= 0x200000;
}

/* ObjectPointerRegistryClearMatch [group 2C] */
// FUNCTION: LEGORR 0x0042dfc0
void lego_object_pointer_registry_clear_match(void* object)
{
    unsigned int i;

    for (i = 0; i < legoGlobs_recordObjectCount; i++) {
        if (object == legoGlobs_recordObjects[i])
            legoGlobs_recordObjects[i] = NULL;
    }
}

/* ObjectPointerRegistryGetSlot [group 2C] */
// FUNCTION: LEGORR 0x0042dff0
int lego_object_pointer_registry_get_slot(unsigned int index, void** objOut)
{
    if (index < legoGlobs_recordObjectCount) {
        if ((*objOut = legoGlobs_recordObjects[index]) != NULL)
            return 1;
    }
    return 0;
}

/* ---- part 2 functions, group 2C: end ---- */

/* ---- part 2 functions, group 2D (0042e020-0042ef40): begin ---- */

/* OListEntityRecordParse: Lego_LoadOLObjectList. Create the objects of a
 * level's object list (`.ol`): each item gives a type, a block position
 * (1-based, fractional), a heading in degrees, a health and the item name
 * of the object it drives. TVCamera places the top-down camera instead.
 * Objects on blocks not yet uncovered are registered hidden. The `driving`
 * links are resolved once every object exists. */
// FUNCTION: LEGORR 0x0042e020
int lego_object_list_load_file(Lego_Level* level, const char* fileName)
{
    struct BlockRec* ol;
    struct BlockRec* item;
    char* typeName;
    char* olName;
    char* drivingName;
    float xPos;
    float yPos;
    float heading;
    float health;
    unsigned int bx;
    unsigned int by;
    LegoPoint2FC wPos;
    LegoPoint2FC hiddenPos;
    int objType;
    unsigned int objID;
    void* objModel;
    struct EntitySlot* obj;
    struct EntitySlot* target;
    int blockPos[2];
    void* shape;
    unsigned int shapeCount;
    unsigned int dir;
    unsigned int i;
    unsigned int j;
    int first = 1;
    unsigned int result = 0;
    unsigned int count = 0;
    struct EntitySlot* objList[200];
    char* olNames[200];
    char* drivingNames[200];

    if ((ol = block_open(fileName)) != NULL) {
        memset(legoGlobs_recordObjects, 0, sizeof(void*) * 10);
        legoGlobs_recordObjectCount = 0;

        if ((item = block_subgroup(ol, string_path_join(s_engine_config_root, NULL))) != NULL) {
            for (; item != NULL; item = block_next_equal_key(item)) {
                if ((typeName = block_get_str_copy(ol, string_path_join(s_engine_config_root, item->key,
                        // STRING: LEGORR 0x004a46d4
                        "type", NULL))) == NULL) break;
                if (block_find(ol, string_path_join(s_engine_config_root, item->key,
                        // STRING: LEGORR 0x004a46cc
                        "xpos", NULL)) == NULL) break;
                if (block_find(ol, string_path_join(s_engine_config_root, item->key,
                        // STRING: LEGORR 0x004a46c4
                        "ypos", NULL)) == NULL) break;
                if (block_find(ol, string_path_join(s_engine_config_root, item->key,
                        // STRING: LEGORR 0x004a46bc
                        "heading", NULL)) == NULL) break;

                xPos = (float)LEGO_A_CFG_FLOAT(ol, string_path_join(s_engine_config_root, item->key, "xpos", NULL));
                yPos = (float)LEGO_A_CFG_FLOAT(ol, string_path_join(s_engine_config_root, item->key, "ypos", NULL));
                heading = (float)LEGO_A_CFG_FLOAT(ol, string_path_join(s_engine_config_root, item->key, "heading", NULL));
                if ((health = (float)LEGO_A_CFG_FLOAT(ol, string_path_join(s_engine_config_root, item->key,
                        // STRING: LEGORR 0x004a46b4
                        "health", NULL))) == 0.0f) health = 100.0f;
                drivingNames[count] = drivingName = block_get_str_copy(ol, string_path_join(s_engine_config_root, item->key,
                        // STRING: LEGORR 0x004a46ac
                        "driving", NULL));
                olNames[count] = olName = string_duplicate(item->key);

                xPos -= 1.0f;
                yPos -= 1.0f;
                by = (unsigned int)yPos;
                bx = (unsigned int)xPos;
                terrain_map_block_to_world(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by, &wPos.x, &wPos.y);
                wPos.x += (xPos - (bx + 0.5f)) * level->blockSize;
                yPos = (yPos - (by + 0.5f)) * level->blockSize;
                wPos.y -= yPos;
                heading = (heading * (1.0f / 180.0f)) * 3.1415927f;

                if (!lego_entity_type_pair_lookup(typeName, &objType, &objID, &objModel)) break;

                obj = NULL;
                if (objType == -1) {
                    selection_record_point_set((struct ViewContext*)s_zoom_step_context, wPos.x, wPos.y);
                    camera_rotation_set(s_zoom_step_context, heading);
                    camera_tilt_set(s_zoom_step_context, 0.8f);
                    legoGlobs_f_5563f8 = wPos;
                    s_lego_radar_zoom = 15.0f;
                } else if (!((bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1)
                                 ? (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                                        [legoGlobs_currLevel->width * by + bx].flags1 & 8)
                                 : 0)) {
                    hiddenPos = wPos;
                    deferred_olist_entity_queue_add_original(objModel, objType, objID, &hiddenPos.x,
                                                             heading, health, olName, drivingName);
                } else if (objType == 4) {
                    blockPos[0] = bx;
                    blockPos[1] = by;
                    dir = (unsigned int)((float)(heading * 0.15915494f) * 8.0f);
                    shape = (void*)(uintptr_t)task_field84_get_and_field88_out(&legoGlobs_buildingData[objID], (uint32_t*)&shapeCount);
                    dir = ((dir & 1) + dir) >> 1;
                    shape = rotated_pairs_scratch_fill(blockPos, (const int*)shape, shapeCount, dir);
                    obj = (struct EntitySlot*)construction_place_building(objID, blockPos, dir,
                        (int*)shape, shapeCount, level->disableStartTeleport);
                    if (obj) entity_kind_record_update_dispatch(obj);
                } else {
                    obj = object_create_from_container_adapter(objModel, objType, objID);
                }

                objList[count] = obj;
                if (obj) {
                    if (first) {
                        screen_effect_record_set_original(legoGlobs_cameraTrack, obj, 2.0f,
                                                          entity_stats_float_at7c(obj), 0.7f, 0.01f);
                    }
                    first = 0;
                    if (objType != 4) entity_world_position_set(obj, wPos.x, wPos.y, heading, 1);
                    if (objType == 6 || objType == 7 || objType == 8 || objType == 9) cmd_create_task(obj, 0);
                    if (s_lego_f_4e8448 == NULL && LEGO_FIELD(obj, int, 0x0) == 2) s_lego_f_4e8448 = obj;
                    if (legoGlobs_recordObjectCount < 10) {
                        legoGlobs_recordObjects[legoGlobs_recordObjectCount] = obj;
                        legoGlobs_recordObjectCount++;
                    }
                    LEGO_FIELD(obj, uint32_t, 0x3ec) |= 0x200;
                    LEGO_FIELD(obj, float, 0x340) = health;
                    building_upgrade_state_changed_dispatch(LEGO_FIELD(obj, int, 0x0),
                                                            LEGO_FIELD(obj, int, 0x4), 0, 1);
                    if (s_panel_highlight_flags & 0x40000) entity_state_override_pop(obj);
                    entity_queue_for_rockfall_processing_if_eligible(obj);
                    if (objType == 12) {
                        LEGO_FIELD(obj, uint32_t, 0x3e8) |= 0x10000;
                        scene_action_name_set(LEGO_FIELD(obj, void*, 0x1c),
                            // STRING: LEGORR 0x004a46a0
                            "FlapInWind");
                        ddiscene_frame_update_and_callbacks(LEGO_FIELD(obj, void*, 0x1c), 0.0f);
                        spiderweb_add(bx, by, (struct Lego_Object*)obj);
                    } else if (objType == 11) {
                        electric_fence_add((struct ElectricFenceObject*)obj);
                    }
                }

                lego_e_mem_free(typeName);
                count++;
            }

            if (item == NULL) result = 1;
        }
        block_close(ol);
    }

    if (result) {
    for (result = 0; result < count; result++) {
        if ((obj = objList[result]) != NULL && (drivingName = drivingNames[result]) != NULL) {
            for (j = 0; j < count; j++) {
                if (!_strcmpi(drivingName, olNames[j])) {
                    if ((target = objList[j]) != NULL) {
                        LEGO_FIELD(obj, struct EntitySlot*, 0x2fc) = target;
                        LEGO_FIELD(obj, uint32_t, 0x3e4) |= 4;
                        entity_linked_activity_event_dispatch(obj,
                            (entity_capability_flags_get(target) & 0x100) ? 1 :
                            (entity_stats_word_get(target) & 0x1000000) ?
                                ((entity_stats_word_get(target) & 0x2000000) ? 1 : 2) : 4);
                        entity_linked_route_flag_clear_original(obj, target);
                    }
                    break;
                }
            }
        }
    }

    for (i = 0; i < count; i++) {
        if (olNames[i]) lego_e_mem_free(olNames[i]);
        if (drivingNames[i]) lego_e_mem_free(drivingNames[i]);
    }

    return 1;
    }
    return 0;
}

/* ObjectListEntityTypeResolve: Lego_GetObjectByName. The object type, the
 * type's index and its model (the type data record or the MiscObjects
 * container) of an object list's type name. The rock monsters are matched
 * first, then the special objects, then the vehicles, mini-figures and
 * buildings. "SpiderWeb" is marked at its first use (004a3f40, finding
 * 620). "tvcamera", "ProcessedOre", "Dynamite", "ElectricFence",
 * "Barrier" and "OohScary" are Lego.c's own literals and are marked here
 * (finding 626); the port's s_item_name_* copies in lego_port.cpp are no
 * longer annotated. "PowerCrystal", "Ore" and "Path" belong to other
 * units. */
// FUNCTION: LEGORR 0x0042e8b0
int lego_entity_type_pair_lookup(const char* objName, int* outType,
                                 unsigned int* outID, void** outModel)
{
    unsigned int i;

    *outID = 0;
    for (i = 0; i < legoGlobs_catalogCounts[2]; i++) {
        if (!_strcmpi(legoGlobs_rockMonsterName[i], objName)) {
            *outType = 3;
            *outID = i;
            if (outModel) *outModel = &legoGlobs_rockMonsterData[i];
            return 1;
        }
    }

    // STRING: LEGORR 0x004a46dc
    if (!_strcmpi("tvcamera", objName)) {
        *outType = -1;
        if (outModel) *outModel = NULL;
    } else if (!_strcmpi("PowerCrystal", objName)) {
        *outType = 6;
        if (outModel) *outModel = legoGlobs_contCrystal;
    } else if (!_strcmpi("Ore", objName)) {
        *outType = 7;
        *outID = 0;
        if (outModel) *outModel = legoGlobs_contOresTable;
    } else if (!_strcmpi(
                   // STRING: LEGORR 0x004a3fa0
                   "ProcessedOre", objName)) {
        *outType = 7;
        *outID = 1;
        if (outModel) *outModel = legoGlobs_contProcessedOre;
    } else if (!_strcmpi(
                   // STRING: LEGORR 0x004a401c
                   "Boulder", objName)) {
        *outType = 5;
        if (outModel) *outModel = legoGlobs_contBoulder;
    } else if (!_strcmpi(
                   // STRING: LEGORR 0x004a3f10
                   "Pusher", objName)) {
        *outType = 16;
        if (outModel) *outModel = legoGlobs_contPusher;
    } else if (!_strcmpi("LaserShot", objName)) {
        *outType = 19;
        if (outModel) *outModel = legoGlobs_contLaserShot;
    } else if (!_strcmpi(
                   // STRING: LEGORR 0x004a3f08
                   "Freezer", objName)) {
        *outType = 17;
        if (outModel) *outModel = legoGlobs_contFreezer;
    } else if (!_strcmpi(
                   // STRING: LEGORR 0x004a3fc4
                   "Dynamite", objName)) {
        *outType = 8;
        if (outModel) *outModel = legoGlobs_contDynamite;
    } else if (!_strcmpi(
                   // STRING: LEGORR 0x004a3f90
                   "ElectricFence", objName)) {
        *outType = 11;
        if (outModel) *outModel = legoGlobs_contElectricFence;
    } else if (!_strcmpi("SpiderWeb", objName)) {
        *outType = 12;
        if (outModel) *outModel = legoGlobs_contSpiderWeb;
    } else if (!_strcmpi(
                   // STRING: LEGORR 0x004a3fb0
                   "Barrier", objName)) {
        *outType = 9;
        if (outModel) *outModel = legoGlobs_contBarrier;
    } else if (!_strcmpi(
                   // STRING: LEGORR 0x004a3fb8
                   "OohScary", objName)) {
        *outType = 13;
        if (outModel) *outModel = legoGlobs_contOohScary;
    } else if (!_strcmpi("Path", objName)) {
        *outType = 15;
        if (outModel) *outModel = NULL;
    } else {
        for (i = 0; i < legoGlobs_catalogCounts[0]; i++) {
            if (!_strcmpi(legoGlobs_vehicleName[i], objName)) {
                *outType = 1;
                *outID = i;
                if (outModel) *outModel = &legoGlobs_vehicleData[i];
                return 1;
            }
        }
        for (i = 0; i < legoGlobs_catalogCounts[1]; i++) {
            if (!_strcmpi(legoGlobs_miniFigureName[i], objName)) {
                *outType = 2;
                *outID = i;
                if (outModel) *outModel = &legoGlobs_miniFigureData[i];
                return 1;
            }
        }
        for (i = 0; i < legoGlobs_catalogCounts[3]; i++) {
            if (!_strcmpi(legoGlobs_buildingName[i], objName)) {
                *outType = 4;
                *outID = i;
                if (outModel) *outModel = &legoGlobs_buildingData[i];
                return 1;
            }
        }
        return 0;
    }
    return 1;
}

/* EntityPointer: Lego_GetObjectTypeModel. The model of an object type and
 * index: its type data record or its MiscObjects container. */
// FUNCTION: LEGORR 0x0042ed70
int lego_entity_at(int objType, unsigned int objID, void** outModel)
{
    if (objType == -1) {
        *outModel = NULL;
        return 1;
    }
    if (objType == 6) {
        *outModel = legoGlobs_contCrystal;
        return 1;
    }
    if (objType == 7) {
        *outModel = (&legoGlobs_contOresTable)[objID];
        return 1;
    }
    if (objType == 8) {
        *outModel = legoGlobs_contDynamite;
        return 1;
    }
    if (objType == 9) {
        *outModel = legoGlobs_contBarrier;
        return 1;
    }
    if (objType == 11) {
        *outModel = legoGlobs_contElectricFence;
        return 1;
    }
    if (objType == 12) {
        *outModel = legoGlobs_contSpiderWeb;
        return 1;
    }
    if (objType == 13) {
        *outModel = legoGlobs_contOohScary;
        return 1;
    }
    if (objType == 15) {
        *outModel = NULL;
        return 1;
    }
    if (objType == 5) {
        *outModel = legoGlobs_contBoulder;
        return 1;
    }
    if (objType == 1) {
        *outModel = &legoGlobs_vehicleData[objID];
        return 1;
    }
    if (objType == 2) {
        *outModel = &legoGlobs_miniFigureData[objID];
        return 1;
    }
    if (objType == 3) {
        *outModel = &legoGlobs_rockMonsterData[objID];
        return 1;
    }
    if (objType == 4) {
        *outModel = &legoGlobs_buildingData[objID];
        return 1;
    }
    if (objType == 16) {
        *outModel = legoGlobs_contPusher;
        return 1;
    }
    if (objType == 19) {
        *outModel = legoGlobs_contLaserShot;
        return 1;
    }
    if (objType == 17) {
        *outModel = legoGlobs_contFreezer;
        return 1;
    }
    return 0;
}

/* StatsLevelRecordCountForStatType: Lego_GetObjectTypeIDCount. How many
 * indices an object type has. Like the original, an unknown type returns
 * no value (eax holds the switch index). */
// FUNCTION: LEGORR 0x0042ef40
unsigned int lego_stats_level_record_count_for_stat_type(int objType)
{
    switch (objType) {
    case 2:
        return legoGlobs_catalogCounts[1];
    case 1:
        return legoGlobs_catalogCounts[0];
    case 3:
        return legoGlobs_catalogCounts[2];
    case 4:
        return legoGlobs_catalogCounts[3];
    case 10:
        return legoGlobs_catalogCounts[4];
    case 7:
        return 2;
    case 5:
    case 6:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        return 1;
    }
}


/* ---- part 2 functions, group 2D: end ---- */

/* ---- part 2 functions, group 2E (0042efc0-0042f6f0): begin ---- */

/* Lego_PlayMovie_old: play a movie, at screenPt (its own size) or full
 * screen, until it ends. */
// FUNCTION: LEGORR 0x0042efc0
void lego_play_movie(const char* fName, const float* screenPt)
{
    struct G98CMovie* mov;
    int noScreenPt;
    LegoRect2E rect;

    if (fName) {
        mov = movie_load(fName);
        if (mov) {
            if (screenPt) {
                unsigned int width, height;

                movie_get_size(mov, &width, &height);
                rect.left = (long)screenPt[0];
                rect.top = (long)screenPt[1];
                rect.right = (long)(width + screenPt[0]);
                rect.bottom = (long)(height + screenPt[1]);
                noScreenPt = 0;
            } else noScreenPt = 1;
            if (!noScreenPt) directdraw_return_front_buffer();
            while (movie_update(mov, 1.0f,
                                noScreenPt ? NULL : (struct tagRECT*)&rect)) {
                main_loop_frame_pump(noScreenPt);
            }
            movie_free(mov);
        }
    }
}

/* Level_Free: free the current level and reset the level state; returns
 * the level's NextLevel. */
// FUNCTION: LEGORR 0x0042f0c0
const char* lego_level_free(void)
{
    Lego_Level* level = legoGlobs_currLevel;
    const char* nextLevel = NULL;

    if (level) {
        free(LEGO_LEVEL_PTR(void, level->emergeMap));
        if (legoGlobs_endGameAVI1) free(legoGlobs_endGameAVI1);
        if (legoGlobs_endGameAVI2) free(legoGlobs_endGameAVI2);
        free(LEGO_LEVEL_PTR(char, level->fullName));

        legoGlobs_flags1 &= ~0x00200000;
        legoGlobs_flags1 &= ~0x08000000;
        s_entity_state_flags &= ~0x40;
        s_panel_highlight_flags &= ~0x2;
        s_panel_highlight_flags &= ~0x2000;
        legoGlobs_f_556158 = 0;
        legoGlobs_f_556514 = 0;
        s_lego_f_4e8448 = NULL;
        s_object_f_4eabc4 = 0;
        s_object_f_4eabc8 = 0;
        lego_panel_selection_highlight_toggle_adapter(0);
        legoGlobs_flags1 &= ~0x10000000;
        s_panel_highlight_flags &= ~0x4;
        s_panel_highlight_flags &= ~0x40;
        s_panel_highlight_flags &= ~0x80;
        s_panel_highlight_flags &= ~0x4000;
        s_cell_refresh_track_count = 0;
        s_cell_previous_track_count = 0;
        entity_fields_a0_a4_store_a8_clear_original(s_zoom_step_context, 0.0f, 0.0f);
        camera_distance_set(s_zoom_step_context, 200.0f);
        transient_effect_registry_release_all();
        global_state_low_byte_clear_and_refresh();

        nextLevel = LEGO_LEVEL_PTR(char, level->nextLevel);

        ui_message_overlay_state_initialize();
        selection_state_flag_clear_bit0();
        grid_record_flag_mask_clear();
        lego_object_pool_lists_free_all_adapter();
        cmd_flush_all(0);
        spawn_coordinate_pool_clear();
        path_record_release_all();
        ledger_stub_0040bdc0_SetTimeoutTo10000();
        cmd_flush_all(1);
        info_enable(0);
        graphics_overlay_resources_release();
        screen_effect_record_set_original(legoGlobs_cameraTrack, NULL, 0.0f, 0.0f, 0.0f, 0.0f);
        record_fields_4_and_14_set(legoGlobs_cameraFP, 0, 0);
        selection_active_count_reset();
        tutorial_display_resources_release();
        ui_render_record_free((void*)(uintptr_t)level->f_084);
        terrain_map_remove(LEGO_LEVEL_PTR(struct TerrainMap, level->map));
        lego_rectangular_resource_table_destroy(level);
        terrain_cell_resource_grid_dispose(LEGO_LEVEL_PTR(struct TerrainCellResourceGrid, level->textureSet));
        lego_block_activity_remove_all(level);
        scene_resource_singleton_pool_release();
        free(LEGO_LEVEL_PTR(Lego_Block, level->blocks));
        free(level);
        pending_record_release_all();
        interface_reset();
        lego_game_speed_set(1.0f);
        legacy_panel_category_interaction_state_reset();
        advisor_animation_end_cleanup();
        sound_subsystem_stop_all();
        screen_effect_flags_clear_and_release();
    }
    g_active_level_retail = NULL;
    return nextLevel;
}

/* TerrainCellEdgeFlagSetAndUpdate: set or clear a block's 0x200 flag and
 * update the block's surface. */
// FUNCTION: LEGORR 0x0042f2e0
void lego_terrain_cell_edge_flag_set_and_update(Lego_Level* level, int bx, int by, int on)
{
    if (on) {
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags1 |= 0x200;
        lego_cell_init_pass(level, bx, by, 0);
    } else {
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags1 &= ~0x200;
        lego_cell_init_pass(level, bx, by, 0);
    }
}

/* TerrainNeighborCoordinateCheck: the texture (and its direction) of a
 * block from which of its four neighbours are walls; powered selects the
 * second texture set. */
// FUNCTION: LEGORR 0x0042f350
unsigned char lego_terrain_neighbor_coordinate_check(int bx, int by, unsigned char* direction, int powered)
{
    int dirs[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    unsigned int mask = 0;
    unsigned int i;
    int x, y;
    unsigned int solid;
    unsigned char texture;

    for (i = 0; i < 4; i++) {
        x = bx + dirs[i][0];
        y = by + dirs[i][1];
        if (x >= 0 && y >= 0 && x < (int)legoGlobs_currLevel->width && y < (int)legoGlobs_currLevel->height)
            solid = LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[y * legoGlobs_currLevel->width + x].flags1 & 0x20000000;
        else
            solid = 0;
        if (solid || (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * y + x].flags1 & 0x400))
            mask |= 1 << i;
    }

    if (powered) {
        switch (mask) {
        case 15: *direction = 0; texture = 0x71; break;
        case 5: *direction = 0; texture = 0x72; break;
        case 10: *direction = 1; texture = 0x72; break;
        case 6: *direction = 0; texture = 0x73; break;
        case 12: *direction = 1; texture = 0x73; break;
        case 9: *direction = 2; texture = 0x73; break;
        case 3: *direction = 3; texture = 0x73; break;
        case 7: *direction = 0; texture = 0x74; break;
        case 14: *direction = 1; texture = 0x74; break;
        case 13: *direction = 2; texture = 0x74; break;
        case 11: *direction = 3; texture = 0x74; break;
        case 1: *direction = 2; texture = 0x75; break;
        case 2: *direction = 3; texture = 0x75; break;
        case 4: *direction = 0; texture = 0x75; break;
        case 8: *direction = 1; texture = 0x75; break;
        case 0: *direction = 0; texture = 0x75; break;
        }
    } else {
        switch (mask) {
        case 15: *direction = 0; texture = 0x60; break;
        case 5: *direction = 0; texture = 0x62; break;
        case 10: *direction = 1; texture = 0x62; break;
        case 6: *direction = 0; texture = 0x63; break;
        case 12: *direction = 1; texture = 0x63; break;
        case 9: *direction = 2; texture = 0x63; break;
        case 3: *direction = 3; texture = 0x63; break;
        case 7: *direction = 0; texture = 0x64; break;
        case 14: *direction = 1; texture = 0x64; break;
        case 13: *direction = 2; texture = 0x64; break;
        case 11: *direction = 3; texture = 0x64; break;
        case 1: *direction = 2; texture = 0x65; break;
        case 2: *direction = 3; texture = 0x65; break;
        case 4: *direction = 0; texture = 0x65; break;
        case 8: *direction = 1; texture = 0x65; break;
        case 0: *direction = 0; texture = 0x65; break;
        }
    }
    return texture;
}

/* The current level's block at (x, y). */
#define LEGO_2E_CURR_BLOCK(x, y) \
    (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * (y) + (x)])

/* CellInitPass (Level_BlockUpdateSurface): choose a block's surface
 * texture and direction from its walls and flags, and update the map. */
// FUNCTION: LEGORR 0x0042f6f0
void lego_cell_init_pass(Lego_Level* level, unsigned int bx, unsigned int by, int reserved)
{
    Lego_Block* block;
    struct TerrainMap* map;
    unsigned int wallCount;
    unsigned char notWall[4];
    unsigned char* notWallPtr;
    unsigned char wallDir;
    unsigned int i;
    unsigned int origFlags;
    unsigned int flags;
    int fade;
    float height1, height2, diag;
    float dummy;

    if (bx >= level->width - 1) return;
    if (by >= level->height - 1) return;

    {
    int dirs[4][2];

    map = LEGO_LEVEL_PTR(struct TerrainMap, level->map);
    wallCount = 0;
    block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];
    /* Assigned after the block lookup, as the original schedules them. */
    dirs[0][0] = 0; dirs[0][1] = 0; dirs[1][0] = 1; dirs[1][1] = 0;
    dirs[2][0] = 1; dirs[2][1] = 1; dirs[3][0] = 0; dirs[3][1] = 1;
    fade = 1;

    if ((block->flags1 & 0x20000000) && !(block->flags2 & 0x400)) {
        entity_task_counter_increment_by_branch(1);
        block->flags2 |= 0x400;
    } else if (!(block->flags1 & 0x20000000) && (block->flags2 & 0x400)) {
        entity_task_counter_increment_by_branch(0);
        block->flags2 &= ~0x400;
    }

    notWallPtr = notWall;
    for (i = 0; i < 4; i++) {
        if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[(by + dirs[i][1]) * level->width + bx + dirs[i][0]].f_00 == 1) {
            wallDir = (unsigned char)i;
            wallCount++;
        } else {
            *notWallPtr++ = (unsigned char)i;
        }
    }
    }

    origFlags = block->flags1 & 0x8;
    flags = block->flags1 & ~0x20d8;
    block->flags1 = flags;

    if (wallCount == 4) {
        int blockPos[2];

        blockPos[0] = bx;
        blockPos[1] = by;
        block->flags1 = (flags & ~0x20) | 0x0c;
        block->f_03 = 0;
        if (!origFlags) {
            int pos[2];

            pos[0] = bx;
            pos[1] = by;
            terrain_cell_refresh_dispatch_current(blockPos, 0);
            lego_terrain_neighbor_flag_refresh(level, pos, 1);
            if (legoGlobs_flags1 & 0x800000)
                cmd_produce(0xe, NULL, LEGO_2E_CURR_BLOCK(bx, by).f_03, pos);
        }
        if (!(block->flags1 & 0x400000)) {
            if (!(LEGO_2E_CURR_BLOCK(blockPos[0], blockPos[1]).flags1 & 0x400) &&
                !(LEGO_2E_CURR_BLOCK(blockPos[0], blockPos[1]).flags2 & 0x4))
            if (!(LEGO_2E_CURR_BLOCK(blockPos[0], blockPos[1]).flags1 & 0x8000) &&
                !terrain_cell_predicate_check_current((const uint32_t*)blockPos)) {
                LEGO_2E_CURR_BLOCK(blockPos[0], blockPos[1]).flags1 &= ~0x200000;
                LEGO_2E_CURR_BLOCK(blockPos[0], blockPos[1]).flags1 &= ~0x20000000;
                LEGO_2E_CURR_BLOCK(blockPos[0], blockPos[1]).flags1 &= ~0x40000000;
                entity_activity_scene_state_update_retail();
                coordinate_pair_callback_dispatch(blockPos);
                lego_noop_point_adapter(blockPos);
                for (i = 0; i < 4; i++) coordinate_pair_event_queue(blockPos, (void*)0x28);
                LEGO_2E_CURR_BLOCK(blockPos[0], blockPos[1]).flags1 |= 0x3;
            }
            LEGO_2E_CURR_BLOCK(blockPos[0], blockPos[1]).flags1 |= 0x400000;
            selection_mode7_cancel_at_point((const uint32_t*)blockPos);
        }

        if (electric_fence_stud_fits((struct ElectricFenceLevel*)level, bx, by))
            electric_fence_stud_set((struct ElectricFenceLevel*)level, bx, by, 1);
        else
            electric_fence_stud_set((struct ElectricFenceLevel*)level, bx, by, 0);

        if (block->f_02 == 6 || (block->flags1 & 0x80000000)) {
            lava_erosion_pending_cell_add(blockPos, 1);
            if (block->flags1 & 0x200) {
                terrain_map_block_uv_wobble_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by, 0);
                block->texture = 0x47;
                if (block->flags1 & 0x4000) transient_effect_enable_set(LEGO_FIELD(block, void*, 0x1c), 1);
            } else if (block->f_07 == 4) {
                block->texture = 0x46;
                terrain_map_block_uv_wobble_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by, 1);
                if (block->flags1 & 0x4000) {
                    transient_effect_enable_set(LEGO_FIELD(block, void*, 0x1c), 0);
                } else if (LEGO_FIELD(block, short, 0x20) % 3 == 0) {
                    float dirVec[3];

                    dirVec[0] = 0.1f;
                    dirVec[1] = 0.1f;
                    dirVec[2] = -1.0f;
                    /* The port types the ninth argument as a word: 0.3f. */
                    LEGO_FIELD(block, void*, 0x1c) =
                        transient_effect_emitter_create(NULL, bx, by, (int*)dirVec, 0.6f, 0.4f, 0.0f, 1.0f,
                                                        0x3e99999a, 0, LEGO_FIELD(block, short, 0x20), 0x26);
                    block->flags1 |= 0x4000;
                }
            } else {
                terrain_map_block_uv_wobble_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by, 0);
                switch (block->f_07) {
                case 0: block->texture = 0x06; break;
                case 1: block->texture = 0x16; break;
                case 2: block->texture = 0x26; break;
                case 3: block->texture = 0x36; break;
                }
            }
        } else if (block->f_02 == 9) {
            block->texture = 0x45;
            terrain_map_block_uv_wobble_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by, 1);
        } else if (block->flags1 & 0x100000) {
            if (block->flags2 & 0x100) block->texture = 0x66;
            else block->texture = 0x76;
        } else if (block->flags1 & 0x20000000) {
            block->texture = lego_terrain_neighbor_coordinate_check(bx, by, &block->f_03, block->flags2 & 0x100);
        } else if (block->flags2 & 0x20) {
            block->texture = 0x30;
        } else if (!(block->flags1 & 0x200000)) {
            switch (block->flags1 & 0x3) {
            case 0: block->texture = 0x13; break;
            case 1: block->texture = 0x12; break;
            case 2: block->texture = 0x11; break;
            case 3: block->texture = 0x10; break;
            }
        } else if (block->flags1 & 0x40000000) {
            block->texture = 0x61;
        } else if (block->flags1 & 0x20) {
            block->texture = 0x00;
        } else {
            block->texture = 0x00;
        }
    } else if (wallCount == 2) {
        unsigned int pairs[6][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 0, 3 }, { 0, 2 }, { 1, 3 } };
        unsigned char dir;

        flags |= 0x14;
        block->flags1 = flags;
        for (i = 0; i < 6; i++) {
            if (pairs[i][0] == notWall[0] && pairs[i][1] == notWall[1]) {
                dir = (unsigned char)i;
                break;
            }
        }
        if (dir < 4) {
            if (flags & 0x20) {
                switch (block->f_02) {
                case 2: block->texture = 0x24; break;
                case 3: block->texture = 0x23; break;
                case 4: block->texture = 0x22; break;
                case 5: block->texture = 0x21; break;
                case 1: case 7: block->texture = 0x25; break;
                }
                block->f_03 = dir;
            } else if (LEGO_FIELD(block, short, 0x20) % 10) {
                switch (block->f_02) {
                case 2: block->texture = 0x04; break;
                case 3: block->texture = 0x03; break;
                case 4: block->texture = 0x02; break;
                case 5: block->texture = 0x01; break;
                case 1: case 7: block->texture = 0x05; break;
                case 8: block->texture = 0x40; break;
                case 10: block->texture = 0x20; break;
                case 11: block->texture = 0x67; break;
                }
                block->f_03 = dir;
            } else {
                switch (block->f_02) {
                case 2: block->texture = 0x04; break;
                case 3: block->texture = 0x03; break;
                case 4: block->texture = 0x02; break;
                case 5: block->texture = 0x01; break;
                case 1: case 7: block->texture = 0x05; break;
                case 8: block->texture = 0x40; break;
                case 10: block->texture = 0x20; break;
                case 11: block->texture = 0x67; break;
                }
                block->f_03 = dir;
            }
        } else {
            switch (block->f_02) {
            case 1: block->texture = 0x77; break;
            case 2: block->texture = 0x77; break;
            case 3: block->texture = 0x77; break;
            case 4: block->texture = 0x77; break;
            case 5: block->texture = 0x77; break;
            }
            block->flags1 = flags | 0x2000;
            block->f_03 = dir - 4;
            terrain_map_block_rotated_set(map, bx, by, block->f_03 % 2);
        }
    } else if (wallCount == 1 || wallCount == 3) {
        flags = (flags & ~0x20) | 0x14;
        block->flags1 = flags;
        if (wallCount == 1) {
            int blockPos[2];

            if (flags & 0x20) {
                switch (block->f_02) {
                case 2: block->texture = 0x34; break;
                case 3: block->texture = 0x33; break;
                case 4: block->texture = 0x32; break;
                case 5: block->texture = 0x31; break;
                case 1: case 7: block->texture = 0x35; break;
                }
            } else {
                switch (block->f_02) {
                case 2: block->texture = 0x34; break;
                case 3: block->texture = 0x33; break;
                case 4: block->texture = 0x32; break;
                case 5: block->texture = 0x31; break;
                case 1: case 7: block->texture = 0x35; break;
                }
            }
            block->f_03 = (wallDir + 2) % 4;
            block->flags1 = flags | 0x40;
            blockPos[0] = bx;
            blockPos[1] = by;
            lego_terrain_neighbor_flag_refresh(level, blockPos, 1);
        } else if (wallCount == 3) {
            int pos[2];

            if (flags & 0x20) {
                switch (block->f_02) {
                case 2: block->texture = 0x54; break;
                case 3: block->texture = 0x53; break;
                case 4: block->texture = 0x52; break;
                case 5: block->texture = 0x51; break;
                case 1: case 7: block->texture = 0x55; break;
                }
            } else {
                switch (block->f_02) {
                case 2: block->texture = 0x54; break;
                case 3: block->texture = 0x53; break;
                case 4: block->texture = 0x52; break;
                case 5: block->texture = 0x51; break;
                case 1: case 7: block->texture = 0x55; break;
                }
            }
            pos[0] = bx;
            pos[1] = by;
            lego_terrain_neighbor_flag_refresh(level, pos, 1);
            coordinate_callback_dispatch_current(pos);
            block->f_03 = notWall[0];
            block->flags1 |= 0x80;
        }

        {
        int blockPos[2];

        terrain_map_vertex_position_get(map, bx, by, &dummy, &dummy, &height1);
        terrain_map_vertex_position_get(map, bx + 1, by + 1, &dummy, &dummy, &height2);
        diag = height1 - height2;
        terrain_map_vertex_position_get(map, bx + 1, by, &dummy, &dummy, &height1);
        terrain_map_vertex_position_get(map, bx, by + 1, &dummy, &dummy, &height2);
        if (fabs(height1 - height2) < fabs(diag))
            terrain_map_block_rotated_set(map, bx, by, 1);
        else
            terrain_map_block_rotated_set(map, bx, by, 0);
        blockPos[0] = bx;
        blockPos[1] = by;
        lego_terrain_neighbor_flag_refresh(level, blockPos, 1);
        }
    } else {
        if (flags & 0x4) {
            switch (block->f_02) {
            case 1: block->texture = 0x70; break;
            case 2: block->texture = 0x70; break;
            case 3: block->texture = 0x70; break;
            case 4: block->texture = 0x70; break;
            case 5: block->texture = 0x70; break;
            }
        } else {
            block->texture = 0x70;
        }
        block->f_03 = 0;
    }

    if ((block->flags1 & 0x4) && (block->flags1 & 0x800000) &&
        (block->f_02 == 1 || block->f_02 == 7 || (block->flags1 & 0x8))) {
        block->flags1 &= ~0x800000;
        terrain_map_block_highlight_clear(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by);
    }

    if (!(render_feature_flags_get() & 0x8000)) fade = 0;
    if (fade)
        terrain_map_block_texture_fade_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by,
                                           block->texture, block->f_03);
    else
        terrain_map_block_texture_set(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by,
                                      block->texture, block->f_03);

    if (level->useRoof) terrain_cell_mutation_progress((struct LevelState*)level, bx, by);
}

/* ---- part 2 functions, group 2E: end ---- */

/* ---- part 2 functions, group 2F (004302b0-00431570): begin ---- */

/* The three shared entries 004302b0, 00430320 and 00431560-00431570 are the
 * original's code only on MSVC x86: elsewhere Lego_Level's pointer words are
 * not live pointers (LEGO_LEVEL_PTR) and the port prefers its GameContext's
 * level, so lego.cpp keeps their port models for the other targets. Their
 * callers already use these entries' contracts. */
#if defined(_MSC_VER) && defined(_M_IX86)

/* Clears the block's two tracking flags and refreshes the objects and
 * routes around it. Inline in the original as well: 00430320 and 00430530
 * repeat its body. */
// FUNCTION: LEGORR 0x004302b0
extern __inline void map_cell_tracking_flags_clear_and_refresh_current(const int cell_xy[2])
{
    LEGO_2F_BLOCK(legoGlobs_currLevel, cell_xy[0], cell_xy[1]).flags1 &= ~0x20000000;
    LEGO_2F_BLOCK(legoGlobs_currLevel, cell_xy[0], cell_xy[1]).flags1 &= ~0x40000000;
    entity_activity_scene_state_update_retail();
    coordinate_pair_callback_dispatch(cell_xy);
    ((void (*)(const int*))flic_opcode27_fallback_noop)(cell_xy);
}

/* Clears a block's pending work once nothing occupies or reserves it, and
 * queues four of the given event for it. The original computes the block
 * address once for its first two tests (flags1 & 0x400, flags2 & 4) and
 * again, re-reading the level's width and blocks, for flags1 & 0x8000;
 * MSVC 5.0 merges the three tests of every C form tried (one && chain, an
 * if-return, nested ifs, a block pointer, inline helpers taking the level,
 * the block coordinates or the pair, operand orders, value copies and a
 * second index form), giving 70.66-74.42%. As for 00429160, the matching
 * build keeps the port's instruction-for-instruction body. 00430530 inlines
 * the same code. */
// FUNCTION: LEGORR 0x00430320
__declspec(naked) void terrain_cell_work_flag_clear_current(
    const int coord[2], uint32_t selector)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push ebx
        push esi
        mov esi, dword ptr [esp + 0ch]
        mov eax, dword ptr [ecx + 14h]
        mov ebx, dword ptr [ecx + 0b8h]
        mov edx, dword ptr [esi + 4]
        push edi
        imul eax, edx
        mov edi, dword ptr [esi]
        add eax, edi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ebx + eax*8]
        mov ebx, dword ptr [eax + 8]
        test bh, 4
        jne terrain_cell_work_flag_clear_done
        test byte ptr [eax + 0ch], 4
        jne terrain_cell_work_flag_clear_done
        mov eax, dword ptr [ecx + 14h]
        imul eax, edx
        add eax, edi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        test ch, 80h
        jne terrain_cell_work_flag_clear_done
        push esi
        call terrain_cell_predicate_check_current
        add esp, 4
        test eax, eax
        jne terrain_cell_work_flag_clear_done
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 0ffdfffffh
        mov dword ptr [eax], ecx
        mov ecx, dword ptr [g_active_level_retail]
        mov ebx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, ebx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov edi, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and edi, 0dfffffffh
        mov dword ptr [eax], edi
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ebx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ebx, 0bfffffffh
        mov dword ptr [eax], ebx
        call entity_activity_scene_state_update_retail
        push esi
        call coordinate_pair_callback_dispatch
        add esp, 4
        push esi
        call flic_opcode27_fallback_noop
        mov ebx, dword ptr [esp + 18h]
        add esp, 4
        mov edi, 4
    terrain_cell_work_flag_clear_queue_loop:
        push ebx
        push esi
        call coordinate_pair_event_queue
        add esp, 8
        dec edi
        jne terrain_cell_work_flag_clear_queue_loop
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        or ecx, 3
        mov dword ptr [eax], ecx
    terrain_cell_work_flag_clear_done:
        pop edi
        pop esi
        pop ebx
        ret
    }
}

#endif /* _MSC_VER && _M_IX86 */

// FUNCTION: LEGORR 0x00430470
void lego_terrain_neighbor_flag_set(Lego_Level* level, int unused,
                                    unsigned int bx, unsigned int by)
{
    if (bx > 0 && by > 0 && bx < level->width - 2 && by < level->height - 2) {
        if (LEGO_2F_BLOCK(level, bx, by).flags1 & 0x8) {
            if (world_spatial_record_find_at_grid_coordinate(bx, by, NULL)) {
                LEGO_2F_BLOCK(level, bx, by).flags1 &= ~0x8;
                LEGO_2F_BLOCK(level, bx, by).flags1 |= 0x10;
                lego_cell_init_pass(level, bx, by, 0);
                terrain_cell_surface_quad_reset_current(bx, by);
            }
        }
    }
}

/* Level_DestroyWall: digs the wall at (bx, by). Ore and crystal seams take
 * four hits; isHiddenCavern is set when a cavern is uncovered. */
// FUNCTION: LEGORR 0x00430530
int lego_terrain_cell_mutate(Lego_Level* level, unsigned int bx,
                             unsigned int by, int isHiddenCavern)
{
    struct TerrainMap* map = LEGO_LEVEL_PTR(struct TerrainMap, level->map);
    int quad[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
    int pos[2];
    int pos2[2];
    unsigned int x, y;
    int i;

    if (bx > 0 && by > 0 && bx < level->width - 2 && by < level->height - 2 &&
        !(LEGO_2F_BLOCK(level, bx, by).flags1 & 0x8)) {
        if (LEGO_2F_BLOCK(level, bx, by).f_02 != 8 &&
            LEGO_2F_BLOCK(level, bx, by).f_02 != 10) {
            if (LEGO_2F_BLOCK(level, bx, by).f_00 == 0) {
                terrain_map_vertex_move(map, bx, by, level->digDepth);
                LEGO_2F_BLOCK(level, bx, by).f_00 = 1;
            }
            if (LEGO_2F_BLOCK(level, bx + 1, by).f_00 == 0) {
                terrain_map_vertex_move(map, bx + 1, by, level->digDepth);
                LEGO_2F_BLOCK(level, bx + 1, by).f_00 = 1;
            }
            if (LEGO_2F_BLOCK(level, bx + 1, by + 1).f_00 == 0) {
                terrain_map_vertex_move(map, bx + 1, by + 1, level->digDepth);
                LEGO_2F_BLOCK(level, bx + 1, by + 1).f_00 = 1;
            }
            if (LEGO_2F_BLOCK(level, bx, by + 1).f_00 == 0) {
                terrain_map_vertex_move(map, bx, by + 1, level->digDepth);
                LEGO_2F_BLOCK(level, bx, by + 1).f_00 = 1;
            }

            {
                int around[8][2] = {
                    { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 },
                    { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
                };

                for (i = 0; i < 8; i++) {
                    if (!(LEGO_2F_BLOCK(level, bx + around[i][0], by + around[i][1]).flags1 & 0x8)) {
                        LEGO_2F_BLOCK(level, bx + around[i][0], by + around[i][1]).flags1 |= 0x10;
                        LEGO_2F_BLOCK(level, bx + around[i][0], by + around[i][1]).flags1 &= ~0x40000;
                    }
                }
            }

            LEGO_2F_BLOCK(level, bx, by).flags1 |= 0x8;
            pos[0] = bx;
            pos[1] = by;
            lego_terrain_neighbor_flag_refresh(level, pos, 1);
            LEGO_2F_BLOCK(level, bx, by).flags1 &= ~0x20;
            LEGO_2F_BLOCK(level, bx, by).flags1 |= 0x400000;

            if ((!(LEGO_2F_BLOCK(level, bx, by).flags1 & 0x4000000) ||
                 (LEGO_2F_BLOCK(level, bx, by).flags1 & 0x20000)) &&
                (LEGO_2F_BLOCK(level, bx, by).flags1 & 0x800000)) {
                global_float_at_00554adc_add_one();
            }

            if (isHiddenCavern) {
                LEGO_2F_BLOCK(level, bx, by).flags1 |= 0x200000;
            } else {
                pos[0] = bx;
                pos[1] = by;
                for (y = by - 1; y <= by + 1; y++) {
                    for (x = bx - 1; x <= bx + 1; x++) {
                        if (LEGO_2F_BLOCK(level, x, y).flags1 & 0x20000) {
                            lego_terrain_neighbor_work_flag_scan(x, y);
                            if (!system_state_flags_get_original(NULL)) {
                                info_send(9, NULL, NULL, (struct InfoPoint2I*)pos);
                                advisor_text_animation_update(0x12, 1, 0);
                                global_float_at_00554efc_add_one();
                            }
                        }
                    }
                }

                LEGO_2F_BLOCK(legoGlobs_currLevel, bx, by).flags1 |= 0x800;
                cmd_produce(0xe, NULL, LEGO_2F_BLOCK(legoGlobs_currLevel, bx, by).f_03, pos);
                terrain_cell_surface_quad_height_fill_current((struct LevelState*)level, bx, by);
                pos2[0] = bx;
                pos2[1] = by;
                /* TerrainCellWorkFlagClear (00430320), inline. */
                if (!(LEGO_2F_BLOCK(legoGlobs_currLevel, pos2[0], pos2[1]).flags1 & 0x400) &&
                    !(LEGO_2F_BLOCK(legoGlobs_currLevel, pos2[0], pos2[1]).flags2 & 0x4) &&
                    !(LEGO_2F_BLOCK(legoGlobs_currLevel, pos2[0], pos2[1]).flags1 & 0x8000) &&
                    !terrain_cell_predicate_check_current((const uint32_t*)pos2)) {
                    LEGO_2F_BLOCK(legoGlobs_currLevel, pos2[0], pos2[1]).flags1 &= ~0x200000;
                    map_cell_tracking_flags_clear_and_refresh_current(pos2);
                    for (i = 0; i < 4; i++)
                        coordinate_pair_event_queue(pos2, (void*)0x28);
                    LEGO_2F_BLOCK(legoGlobs_currLevel, pos2[0], pos2[1]).flags1 |= 0x3;
                }
                selection_mode7_cancel_at_point((const uint32_t*)pos2);
            }

            pos[0] = bx;
            pos[1] = by;
            terrain_cell_refresh_dispatch_current(pos, 0);
            for (y = by - 1; y < by + 2; y++) {
                for (x = bx - 1; x < bx + 2; x++)
                    lego_cell_init_pass(level, x, y, isHiddenCavern);
            }
            LEGO_2F_BLOCK(level, bx, by).flags1 &= ~0x800000;
            for (y = by - 1; y <= by + 2; y++) {
                for (x = bx - 1; x <= bx + 2; x++)
                    terrain_map_block_vertex_modified(map, x, y);
            }

            if (!isHiddenCavern && (s_panel_highlight_flags & 0x80000)) {
                short seed = LEGO_FIELD(&LEGO_2F_BLOCK(level, bx, by), short, 0x20);
                if (seed % 4 == 0)
                    ledger_stub_0044b140_SmallSpiderCellSwarmSpawn(bx, by, seed % 1353);
            }

            pos2[0] = bx;
            pos2[1] = by;
            deferred_olist_entity_create_and_link(pos2);
            if (LEGO_2F_BLOCK(level, bx, by).flags2 & 0x200) {
                LEGO_2F_BLOCK(level, bx, by).flags2 &= ~0x200;
                LEGO_2F_BLOCK(level, bx, by).flags2 |= 0x20;
                predug_coordinate_pool_append(pos2);
                lego_cell_init_pass(level, bx, by, 0);
            }
            if (!isHiddenCavern) {
                info_send(0x1b, NULL, NULL, (struct InfoPoint2I*)pos2);
                return 1;
            }
            return 1;
        } else {
            pos[0] = bx;
            pos[1] = by;
            LEGO_2F_BLOCK(level, bx, by).f_10[1] = 0;
            for (i = 0; i < 4; i++) {
                if (LEGO_2F_BLOCK(level, bx + quad[i][0], by + quad[i][1]).f_00 == 0) {
                    pos2[0] = bx;
                    pos2[1] = by;
                    LEGO_2F_BLOCK(level, bx + quad[i][0], by + quad[i][1]).f_00 = 5;
                    if (LEGO_2F_BLOCK(level, bx, by).f_02 == 8 ||
                        LEGO_2F_BLOCK(level, bx, by).f_02 == 10) {
                        info_send(0x25, NULL, NULL, (struct InfoPoint2I*)pos2);
                    }
                    if (LEGO_2F_BLOCK(level, bx, by).f_02 == 8)
                        info_send(1, NULL, NULL, (struct InfoPoint2I*)pos2);
                    else if (LEGO_2F_BLOCK(level, bx, by).f_02 == 10)
                        info_send(0x24, NULL, NULL, (struct InfoPoint2I*)pos2);
                }
                if (LEGO_2F_BLOCK(level, bx + quad[i][0], by + quad[i][1]).f_00 != 1) {
                    LEGO_2F_BLOCK(level, bx + quad[i][0], by + quad[i][1]).f_00--;
                    terrain_map_vertex_move(map, bx + quad[i][0], by + quad[i][1],
                                            level->digDepth * 0.25f);
                }
            }
            lego_terrain_neighbor_refresh(pos);
            LEGO_FIELD(&LEGO_2F_BLOCK(level, bx, by), short, 0x22)++;
            if (LEGO_FIELD(&LEGO_2F_BLOCK(level, bx, by), short, 0x22) == 4) {
                LEGO_2F_BLOCK(level, bx, by).f_02 = 5;
                for (i = 0; i < 4; i++)
                    LEGO_2F_BLOCK(level, bx + quad[i][0], by + quad[i][1]).f_00 = 1;
                lego_terrain_cell_mutate(level, bx, by, isHiddenCavern);
                return 1;
            }
            terrain_cell_mutation_progress((struct LevelState*)legoGlobs_currLevel, bx, by);
            terrain_cell_refresh_eight_neighbors_current(pos);
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00430df0
void lego_terrain_neighbor_refresh(const int* blockPos)
{
    int dirs[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    int target[2];
    unsigned int i;

    for (i = 0; i < 4; i++) {
        if ((LEGO_2F_BLOCK(legoGlobs_currLevel, blockPos[0] + dirs[i][0], blockPos[1] + dirs[i][1]).flags1 & 0x8) &&
            LEGO_2F_BLOCK(legoGlobs_currLevel, blockPos[0] + dirs[i][0], blockPos[1] + dirs[i][1]).f_02 != 9 &&
            LEGO_2F_BLOCK(legoGlobs_currLevel, blockPos[0] + dirs[i][0], blockPos[1] + dirs[i][1]).f_02 != 6)
            break;
    }
    if (i != 4) {
        target[0] = blockPos[0] + dirs[i][0];
        target[1] = blockPos[1] + dirs[i][1];
        if (LEGO_2F_BLOCK(legoGlobs_currLevel, blockPos[0], blockPos[1]).f_02 == 8)
            entity_build_placement_alt_setup_original(target, 0, NULL, 1);
        else if (LEGO_2F_BLOCK(legoGlobs_currLevel, blockPos[0], blockPos[1]).f_02 == 10)
            entity_build_placement_setup_original(target, 0, NULL, 1);
    }
}

// FUNCTION: LEGORR 0x00430ee0
int lego_terrain_cell_initialize(Lego_Level* level, unsigned int bx, unsigned int by)
{
    struct TerrainMap* map = LEGO_LEVEL_PTR(struct TerrainMap, level->map);
    LegoPoint2I2F dirs[4] = { { -1, -1 }, { 0, -1 }, { 0, 0 }, { -1, 0 } };
    unsigned int x, y;
    unsigned int i;

    if (bx > 0 && by > 0 && bx < level->width - 2 && by < level->height - 2) {
        if (LEGO_2F_BLOCK(level, bx, by).f_00 == 0) {
            terrain_map_vertex_move(map, bx, by, level->digDepth);
            LEGO_2F_BLOCK(level, bx, by).f_00 = 1;
            for (i = 0; i < 4; i++)
                LEGO_2F_BLOCK(level, bx + dirs[i].x, by + dirs[i].y).flags1 |= 0x40000;
            for (y = by - 1; y <= by; y++) {
                for (x = bx - 1; x <= bx; x++)
                    lego_cell_init_pass(level, x, y, 0);
            }
            for (y = by - 1; y <= by + 1; y++) {
                for (x = bx - 1; x <= bx + 1; x++)
                    terrain_map_block_vertex_modified(map, x, y);
            }

            {
                int pos[2];
                unsigned int count, last, dir;

                dirs[0].x = 0;
                dirs[0].y = -1;
                dirs[1].x = 1;
                dirs[1].y = 0;
                dirs[2].x = 0;
                dirs[2].y = 1;
                dirs[3].x = -1;
                dirs[3].y = 0;
                pos[0] = bx;
                pos[1] = by;
                count = 0;
                for (i = 0; i < 4; i++) {
                    if (LEGO_2F_BLOCK(level, bx + dirs[i].x, by + dirs[i].y).f_00 == 1) {
                        count++;
                        last = i;
                    }
                }
                dir = (last - 2) & 3;
                if (count == 1)
                    cmd_produce(0xe, NULL, dir | 0x10000, pos);
                terrain_cell_refresh_dispatch_current(pos, 1);
            }
        }
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x004310f0
void lego_terrain_cell_flag20_clear_and_refresh(const int* blockPos)
{
    Lego_Level* level = legoGlobs_currLevel;

    lego_terrain_neighbor_flag_refresh(level, blockPos, 0);
    LEGO_2F_BLOCK(level, blockPos[0], blockPos[1]).flags1 &= ~0x20;
    lego_cell_init_pass(level, blockPos[0], blockPos[1], 0);
}

// FUNCTION: LEGORR 0x00431140
void lego_terrain_cell_queued_work_transition(unsigned int bx, unsigned int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    int pos[2];

    if ((LEGO_2F_BLOCK(level, bx, by).flags1 & 0x10) &&
        !(LEGO_2F_BLOCK(level, bx, by).flags1 & 0xe0)) {
        LEGO_2F_BLOCK(level, bx, by).flags1 |= 0x20;
        lego_cell_init_pass(level, bx, by, 0);
        pos[0] = bx;
        pos[1] = by;
        lego_block_activity_create(level, pos, 0);
        coordinate_callback_dispatch_current(pos);
        info_send(0x1c, NULL, NULL, (struct InfoPoint2I*)pos);
    }
}

// FUNCTION: LEGORR 0x004311d0
void lego_block_activity_create(Lego_Level* level, const int* blockPos, int build)
{
    float wx, wy, wz;
    LegoVector3FC dirs[4] = {
        { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }
    };
    unsigned int direction = LEGO_2F_BLOCK(level, blockPos[0], blockPos[1]).f_03;
    LegoBlockActivity2F* activity;

    if (build) {
        void* cont = legoGlobs_contRechargeSparkle;

        if ((activity = LEGO_2F_PTR(LegoBlockActivity2F,
                LEGO_2F_BLOCK(level, blockPos[0], blockPos[1]).f_10[2])) == NULL) {
            activity = (LegoBlockActivity2F*)malloc(sizeof(LegoBlockActivity2F));
            memset(activity, 0, sizeof(LegoBlockActivity2F));
            activity->build = build;
            activity->cont = entity_visual_resource_load(cont);
            if (!activity->build)
                // STRING: LEGORR 0x004a46e8
                scene_action_name_set(activity->cont, "Activity_Build");
            ddiscene_frame_update_and_callbacks(activity->cont, 0.0f);
            activity->bx = blockPos[0];
            activity->by = blockPos[1];
            activity->prev = NULL;
            activity->next = LEGO_2F_PTR(LegoBlockActivity2F, level->f_088);
            if (activity->next)
                activity->next->prev = activity;
            LEGO_2F_PTR_SET(level->f_088, activity);
            LEGO_2F_PTR_SET(LEGO_2F_BLOCK(level, blockPos[0], blockPos[1]).f_10[2], activity);
        }
        terrain_map_block_to_world(LEGO_LEVEL_PTR(struct TerrainMap, level->map),
                                   blockPos[0], blockPos[1], &wx, &wy);
        wz = terrain_map_world_z_get(LEGO_LEVEL_PTR(struct TerrainMap, level->map), wx, wy);
        ddiscene_object_position_set(activity->cont, NULL, wx, wy, wz);
        ddiscene_object_pose_method_dispatch(activity->cont, NULL, (float)dirs[direction].x,
                                             (float)dirs[direction].y, (float)dirs[direction].z,
                                             0.0f, 0.0f, -1.0f);
    }
}

// FUNCTION: LEGORR 0x004313b0
void lego_block_activity_update_all(Lego_Level* level, float elapsedWorld)
{
    LegoBlockActivity2F* activity;
    LegoBlockActivity2F* next;
    float t;

    for (activity = LEGO_2F_PTR(LegoBlockActivity2F, level->f_088); activity != NULL; activity = next) {
        next = activity->next;
        t = ddiscene_advance_by_time_target(activity->cont, elapsedWorld);
        if (!activity->build) {
            if (!(activity->flags & 1) && 0.0f != t) {
                activity->flags |= 1;
                // STRING: LEGORR 0x004a46f8
                scene_action_name_set(activity->cont, "Activity_Stand");
            } else if (activity->flags & 2) {
                lego_block_activity_remove(activity);
            } else if ((activity->flags & 4) && 0.0f != t) {
                activity->flags |= 2;
            }
        }
    }
}

// FUNCTION: LEGORR 0x00431450
void lego_terrain_neighbor_flag_refresh(Lego_Level* level, const int* blockPos, int build)
{
    LegoBlockActivity2F* activity = LEGO_2F_PTR(LegoBlockActivity2F,
        LEGO_2F_BLOCK(level, blockPos[0], blockPos[1]).f_10[2]);

    if (activity != NULL) {
        if (!build && !activity->build) {
            // STRING: LEGORR 0x004a4708
            scene_action_name_set(activity->cont, "Activity_Destroy");
            ddiscene_frame_update_and_callbacks(activity->cont, 0.0f);
            activity->flags |= 4;
        } else {
            activity->flags |= 2;
        }
    }
}

// FUNCTION: LEGORR 0x004314c0
void lego_block_activity_remove(LegoBlockActivity2F* activity)
{
    Lego_Level* level = legoGlobs_currLevel;

    ledger_stub_00472a40_DDiSceneObjectDestroy(activity->cont);
    LEGO_2F_BLOCK(level, activity->bx, activity->by).f_10[2] = 0;
    if (activity->prev)
        activity->prev->next = activity->next;
    else
        LEGO_2F_PTR_SET(level->f_088, activity->next);
    if (activity->next)
        activity->next->prev = activity->prev;
    free(activity);
}

// FUNCTION: LEGORR 0x00431530
void lego_block_activity_remove_all(Lego_Level* level)
{
    LegoBlockActivity2F* activity = LEGO_2F_PTR(LegoBlockActivity2F, level->f_088);
    LegoBlockActivity2F* next;

    while (activity != NULL) {
        next = activity->next;
        lego_block_activity_remove(activity);
        activity = next;
    }
}

#if defined(_MSC_VER) && defined(_M_IX86)

// FUNCTION: LEGORR 0x00431560
struct LevelState* level_current_owner_get(void)
{
    return g_active_level_retail;
}

/* Lego_GetMap. The port's callers pass the two block coordinates the
 * original pushes for the call that consumes the map; it reads neither. */
// FUNCTION: LEGORR 0x00431570
void* level_current_terrain_get(int cell_x, int cell_y)
{
    return LEGO_LEVEL_PTR(void, ((Lego_Level*)level_current_owner_get())->map);
}

#endif /* _MSC_VER && _M_IX86 */

/* ---- part 2 functions, group 2F: end ---- */

/* ---- part 2 functions, group 2G (00431580-00432490): begin ---- */

/* TerrainNeighborWorkFlagScan [group 2G]: a breadth-first sweep from one
 * block. Every block in the 3x3 neighbourhood of a listed position whose
 * flags1 carries 0x20000 is updated (Level_Block mutation, mode 1), loses
 * the flag and is listed for the next pass. Two position lists alternate.
 * The list size is computed after the counts are cleared, as in the
 * original's register use (finding 634). */
// FUNCTION: LEGORR 0x00431580
void lego_terrain_neighbor_work_flag_scan(unsigned int bx, unsigned int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned int listSize;
    unsigned int counts[2];
    LegoPoint2F2G* lists[2];
    unsigned int curr;
    unsigned int i;
    LegoPoint2F2G pos;
    int x, y;

    counts[0] = 0;
    counts[1] = 0;
    listSize = (level->width + level->height) * 2;
    curr = 0;
    lists[0] = (LegoPoint2F2G*)malloc(sizeof(LegoPoint2F2G) * listSize * 2);
    if (lists[0] != NULL) {
        lists[1] = &lists[0][listSize];
        lists[0][0].x = (float)bx;
        lists[0][0].y = (float)by;
        counts[0] = 1;
        do {
            for (i = 0; i < counts[curr]; i++) {
                pos = lists[curr][i];
                for (y = (int)pos.y - 1; y <= (int)pos.y + 1; y++) {
                    for (x = (int)pos.x - 1; x <= (int)pos.x + 1; x++) {
                        if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1 & 0x20000) {
                            lego_terrain_cell_mutate(level, x, y, 1);
                            LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1 &= ~0x20000;
                            if (pos.x != (float)x || pos.y != (float)y) {
                                lists[!curr][counts[!curr]].x = (float)x;
                                lists[!curr][counts[!curr]].y = (float)y;
                                counts[!curr]++;
                            }
                        }
                    }
                }
            }
            counts[curr] = 0;
            curr = !curr;
        } while (counts[curr]);
        free(lists[0]);
    }
}

/* TerrainStampImpactHandler [group 2G]: a rock fall at a block. Counts the
 * solid neighbours (flags1 & 8), plays the fall sound at the block (or the
 * vertex) and spawns the rock-fall effect facing direction; on success the
 * block is marked 0x800. */
// FUNCTION: LEGORR 0x00431780
void lego_terrain_stamp_impact_handle(unsigned int bx, unsigned int by,
                                      int direction, int vertexMode)
{
    unsigned int count = 0;
    unsigned int i;
    float x;
    float y;
    float z;
    LegoVector3F2G sPos;
    int big;

    {
        LegoPoint2F2G offsets[4] = { { 1.0f, 0.0f }, { 0.0f, 1.0f },
                                     { -1.0f, 0.0f }, { 0.0f, -1.0f } };
        for (i = 0; i < 4; i++) {
            offsets[i].x += (float)bx;
            offsets[i].y += (float)by;
            if (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                    [legoGlobs_currLevel->width * (int)offsets[i].y + (int)offsets[i].x].flags1 & 8)
                count++;
        }
    }
    if (vertexMode) {
        terrain_map_vertex_position_get((struct TerrainMap*)LEGO_LEVEL_PTR(void, legoGlobs_currLevel->map),
                                        bx, by, &x, &y, &z);
    } else {
        terrain_map_block_to_world((struct TerrainMap*)LEGO_LEVEL_PTR(void, legoGlobs_currLevel->map),
                                   bx, by, &x, &y);
    }
    sPos.y = y;
    sPos.x = x;
    sPos.z = terrain_map_world_z_get((struct TerrainMap*)LEGO_LEVEL_PTR(void, legoGlobs_currLevel->map),
                                     x, y);
    sound_submission_route_wrapper(NULL, 4, 0, 0, &sPos.x);
    big = (count > 1);
    {
        LegoPoint2F2G dirs[4] = { { 0.0f, 1.0f }, { 1.0f, 0.0f },
                                  { 0.0f, -1.0f }, { -1.0f, 0.0f } };
        if (scene_slot_pose_scale_register((unsigned short)big, bx, by, x, y,
                terrain_map_world_z_get((struct TerrainMap*)LEGO_LEVEL_PTR(void, legoGlobs_currLevel->map),
                                        x, y),
                dirs[direction].x, dirs[direction].y)) {
            LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                [legoGlobs_currLevel->width * by + bx].flags1 |= 0x800;
        }
    }
}

/* TerrainCellMaterialGet [group 2G]: the block's terrain type. */
// FUNCTION: LEGORR 0x004319b0
int lego_terrain_cell_material_get(int bx, int by)
{
    return LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
        [legoGlobs_currLevel->width * by + bx].f_02;
}

/* MapFileCellGet [group 2G]: a cell of a level map file (a Map_* image),
 * row by, column bx; the width comes from the image header. Shared: the
 * port's callers use this contract. */
// FUNCTION: LEGORR 0x004319e0
uint32_t level_map_cell_get(uint32_t slot, int x, int y)
{
    uint16_t* data = (uint16_t*)(level_map_slot_get(slot) + 0x10);
    uint32_t width, height;
    level_map_dimensions_get(slot, &width, &height);
    return data[y * width + x];
}

/* SelectedEntitiesAnyPredicateMatch [group 2G]: does any selected unit pass
 * the walkability query for the block? Shared. */
// FUNCTION: LEGORR 0x00431a30
int selected_entities_any_predicate_match(int cx, int cy, int require_stat_bit)
{
    struct EntitySlot* const* units;
    unsigned int count;
    unsigned int i;

    units = selection_active_context_record_get((int*)&count);
    if (units != NULL) {
        for (i = 0; i < count; i++) {
            if (terrain_cell_walkability_query_current(units[i], cx, cy, require_stat_bit, 1))
                return 1;
        }
    }
    return 0;
}

/* AnyObjectCanOccupyTerrainCell [group 2G]: can any selected unit be placed
 * on the block? */
// FUNCTION: LEGORR 0x00431ab0
int lego_any_object_can_occupy_terrain_cell(int bx, int by)
{
    struct EntitySlot* const* units;
    unsigned int count;
    unsigned int i;

    units = selection_active_context_record_get((int*)&count);
    if (units != NULL) {
        for (i = 0; i < count; i++) {
            if (lego_terrain_cell_allows_placement(units[i], bx, by))
                return 1;
        }
    }
    return 0;
}

/* PlacementBlockedAdjacentCellScan [group 2G]: can a building's block go
 * here? The block must be a known, solid-path ground block without a fence,
 * none of whose four neighbours is terrain type 11. */
// FUNCTION: LEGORR 0x00431b20
int lego_terrain_placement_blocked_adjacent_cell_scan(int bx, int by,
                                                      int mode, int water)
{
    int blockPos[2];
    Lego_Block* block;
    unsigned int i;

    blockPos[1] = by;
    blockPos[0] = bx;
    block = &LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
        [legoGlobs_currLevel->width * by + bx];
    if (!(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
              [legoGlobs_currLevel->width * by + bx].flags2 & 0x20) &&
        !terrain_cell_predicate_check_current((const uint32_t*)blockPos) &&
        !(block->flags1 & 0x20000000)) {
        if ((!mode || !water || block->f_02 == 9) &&
            (!mode || !(block->flags1 & 0x8000)) &&
            (water || block->f_02 != 9) && block->f_02 != 6 &&
            (block->flags1 & 8) && (block->flags1 & 0x200000) &&
            !(block->flags1 & 0x108400)) {
            if (!electric_fence_block_is_fence(bx, by)) {
                int DIRECTIONS[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
                for (i = 0; i < 4; i++) {
                    if (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                            [(DIRECTIONS[i][1] + by) * legoGlobs_currLevel->width + bx + DIRECTIONS[i][0]].f_02 == 11)
                        return 0;
                }
                return 1;
            }
        }
    }
    return 0;
}

/* FirstFreeNeighborCellPick [group 2G]: the first of the four neighbours,
 * starting from a rotating direction, that the unit can route to. */
// FUNCTION: LEGORR 0x00431c70
int lego_first_free_neighbor_cell_pick(struct EntitySlot* liveObj,
                                       const int blockPos[2], int outPos[2],
                                       int skipWalls)
{
    int DIRECTIONS[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    unsigned int i;
    unsigned int dir;
    unsigned int bx, by;
    int wall;

    s_neighbor_pick_rotation++;
    for (i = 0; i < 4; i++) {
        dir = (s_neighbor_pick_rotation + i) & 3;
        /* The original inlines a helper here: is the neighbour inside the
         * map's border, with flags1 0x10 set and 8 clear, as a 0/1 value. */
        if (!skipWalls ||
            (bx = DIRECTIONS[dir][0] + blockPos[0], by = DIRECTIONS[dir][1] + blockPos[1],
             wall = 0,
             (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1 &&
              (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[by * legoGlobs_currLevel->width + bx].flags1 & 0x10) &&
              !(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[by * legoGlobs_currLevel->width + bx].flags1 & 8))
                 ? (wall = 1) : 0,
             !wall)) {
            if (lego_terrain_cell_route_and_occupancy_check(liveObj,
                    DIRECTIONS[dir][0] + blockPos[0], DIRECTIONS[dir][1] + blockPos[1],
                    DIRECTIONS[dir][0] + blockPos[0], DIRECTIONS[dir][1] + blockPos[1], 0)) {
                outPos[0] = DIRECTIONS[dir][0] + blockPos[0];
                outPos[1] = DIRECTIONS[dir][1] + blockPos[1];
                return 1;
            }
        }
    }
    return 0;
}

/* TerrainCellRouteAndOccupancyCheck [group 2G]: can the unit (or any unit,
 * for NULL) move from block (bx, by) to (toX, toY)? Returns 0 (no), 1, 2
 * (diagonal or slow) or 3 (water). */
// FUNCTION: LEGORR 0x00431da0
int lego_terrain_cell_route_and_occupancy_check(struct EntitySlot* liveObj,
                                                int bx, int by, int toX,
                                                int toY, int ignoreOwnBlock)
{
    Lego_Level* level = legoGlobs_currLevel;
    uint32_t crossLand = 0;
    uint32_t crossOther = 1;
    uint32_t enterLava = 0;
    uint32_t crossRubble = 0;
    Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];
    Lego_Block* toBlock = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[toY * level->width + toX];
    int result = 1;
    int DIRECTIONS[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    unsigned int dir;
    uint32_t flags;
    uint32_t water;
    int objX, objY;

    if (liveObj != NULL && (entity_stats_word_get(liveObj) & 0x20000) &&
        LEGO_FIELD(liveObj, void*, 0x36c) == NULL)
        return 0;

    for (dir = 0; dir < 4; dir++) {
        if (DIRECTIONS[dir][0] + toX == bx && toY + DIRECTIONS[dir][1] == by)
            break;
    }

    if (liveObj != NULL) {
        if (LEGO_FIELD(liveObj, int, 0) == 11)
            return 0;
        crossRubble = entity_capability_flags_get(liveObj) & 0x200;
        water = entity_stats_word_get(liveObj) & 1;
        crossLand = entity_stats_word_get(liveObj) & 0x1000000;
        crossOther = entity_stats_word_get(liveObj) & 0x2000000;
        enterLava = entity_state_flags_get(liveObj) & 0x200000;
        if (!ignoreOwnBlock && entity_block_pos_get_adapter(liveObj, &objX, &objY) &&
            bx == objX && by == objY)
            return 1;
    } else {
        water = 0;
    }

    if (block->f_02 == 9 && !crossLand)
        return 0;
    if (block->f_02 != 9 && !crossOther)
        return 0;
    if (block->f_02 == 6 && !enterLava && !(block->flags1 & 0x200))
        return 0;
    flags = block->flags1;
    if (flags & 0x400)
        return 0;
    if (ignoreOwnBlock && (flags & 0x40))
        return 0;
    if (!crossRubble && (block->flags2 & 4))
        return 0;
    if (water) {
        if (flags & 0x10)
            result = 3;
        if (block->f_00 == 0)
            return 0;
        if ((LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].flags1 & 0x10) &&
            (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx - 1].f_02 == 6 ||
             LEGO_LEVEL_PTR(Lego_Block, level->blocks)[(by - 1) * level->width + bx].f_02 == 6))
            return 0;
        if ((LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].flags1 & 0x10) &&
            (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx - 1].f_02 == 7 ||
             LEGO_LEVEL_PTR(Lego_Block, level->blocks)[(by - 1) * level->width + bx].f_02 == 7))
            return 0;
        if (dir == 0) {
            if (flags & 0x2000)
                result = 2;
            if (toBlock->flags1 & 0x2000)
                result = 2;
        }
    } else if (!(flags & 8)) {
        return 0;
    }
    if (toX != bx && toY != by) {
        if ((LEGO_LEVEL_PTR(Lego_Block, level->blocks)[toY * level->width + bx].flags1 & 8) &&
            (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + toX].flags1 & 8))
            return 2;
    }
    return result;
}

/* The block-update lists [group 2G]: blocks marked flags2 0x100 this frame
 * (s_cell_refresh_track, 0055684c), last frame's copy (s_cell_previous_track,
 * 00556b6c) and the visited blocks (s_cell_visited_track, 00556528). */
// FUNCTION: LEGORR 0x00432100
void lego_terrain_cell_flag100_set_and_track(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    int32_t (*list)[2] = s_cell_refresh_track;
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags2 |= 0x100;
    list[s_cell_refresh_track_count][0] = blockPos[0];
    list[s_cell_refresh_track_count][1] = blockPos[1];
    s_cell_refresh_track_count++;
    lego_cell_init_pass(level, blockPos[0], blockPos[1], 0);
}

// FUNCTION: LEGORR 0x00432170
uint32_t lego_terrain_cell_secondary_flag_100(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    return LEGO_LEVEL_PTR(Lego_Block, level->blocks)
        [blockPos[1] * level->width + blockPos[0]].flags2 & 0x100;
}

// FUNCTION: LEGORR 0x004321a0
void lego_terrain_cell_update_queue_flush(void)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned int i;

    for (i = 0; i < (unsigned int)s_cell_previous_track_count; i++) {
        if (!(LEGO_LEVEL_PTR(Lego_Block, level->blocks)
                  [level->width * s_cell_previous_track[i][1] + s_cell_previous_track[i][0]].flags2 & 0x100))
            lego_cell_init_pass(level, s_cell_previous_track[i][0], s_cell_previous_track[i][1], 0);
    }
    s_cell_previous_track_count = 0;
}

/* The original's lists are adjacent legoGlobs fields, so it walks one
 * pointer over the current list and writes the previous one at +0x320.
 * With legoGlobs one struct (finding 653), a C form with a Point2I copy
 * gives that loop except the order of the imul's operands (93.75%, an
 * effective match, at every counter phase tried); the separate-array form
 * scored 23.88%. So the matching build keeps the original's code, its
 * operands now legoGlobs' fields; the C form is for other compilers. */
// FUNCTION: LEGORR 0x00432200
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void lego_terrain_cell_previous_state_track(void)
{
    __asm {
        mov eax, dword ptr [s_cell_refresh_track_count]
        xor edx, edx
        push esi
        mov esi, dword ptr [g_active_level_retail]
        test eax, eax
        jbe lego_previous_state_track_done
        push edi
        mov ecx, offset legoGlobs + 78ch
    lego_previous_state_track_loop:
        mov eax, dword ptr [ecx]
        mov dword ptr [ecx + 320h], eax
        mov eax, dword ptr [ecx + 4]
        mov dword ptr [ecx + 324h], eax
        mov eax, dword ptr [ecx + 4]
        imul eax, dword ptr [esi + 14h]
        mov edi, dword ptr [ecx]
        add ecx, 8
        add eax, edi
        mov edi, dword ptr [esi + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [edi + eax*8 + 0ch]
        mov edi, dword ptr [eax]
        and edi, 0fffffeffh
        inc edx
        mov dword ptr [eax], edi
        mov eax, dword ptr [s_cell_refresh_track_count]
        cmp edx, eax
        jb lego_previous_state_track_loop
        pop edi
    lego_previous_state_track_done:
        mov dword ptr [s_cell_previous_track_count], eax
        mov dword ptr [s_cell_refresh_track_count], 0
        pop esi
        ret
    }
}
#else
void lego_terrain_cell_previous_state_track(void)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned int i;

    for (i = 0; i < (unsigned int)s_cell_refresh_track_count; i++) {
        s_cell_previous_track[i][0] = s_cell_refresh_track[i][0];
        s_cell_previous_track[i][1] = s_cell_refresh_track[i][1];
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)
            [s_cell_refresh_track[i][1] * level->width + s_cell_refresh_track[i][0]].flags2 &= ~0x100;
    }
    s_cell_previous_track_count = s_cell_refresh_track_count;
    s_cell_refresh_track_count = 0;
}
#endif

// FUNCTION: LEGORR 0x00432270
void lego_terrain_cell_visited_flag_set_and_track(const int blockPos[2])
{
    int32_t (*list)[2] = s_cell_visited_track;
    LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
        [legoGlobs_currLevel->width * blockPos[1] + blockPos[0]].flags2 |= 2;
    list[s_cell_visited_track_count][0] = blockPos[0];
    list[s_cell_visited_track_count][1] = blockPos[1];
    s_cell_visited_track_count++;
}

// FUNCTION: LEGORR 0x004322d0
uint32_t lego_terrain_cell_visited_flag(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    return LEGO_LEVEL_PTR(Lego_Block, level->blocks)
        [blockPos[1] * level->width + blockPos[0]].flags2 & 2;
}

/* Shared under MSVC x86 with the port's name: same contract, same globals.
 * Other targets keep the port model in lego.cpp (the level's pointer words
 * are not pointers on 64-bit). */
#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x00432300
void terrain_cell_visited_flags_clear_all_current(void)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned int i;

    for (i = 0; i < (unsigned int)s_cell_visited_track_count; i++) {
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)
            [s_cell_visited_track[i][1] * level->width + s_cell_visited_track[i][0]].flags2 &= ~2;
    }
    s_cell_visited_track_count = 0;
}
#endif

// FUNCTION: LEGORR 0x00432360
void lego_terrain_cell_primary_flags_mask_clear_and_refresh(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 &= 0xdfef7bff;
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags2 &= ~4;
    lego_cell_init_pass(level, blockPos[0], blockPos[1], 0);
}

// FUNCTION: LEGORR 0x004323c0
void lego_terrain_cell_secondary_bit0_clear(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)
        [blockPos[1] * level->width + blockPos[0]].flags2 &= ~1;
}

// FUNCTION: LEGORR 0x004323f0
void lego_terrain_cell_flags_set_and_refresh(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 |= 0x108000;
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags2 |= 4;
    lego_cell_init_pass(level, blockPos[0], blockPos[1], 0);
}

// FUNCTION: LEGORR 0x00432450
void lego_terrain_cell_flags_set_mask100400(int bx, int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags1 |= 0x100400;
    lego_cell_init_pass(level, bx, by, 0);
}

// FUNCTION: LEGORR 0x00432490
void lego_terrain_cell_flags_set_mask20108000(int bx, int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags1 |= 0x20108000;
    lego_cell_init_pass(level, bx, by, 0);
}

/* ---- part 2 functions, group 2G: end ---- */

/* ---- part 3 functions, group 3A (004324d0-00433330): begin ---- */

/* Natively this file builds as C++. Part 3 declares its imports here, not
 * in the extern "C" headers (finding 538), so it opens its own block. */
#ifdef __cplusplus
extern "C" {
#endif

/* Group 3A's imports and declarations sit here, after parts 1 and 2,
 * not in the shared headers: MSVC's symbol counter (finding 538, rule B)
 * then stays the same for every function before this point. */
/* Callees of the block accessors. */
struct InfoPoint2I;
void info_remove_block_references(struct InfoPoint2I* blockPos); /* info_unit.c, 004198f0 */
void selection_mode5_cancel_at_point(const uint32_t coordinate[2]); /* interface_unit.c, 0041cfe0 */
void aitask_coordinate_callback_dispatch_by_mode(const int* blockPos, int mode); /* aitask.c, 00402b60 */
float* d3drm_vector_random(float* r);                       /* maths.c, 00479500 */
int lava_effect_select_and_place(int slot, const float* position,
                                 const float* rotation);     /* resource.hpp, 0040cce0 */
int level_coord_pair_within_bounds_target(const unsigned int* dimensions,
                                          unsigned int cx, unsigned int cy); /* terrain_map.c, 004502c0 */
float entity_stats_float_at24_indexed(const struct EntitySlot* e, int index); /* entity.hpp, 00469cc0 */
uint32_t object_type_name_index_find(const char* name);    /* weapon.hpp, 0046f0d0 */
float entity_indexed_float_lookup_with_minus_one_fallback(int weapon_type,
                                                          const void* entity_record); /* weapon.hpp, 0046f1d0 */

/* Lego.c's block accessors (004324d0-00433330). Most share the port's name:
 * the port's declaration already has the original's contract, so the
 * converted callers (aitask.c, interface_unit.c, legoobject_unit.c,
 * terrain_cell_objects.c) reach them unchanged. They are compiled for MSVC
 * x86 only: the level's pointer words are not pointers on 64-bit, so other
 * targets keep the port models in lego.cpp. 00432950 and 00432a20 get a
 * lego_ name: the port's functions also take the level. 00432770 and
 * 00433180 take a LegoObject; the port's callers pass its EntitySlot (0x674
 * bytes, not the original's 0x40c), which only the matching build reads at
 * the original's offsets, as the port's own MSVC bodies already did. */
#define LEGO_3A_BLOCKS (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks))

#if defined(_MSC_VER) && defined(_M_IX86)

// FUNCTION: LEGORR 0x004324d0
void terrain_cell_secondary_flag_8_set_current(int bx, int by, int on)
{
    Lego_Level* level = legoGlobs_currLevel;
    if (on) {
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags2 |= 8;
        return;
    }
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags2 &= ~8;
}

// FUNCTION: LEGORR 0x00432520
uint32_t terrain_cell_secondary_flag_8_current(int bx, int by)
{
    return LEGO_3A_BLOCKS[legoGlobs_currLevel->width * by + bx].flags2 & 8;
}

// FUNCTION: LEGORR 0x00432550
int always_accept_place_command_cell(void)
{
    return 1;
}

// FUNCTION: LEGORR 0x00432560
void terrain_cell_flag40000000_clear_or_set_current(const int blockPos[2], int on)
{
    Lego_Level* level = legoGlobs_currLevel;
    if (on)
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 |= 0x40000000;
    else
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 &= ~0x40000000;
    lego_cell_init_pass(level, blockPos[0], blockPos[1], 0);
}

/* As the accessors below: MSVC 5 swaps ecx/edx here in every C form tried
 * (global or local level, both imul operand orders, a block pointer).
 * The closest C form was:
 *
 *   void terrain_cell_secondary_bit0_set_current(const int blockPos[2])
 *   {
 *       LEGO_3A_BLOCKS[blockPos[1] * legoGlobs_currLevel->width + blockPos[0]].flags2 |= 1;
 *   }
 */
// FUNCTION: LEGORR 0x004325d0
__declspec(naked) void terrain_cell_secondary_bit0_set_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 0ch]
        or dword ptr [eax], 1
        ret
    }
}

// FUNCTION: LEGORR 0x00432600
int terrain_cell_variant_flag_propagate_current(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]];
    uint32_t flags = block->flags1;

    if ((flags & 0x200000) && block->f_02 != 6) {
        int offsets[5][2];
        unsigned int i;

        offsets[0][0] = 0;
        offsets[0][1] = 0;
        offsets[1][0] = 0;
        offsets[1][1] = -1;
        offsets[2][0] = 1;
        offsets[2][1] = 0;
        offsets[3][0] = 0;
        offsets[3][1] = 1;
        offsets[4][0] = -1;
        offsets[4][1] = 0;
        if (flags & 0x80000000) {
            lava_erosion_pending_cell_add(blockPos, 0);
            LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].f_07 = 0;
        } else {
            block->flags1 = flags | 0x20000000;
        }
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 &= ~0x40000000;
        for (i = 0; i < 5; i++)
            lego_cell_init_pass(level, blockPos[0] + offsets[i][0], offsets[i][1] + blockPos[1], 0);
        entity_activity_scene_state_update_retail();
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00432710
void terrain_cell_flag_80000_clear_or_set_current(const int blockPos[2], int on)
{
    Lego_Level* level = legoGlobs_currLevel;
    if (on) {
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 |= 0x80000;
        return;
    }
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 &= ~0x80000;
}

/* Shared in the matching build only: the port's callers in ai.cpp and
 * legoobject_port.cpp pass the port's EntitySlot, which this build reads at the
 * original's offsets (the port's own MSVC body already did). */
// FUNCTION: LEGORR 0x00432770
int terrain_cell_walkability_query_current(const struct EntitySlot* liveObj,
                                           uint32_t bx, uint32_t by,
                                           int digging, int ignoreFlag400)
{
    Lego_Level* level = legoGlobs_currLevel;
    Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];

    if (!(entity_stats_word_get(liveObj) & 0x20000) || LEGO_FIELD(liveObj, void*, 0x36c) != NULL) {
        if (digging && !(entity_stats_word_get(liveObj) & 1))
            return 0;
        if ((!ignoreFlag400 || !(LEGO_FIELD(liveObj, uint32_t, 0x3e0) & 0x400)) &&
            (LEGO_FIELD(liveObj, unsigned char, 0x3e8) & 2) &&
            entity_stats_float_at24_indexed(liveObj, block->f_02) != 0.0f &&
            bx < level->width - 2 && by < level->height - 2) {
            if (digging) {
                if (block->f_00 == 0) {
                    int offsets[8][2];
                    unsigned int i;

                    offsets[4][0] = -1;
                    offsets[4][1] = -1;
                    offsets[5][0] = 0;
                    offsets[5][1] = -1;
                    offsets[6][0] = 0;
                    offsets[6][1] = 0;
                    offsets[7][0] = -1;
                    offsets[7][1] = 0;
                    offsets[0][0] = 0;
                    offsets[0][1] = -1;
                    offsets[1][0] = 1;
                    offsets[1][1] = 0;
                    offsets[2][0] = 0;
                    offsets[2][1] = 1;
                    offsets[3][0] = -1;
                    offsets[3][1] = 0;
                    for (i = 0; i < 4; i++) {
                        if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)
                                [(offsets[i][1] + by) * level->width + bx + offsets[i][0]].f_00 != 0)
                            break;
                    }
                    if (i < 4) {
                        for (i = 0; i < 4; i++) {
                            unsigned char type = LEGO_LEVEL_PTR(Lego_Block, level->blocks)
                                [(offsets[i + 4][1] + by) * level->width + bx + offsets[i + 4][0]].f_02;
                            if (type != 1 && type != 7)
                                break;
                        }
                        if (i < 4)
                            return 1;
                    }
                }
            } else {
                if ((block->flags1 & 0x10) && !(block->flags1 & 0x40) &&
                    block->f_02 != 1 && block->f_02 != 7 && block->f_02 != 11)
                    return 1;
            }
        }
    }
    return 0;
}

#endif /* _MSC_VER && _M_IX86 */

/* The port's functions take the level as well (and an EntitySlot); its
 * callers keep them. */
// FUNCTION: LEGORR 0x00432950
int lego_entity_move_destination_cell_usable(struct EntitySlot* liveObj, unsigned int bx, unsigned int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    if ((LEGO_FIELD(liveObj, uint32_t, 0x3e8) & 0x8000) && bx > 0 && by > 0 &&
        bx < level->width - 2 && by < level->height - 2) {
        Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];
        uint32_t flags = block->flags1;
        unsigned char type;
        if ((flags & 0x10) && !(flags & 0x40) && (type = block->f_02) != 1 && type != 11 && type != 7)
            return 1;
    }
    return 0;
}

#if defined(_MSC_VER) && defined(_M_IX86)

// FUNCTION: LEGORR 0x004329d0
int level_cell_flag8_check_target(const void* unused, int bx, int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    if (level_coord_pair_within_bounds_target(LEGO_LEVEL_PTR(const unsigned int, level->map), bx, by) &&
        (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags1 & 8))
        return 1;
    return 0;
}

#endif /* _MSC_VER && _M_IX86 */

// FUNCTION: LEGORR 0x00432a20
int lego_terrain_cell_allows_placement(struct EntitySlot* liveObj, unsigned int bx, unsigned int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    if ((liveObj == NULL || (LEGO_FIELD(liveObj, unsigned char, 0x3e8) & 4)) && bx > 0 && by > 0 &&
        bx < level->width - 2 && by < level->height - 2) {
        Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];
        uint32_t flags = block->flags1;
        unsigned char type;
        if ((flags & 0x10) && !(flags & 0x20) && (type = block->f_02) != 11 && type != 8 && type != 10)
            return 1;
    }
    return 0;
}

#if defined(_MSC_VER) && defined(_M_IX86)

// FUNCTION: LEGORR 0x00432aa0
uint32_t terrain_cell_occupied_or_flag_current(int bx, int by, int checkFlags2)
{
    Lego_Level* level = legoGlobs_currLevel;
    if (checkFlags2) {
        Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx];
        if (!(block->flags1 & 0x400) && !(block->flags2 & 4))
            return 0;
        return 1;
    }
    return LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags1 & 0x400;
}

// FUNCTION: LEGORR 0x00432b00
uint32_t terrain_cell_flag_800_bounded_current(uint32_t bx, uint32_t by)
{
    if (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1)
        return LEGO_3A_BLOCKS[by * legoGlobs_currLevel->width + bx].flags1 & 0x800;
    return 0;
}

// FUNCTION: LEGORR 0x00432b50
uint32_t terrain_cell_flag_8_bounded_current(uint32_t bx, uint32_t by)
{
    if (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1)
        return LEGO_3A_BLOCKS[by * legoGlobs_currLevel->width + bx].flags1 & 8;
    return 0;
}

// FUNCTION: LEGORR 0x00432b90
int terrain_cell_type_is_8_or_10_or_11_current(int bx, int by)
{
    unsigned char type = LEGO_3A_BLOCKS[legoGlobs_currLevel->width * by + bx].f_02;
    if (type == 8 || type == 10 || type == 11)
        return 1;
    return 0;
}

// FUNCTION: LEGORR 0x00432bd0
int terrain_cell_walkable_flag_current(uint32_t bx, uint32_t by)
{
    if (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1) {
        uint32_t flags = LEGO_3A_BLOCKS[by * legoGlobs_currLevel->width + bx].flags1;
        if ((flags & 0x10) && !(flags & 8))
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00432c20
uint32_t terrain_cell_flag_wave_current(int bx, int by)
{
    return LEGO_3A_BLOCKS[legoGlobs_currLevel->width * by + bx].flags1 & 0x40000;
}

// FUNCTION: LEGORR 0x00432c50
int terrain_cell_speed_class_current(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    uint32_t flags = LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1;
    if (flags & 0x200000)
        return 0;
    return (flags & 3) + 1;
}

// FUNCTION: LEGORR 0x00432c90
int terrain_cell_state_update_current(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    int result = 1;
    Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]];
    uint32_t flags = block->flags1;
    uint32_t stage = flags & 3;

    if (!(flags & 0x200000)) {
        if (stage == 0) {
            float wPos[3];
            float dir[3];
            block->flags1 = flags | 0x200000;
            info_remove_block_references((struct InfoPoint2I*)blockPos);
            selection_mode5_cancel_at_point((const uint32_t*)blockPos);
            aitask_coordinate_callback_dispatch_by_mode(blockPos, 0);
            terrain_map_block_to_world(LEGO_LEVEL_PTR(struct TerrainMap, level->map),
                                       blockPos[0], blockPos[1], &wPos[0], &wPos[1]);
            wPos[2] = terrain_map_world_z_get(LEGO_LEVEL_PTR(struct TerrainMap, level->map), wPos[0], wPos[1]);
            d3drm_vector_random(dir);
            dir[2] = 0.0f;
            lava_effect_select_and_place(3, wPos, dir);
        } else {
            /* The original stores the stage as a 2-bit field: */
            block->flags1 = (((stage - 1) ^ flags) & 3) ^ flags;
            result = 0;
        }
        lego_cell_init_pass(level, blockPos[0], blockPos[1], 0);
    }
    return result;
}

// FUNCTION: LEGORR 0x00432d90
uint32_t terrain_cell_flag_20_bounded_current(uint32_t bx, uint32_t by)
{
    if (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1)
        return LEGO_3A_BLOCKS[by * legoGlobs_currLevel->width + bx].flags1 & 0x20;
    return 0;
}

/* MSVC 5 swaps the two base registers (ecx/edx) of this accessor in every
 * C form tried: the level read directly or through a local, both imul
 * operand orders, a block-pointer or block-array local (finding 538,
 * rule B; the port's lego.cpp had the same body). The original's code is
 * kept; the closest C form was:
 *
 *   uint32_t terrain_cell_flag_reserved_current(const int blockPos[2])
 *   {
 *       return LEGO_3A_BLOCKS[blockPos[1] * legoGlobs_currLevel->width + blockPos[0]].flags1 & 0x8000000;
 *   }
 */
// FUNCTION: LEGORR 0x00432dd0
__declspec(naked) uint32_t terrain_cell_flag_reserved_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        and eax, 8000000h
        ret
    }
}

// FUNCTION: LEGORR 0x00432e00
void level_tile_mark_current_target(const int blockPos[2], int on)
{
    Lego_Level* level = legoGlobs_currLevel;
    if (on) {
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 |= 0x8000000;
        return;
    }
    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 &= ~0x8000000;
}

// FUNCTION: LEGORR 0x00432e60
uint32_t terrain_cell_mask_c0_current(int bx, int by)
{
    if ((LEGO_3A_BLOCKS[legoGlobs_currLevel->width * by + bx].flags1 & 0x40) || (LEGO_3A_BLOCKS[legoGlobs_currLevel->width * by + bx].flags1 & 0x80))
        return 1;
    return 0;
}

/* MSVC 5 swaps the two base registers (ecx/edx) of this accessor in every
 * C form tried: the level read directly or through a local, both imul
 * operand orders, a block-pointer or block-array local (finding 538,
 * rule B; the port's lego.cpp had the same body). The original's code is
 * kept; the closest C form was:
 *
 *   uint32_t terrain_cell_edge_wall_current(const int blockPos[2])
 *   {
 *       Lego_Level* level = legoGlobs_currLevel;
 *       return LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 & 0x4000000;
 *   }
 */
// FUNCTION: LEGORR 0x00432e90
__declspec(naked) uint32_t terrain_cell_edge_wall_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        and eax, 4000000h
        ret
    }
}

/* MSVC 5 swaps the two base registers (ecx/edx) of this accessor in every
 * C form tried: the level read directly or through a local, both imul
 * operand orders, a block-pointer or block-array local (finding 538,
 * rule B; the port's lego.cpp had the same body). The original's code is
 * kept; the closest C form was:
 *
 *   int terrain_cell_state_equals_one_current(const int blockPos[2])
 *   {
 *       Lego_Level* level = legoGlobs_currLevel;
 *       return LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].f_02 == 1;
 *   }
 */
// FUNCTION: LEGORR 0x00432ec0
__declspec(naked) int terrain_cell_state_equals_one_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push ebx
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        xor edx, edx
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov bl, byte ptr [ecx + eax*8 + 2]
        cmp bl, 1
        pop ebx
        sete dl
        mov eax, edx
        ret
    }
}

// FUNCTION: LEGORR 0x00432f00
int terrain_cell_state_equals_six_current(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    return LEGO_LEVEL_PTR(Lego_Block, level->blocks)[blockPos[1] * level->width + blockPos[0]].f_02 == 6;
}

// FUNCTION: LEGORR 0x00432f40
int terrain_cell_mask18_clear_current(uint32_t bx, uint32_t by)
{
    if (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1)
        return !(LEGO_3A_BLOCKS[by * legoGlobs_currLevel->width + bx].flags1 & 0x18);
    return 1;
}

// FUNCTION: LEGORR 0x00432f90
uint32_t level_cell_flag_4_current(uint32_t bx, uint32_t by)
{
    if (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1)
        return LEGO_3A_BLOCKS[by * legoGlobs_currLevel->width + bx].flags1 & 4;
    return 0;
}

// FUNCTION: LEGORR 0x00432fd0
uint32_t terrain_cell_flag_2000_current(int bx, int by)
{
    return LEGO_3A_BLOCKS[legoGlobs_currLevel->width * by + bx].flags1 & 0x2000;
}

// FUNCTION: LEGORR 0x00433000
uint32_t terrain_cell_flag_80_current(int bx, int by)
{
    return LEGO_3A_BLOCKS[legoGlobs_currLevel->width * by + bx].flags1 & 0x80;
}

// FUNCTION: LEGORR 0x00433030
uint32_t terrain_cell_flag_8000_current(const int blockPos[2])
{
    Lego_Level* level = legoGlobs_currLevel;
    return LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags1 & 0x8000;
}

/* MSVC 5 swaps the two base registers (ecx/edx) of this accessor in every
 * C form tried: the level read directly or through a local, both imul
 * operand orders, a block-pointer or block-array local (finding 538,
 * rule B; the port's lego.cpp had the same body). The original's code is
 * kept; the closest C form was:
 *
 *   uint32_t terrain_cell_secondary_bit0_current(const int blockPos[2])
 *   {
 *       Lego_Level* level = legoGlobs_currLevel;
 *       return LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * blockPos[1] + blockPos[0]].flags2 & 1;
 *   }
 */
// FUNCTION: LEGORR 0x00433060
__declspec(naked) uint32_t terrain_cell_secondary_bit0_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 0ch]
        and eax, 1
        ret
    }
}

/* MSVC 5 swaps the two base registers (ecx/edx) of this accessor in every
 * C form tried: the level read directly or through a local, both imul
 * operand orders, a block-pointer or block-array local (finding 538,
 * rule B; the port's lego.cpp had the same body). The original's code is
 * kept; the closest C form was:
 *
 *   uint32_t level_cell_occupied_at_current(const int blockPos[2])
 *   {
 *       Lego_Level* level = legoGlobs_currLevel;
 *       int bx = blockPos[0];
 *       int by;
 *       if (bx >= 0 && (by = blockPos[1]) >= 0 &&
 *           bx < (int)level->width && by < (int)level->height)
 *           return LEGO_LEVEL_PTR(Lego_Block, level->blocks)[level->width * by + bx].flags1 & 0x20000000;
 *       return 0;
 *   }
 */
// FUNCTION: LEGORR 0x00433090
__declspec(naked) uint32_t level_cell_occupied_at_current(
    const int cell_xy[2])
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [g_active_level_retail]
        push esi
        mov ecx, dword ptr [eax]
        test ecx, ecx
        jl level_cell_occupied_at_failed
        mov esi, dword ptr [eax + 4]
        test esi, esi
        jl level_cell_occupied_at_failed
        mov eax, dword ptr [edx + 14h]
        cmp ecx, eax
        jge level_cell_occupied_at_failed
        cmp esi, dword ptr [edx + 18h]
        jge level_cell_occupied_at_failed
        imul eax, esi
        add eax, ecx
        mov ecx, dword ptr [edx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        and eax, 20000000h
        pop esi
        ret
    level_cell_occupied_at_failed:
        xor eax, eax
        pop esi
        ret
    }
}

/* MSVC 5 swaps the two base registers (ecx/edx) of this accessor in every
 * C form tried: the level read directly or through a local, both imul
 * operand orders, a block-pointer or block-array local (finding 538,
 * rule B; the port's lego.cpp had the same body). The original's code is
 * kept; the closest C form was:
 *
 *   int terrain_cell_mask_180000_current(const int blockPos[2])
 *   {
 *       Lego_Level* level = legoGlobs_currLevel;
 *       if (!(LEGO_LEVEL_PTR(Lego_Block, level->blocks)[blockPos[1] * level->width + blockPos[0]].flags1 & 0x180000))
 *           return 0;
 *       (no value when set, as in the original)
 *   }
 */
// FUNCTION: LEGORR 0x004330e0
__declspec(naked) int terrain_cell_mask_180000_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        test dword ptr [ecx + eax*8 + 8], 180000h
        jne terrain_cell_mask_180000_true
        xor eax, eax
        pop esi
        ret
    terrain_cell_mask_180000_true:
        mov eax, 1
        pop esi
        ret
    }
}

/* MSVC 5 swaps the two base registers (ecx/edx) of this accessor in every
 * C form tried: the level read directly or through a local, both imul
 * operand orders, a block-pointer or block-array local (finding 538,
 * rule B; the port's lego.cpp had the same body). The original's code is
 * kept; the closest C form was:
 *
 *   void terrain_cell_flag_10000000_set_current(const int blockPos[2], int on)
 *   {
 *       Lego_Level* level = legoGlobs_currLevel;
 *       if (on) {
 *           LEGO_LEVEL_PTR(Lego_Block, level->blocks)[blockPos[1] * level->width + blockPos[0]].flags1 |= 0x10000000;
 *           return;
 *       }
 *       LEGO_LEVEL_PTR(Lego_Block, level->blocks)[blockPos[1] * level->width + blockPos[0]].flags1 &= ~0x10000000;
 *   }
 */
// FUNCTION: LEGORR 0x00433120
__declspec(naked) void terrain_cell_flag_10000000_set_current(
    const int cell_xy[2], int set)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        test eax, eax
        mov eax, dword ptr [edx + 4]
        je terrain_cell_flag_10000000_clear
        imul eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 8]
        or dword ptr [eax], 10000000h
        pop esi
        ret
    terrain_cell_flag_10000000_clear:
        imul eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        add eax, esi
        pop esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 0efffffffh
        mov dword ptr [eax], ecx
        ret
        nop
        nop
        nop
        nop
    }
}

/* Shared in the matching build only, as 00432770. */
// FUNCTION: LEGORR 0x00433180
int entity_cell_type_speed_multiplier_resolve_current(const struct EntitySlot* liveObj, int bx, int by,
                                                      float speed, float* outSpeed)
{
    Lego_Level* level = legoGlobs_currLevel;
    int type = LEGO_FIELD(liveObj, int, 0);

    if (type != 6 && type != 7 && type != 5 && type != 8 && type != 11 && type != 13 &&
        type != 14 && type != 15 && type != 16 && type != 17 && type != 18 && type != 19) {
        Lego_Block* block;
        if (LEGO_FIELD(liveObj, void*, 0x36c) != NULL && !(entity_stats_word_get(liveObj) & 0x20000))
            return 0;
        block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];
        if (block->f_02 == 6 && !(block->flags1 & 0x200) && !(entity_state_flags_get(liveObj) & 0x200000)) {
            if (outSpeed != NULL)
                *outSpeed = speed;
            return 1;
        }
        if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].flags1 & 0x1000) {
            if (outSpeed != NULL)
                *outSpeed = entity_indexed_float_lookup_with_minus_one_fallback(
                                (int)object_type_name_index_find(
                                    // STRING: LEGORR 0x004a471c
                                    "RockFallIn"),
                                liveObj) * speed;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x004332c0
uint32_t terrain_cell_record_byte3_current(int bx, int by)
{
    return LEGO_3A_BLOCKS[legoGlobs_currLevel->width * by + bx].f_03;
}

// FUNCTION: LEGORR 0x004332f0
void terrain_cell_work_flag_set_current(uint32_t bx, uint32_t by)
{
    if (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1)
        LEGO_3A_BLOCKS[by * legoGlobs_currLevel->width + bx].flags1 |= 4;
}

// FUNCTION: LEGORR 0x00433330
int terrain_cell_type_get_at_current(uint32_t bx, uint32_t by, uint32_t* outType)
{
    if (bx < legoGlobs_currLevel->width - 1 && by < legoGlobs_currLevel->height - 1) {
        *outType = LEGO_3A_BLOCKS[by * legoGlobs_currLevel->width + bx].f_02;
        return 1;
    }
    return 0;
}

#endif /* _MSC_VER && _M_IX86 */


/* ---- part 3 functions, group 3A: end ---- */

/* ---- part 3 functions, group 3B (00433380-00434530): begin ---- */

/* Group 3B's state, declarations and imports sit here, before its
 * functions and after every earlier one, so MSVC's symbol counter
 * (finding 538, rule B) does not move for parts 1, 2 and earlier groups. */
/* legoGlobs.quality is a legoGlobs field (lego_globs.h, finding 653). Its
 * old stand-in name is declared once more, unreferenced, so that group 3B's
 * symbol counter does not move. Deviation (scoring only). */
#undef legoGlobs_quality
extern int legoGlobs_quality;                   /* 005560d0 */
#define legoGlobs_quality legoGlobs.quality

/* Lego_LoadGraphicsSettings's projected-cell list (00433e80): the block
 * and its distance and projected area, sorted by area (00434450). */
typedef struct LegoProjectedCell3B {
    float x, y;
    float dist;
    float area;
} LegoProjectedCell3B;
typedef struct LegoPoint2U3B {
    unsigned int x, y;
} LegoPoint2U3B;

/* Shared (the port's entries keep the name; see lego.cpp):
 * terrain_cell_mutation_progress (00433380, MSVC x86 only),
 * level_map_dimensions_get (004334c0) and
 * terrain_grid_entry_resources_release_all_current (00434530, MSVC x86
 * only) are declared in lego_unit_imports.h. */
int lego_render_quality_config_apply(void);                          /* 004334f0 */
int lego_scene_lights_build_from_config(void);                       /* 00433770 */
/* Deviation (scoring only): two of group 3C's entries are declared here
 * rather than with group 3C's state. In the matching build
 * lego_game_config_init calls the two loaders above by these names
 * (lego_unit_imports.h), so their declarations here add no symbol; these
 * two keep MSVC's symbol counter at 00433380 and 00434530 where it was
 * (finding 538 rule B, finding 639). */
int ledger_stub_00435550_GlobalStateZeroTransitionDetect(void);      /* 00435550 */
int lego_global_handle_position_apply_if_present(void);              /* 00435580 */
void lego_current_level_terrain_grid_pick(float x, float y, int* bx,
                                          int* by);                  /* 00433be0 */
int lego_entity_neighbor_blocked_cell_step_probe(struct EntitySlot* object,
                                                 float elapsed,
                                                 int force);         /* 00433c10 */
int lego_grid_cell_entity_eligibility_check(int bx, int by);         /* 00433e30 */
void lego_terrain_projected_cell_state_refresh(void* cont, void* view,
                                               float maxDist,
                                               unsigned int maxCount); /* 00433e80 */
int lego_projected_cell_area_compare(const void* a, const void* b);  /* 00434450 */
int lego_terrain_cell_surface_update_dispatch_level(Lego_Level* level,
                                                    int bx, int by,
                                                    float scale,
                                                    float uniformScale); /* 00434480 */

struct TerrainMap;
struct TerrainMapVector3;
struct EntitySlot;
struct SceneCellVisualRelayRecord;
/* GAME-026 Map3D.c (terrain_map.h). */
int terrain_map_block_vertices_get(struct TerrainMap* map, unsigned int bx,
                                   unsigned int by,
                                   struct TerrainMapVector3* positions); /* 004500d0 */
int terrain_map_block_hidden_get(struct TerrainMap* map, int bx, int by); /* 0044e860 */
void terrain_map_visible_add(struct TerrainMap* map, int bx, int by);     /* 00450730 */
/* G98 Container.c (view.hpp, container_unit.cpp): the light's colour and
 * the spot/range values are float bit patterns in the port's contract. */
void* ddiscene_light_create(void* owner, uint32_t value_a, uint32_t value_b,
                            uint32_t value_c, uint32_t value_d);  /* 004734f0 */
int light_interface_call_slot_38(void* record, uint32_t value);   /* 004734a0 */
int light_interface_call_slot_3c(void* record, uint32_t value);   /* 00473480 */
int light_interface_call_slot_40(void* record, uint32_t value);   /* 00473460 */
void* scene_root_object_get(void);                                /* 00472940 */
/* LightEffects.c (effects.hpp). */
void scene_transform_state_initialize(void* target, void* owner,
                                      float x, float y, float z); /* 0044c710 */
/* LegoObject.c (entity.hpp; aitask_port.cpp's adapter for 00449700). */
int entity_flags_mask107c33de_or_1c00203_any_test(const struct EntitySlot* e); /* 0043baf0 */
int aitask_route_step_adapter(struct EntitySlot* object);         /* 00449700 */
/* Roof.c (record.hpp): 004634f0. */
void indexed_surface_entry_hide_and_record(int x, int y);         /* 004634f0 */
/* Detail.c (detail.c in the matching build, view.hpp). In the matching
 * build the callee is detail.c's Detail_AddBlock, whose texture and
 * direction are bytes (the caller pushes them unextended); the port's
 * view.cpp entry takes words. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LEGO_DETAIL_BYTE unsigned char
#else
#define LEGO_DETAIL_BYTE uint32_t
#endif
void scene_cell_visual_channels_apply(struct SceneCellVisualRelayRecord* record,
                                      const float corner_points[12],
                                      LEGO_DETAIL_BYTE lookup_selector,
                                      LEGO_DETAIL_BYTE corner_index, float scale,
                                      float uniform_scale);       /* 0040b880 */
/* No port entry with the original's contract: adapters at the end of
 * lego.cpp (group 3B). */
void lego_roof_block_vertices_set_adapter(unsigned int bx, unsigned int by,
                                          struct TerrainMapVector3* v0,
                                          struct TerrainMapVector3* v1,
                                          struct TerrainMapVector3* v2,
                                          struct TerrainMapVector3* v3); /* 00462f20 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: Main.c's Main_Setup3D itself (gods98/main.c since #738,
 * which reads the first five of the seven arguments). A rename of the
 * declaration below, one for one. */
#define lego_main_setup3d_adapter direct3d_render_preferences_apply
#endif
void lego_main_setup3d_adapter(int quality, int dither, int filter,
                               int mipmap, int linearMipMap, int blend,
                               int sort);                         /* 00478330 */
/* container_link_names.h's matching-build link name of Container_Create
 * (container.c since #697). */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define lego_container_create_adapter ddiscene_object_allocate
#endif
void* lego_container_create_adapter(void* parent);                /* 00472950 */
float lego_viewport_field_get_adapter(void* view);                /* 00477270 */
void lego_viewport_size_get_adapter(void* view, unsigned int* width,
                                    unsigned int* height);        /* 00476f10 */
float lego_projected_quad_area_adapter(struct TerrainMapVector3* vertices,
                                       void* view,
                                       const float screen[2],
                                       int noClip);               /* 0040b470 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define lego_entity_random_cell_step_route_adapter legoobject_random_cell_step_route_issue
#endif
int lego_entity_random_cell_step_route_adapter(struct EntitySlot* object); /* 00444260 */
/* 0040b810's port entry takes the ledger namespace's channel table. */
void lego_detail_mesh_resources_clear_adapter(void* mesh);        /* 0040b810 */


#if defined(_MSC_VER) && defined(_M_IX86)

/* Lego_UpdateBlockRoof [group 3B]: lowers the roof quad over a wall block
 * (all four corners), or over a reinforced/pre-dug edge block (the corners
 * whose block is pre-dug), by the dig depth plus the roof height, and hands
 * the quad to the roof. Shared under MSVC x86: the port's model in lego.cpp
 * keeps the native path. */
// FUNCTION: LEGORR 0x00433380
void terrain_cell_mutation_progress(struct LevelState* levelState,
                                    uint32_t bx, uint32_t by)
{
    Lego_Level* level = (Lego_Level*)levelState;
    struct TerrainMap* map = LEGO_LEVEL_PTR(struct TerrainMap, level->map);
    LegoVector3FC vertPos[4];

    if (bx < level->width - 1 && by < level->height - 1) {
        Lego_Block* block;

        terrain_map_block_vertices_get(map, bx, by, (struct TerrainMapVector3*)vertPos);
        block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];
        if (block->flags1 & 0x8) {
            float drop = level->digDepth + level->roofHeight;
            vertPos[0].z -= drop;
            vertPos[1].z -= drop;
            vertPos[2].z -= drop;
            vertPos[3].z -= drop;
        } else if (block->flags1 & 0x10) {
            if (block->f_00 == 1)
                vertPos[0].z -= level->digDepth + level->roofHeight;
            if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx + 1].f_00 == 1)
                vertPos[1].z -= level->digDepth + level->roofHeight;
            if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[(by + 1) * level->width + bx + 1].f_00 == 1)
                vertPos[2].z -= level->digDepth + level->roofHeight;
            if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[(by + 1) * level->width + bx].f_00 == 1)
                vertPos[3].z -= level->digDepth + level->roofHeight;
        } else {
            return;
        }
        lego_roof_block_vertices_set_adapter(bx, by,
                                             (struct TerrainMapVector3*)&vertPos[0],
                                             (struct TerrainMapVector3*)&vertPos[1],
                                             (struct TerrainMapVector3*)&vertPos[2],
                                             (struct TerrainMapVector3*)&vertPos[3]);
    }
}

#endif /* _MSC_VER && _M_IX86 */

/* Lego_GetMapFileDimensions [group 3B]: the map file's header dimensions.
 * Shared: the port's definition in lego.cpp is gone. */
// FUNCTION: LEGORR 0x004334c0
void level_map_dimensions_get(uint32_t slot, uint32_t* width,
                              uint32_t* height)
{
    uint8_t* image = level_map_slot_get(slot);
    image += 0x08;
    *width = *(uint32_t*)image;
    *height = *(uint32_t*)(image + 4);
}

/* Lego_LoadGraphicsSettings [group 3B]. */
// FUNCTION: LEGORR 0x004334f0
int lego_render_quality_config_apply(void)
{
    const char* str;
    int quality;
    int dither, filter, blend, sort, mipmap, linearMipMap;

    if (0.0f != (legoGlobs_FPClipBlocks = (float)atof(LEGO_A_CFG_STR(legoGlobs_config,
            string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a4784
                "fpclipblocks", 0))))) {
        str = block_get_strptr(legoGlobs_config,
            string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a477c
                "quality", 0));
        if (str != NULL) {
            // STRING: LEGORR 0x004a4774
            if (_strcmpi("gouraud", str) == 0) quality = 3;
            // STRING: LEGORR 0x004a476c
            else if (_strcmpi("flat", str) == 0) quality = 2;
            // STRING: LEGORR 0x004a4760
            else if (_strcmpi("wireframe", str) == 0) quality = 0;
            else return 0;
            legoGlobs_quality = quality;

            dither = block_get_bool_path(legoGlobs_config,
                string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a4758
                    "dither", 0));
            filter = block_get_bool_path(legoGlobs_config,
                string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a4750
                    "filter", 0));
            blend = block_get_bool_path(legoGlobs_config,
                string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a4748
                    "blend", 0));
            sort = block_get_bool_path(legoGlobs_config,
                string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a4740
                    "sort", 0));
            mipmap = block_get_bool_path(legoGlobs_config,
                string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a4738
                    "mipmap", 0));
            linearMipMap = block_get_bool_path(legoGlobs_config,
                string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a4728
                    "linearmipmap", 0));
            lego_main_setup3d_adapter(quality, dither == 1, filter == 1,
                                      mipmap == 1, linearMipMap == 1,
                                      blend == 1, sort == 1);
            return 1;
        }
    }
    return 0;
}

/* Lego_LoadLighting [group 3B]. The port's Container entries take the
 * light colour and the spot values as float bit patterns. */
// FUNCTION: LEGORR 0x00433770
int lego_scene_lights_build_from_config(void)
{
    float r, g, b;

    if (!config_rgb_vector_read(legoGlobs_config,
            string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a47cc
                "topspotrgb", 0),
            &r, &g, &b)) return 0;
    if ((legoGlobs_spotlightTop = ddiscene_light_create(
             LEGO_FIELD(s_zoom_step_context, void*, 0x2c), 2,
             *(uint32_t*)&r, *(uint32_t*)&g, *(uint32_t*)&b)) == NULL) return 0;
    light_interface_call_slot_40(legoGlobs_spotlightTop, 0x3f59999a); /* 0.85f */
    light_interface_call_slot_3c(legoGlobs_spotlightTop, 0x3ecccccd); /* 0.4f */
    ddiscene_object_position_set(legoGlobs_spotlightTop,
                                 LEGO_FIELD(s_zoom_step_context, void*, 0x2c),
                                 200.0f, 140.0f, -130.0f);
    ddiscene_object_pose_method_dispatch(legoGlobs_spotlightTop,
                                         LEGO_FIELD(s_zoom_step_context, void*, 0x2c),
                                         -1.0f, -0.8f, 0.75f, 0.0f, 1.0f, 0.0f);

    if (!config_rgb_vector_read(legoGlobs_config,
            string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a47bc
                "trackspotrgb", 0),
            &r, &g, &b)) return 0;
    if ((legoGlobs_spotlightTrack = ddiscene_light_create(
             LEGO_FIELD(legoGlobs_cameraTrack, void*, 0x28), 2,
             *(uint32_t*)&r, *(uint32_t*)&g, *(uint32_t*)&b)) == NULL) return 0;
    ddiscene_object_position_set(legoGlobs_spotlightTrack, NULL,
                                 0.0f, 0.0f, -150.0f);
    ddiscene_object_pose_method_dispatch(legoGlobs_spotlightTrack, NULL,
                                         0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);

    if (!config_rgb_vector_read(legoGlobs_config,
            string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a47b0
                "ambientrgb", 0),
            &r, &g, &b)) return 0;
    if ((legoGlobs_ambientLight = ddiscene_light_create(legoGlobs_rootCont, 0,
             *(uint32_t*)&r, *(uint32_t*)&g, *(uint32_t*)&b)) == NULL) return 0;

    if (!config_rgb_vector_read(legoGlobs_config,
            string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a47a4
                "fplightrgb", 0),
            &r, &g, &b)) return 0;
    if ((legoGlobs_f_5560f8 = (uint32_t)(uintptr_t)ddiscene_light_create(
             LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24), 1,
             *(uint32_t*)&r, *(uint32_t*)&g, *(uint32_t*)&b)) == 0) return 0;

    if (config_rgb_vector_read(legoGlobs_config,
            string_path_join(s_engine_config_root, "Main",
                // STRING: LEGORR 0x004a4794
                "fprotlightrgb", 0),
            &r, &g, &b)) {
        legoGlobs_dirLightFP = ddiscene_light_create(
            LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24), 3,
            *(uint32_t*)&r, *(uint32_t*)&g, *(uint32_t*)&b);
    } else {
        legoGlobs_dirLightFP = NULL;
    }

    if ((legoGlobs_dirLightCallToArms = ddiscene_light_create(
             LEGO_FIELD(legoGlobs_cameraFP, void*, 0x24), 3,
             0x3f800000, 0, 0)) == NULL) return 0; /* 1, 0, 0 */

    legoGlobs_f_556108 = lego_container_create_adapter(scene_root_object_get());
    if ((legoGlobs_f_556104 = (uint32_t)(uintptr_t)ddiscene_light_create(
             legoGlobs_f_556108, 2,
             0x3f800000, 0x3f800000, 0x3f800000)) == 0) return 0; /* 1, 1, 1 */
    scene_transform_state_initialize((void*)(uintptr_t)legoGlobs_f_556104,
                                     legoGlobs_f_556108, 0.8f, 0.8f, 0.8f);
    ddiscene_object_pose_method_dispatch((void*)(uintptr_t)legoGlobs_f_556104, NULL,
                                         0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    light_interface_call_slot_40((void*)(uintptr_t)legoGlobs_f_556104, 0x3f59999a); /* 0.85f */
    light_interface_call_slot_3c((void*)(uintptr_t)legoGlobs_f_556104, 0);

    scene_render_lock_toggle((void*)(uintptr_t)legoGlobs_f_5560f8, 1);
    if (legoGlobs_dirLightFP != NULL) scene_render_lock_toggle(legoGlobs_dirLightFP, 1);
    scene_render_lock_toggle(legoGlobs_spotlightTop, 1);
    scene_render_lock_toggle(legoGlobs_spotlightTrack, 1);
    scene_render_lock_toggle(legoGlobs_ambientLight, 1);
    scene_render_lock_toggle((void*)(uintptr_t)legoGlobs_f_556104, 1);
    scene_render_lock_toggle(legoGlobs_dirLightCallToArms, 1);

    light_interface_call_slot_38(legoGlobs_spotlightTop, 0x44fa0000); /* 2000.0f */
    light_interface_call_slot_38(legoGlobs_spotlightTrack, 0x44fa0000);
    light_interface_call_slot_38((void*)(uintptr_t)legoGlobs_f_556104, 0x44fa0000);
    return 1;
}

/* Lego_GetBlockXYFromWorldPos? [group 3B]: the current level's
 * Map3D_WorldToBlockPos. */
// FUNCTION: LEGORR 0x00433be0
void lego_current_level_terrain_grid_pick(float x, float y, int* bx, int* by)
{
    terrain_map_world_to_block(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map),
                               x, y, bx, by);
}

/* 00432bd0's test, which the original inlines here. A macro, not a static
 * helper: the lint takes every unannotated definition for port code. */
#define LEGO_3B_BLOCK_OPEN(x, y) \
    ((unsigned int)(x) < legoGlobs_currLevel->width - 1 && \
     (unsigned int)(y) < legoGlobs_currLevel->height - 1 && \
     (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[(unsigned int)(y) * legoGlobs_currLevel->width + (unsigned int)(x)].flags1 & 0x10) && \
     !(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[(unsigned int)(y) * legoGlobs_currLevel->width + (unsigned int)(x)].flags1 & 0x8))

/* [group 3B] Slug/object scare step: at a random interval (the elapsed
 * time against 300, or against the object's stats value +0x140), an
 * object next to a fresh rubble-free reinforced/cleared block takes a route
 * step, or a random neighbouring step. */
// FUNCTION: LEGORR 0x00433c10
int lego_entity_neighbor_blocked_cell_step_probe(struct EntitySlot* object,
                                                 float elapsed, int force)
{
    int by;
    int bx;
    int offsets[4][2];
    unsigned int i;
    float rate;
    float maxRate;

    offsets[0][0] = -1; offsets[0][1] = 0;
    offsets[1][0] = 1;  offsets[1][1] = 0;
    offsets[2][0] = 0;  offsets[2][1] = -1;
    offsets[3][0] = 0;  offsets[3][1] = 1;

    if ((entity_stats_word_get(object) & 0x1000) &&
        !(LEGO_FIELD(object, uint32_t, 0x3e0) & 1) &&
        !entity_flags_mask107c33de_or_1c00203_any_test(object) &&
        elapsed != 0.0f) {
        rate = elapsed;
        if (elapsed > 300.0f) rate = 300.0f;
        if ((unsigned int)(int)rng_thunk_draw15() % (unsigned int)(300.0f / rate) == 0) {
            entity_block_pos_get_adapter(object, &bx, &by);
            for (i = 0; i < 4; i++) {
                unsigned int y = by + offsets[i][1];
                unsigned int x = bx + offsets[i][0];
                if (LEGO_3B_BLOCK_OPEN(x, y)) {
                    LEGO_FIELD(object, float, 0x340) = -1.0f;
                    aitask_route_step_adapter(object);
                    return 1;
                }
            }
        }
    }

    if ((force || (entity_stats_word_get(object) & 0x400)) &&
        !(LEGO_FIELD(object, uint32_t, 0x3e0) & 1) &&
        !entity_flags_mask107c33de_or_1c00203_any_test(object) &&
        elapsed != 0.0f) {
        maxRate = LEGO_FIELD(LEGO_FIELD(object, void*, 0x334), float, 0x140);
        rate = elapsed;
        if (elapsed > maxRate) rate = maxRate;
        if (force || (unsigned int)(int)rng_thunk_draw15() % (unsigned int)(maxRate / rate) == 0) {
            lego_entity_random_cell_step_route_adapter(object);
            return 1;
        }
    }
    return 0;
}

/* [group 3B] A block an object may be placed on: not flagged 0x100 and
 * visible on the map. */
// FUNCTION: LEGORR 0x00433e30
int lego_grid_cell_entity_eligibility_check(int bx, int by)
{
    if (!(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
              [legoGlobs_currLevel->width * by + bx].flags1 & 0x100) &&
        terrain_map_block_hidden_get((struct TerrainMap*)level_current_terrain_get_original(),
                                     bx, by)) {
        return 1;
    }
    return 0;
}

/* [group 3B] The first-person high-poly blocks: every surface block within
 * maxDist of the camera and inside its view cone is projected to the
 * screen. Blocks with no area are hidden; the rest, largest first, get
 * high-poly detail (up to maxCount within two thirds of maxDist) or go
 * back to the map's low-poly mesh. */
// FUNCTION: LEGORR 0x00433e80
void lego_terrain_projected_cell_state_refresh(void* cont, void* view,
                                               float maxDist,
                                               unsigned int maxCount)
{
    Lego_Level* level = legoGlobs_currLevel;
    struct TerrainMap* map = LEGO_LEVEL_PTR(struct TerrainMap, level->map);
    unsigned int visibleCount = 0;
    float blockSize = terrain_context_scale_get(map);
    int radius = (int)(maxDist / blockSize);
    unsigned int hiddenCount = 0;
    LegoVector3FC camPos;
    LegoVector3FC camDir;
    LegoVector3FC wPos;
    LegoVector3FC diff;
    LegoVector3FC vertPos[4];
    int camX, camY;
    int startX, startY, endX, endY;
    unsigned int x, y;
    float screen[2];
    float cosFov;
    float dist;
    unsigned int i;
    unsigned int highCount;
    LegoPoint2U3B hidden[300];
    LegoProjectedCell3B visible[300];

    ddiscene_object_position_get(cont, NULL, &camPos.x);
    terrain_map_world_to_block(map, camPos.x, camPos.y, &camX, &camY);
    ddiscene_object_facing_pair_get(cont, NULL, &camDir.x, NULL);
    cosFov = (float)cos(lego_viewport_field_get_adapter(legoGlobs_viewMain) * 1.4f);
    lego_viewport_size_get_adapter(legoGlobs_viewMain, &x, &y);
    screen[0] = (float)x;
    screen[1] = (float)y;

    startX = camX - radius;
    startY = camY - radius;
    endX = camX + radius;
    endY = camY + radius;
    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    if (endX < 0) endX = 0;
    if (endY < 0) endY = 0;
    if (endX >= (int)level->width) endX = level->width - 1;
    if (endY >= (int)level->height) endY = level->height - 1;

    for (y = startY; y < (unsigned int)endY; y++) {
        for (x = startX; x < (unsigned int)endX; x++) {
            if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1 & 0x18) {
                terrain_map_block_to_world(map, x, y, &wPos.x, &wPos.y);
                wPos.z = terrain_map_world_z_get(map, wPos.x, wPos.y);
                diff.x = wPos.x - camPos.x;
                diff.y = wPos.y - camPos.y;
                diff.z = wPos.z - camPos.z;
                dist = (float)sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
                if (dist < maxDist) {
                    float inv = 1.0f / dist;
                    float dot = inv * diff.x * camDir.x + inv * diff.y * camDir.y +
                                inv * diff.z * camDir.z;
                    int noClip;
                    if (dist < (dot + 2.0f) * blockSize) {
                        dot = cosFov + 1.0f;
                        noClip = 1;
                    } else {
                        noClip = 0;
                    }
                    if (dot > cosFov) {
                        terrain_map_block_vertices_get(map, x, y,
                                                       (struct TerrainMapVector3*)vertPos);
                        visible[visibleCount].x = (float)x;
                        visible[visibleCount].y = (float)y;
                        visible[visibleCount].dist = dist;
                        if ((visible[visibleCount].area = lego_projected_quad_area_adapter(
                                 (struct TerrainMapVector3*)vertPos, view, screen,
                                 noClip)) > 0.0f) {
                            visibleCount++;
                        } else {
                            hidden[hiddenCount].x = x;
                            hidden[hiddenCount].y = y;
                            hiddenCount++;
                        }
                    }
                }
            }
        }
    }

    qsort(visible, visibleCount, sizeof(LegoProjectedCell3B),
          lego_projected_cell_area_compare);

    maxDist *= 0.6666667f;
    highCount = 0;
    for (i = 0; i < visibleCount; i++) {
        x = (int)visible[i].x;
        y = (int)visible[i].y;
        if (highCount < maxCount && visible[i].dist < maxDist) {
            float scale = 1.0f - visible[i].dist / maxDist;
            float uniform = scale * 0.7f + 0.3f;
            if ((legoGlobs_flags1 & 0x20) &&
                lego_terrain_cell_surface_update_dispatch_level(level, x, y, scale, uniform)) {
                LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1 |= 0x100;
                indexed_surface_entry_hide_and_record(x, y);
                highCount++;
            } else {
                terrain_map_visible_add(map, x, y);
                indexed_surface_entry_hide_and_record(x, y);
                if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1 & 0x4000) {
                    transient_effect_enable_set((void*)(uintptr_t)
                        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_10[3], 0);
                }
                LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1 &= ~0x100;
            }
        } else {
            LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1 &= ~0x100;
            terrain_map_visible_add(map, x, y);
            indexed_surface_entry_hide_and_record(x, y);
            if (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].flags1 & 0x4000) {
                transient_effect_enable_set((void*)(uintptr_t)
                    LEGO_LEVEL_PTR(Lego_Block, level->blocks)[y * level->width + x].f_10[3], 0);
            }
        }
    }

    for (i = 0; i < hiddenCount; i++) {
        indexed_surface_entry_hide_and_record(hidden[i].x, hidden[i].y);
    }
}

/* [group 3B] qsort callback of 00433e80: larger projected area first. */
// FUNCTION: LEGORR 0x00434450
int lego_projected_cell_area_compare(const void* a, const void* b)
{
    if (((const LegoProjectedCell3B*)a)->area < ((const LegoProjectedCell3B*)b)->area) return 1;
    if (((const LegoProjectedCell3B*)a)->area > ((const LegoProjectedCell3B*)b)->area) return -1;
    return 0;
}

/* [group 3B] A surface block's high-poly detail mesh: the level's tile
 * mesh for the block's texture, placed on the block's four corners. */
// FUNCTION: LEGORR 0x00434480
int lego_terrain_cell_surface_update_dispatch_level(Lego_Level* level,
                                                    int bx, int by,
                                                    float scale,
                                                    float uniformScale)
{
    Lego_Block* block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];
    void* mesh;
    LegoVector3FC vertPos[4];

    if ((block->flags1 & 0x18) &&
        (mesh = LEGO_LEVEL_PTR(void*, level->f_080)[level->f_074 * (block->texture & 0xf) +
                                                    (block->texture >> 4)]) != NULL) {
        terrain_map_block_vertices_get(LEGO_LEVEL_PTR(struct TerrainMap, level->map),
                                       bx, by, (struct TerrainMapVector3*)vertPos);
        scene_cell_visual_channels_apply((struct SceneCellVisualRelayRecord*)mesh,
            &vertPos[0].x,
            LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].texture,
            LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].f_03,
            scale, uniformScale);
        return 1;
    }
    return 0;
}

#if defined(_MSC_VER) && defined(_M_IX86)

/* [group 3B] Releases the resources of every tile mesh of the current
 * level. Shared under MSVC x86: the port's model in lego.cpp keeps the
 * native path. */
// FUNCTION: LEGORR 0x00434530
void terrain_grid_entry_resources_release_all_current(void)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned int i;

    for (i = 0; i < level->f_078 * level->f_074; i++) {
        if (LEGO_LEVEL_PTR(void*, level->f_080)[i] != NULL) {
            lego_detail_mesh_resources_clear_adapter(LEGO_LEVEL_PTR(void*, level->f_080)[i]);
        }
    }
}

#endif /* _MSC_VER && _M_IX86 */

/* ---- part 3 functions, group 3B: end ---- */

/* ---- part 3 functions, group 3C (00434570-00435aa0): begin ---- */

/* Group 3C's state, declarations and imports sit here, before its
 * functions and after every earlier one, so MSVC's symbol counter
 * (finding 538, rule B) does not move for parts 1, 2 and earlier groups. */

/* Input's left mouse button (0076ab3c) at the last release check. */
// GLOBAL: LEGORR 0x004de424
uint32_t s_lego_mslbLast;

/* The tutorial icon's bob phase. */
// GLOBAL: LEGORR 0x004de58c
float s_lego_tutorialIconPhase;

/* Maths.h's Vector3F. */
typedef struct LegoVector3F3C { float x, y, z; } LegoVector3F3C;

/* Lego_Block +0x14: the drill damage (0 to 1). */
#define LEGO_BLOCK_DAMAGE(block) (*(float*)&(block)->f_10[1])

extern uint32_t s_lego_mslbLast;                /* 004de424 */
extern float s_lego_tutorialIconPhase;          /* 004de58c */

/* Shared under MSVC x86 (the port keeps its models in lego.cpp elsewhere). */
int terrain_cell_drill_work_add_current(int bx, int by, float destroyTime,
                                        float elapsed);
void terrain_cell_work_flag_circle_set_current(const int center_xy[2],
                                               int radius);
/* Shared: ledger_stub_00435550_GlobalStateZeroTransitionDetect is declared
 * with group 3B's entries above. */

/* The unit's own entries (the port keeps its models in lego.cpp). */
void lego_panels_config_entry_apply(struct BlockRec* config,
                                    unsigned int screenWidth,
                                    unsigned int screenHeight);
void lego_panel_button_layout_config_load(struct BlockRec* config,
                                          unsigned int screenWidth,
                                          unsigned int screenHeight);
void lego_advisor_tutorial_icon_image_load(struct BlockRec* config);
void lego_sound_samples_config_load(struct BlockRec* config,
                                    int noReduceSamples);
void lego_text_messages_config_load(struct BlockRec* config);
void lego_message_load_infomessages(struct BlockRec* config);
void lego_advisor_tooltip_and_sound_config_load(struct BlockRec* config);
int lego_interface_spawn_request_enqueue(int objType, int objIndex);
void lego_surface_type_descriptions_config_load(struct BlockRec* config,
                                                const char* gameName);
const char* lego_terrain_surface_type_name_get(int surfaceType);
uint32_t lego_surface_type_configured_value_at_index(int surfaceType);
void lego_random_variant_cell_scatter(const int* blockPos, int radius,
                                      int stopAtFirst);
void lego_tutorial_block_icons_update(Lego_Level* level, void* view,
                                      float elapsedWorld, float elapsed);
int lego_menu_option_panel_texture_update(const char* title, const char* msg,
                                          const char* ok, const char* cancel);
int lego_keep_running(void);
void lego_camera_registry_probe(void);
void lego_panel_selection_highlight_toggle(int on);

/* gods98 Image.c (images_unit.cpp): Image_SetPenZeroTrans (0047dbc0). */
void resource_record_add_ref(struct Image* record);
/* GAME-030 Panel.c (panel_unit.h). */
int ui_panel_name_table_lookup(const char* name, int* panel);   /* 0045a370 */
void legacy_overlay_image_record_load(const char* fileName, int panel,
                                      unsigned int flags);      /* 0045a2e0 */
int ui_panel_indexed_name_lookup(int panel, const char* name,
                                 unsigned int* button);         /* 0045a410 */
void panel_category_bounds_set(int panel, int xOut, int yOut, int xIn,
                               int yIn);                        /* 0045a650 */
void panel_item_flag8_set_if_enabled(int panel, int button, int on); /* 0045abf0 */
void panel_texture_slots_allocate(int panel, unsigned int count,
                                  const unsigned int* buttonTypes,
                                  const float* areas,
                                  char** normalNames,
                                  char** highlightNames,
                                  char** pressedNames,
                                  const int* toolTips);         /* 0045ac30 */
/* ToolTip.c (frontend.hpp, tooltip.cpp): ToolTip_GetType (0046b4d0). */
int case_insensitive_name_table_lookup(const char* name, int* out_index);
/* SFX.c (sound.hpp, sfx.cpp): 00464c50, 00464d00. */
void sound_alias_registration_set(int enable);
int sound_sample_marker_parse(const char* spec, int alias_index);
/* Text.c (text.c in the matching build; message.hpp, text_port.cpp): 0046aad0, 0046ab10, 0046abb0. */
int ui_message_label_index_find(const char* label, int* out_index);
void ui_message_text_slot_assign(int index, const char* text);
void ui_message_text_slot_assign_with_image(int index, const char* text,
                                            const char* image_path,
                                            const char* display_path);
/* GAME-019 Info.c (info_unit.h). */
int info_get_type(const char* name, int* type);                 /* 004193e0 */
void info_set_type_auto_game_speed(int type, int on);           /* 00419470 */
void info_set_text(int type, char* text);                       /* 004194b0 */
void info_set_type_play_always(int type, int on);               /* 004194f0 */
void info_set_image_file(int type, const char* filename);       /* 00419650 */
void info_set_sfx(int type, int sfxType);                       /* 00419680 */
/* Stats.c (entity.hpp): 00469ee0. */
uint32_t stats_table_word_at_148(int type, int row);
/* LegoObject.c (entity.hpp, legoobject_port.cpp): 0043f150. The original's first
 * argument is the teleporter; the port's entry does not read it. */
struct GameContext;
int entity_spawn_request_queue_add(struct GameContext* ctx, int type,
                                   int request_kind);
/* lego.cpp (level.hpp): 004332f0, 00433e30. */
void terrain_cell_work_flag_set_current(uint32_t cx, uint32_t cy);
int grid_cell_entity_eligibility_check(int x, int y);
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, 00433e30 above. A function-like
 * macro keeps this declaration, so the symbol counter does not move
 * (finding 634). */
#define grid_cell_entity_eligibility_check(x, y) \
    lego_grid_cell_entity_eligibility_check(x, y)
#endif
/* level.cpp (level_for_lego.hpp): legoGlobs.langSurface_desc (005562d8)
 * and langSurface_sound (00556320). In the matching build they are
 * legoGlobs' fields (lego_globs.h). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
extern const char* s_surface_type_names[18];
extern uint32_t s_surface_type_values[18];
#endif
/* GAME-026 Map3D.c (terrain_map.h): 0044f4e0, 0044f540. */
int terrain_map_block_highlight_set(struct TerrainMap* map, int bx, int by,
                                    int highlight);
int terrain_map_block_highlight_get(struct TerrainMap* map, int bx, int by);
/* GAME-020 Interface.c (interface_unit.h), read by offset. */
extern struct InterfaceGlobs interfaceGlobs;                    /* 004dcd58 */
/* gods98 TextWindow.c (heap.hpp): TextWindow_Update (00485390). */
int text_buffer_draw_wrapped(void* record, uint32_t start_line,
                             float line_scale, int* height_out);
/* GAME-015 FrontEnd.c (frontend_api.h): 00416d50, 00416d60. */
int save_slot_open(void);
int is_in_game(void);
/* GAME-031 Reward.c (reward.h). */
int reward_prepare_level(void);                                 /* 0045fda0 */
void reward_free_level(void);                                   /* 004601e0 */
int reward_prepare_results(void);                               /* 00460360 */
int reward_loop(void);                                          /* 00461410 */
/* Stats.c / record_chains.cpp (entity.hpp): 0046a4c0. */
int entity_flag_mask_callbacks_all_pass(uint32_t mask);


/* TerrainCellProgressUpdate [group 3C]: Level_Block_Damage. Adds
 * elapsed / destroyTime to the block's damage and reports whether it is
 * past 1. Shared under MSVC x86 (the level's pointer words). */
#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x00434570
int terrain_cell_drill_work_add_current(int bx, int by, float destroyTime,
                                        float elapsed)
{
    Lego_Level* level = legoGlobs_currLevel;

    if (0.0f != destroyTime) {
        LEGO_BLOCK_DAMAGE(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx]) += elapsed / destroyTime;
        if (LEGO_BLOCK_DAMAGE(&LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx]) > 1.0f) return 1;
    }
    return 0;
}
#endif

/* PanelsConfigEntryApply [group 3C]: Lego_LoadPanels. */
// FUNCTION: LEGORR 0x004345f0
void lego_panels_config_entry_apply(struct BlockRec* config,
                                    unsigned int screenWidth,
                                    unsigned int screenHeight)
{
    char buff[256];
    struct BlockRec* arrayItem;
    int panelType;
    char* argv[10];

    // STRING: LEGORR 0x004a47d8
    sprintf(buff, "Panels%ix%i", screenWidth, screenHeight);
    for (arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, buff, 0));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        if (ui_panel_name_table_lookup(arrayItem->key, &panelType)) {
            strcpy(buff, arrayItem->value);
            sound_variant_split(buff, argv, ",");
            legacy_overlay_image_record_load(argv[0], panelType, 4);
            panel_category_bounds_set(panelType, atoi(argv[1]), atoi(argv[2]),
                                      atoi(argv[3]), atoi(argv[4]));
        }
    }
}

/* PanelButtonLayoutConfigLoad [group 3C]: Lego_LoadPanelButtons. */
// FUNCTION: LEGORR 0x00434710
void lego_panel_button_layout_config_load(struct BlockRec* config,
                                          unsigned int screenWidth,
                                          unsigned int screenHeight)
{
    char buff[256];
    char* argv[9];
    unsigned int buttonCount[12];
    unsigned int buttonTypes[12][24];
    char* normalNames[12][24];
    char* highlightNames[12][24];
    char* pressedNames[12][24];
    float areas[12][24][4];
    int toolTips[12][24];
    char* copies[100];
    unsigned int copyCount;
    struct BlockRec* arrayItem;
    int panelType;
    unsigned int buttonType;
    unsigned int i;

    copyCount = 0;
    memset(buttonCount, 0, sizeof(buttonCount));
    // STRING: LEGORR 0x004a47e4
    sprintf(buff, "Buttons%ix%i", screenWidth, screenHeight);
    if ((arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, buff, 0))) != NULL) {
        do {
            if (ui_panel_name_table_lookup(arrayItem->key, &panelType)) {
                copies[copyCount] = string_duplicate(arrayItem->value);
                sound_variant_split(copies[copyCount], argv, ",");
                if (ui_panel_indexed_name_lookup(panelType, argv[0], &buttonType)) {
                    buttonTypes[panelType][buttonCount[panelType]] = buttonType;
                    for (i = 0; i < 3; i++) {
                        char** names = (i == 0) ? normalNames[panelType] : (i == 1) ? highlightNames[panelType] : pressedNames[panelType];
                        if (_strcmpi("NULL", argv[1 + i]) == 0) names[buttonCount[panelType]] = NULL;
                        else names[buttonCount[panelType]] = argv[1 + i];
                    }
                    areas[panelType][buttonCount[panelType]][0] = (float)atoi(argv[4]);
                    areas[panelType][buttonCount[panelType]][1] = (float)atoi(argv[5]);
                    areas[panelType][buttonCount[panelType]][2] = (float)atoi(argv[6]);
                    areas[panelType][buttonCount[panelType]][3] = (float)atoi(argv[7]);
                    if (case_insensitive_name_table_lookup(argv[8], &toolTips[panelType][buttonCount[panelType]]) == 0)
                        toolTips[panelType][buttonCount[panelType]] = 0;
                    buttonCount[panelType]++;
                    copyCount++;
                }
            }
        } while ((arrayItem = block_next_equal_key(arrayItem)) != NULL);

        for (i = 0; i < 12; i++) {
            panel_texture_slots_allocate(i, buttonCount[i], buttonTypes[i], &areas[i][0][0],
                                         normalNames[i], highlightNames[i], pressedNames[i],
                                         toolTips[i]);
        }
        for (; copyCount != 0; copyCount--) free(copies[copyCount - 1]);
    }
}

/* AdvisorTutorialIconImageLoad [group 3C]: Lego_LoadTutorialIcon. */
// FUNCTION: LEGORR 0x00434a00
void lego_advisor_tutorial_icon_image_load(struct BlockRec* config)
{
    char* fileName;

    if ((fileName = block_get_str_copy(config, string_path_join(s_engine_config_root, "Main",
        // STRING: LEGORR 0x004a47f4
        "TutorialIcon", 0))) != NULL) {
        if ((legoGlobs_TutorialIcon = image_load_bmp_scaled(fileName, 0, 0)) != NULL) {
            resource_record_add_ref(legoGlobs_TutorialIcon);
        }
    }
}

/* SoundSamplesConfigLoad [group 3C]: Lego_LoadSamples. A leading '!' marks
 * a sample skipped when reduceSamples is set. */
// FUNCTION: LEGORR 0x00434a50
void lego_sound_samples_config_load(struct BlockRec* config, int noReduceSamples)
{
    struct BlockRec* arrayItem;
    const char* name;
    int sfxType;
    int skip;

    // STRING: LEGORR 0x004a4804
    if ((arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, "Samples", 0))) != NULL) {
        sound_alias_registration_set(1);
        do {
            name = arrayItem->key;
            skip = 0;
            if (*name == '!') {
                name++;
                if (!noReduceSamples) skip = 1;
            }
            if (!skip) {
                if (sound_alias_lookup(name, &sfxType)) {
                    sound_sample_marker_parse(arrayItem->value, sfxType);
                }
            }
        } while ((arrayItem = block_next_equal_key(arrayItem)) != NULL);
        sound_alias_registration_set(0);
    }
}

/* TextMessagesConfigLoad [group 3C]: Lego_LoadTextMessages. */
// FUNCTION: LEGORR 0x00434af0
void lego_text_messages_config_load(struct BlockRec* config)
{
    char buff[512];
    struct BlockRec* arrayItem;
    int textType;
    char* argv[10];

    // STRING: LEGORR 0x004a4824
    for (arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, "TextMessages", 0));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        if (ui_message_label_index_find(arrayItem->key, &textType)) {
            ui_message_text_slot_assign(textType, arrayItem->value);
        }
    }
    // STRING: LEGORR 0x004a480c
    for (arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, "TextMessagesWithImages", 0));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        if (ui_message_label_index_find(arrayItem->key, &textType)) {
            strcpy(buff, arrayItem->value);
            sound_variant_split(buff, argv, s_config_rgb_separator);
            ui_message_text_slot_assign_with_image(textType, argv[0], argv[1], argv[2]);
        }
    }
}

/* IncidentMessageConfigLoad [group 3C]: Lego_LoadInfoMessages. */
// FUNCTION: LEGORR 0x00434c10
void lego_message_load_infomessages(struct BlockRec* config)
{
    char buff[1024];
    struct BlockRec* arrayItem;
    int infoType;
    int sfxType;
    char* argv[10];
    unsigned int argc;

    // STRING: LEGORR 0x004a484c
    for (arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, "InfoMessages", 0));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        if (info_get_type(arrayItem->key, &infoType)) {
            strcpy(buff, arrayItem->value);
            argc = sound_variant_split(buff, argv, s_config_rgb_separator);
            info_set_text(infoType, argv[0]);
            info_set_image_file(infoType, argv[1]);
            if (sound_alias_lookup(argv[2], &sfxType)) info_set_sfx(infoType, sfxType);
            if (argc > 3) {
                // STRING: LEGORR 0x004a4844
                if (!_strcmpi(argv[3], "SLOW")) info_set_type_auto_game_speed(infoType, 1);
                else info_set_type_auto_game_speed(infoType, 0);
                // STRING: LEGORR 0x004a4834
                if (!_strcmpi(argv[3], "ALWAYSPLAYSFX")) info_set_type_play_always(infoType, 1);
                else if (argc > 4 && !_strcmpi(argv[4], "ALWAYSPLAYSFX")) info_set_type_play_always(infoType, 1);
                else info_set_type_play_always(infoType, 0);
            } else {
                info_set_type_auto_game_speed(infoType, 1);
                info_set_type_play_always(infoType, 0);
            }
        }
    }
}

/* AdvisorTooltipAndTooltipSoundConfigLoad [group 3C]: Lego_LoadToolTips. */
// FUNCTION: LEGORR 0x00434da0
void lego_advisor_tooltip_and_sound_config_load(struct BlockRec* config)
{
    struct BlockRec* arrayItem;
    int toolTipType;
    int sfxType;

    // STRING: LEGORR 0x004a486c
    for (arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, "ToolTips", 0));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        if (case_insensitive_name_table_lookup(arrayItem->key, &toolTipType)) {
            tooltip_set_content(toolTipType, arrayItem->value);
        }
    }
    // STRING: LEGORR 0x004a485c
    for (arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, "ToolTipsSFXs", 0));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        if (case_insensitive_name_table_lookup(arrayItem->key, &toolTipType) &&
            sound_alias_lookup(arrayItem->value, &sfxType)) {
            timed_effect_cue_request_set_corpus(toolTipType, sfxType);
        }
    }
}

/* InterfaceSpawnRequestEnqueue [group 3C]: starts placing a building (the
 * two directional types) or teleports the object in from the first
 * teleporter that takes its kind. */
// FUNCTION: LEGORR 0x00434e80
int lego_interface_spawn_request_enqueue(int objType, int objIndex)
{
    LegoVector3F3C dir;
    int direction;

    s_global_low_word &= ~0x80;
    mode_flag_pair_apply_if_bit2(1);
    if (objType == 4 || objType == 11) {
        s_global_low_word |= 0x80;
        legoGlobs_f_556440 = objType;
        legoGlobs_f_556444 = objIndex;
        ddiscene_object_facing_pair_get(LEGO_FIELD(s_zoom_step_context, void*, 0x24), NULL,
                                        (float*)&dir, NULL);
        if (fabs(dir.x) > fabs(dir.y)) {
            if (dir.x > 0.0f) direction = 3;
            else direction = 1;
        } else {
            if (dir.y > 0.0f) direction = 2;
            else direction = 0;
        }
        legoGlobs_f_556448 = direction;
        return 1;
    }
    if (stats_table_word_at_148(objType, objIndex) & 0x1000)
        return entity_spawn_request_queue_add((struct GameContext*)legoGlobs_f_55644c, objType, objIndex);
    if (stats_table_word_at_148(objType, objIndex) & 0x800)
        return entity_spawn_request_queue_add((struct GameContext*)legoGlobs_f_556450, objType, objIndex);
    if (stats_table_word_at_148(objType, objIndex) & 0x2000)
        return entity_spawn_request_queue_add((struct GameContext*)legoGlobs_f_556454, objType, objIndex);
    if (stats_table_word_at_148(objType, objIndex) & 0x400000)
        return entity_spawn_request_queue_add(NULL, objType, objIndex);
    return 0;
}

/* TerrainCellWorkFlagCircleSet [group 3C]: Level_Block_SetBusyArea?
 * Raises the work flag on the blocks within the radius. Shared under
 * MSVC x86 (the level's pointer words). */
#if defined(_MSC_VER) && defined(_M_IX86)
/* The original keeps the level in a stack slot across the call and reloads
 * the radius after it; MSVC 5 keeps the level in a register in both C
 * forms tried (the bounds tests inline: 46.73%; with radius * radius and
 * the block coordinates in locals: 28.04%). The port's body, the
 * original's code, is kept; the C form was:
 *
 *   void terrain_cell_work_flag_circle_set_current(const int center_xy[2],
 *                                                  int radius)
 *   {
 *       Lego_Level* level = legoGlobs_currLevel;
 *       int radiusSq = radius * radius;
 *       int x, y;
 *
 *       for (y = -radius; y <= radius; y++) {
 *           for (x = -radius; x <= radius; x++) {
 *               if (x * x + y * y <= radiusSq) {
 *                   int bx = center_xy[0] + x;
 *                   if (bx >= 0 && bx < (int)level->width) {
 *                       int by = center_xy[1] + y;
 *                       if (by >= 0 && by < (int)level->height)
 *                           terrain_cell_work_flag_set_current(bx, by);
 *                   }
 *               }
 *           }
 *       }
 *   }
 */
// FUNCTION: LEGORR 0x00435010
__declspec(naked) void terrain_cell_work_flag_circle_set_current(
    const int center_xy[2], int radius)
{
    __asm {
        sub esp, 0ch
        mov eax, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 14h]
        /* mov dword ptr [esp + 0], eax, in the original's encoding (a zero
         * 8-bit displacement); the inline assembler emits the short form,
         * which left the function a byte short of the original. */
        _emit 0x89
        _emit 0x44
        _emit 0x24
        _emit 0x00
        mov eax, edx
        imul eax, edx
        push esi
        mov esi, edx
        neg esi
        push edi
        mov edi, esi
        cmp edi, edx
        mov dword ptr [esp + 0ch], eax
        mov dword ptr [esp + 10h], esi
        jg terrain_cell_work_flag_circle_set_early_done
        push ebp
        mov ebp, dword ptr [esp + 1ch]
        push ebx
    terrain_cell_work_flag_circle_set_outer_loop:
        mov ebx, edi
        imul ebx, edi
        cmp esi, edx
        jg terrain_cell_work_flag_circle_set_outer_next
    terrain_cell_work_flag_circle_set_inner_loop:
        mov ecx, esi
        mov eax, dword ptr [esp + 14h]
        imul ecx, esi
        add ecx, ebx
        cmp ecx, eax
        jg terrain_cell_work_flag_circle_set_next_column
        mov eax, dword ptr [ebp]
        lea ecx, dword ptr [esi + eax]
        test ecx, ecx
        jl terrain_cell_work_flag_circle_set_next_column
        mov eax, dword ptr [esp + 10h]
        cmp ecx, dword ptr [eax + 14h]
        jge terrain_cell_work_flag_circle_set_next_column
        mov eax, dword ptr [ebp + 4]
        add eax, edi
        test eax, eax
        jl terrain_cell_work_flag_circle_set_next_column
        mov edx, dword ptr [esp + 10h]
        cmp eax, dword ptr [edx + 18h]
        jge terrain_cell_work_flag_circle_set_after_mark
        push eax
        push ecx
        call terrain_cell_work_flag_set_current
        add esp, 8
    terrain_cell_work_flag_circle_set_after_mark:
        mov edx, dword ptr [esp + 24h]
    terrain_cell_work_flag_circle_set_next_column:
        inc esi
        cmp esi, edx
        jle terrain_cell_work_flag_circle_set_inner_loop
        mov esi, dword ptr [esp + 18h]
    terrain_cell_work_flag_circle_set_outer_next:
        inc edi
        cmp edi, edx
        jle terrain_cell_work_flag_circle_set_outer_loop
        pop ebx
        pop ebp
    terrain_cell_work_flag_circle_set_early_done:
        pop edi
        pop esi
        add esp, 0ch
        ret
    }
}
#endif

/* legoGlobs.langSurface_name, langSurface_desc and langSurface_sound: three
 * adjacent arrays (00556290, 005562d8, 00556320). */
typedef struct LegoSurfaceDescs3C {
    const char* name[18];
    const char* desc[18];
    uint32_t sfx[18];
} LegoSurfaceDescs3C;
#define LEGO_SURFACE_DESCS_3C ((LegoSurfaceDescs3C*)legoGlobs_langSurface_name)

/* SurfaceTypeDescriptionsConfigLoad [group 3C]:
 * Lego_LoadSurfaceTypeDescriptions_sound. */
// FUNCTION: LEGORR 0x004350a0
void lego_surface_type_descriptions_config_load(struct BlockRec* config,
                                                const char* gameName)
{
    char buff[1024];
    char* argv[10];
    const char* str;
    int sfxType;
    uint32_t* sfx;

    for (sfx = s_surface_type_values; sfx < &s_surface_type_values[18]; sfx++) {
        // STRING: LEGORR 0x004a4878
        if ((str = block_get_strptr(config, string_path_join(gameName, "SurfaceTypeDescriptions",
                                                             ((const char**)sfx)[-36], 0))) != NULL) {
            strcpy(buff, str);
            sound_variant_split((char*)str, argv, ",");
            ((const char**)sfx)[-18] = formatted_text_escape_decode_and_emit(argv[0]);
            *sfx = sound_alias_lookup(argv[1], &sfxType) ? sfxType : 0;
        }
    }
}

/* TerrainSurfaceTypeNameGet [group 3C]: Lego_GetSurfaceTypeDescription. */
// FUNCTION: LEGORR 0x00435170
const char* lego_terrain_surface_type_name_get(int surfaceType)
{
    const char* name = s_surface_type_names[surfaceType];
    // STRING: LEGORR 0x004a4890
    if (name == NULL) name = "Need config for surface type.";
    return name;
}

/* SurfaceTypeConfiguredValueAtIndex [group 3C]:
 * Lego_GetSurfaceTypeSFX. */
// FUNCTION: LEGORR 0x00435190
uint32_t lego_surface_type_configured_value_at_index(int surfaceType)
{
    return s_surface_type_values[surfaceType];
}

/* ActionCodeToSoundAlias [group 3C]: the pointer for a block's surface
 * action. Shared (the port's callers use the original's contract). */
// FUNCTION: LEGORR 0x004351a0
void media_action_code_to_sound_alias(int code)
{
    if (code == 1) {
        media_pointer_cursor_select(0x2b);
        return;
    }
    if (code == 2) {
        media_pointer_cursor_select(0x2c);
        return;
    }
    if (code == 3) {
        media_pointer_cursor_select(0x2d);
        return;
    }
    if (code == 4) {
        media_pointer_cursor_select(0x2e);
        return;
    }
    if (code == 5) {
        media_pointer_cursor_select(0x2f);
        return;
    }
    if (code == 8) {
        media_pointer_cursor_select(0x32);
        return;
    }
    if (code == 10) {
        media_pointer_cursor_select(0x34);
        return;
    }
    if (code == 11) {
        media_pointer_cursor_select(0x35);
        return;
    }
}

/* RandomVariantCellScatter [group 3C]: random lava erosion around a block
 * (Level_GenerateLavaErosion-like). Stops at the first erosion when
 * stopAtFirst is set; otherwise tries a quarter of the box. */
// FUNCTION: LEGORR 0x00435230
void lego_random_variant_cell_scatter(const int* blockPos, int radius, int stopAtFirst)
{
    int done[100];
    unsigned int count;
    unsigned int size;
    unsigned int dx, dy;
    int pos[2];

    memset(done, 0, sizeof(done));
    size = radius * 2;
    for (count = 0; count < (stopAtFirst ? size * size : (size * size) / 4); count++) {
        dx = rng_thunk_draw15() % size;
        dy = rng_thunk_draw15() % size;
        pos[0] = (dx + blockPos[0]) - radius;
        pos[1] = (blockPos[1] + dy) - radius;
        if (!done[dx * 10 + dy]) {
            if (lava_erosion_cell_eligible_and_mutate(pos, 1) && stopAtFirst) return;
            done[dx * 10 + dy] = 1;
        }
    }
}

/* FUN_00435300 [group 3C]: draws the tutorial icon over the blocks the
 * tutorial flags and highlights them. */
// FUNCTION: LEGORR 0x00435300
void lego_tutorial_block_icons_update(Lego_Level* level, void* view,
                                      float elapsedWorld, float elapsed)
{
    float bob = (float)sin(s_lego_tutorialIconPhase);
    unsigned int bx, by;
    Lego_Block* block;
    float wPos[3];
    float sPos[2];

    s_lego_tutorialIconPhase -= elapsed * 25.0f * -0.013333334f;
    if (system_state_flags_get_original(NULL)) {
        for (by = 0; by < ((unsigned int*)LEGO_LEVEL_PTR(void, level->map))[1]; by++) {
            for (bx = 0; bx < ((unsigned int*)LEGO_LEVEL_PTR(void, level->map))[0]; bx++) {
                if (!grid_cell_entity_eligibility_check(bx, by)) {
                    block = &LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx];
                    if (block->f_04) {
                        terrain_map_block_to_world(LEGO_LEVEL_PTR(struct TerrainMap, level->map), bx, by,
                                                   &wPos[0], &wPos[1]);
                        wPos[2] = terrain_map_world_z_get(LEGO_LEVEL_PTR(struct TerrainMap, level->map),
                                                          wPos[0], wPos[1]);
                        camera_project_point_to_screen((const struct ViewProjectionAdapter*)view, sPos, wPos);
                        sPos[0] -= (float)(legoGlobs_TutorialIcon->width / 2);
                        sPos[1] -= bob * 20.0f + (float)(legoGlobs_TutorialIcon->height * 2);
                        image_display_scaled(legoGlobs_TutorialIcon, NULL, (const struct Point2F*)sPos, NULL);
                        if (LEGO_FIELD(&interfaceGlobs, int, 0xfc4)) {
                            terrain_map_block_highlight_set(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), bx, by, 5);
                        } else {
                            terrain_map_block_highlight_clear(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), bx, by);
                        }
                        LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].f_10[13] = 1;
                    } else if (block->f_10[13] == 1) {
                        block->f_10[13] = 0;
                        if (!(system_state_flags_get_original(NULL) & 2) &&
                            LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].f_10[6] &&
                            !LEGO_FIELD(&interfaceGlobs, int, 0xf74)) {
                            terrain_map_block_highlight_set(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), bx, by, 4);
                        } else {
                            terrain_map_block_highlight_clear(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), bx, by);
                        }
                    } else if (block->f_10[13] == 0 &&
                               terrain_map_block_highlight_get(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), bx, by) == 5) {
                        terrain_map_block_highlight_clear(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), bx, by);
                    }
                }
            }
        }
    }
}

/* GlobalStateZeroTransitionDetect [group 3C]: Lego_IsLeftButtonReleased-
 * like. Shared (reward.c and the port call it with the original's
 * contract). */
// FUNCTION: LEGORR 0x00435550
int ledger_stub_00435550_GlobalStateZeroTransitionDetect(void)
{
    uint32_t current = legoInput_mslb;
    int released = current == 0 && s_lego_mslbLast != 0;
    s_lego_mslbLast = current;
    return released;
}

/* GlobalHandlePositionApplyIfPresent [group 3C]: draws the dialog's
 * contrast overlay at the origin. */
// FUNCTION: LEGORR 0x00435580
int lego_global_handle_position_apply_if_present(void)
{
    float pos[2];

    pos[0] = 0.0f;
    pos[1] = 0.0f;
    if (s_position_handle) {
        image_display_scaled((struct Image*)s_position_handle, NULL, (const struct Point2F*)pos, NULL);
        return 1;
    }
    return 0;
}

/* MenuOptionPanelTextureUpdate [group 3C]: draws the in-game dialog and
 * returns 1 for OK, 0 for Cancel and -1 for neither. */
// FUNCTION: LEGORR 0x004355c0
int lego_menu_option_panel_texture_update(const char* title, const char* msg,
                                          const char* ok, const char* cancel)
{
    float pos[2];
    float wndRect[4];
    float textX;
    unsigned int textWidth;
    unsigned int textHeight;
    int result = -1;

    if (legoGlobs_DialogImage && legoGlobs_DialogTextWndTitle && legoGlobs_DialogTextWndMessage) {
        lego_global_handle_position_apply_if_present();
        pos[0] = (float)(s_display_w / 2) - (float)(legoGlobs_DialogImage->width >> 1);
        pos[1] = (float)(s_display_h / 2) - (float)(legoGlobs_DialogImage->height >> 1);
        image_display_scaled(legoGlobs_DialogImage, NULL, (const struct Point2F*)pos, NULL);
        text_buffer_reset_target(legoGlobs_DialogTextWndTitle);
        text_buffer_reset_target(legoGlobs_DialogTextWndMessage);
        text_buffer_reset_target(legoGlobs_DialogTextWndOK);
        text_buffer_reset_target(legoGlobs_DialogTextWndCancel);
        if (ok) {
            wndRect[0] = 0.0f;
            wndRect[1] = 0.0f;
            wndRect[2] = 0.0f;
            textHeight = object_field_be4_dword_get(legoGlobs_fontBriefingHi);
            textWidth = formatted_text_printf_width((const struct TextureSurface*)legoGlobs_fontBriefingHi, ok);
            textX = ((float)LEGO_FIELD(legoGlobs_DialogTextWndOK, int, 4) -
                     (float)LEGO_FIELD(legoGlobs_DialogTextWndOK, int, 12) * -0.5f) - (float)(textWidth >> 1);
            wndRect[0] = (float)LEGO_FIELD(legoGlobs_DialogTextWndOK, int, 8);
            wndRect[2] = (float)textHeight;
            if (legoInput_mouseX < textX || legoInput_mouseX >= textX + textWidth ||
                legoInput_mouseY < wndRect[0] || legoInput_mouseY >= wndRect[0] + wndRect[2]) {
                LEGO_FIELD(legoGlobs_DialogTextWndOK, void*, 0) = legoGlobs_fontBriefingHi;
                text_buffer_append_format(legoGlobs_DialogTextWndOK, ok);
            } else {
                LEGO_FIELD(legoGlobs_DialogTextWndOK, void*, 0) = legoGlobs_fontBriefingLo;
                text_buffer_append_format(legoGlobs_DialogTextWndOK, ok);
                if (ledger_stub_00435550_GlobalStateZeroTransitionDetect()) result = 1;
            }
        }
        if (cancel) {
            wndRect[0] = 0.0f;
            wndRect[1] = 0.0f;
            wndRect[2] = 0.0f;
            textWidth = formatted_text_printf_width((const struct TextureSurface*)legoGlobs_fontBriefingHi, cancel);
            textHeight = object_field_be4_dword_get(legoGlobs_fontBriefingHi);
            textX = ((float)LEGO_FIELD(legoGlobs_DialogTextWndCancel, int, 4) -
                     (float)LEGO_FIELD(legoGlobs_DialogTextWndCancel, int, 12) * -0.5f) - (float)(textWidth >> 1);
            wndRect[0] = (float)LEGO_FIELD(legoGlobs_DialogTextWndCancel, int, 8);
            wndRect[2] = (float)textHeight;
            if (legoInput_mouseX < textX || legoInput_mouseX >= textX + textWidth ||
                legoInput_mouseY < wndRect[0] || legoInput_mouseY >= wndRect[0] + wndRect[2]) {
                LEGO_FIELD(legoGlobs_DialogTextWndCancel, void*, 0) = legoGlobs_fontBriefingHi;
                text_buffer_append_format(legoGlobs_DialogTextWndCancel, cancel);
            } else {
                LEGO_FIELD(legoGlobs_DialogTextWndCancel, void*, 0) = legoGlobs_fontBriefingLo;
                text_buffer_append_format(legoGlobs_DialogTextWndCancel, cancel);
                if (ledger_stub_00435550_GlobalStateZeroTransitionDetect()) result = 0;
            }
        }
        text_buffer_append_format(legoGlobs_DialogTextWndTitle, title);
        text_buffer_append_format(legoGlobs_DialogTextWndMessage, msg);
        text_buffer_draw_wrapped(legoGlobs_DialogTextWndTitle, 0, 1.0f, NULL);
        text_buffer_draw_wrapped(legoGlobs_DialogTextWndMessage, 0, 1.0f, NULL);
        text_buffer_draw_wrapped(legoGlobs_DialogTextWndOK, 0, 1.0f, NULL);
        text_buffer_draw_wrapped(legoGlobs_DialogTextWndCancel, 0, 1.0f, NULL);
        return result;
    }
    return -1;
}

/* KeepRunning [group 3C]: Lego_EndLevel. Frees the level, runs the
 * rewards and the front end, and loads the next level. */
// FUNCTION: LEGORR 0x00435940
int lego_keep_running(void)
{
    int saveOpen, inGame;
    const char* levelName;

    level_current_owner_get();
    saveOpen = save_slot_open();
    inGame = is_in_game();
    sound_subsystem_stop_all();
    if (frontend_enabled_original()) {
        reward_prepare_level();
        reward_prepare_results();
        reward_loop();
        reward_free_level();
    }
    lego_sound_minimum_distance_update_mode(1, NULL, 0);
    text_buffer_reset_target(legoGlobs_textOnlyWindow);
    levelName = lego_level_free();
    if (frontend_enabled_original()) {
        if (saveOpen) {
            front_run_screen_menu_type_adapter(1);
            if (quit_menu_probe()) return 0;
            levelName = front_get_selected_level_adapter();
        } else if (inGame) {
            front_run_screen_menu_type_adapter(2);
            if (quit_menu_probe()) return 0;
            levelName = front_get_selected_level_adapter();
        }
    }
    options_load_or_default_target(1, 0);
    if (levelName && lego_level_load(levelName)) {
        free((void*)levelName);
        return 1;
    }
    return 0;
}

/* GlobalStateLowByteClearAndRefresh [group 3C]: clears the low byte of
 * flags3 and hides the placement marker. Shared. */
// FUNCTION: LEGORR 0x00435a20
void global_state_low_byte_clear_and_refresh(void)
{
    /* The original clears eight one-bit fields of legoGlobs.flags3, one
     * `and` each; MSVC folds the same masks written as integers. */
    struct LegoFlags3Bits3C {
        unsigned int b0 : 1, b1 : 1, b2 : 1, b3 : 1, b4 : 1, b5 : 1, b6 : 1, b7 : 1;
    } *bits = (struct LegoFlags3Bits3C*)&s_global_low_word;
    bits->b1 = 0;
    bits->b2 = 0;
    bits->b3 = 0;
    bits->b4 = 0;
    bits->b5 = 0;
    bits->b6 = 0;
    bits->b7 = 0;
    bits->b0 = 0;
    surface_visibility_set(s_refresh_record, 1);
}

/* MainSimFeatureFlagLatch [group 3C]. */
// FUNCTION: LEGORR 0x00435a50
void lego_camera_registry_probe(void)
{
    if (legoGlobs_flags1 & 0x8000000) {
        if (entity_flag_mask_callbacks_all_pass(0x40b)) s_panel_highlight_flags |= 0x2;
    }
}

/* PanelSelectionHighlightToggle [group 3C]: Lego_SetCallToArmsOn. */
// FUNCTION: LEGORR 0x00435aa0
void lego_panel_selection_highlight_toggle(int on)
{
    if (on) {
        s_panel_highlight_flags |= 0x1;
        if (s_panel_highlight_timer <= 0.0f) {
            sound_alias_play(0x19, 0);
            s_panel_highlight_timer = 250.0f;
        }
        panel_item_flag8_set_if_enabled(6, 2, 1);
    } else {
        s_panel_highlight_flags &= ~0x1;
        s_panel_highlight_timer = 0.0f;
        panel_item_flag8_set_if_enabled(6, 2, 0);
    }
}

/* ---- part 3 functions, group 3C: end ---- */

/* Lego.c data the port defined in lego.cpp until GAME-021 part 3 (#587)
 * retired it to the port tail (lego_port.cpp). Defined last, so no
 * function's symbol counter changes (finding 538). */

/* Lego.c .bss: the scroll guard and the neighbour-pick rotation. */
// GLOBAL: LEGORR 0x004de548
int s_scroll_guard;
// GLOBAL: LEGORR 0x004de588
uint32_t s_neighbor_pick_rotation;

#ifdef __cplusplus
}
#endif
