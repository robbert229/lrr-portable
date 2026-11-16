// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-021 Lego.c's level and block records, shared by every unit that
 * reads the current level (legoGlobs.currLevel). The +0x... comments are
 * the Win32 offsets; the pointer fields hold real pointers (ADR 0007
 * decision 4). */
#ifndef LEGORRR_GAME_LEGO_LEVEL_RECORD_H
#define LEGORRR_GAME_LEGO_LEVEL_RECORD_H

#include <stdint.h>

#include "objective_record.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TerrainMap;
struct Lego_Object;
struct Lego_ConstructionZone;
struct LegoBlockActivity2F;
struct NERPs_BlockPointer;

/* Lego_Level (0x284 bytes on Win32): the current level,
 * legoGlobs.currLevel (005560d4). The port's name for it was LevelState.
 * Unnamed fields are named by offset. */
typedef struct Lego_Block Lego_Block;
typedef struct Lego_Level {
    char* name;                     /* +0x000 */
    struct TerrainMap* map;         /* +0x004 Map3D */
    void* emergeMap;                /* +0x008 the emerge triggers */
    uint32_t f_00c;                 /* +0x00c the emerge trigger count */
    uint32_t f_010;                 /* +0x010 */
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
    void* textureSet;               /* +0x07c Detail_TextureSet */
    void** f_080;                   /* +0x080 tile meshes, f_074 * f_078 */
    void* f_084;                    /* +0x084 radar map record */
    struct LegoBlockActivity2F* f_088; /* +0x088 the block activities */
    int f_08c;                      /* +0x08c */
    uint32_t f_090;                 /* +0x090 */
    int f_094;                      /* +0x094 NERPs: crystals picked up */
    int f_098;                      /* +0x098 */
    int f_09c;                      /* +0x09c */
    int crystals;                   /* +0x0a0 NERPs reads it as the ore stored */
    uint32_t f_0a4;                 /* +0x0a4 */
    int f_0a8;                      /* +0x0a8 NERPs: ore picked up */
    int ore;                        /* +0x0ac */
    uint32_t f_0b0;                 /* +0x0b0 */
    char* nextLevel;                /* +0x0b4 */
    Lego_Block* blocks;             /* +0x0b8 width * height */
    Objective_Data objective;       /* +0x0bc */
    int f_110;                      /* +0x110 */
    struct NERPs_BlockPointer* blockPointers[0x37]; /* +0x114 */
    uint32_t f_1f0;                 /* +0x1f0 */
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
    char* fullName;                 /* +0x270 */
    uint32_t f_274;                 /* +0x274 BoulderAnimation */
    uint32_t f_278;                 /* +0x278 */
    int status;                     /* +0x27c */
    int disableStartTeleport;       /* +0x280 */
} Lego_Level;

/* Lego_Block (0x48 bytes on Win32): one map block,
 * currLevel->blocks[y * width + x]. */
struct Lego_Block {
    unsigned char f_00;             /* +0x00 */
    unsigned char texture;          /* +0x01 */
    unsigned char f_02;             /* +0x02 the surface (predug) type */
    unsigned char f_03;             /* +0x03 */
    unsigned char f_04;             /* +0x04 the NERPs block pointer */
    unsigned char f_05;             /* +0x05 */
    unsigned char f_06;             /* +0x06 */
    unsigned char f_07;             /* +0x07 */
    uint32_t flags1;                /* +0x08 */
    uint32_t flags2;                /* +0x0c */
    struct Lego_ConstructionZone* construct; /* +0x10 */
    float damage;                   /* +0x14 */
    struct LegoBlockActivity2F* activity; /* +0x18 */
    void* f_1c;                     /* +0x1c */
    short f_20;                     /* +0x20 */
    short f_22;                     /* +0x22 */
    uint32_t f_24;                  /* +0x24 cave-in count */
    uint32_t f_28;                  /* +0x28 click count */
    int f_2c;                       /* +0x2c NERPs: tutorial crystals */
    int f_30;                       /* +0x30 NERPs: tutorial ore */
    unsigned char f_34;             /* +0x34 */
    unsigned char f_35[3];          /* +0x35 */
    int f_38;                       /* +0x38 */
    unsigned int f_3c;              /* +0x3c */
    float f_40;                     /* +0x40 */
    uint32_t f_44;                  /* +0x44 */
};

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGO_LEVEL_RECORD_H */
