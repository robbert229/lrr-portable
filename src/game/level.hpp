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
#define LEVEL_OFF_TERRAIN_LOADED   0x110 /* 1 once LoadTerrainMap succeeded        */
#define LEVEL_OFF_STARTP           0x1F4 /* +0x7d StartFP flag                    */
#define LEVEL_OFF_NODRAIN          0x1F8 /* +0x7e NoDrain flag                    */
#define LEVEL_OFF_OXYGEN_RATE      0x200 /* +0x80 float OxygenRate                */
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
    /* --- reconstruction-side objective state (finding 22) -----------------
     * The corpus keeps these in globals (DAT_004ffbc0 / DAT_004ffe30 /
     * DAT_004ffe34); living on the level block keeps loads isolated and
     * testable. */
    uint32_t objective;         /* DAT_004ffbc0 */
    int      objective_pending; /* DAT_004ffe34 (1 = result pending) */
    int      objective_result;  /* DAT_004ffe30 (1 = achieved) */
    char     obj_stream[80];    /* DAT_004ffe38: "Stream Objective_…" text */
} LevelState;

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
 * docs/coverage/blocked.md). The port models the table as parallel
 * arrays; the allocator calloc's the image (the corpus allocates through
 * the engine-heap wrapper, TODO(corpus)). Reset clears without freeing,
 * exactly as the corpus leak loop does. */

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

/* TerrainCellSurfaceTypeGet (FUN_0044f540, finding 226): the per-cell
 * surface-type byte (block-table +0x24) of the terrain handle. The
 * writer is the renderer-side appearance apply (blocked), so a fresh
 * terrain answers the zero state; NULL/out of range answer 0 (bounds
 * guard added, corpus indexes blind). */
int terrain_cell_surface_type_get(const void* terrain, int x, int y);

/* FUN_00432e00: set (nonzero) or clear the cell's OCCUPIED flag — the
 * entity "reservation" bit the mover-state sync raises and the arrival
 * clears. Bounds-guarded (the corpus writes OOB for bad coords). */
void level_tile_mark(LevelState* s, int cx, int cy, int set);

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

/* FUN_004330e0: the terrain/wave pair mask 0x180000 — nonzero = in. */
int level_cell_mask_180000(const LevelState* s, int cx, int cy);

/* FUN_00433120: raise (set nonzero) or clear flag bit 0x10000000.
 * Bounds-guarded (the corpus writes OOB for bad coords). */
void level_cell_flag_10000000_set(LevelState* s, int cx, int cy, int set);

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

/* FUN_004332f0: raise the in-gate work bit (flag 0x4) — the bit the
 * update-queue flush tests; OOB cells stay untouched. */
void level_cell_work_flag_set(LevelState* s, int cx, int cy);

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

/* FUN_0042b500: the dword at +0x84 (field meaning unresolved per
 * finding 324 — this is the faithful read). */
uint32_t global_state_field84_get(const LevelState* s);

/* FUN_0041f900: level field +0xa0 when selector is 0, +0xac otherwise,
 * 0 when no level object is active (finding 475's resource check). */
uint32_t upgrade_resource_check(const LevelState* s, int selector);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEVEL_HPP */
