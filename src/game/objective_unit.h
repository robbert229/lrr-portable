// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-029 Objective.c (original translation unit, finding 505), rebuilt in
 * original shape (ADR 0005, finding 631): the level's objectives, the
 * briefing, completion and failure text windows and the objective voice
 * streams.
 *
 * C-compatible header (ADR 0005). objective_unit.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the same functions.
 *
 * Natively the port's objective code (objective_port.cpp, objective.cpp)
 * stays the live one and nothing calls the unit; the records below are the
 * original's Win32 layout. */
#ifndef LEGORRR_GAME_OBJECTIVE_UNIT_H
#define LEGORRR_GAME_OBJECTIVE_UNIT_H

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Image;
struct Lego_Level;
struct Lego_Object;

/* Objective_Globs.flags (004ffbc0). */
#define OBJECTIVE_FLAG_BRIEFING             0x1
#define OBJECTIVE_FLAG_COMPLETED            0x2
#define OBJECTIVE_FLAG_FAILED               0x4
#define OBJECTIVE_FLAG_STATUSREAD           0x8
#define OBJECTIVE_FLAG_SHOWBRIEFINGADVISOR  0x10
#define OBJECTIVE_FLAG_SHOWACHIEVEADVISOR   0x20
#define OBJECTIVE_FLAG_HITTIMEFAIL          0x20
#define OBJECTIVE_FLAG_SHOWFAILEDADVISOR    0x80
#define OBJECTIVE_FLAG_CRYSTAL              0x100
#define OBJECTIVE_FLAG_ORE                  0x200
#define OBJECTIVE_FLAG_BLOCK                0x400
#define OBJECTIVE_FLAG_TIMER                0x800
#define OBJECTIVE_FLAG_CONSTRUCTION         0x1000

/* The four objective text pages (Objective:, Completion:, Failure: and
 * CrystalFailure:). */
#define OBJECTIVE_TYPE_COUNT 4

/* The level's objective data (Lego_Level +0xbc, 0x54 bytes). The pointer
 * fields are pointers only in the Win32 builds, where the unit runs. */
typedef struct Objective_Data {
    struct Image* image;            /* +0x00 ObjectiveImage */
    float imagePos[2];              /* +0x04 */
    struct Image* achievedImage;    /* +0x0c ObjectiveAcheivedImage */
    float achievedImagePos[2];      /* +0x10 */
    struct Image* failedImage;      /* +0x18 ObjectiveFailedImage */
    float failedImagePos[2];        /* +0x1c */
    char* achievedVideoName;        /* +0x24 ObjectiveAcheivedAVI */
    float achievedVideoPos[2];      /* +0x28 */
    int noAchievedVideoPos;         /* +0x30 */
    int achievedVideoPlayed;        /* +0x34 */
    unsigned int crystals;          /* +0x38 CrystalObjective */
    unsigned int ore;               /* +0x3c OreObjective */
    int block[2];                   /* +0x40 BlockObjective */
    float timer;                    /* +0x48 TimerObjective */
    int constructionType;           /* +0x4c ConstructionObjective */
    int constructionID;             /* +0x50 */
} Objective_Data;

/* 004574e0 */
void objective_load_objective_text(struct BlockRec* config,
                                   const char* gameName,
                                   const char* levelName,
                                   struct Lego_Level* level,
                                   const char* filename);
/* 00457d40 */
void objective_load_level(struct BlockRec* config, const char* gameName,
                          const char* levelName, struct Lego_Level* level,
                          unsigned int screenWidth,
                          unsigned int screenHeight);
/* 00458580 */
void objective_set_cry_ore_objectives(struct Lego_Level* level,
                                      unsigned int crystals,
                                      unsigned int ore);
/* 004585c0 */
void objective_set_block_objective(struct Lego_Level* level,
                                   const int* blockPos);
/* 004585f0 */
void objective_set_timer_objective(struct Lego_Level* level, float timer,
                                   int hitTimeFail);
/* 00458620 */
void objective_set_construction_objective(struct Lego_Level* level,
                                          int objType, int objID);
/* 00458650 */
int objective_has_achieved(void);
/* 00458660 */
void objective_set_end_teleport_enabled(int on);
/* 00458670 */
void objective_set_status(int status);
/* 004588e0 */
void objective_stop_showing_objective(void);
/* 004589a0 */
int objective_is_ended(void);
/* 004589c0 */
int objective_handle_keys(int nextKeyPressed, int leftButtonReleased,
                          int* outKeyPressed);
/* 00458be0 */
void objective_update(void* textWnd, struct Lego_Level* level,
                      float elapsedGame, float elapsedInterface);
/* 00459050 */
int objective_check_completed(struct Lego_Level* level, int* outFailed,
                              float elapsed);
/* 00459100 */
int objective_callback_check_completed_object(struct Lego_Object* liveObj,
                                              void* data);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OBJECTIVE_UNIT_H */
