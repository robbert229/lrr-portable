// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-038 Roof.c (original translation unit, finding 505; finding 619):
 * the level's roof, one mesh group per block over the blocks the player
 * cannot see into.
 *
 * C-compatible header (ADR 0005). roof.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file, so its declarations move no other unit's symbol numbering
 * (finding 538); src/game/test_c_boundaries.cpp includes it.
 *
 * The names are the unit's own, after OpenLRR's Roof_* functions. In the
 * matching build lego_unit_imports.h points Lego.c's callers at them
 * (finding 619). The record layouts are the original's Win32 ones. */
#ifndef LEGORRR_GAME_ROOF_H
#define LEGORRR_GAME_ROOF_H

#ifdef __cplusplus
extern "C" {
#endif

#define ROOF_MAXHIDDEN 500

/* RoofBlock flags (+0x4). */
#define ROOF_FLAG_NOGROUP 0x1   /* the block has no mesh group yet */
#define ROOF_FLAG_UPDATE 0x2    /* the normals need recalculating */
#define ROOF_FLAG_ROTATED 0x4   /* the quad is split along the other diagonal */

typedef struct Roof_Vector3F {
    float x, y, z;
} Roof_Vector3F;

/* One block of the roof (8 bytes). */
typedef struct RoofBlock {
    unsigned int groupID;   /* +0x0 the block's group in the roof mesh */
    unsigned char flags;    /* +0x4 ROOF_FLAG_* */
} RoofBlock;

/* The unit's state, the common block 00552180-00552970 (0x7f0 bytes). */
typedef struct Roof_Globs {
    void* contMesh;                       /* +0x000 Container (mesh) */
    unsigned int width;                   /* +0x004 */
    unsigned int height;                  /* +0x008 */
    int hidden;                           /* +0x00c Roof_Hide's last value */
    int needUpdate;                       /* +0x010 some block's normals */
    void* texture;                        /* +0x014 Container_Texture */
    RoofBlock* grid;                      /* +0x018 width * height */
    RoofBlock* hiddenList[ROOF_MAXHIDDEN]; /* +0x01c */
    unsigned int hiddenCount;             /* +0x7ec */
} Roof_Globs;

extern Roof_Globs roofGlobs;

/* Roof_Initialise (FUN_00462da0): the roof mesh under contRoot and a grid
 * of width * height blocks, none with a group. Returns 1, or 0 on failure. */
int roof_initialise(void* contRoot, unsigned int width, unsigned int height);

/* Roof_SetTexture (FUN_00462e60): load the texture and set it on every
 * group. Returns 1, or 0 when it cannot be loaded. */
int roof_set_texture(const char* filename);

/* Roof_Shutdown (FUN_00462ed0). */
void roof_shutdown(void);

/* Roof_SetBlockRoofVertices (FUN_00462f20): the block's quad, with its
 * corners in the order the lower diagonal needs, and mark it for update. */
void roof_set_block_roof_vertices(unsigned int bx, unsigned int by,
                                  const Roof_Vector3F* pos0,
                                  const Roof_Vector3F* pos1,
                                  const Roof_Vector3F* pos2,
                                  const Roof_Vector3F* pos3);

/* Roof_Update (FUN_00463230): give each marked block's corners the mean of
 * the plane normals of the blocks that share them. */
void roof_update(void);

/* Roof_Hide (FUN_004634b0). */
void roof_hide(int hide);

/* Roof_HideBlock (FUN_004634f0): hide the block's group until
 * roof_show_hidden_blocks. */
void roof_hide_block(unsigned int bx, unsigned int by);

/* Roof_ShowHiddenBlocks (FUN_00463540). */
void roof_show_hidden_blocks(void);

/* Roof_GetBlockPlaneNormal (FUN_00463590): the normal of the block's quad.
 * Returns 0 outside the grid or when the block has no group. */
int roof_get_block_plane_normal(unsigned int bx, unsigned int by,
                                Roof_Vector3F* normal);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ROOF_H */
