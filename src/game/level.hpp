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
 * bit 1, gated-mode `+0x3e0` bit 0x400, and the per-type speed probe
 * FUN_00469cc0) are TODO(corpus) and treated as pass here. */
int level_walkable(const LevelState* s, int cx, int cy);

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

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEVEL_HPP */
