// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/resource.hpp"
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

static uint16_t level_rand_u16(void)
{
    s_rand_state = s_rand_state * 214013u + 2531011u;
    return (uint16_t)((s_rand_state >> 16) & 0x7fffu);
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

TerrainState* terrain_create(const char* surface_map, float blocksize)
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
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
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
    for (int y = 0; y + 1 < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
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

void terrain_destroy(TerrainState* t)
{
    if (t != NULL) {
        free(t->blocks);
        free(t);
    }
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
    TerrainState* terrain = terrain_create(names[0], level_float(s, LEVEL_OFF_BLOCKSIZE));
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
    terrain_destroy((TerrainState*)s->ptr_terrain);
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

int level_tile_type(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (int)(signed char)rec[2] : -1;
}
