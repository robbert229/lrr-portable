// SPDX-License-Identifier: LGPL-3.0-or-later
/* legoGlobs (Lego.c, GAME-021): the original's one Lego_Globs struct, at
 * 005560c0, 0xef8 bytes (005560c0-00556fb8; Lego_Gods_Go zeroes exactly
 * that much). lego_unit.c defines it (issue #759, finding 653).
 *
 * Every field sits at its original offset. The names the units used for
 * the fields while they were separate stand-in globals are macros over the
 * fields, so the units' code is unchanged:
 *
 * - fields no port module owns (lego_unit.c's and legoobject_unit.c's old
 *   stand-ins) are the struct's in every build;
 * - fields a C++ port module keeps as a global of its own (engine.cpp,
 *   entity.cpp, level.cpp, ...) are the struct's in the matching build
 *   only. Natively the port keeps its globals, with their initial values,
 *   and the units read them by the port's names, as before.
 *
 * The macros make any later declaration of the same names invalid, so
 * include this header after every header that declares them (C++ port
 * files: after their frozen headers). */
#ifndef LEGORRR_GAME_LEGO_GLOBS_H
#define LEGORRR_GAME_LEGO_GLOBS_H

#include <stdint.h>

/* The members have only basic and void* types: the struct and legoGlobs
 * then cost every unit that includes this header exactly two symbols
 * (finding 538). The macros give the pointer fields their types. */
#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Globs {
    void* config;                            /* +0x000 005560c0 legoGlobs_config (BlockRec*); Lego.cfg's block root */
    const char* gameName;                    /* +0x004 005560c4 s_engine_config_root; engine.cpp */
    uint32_t field_8;                        /* +0x008 005560c8 */
    uint32_t field_c;                        /* +0x00c 005560cc */
    int quality;                             /* +0x010 005560d0 legoGlobs_quality; render quality */
#ifdef __cplusplus
    struct LevelState* currLevel;            /* +0x014 005560d4 g_active_level_retail (LevelState*); terrain_grid.cpp */
#else
    void* currLevel;                         /* +0x014 005560d4 g_active_level_retail (LevelState*); terrain_grid.cpp */
#endif
    void* rootCont;                          /* +0x018 005560d8 legoGlobs_rootCont */
    void* viewMain;                          /* +0x01c 005560dc legoGlobs_viewMain */
    void* viewTrack;                         /* +0x020 005560e0 legoGlobs_viewTrack */
    void* cameraMain;                        /* +0x024 005560e4 s_zoom_step_context; view.cpp */
    void* cameraTrack;                       /* +0x028 005560e8 legoGlobs_cameraTrack */
    void* cameraFP;                          /* +0x02c 005560ec legoGlobs_cameraFP */
    void* spotlightTop;                      /* +0x030 005560f0 legoGlobs_spotlightTop */
    void* spotlightTrack;                    /* +0x034 005560f4 legoGlobs_spotlightTrack */
    uint32_t field_38;                       /* +0x038 005560f8 legoGlobs_f_5560f8; pointLightFP */
    void* dirLightFP;                        /* +0x03c 005560fc legoGlobs_dirLightFP */
    void* ambientLight;                      /* +0x040 00556100 legoGlobs_ambientLight */
    uint32_t field_44;                       /* +0x044 00556104 legoGlobs_f_556104 */
    void* field_48;                          /* +0x048 00556108 legoGlobs_f_556108; the pointer-following container */
    void* dirLightCallToArms;                /* +0x04c 0055610c legoGlobs_dirLightCallToArms */
    float FPClipBlocks;                      /* +0x050 00556110 legoGlobs_FPClipBlocks */
    float TVClipDist;                        /* +0x054 00556114 legoGlobs_TVClipDist */
    uint32_t field_58[3];                    /* +0x058 00556118 */
    void* fontStandard;                      /* +0x064 00556124 s_lego_text_window_font (TextureSurface*) */
    void* fontToolTip;                       /* +0x068 00556128 s_lego_crystal_count_font (TextureSurface*) */
    void* fontDesktop;                       /* +0x06c 0055612c legoGlobs_fontDesktop */
    void* fontTextWindow;                    /* +0x070 00556130 legoGlobs_fontTextWindow */
    void* fontBriefingHi;                    /* +0x074 00556134 legoGlobs_fontBriefingHi */
    void* fontBriefingLo;                    /* +0x078 00556138 legoGlobs_fontBriefingLo */
    void* fontRSFont;                        /* +0x07c 0055613c DAT_0055613c; frontend.cpp */
    void* textOnlyWindow;                    /* +0x080 00556140 legoGlobs_textOnlyWindow */
    void* textImageWindow;                   /* +0x084 00556144 legoGlobs_textImageWindow */
    float field_88;                          /* +0x088 00556148 legoGlobs_f_556148 */
    float field_8c;                          /* +0x08c 0055614c legoGlobs_f_55614c */
    float field_90;                          /* +0x090 00556150 legoGlobs_f_556150 */
    float field_94;                          /* +0x094 00556154 legoGlobs_f_556154 */
    int field_98;                            /* +0x098 00556158 legoGlobs_f_556158 */
    float gameSpeed;                         /* +0x09c 0055615c s_scroll_scale; engine.cpp */
    void* objectFP;                          /* +0x0a0 00556160 legoGlobs_selectedObject */
    float field_a4[3];                       /* +0x0a4 00556164 legoGlobs_f_556164 */
    float field_b0[2];                       /* +0x0b0 00556170 legoGlobs_f_556170 */
    float field_b8[2];                       /* +0x0b8 00556178 legoGlobs_f_556178 */
    int field_c0;                            /* +0x0c0 00556180 legoGlobs_f_556180 */
    int field_c4;                            /* +0x0c4 00556184 legoGlobs_f_556184 */
    int field_c8;                            /* +0x0c8 00556188 legoGlobs_f_556188 */
    const char* objectNamesSpecial[9];       /* +0x0cc 0055618c s_object_name_special; entity.cpp */
    const char* objectTheNamesSpecial[9];    /* +0x0f0 005561b0 s_object_the_name_special; entity.cpp */
    void* vehicleData;                       /* +0x114 005561d4 legoGlobs_vehicleData (LegoVehicleModelE*) */
    void* miniFigureData;                    /* +0x118 005561d8 legoGlobs_miniFigureData (LegoCreatureModelE*) */
    void* rockMonsterData;                   /* +0x11c 005561dc legoGlobs_rockMonsterData (LegoCreatureModelE*) */
    void* buildingData;                      /* +0x120 005561e0 legoGlobs_buildingData (LegoBuildingModelE*) */
    void* upgradeData;                       /* +0x124 005561e4 s_upgrade_records; cmd.cpp */
    char** vehicleName;                      /* +0x128 005561e8 legoGlobs_vehicleName */
    char** miniFigureName;                   /* +0x12c 005561ec legoGlobs_miniFigureName */
    char** rockMonsterName;                  /* +0x130 005561f0 legoGlobs_rockMonsterName */
    char** buildingName;                     /* +0x134 005561f4 legoGlobs_buildingName */
    char** upgradeName;                      /* +0x138 005561f8 legoGlobs_upgradeName */
    const char* toolName[11];                /* +0x13c 005561fc s_tool_type_names; ai.cpp */
    char* langToolName[11];                  /* +0x168 00556228 */
    const char** objectNamesKind1;           /* +0x194 00556254 s_object_name_kind1; entity.cpp */
    const char** objectNamesKind2;           /* +0x198 00556258 s_object_name_kind2; entity.cpp */
    const char** objectNamesKind3;           /* +0x19c 0055625c s_object_name_kind3; entity.cpp */
    const char** objectNamesKind4;           /* +0x1a0 00556260 s_object_name_kind4; entity.cpp */
    const char** objectNamesKind10;          /* +0x1a4 00556264 s_object_name_kind10; entity.cpp */
    const char** objectTheNamesKind1;        /* +0x1a8 00556268 s_object_the_name_kind1; entity.cpp */
    const char** objectTheNamesKind2;        /* +0x1ac 0055626c s_object_the_name_kind2; entity.cpp */
    const char** objectTheNamesKind3;        /* +0x1b0 00556270 s_object_the_name_kind3; entity.cpp */
    const char** objectTheNamesKind4;        /* +0x1b4 00556274 s_object_the_name_kind4; entity.cpp */
    const char** objectTheNamesKind10;       /* +0x1b8 00556278 s_object_the_name_kind10; entity.cpp */
    uint32_t typeCounts[5];                  /* +0x1bc 0055627c s_catalog_counts; entity.cpp */
    const char* langSurface_name[18];        /* +0x1d0 00556290 legoGlobs_langSurface_name */
    const char* surfaceName[18];             /* +0x218 005562d8 s_surface_type_names; level.cpp */
    uint32_t surfaceValue[18];               /* +0x260 00556320 s_surface_type_values; level.cpp */
    void* contBoulder;                       /* +0x2a8 00556368 legoGlobs_contBoulder */
    void* contBoulderExplode;                /* +0x2ac 0055636c legoGlobs_contBoulderExplode */
    void* contBoulderExplodeIce;             /* +0x2b0 00556370 legoGlobs_contBoulderExplodeIce */
    void* contCrystal;                       /* +0x2b4 00556374 legoGlobs_contCrystal */
    void* contDynamite;                      /* +0x2b8 00556378 legoGlobs_contDynamite */
    void* contOresTable;                     /* +0x2bc 0055637c legoGlobs_contOresTable */
    void* contProcessedOre;                  /* +0x2c0 00556380 legoGlobs_contProcessedOre */
    void* contOohScary;                      /* +0x2c4 00556384 legoGlobs_contOohScary */
    void* contBarrier;                       /* +0x2c8 00556388 legoGlobs_contBarrier */
    void* contElectricFence;                 /* +0x2cc 0055638c legoGlobs_contElectricFence */
    void* contSpiderWeb;                     /* +0x2d0 00556390 legoGlobs_contSpiderWeb */
    void* contRechargeSparkle;               /* +0x2d4 00556394 legoGlobs_contRechargeSparkle */
    void* contMiniTeleportUp;                /* +0x2d8 00556398 legoGlobs_contMiniTeleportUp */
    void* contElectricFenceStud;             /* +0x2dc 0055639c legoGlobs_contElectricFenceStud */
    void* contPusher;                        /* +0x2e0 005563a0 legoGlobs_contPusher */
    void* contFreezer;                       /* +0x2e4 005563a4 legoGlobs_contFreezer */
    void* contIceCube;                       /* +0x2e8 005563a8 legoGlobs_contIceCube */
    void* contSmashPath;                     /* +0x2ec 005563ac legoGlobs_contSmashPath */
    void* contLaserShot;                     /* +0x2f0 005563b0 legoGlobs_contLaserShot */
    void* field_2f4;                         /* +0x2f4 005563b4 s_refresh_record; engine.cpp */
    uint32_t recordObjects[10];              /* +0x2f8 005563b8 s_object_pointer_registry_words; entity.cpp */
    uint32_t recordObjectCount;              /* +0x320 005563e0 s_object_pointer_registry_count; entity.cpp */
    float field_324[4];                      /* +0x324 005563e4 legoGlobs_f_5563e4; the radar view's area */
    float radarZoom;                         /* +0x334 005563f4 s_lego_radar_zoom */
    float field_338[2];                      /* +0x338 005563f8 the radar centre (a Point2F) */
    float MedPolyRange;                      /* +0x340 00556400 legoGlobs_MedPolyRange */
    float HighPolyRange;                     /* +0x344 00556404 legoGlobs_HighPolyRange */
    int HPBlocks;                            /* +0x348 00556408 legoGlobs_HPBlocks */
    float FogColourRGB[3];                   /* +0x34c 0055640c legoGlobs_fogColourRGB */
    float HighFogColourRGB[3];               /* +0x358 00556418 legoGlobs_highFogColourRGB */
    float field_364;                         /* +0x364 00556424 legoGlobs_f_556424 */
    float PowerCrystalRGB[3];                /* +0x368 00556428 legoGlobs_PowerCrystalRGB */
    float UnpoweredCrystalRGB[3];            /* +0x374 00556434 legoGlobs_UnpoweredCrystalRGB */
    int field_380;                           /* +0x380 00556440 legoGlobs_f_556440 */
    int field_384;                           /* +0x384 00556444 legoGlobs_f_556444 */
    int field_388;                           /* +0x388 00556448 legoGlobs_f_556448 */
    uint32_t field_38c;                      /* +0x38c 0055644c legoGlobs_f_55644c */
    uint32_t field_390;                      /* +0x390 00556450 legoGlobs_f_556450 */
    void* field_394;                         /* +0x394 00556454 legoGlobs_f_556454 (Lego_Object*) */
    float MinEnergyForEat;                   /* +0x398 00556458 legoGlobs_MinEnergyForEat */
    void* TutorialIcon;                      /* +0x39c 0055645c legoGlobs_TutorialIcon (Image*) */
    uint32_t field_3a0;                      /* +0x3a0 00556460 */
    float DynamiteDamageRadius;              /* +0x3a4 00556464 legoGlobs_DynamiteDamageRadius */
    float DynamiteMaxDamage;                 /* +0x3a8 00556468 legoGlobs_DynamiteMaxDamage */
    float DynamiteWakeRadius;                /* +0x3ac 0055646c legoGlobs_DynamiteWakeRadius */
    float BirdScarerRadius;                  /* +0x3b0 00556470 legoGlobs_BirdScarerRadius */
    int objectRequests[40];                  /* +0x3b4 00556474 the queued object requests (LegoObjI_Requests) */
    unsigned int objectRequestCount;         /* +0x454 00556514 legoGlobs_f_556514 */
    float MiniFigureRunAway;                 /* +0x458 00556518 legoGlobs_MiniFigureRunAway */
    uint32_t field_45c;                      /* +0x45c 0055651c s_triple_first; engine.cpp */
    uint32_t field_460;                      /* +0x460 00556520 s_triple_second; engine.cpp */
    uint32_t field_464;                      /* +0x464 00556524 s_triple_third; engine.cpp */
    int32_t visitedBlocks[100][2];           /* +0x468 00556528 s_cell_visited_track; level.cpp */
    int visitedBlockCount;                   /* +0x788 00556848 s_cell_visited_track_count; level.cpp */
    int32_t updateBlocks[100][2];            /* +0x78c 0055684c s_cell_refresh_track; level.cpp */
    int32_t previousBlocks[100][2];          /* +0xaac 00556b6c s_cell_previous_track; level.cpp */
    int updateBlockCount;                    /* +0xdcc 00556e8c s_cell_refresh_track_count; level.cpp */
    int previousBlockCount;                  /* +0xdd0 00556e90 s_cell_previous_track_count; level.cpp */
    int MaxReturnedCrystals;                 /* +0xdd4 00556e94 legoGlobs_MaxReturnedCrystals */
    int MouseScrollBorder;                   /* +0xdd8 00556e98 legoGlobs_MouseScrollBorder */
    char* HealthText;                        /* +0xddc 00556e9c legoGlobs_healthText */
    char* EnergyText;                        /* +0xde0 00556ea0 legoGlobs_energyText */
    char* CrystalsText;                      /* +0xde4 00556ea4 legoGlobs_crystalsText */
    char* OreText;                           /* +0xde8 00556ea8 legoGlobs_oreText */
    char* StudsText;                         /* +0xdec 00556eac legoGlobs_studsText */
    char* ToolsText;                         /* +0xdf0 00556eb0 legoGlobs_toolsText */
    char* CarryObjectText;                   /* +0xdf4 00556eb4 legoGlobs_carryObjectText */
    char* DrivenByText;                      /* +0xdf8 00556eb8 legoGlobs_drivenByText */
    char* OreRequiredText;                   /* +0xdfc 00556ebc legoGlobs_oreRequiredText */
    int field_e00;                           /* +0xe00 00556ec0 legoGlobs_f_556ec0 */
    float MinDistFor3DSoundsOnTopView;       /* +0xe04 00556ec4 legoGlobs_MinDistFor3DSoundsOnTopView */
    int32_t viewMode;                        /* +0xe08 00556ec8 s_context_mode; engine.cpp */
    uint32_t flags1;                         /* +0xe0c 00556ecc legoGlobs_flags1 */
    uint32_t flags2;                         /* +0xe10 00556ed0 s_panel_highlight_flags; engine.cpp */
    uint32_t flags3;                         /* +0xe14 00556ed4 s_global_low_word; engine.cpp */
    float field_e18;                         /* +0xe18 00556ed8 g_camera_countdown; camera.cpp */
    float NextButtonPos[2];                  /* +0xe1c 00556edc legoGlobs_NextButtonPos */
    float BackButtonPos[2];                  /* +0xe24 00556ee4 legoGlobs_BackButtonPos */
    void* NextButtonImage;                   /* +0xe2c 00556eec legoGlobs_NextButtonImage (Image*) */
    void* BackButtonImage;                   /* +0xe30 00556ef0 legoGlobs_BackButtonImage (Image*) */
    void* BackArrowImage;                    /* +0xe34 00556ef4 legoGlobs_BackArrowImage (Image*) */
    float field_e38;                         /* +0xe38 00556ef8 legoGlobs_f_556ef8; FogRate * 25 */
    float field_e3c;                         /* +0xe3c 00556efc s_panel_highlight_timer; engine.cpp */
    float field_e40;                         /* +0xe40 00556f00 s_global_float_00556f00; port_state.cpp */
    float DrainTime;                         /* +0xe44 00556f04 legoGlobs_DrainTime */
    int ReinforceHits;                       /* +0xe48 00556f08 legoGlobs_ReinforceHits */
    int CDStartTrack;                        /* +0xe4c 00556f0c legoGlobs_CDStartTrack */
    uint32_t CDTracks;                       /* +0xe50 00556f10 legoGlobs_CDTracks */
    int FallinMultiplier;                    /* +0xe54 00556f14 legoGlobs_f_556f14 */
    int field_e58;                           /* +0xe58 00556f18 legoGlobs_f_556f18 */
    float scoreTextX;                        /* +0xe5c 00556f1c s_score_text_origin_x; engine.cpp */
    float scoreTextY;                        /* +0xe60 00556f20 s_score_text_origin_y; engine.cpp */
    float taskTextX;                         /* +0xe64 00556f24 s_task_text_origin_x; engine.cpp */
    float taskTextY;                         /* +0xe68 00556f28 s_task_text_origin_y; engine.cpp */
    float DragBoxRGB[3];                     /* +0xe6c 00556f2c legoGlobs_dragBoxRGB */
    void* DialogImage;                       /* +0xe78 00556f38 legoGlobs_DialogImage (Image*) */
    void* field_e7c;                         /* +0xe7c 00556f3c s_position_handle; engine.cpp */
    void* DialogTextWndTitle;                /* +0xe80 00556f40 legoGlobs_DialogTextWndTitle */
    void* DialogTextWndMessage;              /* +0xe84 00556f44 legoGlobs_DialogTextWndMessage */
    void* DialogTextWndOK;                   /* +0xe88 00556f48 legoGlobs_DialogTextWndOK */
    void* DialogTextWndCancel;               /* +0xe8c 00556f4c legoGlobs_DialogTextWndCancel */
    void* CreditsTextFile;                   /* +0xe90 00556f50 DAT_00556f50; frontend.cpp */
    void* CreditsBackAVI;                    /* +0xe94 00556f54 DAT_00556f54; frontend.cpp */
    char* UpgradeNames[16];                  /* +0xe98 00556f58 legoGlobs_upgradeNames */
    int BuildingUpgradeCostOre;              /* +0xed8 00556f98 legoGlobs_BuildingUpgradeCostOre */
    int BuildingUpgradeCostStuds;            /* +0xedc 00556f9c legoGlobs_BuildingUpgradeCostStuds */
    char* renameInput;                       /* +0xee0 00556fa0 legoGlobs_renameInput */
    float renamePosition[2];                 /* +0xee4 00556fa4 legoGlobs_renamePosition */
    char* RenameReplace;                     /* +0xeec 00556fac legoGlobs_RenameReplace */
    char* EndGameAVI1;                       /* +0xef0 00556fb0 legoGlobs_endGameAVI1 */
    char* EndGameAVI2;                       /* +0xef4 00556fb4 legoGlobs_endGameAVI2 */
};

extern struct Lego_Globs legoGlobs;

/* The unit-owned fields, under the units' names (every build). */
#define legoGlobs_config (*(struct BlockRec**)&legoGlobs.config)
#define legoGlobs_quality legoGlobs.quality
#define legoGlobs_rootCont legoGlobs.rootCont
#define legoGlobs_viewMain legoGlobs.viewMain
#define legoGlobs_viewTrack legoGlobs.viewTrack
#define legoGlobs_cameraTrack legoGlobs.cameraTrack
#define legoGlobs_cameraFP legoGlobs.cameraFP
#define legoGlobs_spotlightTop legoGlobs.spotlightTop
#define legoGlobs_spotlightTrack legoGlobs.spotlightTrack
#define legoGlobs_f_5560f8 legoGlobs.field_38
#define legoGlobs_dirLightFP legoGlobs.dirLightFP
#define legoGlobs_ambientLight legoGlobs.ambientLight
#define legoGlobs_f_556104 legoGlobs.field_44
#define legoGlobs_f_556108 legoGlobs.field_48
#define legoGlobs_dirLightCallToArms legoGlobs.dirLightCallToArms
#define legoGlobs_FPClipBlocks legoGlobs.FPClipBlocks
#define legoGlobs_TVClipDist legoGlobs.TVClipDist
#define s_lego_text_window_font (*(struct TextureSurface**)&legoGlobs.fontStandard)
#define s_lego_crystal_count_font (*(struct TextureSurface**)&legoGlobs.fontToolTip)
#define legoGlobs_fontDesktop legoGlobs.fontDesktop
#define legoGlobs_fontTextWindow legoGlobs.fontTextWindow
#define legoGlobs_fontBriefingHi legoGlobs.fontBriefingHi
#define legoGlobs_fontBriefingLo legoGlobs.fontBriefingLo
#define legoGlobs_textOnlyWindow legoGlobs.textOnlyWindow
#define legoGlobs_textImageWindow legoGlobs.textImageWindow
#define legoGlobs_f_556148 legoGlobs.field_88
#define legoGlobs_f_55614c legoGlobs.field_8c
#define legoGlobs_f_556150 legoGlobs.field_90
#define legoGlobs_f_556154 legoGlobs.field_94
#define legoGlobs_f_556158 legoGlobs.field_98
#define legoGlobs_selectedObject legoGlobs.objectFP
#define legoGlobs_f_556164 legoGlobs.field_a4
#define legoGlobs_f_556170 legoGlobs.field_b0
#define legoGlobs_f_556178 legoGlobs.field_b8
#define legoGlobs_f_556180 legoGlobs.field_c0
#define legoGlobs_f_556184 legoGlobs.field_c4
#define legoGlobs_f_556188 legoGlobs.field_c8
#define legoGlobs_vehicleData (*(struct LegoVehicleModelE**)&legoGlobs.vehicleData)
#define legoGlobs_miniFigureData (*(struct LegoCreatureModelE**)&legoGlobs.miniFigureData)
#define legoGlobs_rockMonsterData (*(struct LegoCreatureModelE**)&legoGlobs.rockMonsterData)
#define legoGlobs_buildingData (*(struct LegoBuildingModelE**)&legoGlobs.buildingData)
#define legoGlobs_vehicleName legoGlobs.vehicleName
#define legoGlobs_miniFigureName legoGlobs.miniFigureName
#define legoGlobs_rockMonsterName legoGlobs.rockMonsterName
#define legoGlobs_buildingName legoGlobs.buildingName
#define legoGlobs_upgradeName legoGlobs.upgradeName
#define legoGlobs_langSurface_name legoGlobs.langSurface_name
#define legoGlobs_contBoulder legoGlobs.contBoulder
#define legoGlobs_contBoulderExplode legoGlobs.contBoulderExplode
#define legoGlobs_contBoulderExplodeIce legoGlobs.contBoulderExplodeIce
#define legoGlobs_contCrystal legoGlobs.contCrystal
#define legoGlobs_contDynamite legoGlobs.contDynamite
#define legoGlobs_contOresTable legoGlobs.contOresTable
#define legoGlobs_contProcessedOre legoGlobs.contProcessedOre
#define legoGlobs_contOohScary legoGlobs.contOohScary
#define legoGlobs_contBarrier legoGlobs.contBarrier
#define legoGlobs_contElectricFence legoGlobs.contElectricFence
#define legoGlobs_contSpiderWeb legoGlobs.contSpiderWeb
#define legoGlobs_contRechargeSparkle legoGlobs.contRechargeSparkle
#define legoGlobs_contMiniTeleportUp legoGlobs.contMiniTeleportUp
#define legoGlobs_contElectricFenceStud legoGlobs.contElectricFenceStud
#define legoGlobs_contPusher legoGlobs.contPusher
#define legoGlobs_contFreezer legoGlobs.contFreezer
#define legoGlobs_contIceCube legoGlobs.contIceCube
#define legoGlobs_contSmashPath legoGlobs.contSmashPath
#define legoGlobs_contLaserShot legoGlobs.contLaserShot
#define legoGlobs_f_5563e4 legoGlobs.field_324
#define s_lego_radar_zoom legoGlobs.radarZoom
#define legoGlobs_MedPolyRange legoGlobs.MedPolyRange
#define legoGlobs_HighPolyRange legoGlobs.HighPolyRange
#define legoGlobs_HPBlocks legoGlobs.HPBlocks
#define legoGlobs_fogColourRGB legoGlobs.FogColourRGB
#define legoGlobs_highFogColourRGB legoGlobs.HighFogColourRGB
#define legoGlobs_f_556424 legoGlobs.field_364
#define legoGlobs_PowerCrystalRGB legoGlobs.PowerCrystalRGB
#define legoGlobs_UnpoweredCrystalRGB legoGlobs.UnpoweredCrystalRGB
#define legoGlobs_f_556440 legoGlobs.field_380
#define legoGlobs_f_556444 legoGlobs.field_384
#define legoGlobs_f_556448 legoGlobs.field_388
#define legoGlobs_f_55644c legoGlobs.field_38c
#define legoGlobs_f_556450 legoGlobs.field_390
#define legoGlobs_f_556454 (*(struct Lego_Object**)&legoGlobs.field_394)
#define legoGlobs_MinEnergyForEat legoGlobs.MinEnergyForEat
#define legoGlobs_TutorialIcon (*(struct Image**)&legoGlobs.TutorialIcon)
#define legoGlobs_DynamiteDamageRadius legoGlobs.DynamiteDamageRadius
#define legoGlobs_DynamiteMaxDamage legoGlobs.DynamiteMaxDamage
#define legoGlobs_DynamiteWakeRadius legoGlobs.DynamiteWakeRadius
#define legoGlobs_BirdScarerRadius legoGlobs.BirdScarerRadius
#define legoGlobs_f_556514 legoGlobs.objectRequestCount
#define legoGlobs_MiniFigureRunAway legoGlobs.MiniFigureRunAway
#define legoGlobs_MaxReturnedCrystals legoGlobs.MaxReturnedCrystals
#define legoGlobs_MouseScrollBorder legoGlobs.MouseScrollBorder
#define legoGlobs_healthText legoGlobs.HealthText
#define legoGlobs_energyText legoGlobs.EnergyText
#define legoGlobs_crystalsText legoGlobs.CrystalsText
#define legoGlobs_oreText legoGlobs.OreText
#define legoGlobs_studsText legoGlobs.StudsText
#define legoGlobs_toolsText legoGlobs.ToolsText
#define legoGlobs_carryObjectText legoGlobs.CarryObjectText
#define legoGlobs_drivenByText legoGlobs.DrivenByText
#define legoGlobs_oreRequiredText legoGlobs.OreRequiredText
#define legoGlobs_f_556ec0 legoGlobs.field_e00
#define legoGlobs_MinDistFor3DSoundsOnTopView legoGlobs.MinDistFor3DSoundsOnTopView
#define legoGlobs_flags1 legoGlobs.flags1
#define legoGlobs_NextButtonPos legoGlobs.NextButtonPos
#define legoGlobs_BackButtonPos legoGlobs.BackButtonPos
#define legoGlobs_NextButtonImage (*(struct Image**)&legoGlobs.NextButtonImage)
#define legoGlobs_BackButtonImage (*(struct Image**)&legoGlobs.BackButtonImage)
#define legoGlobs_BackArrowImage (*(struct Image**)&legoGlobs.BackArrowImage)
#define legoGlobs_f_556ef8 legoGlobs.field_e38
#define legoGlobs_DrainTime legoGlobs.DrainTime
#define legoGlobs_ReinforceHits legoGlobs.ReinforceHits
#define legoGlobs_CDStartTrack legoGlobs.CDStartTrack
#define legoGlobs_CDTracks legoGlobs.CDTracks
#define legoGlobs_f_556f14 legoGlobs.FallinMultiplier
#define legoGlobs_f_556f18 legoGlobs.field_e58
#define legoGlobs_dragBoxRGB legoGlobs.DragBoxRGB
#define legoGlobs_DialogImage (*(struct Image**)&legoGlobs.DialogImage)
#define legoGlobs_DialogTextWndTitle legoGlobs.DialogTextWndTitle
#define legoGlobs_DialogTextWndMessage legoGlobs.DialogTextWndMessage
#define legoGlobs_DialogTextWndOK legoGlobs.DialogTextWndOK
#define legoGlobs_DialogTextWndCancel legoGlobs.DialogTextWndCancel
#define legoGlobs_upgradeNames legoGlobs.UpgradeNames
#define legoGlobs_BuildingUpgradeCostOre legoGlobs.BuildingUpgradeCostOre
#define legoGlobs_BuildingUpgradeCostStuds legoGlobs.BuildingUpgradeCostStuds
#define legoGlobs_renameInput legoGlobs.renameInput
#define legoGlobs_renamePosition legoGlobs.renamePosition
#define legoGlobs_RenameReplace legoGlobs.RenameReplace
#define legoGlobs_endGameAVI1 legoGlobs.EndGameAVI1
#define legoGlobs_endGameAVI2 legoGlobs.EndGameAVI2
#define legoGlobs_f_5563f8 (*(struct LegoPoint2FC*)legoGlobs.field_338)
#define legoGlobs_objectRequests (*(struct LegoObjI_Requests*)legoGlobs.objectRequests)

/* The port-owned fields, under the port's names (matching build only).
 * currLevel is void* to the C units (no struct tag to declare) and the
 * port's struct LevelState* to the C++ port files; the field stays a plain
 * member access, as the units' and the port's inline asm need. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LEGO_GLOBS_CURRLEVEL legoGlobs.currLevel
#define s_engine_config_root legoGlobs.gameName
#define g_active_level_retail LEGO_GLOBS_CURRLEVEL
#define s_zoom_step_context legoGlobs.cameraMain
#define DAT_0055613c legoGlobs.fontRSFont
#define s_scroll_scale legoGlobs.gameSpeed
#define s_object_name_special legoGlobs.objectNamesSpecial
#define s_object_the_name_special legoGlobs.objectTheNamesSpecial
#define s_upgrade_records legoGlobs.upgradeData
#define s_tool_type_names legoGlobs.toolName
#define s_object_name_kind1 legoGlobs.objectNamesKind1
#define s_object_name_kind2 legoGlobs.objectNamesKind2
#define s_object_name_kind3 legoGlobs.objectNamesKind3
#define s_object_name_kind4 legoGlobs.objectNamesKind4
#define s_object_name_kind10 legoGlobs.objectNamesKind10
#define s_object_the_name_kind1 legoGlobs.objectTheNamesKind1
#define s_object_the_name_kind2 legoGlobs.objectTheNamesKind2
#define s_object_the_name_kind3 legoGlobs.objectTheNamesKind3
#define s_object_the_name_kind4 legoGlobs.objectTheNamesKind4
#define s_object_the_name_kind10 legoGlobs.objectTheNamesKind10
#define s_catalog_counts legoGlobs.typeCounts
#define s_surface_type_names legoGlobs.surfaceName
#define s_surface_type_values legoGlobs.surfaceValue
#define s_refresh_record legoGlobs.field_2f4
#define s_object_pointer_registry_words legoGlobs.recordObjects
#define s_object_pointer_registry_count legoGlobs.recordObjectCount
#define s_triple_first legoGlobs.field_45c
#define s_triple_second legoGlobs.field_460
#define s_triple_third legoGlobs.field_464
#define s_cell_visited_track legoGlobs.visitedBlocks
#define s_cell_visited_track_count legoGlobs.visitedBlockCount
#define s_cell_refresh_track legoGlobs.updateBlocks
#define s_cell_previous_track legoGlobs.previousBlocks
#define s_cell_refresh_track_count legoGlobs.updateBlockCount
#define s_cell_previous_track_count legoGlobs.previousBlockCount
#define s_context_mode legoGlobs.viewMode
#define s_panel_highlight_flags legoGlobs.flags2
#define s_global_low_word legoGlobs.flags3
#define g_camera_countdown legoGlobs.field_e18
#define s_panel_highlight_timer legoGlobs.field_e3c
#define s_global_float_00556f00 legoGlobs.field_e40
#define s_score_text_origin_x legoGlobs.scoreTextX
#define s_score_text_origin_y legoGlobs.scoreTextY
#define s_task_text_origin_x legoGlobs.taskTextX
#define s_task_text_origin_y legoGlobs.taskTextY
#define s_position_handle legoGlobs.field_e7c
#define DAT_00556f50 legoGlobs.CreditsTextFile
#define DAT_00556f54 legoGlobs.CreditsBackAVI
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGO_GLOBS_H */
