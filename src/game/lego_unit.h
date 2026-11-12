// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-021 Lego.c (original translation unit, finding 505), part 1 of 3
 * (issue #544): the original-shaped records the unit reads, the legoGlobs
 * stand-ins and the unit's functions.
 *
 * C-compatible header (ADR 0005). lego_unit.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the shared functions.
 *
 * The offsets are the original's Win32 layout. lego.h stays the boundary
 * header of the port's Lego.c entry points that other C units call. */
#ifndef LEGORRR_GAME_LEGO_UNIT_H
#define LEGORRR_GAME_LEGO_UNIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- group M ---- */

/* Lego_Level (0x284 bytes): the current level, legoGlobs.currLevel
 * (005560d4). The port's LevelState begins with a 32-bit word image of the
 * same layout on every target (level.hpp), so the shared accessors below
 * read the port's live level natively too. Its pointer fields are pointers
 * only in the Win32 builds; elsewhere they are image words, and
 * LEGO_LEVEL_PTR reads one as a pointer (unit-only code, which the native
 * port compiles but never calls). Unnamed fields are named by offset. */
#if defined(_WIN32) && !defined(_WIN64)
#define LEGO_LEVEL_PTR_FIELD(T) T*
#define LEGO_LEVEL_PTR(T, field) (field)
#else
#define LEGO_LEVEL_PTR_FIELD(T) uint32_t
#define LEGO_LEVEL_PTR(T, field) ((T*)(uintptr_t)(field))
#endif

typedef struct Lego_Block Lego_Block;
typedef struct Lego_Level {
    LEGO_LEVEL_PTR_FIELD(char) name;            /* +0x000 */
    LEGO_LEVEL_PTR_FIELD(void) map;             /* +0x004 Map3D */
    LEGO_LEVEL_PTR_FIELD(void) emergeMap;       /* +0x008 */
    uint32_t f_00c;
    uint32_t f_010;
    uint32_t width;                 /* +0x014 */
    uint32_t height;                /* +0x018 */
    float blockSize;                /* +0x01c */
    float digDepth;                 /* +0x020 */
    float roofHeight;               /* +0x024 */
    float roughLevel;               /* +0x028 */
    int useRoof;                    /* +0x02c */
    int safeCaverns;                /* +0x030 */
    float selBoxHeight;             /* +0x034 */
    uint32_t f_038[15];             /* +0x038 */
    uint32_t f_074;                 /* +0x074 tile mesh grid width */
    uint32_t f_078;                 /* +0x078 tile mesh grid height */
    LEGO_LEVEL_PTR_FIELD(void) textureSet;      /* +0x07c Detail_TextureSet */
    LEGO_LEVEL_PTR_FIELD(void*) f_080;          /* +0x080 tile meshes, f_074 * f_078 */
    uint32_t f_084;                 /* +0x084 */
    uint32_t f_088;
    int f_08c;                      /* +0x08c */
    uint32_t f_090;
    int f_094;                      /* +0x094 */
    int f_098;                      /* +0x098 */
    int f_09c;                      /* +0x09c */
    int crystals;                   /* +0x0a0 */
    uint32_t f_0a4;
    int f_0a8;                      /* +0x0a8 */
    int ore;                        /* +0x0ac */
    uint32_t f_0b0;                 /* +0x0b0 */
    LEGO_LEVEL_PTR_FIELD(char) nextLevel;       /* +0x0b4 */
    LEGO_LEVEL_PTR_FIELD(Lego_Block) blocks;     /* +0x0b8 width * height */
    uint32_t f_0bc[78];             /* +0x0bc objective data */
    int startFP;                    /* +0x1f4 */
    int noDrain;                    /* +0x1f8 */
    float oxygenLevel;              /* +0x1fc */
    float oxygenRate;               /* +0x200 */
    float f_204;                    /* +0x204 oxygen level of the last air message */
    float buildingTolerance;        /* +0x208 */
    float buildingMaxVariation;     /* +0x20c */
    float upgradeTimes[20];         /* +0x210 */
    float trainTime;                /* +0x260 */
    float emergeTimeOut;            /* +0x264 */
    float f_268;                    /* +0x268 */
    uint32_t f_26c;                 /* +0x26c */
    LEGO_LEVEL_PTR_FIELD(char) fullName;        /* +0x270 */
    uint32_t f_274;                 /* +0x274 BoulderAnimation */
    uint32_t f_278;
    int status;                     /* +0x27c */
    int disableStartTeleport;       /* +0x280 */
} Lego_Level;

/* Lego_Block (0x48 bytes): one map block, currLevel->blocks[y * width + x]. */
struct Lego_Block {
    unsigned char f_00;
    unsigned char texture;          /* +0x01 */
    unsigned char f_02;
    unsigned char f_03;
    unsigned char f_04;
    unsigned char f_05;
    unsigned char f_06;
    unsigned char f_07;
    uint32_t flags1;                /* +0x08 */
    uint32_t flags2;                /* +0x0c */
    uint32_t f_10[14];              /* +0x10 */
};

/* A field of an original record the port does not lay out as the original
 * (LegoObject, LegoCamera and the like), read by offset as the original
 * code does. The offsets are the Win32 layout; only the matching and
 * Win32 reference builds run this code. */
#define LEGO_FIELD(ptr, type, offset) (*(type*)((char*)(ptr) + (offset)))

/* legoGlobs.currLevel: the port's g_active_level_retail (terrain_grid.cpp,
 * 005560d4) through the original's view. */
#define legoGlobs_currLevel ((Lego_Level*)g_active_level_retail)

/* legoGlobs fields the port does not model: stand-ins defined in
 * lego_unit.c. */
extern uint32_t legoGlobs_flags1;               /* 00556ecc */
extern void* legoGlobs_cameraFP;                /* 005560ec */
extern uint32_t legoGlobs_f_5560f8;             /* 005560f8 */
extern uint32_t legoGlobs_f_556104;             /* 00556104 */
extern void* legoGlobs_selectedObject;          /* 00556160 */

/* Shared: their callers already use the original's contract. */
void global_object_field8c_increment(void);
void global_object_field8c_subtract(int amount);
void global_object_field98_add(int amount);
void global_object_field98_clear_current(void);
void noop_callback(void);
void level_state_field9c_add(int amount);
void global_object_field94_increment(void);
void global_object_fielda8_increment(void);
float EngineTimeScale(void);
float global_float_field_00556f00_get(void);
void global_scroll_guard_set(int value);
int current_engine_mode_is_zero(void);
void objective_score_text_origin_set(const float* pair);
void engine_global_state_flags_bits6_and_9_set_target(int value,
                                                      int set_bit9);
void engine_global_state_flags_bit7_set_target(int value);
void media_action_feedback_sound_dispatch(int event);
uint32_t active_engine_field_b0_get_current(void);
float engine_float_field210_get_by_index_current(int index);
float engine_float_field260_get_current_target(void);
uint32_t global_state_field84_get_current_target(void);

/* The unit's own entries (the port keeps its models in lego.cpp). */
int lego_upgrade_resource_check(int ore);
void lego_flags1_bit17_set(int on);
uint32_t lego_runtime_flag40000000_get(void);
void lego_level_resource_balance_add_and_sound(int ore);
void lego_upgrade_resource_debit(int ore, int amount);
void* lego_mode_dependent_field24_get(void);
void lego_game_speed_set(float speed);
void* lego_terrain_cell_palette_resource_at_grid_position(const int* xy);
void lego_global_triple_copy_out(uint32_t* out);
void lego_objective_task_text_origin_set(const float* pair);
uint32_t lego_context_selected_global_pointer_get(void);
int lego_entity_matches_current_selection_or_owner(const void* object);



/* ---- group A ---- */

/* Config.h's value macros, as the original expands them: the lookup and
 * the path are evaluated twice when the item exists, and a missing item
 * reads as "". */
#define LEGO_A_CFG_STR(config, id) \
    (block_get_strptr((config), (id)) ? block_get_strptr((config), (id)) : "")
#define LEGO_A_CFG_INT(config, id) atoi(LEGO_A_CFG_STR((config), (id)))
#define LEGO_A_CFG_FLOAT(config, id) atof(LEGO_A_CFG_STR((config), (id)))

/* legoGlobs fields read by Lego_Initialise (stand-ins in lego_unit.c). */
extern struct BlockRec* legoGlobs_config;  /* 005560c0 */
extern void* legoGlobs_rootCont;                   /* 005560d8 */
extern void* legoGlobs_viewMain;  /* 005560dc */
extern void* legoGlobs_viewTrack;                  /* 005560e0 */
extern void* legoGlobs_cameraTrack;                /* 005560e8 */
extern float legoGlobs_TVClipDist;                 /* 00556114 */
extern void* legoGlobs_fontDesktop;                /* 0055612c */
extern void* legoGlobs_fontTextWindow;             /* 00556130 */
extern void* legoGlobs_fontBriefingHi;             /* 00556134 */
extern void* legoGlobs_fontBriefingLo;             /* 00556138 */
extern void* legoGlobs_textOnlyWindow;             /* 00556140 */
extern void* legoGlobs_textImageWindow;            /* 00556144 */
extern const char* legoGlobs_langSurface_name[18]; /* 00556290 */
extern float legoGlobs_f_5563e4[4];                /* 005563e4 */
extern float legoGlobs_MedPolyRange;               /* 00556400 */
extern float legoGlobs_HighPolyRange;              /* 00556404 */
extern int legoGlobs_HPBlocks;                     /* 00556408 */
extern float legoGlobs_PowerCrystalRGB[3];         /* 00556428 */
extern float legoGlobs_UnpoweredCrystalRGB[3];     /* 00556434 */
extern float legoGlobs_MinEnergyForEat;            /* 00556458 */
extern float legoGlobs_DynamiteDamageRadius;       /* 00556464 */
extern float legoGlobs_DynamiteMaxDamage;          /* 00556468 */
extern float legoGlobs_DynamiteWakeRadius;         /* 0055646c */
extern float legoGlobs_BirdScarerRadius;           /* 00556470 */
extern float legoGlobs_MiniFigureRunAway;          /* 00556518 */
extern int legoGlobs_MaxReturnedCrystals;          /* 00556e94 */
extern int legoGlobs_MouseScrollBorder;            /* 00556e98 */
extern float legoGlobs_MinDistFor3DSoundsOnTopView; /* 00556ec4 */
extern float legoGlobs_NextButtonPos[2];           /* 00556edc */
extern float legoGlobs_BackButtonPos[2];           /* 00556ee4 */
extern struct Image* legoGlobs_NextButtonImage;    /* 00556eec */
extern struct Image* legoGlobs_BackButtonImage;    /* 00556ef0 */
extern struct Image* legoGlobs_BackArrowImage;     /* 00556ef4 */
extern float legoGlobs_DrainTime;                  /* 00556f04 */
extern int legoGlobs_ReinforceHits;                /* 00556f08 */
extern int legoGlobs_CDStartTrack;                 /* 00556f0c */
extern uint32_t legoGlobs_CDTracks;                     /* 00556f10 */
extern float legoGlobs_dragBoxRGB[3];                /* 00556f2c */
extern struct Image* legoGlobs_DialogImage;        /* 00556f38 */
extern void* legoGlobs_DialogTextWndTitle;         /* 00556f40 */
extern void* legoGlobs_DialogTextWndMessage;       /* 00556f44 */
extern void* legoGlobs_DialogTextWndOK;            /* 00556f48 */
extern void* legoGlobs_DialogTextWndCancel;        /* 00556f4c */
extern int legoGlobs_BuildingUpgradeCostOre;       /* 00556f98 */
extern int legoGlobs_BuildingUpgradeCostStuds;     /* 00556f9c */
extern char* legoGlobs_RenameReplace;              /* 00556fac */

int lego_game_config_init(void);
/* Other groups' functions, with the original signatures assumed here. */
void lego_misc_object_resources_load(struct BlockRec* config);   /* F 00422850 */
void lego_tooltip_info_strings_load(struct BlockRec* config,
                                    const char* gameName);       /* G 00427e00 */
void lego_level_upgrade_name_strings_load(struct BlockRec* config); /* G 00427f80 */
void lego_audio_playback_toggle(int on);                         /* G 00429810 */
int lego_level_load(const char* levelName);                      /* D 00429890 */



/* ---- group B ---- */

struct TextureSurface;
struct Image;

/* legoGlobs stand-ins (lego_unit.c). */
extern void* legoGlobs_spotlightTop;            /* 005560f0 */
extern void* legoGlobs_spotlightTrack;          /* 005560f4 */
extern void* legoGlobs_dirLightFP;              /* 005560fc */
extern void* legoGlobs_ambientLight;            /* 00556100 */
extern void* legoGlobs_dirLightCallToArms;      /* 0055610c */
extern float legoGlobs_f_556148;                /* 00556148 */
extern float legoGlobs_f_55614c;                /* 0055614c */
extern float legoGlobs_f_556150;                /* 00556150 */
extern float legoGlobs_f_556154;                /* 00556154 */
extern int legoGlobs_f_556158;                  /* 00556158 */
extern int legoGlobs_f_556180;                  /* 00556180 */
extern int legoGlobs_f_556184;                  /* 00556184 */
extern int legoGlobs_f_556188;                  /* 00556188 */
extern int legoGlobs_f_556ec0;  /* 00556ec0 */
extern char* legoGlobs_renameInput;             /* 00556fa0 */
extern float legoGlobs_renamePosition[2];       /* 00556fa4 */
/* gods98 Input's key maps (stand-ins, lego_unit.c). */
extern uint8_t s_global_current_keys[256];  /* input.cpp, 0076ab80 */
extern uint8_t s_global_previous_keys[256]; /* input.cpp, 0076aa00 */

void lego_debug_panel_text_draw_update(float elapsed);
void lego_engine_pre_tick(float elapsed);
int lego_engine_tick(float elapsed);

/* Other groups' functions, with the signatures group B assumes. */
void lego_start_level_pilots(void);                                   /* G 0041fa80 */
void lego_terrain_cell_flag_cleanup(Lego_Level* level, float elapsedWorld); /* E 00424600 */
void lego_sine_phase_midpoint_blend_dispatch(int on, float elapsedWorld);  /* E 00424730 */
void lego_default_transform_apply_all(void* view);                    /* E 00424810 */
int lego_main_sim_step(float elapsedWorld, float elapsed, int* keyDownT,
                       int* keyDownR, int* keyDownShift);             /* E 004250c0 */
void lego_transition_control(int on, unsigned int flags);             /* G 00425c30 */
void lego_terrain_pointer_command_update(void);                       /* F 00425d90 */
void lego_spawn_coordinate_pool_tick(float elapsedWorld);             /* F 004261c0 */
void lego_update_render_viewport_state(void);                         /* C 00426250 */
void lego_main_sim_interaction_and_cell_update(float elapsedAbs, float elapsed,
                                               int keyDownT, int keyDownR,
                                               int keyDownShift);     /* C 00426520 */
void lego_scene_marker_update_and_draw(float elapsed, int track);     /* G 004291a0 */
void lego_selection_rectangle_render(Lego_Level* level);               /* G 004293b0 */
void lego_cd_audio_watchdog_mode_update(int on);                      /* G 004297b0 */



/* ---- group C ---- */

/* Maths.h's Point2F and Vector3F. */
typedef struct LegoPoint2FC {
    float x, y;
} LegoPoint2FC;
typedef struct LegoVector3FC {
    float x, y, z;
} LegoVector3FC;

/* legoGlobs fields the port does not model (stand-ins in lego_unit.c). */
extern void* legoGlobs_f_556108;                /* 00556108 */
extern float legoGlobs_f_556164[3];             /* 00556164 */
extern float legoGlobs_f_556170[2];             /* 00556170 */
extern float legoGlobs_f_556178[2];             /* 00556178 */
extern LegoPoint2FC legoGlobs_f_5563f8;  /* 005563f8 */
extern int legoGlobs_f_556440;                  /* 00556440 */
extern int legoGlobs_f_556444;                  /* 00556444 */
extern int legoGlobs_f_556fb8[2];               /* 00556fb8 mouseBlockPos */

struct EntitySlot;
void lego_render_record_set_from_input(struct EntitySlot* object);
void lego_update_render_viewport_state(void);
void lego_cursor_ray_to_camera_route(float elapsed);
void lego_main_sim_interaction_and_cell_update(float elapsedGame,
                                               float elapsedAbs,
                                               int keyDownT, int keyDownR,
                                               int keyDownShift);
/* Other groups' functions, with the signatures group C assumed. */
void lego_entity_command_status_feedback_dispatch(struct EntitySlot* object); /* F 00428020 */
void lego_terrain_command_feedback_dispatch(const int* blockPos, int a,
                                            int b, int c);   /* F 00428330 */
int lego_terrain_cell_command_predicate(const int* blockPos); /* F 00428780 */



/* ---- group D ---- */

/* Writes a Lego_Level pointer field (see LEGO_LEVEL_PTR). */
#if defined(_WIN32) && !defined(_WIN64)
#define LEGO_LEVEL_PTR_SET(field, value) ((field) = (value))
#else
#define LEGO_LEVEL_PTR_SET(field, value) ((field) = (uint32_t)(uintptr_t)(value))
#endif

/* The record LevelLoad passes to EntityEffectCountersApplyFromRecord for
 * each object: the level's InitialCrystals and InitialOre. */
typedef struct LegoInitialResourcesD {
    int f_0;
    int crystals;                   /* +0x04 */
    int ore;                        /* +0x08 */
} LegoInitialResourcesD;

struct BlockRec;
struct EntitySlot;
extern float legoGlobs_FPClipBlocks;                /* 00556110 */
extern float legoGlobs_fogColourRGB[3];         /* 0055640c */
extern float legoGlobs_highFogColourRGB[3];     /* 00556418 */
extern float legoGlobs_f_556424;                /* 00556424 */
extern uint32_t legoGlobs_f_55644c;             /* 0055644c */
extern uint32_t legoGlobs_f_556450;             /* 00556450 */
extern float legoGlobs_f_556ef8;                /* 00556ef8 */
extern int legoGlobs_f_556f14;                  /* 00556f14 */
extern char* legoGlobs_endGameAVI1;             /* 00556fb0 */
extern char* legoGlobs_endGameAVI2;             /* 00556fb4 */

int lego_level_load(const char* levelName);
int lego_entity_effect_counters_apply_from_record(struct EntitySlot* object, void* data);
int lego_load_tile_meshes(Lego_Level* level, const char* baseName);
void lego_rectangular_resource_table_destroy(Lego_Level* level);
/* Group G's SetViewMode (004295f0); signature assumed here. */
void lego_sound_minimum_distance_update_mode(int mode, void* object, int cameraFrame);



/* ---- group E ---- */

/* The per-type model records legoGlobs keeps arrays of, by size only (the
 * shutdown passes their addresses to their owners' release functions). */
typedef struct LegoVehicleModelE { unsigned char bytes[0x1ec]; } LegoVehicleModelE;
typedef struct LegoCreatureModelE { unsigned char bytes[0x74]; } LegoCreatureModelE;
typedef struct LegoBuildingModelE { unsigned char bytes[0x14c]; } LegoBuildingModelE;
typedef struct LegoUpgradeModelE { unsigned char bytes[0x10]; } LegoUpgradeModelE;
/* Maths.h Point2F / Vector3F / Vector4F. */
typedef struct LegoPoint2FE { float x, y; } LegoPoint2FE;
typedef struct LegoVector3FE { float x, y, z; } LegoVector3FE;
typedef struct LegoVector4FE { float x, y, z, w; } LegoVector4FE;

struct BlockRec;
struct ViewProjectionAdapter;
struct EntitySlot;

extern LegoVehicleModelE* legoGlobs_vehicleData;        /* 005561d4 */
extern LegoCreatureModelE* legoGlobs_miniFigureData;    /* 005561d8 */
extern LegoCreatureModelE* legoGlobs_rockMonsterData;   /* 005561dc */
extern LegoBuildingModelE* legoGlobs_buildingData;      /* 005561e0 */
extern char** legoGlobs_vehicleName;                    /* 005561e8 */
extern char** legoGlobs_miniFigureName;                 /* 005561ec */
extern char** legoGlobs_rockMonsterName;                /* 005561f0 */
extern char** legoGlobs_buildingName;                   /* 005561f4 */
extern char** legoGlobs_upgradeName;                    /* 005561f8 */

/* gods98 Input's key maps (0076ab80 this frame, 0076aa00 last frame). */

void lego_air_supply_amount_update(struct EntitySlot* liveObj,
                                   float elapsed);
void lego_terrain_cell_flag_cleanup(Lego_Level* level, float elapsed);
void lego_sine_phase_midpoint_blend_dispatch(int on, float elapsed);
int lego_default_transform_apply_if_flagged(struct EntitySlot* liveObj,
                                            void* data);
void lego_default_transform_apply_all(void* view);
void lego_selected_object_outlines_draw(struct ViewProjectionAdapter* view);
void lego_selected_entity_ground_marker_render(
    struct EntitySlot* liveObj, struct ViewProjectionAdapter* view,
    float r, float g, float b);
void lego_engine_shutdown_alt(void);
void lego_engine_shutdown(void);
void lego_exit(void);
int lego_main_sim_step(float elapsedGame, float elapsed, int* keyDownT,
                       int* keyDownR, int* keyDownAnyShift);
void lego_periodic_callback_dispatch(int stop);
int lego_object_sound_restart_callback(struct EntitySlot* liveObj,
                                       void* data);
/* Group G's (assumed signatures). */
void lego_transition_control(int a, unsigned int b);
void lego_cd_audio_watchdog_mode_update(int on);
void lego_audio_playback_toggle(int on);



/* ---- group F ---- */

/* Lego_RockMonsterData? (0x74 bytes): one creature type's data, read by
 * address only. */
typedef struct LegoCreatureDataF {
    unsigned char f_00[0x74];
} LegoCreatureDataF;

/* gods98 Input's key maps (stand-ins in lego_unit.c), and Input.h's
 * Input_IsKeyPressed. */
#define LEGO_KEY_PRESSED(k) \
    (s_global_current_keys[k] && (s_global_previous_keys[k] ^ s_global_current_keys[k]))

/* legoGlobs stand-ins (lego_unit.c). */
extern void* legoGlobs_contBoulder;               /* 00556368 */
extern void* legoGlobs_contBoulderExplode;        /* 0055636c */
extern void* legoGlobs_contBoulderExplodeIce;     /* 00556370 */
extern void* legoGlobs_contCrystal;               /* 00556374 */
extern void* legoGlobs_contDynamite;              /* 00556378 */
extern void* legoGlobs_contOresTable;             /* 0055637c */
extern void* legoGlobs_contProcessedOre;          /* 00556380 */
extern void* legoGlobs_contOohScary;              /* 00556384 */
extern void* legoGlobs_contBarrier;               /* 00556388 */
extern void* legoGlobs_contElectricFence;         /* 0055638c */
extern void* legoGlobs_contSpiderWeb;             /* 00556390 */
extern void* legoGlobs_contRechargeSparkle;       /* 00556394 */
extern void* legoGlobs_contMiniTeleportUp;        /* 00556398 */
extern void* legoGlobs_contElectricFenceStud;     /* 0055639c */
extern void* legoGlobs_contPusher;                /* 005563a0 */
extern void* legoGlobs_contFreezer;               /* 005563a4 */
extern void* legoGlobs_contIceCube;               /* 005563a8 */
extern void* legoGlobs_contSmashPath;             /* 005563ac */
extern void* legoGlobs_contLaserShot;             /* 005563b0 */
extern char* legoGlobs_energyText;  /* 00556ea0 */
extern char* legoGlobs_crystalsText;  /* 00556ea4 */
extern char* legoGlobs_oreText;  /* 00556ea8 */
extern char* legoGlobs_studsText;  /* 00556eac */
extern char* legoGlobs_drivenByText;  /* 00556eb8 */
extern char* legoGlobs_upgradeNames[16];  /* 00556f58 */

struct BlockRec;
void lego_misc_object_resources_load(struct BlockRec* config);

struct EntitySlot;
void lego_terrain_pointer_command_update(void);
void lego_spawn_coordinate_pool_tick(float elapsed);
void lego_entity_command_status_feedback_dispatch(struct EntitySlot* liveObj);
void lego_terrain_command_feedback_dispatch(const int* blockPos, int showConstruction,
                                            int playSound, int showCavern);
int lego_terrain_cell_command_predicate(const int* blockPos);
void lego_level_debug_input_cell_mutator_dispatch(int bx, int by, struct EntitySlot* liveObj,
                                                   float elapsed);



/* ---- group G ---- */

struct BlockRec;

/* legoGlobs.config (005560c0): the first field, so &legoGlobs_config is
 * the address of legoGlobs itself. The port has no Lego_Globs object
 * (several fields are port globals annotated in other files), so Lego_Gods_Go
 * zeroes the original's 0xef8 bytes from this field's address. */
#define LEGO_GLOBS_SIZE 0xef8
extern char* legoGlobs_healthText;              /* 00556e9c ToolTipInfo */
extern char* legoGlobs_toolsText;               /* 00556eb0 */
extern char* legoGlobs_carryObjectText;         /* 00556eb4 */
extern char* legoGlobs_oreRequiredText;         /* 00556ebc */

/* A 2D point (the original's Point2F) for the drag-box line list. */
typedef struct LegoPoint2FG {
    float x;
    float y;
} LegoPoint2FG;

/* Main_State: the three callbacks Lego_Gods_Go installs. */
typedef struct LegoMainStateG {
    int (*initFunc)(void);
    int (*mainLoopFunc)(float elapsed);
    void (*shutdownFunc)(void);
} LegoMainStateG;

void lego_game_config_callback_registry_init(const char* gameName);
void lego_start_level_pilots(void);
void lego_transition_control(int toggle, unsigned int paused);
void lego_global_subsystem_state_reset(void);
void lego_level_upgrade_name_strings_load(struct BlockRec* config);
void lego_scene_marker_update_and_draw(float elapsed, int track);
void lego_selection_rectangle_render(Lego_Level* level);
void lego_camera_pointer_ray_route(unsigned int mouseX, unsigned int mouseY,
                                   float* xOut, float* yOut);
void lego_sound_minimum_distance_update_mode(int viewMode, void* liveObj,
                                             int cameraFrame);
void lego_cd_audio_watchdog_mode_update(int on);
void lego_audio_playback_toggle(int on);

/* Other groups' functions group G calls (signatures assumed). */
int lego_game_config_init(void);                /* A, 0041fb50 */
int lego_engine_tick(float elapsed);            /* B, 004232e0 */
void lego_engine_shutdown_alt(void);            /* E, 00424cf0 */
void lego_engine_shutdown(void);                /* E, 00424d00 */
void lego_periodic_callback_dispatch(int on);   /* E, 00425b40 */

/* ---- GAME-021 part 2 (issue #574) ---- */

/* ---- part 2 declarations, group 2A (0042b510-0042c440): begin ---- */

/* ---- part 2 declarations, group 2A: end ---- */

/* ---- part 2 declarations, group 2B (0042c480-0042d460): begin ---- */

/* ---- part 2 declarations, group 2B: end ---- */

/* ---- part 2 declarations, group 2C (0042d600-0042dff0): begin ---- */

/* ---- part 2 declarations, group 2C: end ---- */

/* ---- part 2 declarations, group 2D (0042e020-0042ef40): begin ---- */

/* ---- part 2 declarations, group 2D: end ---- */

/* ---- part 2 declarations, group 2E (0042efc0-0042f6f0): begin ---- */

/* Win32 RECT, as Lego_PlayMovie's destination area. */
typedef struct LegoRect2E {
    long left, top, right, bottom;
} LegoRect2E;

/* Stand-ins (lego_unit.c). */
extern unsigned int legoGlobs_f_556514;         /* 00556514 */
extern int s_object_f_4eabc4;                   /* 004eabc4 */
extern int s_object_f_4eabc8;                   /* 004eabc8 */

void lego_play_movie(const char* fName, const float* screenPt);
const char* lego_level_free(void);
void lego_terrain_cell_edge_flag_set_and_update(Lego_Level* level, int bx, int by, int on);
unsigned char lego_terrain_neighbor_coordinate_check(int bx, int by, unsigned char* direction,
                                                     int powered);
/* Level_BlockUpdateSurface: the fourth argument is unused. */
void lego_cell_init_pass(Lego_Level* level, unsigned int bx, unsigned int by, int reserved);

/* Assumed from other groups (original signatures). */
void lego_terrain_neighbor_flag_refresh(Lego_Level* level, const int* blockPos,
                                        int mode);               /* 2F 00431450 */
void lego_terrain_activity_build_records_release_all(Lego_Level* level); /* 2F 00431530 */

/* ---- part 2 declarations, group 2E: end ---- */

/* ---- part 2 declarations, group 2F (004302b0-00431570): begin ---- */

/* ---- part 2 declarations, group 2F: end ---- */

/* ---- part 2 declarations, group 2G (00431580-00432490): begin ---- */

/* ---- part 2 declarations, group 2G: end ---- */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGO_UNIT_H */
