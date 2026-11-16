// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-017 FrontEnd.c's save slot records and frontGlobs, shared with the
 * units that read them (Reward.c, which writes a level's reward record
 * into the current slot and reads the main menu, and Objective.c). The +0x... comments are the
 * Win32 offsets; the pointer fields hold real pointers (ADR 0007 decision
 * 4). */
#ifndef LEGORRR_GAME_FRONTEND_RECORD_H
#define LEGORRR_GAME_FRONTEND_RECORD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One level's saved reward record, 0x3190 bytes (00417a90): a flag word
 * and the save image of Reward.c's level record (reward_level_serialize,
 * 0x318c bytes). The save file holds missionsCount of them. */
typedef struct Front_RewardLevel {
    unsigned char field_0[0x2fb4];  /* +0x0000 */
    float score;                    /* +0x2fb4 itemsArray[9].percent */
    unsigned char field_2fb8[0x1d4]; /* +0x2fb8 */
} Front_RewardLevel;

typedef struct Front_SaveReward {
    unsigned int flags;             /* +0x0000 1 completed, 2 a tutorial */
    Front_RewardLevel reward;       /* +0x0004 Reward.c's record */
} Front_SaveReward;

/* The save file reads and writes the records as they are: keep their
 * layout fixed. */
#ifdef __cplusplus
static_assert(sizeof(Front_RewardLevel) == 0x318c, "Front_RewardLevel is a save image");
static_assert(offsetof(Front_RewardLevel, score) == 0x2fb4, "Front_RewardLevel.score");
static_assert(sizeof(Front_SaveReward) == 0x3190, "Front_SaveReward is a save record");
#endif

/* The six values a save keeps for Reward.c (00417f20). */
typedef struct Front_SaveStruct18 {
    int values[6];
} Front_SaveStruct18;

/* A save slot, 0xb8 bytes in frontGlobs on Win32 (00417e50). The file
 * holds the first 0xb0 (read) or 0xb4 (written) bytes of its Win32 image
 * (front_save_data_parse, front_save_data_serialize in frontend_unit.c). */
typedef struct Front_SaveData {
    int field_0;                    /* +0x00 */
    int musicVolume;                /* +0x04 */
    int sfxVolume;                  /* +0x08 */
    int gameSpeed;                  /* +0x0c */
    int brightness;                 /* +0x10 */
    int field_14;                   /* +0x14 */
    unsigned int missionsCount;     /* +0x18 */
    Front_SaveStruct18 saveStruct18; /* +0x1c */
    Front_SaveReward* missionsTable; /* +0x34 */
    unsigned char field_38[0x80];   /* +0x38 */
} Front_SaveData;

struct Image;
/* 0x3c bytes (00410d50, 00410e20). */
typedef struct Front_MenuItem {
    char* banner;               /* +0x00 */
    int length;                 /* +0x04 */
    struct TextureSurface* fontHi;         /* +0x08 */
    struct TextureSurface* fontLo;         /* +0x0c */
    void* itemData;             /* +0x10 */
    int itemType;               /* +0x14 Front_MenuItemType */
    int x1;                     /* +0x18 */
    int y1;                     /* +0x1c */
    int centerOffLo;            /* +0x20 */
    int centerOffHi;            /* +0x24 */
    int isImageItem;            /* +0x28 */
    struct Image* imageLo;      /* +0x2c */
    struct Image* imageHi;      /* +0x30 */
    int toolTipType;            /* +0x34 */
    int notInTuto;              /* +0x38 */
} Front_MenuItem;

/* 0x20 bytes (00413f90): an animation played over a menu. */
typedef struct Front_MenuOverlay {
    char* filename;                     /* +0x00 */
    int overlayType;                    /* +0x04 1 avi, 2 bmp, 3 lws */
    int field_8;                        /* +0x08 */
    int x;                              /* +0x0c */
    int y;                              /* +0x10 */
    int sfxType;                        /* +0x14 */
    struct Front_MenuOverlay* next;     /* +0x18 */
    int field_1c;                       /* +0x1c */
} Front_MenuOverlay;

/* 0xa0 bytes (00411100). */
typedef struct Front_Menu {
    char* title;                    /* +0x00 */
    char* fullName;                 /* +0x04 */
    int titleLength;                /* +0x08 */
    struct TextureSurface* menuFont;           /* +0x0c */
    struct Image* menuImage;        /* +0x10 */
    struct Image* menuImageDark;    /* +0x14 */
    Front_MenuItem** items;         /* +0x18 */
    int itemCount;                  /* +0x1c */
    int itemCapacity;               /* +0x20 */
    int itemFocus;                  /* +0x24 */
    int closed;                     /* +0x28 */
    int xPos;                       /* +0x2c */
    int yPos;                       /* +0x30 */
    unsigned int flags;             /* +0x34 1 image position set, 2 can scroll */
    float imagePos[2];              /* +0x38 */
    Front_MenuOverlay* overlays;    /* +0x40 */
    int autoCenter;                 /* +0x44 */
    int displayTitle;               /* +0x48 */
    int anchored;                   /* +0x4c */
    int anchoredX;                  /* +0x50 */
    int anchoredY;                  /* +0x54 */
    int centerOffset;               /* +0x58 */
    char name[64];                  /* +0x5c */
    int playRandom;                 /* +0x9c */
} Front_Menu;

/* 8 bytes (00414070). */
typedef struct Front_MenuSet {
    Front_Menu** menus;             /* +0x00 */
    int menuCount;                  /* +0x04 */
} Front_MenuSet;

struct Front_SaveTextWindow;
struct Front_LevelTextWindow;
struct TextureSurface;
struct Flic;

/* 0x14 bytes (00417090): one level of a level set's link graph. */
typedef struct Front_LevelLink {
    int setIndex;                           /* +0x00 index in the set */
    struct Front_LevelLink** linkLevels;    /* +0x04 */
    unsigned int linkCount;                 /* +0x08 */
    int field_c;                            /* +0x0c */
    int visited;                            /* +0x10 */
} Front_LevelLink;

/* 0x14 bytes (004154b0): the missions or the tutorials. */
typedef struct Front_LevelSet {
    int count;                      /* +0x00 */
    char** idNames;                 /* +0x04 */
    char** langNames;               /* +0x08 */
    Front_LevelLink** levels;       /* +0x0c */
    int* visitedList;               /* +0x10 */
} Front_LevelSet;

/* frontGlobs (00556fc0, 0x890 bytes on Win32), as the original's one struct: a
 * store into one field then orders the stores around it as the original
 * does (finding 635). Fields the port does not model keep their offsets
 * as field_<offset>; the unit's code names the fields through the
 * frontGlobs_* macros below. */
typedef struct Front_Globs {
    struct Front_MenuSet* pausedMenuSet;                 /* +0x000 */
    void* mainMenuSet;                            /* +0x004 struct Front_MenuSet* */
    struct Front_MenuSet* optionsMenuSet;                /* +0x008 */
    struct Front_MenuSet* saveMenuSet;                   /* +0x00c */
    Front_LevelSet tutorialLevels;                /* +0x010 */
    Front_LevelSet missionLevels;                 /* +0x024 */
    Front_LevelLink* startMissionLink;            /* +0x038 */
    Front_LevelLink* startTutorialLink;           /* +0x03c */
    int field_40[4];                              /* +0x040 */
    /* The main menu's item values, by menu (Front_RunScreenMenuType
     * indexes them): [0] the credits trigger, [3] quit, [4] the mission
     * select, [5] the tutorial select. */
    int mainMenuValues[6];                        /* +0x050 */
    int field_68;                                 /* +0x068 */
    int triggerContinueMission;                   /* +0x06c */
    int sliderGameSpeed;                          /* +0x070 */
    int sliderSFXVolume;                          /* +0x074 */
    int sliderMusicVolume;                        /* +0x078 */
    int sliderBrightness;                         /* +0x07c */
    int cycleHelpWindow;                          /* +0x080 */
    int triggerReplayObjective;                   /* +0x084 */
    int triggerQuitMission;                       /* +0x088 */
    int triggerRestartMission;                    /* +0x08c */
    int cycleWallDetail;                          /* +0x090 */
    int cycleMusicOn;                             /* +0x094 */
    int cycleSFXOn;                               /* +0x098 */
    int cycleAutoGameSpeed;                       /* +0x09c */
    int saveMenuSelect;                           /* +0x0a0 */
    int overlayY;                                 /* +0x0a4 */
    int overlayX;                                 /* +0x0a8 */
    struct Flic* overlayImage;                    /* +0x0ac */
    unsigned int overlayStartTime;                /* +0x0b0 */
    unsigned int overlayCurrTime;                 /* +0x0b4 */
    struct TextureSurface* versionFont;                      /* +0x0b8 */
    char* versionString;                          /* +0x0bc */
    /* Five save slots and the current game's (slot 5). */
    Front_SaveData saveData[6];                   /* +0x0c0 */
    int saveNumber;                               /* +0x510 */
    char* defaultLevelBMPS;                       /* +0x514 */
    int field_518;                                /* +0x518 */
    int scrollOffsetX;                            /* +0x51c */
    int scrollOffsetY;                            /* +0x520 */
    int field_524[2];                             /* +0x524 */
    void* rockWipeAnim;                           /* +0x52c */
    unsigned int rockWipeFlags;                   /* +0x530 */
    float rockWipeTimer;                          /* +0x534 */
    float rockWipeLastUpdateTime;                 /* +0x538 */
    void* rockWipeLight;                          /* +0x53c */
    int isLoadSaveMenu;                           /* +0x540 */
    int saveMenuResult;                           /* +0x544 */
    int saveMenuOpen;                             /* +0x548 */
    /* Unsigned: 004124f0 converts them with a zero high word (fild qword). */
    unsigned int saveImageBigWidth;               /* +0x54c */
    unsigned int saveImageBigHeight;              /* +0x550 */
    char saveSlotFormat[64];                      /* +0x554 */
    char langSaveGame[64];                        /* +0x594 */
    struct Front_SaveTextWindow* saveTextWnd;            /* +0x5d4 */
    struct Front_LevelTextWindow* saveLevelWnd;          /* +0x5d8 */
    char overwriteTitle[128];                     /* +0x5dc */
    char overwriteText[256];                      /* +0x65c */
    char overwriteOK[128];                        /* +0x75c */
    char overwriteCancel[128];                    /* +0x7dc */
    int levelSelectUpdate;                        /* +0x85c */
    int field_860;                                /* +0x860 */
    int levelSelectHoverNumber;                   /* +0x864 */
    int maxLevelScreens;                          /* +0x868 */
    int field_86c[2];                             /* +0x86c */
    int currLevelNumber;                          /* +0x874 */
    int prevLevelNumber;                          /* +0x878 */
    int playLevelStream;                          /* +0x87c */
    float levelStreamTimer;                       /* +0x880 */
    int field_884[3];                             /* +0x884 */
} Front_Globs;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FRONTEND_RECORD_H */
