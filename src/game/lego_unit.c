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
 * 005560c0-00556fb8). The port has no such struct: some fields are port
 * globals annotated in other files, which this unit reads directly. The
 * fields the port does not model are defined here, one annotated global
 * each, as stand-ins until the port's owners are converted. */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "lego_unit.h"
#include "lego_unit_imports.h"

/* ------------------------------------------------------------------ */
/* State: the unit's .bss and the legoGlobs stand-ins.                 */
/* ------------------------------------------------------------------ */

/* ---- state, group M ---- */

/* legoGlobs.cameraFP (005560ec). */
// GLOBAL: LEGORR 0x005560ec
void* legoGlobs_cameraFP;

// GLOBAL: LEGORR 0x005560f8
uint32_t legoGlobs_f_5560f8;

// GLOBAL: LEGORR 0x00556104
uint32_t legoGlobs_f_556104;

/* legoGlobs.selectedObject? The object the first-person view follows. */
// GLOBAL: LEGORR 0x00556160
void* legoGlobs_selectedObject;

/* legoGlobs.flags1 (00556ecc). The port keeps its copy of this word in the
 * EngineContext (engine_shared_state_word). */
// GLOBAL: LEGORR 0x00556ecc
uint32_t legoGlobs_flags1;
/* ---- state, group A ---- */

/* legoGlobs fields Lego_Initialise sets (005560c0 + offset). */
// GLOBAL: LEGORR 0x005560c0
struct BlockRec* legoGlobs_config;

// GLOBAL: LEGORR 0x005560d8
void* legoGlobs_rootCont;

// GLOBAL: LEGORR 0x005560dc
void* legoGlobs_viewMain;

// GLOBAL: LEGORR 0x005560e0
void* legoGlobs_viewTrack;

// GLOBAL: LEGORR 0x005560e8
void* legoGlobs_cameraTrack;

// GLOBAL: LEGORR 0x00556114
float legoGlobs_TVClipDist;

// GLOBAL: LEGORR 0x0055612c
void* legoGlobs_fontDesktop;

// GLOBAL: LEGORR 0x00556130
void* legoGlobs_fontTextWindow;

// GLOBAL: LEGORR 0x00556134
void* legoGlobs_fontBriefingHi;

// GLOBAL: LEGORR 0x00556138
void* legoGlobs_fontBriefingLo;

// GLOBAL: LEGORR 0x00556140
void* legoGlobs_textOnlyWindow;

// GLOBAL: LEGORR 0x00556144
void* legoGlobs_textImageWindow;

/* The surface types' language keys. */
// GLOBAL: LEGORR 0x00556290
const char* legoGlobs_langSurface_name[18];

/* The radar view's area (16, 13, 151, 151); its zoom (005563f4) is
 * lego.cpp's s_lego_radar_zoom. */
// GLOBAL: LEGORR 0x005563e4
float legoGlobs_f_5563e4[4];

// GLOBAL: LEGORR 0x00556400
float legoGlobs_MedPolyRange;

// GLOBAL: LEGORR 0x00556404
float legoGlobs_HighPolyRange;

// GLOBAL: LEGORR 0x00556408
int legoGlobs_HPBlocks;

// GLOBAL: LEGORR 0x00556428
float legoGlobs_PowerCrystalRGB[3];

// GLOBAL: LEGORR 0x00556434
float legoGlobs_UnpoweredCrystalRGB[3];

// GLOBAL: LEGORR 0x00556458
float legoGlobs_MinEnergyForEat;

// GLOBAL: LEGORR 0x00556464
float legoGlobs_DynamiteDamageRadius;

// GLOBAL: LEGORR 0x00556468
float legoGlobs_DynamiteMaxDamage;

// GLOBAL: LEGORR 0x0055646c
float legoGlobs_DynamiteWakeRadius;

// GLOBAL: LEGORR 0x00556470
float legoGlobs_BirdScarerRadius;

// GLOBAL: LEGORR 0x00556518
float legoGlobs_MiniFigureRunAway;

// GLOBAL: LEGORR 0x00556e94
int legoGlobs_MaxReturnedCrystals;

// GLOBAL: LEGORR 0x00556e98
int legoGlobs_MouseScrollBorder;

// GLOBAL: LEGORR 0x00556ec4
float legoGlobs_MinDistFor3DSoundsOnTopView;

// GLOBAL: LEGORR 0x00556ef4
struct Image* legoGlobs_BackArrowImage;

// GLOBAL: LEGORR 0x00556f04
float legoGlobs_DrainTime;

// GLOBAL: LEGORR 0x00556f08
int legoGlobs_ReinforceHits;

// GLOBAL: LEGORR 0x00556f0c
int legoGlobs_CDStartTrack;

// GLOBAL: LEGORR 0x00556f10
uint32_t legoGlobs_CDTracks;

/* Three factors set to 0.2, 0.7 and 1.0 at start-up. */

// GLOBAL: LEGORR 0x00556f38
struct Image* legoGlobs_DialogImage;

// GLOBAL: LEGORR 0x00556f40
void* legoGlobs_DialogTextWndTitle;

// GLOBAL: LEGORR 0x00556f44
void* legoGlobs_DialogTextWndMessage;

// GLOBAL: LEGORR 0x00556f48
void* legoGlobs_DialogTextWndOK;

// GLOBAL: LEGORR 0x00556f4c
void* legoGlobs_DialogTextWndCancel;

// GLOBAL: LEGORR 0x00556f98
int legoGlobs_BuildingUpgradeCostOre;

// GLOBAL: LEGORR 0x00556f9c
int legoGlobs_BuildingUpgradeCostStuds;

// GLOBAL: LEGORR 0x00556fac
char* legoGlobs_RenameReplace;


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

// GLOBAL: LEGORR 0x005560f0
void* legoGlobs_spotlightTop;

// GLOBAL: LEGORR 0x005560f4
void* legoGlobs_spotlightTrack;

// GLOBAL: LEGORR 0x005560fc
void* legoGlobs_dirLightFP;

// GLOBAL: LEGORR 0x00556100
void* legoGlobs_ambientLight;

// GLOBAL: LEGORR 0x0055610c
void* legoGlobs_dirLightCallToArms;

// GLOBAL: LEGORR 0x00556148
float legoGlobs_f_556148;

// GLOBAL: LEGORR 0x0055614c
float legoGlobs_f_55614c;

// GLOBAL: LEGORR 0x00556150
float legoGlobs_f_556150;

// GLOBAL: LEGORR 0x00556154
float legoGlobs_f_556154;

// GLOBAL: LEGORR 0x00556158
int legoGlobs_f_556158;

// GLOBAL: LEGORR 0x00556180
int legoGlobs_f_556180;

// GLOBAL: LEGORR 0x00556184
int legoGlobs_f_556184;

// GLOBAL: LEGORR 0x00556188
int legoGlobs_f_556188;

// GLOBAL: LEGORR 0x00556ec0
int legoGlobs_f_556ec0;

// GLOBAL: LEGORR 0x00556edc
float legoGlobs_NextButtonPos[2];

// GLOBAL: LEGORR 0x00556ee4
float legoGlobs_BackButtonPos[2];

// GLOBAL: LEGORR 0x00556eec
struct Image* legoGlobs_NextButtonImage;

// GLOBAL: LEGORR 0x00556ef0
struct Image* legoGlobs_BackButtonImage;

/* legoGlobs.renameInput: the name being typed, or NULL. */
// GLOBAL: LEGORR 0x00556fa0
char* legoGlobs_renameInput;

// GLOBAL: LEGORR 0x00556fa4
float legoGlobs_renamePosition[2];

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

/* The container that follows the pointer's world position. */
// GLOBAL: LEGORR 0x00556108
void* legoGlobs_f_556108;

/* The pointer's world position when a selection drag starts. */
// GLOBAL: LEGORR 0x00556164
float legoGlobs_f_556164[3];

/* The block under the pointer, and the block under it during a drag, as
 * floats. */
// GLOBAL: LEGORR 0x00556170
float legoGlobs_f_556170[2];

// GLOBAL: LEGORR 0x00556178
float legoGlobs_f_556178[2];

/* The block a vehicle placement points at, and whether it is valid. */

/* The radar view's centre when it does not follow an object. */
// GLOBAL: LEGORR 0x005563f8
LegoPoint2FC legoGlobs_f_5563f8;

/* The object type and index a building placement places. */
// GLOBAL: LEGORR 0x00556440
int legoGlobs_f_556440;

// GLOBAL: LEGORR 0x00556444
int legoGlobs_f_556444;

/* The screen border (pixels) that scrolls the top-down view. */

/* The two objective-panel images and their positions. */

/* legoGlobs.mouseBlockPos: the block under the pointer. */
// GLOBAL: LEGORR 0x00556fb8
int legoGlobs_f_556fb8[2];
/* ---- state, group D ---- */

/* legoGlobs.config (005560c0): the loaded Lego.cfg. */

/* The object manager LoadTileMeshes registers the tile meshes with. */

/* A float factor of the fog distance (with the level's block size). */
// GLOBAL: LEGORR 0x00556110
float legoGlobs_FPClipBlocks;

/* The objective tick's first argument. */

/* The type data the Precreate key creates its ten mini-figures from. */
// GLOBAL: LEGORR 0x005561d8
LegoCreatureModelE* legoGlobs_miniFigureData;

/* FogColourRGB and HighFogColourRGB. */
// GLOBAL: LEGORR 0x0055640c
float legoGlobs_fogColourRGB[3];

// GLOBAL: LEGORR 0x00556418
float legoGlobs_highFogColourRGB[3];

// GLOBAL: LEGORR 0x00556424
float legoGlobs_f_556424;

// GLOBAL: LEGORR 0x0055644c
uint32_t legoGlobs_f_55644c;

// GLOBAL: LEGORR 0x00556450
uint32_t legoGlobs_f_556450;

/* Not noFallins. */

/* FogRate * 25. */
// GLOBAL: LEGORR 0x00556ef8
float legoGlobs_f_556ef8;

/* FallinMultiplier (1 when unset). The port keeps a copy in camera.cpp
 * (g_camera_scale_14, unannotated). */
// GLOBAL: LEGORR 0x00556f14
int legoGlobs_f_556f14;

/* DragBoxRGB, 0-1 each. */
// GLOBAL: LEGORR 0x00556f2c
float legoGlobs_dragBoxRGB[3];

/* Gates the AllowRename key. */

// GLOBAL: LEGORR 0x00556fb0
char* legoGlobs_endGameAVI1;

// GLOBAL: LEGORR 0x00556fb4
char* legoGlobs_endGameAVI2;

/* The first mini-figure the object list loads (OListEntityRecordParse sets
 * it); the StartFP view follows it. Outside legoGlobs. */
// GLOBAL: LEGORR 0x004e8448
void* s_lego_f_4e8448;
/* ---- state, group E ---- */

/* legoGlobs.config (005560c0): the game's Lego.cfg block root. */

/* legoGlobs.rootCont (005560d8). */

/* legoGlobs.viewMain (005560dc). */

/* legoGlobs.cameraTrack (005560e8). */

/* legoGlobs.bmpFONT5_HI (00556124). */

/* legoGlobs.bmpToolTipFont (00556128). */

/* legoGlobs.textOnlyWindow (00556140). */

/* legoGlobs.vehicleData (005561d4): 0x1ec-byte records. */
// GLOBAL: LEGORR 0x005561d4
LegoVehicleModelE* legoGlobs_vehicleData;

/* legoGlobs.miniFigureData (005561d8): 0x74-byte records. */

/* legoGlobs.rockMonsterData (005561dc): 0x74-byte records. */
// GLOBAL: LEGORR 0x005561dc
LegoCreatureModelE* legoGlobs_rockMonsterData;

/* legoGlobs.buildingData (005561e0): 0x14c-byte records. */
// GLOBAL: LEGORR 0x005561e0
LegoBuildingModelE* legoGlobs_buildingData;

/* legoGlobs.vehicleName (005561e8). */
// GLOBAL: LEGORR 0x005561e8
char** legoGlobs_vehicleName;

/* legoGlobs.miniFigureName (005561ec). */
// GLOBAL: LEGORR 0x005561ec
char** legoGlobs_miniFigureName;

/* legoGlobs.rockMonsterName (005561f0). */
// GLOBAL: LEGORR 0x005561f0
char** legoGlobs_rockMonsterName;

/* legoGlobs.buildingName (005561f4). */
// GLOBAL: LEGORR 0x005561f4
char** legoGlobs_buildingName;

/* legoGlobs.upgradeName (005561f8). */
// GLOBAL: LEGORR 0x005561f8
char** legoGlobs_upgradeName;

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

/* legoGlobs' MiscObjects containers (00556368-005563b0). */
// GLOBAL: LEGORR 0x00556368
void* legoGlobs_contBoulder;
// GLOBAL: LEGORR 0x0055636c
void* legoGlobs_contBoulderExplode;
// GLOBAL: LEGORR 0x00556370
void* legoGlobs_contBoulderExplodeIce;
// GLOBAL: LEGORR 0x00556374
void* legoGlobs_contCrystal;
// GLOBAL: LEGORR 0x00556378
void* legoGlobs_contDynamite;
// GLOBAL: LEGORR 0x0055637c
void* legoGlobs_contOresTable;
// GLOBAL: LEGORR 0x00556380
void* legoGlobs_contProcessedOre;
// GLOBAL: LEGORR 0x00556384
void* legoGlobs_contOohScary;
// GLOBAL: LEGORR 0x00556388
void* legoGlobs_contBarrier;
// GLOBAL: LEGORR 0x0055638c
void* legoGlobs_contElectricFence;
// GLOBAL: LEGORR 0x00556390
void* legoGlobs_contSpiderWeb;
// GLOBAL: LEGORR 0x00556394
void* legoGlobs_contRechargeSparkle;
// GLOBAL: LEGORR 0x00556398
void* legoGlobs_contMiniTeleportUp;
// GLOBAL: LEGORR 0x0055639c
void* legoGlobs_contElectricFenceStud;
// GLOBAL: LEGORR 0x005563a0
void* legoGlobs_contPusher;
// GLOBAL: LEGORR 0x005563a4
void* legoGlobs_contFreezer;
// GLOBAL: LEGORR 0x005563a8
void* legoGlobs_contIceCube;
// GLOBAL: LEGORR 0x005563ac
void* legoGlobs_contSmashPath;
// GLOBAL: LEGORR 0x005563b0
void* legoGlobs_contLaserShot;

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

/* LegoObject.c's tool tip icons (004ead78-004eadbc): one per ability, one
 * per carried object type, and the blank icon. No port owner yet. */
// GLOBAL: LEGORR 0x004ead78
void* s_object_toolTipIconAbilities[6];
// GLOBAL: LEGORR 0x004ead90
void* s_object_toolTipIcons[11];
// GLOBAL: LEGORR 0x004eadbc
void* s_object_toolTipIconBlank;
/* ---- state, group G ---- */

/* legoGlobs.config (005560c0), the struct's first field: see
 * LEGO_GLOBS_SIZE. */

/* A text window GameConfigInit creates (legoGlobs +0x80). */

/* The world position the drag box started at. */

/* The ToolTipInfo strings. */
// GLOBAL: LEGORR 0x00556e9c
char* legoGlobs_healthText;

// GLOBAL: LEGORR 0x00556ea0
char* legoGlobs_energyText;

// GLOBAL: LEGORR 0x00556ea4
char* legoGlobs_crystalsText;

// GLOBAL: LEGORR 0x00556ea8
char* legoGlobs_oreText;

// GLOBAL: LEGORR 0x00556eac
char* legoGlobs_studsText;

// GLOBAL: LEGORR 0x00556eb0
char* legoGlobs_toolsText;

// GLOBAL: LEGORR 0x00556eb4
char* legoGlobs_carryObjectText;

// GLOBAL: LEGORR 0x00556eb8
char* legoGlobs_drivenByText;

// GLOBAL: LEGORR 0x00556ebc
char* legoGlobs_oreRequiredText;

// GLOBAL: LEGORR 0x00556f58
char* legoGlobs_upgradeNames[16];

/* Text.c's word after the two the port's text.cpp models (00503610,
 * 00503614); the port has no owner for it. */
// GLOBAL: LEGORR 0x00503618
uint32_t textGlobs_f_503618;


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

/* ---- part 2 state, group 2E: end ---- */

/* ---- part 2 state, group 2F (004302b0-00431570): begin ---- */

/* ---- part 2 state, group 2F: end ---- */

/* ---- part 2 state, group 2G (00431580-00432490): begin ---- */

/* ---- part 2 state, group 2G: end ---- */


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

            if (surface_description_get((int*)&totalMemValue, (int*)&availMemF)) {
                if ((textureUsageE = LEGO_A_CFG_INT(cfgStr, string_path_join(s_engine_config_root, "Main",
                    // STRING: LEGORR 0x004a3bb8
                    "TextureUsage", 0))) != 0) {
                    query_texture_memory_original(&textureUsageE);
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
                if (sound_variant_split((char*)progressWindowStr, argvVal, ":|") == 4 && progressWindowStr[1] == ':' && barDirA != 4) {
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
                        if (sound_variant_split(posString, posArgvStr, ":|") == 2) {
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
                        if (sound_variant_split(posString, posArgvStr, ":|") == 2) {
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
                    upgrade_types_load_original() && entity_vehicle_type_table_load() &&
                    entity_minifigure_type_table_load() && entity_rockmonster_type_table_load() &&
                    entity_building_types_load())
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

                    entity_object_names_load(cfgStr);
                    entity_object_the_names_load(cfgStr);

                    if (block_get_bool_path(cfgStr, string_path_join(s_engine_config_root, "Main",
                        // STRING: LEGORR 0x004a3904
                        "HelpWindowOn", 0)) == 1) {
                        selection_mode_state_set((uint32_t)(uintptr_t)legoGlobs_fontBriefingLo);
                        help_window_initialise_adapter(cfgStr, s_engine_config_root);
                    }
                    frontend_render_three_pass_update(cfgStr, s_engine_config_root);
                    light_effect_configuration_load(cfgStr, s_engine_config_root);
                    stats_initialise_adapter(cfgStr, s_engine_config_root);
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

    fileName = block_get_strptr(config, string_path_join(s_engine_config_root, "MiscObjects", "SpiderWeb", NULL));
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
    int x;
    int y;
    unsigned int width;
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
                     "%s%s%s",
                     // STRING: LEGORR 0x004a402c
                     s_lego_renameCursorOn ? "\"" : "",
                     text,
                     s_lego_renameCursorOn ? "\"" : "");
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
    camera_grid_timers_original(level_current_owner_get(), elapsed);
    camera_grid_expiry_original(elapsedWorld);
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
    simtick_minion_states_original(legoGlobs_currLevel, elapsedWorld);
    cmd_queue_tick();
    view_tick_original(s_zoom_step_context, legoGlobs_currLevel, elapsed, elapsedWorld);
    view_tick_original(legoGlobs_cameraTrack, legoGlobs_currLevel, elapsed, elapsedWorld);
    view_tick_original(legoGlobs_cameraFP, legoGlobs_currLevel, elapsed, elapsedWorld);
    simtick_bobbins_original(elapsedWorld);
    interface_update_hover_sfx(elapsedAbs);
    lego_scene_marker_update_and_draw(elapsed, 0);
    simtick_render_step_original();
    simtick_post_cluster_original(elapsedWorld);
    lego_main_sim_interaction_and_cell_update(elapsedAbs, elapsed, keyDownT, keyDownR, keyDownShift);
    terrain_map_update(LEGO_LEVEL_PTR(struct TerrainMap, legoGlobs_currLevel->map), elapsedWorld);
    simtick_fx_bank_tick_original(legoGlobs_currLevel, elapsedWorld);
    simtick_sound_keepalive_original();
    transient_effect_instances_tick(elapsedWorld);
    simtick_post_step_original(elapsedWorld);
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

    overlay_text_buffer_refresh();
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
            volume = (int)(constant_negative_10000_get() / 10.0f * (legoGlobs_currLevel->oxygenLevel / 10.0f));
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
    lego_level_names_free_adapter();
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
    projection_overlay_draw_submit_adapter(0, 0);
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
            legoGlobs_renameInput = LEGO_FIELD(liveObj, char*, 0x8);
            legoGlobs_renamePosition[0] = sPos[0];
            legoGlobs_renamePosition[1] = sPos[1];
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
                                        legoGlobs_f_556184 = by;
                                        legoGlobs_f_556180 = bx;
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
                        legoGlobs_f_556164[0] = wPos.x;
                        legoGlobs_f_556164[1] = wPos.y;
                        legoGlobs_f_556164[2] = wPos.z;
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
        if (_strnicmp(prefix, LEGO_FIELD(item, char*, 0x04),
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
            text = terrain_surface_type_name_get(0);
            sfx = surface_type_configured_value_at_index(0);
            show = 1;
        } else if (!(flags & 0x200000)) {
            text = terrain_surface_type_name_get(0xc);
            sfx = surface_type_configured_value_at_index(0xc);
            show = 1;
        } else if (block->flags2 & 0x20) {
            text = terrain_surface_type_name_get(0xf);
            sfx = surface_type_configured_value_at_index(0xf);
            show = 1;
        } else {
            if (bx >= 0 && by >= 0 && bx < (int)legoGlobs_currLevel->width && by < (int)legoGlobs_currLevel->height)
                valid = LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * by + bx].flags1 & 0x20000000;
            else
                valid = 0;
            if (valid || (flags & 0x8000)) {
                text = terrain_surface_type_name_get(0xe);
                sfx = surface_type_configured_value_at_index(0xe);
                show = 1;
            } else if (!(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * by + bx].flags1 & 0x400) &&
                       !(LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)[legoGlobs_currLevel->width * by + bx].flags2 & 4) &&
                       !(flags & 0x100000)) {
                if (showCavern) {
                    text = terrain_surface_type_name_get(0x11);
                    sfx = surface_type_configured_value_at_index(0x11);
                } else {
                    text = terrain_surface_type_name_get(0);
                    sfx = surface_type_configured_value_at_index(0);
                }
                show = 1;
            }
        }
        if (block->f_02 == 9) {
            text = terrain_surface_type_name_get(block->f_02);
            sfx = surface_type_configured_value_at_index(block->f_02);
            show = 1;
        } else if (block->f_02 == 6) {
            text = terrain_surface_type_name_get(block->f_02);
            sfx = surface_type_configured_value_at_index(block->f_02);
            show = 1;
        }
    } else {
        if (!(flags & 0x10) && (flags & 0x20000)) {
            text = terrain_surface_type_name_get(0x10);
            sfx = surface_type_configured_value_at_index(0x10);
        } else if (flags & 0x20) {
            text = terrain_surface_type_name_get(0xd);
            sfx = surface_type_configured_value_at_index(0xd);
        } else {
            text = terrain_surface_type_name_get(block->f_02);
            sfx = surface_type_configured_value_at_index(block->f_02);
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
        terrain_neighbor_flag_set((struct LevelState*)legoGlobs_currLevel, 0, bx, by);
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
        terrain_cell_mutate((struct LevelState*)legoGlobs_currLevel, bx, by, 0);
    if (s_global_current_keys[0x51])
        terrain_cell_initialize((struct LevelState*)legoGlobs_currLevel, bx, by);

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
        expand_cell_entity_spawn_adapter(bx, by);
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
            viewport_target_position_clamp_original(legoGlobs_selectedObject,
                                                    0, 0);
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
#define LEGO_D_STR(key) (LEGO_D_TEMP(key) ? LEGO_D_TEMP(key) : "")
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
    char buff[260];
    char* parts[10];
    float upgradeTime;
    float value;
    float triggerTime;
    float erodeTime;
    float lockTime;
    int seed;
    int number;
    unsigned int i;
    int objType;
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
        if ((seed = LEGO_D_INT("Seed")) != 0) srand(seed);

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

        // STRING: LEGORR 0x004a4338
        if ((copy = LEGO_D_COPY("DragBoxRGB")) != NULL) {
            char* argv[10];
            if (sound_variant_split(copy, argv, ":") == 3) {
                for (i = 0; i < 3; i++) {
                    legoGlobs_dragBoxRGB[i] = (float)atoi(argv[i]) * (1.0f / 255.0f);
                }
            }
            free(copy);
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
            if (entity_type_pair_for_name_original(str, &objType, (int*)&level->f_26c, NULL)) {
                // STRING: LEGORR 0x004a42ec
                value = LEGO_D_FLOAT("SlugTime");
                level->f_268 = value;
                if (0.0f == value) level->f_268 = 60.0f;
                level->f_268 *= 25.0f;
                // STRING: LEGORR 0x004a42dc
                value = LEGO_D_FLOAT("InitialSlugTime");
                if (0.0f == value) value = 60.0f;
                g_camera_countdown = value * 25.0f;
            }
        } else level->f_26c = 20;

        // STRING: LEGORR 0x004a42cc
        str = LEGO_D_TEMP("RockFallStyle");
        rock_fall_style_name_lookup(str, &rockFallStyle);
        rockfall_style_table_base_set(rockFallStyle);
        // STRING: LEGORR 0x004a42c0
        legoGlobs_f_556ec0 = (LEGO_D_BOOL("noFallins") != 1);

        mission_crystal_quota_setup(config, s_engine_config_root, levelName);
        entity_type_pair_for_name_original(emergeCreature, &objType, (int*)&level->f_0b0, NULL);

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
                if (0.0f == (value = LEGO_D_FLOAT("FogRate"))) value = 60.0f;
                legoGlobs_f_556ef8 = value * 25.0f;
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
        copy = LEGO_D_COPY("FullName");
        LEGO_LEVEL_PTR_SET(level->fullName, copy);
        if (copy) {
            for (i = 0; i < strlen(LEGO_LEVEL_PTR(char, level->fullName)); i++) {
                LEGO_LEVEL_PTR(char, level->fullName)[i] =
                    (LEGO_LEVEL_PTR(char, level->fullName)[i] == '_') ? ' ' : (unsigned char)LEGO_LEVEL_PTR(char, level->fullName)[i];
            }
        } else LEGO_LEVEL_PTR_SET(level->fullName, NULL);

        objective_load_original(config, s_engine_config_root, levelName, level, s_display_w, s_display_h);

        // STRING: LEGORR 0x004a4274
        if (0.0f == (triggerTime = LEGO_D_FLOAT("ErodeTriggerTime"))) triggerTime = 10.0f;
        // STRING: LEGORR 0x004a4264
        if (0.0f == (erodeTime = LEGO_D_FLOAT("ErodeErodeTime"))) erodeTime = 5.0f;
        // STRING: LEGORR 0x004a4254
        if (0.0f == (lockTime = LEGO_D_FLOAT("ErodeLockTime"))) lockTime = 60.0f;
        erosion_timing_state_init(triggerTime, erodeTime, lockTime);
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
            int surfaceParam, predugParam = 0, terrainParam = 0, blockPointersParam = 0;
            int cryOreParam = 0, pathParam = 0, fallinParam;

            if (surfaceMap && sound_variant_split(surfaceMap, parts, ":") == 2) surfaceParam = atoi(parts[1]);
            if (predugMap && sound_variant_split(predugMap, parts, ":") == 2) predugParam = atoi(parts[1]);
            if (terrainMap && sound_variant_split(terrainMap, parts, ":") == 2) terrainParam = atoi(parts[1]);
            if (blockPointersMap && sound_variant_split(blockPointersMap, parts, ":") == 2)
                blockPointersParam = atoi(parts[1]);
            if (cryOreMap && sound_variant_split(cryOreMap, parts, ":") == 2) cryOreParam = atoi(parts[1]);
            if (pathMap && sound_variant_split(pathMap, parts, ":") == 2) pathParam = atoi(parts[1]);
            if (fallinMap && sound_variant_split(fallinMap, parts, ":") == 2) fallinParam = atoi(parts[1]);
            if (emergeMap && sound_variant_split(emergeMap, parts, ":") == 2) pathParam = atoi(parts[1]);

            if (map_pipeline_original(level, surfaceMap, predugMap, predugParam, terrainMap, terrainParam,
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
                grid_cell_record_buffer_replace((struct EngineState*)level);
                objective_announce_current(0);
                objective_tick_target(legoGlobs_textOnlyWindow, (struct LevelState*)legoGlobs_currLevel, 1.0f, 1.0f);

                if (object_list_load_file_original(level, objectList)) {
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
                    lego_play_movie_adapter(introAVI, pos);

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

/* ---- part 2 functions, group 2A: end ---- */

/* ---- part 2 functions, group 2B (0042c480-0042d460): begin ---- */

/* ---- part 2 functions, group 2B: end ---- */

/* ---- part 2 functions, group 2C (0042d600-0042dff0): begin ---- */

/* ---- part 2 functions, group 2C: end ---- */

/* ---- part 2 functions, group 2D (0042e020-0042ef40): begin ---- */

/* ---- part 2 functions, group 2D: end ---- */

/* ---- part 2 functions, group 2E (0042efc0-0042f6f0): begin ---- */

/* ---- part 2 functions, group 2E: end ---- */

/* ---- part 2 functions, group 2F (004302b0-00431570): begin ---- */

/* ---- part 2 functions, group 2F: end ---- */

/* ---- part 2 functions, group 2G (00431580-00432490): begin ---- */

/* TerrainNeighborWorkFlagScan [group 2G]: a breadth-first sweep from one
 * block. Every block in the 3x3 neighbourhood of a listed position whose
 * flags1 carries 0x20000 is updated (Level_Block mutation, mode 1), loses
 * the flag and is listed for the next pass. Two position lists alternate. */
// FUNCTION: LEGORR 0x00431580
void lego_terrain_neighbor_work_flag_scan(unsigned int bx, unsigned int by)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned int listSize = (level->width + level->height) * 2;
    unsigned int counts[2];
    LegoPoint2F2G* lists[2];
    unsigned int curr;
    unsigned int i;
    LegoPoint2F2G pos;
    int x, y;

    counts[0] = 0;
    counts[1] = 0;
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
    sPos.x = x;
    sPos.y = y;
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
            if (terrain_cell_allows_placement_adapter(units[i], bx, by))
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

    blockPos[0] = bx;
    blockPos[1] = by;
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

/* Is the block inside the map's border, flags1 0x10 set and 8 clear? The
 * original inlines this test (it materialises the 0/1 result). */
static __inline int lego_block_flag10_without_8(Lego_Level* level,
                                                unsigned int bx, unsigned int by)
{
    int result = 0;
    if (bx < level->width - 1 && by < level->height - 1 &&
        (LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].flags1 & 0x10) &&
        !(LEGO_LEVEL_PTR(Lego_Block, level->blocks)[by * level->width + bx].flags1 & 8))
        result = 1;
    return result;
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

    s_neighbor_pick_rotation++;
    for (i = 0; i < 4; i++) {
        dir = (s_neighbor_pick_rotation + i) & 3;
        if (!skipWalls ||
            !lego_block_flag10_without_8(legoGlobs_currLevel, DIRECTIONS[dir][0] + blockPos[0],
                                         DIRECTIONS[dir][1] + blockPos[1])) {
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
 * pointer over the current list and writes the previous one at +0x320. The
 * port keeps them as separate globals (level.cpp), from which no C form
 * reproduces that addressing (the separate-array form scores 23.88%; a
 * one-struct model of the lists gives the original's loop). So the
 * matching build keeps the original's code, as the port did; the C form is
 * for other compilers. */
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
        mov ecx, offset s_cell_refresh_track
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

// FUNCTION: LEGORR 0x00432300
void lego_terrain_cell_visited_flags_clear_all(void)
{
    Lego_Level* level = legoGlobs_currLevel;
    unsigned int i;

    for (i = 0; i < (unsigned int)s_cell_visited_track_count; i++) {
        LEGO_LEVEL_PTR(Lego_Block, level->blocks)
            [s_cell_visited_track[i][1] * level->width + s_cell_visited_track[i][0]].flags2 &= ~2;
    }
    s_cell_visited_track_count = 0;
}

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
