// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/record.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/string.hpp"
#include "gods98/view.hpp"
#include "entity.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"

/* ------------------------------------------------------------------ *
 *  Corpus .rdata constants (identities from finding 07/22 and the
 *  decompile corpus; numeric values are best-effort re-derivations — the
 *  shipped data section is under copyright and not reproduced here).
 * ------------------------------------------------------------------ */

#define LVL_SEC_TO_TICK   25.0f       /* _DAT_0049e2b4: 25 game ticks/second */
#define LVL_OXYGEN_SCALE  1.0f        /* _DAT_0049e324: (unconfirmed scale)  */
#define LVL_RGB_SCALE     0.0039215686f /* _DAT_0049e328: 1/255            */

/* Config key names (labels confirmed in finding 07/22 or the corpus). */
static const char* K_BOULDER_ANIM   = "BoulderAnimation";
static const char* K_FALLIN_MULT    = "FallinMultiplier";
static const char* K_BLOCKSIZE      = "blocksize";
static const char* K_DIGDEPTH       = "digdepth";
static const char* K_USEROOF        = "useroof";
static const char* K_ROOFHEIGHT     = "roofheight";
static const char* K_ROUGHLEVEL     = "roughlevel";
static const char* K_SURFACEMAP     = "SurfaceMap";
static const char* K_PREDUGMAP      = "PredugMap";
static const char* K_TERRAINMAP     = "TerrainMap";
static const char* K_BLOCKPTRMAP    = "BlockPointersMap";
static const char* K_CRYOREMAP      = "CryOreMap";
static const char* K_ERODEMAP       = "ErodeMap";
static const char* K_EMERGEMAP      = "EmergeMap";
static const char* K_AIMAP          = "AIMap";
static const char* K_PATHMAP        = "PathMap";
static const char* K_FALLINMAP      = "FallinMap";
static const char* K_TEXTURESET     = "textureset";
static const char* K_SELBOXH        = "selboxheight";
static const char* K_OLISTFILE      = "olistfile";
static const char* K_SAFECAVERNS    = "SafeCaverns";
static const char* K_STARTFP        = "StartFP";
static const char* K_NODRAIN        = "NoDrain";
static const char* K_OXYGENRATE     = "OxygenRate";
static const char* K_BUILDTOL       = "BuildingTolerance";
static const char* K_BUILDMAXVAR    = "BuildingMaxVariation";
static const char* K_NEXTLEVEL      = "nextlevel";
static const char* K_FULLNAME       = "FullName";
static const char* K_INITCRYSTALS   = "InitialCrystals";
static const char* K_INITORE        = "InitialOre";

/* Cheat-word bits (DAT_00556ed0) set by LevelLoad (finding 07/config): kept
 * accessible for the frame loop but not written by the load itself (the
 * corpus's DAT_00556ed0 is modelled in GameContext). */
#define LVL_OFF_BOULDER_ANIM 0x274   /* +0x9d */
#define LVL_OFF_INIT_CRYS    0x130   /* +0x4c scratch from post-load init */
#define LVL_OFF_INIT_ORE     0x134   /* +0x4d scratch from post-load init */

/* ------------------------------------------------------------------ *
 *  PRNG — corpus thunk_FUN_0048e170 (used for the per-cell seed and the
 *  emergence scatter). MSVCRT-style LCG so results match the era machine.
 * ------------------------------------------------------------------ */

static uint32_t s_rand_state = 1;

void level_random_seed_set(uint32_t seed)
{
    s_rand_state = seed;
}


static uint16_t level_rand_u16(void)
{
    s_rand_state = s_rand_state * 214013u + 2531011u;
    return (uint16_t)((s_rand_state >> 16) & 0x7fffu);
}

int level_random_draw15(void)
{
    return (int)level_rand_u16();
}

static int level_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (ca >= 'a' && ca <= 'z') ca -= 0x20;
        if (cb >= 'a' && cb <= 'z') cb -= 0x20;
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
}

/* ------------------------------------------------------------------ *
 *  `.map` file container (finding 07): raw image, direct-mapped.
 * ------------------------------------------------------------------ */

MapFile* level_map_open(const char* name)
{
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }
    Resource* r = resource_open(name, RESOURCE_AUTO);
    if (r == NULL) {
        return NULL;
    }
    int32_t sz = resource_read(r);
    MapFile* m = NULL;
    if (sz >= 0x10) {
        m = (MapFile*)calloc(1, sizeof(MapFile));
        if (m != NULL) {
            m->data = r->data;
            m->size = sz;
            uint32_t w, h;
            __builtin_memcpy(&w, r->data + 0x08, 4);
            __builtin_memcpy(&h, r->data + 0x0C, 4);
            m->width = (int32_t)w;
            m->height = (int32_t)h;
            r->data = NULL; /* ownership moves to the MapFile */
        }
    }
    resource_free(r);
    return m;
}

void level_map_close(MapFile* m)
{
    if (m != NULL) {
        free(m->data);
        free(m);
    }
}

uint16_t level_map_cell(const MapFile* m, int x, int y)
{
    if (m == NULL || x < 0 || y < 0 || x >= m->width || y >= m->height) {
        return 0;
    }
    uint16_t v;
    __builtin_memcpy(&v, m->data + 0x10 + ((size_t)y * m->width + x) * 2, 2);
    return v;
}

/* Map header helper — corpus FUN_00477440. Returns the bias word the level
 * loaders subtract per layer (0 for the shipped maps, whose cells start at
 * +0x10; see level.hpp header note). */
static int map_header_value(const char* name)
{
    char fields[UTIL_QUERY_MAX_FIELDS][256];
    int status = util_query_fields(name, fields, sizeof(fields[0]));
    if (status == 0) {
        return 0;
    }
    return (int)util_atoi(fields[1]);
}

/* ------------------------------------------------------------------ *
 *  TerrainCreate @ 0x44e0c0 — scoped port (finding 07 §"Voxel record").
 *
 *  Builds the terrain struct (grid dimensions + world scale) and the
 *  parallel physical block table (per-cell stride 0x28: heights, normal
 *  pair, `0x60` sentinel). The corpus-side renderer surfaces (per-vertex
 *  mesh, shader handles) are stubbed — they feed the render lane that the
 *  frame loop's future steps build.
 * ------------------------------------------------------------------ */

#define TERRAIN_BLOCK_STRIDE 0x28

typedef struct TerrainState {
    uint32_t dim[4];      /* [0]=w-1 [1]=h-1 [2]=w [3]=h */
    float    blocksize;
    float    world_off_x; /* -(w-1)*blocksize*0.5 */
    float    world_off_y; /*  (h-1)*blocksize*0.5 */
    uint8_t* blocks;      /* w*h*0x28 physical block table */
    int      width, height;
} TerrainState;

static void terrain_block_fixup(TerrainState* t, int x, int y, int which)
{
    /* Corpus FUN_0044edf0 / FUN_004514f0 — neighbour `+0x1A` contra-`+0x42`
     * wall fix-ups. Only the structural side is retained: mirror the split
     * direction into the two rows' markers. */
    if (t == NULL || t->blocks == NULL || x < 0 || y < 0
        || x >= t->width || y + 1 >= t->height) {
        return;
    }
    uint8_t* a = t->blocks + ((size_t)y * t->width + x) * TERRAIN_BLOCK_STRIDE;
    uint8_t* b = t->blocks + ((size_t)(y + 1) * t->width + x) * TERRAIN_BLOCK_STRIDE;
    if (which == 0) {
        a[0x18] = (uint8_t)((a[0x1A] > b[0x1A]) ? 0x60 : 0x61);
        b[0x18] = 0x62;
    } else {
        a[0x19] = 0x60;
        b[0x19] = 0x60;
    }
}

void* terrain_create(const char* surface_map, float blocksize)
{
    MapFile* m = level_map_open(surface_map);
    if (m == NULL) {
        return NULL;
    }
    TerrainState* t = (TerrainState*)calloc(1, sizeof(TerrainState));
    if (t == NULL) {
        level_map_close(m);
        return NULL;
    }
    t->width = m->width;
    t->height = m->height;
    t->dim[0] = (uint32_t)(m->width - 1);
    t->dim[1] = (uint32_t)(m->height - 1);
    t->dim[2] = (uint32_t)m->width;
    t->dim[3] = (uint32_t)m->height;
    t->blocksize = blocksize;
    t->world_off_x = -(float)(m->width - 1) * blocksize * 0.5f;
    t->world_off_y = (float)(m->height - 1) * blocksize * 0.5f;

    size_t n = (size_t)m->width * m->height;
    t->blocks = (uint8_t*)calloc(n, TERRAIN_BLOCK_STRIDE);
    if (t->blocks == NULL) {
        level_map_close(m);
        free(t);
        return NULL;
    }
    int x, y;
    for (y = 0; y < m->height; ++y) {
        for (x = 0; x < m->width; ++x) {
            uint8_t* rec = t->blocks
                           + ((size_t)y * m->width + x) * TERRAIN_BLOCK_STRIDE;
            rec[0x18] = 0;
            rec[0x19] = 0;
            rec[0x1A] = (uint8_t)level_map_cell(m, x, y);
            /* Normal pair (+0x1C/+0x20). The corpus randomises then
             * normalises with a fixed 0.5 scaling; deterministic here for
             * tests, same structural meaning. */
            float nx = (float)(((uint32_t)x * 2654435761u) % 1000u) / 1000.0f - 0.5f;
            float ny = (float)(((uint32_t)y * 40503u) % 1000u) / 1000.0f - 0.5f;
            float len = nx * nx + ny * ny;
            float inv = 0.5f;
            if (len > 1e-4f) {
                inv = 0.5f / len;
            }
            nx *= inv;
            ny *= inv;
            nx *= 0.5f;
            ny *= 0.5f;
            __builtin_memcpy(rec + 0x1C, &nx, 4);
            __builtin_memcpy(rec + 0x20, &ny, 4);
        }
    }
    /* Edge-continuity fix-ups (corpus FUN_0044edf0 / FUN_004514f0). */
    for (y = 0; y + 1 < m->height; ++y) {
        for (x = 0; x < m->width; ++x) {
            uint8_t* a = t->blocks
                         + ((size_t)y * m->width + x) * TERRAIN_BLOCK_STRIDE;
            int d1 = (int)a[0x1A] - (int)a[0x1A + TERRAIN_BLOCK_STRIDE];
            int d2 = (int)a[0x42] - (int)a[0x1A + TERRAIN_BLOCK_STRIDE];
            if (d1 < 0) d1 = -d1;
            if (d2 < 0) d2 = -d2;
            terrain_block_fixup(t, x, y, d1 < d2 ? 0 : 1);
        }
    }
    level_map_close(m);
    return t;
}

void terrain_destroy(void* handle)
{
    TerrainState* t = (TerrainState*)handle;
    if (t != NULL) {
        free(t->blocks);
        free(t);
    }
}

/* TerrainCellSurfaceTypeGet (FUN_0044f540, finding 226): the per-cell
 * surface-type byte in the physical block table — the port's terrain
 * handle is the TerrainState model, so the corpus's record fields
 * +0x8/+0x28 (stride, table) map to the modeled width/table pair. Bounds
 * guard added — the corpus indexes blind (documented deviation). */
int terrain_cell_surface_type_get(const void* terrain, int x, int y)
{
    const TerrainState* t = (const TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return 0;
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return 0;
    }
    return t->blocks[0x24 + ((size_t)y * (size_t)t->width + (size_t)x) * TERRAIN_BLOCK_STRIDE];
}

/* ------------------------------------------------------------------ *
 *  Layer loaders — the ten grid importers (finding 07).
 * ------------------------------------------------------------------ */

static uint8_t* voxel_at(LevelState* s, int x, int y)
{
    int w = level_int(s, LEVEL_OFF_WIDTH);
    return (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS)
           + ((size_t)y * w + x) * LEVEL_VOXEL_STRIDE;
}

static int dims_ok(const MapFile* m, const LevelState* s)
{
    return m->width == level_int(s, LEVEL_OFF_WIDTH)
           && m->height == level_int(s, LEVEL_OFF_HEIGHT);
}

/* The predug edge-wall cell update — the flag side of the cell update
 * (FUN_00430530) the corpus runs per `v == 1/3` predug cell *before* the
 * 3x3 edge-wall OR (LoadPredugLayer 0x42bd20). Corpus shape, cell-update
 * branch (the type byte is still 0 this early in the pipeline, so the
 * `type == 8/10` branch cannot run):
 *
 *   - bounds gate: x/y == 0 or x >= w-2 / y >= h-2 (unsigned) no-ops;
 *   - already-cave cells (`+0x08` bit 8 set) return untouched;
 *   - the wave-seed marks the northwest 2x2 footprint {cell, x+1,
 *     y+1, x+1/y+1} wave-state bytes (`record+0` = 1, the corpus's
 *     flooded mark of a seeded handle);
 *   - the eight neighbours without the cave flag take the traversable
 *     mark `+0x08 |= 0x10` and lose the `0x400000` open flag
 *     (`& ~0x400000`, the corpus's `& 0xfffbffff`);
 *   - the cell itself raises cave `8`, keeps `+0x40` clear (the corpus
 *     `& ~0x40`), and raises `0x400000` open plus `0x200000` — the
 *     param-4 (load-time) wave flag of the `else` branch.
 *
 * Renderer, wave-handle (`FUN_00431450`, `FUN_0044efb0`), and message
 * calls stay stubbed like the rest of the loading passes. This is the
 * *only* place the shipped load pipeline marks cells traversable
 * (`+0x08` bit `0x10`, the reader of `FUN_00432770` simple mode): the
 * later full-map cell-init never raises it, so without this update a
 * shipped level has no walkable cells at all — found by the T3
 * real-data probe (wave-flood paths must cross real terrain). */
static void predug_wall_cell(LevelState* s, int x, int y)
{
    int w = level_int(s, LEVEL_OFF_WIDTH);
    int h = level_int(s, LEVEL_OFF_HEIGHT);
    int dx, dy;
    if (x == 0 || y == 0 || (unsigned)x >= (unsigned)w - 2u ||
        (unsigned)y >= (unsigned)h - 2u) {
        return;
    }
    uint8_t* rec = voxel_at(s, x, y);
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    if ((flags & 8u) != 0) {
        return; /* the corpus's cave gate: already cave, untouched */
    }

    /* Wave seed over the northwest 2x2 footprint (record+0 = flooded). */
    for (dy = 0; dy <= 1; ++dy) {
        for (dx = 0; dx <= 1; ++dx) {
            uint8_t* f = voxel_at(s, x + dx, y + dy);
            if (f[0x00] == 0) {
                f[0x00] = 1;
            }
        }
    }

    /* The eight neighbours: traversable mark over non-cave cells. */
    for (dy = -1; dy <= 1; ++dy) {
        for (dx = -1; dx <= 1; ++dx) {
            uint8_t* n = voxel_at(s, x + dx, y + dy);
            uint32_t nf;
            __builtin_memcpy(&nf, n + 0x08, sizeof(nf));
            if ((nf & 8u) == 0) {
                nf |= TILE_FLAG_WALKABLE;
                nf &= ~0x400000u;
                __builtin_memcpy(n + 0x08, &nf, sizeof(nf));
            }
        }
    }

    /* The cell itself: cave + open + wave flag, blocking bit kept clear. */
    flags |= 8u | 0x400000u | 0x200000u;
    flags &= ~0x40u;
    __builtin_memcpy(rec + 0x08, &flags, sizeof(flags));
}

/* LoadPredugLayer @ 0x42bd20 */
static int load_predug(LevelState* s, const char* name, int bias)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 0;
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 1; y + 1 < m->height; ++y) {
        for (int x = 1; x + 1 < m->width; ++x) {
            int v = (int)level_map_cell(m, x, y) - bias;
            uint8_t* rec = voxel_at(s, x, y);
            uint32_t f8 = 0, fC = 0;
            if (v == 1 || v == 3) {
                /* Corpus cell update first (sets cave/traversable marks),
                 * then the edge-wall OR. */
                predug_wall_cell(s, x, y);
                /* Edge-wall: the corpus ORs 0x4000000 into the 3×3
                 * neighbourhood's +0x08 words. */
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int qx = x + dx, qy = y + dy;
                        if (qx >= 0 && qy >= 0 && qx < m->width && qy < m->height) {
                            uint32_t* f = (uint32_t*)(voxel_at(s, qx, qy) + 0x08);
                            *f |= 0x4000000u;
                        }
                    }
                }
            } else if (v == 2 || v == 4) {
                f8 |= 0x20000u;
            }
            if (v == 3) {
                fC |= 0x20u; /* wall trigger -> FUN_0044a4b0 (stub) */
            }
            if (v == 4) {
                fC |= 0x200u;
            }
            if (f8) *(uint32_t*)(rec + 0x08) |= f8;
            if (fC) *(uint32_t*)(rec + 0x0C) |= fC;
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadErodeCryoreLayers @ 0x42c480 — reads the TerrainMap (Surf materials),
 * writes the erosion/cryo byte at voxel +0x02. */
static int load_erode_cryore(LevelState* s, const char* name, int bias)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 0;
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            int v = (int)level_map_cell(m, x, y) - bias;
            if (v == 5) {
                v = 4;
            }
            uint8_t* rec = voxel_at(s, x, y);
            rec[0x02] = (uint8_t)v;
            if (v == 6) {
                rec[0x07] = 4; /* crystal seam -> entity spawn (stub) */
            } else if (v == 11) {
                /* rock-monster / cavern spawn point -> FUN_004311d0 (stub) */
            }
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadEmergeLayer @ 0x42bf40 — reads the ErodeMap, writes emerge byte
 * (value >> 1) at voxel +0x06, with the odd-value random scatter. */
static int load_emerge(LevelState* s, const char* name)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 1; /* sparse layer; absence is not fatal */
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            uint16_t c = level_map_cell(m, x, y);
            uint8_t* rec = voxel_at(s, x, y);
            rec[0x06] = (uint8_t)(c >> 1);
            if ((c & 1) != 0) {
                /* odd value: scatter to a random neighbour (FUN_0040ea10). */
                uint8_t d = (uint8_t)(level_rand_u16() & 3u);
                int tx = x, ty = y;
                if (d == 0)      { tx = x > 0 ? x - 1 : x; }
                else if (d == 1) { tx = x + 1 < m->width ? x + 1 : x; }
                else if (d == 2) { ty = y > 0 ? y - 1 : y; }
                else             { ty = y + 1 < m->height ? y + 1 : y; }
                voxel_at(s, tx, ty)[0x06] = rec[0x06];
            }
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadPathAiLayers @ 0x42c6a0 — reads the CryOreMap, writes path/AI byte
 * (value − bias) at voxel +0x05. */
static int load_path_ai(LevelState* s, const char* name, int bias)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 0;
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            int v = (int)(int16_t)level_map_cell(m, x, y) - bias;
            voxel_at(s, x, y)[0x05] = (uint8_t)v;
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadFallinLayer @ 0x42c760 — reads the PathMap, ORs fallin flag bits into
 * voxel +0x08 over an existing cave flag (the corpus's `(flags & 8)` gate). */
static int load_fallin(LevelState* s, const char* name, int bias)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 1; /* sparse; optional */
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            int v = (int)(int16_t)level_map_cell(m, x, y) - bias;
            uint8_t* rec = voxel_at(s, x, y);
            uint32_t flags;
            __builtin_memcpy(&flags, rec + 0x08, 4);
            if ((flags & 8) != 0) {
                if (v == 2) {
                    flags |= 0x20000000u;
                } else if (v == 1) {
                    /* corpus clears the still-cave bits (0x200000/0x400000/
                     * 0x800000 on +0x08) then marks the floor with 3. The
                     * entity teardown that follows is a stub. */
                    flags &= 0xffdfffffu;
                    flags &= 0xdfffffffu;
                    flags &= 0xbfffffffu;
                    flags |= 3u;
                }
                __builtin_memcpy(rec + 0x08, &flags, 4);
            }
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadBlockPointers @ 0x42c9d0 — reads the FallinMap, writes the block
 * pointer count/flag at voxel +0x38/+0x3C and the hash mod at +0x40.
 * (Key→loader roles are the code's mapping, see level.hpp.) */
static int load_block_pointers(LevelState* s, const char* name, int fallin_mult)
{
    if (fallin_mult <= 0) {
        fallin_mult = 1;
    }
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 0;
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            uint32_t v = level_map_cell(m, x, y);
            uint8_t* rec = voxel_at(s, x, y);
            __builtin_memset(rec + 0x3C, 0, 4);
            if (v != 0) {
                __builtin_memcpy(rec + 0x3C, &v, 4);
                uint16_t seed;
                __builtin_memcpy(&seed, rec + 0x20, 2);
                uint32_t hash = seed % (v * (uint32_t)fallin_mult * 0x19u);
                uint32_t zero = 0, one = 1;
                __builtin_memcpy(rec + 0x38, &zero, 4);
                __builtin_memcpy(rec + 0x40, &hash, 4);
                if (v > 4) {
                    uint32_t reduced = v - 4;
                    __builtin_memcpy(rec + 0x3C, &reduced, 4);
                    __builtin_memcpy(rec + 0x38, &one, 4);
                }
            }
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadTerrainMap @ 0x42cc90 — reads the BlockPointersMap, writes surface
 * type byte (value − bias) at voxel +0x04 (mapping note: level.hpp). */
static int load_terrain_map(LevelState* s, const char* name, int bias)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 0;
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            int v = (int)(int16_t)level_map_cell(m, x, y) - bias;
            voxel_at(s, x, y)[0x04] = (uint8_t)v;
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadLayerL @ 0x42c120 — reads the EmergeMap; ORs `0x40`/`0x80` flag bits
 * at voxel +0x0C and appends 0x4C-stride records into level+0x08. */
static int load_layer_l(LevelState* s, const char* name)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 1; /* sparse; optional */
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    uint8_t* records = (uint8_t*)level_load_ptr(s, LEVEL_OFF_LAYERL_RECORDS);
    int      count   = level_int(s, LEVEL_OFF_LAYERL_COUNT);
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            uint16_t c = level_map_cell(m, x, y);
            if (c == 0) {
                continue;
            }
            int slot = ((int)c - 1) % 6;
            int rec  = ((int)c - 1) / 6;
            while (rec >= count) {
                uint8_t* grown = (uint8_t*)realloc(records, (size_t)(count + 1) * 0x4C);
                if (grown == NULL) {
                    level_store_ptr(s, LEVEL_OFF_LAYERL_RECORDS, records);
                    level_set_int(s, LEVEL_OFF_LAYERL_COUNT, count);
                    level_map_close(m);
                    return 0;
                }
                records = grown;
                __builtin_memset(records + (size_t)count * 0x4C, 0, 0x4C);
                ++count;
            }
            uint8_t* recp = records + (size_t)rec * 0x4C;
            uint8_t* vox = voxel_at(s, x, y);
            if (slot == 0) {
                *(uint32_t*)(vox + 0x0C) |= 0x80u;
                __builtin_memcpy(recp + 4, &x, 4);
                __builtin_memcpy(recp + 8, &y, 4);
            } else {
                *(uint32_t*)(vox + 0x0C) |= 0x40u;
                __builtin_memcpy(recp + (size_t)slot * 0xC + 4, &x, 4);
                __builtin_memcpy(recp + (size_t)slot * 0xC + 8, &y, 4);
                uint32_t one = 1;
                __builtin_memcpy(recp + (size_t)(slot + 1) * 0xC, &one, 4);
            }
        }
    }
    level_store_ptr(s, LEVEL_OFF_LAYERL_RECORDS, records);
    level_set_int(s, LEVEL_OFF_LAYERL_COUNT, count);
    level_map_close(m);
    return 1;
}

/* LoadLayerM @ 0x42c060 — reads the AIMap, writes raw byte at voxel +0x34. */
static int load_layer_m(LevelState* s, const char* name)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 1; /* sparse; optional */
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            voxel_at(s, x, y)[0x34] = (uint8_t)level_map_cell(m, x, y);
        }
    }
    level_map_close(m);
    return 1;
}

/* CellInitPass @ 0x42f6f0 — scoped port. The corpus sweeps every cell and
 * derives the cave/block relationship from the neighbourhood. The flag side
 * later systems read is kept; render calls are stubbed. */
static void cell_init_pass(LevelState* s)
{
    int w = level_int(s, LEVEL_OFF_WIDTH);
    int h = level_int(s, LEVEL_OFF_HEIGHT);
    for (int y = 0; y < h - 1; ++y) {
        for (int x = 0; x < w - 1; ++x) {
            uint8_t* rec = voxel_at(s, x, y);
            uint32_t f8, fC;
            __builtin_memcpy(&f8, rec + 0x08, 4);
            __builtin_memcpy(&fC, rec + 0x0C, 4);
            /* Count the 4-neighbour solidity from the erosion byte. */
            static const int dxs[4] = { -1, 0, 1, 0 };
            static const int dys[4] = { 0, 1, 0, -1 };
            int solid = 0;
            for (int i = 0; i < 4; ++i) {
                int qx = x + dxs[i], qy = y + dys[i];
                if (qx >= 0 && qy >= 0 && qx < w && qy < h
                    && voxel_at(s, qx, qy)[0x02] == 1) {
                    ++solid;
                }
            }
            f8 &= ~0x800000u;
            f8 &= ~0x400000u;
            f8 &= ~0x20000000u;
            fC &= 0xfffff0ffu;
            if (solid == 4) {
                f8 &= ~0x8u;
            } else {
                f8 |= 0x8u;
                if (solid == 0) {
                    f8 |= 0x400000u;
                }
            }
            if (rec[0x02] == 6) {
                fC |= 0x100u;
            }
            __builtin_memcpy(rec + 0x08, &f8, 4);
            __builtin_memcpy(rec + 0x0C, &fC, 4);
        }
    }
}

/* ------------------------------------------------------------------ *
 *  MapLoadPipeline @ 0x42b510.
 * ------------------------------------------------------------------ */

static int map_pipeline(LevelState* s, const char** names, const int* biases)
{
    /* names: 0 surface, 1 predug, 2 terrain(Surf), 3 blockptrs, 4 cryore,
     * 5 erode, 6 path, 7 emerge, 8 ai, 9 fallin. (Corpus positional order,
     * see level.hpp header note.) */
    TerrainState* terrain = (TerrainState*)terrain_create(names[0],
                                                          level_float(s, LEVEL_OFF_BLOCKSIZE));
    if (terrain == NULL) {
        return 0;
    }
    level_store_ptr(s, LEVEL_OFF_TERRAIN, terrain);
    level_set_int(s, LEVEL_OFF_WIDTH, terrain->width);
    level_set_int(s, LEVEL_OFF_HEIGHT, terrain->height);

    size_t w = (size_t)terrain->width, h = (size_t)terrain->height;
    uint8_t* vox = (uint8_t*)calloc(w * h, LEVEL_VOXEL_STRIDE);
    if (vox == NULL) {
        terrain_destroy(terrain);
        level_store_ptr(s, LEVEL_OFF_TERRAIN, NULL);
        return 0;
    }
    level_store_ptr(s, LEVEL_OFF_VOXELS, vox);
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            uint16_t seed = level_rand_u16();
            __builtin_memcpy(vox + (y * w + x) * LEVEL_VOXEL_STRIDE + 0x20, &seed, 2);
        }
    }

    /* Layer order is the corpus call order; a header-carrying or mandatory
     * layer failing aborts the pipeline (0) exactly as the corpus does. */
    int ok = load_predug(s, names[1], biases[1]);
    if (!ok) { return 0; }
    ok = load_erode_cryore(s, names[2], biases[2]);
    if (!ok) { return 0; }
    load_path_ai(s, names[4], biases[4]);
    load_emerge(s, names[5]);
    load_fallin(s, names[6], biases[6]);
    load_layer_l(s, names[7]);
    load_layer_m(s, names[8]);
    load_block_pointers(s, names[9], 1);
    ok = load_terrain_map(s, names[3], biases[3]);
    if (ok) {
        level_set_int(s, LEVEL_OFF_TERRAIN_LOADED, 1);
    }
    cell_init_pass(s);
    return 1;
}

/* ------------------------------------------------------------------ *
 *  Level load @ 0x429890 — config read + pipeline + post-load init.
 *
 *  Config paths are rooted at the file's first record (upstream block
 *  parser keeps the `Lego` root, so a level key reads as
 *  "Lego* <levelname> <key>", matching the corpus's BlockQueryDefault
 *  path-building "%s%s%s").
 * ------------------------------------------------------------------ */

/* Typed getter mirroring the corpus's FUN_004790d0: boolean vocabulary
 * (TRUE/YES/ON → 1, FALSE/NO/OFF → 0), else strtol; missing → dflt. */
static int lvl_typed_int(BlockFile* cfg, const char* path, int dflt)
{
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return dflt;
    }
    if (level_strcasecmp(v, "TRUE") == 0 || level_strcasecmp(v, "YES") == 0
        || level_strcasecmp(v, "ON") == 0) {
        return 1;
    }
    if (level_strcasecmp(v, "FALSE") == 0 || level_strcasecmp(v, "NO") == 0
        || level_strcasecmp(v, "OFF") == 0) {
        return 0;
    }
    char* end = NULL;
    long r = strtol(v, &end, 10);
    return (end == v) ? 2 : (int)r;
}

/* Build the config path "<root> <level> <key>", rooted at the file's first
 * record name (`util_root_name`) — the block parser keeps the top-level
 * `Lego {…}` record as the root, and the corpus's LevelLoad paths are
 * relative to it (finding 12, FUN_004792e0). */

/* The shipped Lego.cfg nests every level group under the root's `Levels`
 * container (`Lego* { … Levels { … Level01 { … } } }`), so the flat
 * `root level key` path misses. Resolve through `Levels` first, then fall
 * back to the direct shape (the test fixtures and any flatter config).
 * Returns the resolving path (into out) or NULL. */
static const char* cfg_resolve_path(BlockFile* cfg, const char* level,
                                    const char* key, char* out, size_t out_sz)
{
    return util_level_cfg_path(cfg, level, key, out, out_sz);
}

static const char* cfg_string(BlockFile* cfg, const char* level, const char* key,
                              char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL || key == NULL) {
        return NULL;
    }
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return NULL;
    }
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return NULL;
    }
    snprintf(out, out_sz, "%s", v);
    return out;
}

static long cfg_int(BlockFile* cfg, const char* level, const char* key, long dflt)
{
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    return block_get_int(cfg, path, (int)dflt);
}

static float cfg_float(BlockFile* cfg, const char* level, const char* key, float dflt)
{
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    return block_get_float(cfg, path, dflt);
}

static int cfg_typed(BlockFile* cfg, const char* level, const char* key, int dflt)
{
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    return lvl_typed_int(cfg, path, dflt);
}

static int map_bias_name(BlockFile* cfg, const char* level, const char* key,
                         char* out, size_t out_sz)
{
    out[0] = '\0';
    if (cfg_string(cfg, level, key, out, out_sz) == NULL) {
        return 0;
    }
    return map_header_value(out);
}

LevelState* level_load(BlockFile* cfg, const char* level_name)
{
    if (cfg == NULL || level_name == NULL) {
        return NULL;
    }

    LevelState* s = (LevelState*)calloc(1, sizeof(LevelState));
    if (s == NULL) {
        return NULL;
    }

    /* --- scalar config keys (finding 07 §"What a level is") ------------- */
    level_store_ptr(s, LEVEL_OFF_LEVEL_NAME, level_name);

    float blocksize = cfg_float(cfg, level_name, K_BLOCKSIZE, 0.0f);
    level_set_float(s, LEVEL_OFF_BLOCKSIZE, blocksize);
    level_set_float(s, LEVEL_OFF_DIGDEPTH, cfg_float(cfg, level_name, K_DIGDEPTH, 0.0f));
    level_set_int(s, LEVEL_OFF_USEROOF, (int32_t)cfg_typed(cfg, level_name, K_USEROOF, 0));
    level_set_float(s, LEVEL_OFF_ROOFHEIGHT, cfg_float(cfg, level_name, K_ROOFHEIGHT, 0.0f));
    level_set_float(s, LEVEL_OFF_ROUGHLEVEL, cfg_float(cfg, level_name, K_ROUGHLEVEL, 0.0f));
    level_set_int(s, LEVEL_OFF_SAFECAVERNS, (int32_t)cfg_typed(cfg, level_name, K_SAFECAVERNS, 0));
    level_set_float(s, LEVEL_OFF_SELBOXHEIGHT, cfg_float(cfg, level_name, K_SELBOXH, 0.0f));

    /* Boulder-animation flavour: 0 = none/default, 1 = any other value. */
    {
        char boulder[64] = { 0 };
        if (cfg_string(cfg, level_name, K_BOULDER_ANIM, boulder, sizeof(boulder)) != NULL) {
            level_set_int(s, LVL_OFF_BOULDER_ANIM,
                          level_strcasecmp(boulder, "None") == 0 ? 0 : 1);
        }
    }

    /* --- the ten `.map` keys + their header (bias) words ------------------ */
    char surface[256], predug[256], terrain[256], blockptrs[256];
    char cryore[256], erode[256], emerge[256], aimap[256];
    char pathmap[256], fallin[256];
    const char* names[10] = { surface, predug, terrain, blockptrs,
                              cryore, erode, pathmap, emerge, aimap, fallin };
    const char* keys[10]  = { K_SURFACEMAP, K_PREDUGMAP, K_TERRAINMAP,
                              K_BLOCKPTRMAP, K_CRYOREMAP, K_ERODEMAP,
                              K_PATHMAP, K_EMERGEMAP, K_AIMAP, K_FALLINMAP };
    int biases[10];
    for (int i = 0; i < 10; ++i) {
        char* dst = (char*)names[i];
        biases[i] = map_bias_name(cfg, level_name, keys[i], dst, 256);
    }
    /* Only the three mandatory layers abort the load if absent (the corpus's
     * pipeline gate); the sparse layers (erode/emerge/path/ai/fallin) may be
     * missing — their loaders treat a missing name as an optional layer. */

    /* --- mission-layer helper keys -------------------------------------- */
    char selbox[64];
    cfg_string(cfg, level_name, K_TEXTURESET, selbox, sizeof(selbox)); /* unused this milestone */
    long fallin_mult = cfg_int(cfg, level_name, K_FALLIN_MULT, 1);
    if (fallin_mult == 0) {
        fallin_mult = 1;
    }

    if (!map_pipeline(s, names, biases)) {
        level_destroy(s);
        return NULL;
    }

    /* --- objective/advisor config (finding 22) ---------------------------- */
    objective_load(cfg, level_name, s);

    /* --- post-load scalar stores ----------------------------------------- */
    level_set_int(s, LEVEL_OFF_STARTP, cfg_typed(cfg, level_name, K_STARTFP, 0) == 1 ? 1 : 0);
    level_set_int(s, LEVEL_OFF_NODRAIN, cfg_typed(cfg, level_name, K_NODRAIN, 0) == 1 ? 1 : 0);
    level_set_float(s, LEVEL_OFF_OXYGEN_RATE,
                    cfg_float(cfg, level_name, K_OXYGENRATE, 0.0f) * LVL_OXYGEN_SCALE);
    level_set_float(s, LEVEL_OFF_TOLERANCE,
                    cfg_float(cfg, level_name, K_BUILDTOL, 0.0f));
    level_set_float(s, LEVEL_OFF_MAXVARIATION,
                    cfg_float(cfg, level_name, K_BUILDMAXVAR, 0.0f));

    /* Next-level chain (freed by level_destroy). */
    char next[256] = { 0 };
    if (cfg_string(cfg, level_name, K_NEXTLEVEL, next, sizeof(next)) != NULL) {
        char* dup = (char*)malloc(strlen(next) + 1);
        if (dup != NULL) {
            strcpy(dup, next);
            level_store_ptr(s, LEVEL_OFF_NEXTLEVEL, dup);
        }
    }

    /* Initial-collection counters read by the entity layer (post-load). */
    level_set_int(s, LVL_OFF_INIT_CRYS, (int32_t)cfg_int(cfg, level_name, K_INITCRYSTALS, 0));
    level_set_int(s, LVL_OFF_INIT_ORE, (int32_t)cfg_int(cfg, level_name, K_INITORE, 0));

    (void)K_OLISTFILE;
    (void)K_FULLNAME;
    (void)fallin_mult;
    return s;
}

void level_destroy(LevelState* s)
{
    if (s == NULL) {
        return;
    }
    free(s->ptr_voxels);
    free(s->ptr_layerl);
    terrain_destroy(s->ptr_terrain);
    free(s->ptr_next);
    /* ptr_name is borrowed from the caller (the level *name* passed to
     * level_load); never freed here. */
    __builtin_memset(s, 0, sizeof(*s));
    free(s);
}

int level_width(const LevelState* s) { return s ? level_int(s, LEVEL_OFF_WIDTH) : 0; }
int level_height(const LevelState* s) { return s ? level_int(s, LEVEL_OFF_HEIGHT) : 0; }

const char* level_next_name(const LevelState* s)
{
    return s != NULL ? (const char*)s->ptr_next : NULL;
}

uint8_t* level_voxel(const LevelState* s, int x, int y)
{
    if (s == NULL || x < 0 || y < 0 || x >= level_width(s) || y >= level_height(s)) {
        return NULL;
    }
    return voxel_at((LevelState*)s, x, y);
}

/* FUN_0048e060 — x87 round-to-nearest float→int (control-word trick). */
int level_cell_of(float v)
{
    return (int)__builtin_nearbyintf(v);
}

int level_walkable(const LevelState* s, int cx, int cy)
{
    if (s == NULL) {
        return 0;
    }
    /* Corpus `param < w - 2U` unsigned compare: negatives fail too. */
    int w = level_width(s), h = level_height(s);
    if ((unsigned)cx >= (unsigned)w - 2u || (unsigned)cy >= (unsigned)h - 2u) {
        return 0;
    }
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    int type = (int)(signed char)rec[2];
    return (flags & TILE_FLAG_WALKABLE) != 0 && (flags & TILE_FLAG_BLOCKING) == 0 &&
           type != 1 && type != 7 && type != 11;
}

/* FUN_00432950 — EntityMoveDestinationCellIsUsable. The corpus reads the
 * cell address without a bounds check past the destination gate (the gate
 * keeps every read in range); the port routes through level_voxel, whose
 * NULL for an unreachable cell reports unusable (documented deviation). */
int entity_move_destination_cell_usable(const struct EntitySlot* e,
                                       const LevelState* s, int cx, int cy)
{
    /* Corpus: *(uint *)(param_1 + 1000) & 0x8000 — +1000 decimal is
     * +0x3e8, the port's flags_3e8 mirror. */
    if (e == NULL || (e->flags_3e8 & 0x8000u) == 0u) {
        return 0;
    }
    const int w = level_width(s), h = level_height(s);
    /* Corpus unsigned gates: cx != 0, cy != 0, and both < dim - 2. */
    if (cx == 0 || cy == 0
        || (unsigned)cx >= (unsigned)w - 2u
        || (unsigned)cy >= (unsigned)h - 2u) {
        return 0;
    }
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof flags);
    const int type = (int)(signed char)rec[0x02];
    return (flags & TILE_FLAG_WALKABLE) != 0
           && (flags & TILE_FLAG_BLOCKING) == 0
           && type != 1 && type != 7 && type != 11;
}

/* FUN_00432a20 — TerrainCellAllowsPlacement. The gate is the corpus's
 * byte read at +1000: the port's flags_3e8 low byte (little-endian), bit
 * 0x4; the record-NULL case passes, matching the config-driven NULL call. */
int terrain_cell_allows_placement(const struct EntitySlot* e,
                                  const LevelState* s, int cx, int cy)
{
    if (e != NULL && (e->flags_3e8 & 0x04u) == 0u) {
        return 0;
    }
    const int w = level_width(s), h = level_height(s);
    if (cx == 0 || cy == 0
        || (unsigned)cx >= (unsigned)w - 2u
        || (unsigned)cy >= (unsigned)h - 2u) {
        return 0;
    }
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof flags);
    const int type = (int)(signed char)rec[0x02];
    return (flags & TILE_FLAG_WALKABLE) != 0
           && (flags & 0x20u) == 0
           && type != 8 && type != 10 && type != 11;
}

void level_tile_mark(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if (set != 0) {
        *flags |= TILE_FLAG_OCCUPIED;
    } else {
        *flags &= ~TILE_FLAG_OCCUPIED;
    }
}

int level_tile_occupied(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 1; /* no handle / no cell: cannot rule a reservation out */
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    return (flags & TILE_FLAG_OCCUPIED) != 0;
}

int level_tile_type(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (int)(signed char)rec[2] : -1;
}

/* --- terrain-cell flag primitives ----------------------------------------
 * Core readers over the voxel record. The corpus family computes the record
 * address with no bounds check at all; every port accessor goes through
 * level_voxel, so an unaddressable cell reads as all-zero flags / no state
 * (documented deviation in level.hpp). */

static uint32_t cell_flag_word(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    uint32_t flags = 0;
    if (rec != NULL) {
        __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    }
    return flags;
}

static uint32_t cell_secondary_word(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    uint32_t word = 0;
    if (rec != NULL) {
        __builtin_memcpy(&word, rec + 0x0C, sizeof(word));
    }
    return word;
}

/* FUN_00432bd0: traversable bit set, blocked gate (bit 3) clear, inside
 * the unsigned dim-1 gate. The cast keeps the corpus's unsigned wrap: a
 * zero dimension lets every coordinate through. */
int level_cell_walkable_flag(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint32_t flags = cell_flag_word(s, cx, cy);
        return (flags & 0x10u) != 0 && (flags & 0x8u) == 0;
    }
    return 0;
}

/* FUN_00432c20: wave-spawn flag bit (finding 10's wave flag). */
uint32_t level_cell_flag_wave(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x40000u;
}

/* FUN_00432c50: speed class 0 under the terrain block bit, else the low
 * terrain-type bits + 1. */
int level_cell_speed_class(const LevelState* s, int cx, int cy)
{
    const uint32_t flags = cell_flag_word(s, cx, cy);
    if ((flags & 0x200000u) != 0) {
        return 0;
    }
    return (int)(flags & 3u) + 1;
}

/* FUN_00432e60: flag word's LOW BYTE (corpus byte read) masked 0xc0. */
uint32_t level_cell_mask_c0(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (uint32_t)rec[8] & 0xc0u : 0u;
}

/* FUN_00432e90: predug edge-wall bit (finding 07, load-time 3×3 OR). */
uint32_t level_cell_flag_edge_wall(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x4000000u;
}

/* FUN_00432ec0 / FUN_00432f00: state byte equals 1 / 6. */
int level_cell_state_is_1(const LevelState* s, int cx, int cy)
{
    return level_tile_type(s, cx, cy) == 1;
}

int level_cell_state_is_6(const LevelState* s, int cx, int cy)
{
    return level_tile_type(s, cx, cy) == 6;
}

/* FUN_00432f40: terrain bits 0x18 clear; outside the gate counts as
 * clear, exactly as the corpus returns true for the early-out. */
int level_cell_mask18_clear(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        return (cell_flag_word(s, cx, cy) & 0x18u) == 0;
    }
    return 1;
}

/* FUN_00432f90: flag bit 4 inside the gate, 0 outside. */
uint32_t level_cell_flag_4(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        return cell_flag_word(s, cx, cy) & 4u;
    }
    return 0u;
}

/* FUN_00432fd0 / FUN_00433000 / FUN_00433030: single-bit masks. */
uint32_t level_cell_flag_2000(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x2000u;
}

uint32_t level_cell_flag_80(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x80u;
}

uint32_t level_cell_flag_8000(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x8000u;
}

/* FUN_00433060: secondary flag word (record+12) bit 0. */
uint32_t level_cell_secondary_bit0(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 1u;
}

/* FUN_00433090: in-bounds and OCCUPIED-class bit 0x20000000 set. The
 * corpus's signed `-1 < c < dim` is exactly level_voxel's gate, and its
 * false arm returns 0 like the NULL read here. */
int level_cell_occupied_at(const LevelState* s, int cx, int cy)
{
    return (cell_flag_word(s, cx, cy) & 0x20000000u) != 0;
}

/* FUN_004330e0: terrain/wave pair mask 0x180000. */
int level_cell_mask_180000(const LevelState* s, int cx, int cy)
{
    return (cell_flag_word(s, cx, cy) & 0x180000u) != 0;
}

/* FUN_00433120: raise/clear flag bit 0x10000000 (bounds-guarded). */
void level_cell_flag_10000000_set(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if (set != 0) {
        *flags |= 0x10000000u;
    } else {
        *flags &= ~0x10000000u;
    }
}

/* Raise/clear flag bit 0x800000 (the cell-command terrain half; see
 * level.hpp). Bounds-guarded. */
void level_cell_flag_800000_set(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if (set != 0) {
        *flags |= 0x800000u;
    } else {
        *flags &= ~0x800000u;
    }
}

/* --- second terrain-cell leaf cluster ------------------------------------
 * Same voxel-record model and bounds deviation. The corpus's tracking /
 * refresh companions landed later with the tracked set-family below —
 * the queues keep module statics and the per-cell FUN_0042f6f0 refresh
 * stays a TODO(corpus) seam. */

/* FUN_00432170: secondary word bit 0x100. */
uint32_t level_cell_secondary_flag_100(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 0x100u;
}

/* FUN_004322d0: visited mark, secondary bit 1. */
uint32_t level_cell_visited_flag(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 2u;
}

/* FUN_004323c0 / FUN_004325d0: clear/raise secondary bit 0. */
void level_cell_secondary_bit0_clear(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word &= ~1u;
}

void level_cell_secondary_bit0_set(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word |= 1u;
}

/* FUN_004324d0 / FUN_00432520: secondary bit 3 pair. */
void level_cell_secondary_bit8_set(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    if (set != 0) {
        *word |= 8u;
    } else {
        *word &= ~8u;
    }
}

uint32_t level_cell_secondary_flag_8(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 8u;
}

/* FUN_00432aa0: mode 0 = masked 0x400 read; mode nonzero = OR test of
 * the 0x400 flag and the secondary BYTE's bit 4 (the byte read is the
 * low byte of the secondary word, so the word mask is equivalent). */
uint32_t level_cell_occupied_or_flag(const LevelState* s, int cx, int cy,
                                     int mode)
{
    const uint32_t flags = cell_flag_word(s, cx, cy);
    if (mode == 0) {
        return flags & 0x400u;
    }
    if ((flags & 0x400u) == 0 && (cell_secondary_word(s, cx, cy) & 4u) == 0) {
        return 0;
    }
    return 1;
}

/* FUN_00432b00 / FUN_00432b50 / FUN_00432d90: dim-1-gated bit reads. */
static uint32_t cell_flag_word_in_gate(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        return cell_flag_word(s, cx, cy);
    }
    return 0u;
}

uint32_t level_cell_flag_800_bounded(const LevelState* s, int cx, int cy)
{
    return cell_flag_word_in_gate(s, cx, cy) & 0x800u;
}

uint32_t level_cell_flag_8_bounded(const LevelState* s, int cx, int cy)
{
    return cell_flag_word_in_gate(s, cx, cy) & 8u;
}

uint32_t level_cell_flag_20_bounded(const LevelState* s, int cx, int cy)
{
    return cell_flag_word_in_gate(s, cx, cy) & 0x20u;
}

/* FUN_00432b90: signed state byte in {8, 10, 11}. No cell reads absent. */
int level_cell_type_is_8_or_10_or_11(const LevelState* s, int cx, int cy)
{
    const int type = level_tile_type(s, cx, cy);
    return type == 8 || type == 10 || type == 11;
}

/* FUN_00432dd0: entity-reservation bit (the level_tile_mark flag). */
uint32_t level_cell_flag_reserved(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & TILE_FLAG_OCCUPIED;
}

/* FUN_00432710: raise/clear flag 0x80000 (bounds-guarded). */
void level_cell_flag_80000_set(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if (set != 0) {
        *flags |= 0x80000u;
    } else {
        *flags &= ~0x80000u;
    }
}

/* FUN_004332c0: record byte +3, unsigned. */
int level_cell_record_byte3(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (int)rec[3] : 0;
}

/* FUN_004332f0: the in-gate work-bit raise. */
void level_cell_work_flag_set(LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        uint8_t* rec = level_voxel(s, cx, cy);
        if (rec != NULL) {
            uint32_t* flags = (uint32_t*)(rec + 0x08);
            *flags |= 4u;
        }
    }
}

/* FUN_004502c0: unsigned pair membership (negative coords wrap). */
int level_coord_pair_within_bounds(const LevelState* s, int cx, int cy)
{
    return (unsigned)cx < (unsigned)level_int(s, LEVEL_OFF_WIDTH) &&
           (unsigned)cy < (unsigned)level_int(s, LEVEL_OFF_HEIGHT);
}

/* FUN_004329d0: valid coordinate with the blocked bit (flag 8) set. */
int level_cell_flag8_check(const LevelState* s, int cx, int cy)
{
    if (level_coord_pair_within_bounds(s, cx, cy)) {
        return (cell_flag_word(s, cx, cy) & 8u) != 0;
    }
    return 0;
}

/* --- map-file resource slots ---------------------------------------------
 * The corpus table interleaves (ptr, flag) pairs across 2000 entries; the
 * port keeps parallel arrays. See level.hpp for the deviation notes (the
 * corpus's MapOpen2 opener rides the unported wad seam; the allocator's
 * engine-heap wrapper is a TODO(corpus) seam mapped to calloc). */

static uint8_t* s_map_slot_ptr[LEVEL_MAP_SLOT_COUNT];
static uint32_t s_map_slot_used[LEVEL_MAP_SLOT_COUNT];

/* FUN_004893f0: clear every pair (no frees — the corpus leak loop). */
void level_map_slots_reset(void)
{
    for (int i = 0; i < LEVEL_MAP_SLOT_COUNT; ++i) {
        s_map_slot_ptr[i] = NULL;
        s_map_slot_used[i] = 0;
    }
}

/* FUN_00489410: first free slot, zero-filled image, -1 when full. */
int level_map_slot_allocate(size_t size)
{
    for (int i = 0; i < LEVEL_MAP_SLOT_COUNT; ++i) {
        if (s_map_slot_used[i] == 0) {
            void* img = calloc(1, size != 0 ? size : 1u);
            if (img == NULL) {
                return -1; /* corpus: heap seam would have asserted */
            }
            s_map_slot_ptr[i] = (uint8_t*)img;
            s_map_slot_used[i] = 1;
            return i;
        }
    }
    return -1;
}

/* FUN_00489460: free + clear an in-use slot; bad/free ids are no-ops. */
void level_map_slot_release(uint32_t slot)
{
    if (slot < LEVEL_MAP_SLOT_COUNT && s_map_slot_used[slot] != 0) {
        free(s_map_slot_ptr[slot]);
        s_map_slot_ptr[slot] = NULL;
        s_map_slot_used[slot] = 0;
    }
}

/* FUN_004894a0: the image pointer, or NULL when free/out of range. */
uint8_t* level_map_slot_get(uint32_t slot)
{
    if (slot < LEVEL_MAP_SLOT_COUNT && s_map_slot_used[slot] != 0) {
        return s_map_slot_ptr[slot];
    }
    return NULL;
}

/* FUN_004515a0: header dims at +0x08/+0x0C (port guards a NULL image). */
void level_map_image_header(const uint8_t* image, uint32_t* width,
                            uint32_t* height)
{
    *width = 0;
    *height = 0;
    if (image != NULL) {
        __builtin_memcpy(width, image + 0x08, sizeof(*width));
        __builtin_memcpy(height, image + 0x0C, sizeof(*height));
    }
}

/* FUN_004515c0: zero-extended u16 cell at +0x10 + (y*width+x)*2. */
uint16_t level_map_image_cell(const uint8_t* image, int x, int width, int y)
{
    if (image == NULL) {
        return 0;
    }
    const size_t idx = (size_t)y * (size_t)width + (size_t)x;
    uint16_t cell;
    __builtin_memcpy(&cell, image + 0x10 + idx * 2, sizeof(cell));
    return cell;
}

/* FUN_004334c0: header dims through the slot (NULL image -> 0/0, the
 * documented stand-in for the corpus's read through the unresolved 0). */
void level_map_dimensions_get(uint32_t slot, uint32_t* width,
                              uint32_t* height)
{
    level_map_image_header(level_map_slot_get(slot), width, height);
}

/* FUN_004319e0: cell through the slot; the width comes from the header. */
uint16_t level_map_cell_get(uint32_t slot, int x, int y)
{
    const uint8_t* img = level_map_slot_get(slot);
    uint32_t w = 0, h = 0;
    level_map_image_header(img, &w, &h);
    return level_map_image_cell(img, x, (int)w, y);
}

/* FUN_00433330: gated state-byte read through an out param. */
int level_cell_type_get_at(const LevelState* s, uint32_t cx, uint32_t cy,
                           uint32_t* out_type)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if (cx < ((unsigned)w - 1u) && cy < ((unsigned)h - 1u)) {
        const uint8_t* rec = level_voxel(s, (int)cx, (int)cy);
        if (rec != NULL) {
            *out_type = (uint32_t)rec[0x02];
        }
        return 1;
    }
    return 0;
}

/* FUN_0042c5b0: byte classification over the five-entry table. */
int level_cell_byte_state_classify(const LevelState* s, int cx, int cy,
                                   uint32_t* out_a, uint32_t* out_b,
                                   uint32_t* out_c, uint32_t* out_d)
{
    static const uint32_t kTable[5] = { 1, 3, 5, 10, 25 };
    *out_a = 0;
    *out_b = 0;
    *out_c = 0;
    *out_d = 0;

    const uint8_t* rec = level_voxel(s, cx, cy);
    const uint32_t v = rec != NULL ? (uint32_t)rec[0x05] : 0u;
    if (v == 0) {
        return 0;
    }
    const uint32_t idx = v - 1u;
    const uint32_t slot = idx >> 2;
    const uint32_t value = slot < 5 ? kTable[slot] : 0u; /* stack-read guard */
    switch (idx & 3u) {
        case 0u:  *out_a = value; break;
        case 1u:  *out_c = value; break;
        case 2u:  *out_b = value; break;
        default:  *out_d = value; break;
    }
    return 1;
}

/* ActiveEngineFieldB0Get (FUN_00429850, findings 295/392): the active
 * engine-state dword at level-block +0xb0. */
uint32_t active_engine_field_b0_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, LEVEL_OFF_ENGINE_B0) : 0u;
}

/* EngineFloatField210GetByIndex (FUN_00429860, findings 295/392): the
 * indexed float at +0x210 + 4*index. */
float engine_float_field210_get_by_index(const LevelState* s, int index)
{
    if (s == NULL) {
        return 0.0f;
    }
    return level_float(s, LEVEL_OFF_ENGINE_FLOATS + index * 4);
}

/* EngineFloatField260Get (FUN_00429880, findings 295/392): the float at
 * active engine-state +0x260. */
float engine_float_field260_get(const LevelState* s)
{
    return s != NULL ? level_float(s, LEVEL_OFF_ENGINE_F260) : 0.0f;
}

/* AirSupplyAmountUpdate (FUN_00424560, finding 317): see level.hpp. The
 * stats read lands before the gate (corpus order); the corpus's float10
 * accumulation is modelled with the port's double — the documented
 * extended-precision deviation. */
void air_supply_amount_update(LevelState* s, const struct EntitySlot* e,
                              float delta)
{
    if (s == NULL || e == NULL) {
        return; /* deviation: the corpus strides both blindly */
    }
    const double stats = (double)entity_stats_float_atdc(e);
    if (e->type == 4 && (e->flags_3e8 & 0x20000000u) == 0u) {
        return;
    }
    const double rate = (double)level_float(s, LEVEL_OFF_OXYGEN_RATE);
    const double acc = (double)level_float(s, LEVEL_OFF_AIR_SUPPLY);
    const double next = (double)delta * 0.04 * rate * stats + acc;
    float stored = (float)next;
    if (next > 100.0) {
        stored = 100.0f;
    }
    level_set_float(s, LEVEL_OFF_AIR_SUPPLY, stored);
    if (stored <= 0.0f) {
        level_set_float(s, LEVEL_OFF_AIR_SUPPLY, 0.0f);
        if ((system_state_flags_get() & 0x08000000u) == 0u) {
            objective_announce(s, 2);
        }
    }
}

/* GlobalStateField84Get (FUN_0042b500, findings 324/392): the dword at
 * +0x84; the field's meaning stays unresolved (finding 324 says so),
 * this is the faithful read. */
uint32_t global_state_field84_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, LEVEL_OFF_STATE84) : 0u;
}

/* UpgradeResourceCheck (FUN_0041f900, finding 475): the level's resource
 * counters — +0xa0 for selector 0, +0xac for any nonzero selector; and
 * zero when no level object is active (the corpus's own DAT-null rule,
 * which doubles as the port's NULL guard). */
uint32_t upgrade_resource_check(const LevelState* s, int selector)
{
    if (s == NULL) {
        return 0u;
    }
    return (uint32_t)level_int(s, selector != 0 ? LEVEL_OFF_RES_B
                                               : LEVEL_OFF_RES_A);
}

/* AlwaysAcceptPlaceCommandCell (FUN_00432550, finding 455): always yes. */
int always_accept_place_command_cell(void)
{
    return 1;
}

/* --- tracked/refreshing cell-flag set family ------------------------------
 * See level.hpp for the deviations. The corpus's tracking DAT regions:
 *   DAT_00556e8c        refresh-track counter (queue ends at it)
 *   DAT_0055684c / DAT_00556850   refresh-track (x,y) pair slots
 *   DAT_00556848        visited-track counter
 *   DAT_00556528 / DAT_0055652c   visited-track (x,y) pair slots
 * modeled as module statics; consumers land with their own clusters. */

#define LEVEL_CELL_REFRESH_TRACK_CAP 200
#define LEVEL_CELL_VISITED_TRACK_CAP 160

static int32_t s_cell_refresh_track[LEVEL_CELL_REFRESH_TRACK_CAP][2];
static int     s_cell_refresh_track_count;
static int32_t s_cell_visited_track[LEVEL_CELL_VISITED_TRACK_CAP][2];
static int     s_cell_visited_track_count;

static void cell_refresh_track_push(int cx, int cy)
{
    if (s_cell_refresh_track_count < LEVEL_CELL_REFRESH_TRACK_CAP) {
        /* corpus writes unbounded (deviation: silent bound). */
        s_cell_refresh_track[s_cell_refresh_track_count][0] = cx;
        s_cell_refresh_track[s_cell_refresh_track_count][1] = cy;
        ++s_cell_refresh_track_count;
    }
}

static void cell_visited_track_push(int cx, int cy)
{
    if (s_cell_visited_track_count < LEVEL_CELL_VISITED_TRACK_CAP) {
        s_cell_visited_track[s_cell_visited_track_count][0] = cx;
        s_cell_visited_track[s_cell_visited_track_count][1] = cy;
        ++s_cell_visited_track_count;
    }
}

static void cell_refresh_seam(LevelState* s, int cx, int cy)
{
    (void)s; (void)cx; (void)cy;
    /* TODO(corpus): FUN_0042f6f0(manager, x, y, 0) per-cell refresh;
     * the load-path cell_init_pass covers the sweep form. */
}

void terrain_cell_flag100_set_and_track(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word |= 0x100u;
    cell_refresh_track_push(cx, cy);
    cell_refresh_seam(s, cx, cy);
}

void terrain_cell_visited_flag_set_and_track(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word |= 2u;
    cell_visited_track_push(cx, cy);
}

void terrain_cell_visited_flags_clear_all(LevelState* s)
{
    int i;
    for (i = 0; i < s_cell_visited_track_count; ++i) {
        uint8_t* rec = level_voxel(s, s_cell_visited_track[i][0],
                                  s_cell_visited_track[i][1]);
        if (rec == NULL) {
            continue;
        }
        uint32_t* word = (uint32_t*)(rec + 0x0C);
        *word &= ~2u;
    }
    s_cell_visited_track_count = 0;
}

void terrain_cell_primary_flags_mask_clear_and_refresh(LevelState* s,
                                                       int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    *primary &= 0xdfef7bffu;
    uint32_t* secondary = (uint32_t*)(rec + 0x0C);
    *secondary &= 0xfffffffbu;
    cell_refresh_seam(s, cx, cy);
}

void terrain_cell_flags_set_and_refresh(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    *primary |= 0x108000u;
    uint32_t* secondary = (uint32_t*)(rec + 0x0C);
    *secondary |= 4u;
    cell_refresh_seam(s, cx, cy);
}

void terrain_cell_flags_set_mask100400(LevelState* s, int x, int y)
{
    uint8_t* rec = level_voxel(s, x, y);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    *primary |= 0x100400u;
    cell_refresh_seam(s, x, y);
}

void terrain_cell_flags_set_mask20108000(LevelState* s, int x, int y)
{
    uint8_t* rec = level_voxel(s, x, y);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    *primary |= 0x20108000u;
    cell_refresh_seam(s, x, y);
}

void terrain_cell_flag40000000_clear_or_set(LevelState* s, int cx, int cy,
                                            int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    if (set == 0) {
        *primary &= 0xbfffffffu;
    } else {
        *primary |= 0x40000000u;
    }
    cell_refresh_seam(s, cx, cy);
}

/* CellNeighborDirectionCode (FUN_004508a0): eight neighbors, -1 else. */
int cell_neighbor_direction_code(int x1, int y1, int x2, int y2)
{
    if (x1 == x2) {
        if (y2 == y1 - 1) {
            return 0;
        }
        if (y2 == y1 + 1) {
            return 2;
        }
    } else if (y1 == y2) {
        if (x2 == x1 + 1) {
            return 1;
        }
        if (x2 == x1 - 1) {
            return 3;
        }
    } else if (x2 == x1 + 1) {
        if (y2 == y1 - 1) {
            return 4;
        }
        if (y2 == y1 + 1) {
            return 5;
        }
    } else if (x2 == x1 - 1) {
        if (y2 == y1 + 1) {
            return 6;
        }
        if (y2 == y1 - 1) {
            return 7;
        }
    }
    return -1;
}

/* --- neighbor-refresh / circle / edge-flag cluster (see level.hpp) ------- */

void terrain_neighbor_flag_refresh(LevelState* s, int cx, int cy, int mode)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return; /* bounds deviation (as across this module) */
    }
    uint32_t build = 0u;
    __builtin_memcpy(&build, rec + 0x18, sizeof(build));
    /* The corpus reads a cell-record word +0x18 as the activity-build
     * record pointer; in the corpus's 32-bit space the word is the
     * pointer. The port's voxel layout keeps the corpus's byte layout,
     * and the word's owner — the activity-build release family — is
     * unported, so every port case reads zero and both corpus branches
     * (destroy registration through the ported SceneActionNameSet leaf
     * with the scene-frame seam, or the byte +0xC marks) collapse to
     * the no-op below. TODO(corpus): the activity-build layer seam. */
    if (build == 0u) {
        return;
    }
    (void)mode;
}

void terrain_cell_flag20_clear_and_refresh(LevelState* s, int cx, int cy)
{
    terrain_neighbor_flag_refresh(s, cx, cy, 0);
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec != NULL) {
        uint32_t* word = (uint32_t*)(rec + 0x08);
        *word &= 0xffffffdfu;
    }
    cell_refresh_seam(s, cx, cy);
}

void terrain_cell_edge_flag_set_and_update(LevelState* s, int cx, int cy,
                                           int set)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec != NULL) {
        uint32_t* word = (uint32_t*)(rec + 0x08);
        if (set != 0) {
            *word |= 0x200u;
        } else {
            *word &= 0xfffffdffu;
        }
    }
    cell_refresh_seam(s, cx, cy);
}

void terrain_cell_work_flag_circle_set(LevelState* s, int center_x,
                                       int center_y, int radius)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    const int r2 = radius * radius; /* corpus wrapping int math */
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy > r2) {
                continue;
            }
            const int cx = center_x + dx;
            const int cy = center_y + dy;
            /* The corpus's signed candidate bounds, then the setter's
             * own in-gate test — both faithful. */
            if (-1 < cx && cx < w && -1 < cy && cy < h) {
                level_cell_work_flag_set(s, cx, cy);
            }
        }
    }
}

/* TerrainNeighborWorkFlagScan (FUN_00431580): the work-flag breadth-first
 * sweep. A double-buffered float-pair queue (two pages of (w+h)*2 entries
 * carved from one (w+h)*0x20 engine-heap block — FUN_0048dbd0/FUN_0048db80)
 * starts at the seed coordinate; each round drains the current page and, for
 * every neighbour (row = y±1, col = x±1, indexed row*w + col in the stride-
 * 0x48 cell records at DAT+0xb8, port: level_voxel), processes the cells
 * carrying the work flag bit 0x20000: the corpus fires TerrainCellMutation
 * (FUN_00430530) in mode 1 — whose side effects live in the hook-folded
 * wave-flood core, so the word the next line clears is the only observable
 * state here — clears the bit, and enqueues the cell into the other page.
 * Rounds alternate pages until the new page arrives empty. Deviations: the
 * corpus indexes out-of-bounds cells blindly, the port's level_voxel reads
 * answer zero; and the corpus never bounds-checks the enqueue, so a sweep
 * wider than the pages spills (the port mirrors this unbounded store).
 * Corpus quirk, kept faithful: the enqueue writes the float of the entry's
 * col-bound (col+1, the FPU-stack leftover) as the x coordinate and only
 * the row rides the loop variable — queue cells drift one column right per
 * hop — and the self-skip compares that bound float against the float of
 * the queue-buffer pointer, a test that never lands, so every hit cell is
 * enqueued including the seed cell itself. */
void terrain_neighbor_work_flag_scan(LevelState* s, unsigned int seed_x,
                                     unsigned int seed_y)
{
    const int w = level_width(s);
    const int h = level_height(s);
    const int span = w + h;
    if (span <= 0) {
        return; /* deviation: the corpus seeds a zero-size (1-byte) alloc
                 * and spills into the block header; the port declines */
    }
    float* buf = (float*)engine_heap_alloc((long)span * 0x20);
    if (buf == NULL) {
        return; /* corpus: the entire scan is alloc-guarded */
    }
    float* page[2];
    unsigned int count[2];
    page[0] = buf;
    page[1] = buf + span * 4; /* +16(w+h) bytes = 4(w+h) floats */
    count[0] = 1u;
    count[1] = 0u;
    page[0][0] = (float)seed_x; /* corpus fildll's the dword's bits */
    page[0][1] = (float)seed_y;
    int cur = 0;
    do {
        if (count[cur] != 0u) {
            const float* pf = page[cur];
            for (unsigned int idx = 0; idx < count[cur]; ++idx, pf += 2) {
                const int col0 = level_cell_of(pf[0]); /* FUN_0048e060 pair */
                const int row0 = level_cell_of(pf[1]);
                if ((long long)row0 - 1 <= (long long)row0 + 1) { /* corpus's wrap guard */
                    for (int row = row0 - 1; row <= row0 + 1; ++row) {
                        for (int col = col0 - 1; col <= col0 + 1; ++col) {
                            uint8_t* rec = level_voxel(s, col, row);
                            uint32_t word = 0;
                            if (rec != NULL) {
                                __builtin_memcpy(&word, rec + 0x08,
                                                 sizeof(word));
                            }
                            if ((word & 0x20000u) != 0u) {
                                /* TerrainCellMutation mode 1 (FUN_00430530):
                                 * folded hook, see the note above. */
                                word &= 0xfffdffffu;
                                if (rec != NULL) {
                                    __builtin_memcpy(rec + 0x08, &word,
                                                     sizeof(word));
                                }
                                unsigned int n = count[1 - cur];
                                float* dst = page[1 - cur];
                                dst[2 * n] = (float)(col0 + 1);
                                count[1 - cur] = n + 1u;
                                dst[2 * n + 1] = (float)row;
                            }
                        }
                    }
                }
            }
        }
        cur = (cur == 0) ? 1 : 0;
        count[1 - cur] = 0u; /* the drained page's count */
    } while (count[cur] != 0u);
    engine_heap_free(buf);
}

/* --- cell timestamp dedup (see level.hpp) --------------------------------
 * Corpus regions: the (x,y) pair slots (stride 8) end at the per-entry
 * active flags (stride 4), the stamps live in their own stride-4 region,
 * and the scan bound is the first address past the flag region — all
 * 1000 entries. */
#define LEVEL_TS_COUNT 1000

static int32_t  s_ts_x[LEVEL_TS_COUNT];
static int32_t  s_ts_y[LEVEL_TS_COUNT];
static int32_t  s_ts_active[LEVEL_TS_COUNT];
static int32_t  s_ts_stamps[LEVEL_TS_COUNT];
static int32_t  s_ts_now; /* _DAT_004d78c0: erosion_timing_state_init seeds the float bits (zero until then) */

void cell_coordinate_timestamp_remember(int x, int y)
{
    int i;
    for (i = 0; i < LEVEL_TS_COUNT; ++i) {
        if (s_ts_active[i] == 0) {
            s_ts_x[i] = x;
            s_ts_y[i] = y;
            s_ts_stamps[i] = s_ts_now;
            s_ts_active[i] = 1;
            return;
        }
    }
    /* full table: stores nothing (faithful) */
}

uint32_t cell_coordinate_timestamp_exists(int x, int y)
{
    int i;
    for (i = 0; i < LEVEL_TS_COUNT; ++i) {
        if (s_ts_active[i] != 0 && s_ts_x[i] == x && s_ts_y[i] == y) {
            return 1u;
        }
    }
    return 0u;
}

/* --- terrain dirty queue (see level.hpp) ---------------------------------
 * The corpus's queue lives inside its terrain context (u16 pair entries,
 * stride 4, capacity (0x7198-0x4a88)/4 = 2500, count at +0x7198). */
#define LEVEL_DIRTY_QUEUE_CAP 2500

static int16_t s_dirty_queue_x[LEVEL_DIRTY_QUEUE_CAP];
static int16_t s_dirty_queue_y[LEVEL_DIRTY_QUEUE_CAP];
static int     s_dirty_queue_count;

void terrain_cell_dirty_queue_push_and_clear(void* terrain, int x, int y,
                                             int push)
{
    const TerrainState* t = (const TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return; /* deviation: corpus dereferences its context blindly */
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return; /* bounds deviation (as across this module) */
    }
    uint8_t* flag = t->blocks
                    + ((size_t)y * (size_t)t->width + (size_t)x)
                      * TERRAIN_BLOCK_STRIDE + 0x19;
    if (push == 0) {
        if ((*flag & 0x10) == 0) {
            return;
        }
        if (s_dirty_queue_count == 0) {
            return; /* corpus's empty-queue path: flag stays set */
        }
        int i;
        for (i = 0; i < s_dirty_queue_count; ++i) {
            if (s_dirty_queue_x[i] == x && s_dirty_queue_y[i] == y) {
                break;
            }
        }
        if (i >= s_dirty_queue_count) {
            return; /* corpus's not-found path: flag stays set */
        }
        /* overwrite the found entry with the last one, then drop it */
        s_dirty_queue_x[i] = s_dirty_queue_x[s_dirty_queue_count - 1];
        s_dirty_queue_y[i] = s_dirty_queue_y[s_dirty_queue_count - 1];
        --s_dirty_queue_count;
        *flag &= 0xef;
        return;
    }
    if ((*flag & 0x10) != 0) {
        return; /* already queued (faithful) */
    }
    if (s_dirty_queue_count < LEVEL_DIRTY_QUEUE_CAP) {
        s_dirty_queue_x[s_dirty_queue_count] = (int16_t)x;
        s_dirty_queue_y[s_dirty_queue_count] = (int16_t)y;
        ++s_dirty_queue_count;
    } /* deviation: corpus appends past its context's region end */
    *flag |= 0x10;
}

/* terrain_cell_dirty_flag: faithful physical-byte read (see level.hpp). */
int terrain_cell_dirty_flag(const void* terrain, int x, int y)
{
    const TerrainState* t = (const TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL || x < 0 || y < 0
        || x >= t->width || y >= t->height) {
        return 0;
    }
    return (t->blocks
            + ((size_t)y * (size_t)t->width + (size_t)x)
              * TERRAIN_BLOCK_STRIDE + 0x19)[0] & 0x10;
}

/* --- the cell-list min-distance scale apply (see level.hpp) ---------------
 * The corpus's cell-list context (the level block's +0x10 pointer) has no
 * ported owner, so its scale word, scene-object slot, and the applied-
 * flag bit land as zero-init own storage behind install/peek hooks (the
 * unowned-DAT convention). The dirty-cell queue it strides is the port's
 * module static above, entry-for-entry the same capacity. */
static void*    s_cell_list_object;
static int      s_cell_list_scale;
static uint32_t s_cell_list_flags; /* corpus word +0x73d0: bit 2 = applied */

void cell_list_apply_state_install(void* scene_object, int scale)
{
    s_cell_list_object = scene_object;
    s_cell_list_scale = scale;
}

void* cell_list_object_peek(void)
{
    return s_cell_list_object;
}

uint32_t cell_list_apply_flags_peek(void)
{
    return s_cell_list_flags;
}

void cell_list_min_distance_scale_apply(int enable) /* FUN_00450a80 */
{
    if (enable != 0 && (s_cell_list_flags & 4u) == 0) {
        for (int i = 0; i < s_dirty_queue_count; ++i) {
            /* The corpus strides the u16 pair entries unsigned: the
             * index folds the y-entry through the scale word. */
            const uint32_t x = (uint32_t)(uint16_t)s_dirty_queue_x[i];
            const uint32_t y = (uint32_t)(uint16_t)s_dirty_queue_y[i];
            scene_mesh_color_set(s_cell_list_object,
                                 (int)(y * (uint32_t)s_cell_list_scale + x),
                                 0.3f, 0.3f, 0.3f);
        }
        s_cell_list_flags |= 4u;
        return;
    }
    if (enable == 0 && (s_cell_list_flags & 4u) != 0) {
        for (int i = 0; i < s_dirty_queue_count; ++i) {
            const uint32_t x = (uint32_t)(uint16_t)s_dirty_queue_x[i];
            const uint32_t y = (uint32_t)(uint16_t)s_dirty_queue_y[i];
            scene_mesh_color_set(s_cell_list_object,
                                 (int)(y * (uint32_t)s_cell_list_scale + x),
                                 0.0f, 0.0f, 0.0f);
        }
        s_cell_list_flags &= ~4u;
    }
}

/* --- tile-variant grid-record table (see level.hpp) -----------------------
 * Corpus region: DAT 0x4b8bc8..0x4c71fc as one dword array; the block
 * base (record pointer word) sits at dword 32, the field words follow
 * at +1, and the two variant-value tables alias the region at dwords
 * 0 and 16 behind the block base. Zero-initialized static: the loaders
 * that feed the corpus tables are unported, so variant pointers and
 * values stay zero in every port case. */
#define GRID_REC_REGION_DWORDS 14712 /* (0x4c71fc - 0x4b8bc8) / 4 */
#define GRID_REC_COLS          15    /* x scale (index = y + x * 15) */
#define GRID_REC_STRIDE        0x31  /* dwords per record block */
#define GRID_REC_FIELD_WORDS   16    /* cleared by the mask sweep */

static uint32_t s_grid_rec[GRID_REC_REGION_DWORDS];

/* Flat dword index of a block's record-pointer word, or -1 when the
 * coordinate cannot address the region (bounds deviation). */
static long long grid_rec_block_index(int x, int y)
{
    const long long idx = (long long)y + (long long)x * GRID_REC_COLS;
    const long long off = 32 + idx * GRID_REC_STRIDE;
    if (idx < 0 || off < 0 || off >= GRID_REC_REGION_DWORDS) {
        return -1;
    }
    return off;
}

/* Flat dword index of a block field word, or -1. */
static long long grid_rec_field_index(int x, int y, int field)
{
    const long long off = grid_rec_block_index(x, y);
    if (off < 0 || field < 0) {
        return -1;
    }
    const long long word = off + 1 + field;
    if (word >= GRID_REC_REGION_DWORDS) {
        return -1;
    }
    return word;
}

void grid_record_flag_mask_clear(void)
{
    size_t k = 33; /* the corpus's field-word origin */
    do {
        for (int row = 0; row < 15; ++row) {
            for (int d = 0; d < GRID_REC_FIELD_WORDS; ++d) {
                s_grid_rec[k + (size_t)d] &= 0xfffffef3u; /* ~ 0x10c */
            }
            k += GRID_REC_STRIDE;
        }
    } while (k + 33 < GRID_REC_REGION_DWORDS);
}

void terrain_cell_record_get(int x, int y, int field,
                             uint32_t* value_a, uint32_t* value_b)
{
    const long long block = grid_rec_block_index(x, y);
    if (block < 0) {
        return;
    }
    const uint32_t variant = s_grid_rec[(size_t)block];
    const long long off = block + (variant != 0u ? field : 0);
    /* The corpus indexes both tables by (field + idx * 0x31) dwords,
     * with the fallback form adding the 0xc4-byte per-cell stride; the
     * region guard covers both shapes. */
    if (off < 0 || off + 16 >= GRID_REC_REGION_DWORDS) {
        return;
    }
    const uint32_t a = s_grid_rec[(size_t)off];         /* DAT table A */
    const uint32_t b = s_grid_rec[(size_t)off + 16];    /* DAT table B */
    if (value_a != NULL) {
        *value_a = a;
    }
    if (value_b != NULL) {
        *value_b = b;
    }
}

uint32_t cell_flag4_is_set(int x, int y, int field)
{
    const long long block = grid_rec_block_index(x, y);
    if (block < 0) {
        return 0;
    }
    const long long word = (s_grid_rec[(size_t)block] != 0u)
        ? grid_rec_field_index(x, y, field)
        : grid_rec_field_index(x, y, 0);
    if (word < 0) {
        return 0;
    }
    return s_grid_rec[(size_t)word] & 4u;
}

void cell_flag100_set(int x, int y, int field)
{
    const long long word = grid_rec_field_index(x, y, field);
    if (word >= 0) {
        s_grid_rec[(size_t)word] |= 0x100u;
    }
}

uint32_t cell_flag100_is_set(int x, int y, int field)
{
    const long long word = grid_rec_field_index(x, y, field);
    if (word < 0) {
        return 0;
    }
    return s_grid_rec[(size_t)word] & 0x100u;
}

/* The present table lands NULL: its heap owner (the grid-initialize
 * leaf, address deliberately unquoted) is unported. */
static uint32_t* s_grid_present_table;
static const uint32_t* s_grid_record_manager;

int grid_cell_record_present_check(int x, int y)
{
    if (s_grid_present_table == NULL || s_grid_record_manager == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    const int w = (int)s_grid_record_manager[5]; /* +0x14 */
    const long long idx = (long long)w * y + x;
    if (idx < 0) {
        return 0;
    }
    return s_grid_present_table[(size_t)idx * 3] != 0u; /* stride 0xc */
}

/* --- corner normal sampling (see level.hpp) ------------------------------- */
int terrain_cell_normal_sample(const void* terrain, uint32_t x, uint32_t y,
                               uint32_t corner, float out[3])
{
    const TerrainState* t = (const TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL || out == NULL
        || x >= (uint32_t)t->width || y >= (uint32_t)t->height) {
        for (int i = 0; i < 3; ++i) {
            out[i] = 0.0f; /* deviation: corpus writes blindly */
        }
        return 0;
    }
    const uint8_t* p = t->blocks
                       + ((size_t)y * (size_t)t->width + (size_t)x)
                         * TERRAIN_BLOCK_STRIDE;
    uint32_t sel = corner;
    if ((p[0x19] & 1u) != 0u) {
        sel = (sel + 1u) & 3u;
    }
    float v[3];
    const float* fa = (const float*)(p + 0x00);
    const float* fb = (const float*)(p + 0x0c);
    if (sel == 0u) {
        v[0] = fa[0]; v[1] = fa[1]; v[2] = fa[2];
    } else if (sel == 2u) {
        v[0] = fb[0]; v[1] = fb[1]; v[2] = fb[2];
    } else {
        v[0] = fb[0] + fa[0];
        v[1] = fb[1] + fa[1];
        v[2] = fb[2] + fa[2];
        const float inv = 1.0f / (float)
            __builtin_sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        v[0] *= inv;
        v[1] *= inv;
        v[2] *= inv;
    }
    out[0] = v[0];
    out[1] = v[1];
    out[2] = v[2];
    return 1;
}

/* --- surface-type config leaves (see level.hpp) ----------------------------
 * Both DAT tables belong to the unported surface-type descriptions loader,
 * so they land zero-initialized / NULL as module statics. */
#define SURFACE_TYPE_SLOTS 64

static const char* s_surface_type_names[SURFACE_TYPE_SLOTS];
static uint32_t s_surface_type_values[SURFACE_TYPE_SLOTS];

const char* terrain_surface_type_name_get(int index)
{
    static const char missing[] = "Need config for surface type";
    if (index < 0 || index >= SURFACE_TYPE_SLOTS
        || s_surface_type_names[index] == NULL) {
        return missing;
    }
    return s_surface_type_names[index];
}

uint32_t surface_type_configured_value_at_index(int index)
{
    if (index < 0 || index >= SURFACE_TYPE_SLOTS) {
        return 0; /* bounds deviation (corpus indexes blindly) */
    }
    return s_surface_type_values[index];
}

/* --- neighbor coordinate classification (see level.hpp) ------------------- */
uint32_t terrain_neighbor_coordinate_check(const LevelState* s, int x, int y,
                                           uint8_t* kind, int mode)
{
    /* Corpus neighbor deltas: (0,-1), (+1,0), (0,+1), (-1,0). */
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    /* Mask -> (kind, code) tables for mode 0 / mode != 0. */
    static const uint8_t kind_tbl[16] = {
        0, 2, 3, 3, 0, 0, 0, 0, 1, 2, 1, 3, 1, 2, 1, 0
    };
    static const uint8_t code_tbl[16] = {
        0x65, 0x65, 0x65, 99, 0x65, 0x62, 99, 100,
        0x65, 99, 0x62, 100, 99, 100, 100, 0x60
    };
    static const uint8_t alt_code_tbl[16] = {
        0x75, 0x75, 0x75, 0x73, 0x75, 0x72, 0x73, 0x74,
        0x75, 0x73, 0x72, 0x74, 0x73, 0x74, 0x74, 0x71
    };

    unsigned mask = 0;
    for (unsigned i = 0; i < 4; ++i) {
        const uint32_t w = cell_flag_word(s, x + dx[i], y + dy[i]);
        if ((w & 0x20000000u) != 0u || (w & 0x400u) != 0u) {
            mask |= 1u << i;
        }
    }
    const uint8_t k = kind_tbl[mask];
    if (kind != NULL) {
        *kind = k;
    }
    return (mode != 0) ? alt_code_tbl[mask] : code_tbl[mask];
}

/* --- grid-record count tables (see level.hpp) ------------------------------
 * The corpus's two adjacent tables (DAT pairs 0x4dee44/0x4e3944) as two
 * zero-initialized module statics: no ported code feeds them. Table 0 is
 * the first table (the read leaf's source / the difference's subtrahend),
 * table 1 the second (the difference's minimend). */
#define GRID_COUNT_TABLE_DWORDS 0x12c0 /* 300 cells * 16 slots */

static uint32_t s_grid_count_first[GRID_COUNT_TABLE_DWORDS];
static uint32_t s_grid_count_second[GRID_COUNT_TABLE_DWORDS];

static long long grid_count_index(int row, int column, int slot)
{
    const long long cell = (long long)row * 15 + column;
    const long long idx = cell * 16 + slot;
    if (cell < 0 || slot < 0 || idx >= GRID_COUNT_TABLE_DWORDS) {
        return -1; /* bounds deviation (corpus indexes blindly) */
    }
    return idx;
}

void grid_record_count_tables_clear(void)
{
    for (size_t k = 0; k < GRID_COUNT_TABLE_DWORDS; ++k) {
        s_grid_count_first[k] = 0u;
        s_grid_count_second[k] = 0u;
    }
}

int grid_record_count_difference_get(int row, int column, int slot,
                                     int subtract_first_table)
{
    const long long idx = grid_count_index(row, column, slot);
    if (idx < 0) {
        return 0;
    }
    const int v = (int)s_grid_count_second[(size_t)idx];
    return (subtract_first_table != 0)
        ? v - (int)s_grid_count_first[(size_t)idx]
        : v;
}

uint32_t grid_record_count_get(int row, int column, int slot)
{
    const long long idx = grid_count_index(row, column, slot);
    return (idx < 0) ? 0u : s_grid_count_first[(size_t)idx];
}

int32_t grid_count_table_slot_peek(int row, int column, int slot, int table)
{
    const long long idx = grid_count_index(row, column, slot);
    if (idx < 0) {
        return -1;
    }
    return (int)(table != 0 ? s_grid_count_second[(size_t)idx]
                            : s_grid_count_first[(size_t)idx]);
}

/* --- surface-type writer pair (2026-10-24, see level.hpp) -------------------
 * The corpus's appearance table (DAT_004a52c0, 12-byte entries) ships the
 * reference data's six color triplets: white, half-gray, brown, light
 * green, blue, yellow. Larger type indexes read past the table into
 * undefined data in the corpus; the port substitutes white. The triplets
 * feed only the scene mesh color and alpha seam, so no other port state
 * depends on them. */
static const float s_surface_appearance[6][3] = {
    { 1.0f, 1.0f, 1.0f },   /* 0 white */
    { 0.5f, 0.5f, 0.5f },   /* 1 half-gray */
    { 1.0f, 0.5f, 0.4f },   /* 2 brown */
    { 0.5f, 1.0f, 0.5f },   /* 3 light green */
    { 0.4f, 0.5f, 1.0f },   /* 4 blue */
    { 0.6f, 0.6f, 0.0f },   /* 5 yellow */
};

void terrain_cell_surface_appearance_apply(void* terrain, int x, int y,
                                           int mode, float r, float g,
                                           float b)
{
    TerrainState* t = (TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return;
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return; /* deviation: the corpus indexes blind */
    }
    uint8_t* blk = t->blocks
        + ((size_t)y * (size_t)t->width + (size_t)x) * TERRAIN_BLOCK_STRIDE;
    if (mode == 0) {
        /* corpus: the mesh half gets (1,1,1) and (1,1,1) alpha, and the
         * byte is cleared. */
        blk[0x24] = 0u;
        (void)r; (void)g; (void)b;
    }
    /* mode nonzero: the triplet (and a 1.0 alpha) is handed to the scene
     * mesh color seam — the renderer boundary, no ported state (the
     * corpus's mesh-record index uses the width-minus-one stride, like
     * the other mesh-half deviations; no ported consumer). */
    (void)r; (void)g; (void)b;
}

int terrain_cell_surface_type_set(void* terrain, int x, int y, int type)
{
    TerrainState* t = (TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return 0;
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return 0; /* deviation: the corpus indexes blind */
    }
    uint8_t* blk = t->blocks
        + ((size_t)y * (size_t)t->width + (size_t)x) * TERRAIN_BLOCK_STRIDE;
    const int prior = (int)blk[0x24];
    blk[0x24] = (uint8_t)type;
    const float* c = ((unsigned)type < 6u)
        ? s_surface_appearance[type] : s_surface_appearance[0];
    /* The corpus looks up the triplet at table + type*0xc and hands it to
     * the appearance leaf (mode 1) — the seam half above. */
    terrain_cell_surface_appearance_apply(t, x, y, 1, c[0], c[1], c[2]);
    return prior;
}

void terrain_cell_surface_type_reset(void* terrain, int x, int y)
{
    /* corpus: the white appearance (1,1,1) through the leaf in mode 0,
     * then the byte cleared — the leaf's mode-0 half already clears. */
    terrain_cell_surface_appearance_apply(terrain, x, y, 0, 1.0f, 1.0f,
                                          1.0f);
}

/* --- coordinate-callback dispatchers (see level.hpp) -------------------------
 * The entity-pool callback registry (DAT_004b31c8 band) and the fixed
 * code-literal callbacks are unported seams, like the media mode-leaf's
 * hooks — the observable port state here is the refresh dispatch's
 * terrain half (the surface-type clear). */
void terrain_cell_refresh_dispatch(const struct GameContext* ctx, int x,
                                   int y)
{
    const LevelState* lvl = (ctx != NULL) ? ctx->level : NULL;
    /* Corpus: CurrentLevelGet (port: GameContext.level -> +0x04 terrain
     * handle); a missing level or terrain skips (deviation). */
    if (lvl == NULL || lvl->ptr_terrain == NULL) {
        return;
    }
    /* Coordinate callback dispatch through EntityPoolCallbackSearch:
     * the object-record pool scan is the seam half. */
    terrain_cell_surface_type_reset(lvl->ptr_terrain, x, y);
}

void coordinate_callback_dispatch(const void* coord_pair, int value)
{
    (void)coord_pair;
    (void)value;
    /* Seam: pair the coordinate word with zero and traverse the entity
     * pool with the fixed callback — the callback machinery is the
     * unported entity seam above. */
}

void coordinate_callback_dispatch_by_mode(const void* coord_pair, int mode)
{
    (void)coord_pair;
    (void)mode;
    /* Seam: same pair context, callback chosen by the mode word. */
}

/* --- the global-object field band (see level.hpp) ---------------------------
 * The corpus's `DAT_005560d4 != 0` guard lands as the port's NULL-level
 * guard; every other detail is the corpus's own arithmetic. */
void global_object_field8c_increment(LevelState* s)
{
    if (s == NULL) {
        return;
    }
    level_set_int(s, 0x8C, level_int(s, 0x8C) + 1);
}

void global_object_field8c_subtract(LevelState* s, int amount)
{
    if (s == NULL) {
        return;
    }
    level_set_int(s, 0x8C, level_int(s, 0x8C) - amount);
}

void global_object_field98_add(LevelState* s, int amount)
{
    if (s == NULL) {
        return;
    }
    level_set_int(s, 0x98, level_int(s, 0x98) + amount);
}

void global_object_field98_clear(LevelState* s)
{
    if (s != NULL) {
        level_set_int(s, 0x98, 0);
    }
}

void level_state_field9c_add(LevelState* s, int amount)
{
    if (s == NULL) {
        return;
    }
    level_set_int(s, 0x9C, level_int(s, 0x9C) + amount);
}

void global_object_field94_increment(LevelState* s)
{
    if (s == NULL) {
        return;
    }
    level_set_int(s, 0x94, level_int(s, 0x94) + 1);
}

void global_object_fielda8_increment(LevelState* s)
{
    if (s == NULL) {
        return;
    }
    level_set_int(s, 0xA8, level_int(s, 0xA8) + 1);
}

uint32_t level_field8c_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0x8C) : 0u;
}

uint32_t level_field98_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0x98) : 0u;
}

uint32_t level_field9c_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0x9C) : 0u;
}

uint32_t level_field94_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0x94) : 0u;
}

uint32_t level_fielda8_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, 0xA8) : 0u;
}

/* GlobalOptionLowBitsEnabled (FUN_004589a0): faithful gate over the
 * objective word's low three bits. */
int global_option_low_bits_enabled(const LevelState* s)
{
    return s != NULL && (s->objective & 7u) != 0u;
}

/* GlobalFlagGet_004ffe30 (FUN_00458650): threaded objective-result read. */
int level_objective_result_get(const LevelState* s)
{
    return s != NULL ? s->objective_result : 0;
}

/* TerrainObjectTypeSfxAtCellGet (FUN_0044ad80) — see level.hpp. */
#define TERRAIN_SFX_GRID_CELLS 300
static int s_terrain_sfx_grid[TERRAIN_SFX_GRID_CELLS]; /* DAT_004de814 role */

int terrain_object_type_sfx_at_cell_get(const void* position_record)
{
    uint32_t x = 0, y = 0;
    if (position_record == NULL) {
        return 0; /* deviation: the corpus strides blindly */
    }
    __builtin_memcpy(&x, (const char*)position_record, 4);
    __builtin_memcpy(&y, (const char*)position_record + 4, 4);
    const int cell = (int)(y + x * 15u); /* corpus's second-word-first index */
    if (cell < 0 || cell >= TERRAIN_SFX_GRID_CELLS) {
        return 0;
    }
    return s_terrain_sfx_grid[cell];
}

void terrain_sfx_grid_cell_set(int cell, int handle)
{
    if (cell >= 0 && cell < TERRAIN_SFX_GRID_CELLS) {
        s_terrain_sfx_grid[cell] = handle;
    }
}

int terrain_sfx_grid_cell_peek(int cell)
{
    if (cell < 0 || cell >= TERRAIN_SFX_GRID_CELLS) {
        return 0;
    }
    return s_terrain_sfx_grid[cell];
}

/* TerrainObjectTypeSfxGridConfigLoad (FUN_0044acc0) — see level.hpp.
 * Deviations: the corpus strides its zero loop blindly (the port memsets
 * the window); the corpus's two dead preamble loops leave the running
 * pair initialized 20/15 and carry them ACROSS entries, so a name whose
 * resolution writes only one half inherits the other from the previous
 * entry (pinned quirk — the port carries them the same way); the corpus
 * strides a missing value string into the alias table (the port's lookup
 * guards NULL) and stores blind past the 300-entry window (the port
 * guards). The entry chain is the subgroup record followed by its
 * equal-key siblings, entry name = the object name, entry value = the
 * sound alias reference. */
void terrain_object_sfx_grid_config_load(BlockFile* config,
                                         const char* root_path)
{
    memset(s_terrain_sfx_grid, 0, sizeof s_terrain_sfx_grid);
    int obj_x = 20; /* the corpus's dead-loop pair, carried across entries */
    int obj_y = 15;
    const char* path = string_path_join(root_path, "S_ObjTtSFXs", NULL);
    for (BlockRec* entry = block_subgroup(config, path);
         entry != NULL; entry = block_next_equal_key(entry)) {
        if (entity_type_row_for_name(block_name(entry), &obj_x,
                                     &obj_y) == 0) {
            continue;
        }
        int sfx = 0;
        if (sound_alias_lookup(block_value(entry), &sfx) == 0) {
            continue;
        }
        const int cell = obj_y + obj_x * 15;
        if (cell >= 0 && cell < TERRAIN_SFX_GRID_CELLS) {
            s_terrain_sfx_grid[cell] = sfx;
        }
    }
}

/* --- terrain choice resource select (2026-10-25) ---------------------------
 * TerrainChoiceResourceSelect (FUN_0040e700): copies the position record's
 * pair (the ported pair primitive; the corpus's second read overwrites its
 * own local while the saved original rides the last-record slot), then the
 * record's first word selects the active resource word: cases 1..4 read the
 * choice window (the corpus's four column bases are overlapping strides of
 * one flat word window, word = kind-1 + index, indexed by the record's
 * second word), cases 6/7 read the two scalar words, and everything else
 * clears the flag's bit 0 and stores nothing. A successful select raises
 * the flag's bits 0+1, keeps the record pointer in the last-select slot,
 * and releases the panel's text-output holder slot. Deviations: the corpus
 * strides the window indices blindly — the port guards the window edge and
 * answers clear; the window and scalars have no ported corpus owner, so
 * they ride zero-init own storage behind install hooks. */
static uint32_t s_choice_window[4];  /* DAT_004c7e88..e94 role */
static uint32_t s_choice_scalar[2];  /* DAT_004c7e98/e9c role */
static uint32_t s_choice_active;     /* DAT_004c7ea0 role */
static uint32_t s_choice_flags;      /* DAT_004c7ea8 role */
static uintptr_t s_choice_record;    /* DAT_004c7ea4 role */

void terrain_choice_resource_select(const void* record)
{
    uint32_t kind = 0u, index = 0u;
    copy_two_dword_pair_to_outputs(record, &kind, &index);
    switch (kind) {
    case 1u: case 2u: case 3u: case 4u: {
        const unsigned int word = (kind - 1u) + index;
        s_choice_active = (word < 4u) ? s_choice_window[word] : 0u;
        break;
    }
    case 6u: s_choice_active = s_choice_scalar[0]; break;
    case 7u: s_choice_active = s_choice_scalar[1]; break;
    default:
        s_choice_active = 0u;
        s_choice_flags &= ~1u;
        return;
    }
    s_choice_flags |= 3u;
    s_choice_record = (uintptr_t)record;
    ui_text_output_object_release((void* const*)panel_text_slot_peek());
}

void terrain_choice_window_install(int word, uint32_t value)
{
    if (word >= 0 && word < 4) {
        s_choice_window[word] = value;
    }
}

uint32_t terrain_choice_window_peek(int word)
{
    return (word >= 0 && word < 4) ? s_choice_window[word] : 0u;
}

void terrain_choice_scalar_install(int scalar, uint32_t value)
{
    if (scalar >= 0 && scalar < 2) {
        s_choice_scalar[scalar] = value;
    }
}

uint32_t terrain_choice_active_peek(void) { return s_choice_active; }
uint32_t terrain_choice_flags_peek(void) { return s_choice_flags; }
uintptr_t terrain_choice_record_peek(void) { return s_choice_record; }

/* EntityAvailableResourceSpend (FUN_00438720) — see level.hpp.
 * The corpus's comparison value is the unconsumed EAX of the void no-op
 * callback; pinning it to 0 matches the corpus call's observable shape. */
int entity_available_resource_spend(LevelState* level, int amount,
                                    const void* resource_hint)
{
    if (level != NULL && level_int(level, 0x1f8) != 0) {
        amount = 0;
    }
    const int probe = 0; /* FUN_0041f8e0's void call: pinned residue */
    if (probe < amount) {
        message_dispatch(0xe, 0, 0, resource_hint);
        return 0;
    }
    global_object_field98_add(level, amount);
    return 1;
}

/* CellCardinalNeighborApply (FUN_00409cd0) — see level.hpp. The corpus's
 * offset pairs, in its loop order: north, east, south, west. */
void cell_cardinal_neighbor_apply(const struct GameContext* ctx, int x,
                                  int y)
{
    static const int k_offsets[8] = { 0, -1, 1, 0, 0, 1, -1, 0 };
    for (int i = 0; i < 4; ++i) {
        terrain_cell_refresh_dispatch(ctx, x + k_offsets[2 * i],
                                      y + k_offsets[2 * i + 1]);
    }
}

/* TerrainCellSpecialStatePredicate (FUN_0040d3f0) — see level.hpp. */
/* CellVariantEntryMatchCheck (FUN_00451180) — see level.hpp. The
 * coordinate pair names two cells through the same window: the block
 * table stride is the window's own dword at + 0x08. */
int cell_variant_entry_match_check(const void* grid, uint32_t x1, uint32_t y1,
                                   uint32_t x2, uint32_t y2)
{
    if (grid == NULL) {
        return 0; /* deviation: the corpus strides the window blindly */
    }
    const unsigned char* w = (const unsigned char*)grid;
    if ((y1 != y2 || x1 != x2) != 0) {
        uint32_t width, height, stride;
        __builtin_memcpy(&width, w + 0x00, sizeof width);
        __builtin_memcpy(&height, w + 0x04, sizeof height);
        __builtin_memcpy(&stride, w + 0x08, sizeof stride);
        if (x1 < width && y1 < height && x2 < width && y2 < height) {
            void* blocks = NULL;
            __builtin_memcpy(&blocks, w + 0x28, sizeof blocks);
            if (blocks == NULL) {
                return 0; /* deviation: the corpus strides the base */
            }
            const unsigned char* base = (const unsigned char*)blocks + 0x18;
            const size_t i1 = ((size_t)y1 * stride + x1) * 0x28;
            const size_t i2 = ((size_t)y2 * stride + x2) * 0x28;
            const uint32_t v1 = base[i1];
            const uint32_t v2 = base[i2];
            if (v1 != 0xffu && v2 != 0xffu) {
                const size_t cnt = w[0x4034u + v1];
                for (size_t i = 0; i < cnt; ++i) {
                    if (w[0x34u + (size_t)v1 * 0x80u + i] == (unsigned char)v2) {
                        return 1;
                    }
                }
                return 0; /* corpus: the list ran out (and count 0) */
            }
        }
    }
    return 0;
}

int terrain_cell_special_state_predicate(const LevelState* s, int cx,
                                         int cy, int secondary_gate)
{
    if (level_cell_occupied_or_flag(s, cx, cy, 1) == 0u) {
        return 0;
    }
    if (secondary_gate == 0) {
        return 1;
    }
    return level_cell_secondary_flag_100(s, cx, cy) != 0u ? 1 : 0;
}

/* --- the path record window band (2026-10-25) ----------------------------- */

/* PathRecordAllocateAndLink (FUN_00409350): allocates the corpus's 0xd4
 * word window (53 dwords), zero-fills it the corpus way, stamps the
 * serial counter store at +0x08, copies the source coordinate pair to
 * +0x0c/+0x10, and links the previous most-recent record (corpus dword
 * slot 9). Deviations: the window is widened past the corpus stride so
 * the link holds a port pointer (port-layout convention), the corpus's
 * belt-and-braces zero stores land inside the zero-fill, and the
 * allocator's failure word is guarded. */

#define PATH_RECORD_WINDOW 0xd4 /* the corpus's 53-dword window           */
#define PATH_RECORD_WORDS (PATH_RECORD_WINDOW / 4u)

static uint32_t s_path_record_count; /* DAT_004b8a20 role */
static void* s_path_record_last;     /* DAT_004b8a24 role */

void* path_record_allocate_and_link(const uint32_t* coord_pair)
{
    const size_t bytes =
        PATH_RECORD_WINDOW + sizeof(void*) - sizeof(uint32_t);
    char* rec = (char*)engine_heap_alloc((long)bytes);
    if (rec == NULL) {
        return NULL; /* deviation: the corpus strides the allocator's word */
    }
    const uint32_t zero = 0u;
    for (size_t n = 0; n < bytes / sizeof(uint32_t); ++n) {
        __builtin_memcpy(rec + n * sizeof(uint32_t), &zero, sizeof zero);
    }
    uint32_t serial = s_path_record_count++;
    __builtin_memcpy(rec + 0x08, &serial, sizeof serial);   /* counter store */
    if (coord_pair != NULL) {
        __builtin_memcpy(rec + 0x0c, &coord_pair[0], sizeof(uint32_t));
        __builtin_memcpy(rec + 0x10, &coord_pair[1], sizeof(uint32_t));
    }
    __builtin_memcpy(rec + 0x24, &s_path_record_last,
                     sizeof(s_path_record_last));           /* prev link */
    s_path_record_last = rec;
    return rec;
}

uint32_t path_record_count_peek(void) { return s_path_record_count; }
void* path_record_last_peek(void) { return s_path_record_last; }

/* Test hook: stages a count-table slot (the corpus's table writers are
 * unported, so the faithful read shape is exercised through this hook —
 * the install-hook precedent). */
void grid_count_table_slot_poke(int row, int column, int slot, int table,
                                uint32_t value)
{
    const long long idx = grid_count_index(row, column, slot);
    if (idx < 0) {
        return; /* the bounds deviation rides through */
    }
    if (table != 0) {
        s_grid_count_second[(size_t)idx] = value;
    } else {
        s_grid_count_first[(size_t)idx] = value;
    }
}

/* TerrainCellQueuedWorkTransition (FUN_00431140) — see level.hpp. The
 * corpus indexes its cell records through the global level word; the port
 * threads GameContext.level and guards the NULL. The spawn-point pass the
 * corpus runs through its coordinate-callback hook (the port's registered
 * spawn seam) is quoted here for the dispatch order. */
void terrain_cell_queued_work_transition(const struct GameContext* ctx,
                                         int x, int y)
{
    const LevelState* lvl = (ctx != NULL) ? ctx->level : NULL;
    if (lvl == NULL) {
        return;
    }
    const uint32_t flags = cell_flag_word(lvl, x, y);
    if ((flags & 0x10u) == 0u || (flags & 0xe0u) != 0u) {
        return;
    }
    uint8_t* rec = level_voxel(lvl, x, y);
    if (rec == NULL) {
        return; /* deviation: the corpus strides OOB cells blindly */
    }
    uint32_t* word = (uint32_t*)(rec + 0x08);
    *word = *word | 0x20u;

    terrain_cell_refresh_dispatch(ctx, x, y);   /* 0x42f6f0 pair */
    const int pair[2] = { x, y };
    coordinate_callback_dispatch(pair, 0);      /* 0x402b30 seam */
    /* Hook seam FUN_004311d0: the corpus coordinate callback for the
     * queued cell (the spawn-point pass, port-side hook). */
    message_dispatch(0x1c, 0, 0, pair);         /* 0x419b80 */
}

/* RandomVariantCellScatter (FUN_00435230) — see level.hpp. Draws ride the
 * module's corpus-RNG model (the shared 15-bit word thunk). */
void random_variant_cell_scatter(int center_x, int center_y, int radius,
                                 int mode)
{
    int dedup[100];
    for (int i = 0; i < 100; ++i) {
        dedup[i] = 0;
    }
    if (radius <= 0) {
        return; /* deviation: the corpus would zero-modulo every draw */
    }
    const unsigned int diam = (unsigned int)(radius * 2);
    unsigned int limit = diam * diam;
    if (mode == 0) {
        limit >>= 2;
    }
    for (unsigned int n = 0; n < limit; ++n) {
        const unsigned int dx =
            (unsigned int)level_random_draw15() % diam;
        const unsigned int dy =
            (unsigned int)level_random_draw15() % diam;
        const unsigned int key = dy + dx * 10u;
        if (key < 100u && dedup[key] == 0) {
            const int cx = (int)((unsigned)center_x + dx) - radius;
            const int cy = (int)((unsigned)center_y + dy) - radius;
            /* Grid-spawn route (FUN_0040f190 quote-only seam): never
             * lands, so the corpus's mode-nonzero early exit cannot
             * fire; the cell pair and mode 1 would ride the call. */
            (void)cx;
            (void)cy;
            const int spawn = 0;
            if (spawn != 0 && mode != 0) {
                return;
            }
            dedup[key] = 1;
        }
    }
}

/* RandomThunkDraw15 (thunk_FUN_0048e170 at FUN_004798a0): the corpus's
 * RNG-thunk body — the corpus inlines the 15-bit draw over DAT_004afcc8 in
 * both thunk copies, sharing one state; the port forwards to the single
 * module RNG model (level_random_draw15, thunk_FUN_0048e170 / 0x48e170). */
uint32_t rng_thunk_draw15(void)
{
    return (uint32_t)level_random_draw15();
}

/* --- erosion timing band (2026-10-25) -------------------------------------
 * The timing floats, the clear word, and the two scratch regions land as
 * module statics (the unowned-global deviation; the corpus's scratch
 * consumers are the hook-stubbed tickers FUN_0040e7f0 / FUN_0040e990).
 * _DAT_004d78c0 rides the existing timestamp tick model (s_ts_now). */
#define EROSION_MARK_COUNT 2000   /* DAT_004cfbb4 */
#define EROSION_TICKET_COUNT 1000 /* DAT_004d6914 */

static float s_erosion_decay_scale;    /* _DAT_004d78b8 */
static float s_erosion_delay_scale;    /* _DAT_004d78bc */
static uint32_t s_erosion_clear_word;  /* DAT_004cbd30 */
static uint32_t s_erosion_marks[EROSION_MARK_COUNT];
static uint32_t s_erosion_tickets[EROSION_TICKET_COUNT];

void erosion_timing_state_init(float value_a, float value_b, float value_c)
{
    s_erosion_decay_scale = value_a * 25.0f;
    s_erosion_delay_scale = value_b * 25.0f * 0.2f;
    const float tick = value_c * 25.0f;
    __builtin_memcpy(&s_ts_now, &tick, sizeof s_ts_now);
    /* deviation: the corpus keeps the seed as a float word; the port's
     * timestamp model reads the tick as a count, so the float bits ride. */
    s_erosion_clear_word = 0u;
    memset(s_erosion_marks, 0, sizeof s_erosion_marks);
    memset(s_erosion_tickets, 0, sizeof s_erosion_tickets);
}

float erosion_decay_scale_peek(void) { return s_erosion_decay_scale; }
float erosion_delay_scale_peek(void) { return s_erosion_delay_scale; }
int erosion_timestamp_tick_peek(void) { return s_ts_now; }

int erosion_scratch_peek(int which, int index)
{
    if (which == 0) {
        return (index >= 0 && index < EROSION_MARK_COUNT)
                   ? (int)s_erosion_marks[index] : -1;
    }
    return (index >= 0 && index < EROSION_TICKET_COUNT)
               ? (int)s_erosion_tickets[index] : -1;
}

float lava_erosion_delay_from_cell(const LevelState* s, int x, int y)
{
    const uint8_t* rec = level_voxel(s, x, y);
    if (rec == NULL) {
        return 0.0f; /* deviation: the corpus strides the grid blindly */
    }
    const int steps = 6 - (int)rec[6];
    return (float)steps * s_erosion_delay_scale * 25.0f;
}

int lava_erosion_neighbor_check(const LevelState* s, int seed_x, int seed_y,
                                int out_xy[2])
{
    const uint8_t* seed = level_voxel(s, seed_x, seed_y);
    if (seed == NULL || seed[2] != 6) {
        return 0; /* deviation: guarded seed (corpus strides blindly) */
    }
    /* Corpus neighbour deltas; the first x-dword is uninitialized stack
     * residue, pinned to 0 (the cardinal ring). */
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    for (int i = 0; i != 4; ++i) {
        const int nx = seed_x + dx[i];
        const int ny = seed_y + dy[i];
        if (out_xy != NULL) {
            out_xy[0] = nx;
            out_xy[1] = ny;
        }
        const uint8_t* rec = level_voxel(s, nx, ny);
        if (rec == NULL) {
            continue; /* deviation: out-of-bounds neighbour (corpus strides) */
        }
        uint32_t flags = 0;
        __builtin_memcpy(&flags, rec + 8, sizeof flags);
        if ((flags & 0x80000000u) == 0 && (flags & 8u) != 0 &&
            rec[6] != 0) {
            if (cell_coordinate_timestamp_exists(nx, ny) == 0u) {
                return 1;
            }
        }
    }
    return 0;
}

/* --- the cell route + neighbour pick band (2026-10-25) -----------------------
 * TerrainCellRouteAndOccupancyCheck (FUN_00431da0) answers whether an
 * entity may move from cell (from_x, from_y) to (to_x, to_y). The corpus
 * indexes a cardinal delta table with a dword walker, so a neighbour hit
 * in slot 0 reads the same as "no cardinal neighbour" (pinned quirk),
 * and the entity's own tile position rides the port's cell-round seam
 * (level_cell_of on the entity's float pair — same precedent as the
 * idle-registration tile probe). Deviations: every cell read routes
 * through level_voxel (the corpus strides the base + y*w+x * 0x48 window
 * without a bounds check; out-of-range reads answer zero), a NULL level
 * answers through the same guards, and the entity cell check lands after
 * the corpus's own stats gates.
 *
 * FirstFreeNeighborCellPick (FUN_00431c70) walks the four cardinal
 * neighbours in a rotation that advances by the corpus's DAT_004de588
 * pick counter (own storage here; peek hook), calling the route check
 * with source and destination pinned to the neighbour cell. In mode 1 a
 * neighbour that is in bounds, open (flag 0x10) and unblocked (flag 8
 * clear) is skipped; otherwise it is tried. Deviation: the corpus
 * reloads the level global between probes — the port reuses one pointer.
 * Both rows guard NULL entity/level where the corpus strides blindly. */
static const int s_cardinal_deltas[4][2] = {{0, -1}, {1, 0}, {0, 1},
                                            {-1, 0}};
static uint32_t s_neighbor_pick_rotation; /* DAT_004de588 role */

uint32_t neighbor_pick_rotation_peek(void) { return s_neighbor_pick_rotation; }

static int route_cell_byte(const LevelState* s, int x, int y, int off)
{
    const uint8_t* rec = level_voxel(s, x, y);
    return rec != NULL ? (int)(signed char)rec[off] : 0;
}

static uint32_t route_cell_flags(const LevelState* s, int x, int y)
{
    const uint8_t* rec = level_voxel(s, x, y);
    uint32_t flags = 0u;
    if (rec != NULL) {
        __builtin_memcpy(&flags, rec + 0x08, sizeof flags);
    }
    return flags;
}

int terrain_cell_route_and_occupancy_check(const struct EntitySlot* entity,
                                           const LevelState* s,
                                           int from_x, int from_y,
                                           int to_x, int to_y, int strict)
{
    if (entity != NULL
        && (entity_stats_word_get(entity) & 0x20000u) != 0u
        && entity->owner_36c == NULL) {
        return 0;
    }

    /* Cardinal delta walk; 4 marks "no hit" (the corpus's walker dword
     * ends past the table). Slot 0 hits as delta_index 0 — the corpus's
     * "param_1 == 0" gate quirk. */
    int delta_index = 4;
    for (int i = 0; i != 4; ++i) {
        if (s_cardinal_deltas[i][0] + to_x == from_x
            && to_y + s_cardinal_deltas[i][1] == from_y) {
            delta_index = i;
            break;
        }
    }

    uint32_t block_relief = 0u; /* corpus +14c word & 0x200 */
    uint32_t special = 0u;      /* stats & 1 */
    uint32_t idle_gate = 0u;    /* stats & 0x1000000 */
    uint32_t walk_gate = 1u;    /* stats & 0x2000000 (initial pass) */
    uint32_t state_mask = 0u;   /* state & 0x200000 */
    if (entity != NULL) {
        if (entity->type == 0xb) {
            return 0;
        }
        block_relief = entity_capability_flags_get(entity) & 0x200u;
        const uint32_t stats = entity_stats_word_get(entity);
        special = stats & 1u;
        idle_gate = stats & 0x1000000u;
        walk_gate = stats & 0x2000000u;
        state_mask = entity_state_flags_get(entity) & 0x200000u;
        if (strict == 0) {
            /* EntityPositionToTerrainCell seam pass: the entity's own
             * cell (level_cell_of round of the float pair) standing on
             * (from_x, to_y) answers a straight yes. */
            if (level_cell_of(entity->x) == from_x
                && level_cell_of(entity->y) == to_y) {
                return 1;
            }
        }
    }

    const int type = route_cell_byte(s, from_x, from_y, 0x02);
    if (type == 9) {
        if (idle_gate == 0u) {
            return 0;
        }
    } else if (walk_gate == 0u) {
        return 0;
    }
    if (type == 6 && state_mask == 0u
        && (route_cell_flags(s, from_x, from_y) & 0x200u) == 0u) {
        return 0;
    }

    const uint32_t flags = route_cell_flags(s, from_x, from_y);
    if ((flags & 0x400u) != 0u) {
        return 0;
    }
    if (strict != 0 && (flags & 0x40u) != 0u) {
        return 0;
    }
    if (block_relief == 0u && (route_cell_byte(s, from_x, from_y, 0x0c) & 4) != 0) {
        return 0;
    }

    int answer = 1;
    if (special == 0u) {
        if ((flags & 8u) == 0u) {
            return 0;
        }
    } else {
        if ((flags & 0x10u) != 0u) {
            answer = 3; /* the corpus promotes the answer on an open cell */
        }
        if (route_cell_byte(s, from_x, from_y, 0x00) == 0) {
            return 0; /* *local_28 == 0: the cell's state byte reads unset */
        }
        const uint32_t corner_flags = route_cell_flags(s, from_x, to_y);
        if ((corner_flags & 0x10u) != 0u) {
            const int above = route_cell_byte(s, from_x, to_y - 1, 0x02);
            if (above == 6 || above == 7) {
                return 0; /* the wall-cell pair above the corner */
            }
        }
        if (delta_index == 0) {
            if ((flags & 0x2000u) != 0u) {
                answer = 2;
            }
            if ((route_cell_flags(s, to_x, to_y) & 0x2000u) != 0u) {
                answer = 2;
            }
        }
    }

    if (to_x != from_x && to_y != from_y) {
        /* diagonal corner pair along the destination row */
        if ((route_cell_flags(s, from_x, to_y) & 8u) != 0u
            && (route_cell_flags(s, to_x, to_y) & 8u) != 0u) {
            return 2;
        }
    }
    return answer;
}

int first_free_neighbor_cell_pick(const struct EntitySlot* entity,
                                  const LevelState* s, const int* cell_in,
                                  int* cell_out, int mode)
{
    ++s_neighbor_pick_rotation;
    const int x = cell_in[0], y = cell_in[1];
    for (int i = 0; i != 4; ++i) {
        const uint32_t k
            = (s_neighbor_pick_rotation + (uint32_t)i) & 3u;
        const int nx = s_cardinal_deltas[k][0] + x;
        const int ny = s_cardinal_deltas[k][1] + y;
        if (mode != 0) {
            const uint32_t flags = route_cell_flags(s, nx, ny);
            if ((unsigned)nx < (unsigned)level_width(s) - 1u
                && (unsigned)ny < (unsigned)level_height(s) - 1u
                && (flags & 0x10u) != 0u && (flags & 8u) == 0u) {
                continue; /* corpus bVar2: already-open cell is skipped */
            }
        }
        if (terrain_cell_route_and_occupancy_check(entity, s, nx, ny, nx,
                                                   ny, 0) != 0) {
            cell_out[0] = nx;
            cell_out[1] = ny;
            return 1;
        }
    }
    return 0;
}
