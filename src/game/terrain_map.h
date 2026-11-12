// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-026 (original translation unit GAME-026, finding 505): the 3D
 * terrain map (the original's Map3D.c) and, at the end of the object, the
 * level-of-detail mesh lists (MeshLOD).
 *
 * The terrain map is a grid of blocks, one per map-file cell. Each block
 * holds the two face normals of its quad, its surface texture, flags, the
 * height read from the map file, a UV wobble offset and a highlight colour.
 * Each quad between four blocks is one group of the terrain mesh. The map
 * also keeps the texture-fade relation table, the no-fade textures, the list
 * of blocks made visible around a point, the list of blocks whose UVs
 * wobble, and ten texture-fade transitions drawn by a second mesh.
 *
 * C-compatible header (ADR 0005). terrain_map.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the shared functions.
 *
 * The offsets are the original's Win32 layout. The native port widens the
 * pointers; nothing outside the unit addresses the map by offset.
 *
 * The port's own terrain model (TerrainState, TerrainSurfaceGrid and the
 * render sidecars) is a different contract. It keeps the port's functions,
 * unannotated, in the port tail (terrain_map_port.cpp), and the native
 * callers use those (finding 556). The functions marked "shared" keep a
 * port name: their callers already use the original's contract. */
#ifndef LEGORRR_GAME_TERRAIN_MAP_H
#define LEGORRR_GAME_TERRAIN_MAP_H

#include "gods98/model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Block flags (+0x19). */
#define TERRAIN_MAP_BLOCK_ROTATED 0x01   /* the quad's diagonal is flipped */
#define TERRAIN_MAP_BLOCK_MODIFIED 0x04  /* vertex normals need an update */
#define TERRAIN_MAP_BLOCK_FLATTENED 0x08 /* a building flattened it */
#define TERRAIN_MAP_BLOCK_UVWOBBLE 0x10  /* in the UV wobble list */

/* Map flags (+0x73d0). */
#define TERRAIN_MAP_FLAG_MODIFIED 0x02       /* some block is modified */
#define TERRAIN_MAP_FLAG_WOBBLE_EMISSIVE 0x04 /* wobble blocks glow */

#define TERRAIN_MAP_MAX_TEXTURES 128
#define TERRAIN_MAP_MAX_VISIBLE 500
#define TERRAIN_MAP_MAX_UVWOBBLES 2500
#define TERRAIN_MAP_MAX_TRANSITIONS 10

typedef struct TerrainMapVector3 {
    float x, y, z;
} TerrainMapVector3;

typedef struct TerrainMapPoint2 {
    float x, y;
} TerrainMapPoint2;

typedef struct TerrainMapPoint2I {
    int x, y;
} TerrainMapPoint2I;

/* A mesh vertex (0x24 bytes), as Container_Mesh_GetVertices and
 * Container_Mesh_SetVertices pass it. */
typedef struct TerrainMapVertex {
    TerrainMapVector3 position;
    TerrainMapVector3 normal;
    float tu, tv;
    unsigned int colour;
} TerrainMapVertex;

/* One block (0x28 bytes). */
typedef struct TerrainMapBlock {
    TerrainMapVector3 normalA; /* +0x00 first triangle */
    TerrainMapVector3 normalB; /* +0x0c second triangle */
    unsigned char texture;     /* +0x18 */
    unsigned char flags;       /* +0x19 TERRAIN_MAP_BLOCK_* */
    unsigned char height;      /* +0x1a map-file height */
    unsigned char padding1;
    TerrainMapPoint2 uvWobble; /* +0x1c */
    unsigned char highlight;   /* +0x24 colour index */
    unsigned char padding2[3];
} TerrainMapBlock;

/* A block position in one of the map's lists (4 bytes). */
typedef struct TerrainMapListPoint {
    unsigned short x;
    unsigned short y;
} TerrainMapListPoint;

/* A texture-fade transition (0x38 bytes): the old texture of one block,
 * drawn over it by the transition mesh while it fades out. */
typedef struct TerrainMapTransition {
    int groupID;               /* +0x00 group in transMesh; -1 none yet */
    float timer;               /* +0x04 0 to 1 */
    unsigned int x, y;         /* +0x08 the block */
    TerrainMapPoint2 uvs[4];   /* +0x10 */
    unsigned int reserved;     /* +0x30 */
    unsigned int flags;        /* +0x34 bit 0: in use */
} TerrainMapTransition;

/* The map (0x73d4 bytes). */
typedef struct TerrainMap {
    unsigned int gridWidth;    /* +0x00 quads: blockWidth - 1 */
    unsigned int gridHeight;   /* +0x04 */
    unsigned int blockWidth;   /* +0x08 map-file cells */
    unsigned int blockHeight;  /* +0x0c */
    float blockSize;           /* +0x10 */
    float roughLevel;          /* +0x14 height per map-file unit */
    float worldX;              /* +0x18 x of block (0, 0) */
    float worldY;              /* +0x1c y of block (0, 0) */
    float lowestZ;             /* +0x20 the lowest vertex */
    void* mesh;                /* +0x24 one group per quad */
    TerrainMapBlock* blocks;   /* +0x28 blockWidth * blockHeight */
    void* textureSet;          /* +0x2c */
    void* material;            /* +0x30 */
    /* +0x34: the textures each texture fades to without a seam. */
    unsigned char fadePairs[TERRAIN_MAP_MAX_TEXTURES][TERRAIN_MAP_MAX_TEXTURES];
    unsigned char fadePairCount[TERRAIN_MAP_MAX_TEXTURES]; /* +0x4034 */
    int noFade[TERRAIN_MAP_MAX_TEXTURES];                  /* +0x40b4 */
    TerrainMapListPoint visible[TERRAIN_MAP_MAX_VISIBLE];  /* +0x42b4 */
    unsigned int visibleCount;                             /* +0x4a84 */
    TerrainMapListPoint uvWobbles[TERRAIN_MAP_MAX_UVWOBBLES]; /* +0x4a88 */
    unsigned int uvWobbleCount;                            /* +0x7198 */
    /* +0x719c */
    TerrainMapTransition transitions[TERRAIN_MAP_MAX_TRANSITIONS];
    void* transMesh;           /* +0x73cc */
    unsigned int flags;        /* +0x73d0 TERRAIN_MAP_FLAG_* */
} TerrainMap;

/* The map file (MAP format): a 16-byte header, then one 16-bit value per
 * cell. */
typedef struct TerrainMapFile {
    char signature[4];         /* +0x00 */
    unsigned int fileSize;     /* +0x04 */
    unsigned int width;        /* +0x08 */
    unsigned int height;       /* +0x0c */
    unsigned short cells[1];   /* +0x10 */
} TerrainMapFile;

/* A level-of-detail mesh list entry (MeshLOD, 0x18 bytes); the C view of
 * gods98/model.hpp's ModelEntryRecord, which defines it when included
 * first (test_c_boundaries.cpp). */
#ifndef LRR_MODEL_ENTRY_RECORD_DEFINED
#define LRR_MODEL_ENTRY_RECORD_DEFINED 1
struct ModelEntryRecord {
    void* object;              /* +0x00 the loaded mesh container */
    void* instance;            /* +0x04 the target it is swapped onto */
    char* name;                /* +0x08 the part name */
    int kind;                  /* +0x0c the set ID */
    unsigned int flags;        /* +0x10 bit 0 clone, bit 1 block head */
    struct ModelEntryRecord* next; /* +0x14 */
};
#endif

typedef void (*TerrainMapBlockCallback)(int x, int y);

/* FUN_0044e0c0 Map3D_Create: load the map file and build the terrain mesh
 * under `root`. */
TerrainMap* terrain_map_create(void* root, const char* filename,
                               float blockSize, float roughLevel);
/* FUN_0044e4d0 Map3D_InitRoughness: set every quad's vertex positions from
 * the block heights. */
void terrain_map_vertices_build(TerrainMap* map);
/* FUN_0044e670 Map3D_Remove. */
void terrain_map_remove(TerrainMap* map);
/* FUN_0044e6b0 Map3D_SetTextureNoFade. */
void terrain_map_texture_no_fade_set(TerrainMap* map,
                                     unsigned char texture);
/* FUN_0044e6d0 Map3D_SetBlockTexture with a fade from the old texture. */
void terrain_map_block_texture_fade_set(TerrainMap* map, unsigned int bx,
                                        unsigned int by,
                                        unsigned char texture,
                                        unsigned char direction);
/* FUN_0044e860 Map3D_IsBlockHidden? (the group's hidden flag). */
int terrain_map_block_hidden_get(TerrainMap* map, int bx, int by);
/* FUN_0044e880: update every block's vertex normals. */
void terrain_map_vertex_normals_update_all(TerrainMap* map);
/* FUN_0044e8c0 Map3D_CheckBuildingTolerance. */
int terrain_map_shape_tolerance_check(TerrainMap* map,
                                      const TerrainMapPoint2I* shape,
                                      unsigned int count, float tolerance,
                                      float maxVariation);
/* FUN_0044ead0 Map3D_FlattenShapeVertices. */
void terrain_map_shape_flatten(TerrainMap* map,
                               const TerrainMapPoint2I* shape,
                               unsigned int count, float tolerance);
/* FUN_0044edf0 Map3D_SetBlockRotated. */
void terrain_map_block_rotated_set(TerrainMap* map, unsigned int bx,
                                   unsigned int by, int on);
/* FUN_0044efb0 Map3D_SetBlockVertexModified. */
void terrain_map_block_vertex_modified(TerrainMap* map, unsigned int vx,
                                       unsigned int vy);
/* FUN_0044eff0 Map3D_Update. */
void terrain_map_update(TerrainMap* map, float elapsed);
/* FUN_0044f090 Map3D_UpdateTextureFades. */
void terrain_map_transitions_update(TerrainMap* map, float elapsed);
/* FUN_0044f1a0 Map3D_AddTextureMapping. */
void terrain_map_fade_pair_add(TerrainMap* map, unsigned char texA,
                               unsigned char texB);
/* FUN_0044f220 Map3D_SetTextureSet. */
void terrain_map_texture_set_set(TerrainMap* map, void* textureSet);
/* FUN_0044f230 Map3D_SetBlockTexture. */
void terrain_map_block_texture_set(TerrainMap* map, unsigned int bx,
                                   unsigned int by, unsigned char texture,
                                   unsigned char direction);
/* FUN_0044f380 Map3D_MoveVertex. */
void terrain_map_vertex_move(TerrainMap* map, unsigned int vx,
                             unsigned int vy, float zDist);
/* FUN_0044f490 Map3D_SetPerspectiveCorrectionAll. */
void terrain_map_perspective_correction_set(TerrainMap* map, int on);
/* FUN_0044f4e0 Map3D_SetBlockHighlight. */
int terrain_map_block_highlight_set(TerrainMap* map, int bx,
                                              int by, int highlight);
/* FUN_0044f540 Map3D_GetBlockHighlight. */
int terrain_map_block_highlight_get(TerrainMap* map, int bx,
                                              int by);
/* FUN_0044f570 Map3D_ClearBlockHighlight. */
void terrain_map_block_highlight_clear(TerrainMap* map, int bx, int by);
/* FUN_0044f5c0 Map3D_SetBlockEmissive. */
void terrain_map_block_emissive_set(TerrainMap* map, int bx, int by,
                                    int on, float r, float g, float b);
/* FUN_0044f640 Map3D_BlockToWorldPos. */
int terrain_map_block_to_world(TerrainMap* map, unsigned int bx,
                               unsigned int by, float* xOut, float* yOut);
/* FUN_0044f6d0 Map3D_WorldToBlockPos_NoZ. */
int terrain_map_world_to_block(TerrainMap* map, float x, float y,
                               int* bx, int* by);
/* FUN_0044f700 Map3D_WorldToBlockPos. */
int terrain_map_world_to_block_centre(TerrainMap* map, float x, float y,
                                      int* bx, int* by, float* centre);
/* FUN_0044f810 Map3D_WorldToBlockPos_Clamped. */
void terrain_map_world_to_block_clamped(TerrainMap* map, float x, float y,
                                        int* bx, int* by);
/* FUN_0044f870 Map3D_GetBlockPositionAndOffset. Always returns 0. */
int terrain_map_world_to_block_offset(TerrainMap* map,
                                      const TerrainMapPoint2* pos,
                                      TerrainMapPoint2* block,
                                      TerrainMapPoint2* offset);
/* FUN_0044f940 Map3D_GetWorldZ. */
float terrain_map_world_z_get(TerrainMap* map, float x, float y);
/* FUN_0044fab0 Map3D_GetBlockNormalAt? */
void terrain_map_world_normal_get(TerrainMap* map, float x, float y,
                                  TerrainMapVector3* normal);
/* FUN_0044fb90 Map3D_FindNearestEdgePoint? */
int terrain_map_world_edge_point_get(TerrainMap* map, float x, float y,
                                     int diagonal, float distance,
                                     float* xOut, float* yOut);
/* FUN_0044fe70 Map3D_UnkWorldZ (the height from the map file only). */
float terrain_map_world_z_unmodified_get(TerrainMap* map, float x, float y);
/* FUN_00450060 Map3D_GetBlockFirstVertexPosition. */
void terrain_map_block_first_vertex_get(TerrainMap* map, int bx, int by,
                                        TerrainMapVector3* position);
/* FUN_004500d0 Map3D_GetBlockVertexPositions: returns the rotated flag. */
int terrain_map_block_vertices_get(TerrainMap* map, unsigned int bx,
                                   unsigned int by,
                                   TerrainMapVector3* positions);
/* FUN_00450220 Map3D_GetBlockVertexPositions_NoRot. */
void terrain_map_block_vertices_raw_get(TerrainMap* map, int bx, int by,
                                        TerrainMapVector3* positions);
/* FUN_004502c0 Map3D_IsInsideDimensions (shared: the callers pass the
 * map, whose first two words are gridWidth and gridHeight). */
int level_coord_pair_within_bounds_target(const unsigned int* dimensions,
                                          unsigned int cx,
                                          unsigned int cy);
/* FUN_004502e0 Map3D_GetIntersections. */
int terrain_map_screen_pick(TerrainMap* map, void* viewport,
                            unsigned int mx, unsigned int my,
                            unsigned int* bx, unsigned int* by,
                            TerrainMapVector3* intersection);
/* FUN_00450560 Map3D_GetIntersection. */
int terrain_map_ray_intersection(TerrainMap* map,
                                 const TerrainMapVector3* origin,
                                 const TerrainMapVector3* direction,
                                 TerrainMapVector3* endPoint,
                                 TerrainMapPoint2I* block,
                                 unsigned int iterations);
/* FUN_00450600 Map3D_AddVisibleBlocksInRadius_AndDoCallbacks. */
void terrain_map_visible_radius_add(TerrainMap* map, int bx, int by,
                                    int radius,
                                    TerrainMapBlockCallback callback);
/* FUN_00450700 Map3D_SetBlockHidden? (hide the quad's group). */
void terrain_map_block_hide(TerrainMap* map, int bx, int by, int hide);
/* FUN_00450730 Map3D_AddVisibleBlock. */
void terrain_map_visible_add(TerrainMap* map, int bx, int by);
/* FUN_00450780 Map3D_HideVisibleBlocksList. */
void terrain_map_visible_clear(TerrainMap* map);
/* FUN_004507d0 Map3D_GetVertexPosition? (by vertex coordinates). */
void terrain_map_vertex_position_get(TerrainMap* map, unsigned int vx,
                                     unsigned int vy, float* x, float* y,
                                     float* z);
/* FUN_00450890 Map3D_BlockSize (shared). */
float terrain_context_scale_get(const void* context);
/* FUN_004508a0 Map3D_CheckBlockDirection? (shared). */
int cell_neighbor_direction_code(int x1, int y1, int x2, int y2);
/* FUN_00450960 Map3D_SetBlockUVWobbles. */
void terrain_map_block_uv_wobble_set(TerrainMap* map, unsigned int bx,
                                     unsigned int by, int on);
/* FUN_00450a80 Map3D_SetEmissive. */
void terrain_map_wobble_emissive_set(TerrainMap* map, int on);
/* FUN_00450b60 Map3D_UpdateAllBlockNormals? (the UV wobble). */
void terrain_map_uv_wobble_update(TerrainMap* map, float elapsed);
/* FUN_00450f30 Map3D_GenerateBlockVertexNormals. */
void terrain_map_block_vertex_normals_update(TerrainMap* map,
                                             unsigned int vx,
                                             unsigned int vy);
/* FUN_00451180 Map3D_IsBlockPairFadeMapped? */
int terrain_map_block_pair_fades(TerrainMap* map, unsigned int bx1,
                                 unsigned int by1, unsigned int bx2,
                                 unsigned int by2);
/* FUN_00451230 Map3D_SetBlockVertexNormal. */
void terrain_map_block_vertex_normal_set(TerrainMap* map, unsigned int bx,
                                         unsigned int by,
                                         unsigned int vertex,
                                         const TerrainMapVector3* normal);
/* FUN_004512d0 Map3D_GetBlockVertexNormal. */
int terrain_map_block_vertex_normal_get(TerrainMap* map, unsigned int bx,
                                        unsigned int by,
                                        unsigned int vertex,
                                        TerrainMapVector3* normal);
/* FUN_00451450 Map3D_OffsetBlockVertex. */
void terrain_map_block_vertex_offset(TerrainMap* map, unsigned int bx,
                                     unsigned int by, unsigned int vertex,
                                     const TerrainMapVector3* offset);
/* FUN_004514f0 Map3D_GenerateBlockPlaneNormals. */
void terrain_map_block_face_normals_update(TerrainMap* map,
                                           unsigned int bx,
                                           unsigned int by);
/* FUN_004515a0 Map3D_MapFileGetSpecs. */
void terrain_map_file_specs_get(const TerrainMapFile* file,
                                unsigned int* width, unsigned int* height);
/* FUN_004515c0 Map3D_MapFileBlockValue. */
unsigned short terrain_map_file_block_value(const TerrainMapFile* file,
                                            unsigned int bx,
                                            unsigned int by,
                                            unsigned int width);
/* FUN_004515e0 Map3D_BlockRayIntersection? */
int terrain_map_block_ray_intersection(TerrainMap* map, int bx, int by,
                                       const TerrainMapVector3* origin,
                                       const TerrainMapVector3* direction,
                                       TerrainMapVector3* intersection);

/* MeshLOD (shared: the port's names and contracts). */
/* FUN_004519b0 MeshLOD_Create. */
ModelEntryRecord* model_entry_record_load(ModelEntryRecord* previous,
                                          const char* partName,
                                          const char* dir,
                                          const char* meshName,
                                          int setID);
/* FUN_00451ab0 MeshLOD_CreateEmpty. */
ModelEntryRecord* model_primitive_record_create(ModelEntryRecord* previous,
                                                const char* name, int kind);
/* FUN_00451b30 MeshLOD_Clone. */
ModelEntryRecord* model_entry_list_flatten(const ModelEntryRecord* head);
/* FUN_00451bc0 MeshLOD_SwapTarget. */
void resource_list_update_matching_key(void* list, void* library,
                                       void* update_value,
                                       unsigned int key);
/* FUN_00451c30 MeshLOD_RemoveTargets. */
void linked_record_field4_clear(void* record);
/* FUN_00451c50 MeshLOD_Free. */
void owned_object_reference_chain_release(void* head);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TERRAIN_MAP_H */
