// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — level state + `.map` loading pipeline (finding 07).
 *
 * Reconstruction of the corpus's LevelLoad @ 0x429890, MapLoadPipeline @
 * 0x42b510 and the ten layer loaders (0x42bd20 … 0x42cc90) plus
 * CellInitPass @ 0x42f6f0. A level is a config-DB group in Lego.cfg (one
 * group per level name) holding ~35 scalar keys and *paths to twelve .map
 * grid files*; every grid materializes into a 2-D array of 0x48-byte
 * voxel records. `.map` is read raw into RAM and accessed in place — the
 * file layout is the memory layout (magic `MAP `, u32 len, u32 w/h, u16
 * cells row-major `y*w+x`).
 *
 * The voxel array lives at LevelState.voxels (+0xb8 in the corpus), a
 * w*h*0x48 buffer. Loader-written fields (documented in finding 07):
 *
 *   +0x02 erosion/cryo byte   LoadErodeCryoreLayers (reads TerrainMap)
 *   +0x04 surface byte − bias  LoadTerrainMap        (reads BlockPointersMap)
 *   +0x05 path/AI byte − bias  LoadPathAiLayers      (reads CryOreMap)
 *   +0x06 emerge value >> 1    LoadEmergeLayer       (reads ErodeMap)
 *   +0x08 flag word            LoadPredugLayer/LoadFallinLayer/…  (Predug/Path)
 *   +0x0C extra flags +5 records LoadLayerL           (reads EmergeMap)
 *   +0x20 u16 per-cell seed    (thunk_FUN_0048e170 PRNG)
 *   +0x34 byte layer           LoadLayerM             (reads AIMap)
 *   +0x38/+0x3C block-pointer  LoadBlockPointers      (reads FallinMap)
 *   +0x40 hash mod word        LoadBlockPointers
 *
 * Note the key→loader mapping is the *code's* mapping (positional args of
 * the corpse pipeline), which the LEVEL config keys do not textually match
 * (e.g. "TerrainMap" feeds the erosion/cryo writer, "BlockPointersMap"
 * feeds the surface-type writer). Port faithfully reproduces the pipeline.
 *
 * Discipline (ADR 0003/0004): plain C-shaped C++, no OS headers, no STL.
 * Field offsets in the struct comment refer to the decompiler offsets
 * (DAT_005560d4 + off); the struct is offset-exact where other modules
 * (objective/savegame) read it.
 */
#ifndef LEGORRR_GAME_LEVEL_HPP
#define LEGORRR_GAME_LEVEL_HPP

#include <cstdint>
#include <cstddef>
#include <string.h>
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
#include <stdlib.h>
#endif

#include "gods98/block.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Level state block size — corpus allocates 0x284 bytes (0xA1 u32 words). */
#define LEVEL_STATE_SIZE 0x284

/* Field offsets within the level block (corpus DAT_005560d4 + off). */
#define LEVEL_OFF_LEVEL_NAME       0x00  /* const char* (level group/name)      */
#define LEVEL_OFF_TERRAIN          0x04  /* TerrainCreate handle                 */
#define LEVEL_OFF_LAYERL_RECORDS   0x08  /* LoadLayerL record array (0x4C stride)*/
#define LEVEL_OFF_LAYERL_COUNT     0x0C  /* LoadLayerL record count              */
#define LEVEL_OFF_WIDTH            0x14  /* grid width  (from first map header)  */
#define LEVEL_OFF_HEIGHT           0x18  /* grid height (from first map header)  */
#define LEVEL_OFF_BLOCKSIZE        0x1C  /* float                                  */
#define LEVEL_OFF_DIGDEPTH         0x20  /* float                                  */
#define LEVEL_OFF_ROOFHEIGHT       0x24  /* float                                  */
#define LEVEL_OFF_ROUGHLEVEL       0x28  /* float                                  */
#define LEVEL_OFF_USEROOF          0x2C  /* int                                    */
#define LEVEL_OFF_SAFECAVERNS      0x30  /* int (SafeCaverns)                      */
#define LEVEL_OFF_SELBOXHEIGHT     0x34  /* float                                  */
#define LEVEL_OFF_NEXTLEVEL        0xB4  /* const char* (nextlevel key)           */
#define LEVEL_OFF_VOXELS           0xB8  /* voxel array w*h*0x48                   */
#define LEVEL_OFF_OBJ_IMGBASE      0xBC  /* objective image handles/dims (0x54 B)  */
#define LEVEL_OFF_OBJ_RESULT       0x27C /* objective result index 0/1/2          */
#define LEVEL_OFF_SPAWN_INTERVAL   0x268 /* SpawnCoordinatePoolTick re-arm float   */
#define LEVEL_OFF_SPAWN_TYPE       0x26C /* SpawnCoordinatePoolTick type (0x14 off)*/
#define LEVEL_OFF_TERRAIN_LOADED   0x110 /* 1 once LoadTerrainMap succeeded        */
#define LEVEL_OFF_STARTP           0x1F4 /* +0x7d StartFP flag                    */
#define LEVEL_OFF_NODRAIN          0x1F8 /* +0x7e NoDrain flag                    */
#define LEVEL_OFF_OXYGEN_RATE      0x200 /* +0x80 float OxygenRate                */
#define LEVEL_OFF_AIR_SUPPLY       0x1fc /* air-supply amount (FUN_00424560)      */
#define LEVEL_OFF_TOLERANCE        0x208 /* +0x82 float BuildingTolerance         */
#define LEVEL_OFF_MAXVARIATION     0x20C /* +0x83 float BuildingMaxVariation     */
#define LEVEL_OFF_CRYSTALS         0x230 /* +0x8c live crystal count (reader)      */
#define LEVEL_OFF_ORE              0x280 /* +0xa0 live ore count (reader)         */
#define LEVEL_OFF_STATE84          0x84  /* active-engine dword read (FUN_0042b500) */
#define LEVEL_OFF_RES_A            0xA0  /* upgrade resource check (selector 0)    */
#define LEVEL_OFF_ENGINE_B0        0xB0  /* active-engine dword read (FUN_00429850) */
#define LEVEL_OFF_RES_B            0xAC  /* upgrade resource check (selector !=0)  */
#define LEVEL_OFF_ENGINE_FLOATS    0x210 /* indexed float array (FUN_00429860)     */
#define LEVEL_OFF_ENGINE_F260      0x260 /* active-engine float read (FUN_00429880) */

/* Voxel record stride (bytes). */
#define LEVEL_VOXEL_STRIDE 0x48

/* The `.map` file container (finding 07): read raw into RAM, accessed in
 * place. `data` is owned by the handle; freed by level_map_close. */
typedef struct MapFile {
    uint8_t* data;
    int32_t  size;
    int32_t  width;   /* +0x08 in the file image */
    int32_t  height;  /* +0x0C */
} MapFile;

/* Open a `.map` resource (loose file or wad member) and byteswap-free read
 * dims. Returns NULL when the resource cannot be opened. */
MapFile* level_map_open(const char* name);

void level_map_close(MapFile* m);

/* Cell (u16) at (x,y): file offset +0x10 + (y*width+x)*2 (MapFileGetCell). */
uint16_t level_map_cell(const MapFile* m, int x, int y);

/* Level state block. Offset-exact subset of the corpus's 0x284-byte block
 * (see the LEVEL_OFF_* constants); fields accessed through level_get/put
 * helpers so every module agrees on layout.
 *
 * The corpus is a 32-bit program: its pointer-valued words occupy exactly
 * one slot. On the 64-bit native build those slots would not hold a native
 * pointer, so the real allocations are registered in `_ptr_*` below (the
 * low word of each pointer is still stored at the corpus offset for
 * fidelity). Other modules must use level_int/level_float on the corpus
 * slots, and level_name()/level_voxels() etc. for the live pointers. */
typedef struct LevelState {
    uint32_t word[LEVEL_STATE_SIZE / 4];
    /* --- reconstruction-side pointer registry (not part of the corpus) --- */
    void*    ptr_terrain;    /* corpus +0x04  TerrainCreate handle */
    void*    ptr_layerl;     /* corpus +0x08  LoadLayerL records   */
    void*    ptr_voxels;     /* corpus +0xB8  voxel array         */
    void*    ptr_next;       /* corpus +0xB4  nextlevel name      */
    void*    ptr_name;       /* corpus +0x00  level name          */
    /* +0x114 pool-table chain heads: the corpus strides 0x37 dword slots
     * in the block; the port keeps them as a widened-link void* image
     * (the object_pool_lists_free_all convention) off the corpus layout. */
    void*    pool_heads[0x37]; /* +0x114 role, port-layout image     */
    /* --- reconstruction-side objective state (finding 22) -----------------
     * The corpus keeps these in globals (DAT_004ffbc0 / DAT_004ffe30 /
     * DAT_004ffe34); living on the level block keeps loads isolated and
     * testable. */
    uint32_t objective;         /* DAT_004ffbc0 */
    int      objective_pending; /* DAT_004ffe34 (1 = result pending) */
    int      objective_result;  /* DAT_004ffe30 (1 = achieved) */
    char     obj_stream[80];    /* DAT_004ffe38: "Stream Objective_…" text */
} LevelState;

/* Logical view of the terrain descriptor list headed at DAT_004b8a24. The
 * corpus stores the single key at +0x08, pair words at +0x0c/+0x10, and
 * next at +0x24; this typed view avoids imposing that x86 layout on 64-bit
 * builds. */
typedef struct TerrainDescriptorNode {
    uint32_t key_single;
    uint32_t key_pair_first;
    uint32_t key_pair_second;
    struct TerrainDescriptorNode* next;
} TerrainDescriptorNode;

TerrainDescriptorNode* terrain_descriptor_find_exact(
    TerrainDescriptorNode* list, const uint32_t* pair_key,
    const uint32_t* single_key);
int terrain_cell_predicate_check(TerrainDescriptorNode* list,
                                 const uint32_t* pair_key);
/* CellResourceEntryMatches @ 0x0040af40: compare a resource-entry record
 * with a cell record, unless a live task of the entry's kind matches. */
int terrain_cell_resource_entry_matches(const uint32_t* cell_record,
                                        const uint32_t* resource_entry);

/* Objective word bits (finding 22 §2). */
#define LVL_OBJECTIVE_STILL_PENDING 0x1u
#define LVL_OBJECTIVE_ACHIEVED      0x2u
#define LVL_OBJECTIVE_FAILED        0x4u
#define LVL_OBJECTIVE_ANNOUNCED     0x8u
#define LVL_OBJECTIVE_NO_ADVISOR    0x10u
#define LVL_OBJECTIVE_NO_ACH_ADV    0x20u
#define LVL_OBJECTIVE_NO_FAIL_ADV   0x80u
#define LVL_OBJECTIVE_CRYSTALS      0x100u
#define LVL_OBJECTIVE_ORE           0x200u
#define LVL_OBJECTIVE_BLOCK         0x400u
#define LVL_OBJECTIVE_TIMER         0x800u
#define LVL_OBJECTIVE_CONSTRUCTION   0x1000u

/* Raw-word accessors mirroring the corpus's `undefined4 puVar6[0xA1]`. */
static inline uint32_t* level_ptr(LevelState* s) { return s->word; }
static inline const uint32_t* level_cptr(const LevelState* s) { return s->word; }

/* Convenience typed accessors. */
static inline float level_float(const LevelState* s, int off)
{
    const uint8_t* p = (const uint8_t*)s->word;
    float f;
    __builtin_memcpy(&f, p + off, sizeof(f));
    return f;
}
static inline void level_set_float(LevelState* s, int off, float v)
{
    __builtin_memcpy(((uint8_t*)s->word) + off, &v, sizeof(v));
}
static inline int32_t level_int(const LevelState* s, int off)
{
    const uint8_t* p = (const uint8_t*)s->word;
    int32_t v;
    __builtin_memcpy(&v, p + off, sizeof(v));
    return v;
}
static inline void level_set_int(LevelState* s, int off, int32_t v)
{
    __builtin_memcpy(((uint8_t*)s->word) + off, &v, sizeof(v));
}

/* Pointer-valued corpus slots: the low word is stored at `off` (fidelity);
 * the live pointer is kept in the registry (offset cross-checked). */
static inline void level_store_ptr(LevelState* s, int off, const void* p)
{
    uintptr_t u = (uintptr_t)p;
    level_set_int(s, off, (int32_t)(u & 0xffffffffu));
    switch (off) {
    case LEVEL_OFF_TERRAIN:      s->ptr_terrain = (void*)u; break;
    case LEVEL_OFF_LAYERL_RECORDS: s->ptr_layerl = (void*)u; break;
    case LEVEL_OFF_VOXELS:       s->ptr_voxels = (void*)u; break;
    case LEVEL_OFF_NEXTLEVEL:    s->ptr_next = (void*)u; break;
    case LEVEL_OFF_LEVEL_NAME:   s->ptr_name = (void*)u; break;
    default: break;
    }
}
static inline void* level_load_ptr(const LevelState* s, int off)
{
    switch (off) {
    case LEVEL_OFF_TERRAIN:      return s->ptr_terrain;
    case LEVEL_OFF_LAYERL_RECORDS: return s->ptr_layerl;
    case LEVEL_OFF_VOXELS:       return s->ptr_voxels;
    case LEVEL_OFF_NEXTLEVEL:    return s->ptr_next;
    case LEVEL_OFF_LEVEL_NAME:   return s->ptr_name;
    default: return NULL;
    }
}

/* TutorialPathCoordinateIndex (FUN_00456b80): zero the 0x37-slot chain
 * table on the level block, then sweep the voxel grid (x outer from +0x14
 * count, y inner from +0x18) and push every cell whose +0x04 coordinate
 * byte is set onto that coordinate's chain — (x, y, next) nodes through
 * the heap seam, head-first, clearing the byte as consumed. Deviations:
 * the heads and node links ride the widened port-layout image (the corpus
 * strides 4-byte slots), the node keeps a padded 0x10 image, a NULL
 * block/voxel window and an out-of-range coordinate byte ride as guards
 * where the corpus strides, and a zeroing re-run leaks the superseded
 * chains (as does the corpus). */
void tutorial_path_coordinate_index(LevelState* s);       /* FUN_00456b80 */

/* --- level loading ------------------------------------------------------ */
/* `pending` is first consulted (PendingLevelName @ 0x42f0c0): when non-null
 * the previous level is torn down and its block freed, returning its
 * "nextlevel" name; the real load then builds the new one.
 *
 * level_load reads the `<level_name>` group from `cfg` (the Lego.cfg DB),
 * materializes every grid and fills the voxel array. Returns a fresh
 * LevelState on success (caller frees with level_destroy), NULL on failure.
 */
const char* level_pending_flush(LevelState** out_level);
LevelState* level_load(BlockFile* cfg, const char* level_name);
void        level_destroy(LevelState* s);

/* Grid size left in the block after a successful load. */
int level_width(const LevelState* s);
int level_height(const LevelState* s);

/* The parsed `nextlevel` name (level+0xb4), or NULL when the level ends
 * the campaign chain. Owned by the level block — valid until level_destroy. */
const char* level_next_name(const LevelState* s);

/* Voxel record pointer for cell (x,y) (or NULL on OOB). */
uint8_t* level_voxel(const LevelState* s, int x, int y);

/* --- map-file resource slots (finding 07: 0x489410's slot registry) ----- *
 * The corpus keeps an interleaved (ptr, in-use-flag) pair table of 2000
 * map-image buffers; MapOpen2 opens through it (unported wad seam, see
 * docs/coverage/blocked.md). The port models the table as one stride-8
 * record array (pointer at +0, used byte at +4) to match the corpus's
 * MapFileResourceSlot layout at DAT 0x00544a20; the claim rides the
 * corpus's engine-heap allocator pair (0x48dbd0 / 0x48db80) — no
 * zero-fill, no failure check, fault-on-use like the corpus. Reset
 * clears without freeing, exactly as the corpus leak loop does. */

#define LEVEL_MAP_SLOT_COUNT 2000 /* the corpus table length */

/* FUN_004893f0: zero every slot pair. */
void level_map_slots_reset(void);

/* FUN_00489410: claim the first free slot with a zero-filled `size`-byte
 * image; -1 when the table is full. */
int level_map_slot_allocate(size_t size);

/* FUN_00489460: free and clear an in-use slot; no-op for bad/free ids. */
void level_map_slot_release(uint32_t slot);

/* FUN_004894a0: the slot's image pointer, or NULL when free/out of range. */
uint8_t* level_map_slot_get(uint32_t slot);

/* FUN_004515a0: read the image header dims (+0x08/+0x0C). NULL reads as
 * zero (the corpus dereferences blindly). */
void level_map_image_header(const uint8_t* image, uint32_t* width,
                            uint32_t* height);

/* FUN_004515c0: the u16 cell at (y*width+x), zero-extended, reading
 * +0x10. NULL image reads 0 (corpus dereferences blindly). */
uint16_t level_map_image_cell(const uint8_t* image, int x, int width, int y);

/* FUN_004334c0: header dims via the slot. An unresolved slot yields
 * 0/0 (the corpus reads address 0x8/0xC — documented deviation). */
void level_map_dimensions_get(uint32_t slot, uint32_t* width,
                              uint32_t* height);

/* FUN_004319e0: cell via the slot (row y, column x; the width comes
 * from the image header). Unresolved slot reads 0. */
uint16_t level_map_cell_get(uint32_t slot, int x, int y);

/* --- tile semantics (entity-callback queries, finding 10) -------------- *
 * The voxel record fields these read are the ones the layer loaders write
 * (finding 07): the tile *type* byte is record+2 (the erosion/terrain byte;
 * the callback compares it against small constants — 1/7/11 block walking,
 * '\t' = 9 is the idle-registration tile), and record+8 is the flag word. */

#define TILE_FLAG_WALKABLE  0x10u       /* traversable layer bit (+8)   */
#define TILE_FLAG_BLOCKING  0x40u       /* blocks walking (+8)          */
#define TILE_FLAG_OCCUPIED  0x8000000u /* reserved by an entity (+8)    */
#define TILE_TYPE_IDLE      '\t'        /* 9: idle-registration tile    */

/* FUN_0048e060: x87 round-to-nearest float→integer (the control-word
 * round-to-nearest `fistp` trick). World units are cell units 1:1. */
int level_cell_of(float v);

/* EntityPositionToTerrainCell (FUN_004424f0): convert an entity's world
 * x/y position to the current level's grid cell. The port threads the level
 * and output pointers; it uses EntitySlot's position mirrors in place of
 * the unmodeled scene-object transform and leaves outputs unchanged when
 * the level/entity/cell is unavailable. */
void entity_position_to_terrain_cell(const struct EntitySlot* entity,
                                     const LevelState* s,
                                     int* cell_x, int* cell_y);

/* FUN_00432770 simple mode (param_4 == 0, the walkability check the move
 * branch calls with): the cell must sit inside a 2-cell grid margin and
 * have flag 0x10 set, flag 0x40 clear, and a type byte not in {1, 7, 11}.
 * NULL level (no map loaded) reports *not walkable* — callers that run
 * without map state (port unit tests, menus) skip the check instead. The
 * corpus's entity-conditional gates (the +0x334 state object, `+0x3e8`
 * bit 1, gated-mode `+0x3e0` bit 0x400, and FUN_00469cc0's zero-speed
 * gate behind them) are TODO(corpus) and treated as pass here; the speed
 * probe itself is ported (`ai_stats_speed`) and drives the mover step. */
int level_walkable(const LevelState* s, int cx, int cy);

/* FUN_00432950 — EntityMoveDestinationCellIsUsable (finding 284): the
 * destination-cell test the mover route runs before assigning a
 * destination. The entity gate is `+1000` (decimal; the port's flags_3e8
 * mirror) bit 0x8000; then both destination coordinates must be nonzero and
 * inside the 2-cell grid margin; the cell is usable when flag 0x10 is set,
 * flag 0x40 clear, and the type byte is not in {1, 7, 11} — the walkable
 * set again. A NULL entity skips the flag read (corpus crashes there; the
 * port answers unusable). */
struct EntitySlot; /* defined in game.hpp */
struct TerrainSurfaceGrid; /* defined in terrain_grid.hpp */
int entity_move_destination_cell_usable(const struct EntitySlot* e,
                                       const LevelState* s, int cx, int cy);

/* FUN_00432a20 — TerrainCellAllowsPlacement (finding 293): placement
 * eligibility. The gate passes when the caller's record is absent (the
 * config-driven NULL call) or its +1000 low byte carries bit 0x4; then both
 * coordinates nonzero and inside the 2-cell margin; the cell needs flag
 * 0x10 set, flag 0x20 clear, and a type byte not in {8, 10, 11}. */
int terrain_cell_allows_placement(const struct EntitySlot* e,
                                  const LevelState* s, int cx, int cy);

/* TerrainCreate (0x44e0c0) scoped port (finding 07): builds the terrain
 * grid handle from a surface map; the handle is the opaque model of the
 * corpus terrain record (free it with terrain_destroy). */
void* terrain_create(const char* surface_map, float blocksize);
void terrain_destroy(void* terrain);
/* Read the retained heightfield used to build the terrain mesh. Heights are
 * the unsigned cell values from SurfaceMap; invalid cells answer zero. */
int terrain_surface_height_get(const void* terrain, int x, int y);
float terrain_blocksize_get(const void* terrain);
int terrain_cell_world_center_xy_get(const void* terrain, int x, int y,
                                     float position_xy[2]); /* FUN_0044f640 */
int terrain_world_xy_to_cell(const void* terrain, float x, float y,
                             int* cell_x, int* cell_y); /* FUN_0044f700 */

/* TerrainCellSurfaceTypeGet (FUN_0044f540, finding 226): the per-cell
 * surface-type byte (block-table +0x24) of the terrain handle. The
 * writer is the renderer-side appearance apply (blocked), so a fresh
 * terrain answers the zero state; NULL/out of range answer 0 (bounds
 * guard added, corpus indexes blind). */
int terrain_cell_surface_type_get(const void* terrain, int x, int y);
/* TerrainCellChangedFlagSet (FUN_0044efb0): set the cell's physical dirty
 * bit after a terrain mutation. */
void terrain_cell_changed_flag_set(void* terrain, int x, int y);

/* TerrainCellSurfaceTypeSet / Reset / AppearanceApply (FUN_0044f4e0 /
 * FUN_0044f570 / FUN_0044f5c0, 2026-10-24; findings 226/450): the writer
 * pair over the same +0x24 byte. The setter
 * returns the prior value and applies the type's appearance triplet from
 * the shipped table (six real entries in the reference data; larger types
 * read undefined neighbours in the corpus — white substituted,
 * documented deviation). Reset applies the white appearance and clears
 * the byte. The appearance leaf's own effect is the scene mesh color and
 * alpha set — the renderer seam — so mode nonzero changes no port state,
 * while mode zero also clears the byte. All leaves guard the bounds (the
 * corpus indexes blind) and answer 0/inert on NULL or misses. */
int   terrain_cell_surface_type_set(void* terrain, int x, int y, int type);
void  terrain_cell_surface_type_reset(void* terrain, int x, int y);
void  terrain_cell_surface_appearance_apply(void* terrain, int x, int y,
                                            int mode, float r, float g,
                                            float b);

/* FUN_00432e00: set (nonzero) or clear the cell's OCCUPIED flag — the
 * entity "reservation" bit the mover-state sync raises and the arrival
 * clears. Bounds-guarded (the corpus writes OOB for bad coords). */
void level_tile_mark(LevelState* s, int cx, int cy, int set);
void level_tile_mark_current(const int cell_xy[2], int set);

/* FUN_00432e00 read side: does the cell currently carry the OCCUPIED
 * reservation flag? The wave-flood pathfinder consults it for its blocked
 * gate (finding 10: the mark "feeds the pathfinder"). No map, or a cell
 * the handle cannot address, reports occupied (cannot rule a reservation
 * out); callers gate on that as blocked. */
int level_tile_occupied(const LevelState* s, int cx, int cy);

/* Cell (x,y) tile type byte (record+2), or -1 when there is no map. */
int level_tile_type(const LevelState* s, int cx, int cy);

/* --- terrain-cell flag primitives (finding 07: record+8 flag word,
 * record+2 state byte, record+12 secondary flag word) --------------------- *
 * The corpus family computes the voxel record address with NO bounds
 * check (it walks the heap past the grid for bad coords); the port guards
 * every accessor with level_voxel and reports an unaddressable cell as
 * all-zero flags / absent state — a documented deviation. The
 * "InBounds"/coordinate-check variants DO bounds-check in the corpus
 * (unsigned `coord < dim - 1U`, or signed `-1 < coord < dim`); that
 * narrower gate is reproduced exactly, including the unsigned wrap that
 * lets every coordinate through when a grid dimension is 0. */

/* FUN_00432bd0: cell inside the grid-minus-one gate with the traversable
 * bit (0x10) set and the blocked gate (flag bit 3) clear. Bounds-checked
 * the corpus way (as unsigned, against dim-1). */
int level_cell_walkable_flag(const LevelState* s, int cx, int cy);

/* FUN_00432c20: the wave-spawn flag bit. Returns the masked word. */
uint32_t level_cell_flag_wave(const LevelState* s, int cx, int cy);

/* FUN_00432c50: terrain speed class — 0 when flag bit 0x200000 is set,
 * otherwise the low two terrain-type bits + 1. */
int level_cell_speed_class(const LevelState* s, int cx, int cy);

/* FUN_00432e60: the flag word's low byte masked with 0xc0 (the corpus
 * reads it as a byte). Returns the masked byte. */
uint32_t level_cell_mask_c0(const LevelState* s, int cx, int cy);

/* FUN_00432e90: the predug edge-wall bit (flag 0x4000000, finding 07).
 * Returns the masked word. */
uint32_t level_cell_flag_edge_wall(const LevelState* s, int cx, int cy);

/* FUN_00432ec0 / FUN_00432f00: the cell state byte (record+2) equals 1
 * or 6. No cell, no match. */
int level_cell_state_is_1(const LevelState* s, int cx, int cy);
int level_cell_state_is_6(const LevelState* s, int cx, int cy);

/* FUN_00432f40: the two low terrain bits (0x18) clear — cells OUTSIDE
 * the grid-minus-one gate count as clear (as in the corpus). */
int level_cell_mask18_clear(const LevelState* s, int cx, int cy);

/* FUN_00432f90: flag bit 4 inside the grid-minus-one gate (0 outside). */
uint32_t level_cell_flag_4(const LevelState* s, int cx, int cy);

/* FUN_00432fd0 / FUN_00433000 / FUN_00433030: single-bit flag tests
 * (0x2000, 0x80, 0x8000). Return the masked word. */
uint32_t level_cell_flag_2000(const LevelState* s, int cx, int cy);
uint32_t level_cell_flag_80(const LevelState* s, int cx, int cy);
uint32_t level_cell_flag_8000(const LevelState* s, int cx, int cy);

/* FUN_00433060: secondary flag word (record+12) bit 0. */
uint32_t level_cell_secondary_bit0(const LevelState* s, int cx, int cy);

/* FUN_00433090: coordinate inside [0,w)×[0,h) and the OCCUPIED-class bit
 * 0x20000000 set (the corpus's signed bounds read as plain in-bounds). */
int level_cell_occupied_at(const LevelState* s, int cx, int cy);
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);

/* FUN_004330e0: the terrain/wave pair mask 0x180000 — nonzero = in. */
int level_cell_mask_180000(const LevelState* s, int cx, int cy);

/* FUN_00433120: raise (set nonzero) or clear flag bit 0x10000000.
 * Bounds-guarded (the corpus writes OOB for bad coords). */
void level_cell_flag_10000000_set(LevelState* s, int cx, int cy, int set);

/* Raise (set nonzero) or clear flag bit 0x800000 — the cell-command
 * issuer's terrain half marks the cell through this leaf (finding 451);
 * no separate corpus leaf owns the write. Bounds-guarded. */
void level_cell_flag_800000_set(LevelState* s, int cx, int cy, int set);

/* --- second terrain-cell leaf cluster ----------------------------------- *
 * Same record layout and same documented bounds deviation as the block
 * above: corpus variants with no bounds check read/write via
 * level_voxel and report zero / no-op off-map; the "bounded" variants
 * reproduce the corpus's unsigned dim-1 gate. */

/* FUN_00432170: secondary flag word (record+12) bit 0x100. */
uint32_t level_cell_secondary_flag_100(const LevelState* s, int cx, int cy);

/* FUN_004322d0: the pathfinder "visited" mark — secondary bit 1. */
uint32_t level_cell_visited_flag(const LevelState* s, int cx, int cy);

/* FUN_004323c0 / FUN_004325d0: clear / raise secondary bit 0 (the
 * pair the "SetAndTrack" corpus variants wrap; the tracking queue is
 * a separate unported seam). Bounds-guarded (corpus writes OOB). */
void level_cell_secondary_bit0_clear(LevelState* s, int cx, int cy);
void level_cell_secondary_bit0_set(LevelState* s, int cx, int cy);

/* FUN_004324d0 / FUN_00432520: secondary bit 3 (the flag-8 pair). */
void level_cell_secondary_bit8_set(LevelState* s, int cx, int cy, int set);
uint32_t level_cell_secondary_flag_8(const LevelState* s, int cx, int cy);

/* FUN_00432aa0: mode 0 reads the flag word's 0x400 mask; mode nonzero
 * answers whether EITHER the 0x400 flag or the secondary byte's bit 4
 * marks the cell. */
uint32_t level_cell_occupied_or_flag(const LevelState* s, int cx, int cy,
                                     int mode);

/* CellFourNeighborWalkCheck (FUN_0041f740): when the center lacks the
 * 0xc0 marker, all four cardinal neighbors must lack occupied/secondary
 * bit 0x400; when marked, at most one neighbor may carry that condition.
 * The port takes the level explicitly and bounds-guards through the cell
 * accessors (the corpus uses global state and unchecked cell addresses). */
int cell_four_neighbor_walk_check(const LevelState* s, int cx, int cy);

/* FUN_00432b00 / FUN_00432b50 / FUN_00432d90: bounded single-bit reads
 * (flags 0x800, 0x8, 0x20) — 0 outside the dim-1 gate. */
uint32_t level_cell_flag_800_bounded(const LevelState* s, int cx, int cy);
uint32_t level_cell_flag_8_bounded(const LevelState* s, int cx, int cy);
uint32_t level_cell_flag_20_bounded(const LevelState* s, int cx, int cy);

/* FUN_00432b90: the state byte (record+2, signed char) is 8, 10 or 11. */
int level_cell_type_is_8_or_10_or_11(const LevelState* s, int cx, int cy);

/* FUN_00432dd0: the entity-reservation bit (flag 0x8000000) — the read
 * side of the level_tile_mark flag. Returns the masked word. */
uint32_t level_cell_flag_reserved(const LevelState* s, int cx, int cy);

/* FUN_00432710: raise (set nonzero) or clear flag bit 0x80000. The
 * corpus twin that raises 0x40000000 additionally refreshes the cell
 * queues; this one is a pure leaf. Bounds-guarded. */
void level_cell_flag_80000_set(LevelState* s, int cx, int cy, int set);

/* FUN_004332c0: record byte +3 as an unsigned value. */
int level_cell_record_byte3(const LevelState* s, int cx, int cy);

/* Explicit-LevelState work-bit helper used by map/load callers. */
void level_cell_work_flag_set(LevelState* s, int cx, int cy);
/* TerrainCellWorkFlagSet @ 0x004332f0: current-level target ABI. */
void terrain_cell_work_flag_set_current(uint32_t cx, uint32_t cy);

/* Explicit-LevelState worker used by erosion and the current-level mapped
 * target below. It applies the descriptor veto, cell flag changes, available
 * entity/callback/FLIC services, and four queued coordinate events. */
void terrain_cell_work_flag_clear(LevelState* s, const int coord[2],
                                  uint32_t selector);
/* TerrainCellWorkFlagClear @ 0x00430320: current-level target ABI. */
void terrain_cell_work_flag_clear_current(const int coord[2],
                                          uint32_t selector);

/* FUN_004502c0: unsigned membership of a coordinate pair against the
 * grid dimensions (the corpus reads the pair from a struct pointer;
 * LevelState's width/height fields stand in — negative coords wrap as
 * the corpus's unsigned comparisons do). */
int level_coord_pair_within_bounds(const LevelState* s, int cx, int cy);

/* FUN_004329d0: a valid coordinate whose cell carries the blocked bit
 * (flag 8). The read is skipped off-grid, exactly as the corpus's
 * short-circuit leaves it. */
int level_cell_flag8_check(const LevelState* s, int cx, int cy);

/* FUN_00433330: the state byte (record+2, unsigned) through an out
 * param, reported as 1 — but only inside the dim-1 gate; the out param
 * keeps its old value outside (as in the corpus). No-map cells never
 * satisfy the gate. */
int level_cell_type_get_at(const LevelState* s, uint32_t cx, uint32_t cy,
                           uint32_t* out_type);

/* FUN_0042c5b0: classify the cell's +5 byte: zero reports 0 with all
 * four outs zeroed; otherwise the byte indexes the {1, 3, 5, 10, 25}
 * table and the low index bits pick which single out receives the
 * value. The corpus routes by (v-1)%4 as 0 -> out_a, 1 -> out_c,
 * 2 -> out_b, 3 -> out_d — outs b/c are SWAPPED relative to the value
 * order while the table is indexed by (v-1)/4. Lookups beyond
 * the five-entry table (bytes above 21) read stack garbage in the
 * corpus; the port reports 0 (documented deviation). Off-map cells
 * report 0 with all outs zeroed. */
int level_cell_byte_state_classify(const LevelState* s, int cx, int cy,
                                   uint32_t* out_a, uint32_t* out_b,
                                   uint32_t* out_c, uint32_t* out_d);

/* --- active-engine state accessors on the level block (findings
 * 295/324/392/475): the corpus reads these fields off DAT_005560d4 —
 * the level block the LevelState models. NULL level answers 0 (the
 * corpus dereferences its global directly; UpgradeResourceCheck's own
 * zero-when-no-level rule generalized, documented deviation). */

/* FUN_00429850: the dword at +0xb0. */
uint32_t active_engine_field_b0_get(const LevelState* s);

/* FUN_00429860: the float at +0x210 + 4*index. */
float engine_float_field210_get_by_index(const LevelState* s, int index);

/* FUN_00429880: the float at +0x260. */
float engine_float_field260_get(const LevelState* s);

/* AirSupplyAmountUpdate (FUN_00424560, finding 317): the air-supply
 * accumulator. The entity's stats float +0xDC is read before the gate:
 * a kind-4 entity skips the update unless its +0x3e8 bit 0x20000000 is
 * set. The update folds the delta through 0.04, the configured rate at
 * block +0x200, and that stats float onto the accumulator at +0x1fc
 * (extended precision in the corpus; the port accumulates in double),
 * stores it clamped high to 100.0, and at zero-or-below pins the field
 * to +0.0 and announces the failed objective (the ported announcer,
 * who == 2) unless the system-state word's bit 0x08000000 suppresses it.
 * Deviation: the corpus strides the entity and its globals blindly. */
void air_supply_amount_update(LevelState* s, const struct EntitySlot* e,
                              float delta);     /* FUN_00424560 */

/* FUN_0042b500: the dword at +0x84 (field meaning unresolved per
 * finding 324 — this is the faithful read). */
uint32_t global_state_field84_get(const LevelState* s);

/* FUN_0041f900: level field +0xa0 when selector is 0, +0xac otherwise,
 * 0 when no level object is active (finding 475's resource check). */
uint32_t upgrade_resource_check(const LevelState* s, int selector);

/* AlwaysAcceptPlaceCommandCell (FUN_00432550, finding 455): the
 * cell-eligibility callback command kind 0x16 installs — answers accept
 * unconditionally; any restriction lives in the callers' command tests.
 * Corpus arguments are ignored; definition stays void. */
int always_accept_place_command_cell(void);

/* --- tracked/refreshing cell-flag set family ----------------------------
 * The target entry points read the current level from GameContext and keep
 * the corpus pointer-pair ABI. LevelState-taking forms remain portable
 * adapters. The two coordinate queues are bounded sidecars; target refresh
 * calls use the available per-cell cell_init_pass implementation. */

/* TerrainCellFlag100SetAndTrack (FUN_00432100): target ABI takes an x/y
 * pair; the explicit-LevelState form is a portable caller adapter. */
void terrain_cell_flag100_set_and_track(LevelState* s, int cx, int cy);
void terrain_cell_flag100_set_and_track_current(const int cell_xy[2]);

/* TerrainCellVisitedFlagSetAndTrack (FUN_00432270): raise the visited
 * mark and queue the pair in the visited list. */
void terrain_cell_visited_flag_set_and_track(LevelState* s, int cx, int cy);

/* TerrainCellVisitedFlagsClearAll (FUN_00432300): walk the visited
 * queue clearing each listed cell's mark, then reset the counter. */
void terrain_cell_visited_flags_clear_all(LevelState* s);

/* TerrainCellPrimaryFlagsMaskClearAndRefresh (FUN_00432360): target ABI
 * takes an x/y pair; the explicit-LevelState form is a caller adapter. */
void terrain_cell_primary_flags_mask_clear_and_refresh(LevelState* s,
                                                       int cx, int cy);
void terrain_cell_primary_flags_mask_clear_and_refresh_current(
    const int cell_xy[2]);

/* TerrainCellFlagsSetAndRefresh (FUN_004323f0): target ABI takes an x/y
 * pair; the explicit-LevelState form is a caller adapter. */
void terrain_cell_flags_set_and_refresh(LevelState* s, int cx, int cy);
void terrain_cell_flags_set_and_refresh_current(const int cell_xy[2]);

/* TerrainCellFlagsSetMask100400 (FUN_00432450): raise primary 0x100400,
 * refresh seam. */
void terrain_cell_flags_set_mask100400(LevelState* s, int x, int y);

/* TerrainCellFlagsSetMask20108000 (FUN_00432490): raise primary
 * 0x20108000, refresh seam. */
void terrain_cell_flags_set_mask20108000(LevelState* s, int x, int y);

/* TerrainCellFlag40000000ClearOrSet (FUN_00432560): target ABI takes an
 * x/y pair and selector; the explicit-LevelState form is a caller adapter. */
void terrain_cell_flag40000000_clear_or_set(LevelState* s, int cx, int cy,
                                            int set);
void terrain_cell_flag40000000_clear_or_set_current(const int cell_xy[2],
                                                     int set);

/* CellNeighborDirectionCode (FUN_004508a0): the 3x3 neighbor direction
 * code — 0..7 for the eight adjacent cells (0 = -y, 1 = +x, 2 = +y,
 * 3 = -x, corners continuing 4..7 clockwise from +x-up), and -1 when
 * (x1,y1) and (x2,y2) are the same cell or not adjacent. Pure, no
 * state. */
int cell_neighbor_direction_code(int x1, int y1, int x2, int y2);

/* --- neighbor-refresh / circle / edge-flag cluster (2026-10-24) ----------
 * The refresh-side companions of the cell-flag families. Deviations (as
 * elsewhere): the corpus indexes its cell table blindly; the port uses
 * level_voxel and skips off-map cells; the per-cell CellInitPass
 * refresh companion stays a TODO(corpus) seam on the load-path form. */

/* TerrainNeighborFlagRefresh (FUN_00431450) takes a level, pointer to an
 * x/y pair, and mode. Its scene action/frame calls need the renderer-owned
 * record handle; the port preserves the record's observable +0x0c bits. */
void terrain_neighbor_flag_refresh(LevelState* s, const int cell_xy[2],
                                   int mode);
static inline void terrain_neighbor_flag_refresh_xy(LevelState* s, int cx,
                                                     int cy, int mode)
{
    const int cell_xy[2] = { cx, cy };
    terrain_neighbor_flag_refresh(s, cell_xy, mode);
}
typedef int (*TerrainNeighborPresenceHook)(uint32_t x, uint32_t y,
                                           int mode, void* context);
typedef void (*TerrainNeighborRenderUpdateHook)(uint32_t x, uint32_t y,
                                                void* context);
void terrain_neighbor_flag_hooks_install(
    TerrainNeighborPresenceHook presence,
    TerrainNeighborRenderUpdateHook render_update, void* context);
void terrain_neighbor_flag_set(LevelState* s, int manager,
                               uint32_t x, uint32_t y); /* FUN_00430470 */
typedef void (*TerrainNeighborActionHook)(const int cell_xy[2],
                                          void* context);
void terrain_neighbor_refresh_hooks_install(
    TerrainNeighborActionHook build_action,
    TerrainNeighborActionHook clear_action, void* context);
void terrain_neighbor_refresh(LevelState* s, const int cell_xy[2]);
typedef void (*TerrainCellVariantCompleteHook)(void* context);
void terrain_cell_variant_complete_hook_install(
    TerrainCellVariantCompleteHook hook, void* context);
int terrain_cell_variant_flag_propagate(LevelState* s,
                                        const int cell_xy[2]);
int terrain_cell_variant_flag_propagate_current(const int cell_xy[2]);
void map_cell_tracking_flags_clear_and_refresh(LevelState* s,
                                               const int cell_xy[2]);
typedef void (*TerrainCellStateProjectionHook)(const int cell_xy[2],
                                               const float position[3],
                                               void* context);
typedef void (*TerrainCellLinkedEntryRemoveHook)(const int cell_xy[2],
                                                 void* context);
void terrain_cell_linked_entry_remove_hook_install(
    TerrainCellLinkedEntryRemoveHook hook, void* context);
void terrain_cell_state_projection_hook_install(
    TerrainCellStateProjectionHook hook, void* context);
int terrain_cell_state_update(LevelState* s, const int cell_xy[2]);
int terrain_cell_state_update_current(const int cell_xy[2]);
typedef int (*DockDefinitionCellGetHook)(int cell_xy[2], void* context);
void dock_definition_cell_get_hook_install(DockDefinitionCellGetHook hook,
                                          void* context);
int dock_definition_matches_cell(const int cell_xy[2]); /* FUN_00438f60 */
typedef struct GridCellEligibilityHooks {
    void* context;
    uint32_t (*cell_flags_get)(int x, int y, void* context);
    int (*surface_entity_eligible)(int x, int y, void* context);
} GridCellEligibilityHooks;
void grid_cell_eligibility_hooks_set(const GridCellEligibilityHooks* hooks);
int grid_cell_entity_eligibility_check(int x, int y); /* FUN_00433e30 */
typedef struct SpatialObjectQueryRequest {
    float center[2];
    float radius;
    uint32_t mask;
    uint32_t filters[4];
    int center_is_cell;
} SpatialObjectQueryRequest;
typedef void* (*SpatialObjectQueryHook)(
    void* context, const SpatialObjectQueryRequest* request);
void spatial_object_query_hook_install(SpatialObjectQueryHook hook,
                                      void* context);
void* spatial_object_query_execute(const SpatialObjectQueryRequest* request);
typedef void (*TerrainCellRecordCountIncrementHook)(int a, int b, int c,
                                                    void* context);
void terrain_cell_record_count_increment_hook_install(
    TerrainCellRecordCountIncrementHook hook, void* context);
void terrain_cell_record_count_increment(int a, int b, int c);
void terrain_cell_record_counts_prune(void* entity); /* 0x00403910 */
void task_record_create_and_scan_grid(void* entity); /* 0x004039a0 */
typedef struct TerrainCellSurfaceUpdateHooks {
    void* context;
    void* (*resource_get)(void* context, unsigned int channel,
                          unsigned int variant);
    int (*geometry_get)(void* context, int x, int y, float coordinates[12]);
    void (*dispatch)(void* context, void* resource,
                     const float coordinates[12], uint32_t packed_surface,
                     uint32_t cell_material, void* param_a, void* param_b);
} TerrainCellSurfaceUpdateHooks;
int terrain_cell_surface_update_dispatch(const uint8_t* cell, int x, int y,
                                         void* param_a, void* param_b,
                                         const TerrainCellSurfaceUpdateHooks* hooks);
typedef struct TerrainMutationCell {
    uint32_t flags;
    uint8_t material;
} TerrainMutationCell;
typedef struct TerrainCellMutationGrid {
    uint32_t width;
    uint32_t height;
    float impact_a;
    float impact_b;
    const TerrainMutationCell* cells;
} TerrainCellMutationGrid;
typedef struct TerrainCellMutationHooks {
    void* context;
    int (*corner_heights_get)(void* context, uint32_t x, uint32_t y,
                              float heights[4]);
    void (*surface_apply)(void* context, uint32_t x, uint32_t y,
                          const float heights[4]);
} TerrainCellMutationHooks;
int terrain_cell_mutation_progress(const TerrainCellMutationGrid* grid,
                                   uint32_t x, uint32_t y,
                                   const TerrainCellMutationHooks* hooks); /* 0x00433380 */
typedef struct TerrainStampImpactHooks {
    void* context;
    uint32_t (*cell_flags_get)(void* context, int x, int y);
    int (*impact_cell_resolve)(void* context, uint32_t x, uint32_t y,
                               int mode, int* cell_x, int* cell_y);
    float (*height_get)(void* context, int x, int y, float offset_x,
                        float offset_y);
    void (*impact_event)(void* context, int cell_x, int cell_y,
                         int mode, int kind, float base_height);
    int (*effect_create)(void* context, int multiple_neighbors,
                         uint32_t x, uint32_t y, int cell_x, int cell_y,
                         float height);
    void (*cell_flags_or)(void* context, uint32_t x, uint32_t y,
                          uint32_t flags);
} TerrainStampImpactHooks;
int terrain_stamp_impact_handle(uint32_t x, uint32_t y, int direction,
                                int mode,
                                const TerrainStampImpactHooks* hooks); /* 0x00431780 */
typedef struct SurfaceTypeDescriptionEntry {
    const char* config_key;
    uint32_t parsed_value;
    uint32_t flags;
} SurfaceTypeDescriptionEntry;
typedef struct SurfaceTypeDescriptionHooks {
    void* context;
    const char* (*value_get)(void* context, const char* key);
    int (*parse)(void* context, const char* value, uint32_t* parsed_value,
                 uint32_t* flags);
} SurfaceTypeDescriptionHooks;
void surface_type_descriptions_load(SurfaceTypeDescriptionEntry entries[18],
                                    const SurfaceTypeDescriptionHooks* hooks); /* 0x004350a0 */
typedef struct TileModelEntityRegistration {
    void* models[2];
    uint32_t record_words[2][100];
    int32_t scale_integer;
    uint32_t registration_flags;
    uint32_t reserved[3];
    int32_t parameter;
} TileModelEntityRegistration;
typedef struct TileModelEntityRegistrationHooks {
    void* context;
    void* (*model_load)(void* context, void* manager, int model_index,
                        const char* entry, int no_texture);
    void (*model_scale)(void* context, void* model, float x, float y, float z);
    void (*model_mode_set)(void* context, void* model, int mode, int enabled);
    int (*mesh_uv_get)(void* context, void* model, float** uv_records);
    void (*mesh_uv_set)(void* context, void* model, int first, int count,
                        float* uv_records);
    void (*uv_records_release)(void* context, float* uv_records);
    void (*model_release)(void* context, void* model);
} TileModelEntityRegistrationHooks;
TileModelEntityRegistration* tile_model_entity_register(
    void* manager, const char* first_entry, const char* second_entry,
    float scale_percent, int parameter,
    const TileModelEntityRegistrationHooks* hooks); /* 0x0040b5f0 */
void tile_model_entity_registration_release(
    TileModelEntityRegistration* registration,
    const TileModelEntityRegistrationHooks* hooks);
typedef struct TerrainProjectedCellRefreshState {
    uint32_t map_width;
    uint32_t map_height;
    int center_x;
    int center_y;
    int radius;
    uint32_t deferred_limit;
    float distance_limit;
    float facing_limit;
    uint32_t overlay_enabled;
} TerrainProjectedCellRefreshState;
typedef struct TerrainProjectedCellRefreshHooks {
    void* context;
    uint32_t (*cell_flags_get)(void* context, int x, int y);
    void (*cell_flags_set)(void* context, int x, int y, uint32_t flags);
    int (*candidate_measure)(void* context, int x, int y, float* distance,
                             float* facing, float* metric);
    void (*cell_refresh)(void* context, int x, int y);
    int (*cell_overlay_update)(void* context, int x, int y);
    void (*cell_sound)(void* context, int x, int y);
    void (*frame_tick)(void* context);
} TerrainProjectedCellRefreshHooks;
uint32_t terrain_projected_cell_state_refresh(
    const TerrainProjectedCellRefreshState* state,
    const TerrainProjectedCellRefreshHooks* hooks); /* 0x00433e80 */

/* TerrainCellFlag20ClearAndRefresh (FUN_004310f0): neighbor refresh
 * with mode 0, then clear primary bit 0x20, then the refresh seam. */
void terrain_cell_flag20_clear_and_refresh(LevelState* s, int cx, int cy);

/* TerrainCellEdgeFlagSetAndUpdate (FUN_0042f2e0): raise or clear the
 * primary 0x200 edge flag on the corpus's set selector (the manager
 * travels as the corpus's first argument), then the refresh seam. */
void terrain_cell_edge_flag_set_and_update(LevelState* s, int cx, int cy,
                                           int set);
void* terrain_activity_build_record_get(LevelState* level, int x, int y);
void terrain_activity_build_record_release(void* record); /* 0x004314c0 */
void terrain_activity_build_records_release_all(LevelState* level); /* 0x00431530 */
typedef void (*TerrainEdgeMidpointUpdateFn)(LevelState* level, float delta,
                                            void* user);
void terrain_edge_midpoint_update_hook_set(TerrainEdgeMidpointUpdateFn hook,
                                           void* user);
void terrain_edge_flag_midpoint_update(LevelState* level, float delta);

/* TerrainCellWorkFlagCircleSet (FUN_00435010): raise the work bit (via
 * the ported in-gate setter) on every cell of the filled circle of the
 * given radius about (center_x, center_y); each candidate answers the
 * corpus's signed -1 < c < dim bounds before the setter's own gate.
 * Radius arithmetic is the corpus's wrapping int math (faithful). */
void terrain_cell_work_flag_circle_set(LevelState* s, int center_x,
                                       int center_y, int radius);
void terrain_cell_work_flag_circle_set_current(const int center_xy[2],
                                               int radius);

/* TerrainNeighborWorkFlagScan (FUN_00431580): breadth-first sweep over the
 * stride-0x48 cell records from the seed coordinate: each queued cell's
 * 3×3 neighbours (row = y±1 outer, col = x±1 inner, row*w + col indexing)
 * that carry the work flag bit 0x20000 get the mutation seam and the bit
 * cleared, and are queued into the alternate page; rounds alternate pages
 * until one arrives empty. Deviation: the corpus indexes out-of-bounds
 * cells blindly and never bounds-checks the enqueue. Corpus quirk kept:
 * the enqueue stores col+1 as x (the entry's col-bound float) with only
 * the row faithful, and its self-skip test never lands, so every hit cell
 * including the seed is requeued. See the level.cpp note for the full
 * corpus-shape audit. */
void terrain_neighbor_work_flag_scan(LevelState* s, unsigned int seed_x,
                                     unsigned int seed_y);
void terrain_neighbor_work_flag_scan_current(unsigned int seed_x,
                                             unsigned int seed_y);

/* TerrainCellQueuedWorkTransition (FUN_00431140): when the cell's flag word
 * (record +0x08) has bit 0x10 raised and bits 0xe0 clear, raise the work
 * bit 0x20 and run the queued-work dispatch chain — the refresh dispatch
 * (port: terrain_cell_refresh_dispatch), the coordinate-pair callback
 * dispatch (port seam), the cell's coordinate callback hook (the corpus's
 * spawn-point pass), and the message bus with the 0x1c cell-work message.
 * Deviation: the corpus strides its global level word blindly; the port
 * guards the NULL level/ctx and threads ctx. */
void terrain_cell_queued_work_transition(const struct GameContext* ctx,
                                         int x, int y);

/* RandomVariantCellScatter (FUN_00435230): scatter variant placements over
 * the (2*radius)² box centred on the coordinate pair: two 15-bit draws per
 * round pick the cell and its dedup key (draw_y + draw_x*10 over a
 * zeroed 100-slot table), each unkeyed cell asks the grid-spawn route with
 * mode 1, and mode nonzero leaves early on a landed spawn. The round limit
 * is the box area, quartered when mode is 0. Deviations: the corpus keys
 * the table out of bounds once the box side passes 10 (the port guards the
 * key); the port's quote-only grid-spawn route never lands, so the early
 * exit and the spawn itself stay unobservable. Radius ≤ 0 would be a
 * zero-modulo crash in the corpus; the port answers the no-op. */
void random_variant_cell_scatter(int center_x, int center_y, int radius,
                                 int mode);

/* terrain_cell_dirty_flag: the corpus's byte +0x19 bit 0x10 read shape
 * on the physical block (the same byte terrain_cell_dirty_queue_
 * push_and_clear raises). Exposed as the read side of the dirty-queue
 * contract; the corpus's scanning consumers stay unported. */
int terrain_cell_dirty_flag(const void* terrain, int x, int y);

/* --- cell timestamp lock table and terrain dirty queue (2026-10-24) ------
 * The timestamp table mirrors the corpus globals; the terrain queue is
 * module-owned with the bounds documented below. */

/* CellCoordinateTimestampRemember (FUN_0040ee00): store the coordinate
 * pair and the configured lock countdown into the first free record of the
 * 1000-entry table; a full table stores nothing. ErosionTimingStateInit
 * seeds _DAT_004d78c0 from ErodeLockTime * 25. */
void cell_coordinate_timestamp_remember(const int xy[2]);

/* CellCoordinateTimestampExists (FUN_0040efb0): answer 1 when an active
 * record holds the exact pair, 0 when the scan runs off the table. */
uint32_t cell_coordinate_timestamp_exists(const int xy[2]);

/* ErosionTimingStateInit (FUN_0040e930): scale the configured trigger,
 * erosion, and lock durations, then clear the ordered pending list, 2,000
 * stage slots, and 1,000 timed coordinate records. */
void erosion_timing_state_init(float value_a, float value_b, float value_c);
float erosion_decay_scale_peek(void);          /* port hook */
float erosion_delay_scale_peek(void);          /* port hook */
int   erosion_timestamp_tick_peek(void);       /* port hook */
int   erosion_scratch_peek(int which, int index); /* which 0: 2000-mark
                                                   * region, 1: 1000-ticket
                                                   * region (port hook) */
int lava_erosion_stage_slot_find_free(int* slot_out); /* 0x0040e990 */

/* LavaErosionDelayFromCell (FUN_0040e9c0): the cell's +6 erosion byte
 * counts down from 6 and rides the timing scale times 25; the corpus
 * strides the grid and timing word blindly — the port guards the cell
 * (deviation: no grid/no level answers zero). */
float lava_erosion_delay_from_cell(const LevelState* s, int x, int y);

/* LavaErosionNeighborCheck (FUN_0040f000): when the seed cell's +2 type
 * byte reads 6, sweep the four cardinal neighbours; a neighbour that is
 * busy-free (flag word +0x08 sign-clear with bit 3 set, byte +6 nonzero)
 * and carries no coordinate timestamp answers 1 with its coordinates in
 * the out pair. Deviations: the corpus strides out-of-bounds neighbours
 * and the seed blindly — the port skips missing cells; and the first
 * neighbour's x-offset comes off uninitialized stack in the corpus
 * (stack residue pinned to 0, giving the cardinal ring below). */
int lava_erosion_neighbor_check(const LevelState* s, int seed_x, int seed_y,
                                int out_xy[2]);
/* LavaErosionNeighborEligible (FUN_0040f2b0): test one erosion candidate
 * against the walkable, reservation, material, and edge-wall gates. The
 * global wrapper uses the installed current-level pointer. */
void lava_erosion_level_context_set(LevelState* s);
void lava_erosion_level_context_release(LevelState* s);
int lava_erosion_neighbor_eligible(const LevelState* s, int x, int y);
int lava_erosion_neighbor_eligible_current(const int xy[2]);
void lava_erosion_pending_cell_add(const int xy[2], int add);
/* Ordered EmergeMap registration and its tick-side state. Pending cells are
 * exposed in the same swap-removed order used by the reference table. */
int lava_erosion_stage_register(const int xy[2], uint32_t stage);
unsigned int lava_erosion_pending_cell_count_get(void);
int lava_erosion_pending_cell_get(unsigned int index, int xy_out[2]);
int lava_erosion_stage_get(unsigned int slot, int xy_out[2],
                           float* timer_out, uint8_t* cell_stage_out);
int lava_erosion_pending_cell_tick(float scale);

/* The geometric terrain operation behind erosion depends on renderer-owned
 * surfaces. Keep that edge as a typed callback; the cell/state transition and
 * neighbor walk remain owned by the level module. */
typedef int (*LavaErosionEdgeApplyHook)(int x, int y, unsigned int direction,
                                        int edge, void* context);
typedef void (*LavaErosionCellActionHook)(int x, int y, int value,
                                          void* context);
typedef void (*LavaErosionSoundHook)(int x, int y, void* context);
typedef void (*LavaErosionEventHook)(int x, int y, void* context);
void lava_erosion_hooks_set(LavaErosionEdgeApplyHook edge_apply,
                            LavaErosionCellActionHook work_clear,
                            LavaErosionCellActionHook cell_change,
                            LavaErosionSoundHook sound,
                            LavaErosionEventHook event,
                            void* context);
void lava_erosion_mutation_threshold_set(unsigned int threshold);
void lava_erosion_neighbor_mutate(const int xy[2], unsigned int direction,
                                 int propagate);
void lava_erosion_eight_neighbor_propagate(const int xy[2],
                                           unsigned int direction);
int lava_erosion_cell_eligible_and_mutate(const int xy[2], int hint);

/* CellDirtyQueuePushAndClear (FUN_00450960): the terrain-owned dirty
 * cell queue over the physical block table. push != 0: when the cell's
 * byte +0x19 lacks the 0x10 flag, append (x,y) and raise the flag.
 * push == 0: when the flag is set, scan the queue for (x,y), replace
 * the found entry with the last one, drop the last, and clear the flag
 * (the corpus's not-found and empty-queue paths answer early, leaving
 * the flag set — faithful). The queue lands as a module static (the
 * corpus's context offsets have no ported owner); the port bounds-checks
 * the block access. */
void terrain_cell_dirty_queue_push_and_clear(void* terrain, int x, int y,
                                             int push);

/* CellListMinDistanceScaleApply (FUN_00450a80): the sound min-distance
 * color sweep over the dirty-cell queue. enable != 0 raises the applied
 * bit (2) and paints every queued cell through the scene object's mesh
 * color setter (block = y-entry folded through the scale word + x-entry,
 * the 0x3 triple); enable == 0 clears the bit and repaints the same cells
 * with the 0 triple. An already-applied (enable != 0) or not-yet-applied
 * (enable == 0) call touches nothing (faithful). The corpus strides its
 * cell-list context (the +0x10 level-block pointer, unowned); the port
 * threads the queue static, scene object, and scale word through
 * install/peek hooks. */
void cell_list_min_distance_scale_apply(int enable);
void cell_list_apply_state_install(void* scene_object, int scale);
void* cell_list_object_peek(void);
uint32_t cell_list_apply_flags_peek(void);

/* --- tile-variant grid-record table (2026-10-24) --------------------------
 * The corpus keeps a fixed 300-block record region (DAT block stride
 * 0x31 dwords; block word 0 is a variant-record pointer and words 1..15
 * are per-field flag words, with two variant-value tables aliasing the
 * region 16 and 32 dwords behind each block). Nothing ported feeds the
 * table contents, so the region lands as a zero-initialized module
 * static (the timestamp-table precedent); the variant-pointer branches
 * therefore answer the default records (zero) in port cases. The corpus
 * indexes these tables blindly; every port leaf bounds-checks first and
 * reports the unaddressable cell as zero / no-op (deviation). */

/* GridRecordFlagMaskClear (FUN_0040ab30): sweep the whole region and
 * clear mask 0x10c from each of the first sixteen block words. */
void grid_record_flag_mask_clear(void);

/* TerrainCellRecordGet (FUN_0040af90, findings 377/380/451): resolve a
 * coordinate to its block and read the variant-pair fields for the
 * requested field word; without a variant record (the port case) the
 * default records answer. NULL outputs skip the stores (deviation). */
void terrain_cell_record_get(int x, int y, int field,
                             uint32_t* value_a, uint32_t* value_b);

/* EnsureCellEntriesLoaded (FUN_0040aea0): validate the cell's configured
 * resource entries before marking a requested cell slot loaded. */
int ensure_cell_entries_loaded(uint32_t kind, int row, int slot);

/* CellFlag4IsSet (FUN_0040b250): the block's field-0 word carries the
 * flag bit 0x4 when the block has no variant record (the port case);
 * with one, the requested field word answers. */
uint32_t cell_flag4_is_set(int x, int y, int field);

/* CellFlag100Set / CellFlag100IsSet (FUN_0040b2a0 / FUN_0040b2e0):
 * raise / test the block field's flag bit 0x100. */
void cell_flag100_set(int x, int y, int field);
uint32_t cell_flag100_is_set(int x, int y, int field);

/* GridCellRecordPresentCheck (FUN_0040e460, finding 356): test the first
 * field of the grid cell record in the heap-allocated present table.
 * The table's owner (the grid-initialize leaf) is unported, so it stays
 * NULL in port cases and the check answers 0 (documented deviation;
 * the corpus dereferences its pointer blindly). */
int grid_cell_record_present_check(int x, int y);

/* --- corner normal sampling + surface-type config leaves (2026-10-24) -----
 * TerrainCellNormalSample (FUN_004512d0): read the cell's corner float
 * triple. The corpus's corner selectors answer block floats +0x00 (corner
 * 0), +0x0c (corner 2), or the normalized sum of both (corners 1/3); the
 * block byte +0x19 bit 0x1 parity raises the selector first. The port's
 * physical blocks carry no loader-fed corner floats (the corpus's vertex
 * builders are unported), so the triples read zero: corners 1/3 then
 * normalize the zero vector — inf/NaN answers, faithful to the corpus
 * under the same loader-less conditions. NULL or out-of-range cells write
 * the zero triple and answer 0 (bounds deviation; corner parity stays
 * unset in every port case). */
int terrain_cell_normal_sample(const void* terrain, uint32_t x, uint32_t y,
                               uint32_t corner, float out[3]);

/* TerrainCellSurfaceNormalGet (FUN_00463590): fetch the four vertices of an
 * indexed renderer quad, then combine its two triangle normals. The engine's
 * layer reader is supplied by the renderer integration. */
typedef int (*TerrainQuadVertexReadHook)(void* context, void* surface,
                                        uint32_t cell_index,
                                        float vertices[4][3]);
void terrain_quad_vertex_source_set(void* surface, uint32_t width,
                                    uint32_t height,
                                    TerrainQuadVertexReadHook hook,
                                    void* context);
int terrain_cell_surface_normal_get(uint32_t x, uint32_t y, float out[3]);
typedef void (*TerrainQuadNormalsEmitHook)(void* context, uint32_t cell_index,
                                          const float normals[4][3]);
typedef struct TerrainDirtyQuadNormalPass {
    uint32_t width;
    uint32_t height;
    uint8_t* records; /* 8-byte indexed quad entries; flags at byte 4 */
    uint32_t* dirty;
    void* context;
    TerrainQuadNormalsEmitHook emit_normals;
} TerrainDirtyQuadNormalPass;
void terrain_dirty_quad_normal_emit_pass(TerrainDirtyQuadNormalPass* pass);
typedef struct TerrainCellQuadEmitHooks {
    void* context;
    int (*create_quad)(void* context, uint32_t cell_index,
                       const uint32_t triangle_indices[6]);
    void (*quad_position_apply)(void* context, int quad,
                                uint32_t position_parameter);
    void (*quad_post_create_update)(void* context, int quad, int enabled);
    void (*visibility_set)(void* context, int quad, int visible);
    int (*vertices_emit)(void* context, int quad,
                         const float vertices[4][3]);
} TerrainCellQuadEmitHooks;
typedef struct TerrainCellQuadEmitGrid {
    uint32_t width;
    uint32_t height;
    uint8_t* records; /* indexed entries, 8-byte stride */
    uint32_t* dirty;
    uint32_t position_parameter;
    TerrainCellQuadEmitHooks hooks;
} TerrainCellQuadEmitGrid;
void terrain_cell_quad_geometry_emit(TerrainCellQuadEmitGrid* grid,
                                     uint32_t x, uint32_t y,
                                     const float vertices[4][3]);

/* TerrainSurfaceTypeNameGet (FUN_00435170): configured display name for a
 * surface type, falling back to the missing-config literal when the entry
 * is null. The current config adapter can retain parsed numeric values but
 * does not yet intern and install the display-name token. */
const char* terrain_surface_type_name_get(int index);

/* SurfaceTypeConfiguredValueAtIndex (FUN_00435190): the indexed value of
 * the same config loader's table, with the corpus's blind indexing guarded
 * (deviation). */
uint32_t surface_type_configured_value_at_index(int index);

/* TerrainNeighborCoordinateCheck (FUN_0042f350): classify a cell's
 * cardinal-neighbor state. The corpus probes north, east, south, west in
 * order, setting mask bit i when the neighbor's flag word (+0x08) carries
 * the occupied class 0x20000000 or the work flag 0x400; an out-of-map
 * neighbor's second probe dereferences the corpus's table out of bounds,
 * so the port answers it zero (bounds deviation). The mask then selects
 * a kind nibble and a return code through two lookup tables, chosen by
 * mode 0 versus non-zero. */
uint32_t terrain_neighbor_coordinate_check(const LevelState* s, int x, int y,
                                           uint8_t* kind, int mode);

/* --- grid-record count tables (2026-10-24) --------------------------------
 * GridRecordCountTablesClear / GridRecordCountDifferenceGet /
 * GridRecordCountGet (FUN_00437440 / FUN_00437460 / FUN_004374a0, findings
 * 326/464): two adjacent count tables of 0x12c0 dwords each — 300 cells
 * (15 columns, the grid-record convention) times sixteen slots, indexed
 * (row * 15 + column) * 16 + slot. The corpus's writers (the entity
 * create/teardown/stats-attach paths) are modeled without feeding these
 * tables, so both answer zero in port cases (documented deviation; their
 * runtime count meanings are not established — findings 326). The
 * difference getter returns table 1 or table 1 minus table 0, matching the
 * corpus. The corpus indexes blindly; the port bounds-checks and answers
 * zero outside the tables. `grid_count_table_slot_peek` is a
 * tests/diagnostics view over
 * the statics (the message_active_count precedent), returning -1 outside. */
void grid_record_count_tables_clear(void);
int grid_record_count_difference_get(int row, int column, int slot,
                                     int subtract_first_table);
/* RenderConfigSixFieldCopyOut / CopyInOrClear (FUN_004370b0 / 004370d0):
 * copy the opaque six-dword render snapshot, or clear it when input is NULL. */
void render_config_six_field_copy_out(uint32_t out[6]);
void render_config_six_field_copy_in_or_clear(const uint32_t in[6]);
/* GridRecordCountDifferenceSum (FUN_00437100): sum all 16 slots for rows
 * 0..14 of `type`, optionally skipping rows whose stats record has bit 8
 * set at offset +0x144. The stats table is the guarded entity model. */
int grid_record_count_difference_sum(int type, int skip_flagged_rows);
uint32_t grid_record_count_get(int row, int column, int slot);
uint32_t entity_type_grid_record_count_get(const char* type_name, int slot);
/* Diagnostics table index: 0 is first, 1 is second. */
int32_t grid_count_table_slot_peek(int row, int column, int slot, int table);

/* Test hook staging a count-table slot (the corpus's table writers stay
 * unported; the install-hook convention). Out-of-bounds pokes skip. */
void grid_count_table_slot_poke(int row, int column, int slot, int table,
                                uint32_t value);

/* --- the global-object field band (2026-10-24; findings 295/324/392) --------
 * The corpus's DAT_005560d4 word fields, threaded through the port's
 * LevelState (the corpus reads the DAT global itself; a NULL level answers
 * as the corpus's NULL global does — no effect). The field meanings stay
 * unresolved; these are the faithful arithmetic the corpus runs.
 * GlobalObjectField8CIncrement @ FUN_0041f870
 * GlobalObjectField8CSubtract @ FUN_0041f880
 * GlobalObjectField98Add @ FUN_0041f8a0
 * GlobalObjectField98Clear @ FUN_0041f8c0
 * LevelStateField9CAdd @ FUN_0041f920
 * GlobalObjectField94Increment @ FUN_0041f980
 * GlobalObjectFieldA8Increment @ FUN_0041f990 */
void global_object_field8c_increment(LevelState* s);
void global_object_field8c_subtract(LevelState* s, int amount);
void global_object_field98_add(LevelState* s, int amount);
void global_object_field98_clear(LevelState* s);

/* EntityAvailableResourceSpend (FUN_00438720, findings 251/372): the +0x1f8
 * gate zeroes the amount when raised; then, when the no-op callback probe
 * (FUN_0041f8e0, whose unconsumed return register the port pins to 0 as a
 * documented deviation) is below the amount, the resource hint rides the
 * message bus as id 0xE and the answer is 0; otherwise the amount lands in
 * the global object's +0x98 accumulator and the answer is 1. */
int entity_available_resource_spend(LevelState* level, int amount,
                                    const void* resource_hint);
                                                /* FUN_00438720 */

void level_state_field9c_add(LevelState* s, int amount);
void global_object_field94_increment(LevelState* s);
void global_object_fielda8_increment(LevelState* s);
uint32_t level_field8c_get(const LevelState* s);
uint32_t level_field98_get(const LevelState* s);
uint32_t level_field9c_get(const LevelState* s);
uint32_t level_field94_get(const LevelState* s);
uint32_t level_fielda8_get(const LevelState* s);

#ifdef __cplusplus
}
#endif

/* --- coordinate-callback dispatchers (2026-10-24) ----------------------------
 * TerrainCellRefreshDispatch (FUN_00402ae0, findings 451/362): resolve the
 * current level's terrain record (the corpus's CurrentLevelGet root,
 * DAT_005560d4 -> GameContext.level, terrain handle at level +0x04),
 * dispatch the fixed coordinate callback through EntityPoolCallbackSearch
 * — the object-record pool registry and its code-literal callback are the
 * unported entity-seam, so the dispatch does nothing in the port — then
 * clear the cell's surface type. The corpus threads the pair as a stack
 * pointer the callback may see; the port reads coordinates by value
 * (documented deviation), guards the missing level/terrain (the corpus
 * dereferences blind), and ignores the second argument (callback-only).
 * CoordinateCallbackDispatch / CoordinateCallbackDispatchByMode
 * (FUN_00402b30 / FUN_00402b60, finding 362/465): the pair-wrapping
 * dispatchers — their whole observable body is the unported callback
 * machinery above, so the port records the seam and changes no state. */
struct GameContext;
void terrain_cell_refresh_dispatch(const struct GameContext* ctx, int x,
                                   int y);
void terrain_cell_refresh_dispatch_current(const int cell_xy[2], int mode);
void coordinate_callback_dispatch(const void* coord_pair, int value);
void coordinate_callback_dispatch_current(const void* coord_pair);
typedef int (*CoordinateEntityDispatchHook)(struct EntitySlot* entity,
                                            const void* coordinate_pair,
                                            int extra, void* context);
void coordinate_entity_dispatch_hooks_install(
    CoordinateEntityDispatchHook direct_callback,
    CoordinateEntityDispatchHook mode_zero_callback,
    CoordinateEntityDispatchHook mode_nonzero_callback, void* context);
void coordinate_callback_dispatch_by_mode(const void* coord_pair,
                                          int mode);

/* GlobalOptionLowBitsEnabled (FUN_004589a0, findings 214/250): the corpus's
 * option word gate — any low bit of the objective word (corpus DAT_004ffbc0,
 * modeled as LevelState.objective) set. A NULL level answers 0 (deviation:
 * the corpus tests its global blindly). */
int global_option_low_bits_enabled(const LevelState* s);

/* GlobalFlagGet_004ffe30 (FUN_00458650, findings 21/22): the objective-
 * result word the corpus reads blindly - LevelState's own field through
 * the port's guarded read (NULL answers 0). */
int level_objective_result_get(const struct LevelState* s);

/* The original shared random stream is VC5 LIBC.LIB rand.obj's _rand/_srand
 * pair. The VC5 x86 build calls that runtime directly; other builds keep the
 * local LCG model and its seed/draw hooks. */
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
#define level_random_seed_set(seed) srand(seed)
#define level_rand_u16() rand()
#else
void level_random_seed_set(uint32_t seed);
int level_rand_u16(void);       /* RandomNext @ 0x48e170 */
#endif
int level_random_draw15(void); /* one 15-bit draw from the active RNG */
uint32_t rng_thunk_draw15(void); /* FUN_004798a0: corpus RNG-thunk forward */

/* TerrainObjectTypeSfxAtCellGet (FUN_0044ad80): turns a position record's
 * coordinate pair into the grid cell through the ported two-dword copy, and
 * answers that cell's entry in the terrain-object SFX grid. Deviation: the
 * corpus's aliasing store through the first output (its own record pointer)
 * is calling-convention residue — the port reads the pair. The grid is fed
 * by the unported config-parsing loader, so it lands as zero-init module
 * storage with set/peek hooks; out-of-range cells answer 0 (guarded
 * deviation: the corpus strides blindly). */
int terrain_object_type_sfx_at_cell_get(const void* position_record);
unsigned int level_predug_coordinate_count_get(void);
int level_predug_coordinate_get(unsigned int index, int cell_xy_out[2]);
void level_predug_coordinate_pool_clear(void);
void spawn_coordinate_pool_clear(void); /* FUN_004372a0 */
                                 /* FUN_0044ad80 */
void terrain_sfx_grid_cell_set(int cell, int handle);  /* port hook */
int  terrain_sfx_grid_cell_peek(int cell);             /* port hook */

/* TerrainObjectTypeSfxGridConfigLoad (FUN_0044acc0): clears the grid, walks
 * the config's S_ObjTtSFXs chain under `root_path`, and stores each entry's
 * resolved sound alias at cell (row + type*15), the (type, row) pair coming
 * from the entity name resolver and carried across entries (the corpus's
 * quirk). Deviations in the module notes. */
void terrain_object_sfx_grid_config_load(BlockFile* config,
                                         const char* root_path);
                                                /* FUN_0044acc0 */

/* TerrainChoiceResourceSelect (FUN_0040e700): the record pair's first word
 * selects the active resource word — cases 1..4 through the flat choice
 * window (word = kind-1 + the pair's second word, guarded), cases 6/7
 * through the scalar pair, anything else clearing the selection flag's bit
 * 0. A success raises flag bits 0+1, keeps the record pointer, and releases
 * the panel text-output slot (frontend hook). Install/peek hooks below. */
void terrain_choice_resource_select(const void* record);
                                                /* FUN_0040e700 */
void      terrain_choice_window_install(int word, uint32_t value);
uint32_t  terrain_choice_window_peek(int word);
void      terrain_choice_scalar_install(int scalar, uint32_t value);
uint32_t  terrain_choice_active_peek(void);     /* port hooks */
uint32_t  terrain_choice_flags_peek(void);
uintptr_t terrain_choice_record_peek(void);
/* CellCardinalNeighborApply (FUN_00409cd0, finding 448): runs the terrain
 * cell refresh dispatch over a cell plus its three cardinal offsets (0,-1),
 * (+1,0), (0,+1), (-1,0) — the corpus's stack-pair loop as four calls.
 * Deviation: the corpus threads the cell's coordinate pair through the
 * record window; the port threads the coordinates. */
void cell_cardinal_neighbor_apply(const struct GameContext* ctx, int x,
                                  int y); /* FUN_00409cd0 */

/* TerrainCellSpecialStatePredicate (FUN_0040d3f0): the cell "special"
 * predicate. The occupancy-or-flag check runs in mode 1; a hit with the
 * caller's second gate word zero answers true outright, otherwise the
 * secondary flag word's bit 0x100 must also hit.
 * Deviations: the corpus threads the coordinate pair through the stack and
 * reads the current-level global in its callees (the port threads the
 * state block); the corpus's first argument is calling-convention residue. */
int terrain_cell_special_state_predicate(const LevelState* s, int cx,
                                         int cy, int secondary_gate);
                                                /* FUN_0040d3f0 */

/* TerrainCellHasOccupiedOrFlaggedNeighbor (FUN_0040d240): reject a center
 * with no flag-8 mark, state 6/7, or special/sidecar state; then look for a
 * special/sidecar state at cardinal distance 1 or 2. DAT_004c7df8's
 * per-cell sidecar is supplied by `sidecar_present`; NULL means absent. */
typedef int (*TerrainCellSidecarPresenceQuery)(const LevelState*, int, int);
int terrain_cell_has_occupied_or_flagged_neighbor(
    const LevelState* s, int cx, int cy,
    TerrainCellSidecarPresenceQuery sidecar_present);

/* --- cell-variant match window (2026-10-25) -------------------------------
 * CellVariantEntryMatchCheck (FUN_00451180; findings 254/450): the cell
 * material-variant compatibility probe, modeled as a pure window
 * operation (the corpus's sole caller stays hook-bound, so the window
 * rides its hooks). The grid window's field shape (corpus dword strides,
 * port reads by offset):
 *   + 0x00 width, + 0x04 height, + 0x08 stride (dwords),
 *   + 0x28 block-table base (widened slot; the corpus strides a dword),
 *   block variant byte at base + 0x18 + (y * stride + x) * 0x28
 *     (0xff = no variant),
 *   inline variant tables: entry byte at window + 0x34 + v * 0x80 + i,
 *     count byte at window + 0x4034 + v (so the window must span the
 *     whole table region).
 * Both cells must answer inside the bounds and name different cells;
 * then the first cell's variant list is scanned for the second cell's
 * variant byte. Deviations: the port guards the window and block-table
 * pointers (the corpus strides both blindly); unsigned coordinate
 * arithmetic rides the corpus shape exactly (out-of-range signed inputs
 * wrap past the bounds test, as in the corpus). */
#define CELL_VARIANT_WINDOW_SIZE 0x4128
int cell_variant_entry_match_check(const void* grid, uint32_t x1, uint32_t y1,
                                   uint32_t x2, uint32_t y2);
                                                /* FUN_00451180 */

/* PathRecordAllocateAndLink (FUN_00409350): the corpus's path record
 * window — 0xd4 zeroed words with a running serial counter store, the
 * source coordinate pair, and the previous-record link. Deviations:
 * widened link slot (port pointers) and a guarded allocator word. The
 * peek hooks read the modeled counter/last globals (DAT roles). */
void* path_record_allocate_and_link(const uint32_t* coord_pair);
                                                /* FUN_00409350 */
/* Port-side widened-pointer image for path record +0x14 payload and voxel
 * +0x10 descriptor links. The corpus stores 32-bit pointers at those slots;
 * native builds retain low-word images there and keep live pointers here. */
int path_record_payload_set(void* record, void* payload, uint32_t count);
void* path_record_payload_get(const void* record);
uint32_t path_record_payload_count_get(const void* record);
void path_record_surface_grid_bind(struct TerrainSurfaceGrid* grid);
struct TerrainSurfaceGrid* path_record_surface_grid_get(void);
int level_cell_path_record_set(LevelState* level, int x, int y,
                               void* record);
void* level_cell_path_record_get(const LevelState* level, int x, int y);
int path_record_entity_append(void* record, struct EntitySlot* entity);
uint32_t path_record_entity_count_get(const void* record);
struct EntitySlot* path_record_entity_get(const void* record,
                                          uint32_t index);
void path_record_cell_links_release(LevelState* level, const void* record);
void path_record_release(void* record);
void* tutorial_path_descriptor_create(const uint32_t* coord_pair);
                                                /* FUN_00409300 */
uint32_t path_record_count_peek(void);
void* path_record_last_peek(void);
void path_record_release_all(void); /* FUN_004099d0 */

/* --- the cell route + neighbour pick band (2026-10-25) ----------------------
 * TerrainCellRouteAndOccupancyCheck (FUN_00431da0): may the entity move
 * from (from_x, from_y) to (to_x, to_y)? Answers 1 (move allowed / on
 * site), 2 (blocked-but-cornered), 3 (open-cell promotion) or 0. The
 * cardinal delta walk pins the corpus's slot-0 quirk; the entity's own
 * tile rides the level_cell_of round seam. strict != 0 skips the
 * on-site answer and rejects the +0x40 flag. Cell reads route through
 * level_voxel (the corpus strides the 0x48 window blind; deviations
 * documented in the .cpp).
 * FirstFreeNeighborCellPick (FUN_00431c70): the four cardinal neighbours
 * in counter-rotated order; mode 1 skips neighbours that are already
 * in-bounds, open and unblocked. The counter is own storage (the corpus
 * DAT_004de588 writer is inside the pick itself); the peek hook stands
 * in for the corpus word. */
int terrain_cell_route_and_occupancy_check(const struct EntitySlot* entity,
                                           const LevelState* s,
                                           int from_x, int from_y,
                                           int to_x, int to_y, int strict);
                                                /* FUN_00431da0 */
int first_free_neighbor_cell_pick(const struct EntitySlot* entity,
                                  const LevelState* s, const int* cell_in,
                                  int* cell_out, int mode);
                                                /* FUN_00431c70 */
uint32_t neighbor_pick_rotation_peek(void);     /* port hook */

#endif /* LEGORRR_GAME_LEVEL_HPP */
