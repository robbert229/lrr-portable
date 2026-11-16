// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-030 Objective.c's per-level record, embedded in Lego_Level
 * (lego_level_record.h). The +0x... comments are the Win32 offsets; the
 * pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_OBJECTIVE_RECORD_H
#define LEGORRR_GAME_OBJECTIVE_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

struct Image;

/* The level's objective data (Lego_Level +0xbc, 0x54 bytes on Win32). */
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

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OBJECTIVE_RECORD_H */
