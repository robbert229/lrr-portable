// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-049 Water.c (original translation unit, finding 505; finding 614):
 * the level's water pools.
 *
 * C-compatible header (ADR 0005). water.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file, so its declarations move no other unit's symbol numbering
 * (finding 538); src/game/test_c_boundaries.cpp includes it.
 *
 * The names are the unit's own. In the matching build water_link_names.h
 * and lego_unit_imports.h give the callers' names to the functions Lego.c
 * calls (finding 614). The record layouts are the original's Win32 ones. */
#ifndef LEGORRR_GAME_WATER_H
#define LEGORRR_GAME_WATER_H

#include "lego_unit.h"
#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WATER_MAXPOOLS 10
#define WATER_MAXPOINTS 100
#define WATER_MAXENTRIES 10
#define WATER_MAXJOINS 10

/* WaterPool flags (+0x424). */
#define WATER_FLAG_VISIBLE 0x1 /* a block beside the pool is open */
#define WATER_FLAG_DRAINED 0x2 /* the level fell to its target */
#define WATER_FLAG_FILLED 0x4  /* the level rose to its target */

/* One edge of a pool where water can flow out (0x18 bytes). */
typedef struct WaterEntry {
    unsigned int pointIndex; /* +0x00 the pool block it leaves */
    unsigned int direction;  /* +0x04 0 to 3: up, right, down, left */
    float level;             /* +0x08 the level it flows at */
    float spread;            /* +0x0c the front's distance */
    float recede;            /* +0x10 the tail's distance */
    int active;              /* +0x14 the wall is open */
} WaterEntry;

/* One pool: a run of lake blocks (Lego_Block terrain 7), 0x428 bytes. */
typedef struct WaterPool {
    TerrainMapPoint2 points[WATER_MAXPOINTS]; /* +0x000 block positions */
    unsigned int pointCount;                  /* +0x320 */
    WaterEntry entries[WATER_MAXENTRIES];     /* +0x324 */
    unsigned int entryCount;                  /* +0x414 */
    float startLevel;                         /* +0x418 */
    float level;                              /* +0x41c */
    void* mesh;                               /* +0x420 Container */
    unsigned int flags;                       /* +0x424 WATER_FLAG_* */
} WaterPool;

/* Two pools that one row joins; the second's points move to the first. */
typedef struct WaterJoin {
    WaterPool* first;  /* +0x0 */
    WaterPool* second; /* +0x4 */
} WaterJoin;

/* The unit's state, 0x29ec bytes at 00549520: a common block. */
typedef struct Water_Globs {
    WaterPool poolList[WATER_MAXPOOLS]; /* 00549520 */
    unsigned int poolCount;             /* 0054beb0 */
    WaterJoin joinList[WATER_MAXJOINS]; /* 0054beb4 */
    unsigned int joinCount;             /* 0054bf04 */
    float digDepth;                     /* 0054bf08 */
} Water_Globs;

extern Water_Globs waterGlobs;

void water_initialise(void* contRoot, Lego_Level* level);           /* 0046db90 */
void water_init_entries(void* contRoot, Lego_Level* level);         /* 0046dd10 */
void water_init_vertices(void* contRoot, Lego_Level* level);        /* 0046de80 */
void water_change_view_mode(int on);                                /* 0046e1c0 */
void water_destroy_wall_complete(Lego_Level* level, int bx, int by); /* 0046e220 */
void water_seal_block(int bx, int by);                              /* 0046e330 */
void water_update(Lego_Level* level, float elapsedGame);            /* 0046e390 */
void water_update_flow_blocks(Lego_Level* level, float elapsedGame); /* 0046e610 */
WaterPool* water_find_pool(int bx, int by, unsigned int* outEntry); /* 0046e8a0 */
int water_compare_pools(const void* a, const void* b);              /* 0046e9a0 */
WaterPool* water_find_joining_pool(unsigned int by, unsigned int bxStart,
                                   unsigned int bxEnd);             /* 0046e9d0 */
void water_add_pool_points(WaterPool* pool, unsigned int by,
                           unsigned int bxStart, unsigned int bxEnd); /* 0046ead0 */
void water_create_pool(unsigned int by, unsigned int bxStart,
                       unsigned int bxEnd);                         /* 0046eb30 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_WATER_H */
