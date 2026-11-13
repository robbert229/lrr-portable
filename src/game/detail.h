// SPDX-License-Identifier: LGPL-3.0-or-later
/* Detail.c (original translation unit GAME-011, finding 615): the block
 * detail meshes and their texture sets.
 *
 * C-compatible header (ADR 0005). detail.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file: every include and declaration a header adds shifts MSVC's
 * symbol numbering (finding 506). src/game/test_c_boundaries.cpp includes
 * it.
 *
 * In the matching build detail_link_names.h gives eight of these functions
 * the port's names. Natively they keep these names and nothing calls them;
 * the port tail tile_variants_port.cpp serves the port's names. */
#ifndef LEGORRR_GAME_DETAIL_H
#define LEGORRR_GAME_DETAIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* A width x height grid of textures, "<base><x><y>.bmp" (0xc bytes). */
typedef struct Detail_TextureSet {
    unsigned int width;   /* +0x0 */
    unsigned int height;  /* +0x4 */
    void** textures;      /* +0x8, row-major */
} Detail_TextureSet;

#define DETAIL_MESH_COUNT 2
#define DETAIL_MAXCLONES 100

/* A block's two detail meshes and their clones (0x340 bytes). */
typedef struct Detail_Mesh {
    void* cont[DETAIL_MESH_COUNT];                       /* +0x000 */
    void* clones[DETAIL_MESH_COUNT][DETAIL_MAXCLONES];   /* +0x008 */
    float blockSize;                                     /* +0x328 */
    unsigned int cloneCount;                             /* +0x32c */
    unsigned int reserved[3];                            /* +0x330 */
    Detail_TextureSet* textureSet;                       /* +0x33c */
} Detail_Mesh;

typedef struct Detail_Vector3F {
    float x, y, z;
} Detail_Vector3F;

/* Detail_LoadTextureSet (0040b310). */
Detail_TextureSet* detail_load_texture_set(const char* textureBaseName,
                                           unsigned int width,
                                           unsigned int height);
/* Detail_FreeTextureSet (0040b410). */
void detail_free_texture_set(Detail_TextureSet* textureSet);
/* 0040b470: how much of the screen a block's four corners cover, less a
 * quarter for each corner that is off screen (unless noClip). */
float detail_get_block_screen_area(Detail_Vector3F* vertPoses, void* vp,
                                   const float* screenSize, int noClip);
/* Detail_LoadMeshes (0040b5f0). */
Detail_Mesh* detail_load_meshes(void* root, const char* uncoveredName,
                                const char* surfaceName, float blockSize,
                                Detail_TextureSet* textureSet);
/* Detail_FreeMesh (0040b7d0). */
void detail_free_mesh(Detail_Mesh* mesh);
/* Detail_ResetAll (0040b810): removes every clone. */
void detail_reset_all(Detail_Mesh* mesh);
/* Detail_GetTexture (0040b850): the texture is 0xXY, x in the high
 * nibble. */
void* detail_get_texture(Detail_TextureSet* textureSet,
                         unsigned char texture);
/* 0040b880: clones both meshes onto a block and places them, the second
 * turned by two corners. */
void detail_add_block(Detail_Mesh* mesh, Detail_Vector3F* vertPoses,
                      unsigned char texture, unsigned char direction,
                      float depth, float alpha);
/* 0040ba00: places a clone on three corners and textures it. */
void detail_place_mesh(void* cont, float blockSize, Detail_Vector3F* a,
                       Detail_Vector3F* b, Detail_Vector3F* c, float depth,
                       float alpha, void* texture, unsigned char flags);
/* 0040bb90: orients and positions a clone on three corners and sets its
 * matrix. */
void detail_set_mesh_frame(void* cont, float blockSize, Detail_Vector3F* a,
                           Detail_Vector3F* b, Detail_Vector3F* c);
/* 0040bd60: the clone's matrix. */
void detail_matrix_setup(float* m, float sx, float sy, float y, float x2,
                         float y2);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_DETAIL_H */
