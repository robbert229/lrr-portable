// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-013 (original translation unit GAME-013, finding 505): the electric
 * fences.
 *
 * A fence is a type-0xb object standing on one block. Each fenced block
 * owns a 0x14-byte list node from the unit's free list, and the unit keeps
 * one 0xc-byte record per map block: the fence node, the connection stud
 * (a type-0xe object between two fences) and a flag word. Every frame the
 * unit clears the flags, then sends a spark from each powered building and
 * along each chain of fences, setting one bit per direction and spawning
 * the beam effect between neighbours. A unit that walks into a live block
 * is struck with the FenceSpark weapon.
 *
 * C-compatible header (ADR 0005). terrain_cell_objects.c is compiled as C
 * in the matching build and as C++ in the native port. Shared C++ headers
 * do not include this file: every include and declaration a header adds
 * shifts MSVC's symbol numbering (finding 506). The port's own fence model
 * stays in the port tail terrain_cell_objects_port.cpp (finding 566), and
 * native callers keep calling it. */
#ifndef LEGORRR_GAME_TERRAIN_CELL_OBJECTS_H
#define LEGORRR_GAME_TERRAIN_CELL_OBJECTS_H

#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct GameContext;

/* Lego_Level fields the unit reads (Lego.c, 0x284 bytes; the port's
 * LevelState words). */
typedef struct ElectricFenceLevelBlock {
    unsigned char unk_00[2];
    unsigned char predug;       /* +0x02 6 and 7 are not fenceable */
    unsigned char unk_03[5];
    unsigned int flags1;        /* +0x08 0x8: the block is ground */
    unsigned char unk_0c[0x3c];
} ElectricFenceLevelBlock;      /* 0x48 */

typedef struct ElectricFenceLevel {
    char* name;                 /* +0x000 */
    TerrainMap* map;            /* +0x004 */
    unsigned char unk_008[0xc];
    unsigned int width;         /* +0x014 */
    unsigned int height;        /* +0x018 */
    unsigned char unk_01c[0x9c];
    ElectricFenceLevelBlock* blocks; /* +0x0b8 */
} ElectricFenceLevel;

/* LegoObject fields the unit reads (LegoObject.c, 0x40c bytes). */
typedef struct ElectricFenceObject {
    int type;                   /* +0x000 4 is a building */
    unsigned char unk_004[0x14];
    void* stats;                /* +0x018 the building's shape owner */
    unsigned char unk_01c[0x3c8];
    unsigned int flags3;        /* +0x3e4 0x2000000: on the fence grid */
    unsigned int flags4;        /* +0x3e8 0x20000000: carries a charge */
} ElectricFenceObject;

/* One list node per fence (0x14 bytes). A live node points nextFree at
 * itself. */
typedef struct ElectricFenceNode {
    ElectricFenceObject* object;    /* +0x00 */
    int blockX;                     /* +0x04 */
    int blockY;                     /* +0x08 */
    float timer;                    /* +0x0c the spark recharge */
    struct ElectricFenceNode* nextFree; /* +0x10 */
} ElectricFenceNode;

/* One record per map block (0xc bytes). */
typedef struct ElectricFenceBlock {
    ElectricFenceNode* fence;       /* +0x00 */
    ElectricFenceObject* stud;      /* +0x04 */
    unsigned int flags;             /* +0x08 0x1-0x8 beams, 0x100 visited */
} ElectricFenceBlock;

typedef void (*ElectricFenceNodeCallback)(ElectricFenceNode* node,
                                          void* data);

/* FUN_0040cdc0 ElectricFence_Initialise. */
void electric_fence_initialise(ElectricFenceLevel* level);
/* FUN_0040ce30 ElectricFence_Shutdown. */
void electric_fence_shutdown(void);
/* FUN_0040ce80 ElectricFence_Restart. */
void electric_fence_restart(ElectricFenceLevel* level);
/* FUN_0040cea0: refresh the surface of a block and its four neighbours. */
void electric_fence_block_surfaces_update(int bx, int by);
/* FUN_0040cf50 ElectricFence_AddFence. */
void electric_fence_add(ElectricFenceObject* object);
/* FUN_0040cf80 ElectricFence_CreateFence. */
ElectricFenceNode* electric_fence_create(ElectricFenceObject* object, int bx,
                                         int by);
/* FUN_0040d030 ElectricFence_AddList. */
void electric_fence_add_list(void);
/* FUN_0040d0a0 ElectricFence_RemoveFence. */
void electric_fence_remove(ElectricFenceObject* object);
/* FUN_0040d100 ElectricFence_RemoveList. */
void electric_fence_remove_node(ElectricFenceNode* node, int bx, int by);
/* FUN_0040d170 ElectricFence_Debug_PlaceFence. */
int electric_fence_debug_place(int bx, int by);
/* FUN_0040d1f0 ElectricFence_Debug_RemoveFence. */
int electric_fence_debug_remove(int bx, int by);
/* FUN_0040d240 ElectricFence_CanPlaceFenceAtBlock. */
int electric_fence_can_place_at_block(int bx, int by);
/* FUN_0040d3f0: a building block, and optionally a powered one. */
int electric_fence_block_is_building(ElectricFenceLevel* level, int bx,
                                     int by, int powered);
/* FUN_0040d450 ElectricFence_Update. */
void electric_fence_update(float elapsed);
/* FUN_0040d490 ElectricFence_RunThroughLists. */
void electric_fence_run_through_lists(ElectricFenceNodeCallback callback,
                                      void* data);
/* FUN_0040d4f0: spark from a building block, or from an object's block. */
int electric_fence_spark_from(ElectricFenceObject* object, int bx, int by);
/* FUN_0040d5e0: LegoObject callback; spark from each powered building. */
int electric_fence_building_spark_callback(ElectricFenceObject* object,
                                           float* elapsed);
/* FUN_0040d720: LegoObject callback; clear a building block's flags. */
int electric_fence_building_clear_callback(ElectricFenceObject* object,
                                           void* data);
/* FUN_0040d770: node callback; clear the flags and count down the timer. */
void electric_fence_node_update_callback(ElectricFenceNode* node,
                                         float* elapsed);
/* FUN_0040d850: spread a spark from a block along the fences. */
void electric_fence_spark(int bx, int by, int force, float elapsed);
/* FUN_0040dc20: spawn the beam between two blocks. */
int electric_fence_beam_create(int bx, int by, int tx, int ty);
/* FUN_0040dd90: the world vector from one block to another. */
void electric_fence_block_vector_get(int bx, int by, int tx, int ty,
                                     TerrainMapVector3* out);
/* FUN_0040de40: strike an object standing on a live block. */
int electric_fence_object_strike(ElectricFenceObject* object);
/* FUN_0040e0c0: the beam to an object, and its damage. */
void electric_fence_object_beam(ElectricFenceObject* object,
                                const TerrainMapPoint2* position, int bx,
                                int by);
/* FUN_0040e1e0: a stud fits between two fences at a block. */
int electric_fence_stud_fits(ElectricFenceLevel* level, int bx, int by);
/* FUN_0040e350: add or remove a block's connection stud. */
void electric_fence_stud_set(ElectricFenceLevel* level, int bx, int by,
                             int on);
/* FUN_0040e460: the block has a fence. */
int electric_fence_block_is_fence(int bx, int by);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TERRAIN_CELL_OBJECTS_H */
