// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-037 (original translation unit GAME-037, finding 505): the level
 * reward screen (the original's Reward.c).
 *
 * Each level's Lego.cfg block has a Reward group: whether the screen is
 * enabled, a score modifier, and per-item importance and quota for ten
 * items (Crystals, Ore, Diggable, Constructions, Caverns, Figures,
 * RockMonsters, Oxygen, Timer, Score). The Reward group of the game block
 * names the screen's wallpaper, fonts, buttons and texts, and each item's
 * text, image, box image, flic, font and sound. At the end of a level the
 * unit fills a RewardLevel with the level's results, scores each item
 * against its quota and plays the screen: items appear one by one, then
 * the score and the save and advance buttons, over a scrolling summary.
 *
 * C-compatible header (ADR 0005). reward.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file: every include and declaration a header adds shifts MSVC's
 * symbol numbering (finding 506). src/game/test_c_boundaries.cpp checks it
 * against the C++ declarations of the shared functions.
 *
 * The offsets are the original's Win32 layout. The native port widens the
 * pointers; nothing outside the unit addresses these records by offset.
 *
 * The port's own reward screen (GameContext's task records, the
 * RewardScreenState loop and its ops table) is a different contract. It
 * keeps the port's functions, unannotated, in the port tail
 * (reward_port.cpp), and the native callers use those (finding 560). Only
 * the functions below marked "shared" keep a port name: their callers
 * already use the original's contract. */
#ifndef LEGORRR_GAME_REWARD_H
#define LEGORRR_GAME_REWARD_H

#include "gods98/config.h"
#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Flic;
struct TextureSurface;

#define REWARD_ITEM_COUNT 10

/* The items, in record order. */
#define REWARD_ITEM_CRYSTALS 0
#define REWARD_ITEM_ORE 1
#define REWARD_ITEM_DIGGABLE 2
#define REWARD_ITEM_CONSTRUCTIONS 3
#define REWARD_ITEM_CAVERNS 4
#define REWARD_ITEM_FIGURES 5
#define REWARD_ITEM_ROCKMONSTERS 6
#define REWARD_ITEM_OXYGEN 7
#define REWARD_ITEM_TIMER 8
#define REWARD_ITEM_SCORE 9

/* Item flags (+0x00). */
#define REWARD_ITEM_FLAG_TEXT 0x01       /* Text loaded */
#define REWARD_ITEM_FLAG_IMAGE 0x02      /* Images loaded */
#define REWARD_ITEM_FLAG_FLIC 0x04       /* Flics named */
#define REWARD_ITEM_FLAG_TEXTBOX 0x10    /* the text has a box */
#define REWARD_ITEM_FLAG_TEXTBOXED 0x20  /* the box is the darker one */
#define REWARD_ITEM_FLAG_BOXIMAGE 0x40   /* BoxImages loaded */

/* Lego_Level fields the unit reads (Lego.c, 0x284 bytes; the port's
 * LevelState words). */
typedef struct RewardLegoBlock {
    unsigned char unk_00[2];
    unsigned char predug;       /* +0x02 */
    unsigned char unk_03[5];
    unsigned int flags1;        /* +0x08 */
    unsigned char unk_0c[0x3c];
} RewardLegoBlock;              /* 0x48 */

typedef struct RewardLegoMap {
    unsigned int blockWidth;    /* +0x00 */
    unsigned int blockHeight;   /* +0x04 */
} RewardLegoMap;

typedef struct RewardLegoLevel {
    char* name;                 /* +0x000 */
    RewardLegoMap* map;         /* +0x004 */
    unsigned char unk_008[8];
    void* rewardKey;            /* +0x010 the argument of reward_level_get */
    unsigned int width;         /* +0x014 */
    unsigned char unk_018[0x74];
    unsigned int crystals;      /* +0x08c */
    unsigned char unk_090[0x10];
    unsigned int ore;           /* +0x0a0 */
    unsigned char unk_0a4[0x14];
    RewardLegoBlock* blocks;    /* +0x0b8 */
    unsigned char unk_0bc[0x140];
    float oxygen;               /* +0x1fc */
    unsigned char unk_200[0x70];
    char* fullName;             /* +0x270 */
    unsigned char unk_274[8];
    int status;                 /* +0x27c 1 complete, 2 failed, 3 quit */
} RewardLegoLevel;

/* One reward item (0x210 bytes). */
typedef struct RewardLevelItem {
    unsigned int flags;         /* +0x000 REWARD_ITEM_FLAG_* */
    char name[32];              /* +0x004 */
    float importance;           /* +0x024 Importance */
    unsigned int quota;         /* +0x028 Quota */
    unsigned int unk_02c;
    float total;                /* +0x030 what the level had */
    float actual;               /* +0x034 what the player got */
    float percent;              /* +0x038 */
    unsigned int count;         /* +0x03c */
    unsigned int startCount;    /* +0x040 */
    unsigned int lostCount;     /* +0x044 */
    float damage;               /* +0x048 */
    unsigned int unk_04c;
    unsigned int percentInt;    /* +0x050 */
    char text[128];             /* +0x054 Text */
    char percentText[128];      /* +0x0d4 */
    unsigned int textX;         /* +0x154 */
    unsigned int textY;         /* +0x158 */
    unsigned int imageX;        /* +0x15c */
    unsigned int imageY;        /* +0x160 */
    unsigned int boxImageX;     /* +0x164 */
    unsigned int boxImageY;     /* +0x168 */
    char flicName[128];         /* +0x16c */
    unsigned int flicWidth;     /* +0x1ec */
    unsigned int flicHeight;    /* +0x1f0 */
    unsigned int flicX;         /* +0x1f4 */
    unsigned int flicY;         /* +0x1f8 */
    struct Flic* flic;          /* +0x1fc */
    Image* image;               /* +0x200 */
    struct TextureSurface* font; /* +0x204 */
    char* soundName;            /* +0x208 */
    Image* boxImage;            /* +0x20c */
} RewardLevelItem;

/* Building names collected at level start (0x198 bytes). */
typedef struct RewardBuildingCounts {
    const char* names[100];     /* +0x000 */
    unsigned int count;         /* +0x190 */
    unsigned int gridCount;     /* +0x194 */
} RewardBuildingCounts;

/* A level's reward state (0x318c bytes). */
typedef struct RewardLevel {
    int enabled;                /* +0x0000 Enable */
    int saveHasCapture;         /* +0x0004 */
    unsigned int saveCapture[8]; /* +0x0008 */
    RewardBuildingCounts buildingCounts; /* +0x0028 */
    unsigned int modifier;      /* +0x01c0 Modifier */
    unsigned int unk_1c4;
    unsigned int blockCount;    /* +0x01c8 */
    unsigned int buildingCount; /* +0x01cc */
    unsigned int buildingGridCount; /* +0x01d0 */
    char buildingNames[100][64]; /* +0x01d4 */
    RewardLevelItem walls;      /* +0x1ad4 walls and reinforcements */
    unsigned int itemCount;     /* +0x1ce4 */
    RewardLevelItem* items;     /* +0x1ce8 itemsArray */
    RewardLevelItem itemsArray[REWARD_ITEM_COUNT]; /* +0x1cec */
} RewardLevel;

/* One line of the scrolling summary (0x20 bytes). */
typedef struct RewardScrollLabel {
    unsigned int type;          /* +0x00 1 text window, 2 image, 3 text */
    float x;                    /* +0x04 */
    float y;                    /* +0x08 */
    void* textWindow;           /* +0x0c */
    Image* image;               /* +0x10 */
    char* text;                 /* +0x14 */
    struct TextureSurface* font; /* +0x18 */
    unsigned int flags;         /* +0x1c 1 centred, 2 fixed */
} RewardScrollLabel;

/* The scrolling summary (0x28 bytes). */
typedef struct RewardScroll {
    float top;                  /* +0x00 */
    Area2F area;                /* +0x04 area.y is the scroll position */
    float speed;                /* +0x14 ScrollSpeed */
    float length;               /* +0x18 */
    unsigned int labelCount;    /* +0x1c */
    RewardScrollLabel** labels; /* +0x20 */
    unsigned int flags;         /* +0x24 1 wraps */
} RewardScroll;

/* The unit's state (0x3250 bytes, in the common region: 00552980). */
typedef struct RewardGlobs {
    int display;                /* +0x0000 Display */
    int centreText;             /* +0x0004 CentreText */
    unsigned int unk_0008;
    float timer;                /* +0x000c Timer */
    RewardLevel* itemPart;      /* +0x0010 the item configuration */
    RewardLevel* current;       /* +0x0014 the level being scored */
    Image* wallpaper;           /* +0x0018 Wallpaper */
    RewardLevel level;          /* +0x001c the level's running counts */
    float scrollSpeed;          /* +0x31a8 ScrollSpeed */
    float vertSpacing;          /* +0x31ac VertSpacing */
    unsigned int unk_31b0[3];
    char* fontName;             /* +0x31bc Font */
    struct TextureSurface* font; /* +0x31c0 */
    char* titleFontName;        /* +0x31c4 TitleFont */
    struct TextureSurface* titleFont; /* +0x31c8 */
    char* backFontName;         /* +0x31cc BackFont */
    struct TextureSurface* backFont; /* +0x31d0 */
    BlockFile* config;          /* +0x31d4 */
    const char* gameName;       /* +0x31d8 */
    RewardScroll* scroll;       /* +0x31dc */
    Point2F saveButtonPos;      /* +0x31e0 */
    Image* saveButton;          /* +0x31e8 SaveButton */
    Image* saveButtonHi;        /* +0x31ec */
    Image* saveButtonIn;        /* +0x31f0 */
    Image* saveButtonOff;       /* +0x31f4 */
    Point2F advanceButtonPos;   /* +0x31f8 */
    Image* advanceButton;       /* +0x3200 AdvanceButton */
    Image* advanceButtonHi;     /* +0x3204 */
    Image* advanceButtonIn;     /* +0x3208 */
    Image* advanceButtonOff;    /* +0x320c */
    char* completeText;         /* +0x3210 CompleteText */
    char* failedText;           /* +0x3214 FailedText */
    char* quitText;             /* +0x3218 QuitText */
    unsigned int textPosX;      /* +0x321c TextPos */
    unsigned int textPosY;      /* +0x3220 */
    char statusText[32];        /* +0x3224 */
    int displayText;            /* +0x3244 DisplayText */
    int displayImages;          /* +0x3248 DisplayImages */
    int displayFlics;           /* +0x324c DisplayFlics */
} RewardGlobs;

extern RewardGlobs rewardGlobs;

int reward_initialise(void);                              /* 0045f030 */
void reward_load_item_sound(RewardLevelItem* item);       /* 0045f1f0 */
void reward_load_item_font(RewardLevelItem* item);        /* 0045f230 */
void reward_load_item_box_image(RewardLevelItem* item);   /* 0045f290 */
void reward_load_item_image(RewardLevelItem* item);       /* 0045f3e0 */
void reward_load_item_text(RewardLevelItem* item);        /* 0045f530 */
void reward_load_item_flic(RewardLevelItem* item);        /* 0045f5f0 */
void reward_shutdown(void);                               /* 0045f750 */
void reward_free_item_image(RewardLevelItem* item);       /* 0045f7b0 */
void reward_free_item_font(RewardLevelItem* item);        /* 0045f7d0 */
int reward_load_graphics(BlockFile* config, const char* gameName); /* 0045f7f0 */
int reward_load_buttons(BlockFile* config, const char* gameName);  /* 0045faf0 */
int reward_prepare_level(void);                           /* 0045fda0 */
void reward_load_item_importance(RewardLevelItem* item);  /* 004600a0 */
void reward_load_item_quota(RewardLevelItem* item);       /* 00460140 */
void reward_free_level(void);                             /* 004601e0 */
RewardLevel* reward_level_get(void* key);                 /* 004601f0 */
RewardLevel* reward_current_level_get(void);              /* 00460200 */
int reward_level_start(void);                            /* 00460220 */
RewardBuildingCounts* reward_building_counts_get(RewardBuildingCounts* out); /* 00460290 */
int reward_building_counts_callback(void* object, RewardBuildingCounts* counts); /* 00460310 */
int reward_prepare_results(void);                         /* 00460360 */
int reward_object_count_get(const char* name, int a, int b); /* 00460910 */
int reward_calculate_score(void);                         /* 00460950 */
int reward_create_scroll(void);                           /* 00460f00 */
int reward_format_values(void);                           /* 00461070 */
int reward_loop(void);                                    /* 00461410 */
void reward_draw_item(RewardLevelItem* item, unsigned int mask, unsigned int count,
                      float time);                       /* 00461790 */
int reward_draw_score(unsigned int count, float time);    /* 00461c90 */
int reward_do_buttons(int* state, int* saved);            /* 00461dd0 */
void reward_save_menu(void);                              /* 00462030 */
void reward_advance(void);                                /* 00462270 */
int reward_debug_keys(int* count, int* state, int* drawn, float* time); /* 00462390 */
void reward_restart_flic(RewardLevelItem* item);          /* 00462460 */
int reward_update_state(int* count, int* state, float* time); /* 004624a0 */
int reward_draw_values(int* count, int* drawn, float time); /* 00462600 */
void reward_update_input(float elapsed);                  /* 00462700 */
int reward_check_skip(void);                              /* 00462780 */

/* Shared: the level's counters, called from the port's game code. */
void reward_score_add(float value);                       /* 00462800 */
void global_float_at_00554adc_add_one(void);              /* 00462830 */
void global_float_at_00554efc_add_one(void);              /* 00462850 */
void global_counter_at_00555324_increment(void);          /* 00462870 */
void global_counter_at_00555328_increment(void);          /* 00462880 */
void global_counter_at_0055532c_increment(void);          /* 00462890 */
void global_float_accumulator_00555330_add(float value);  /* 004628a0 */
void global_float_accumulate_00555120(float value);       /* 004628c0 */

RewardScroll* reward_scroll_create(RewardScroll** out, Area2F area,
                                   float speed);          /* 004628e0 */
int reward_scroll_free(RewardScroll** scroll);            /* 00462960 */
RewardScrollLabel* reward_scroll_add_label(RewardScroll* scroll, const char* text,
                                           struct TextureSurface* font, float x, float y,
                                           unsigned int flags); /* 004629d0 */
void reward_scroll_length_set(RewardScroll* scroll, float length); /* 00462ab0 */
void reward_scroll_flags_set(RewardScroll* scroll, unsigned int flags); /* 00462ac0 */
int reward_scroll_update(RewardScroll* scroll);           /* 00462ad0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_REWARD_H */
