// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/record.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/string.hpp"
#include "crt/crt.hpp"
#include "gods98/view.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "entity.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"
#include "gods98/flic.hpp"
#include "terrain_grid.hpp"
#include "select.hpp"

/* ------------------------------------------------------------------ *
 *  Corpus .rdata constants (identities from finding 07/22 and the
 *  decompile corpus; numeric values are best-effort re-derivations — the
 *  shipped data section is under copyright and not reproduced here).
 * ------------------------------------------------------------------ */

#define LVL_SEC_TO_TICK   25.0f       /* _DAT_0049e2b4: 25 game ticks/second */
#define LVL_OXYGEN_SCALE  1.0f        /* _DAT_0049e324: (unconfirmed scale)  */
#define LVL_RGB_SCALE     0.0039215686f /* _DAT_0049e328: 1/255            */

/* The reference overlay owner is the indexed-surface state at DAT_00552180,
 * DAT_00552184/88, and DAT_00552198. This reconstruction does not yet create
 * or retain that owner from the level/render startup path, so keep the reader
 * explicit instead of pretending that TerrainState owns the render quads.
 * Once the backend binds the indexed surface, the normal helpers below read
 * and update the actual quad records supplied by that owner. */
static void* s_terrain_quad_surface;
static uint32_t s_terrain_quad_width;
static uint32_t s_terrain_quad_height;
static TerrainQuadVertexReadHook s_terrain_quad_vertex_read;
static void* s_terrain_quad_vertex_context;

void terrain_quad_vertex_source_set(void* surface, uint32_t width,
                                    uint32_t height,
                                    TerrainQuadVertexReadHook hook,
                                    void* context)
{
    s_terrain_quad_surface = surface;
    s_terrain_quad_width = width;
    s_terrain_quad_height = height;
    s_terrain_quad_vertex_read = hook;
    s_terrain_quad_vertex_context = context;
}

// FUNCTION: LEGORR 0x00463590
int terrain_cell_surface_normal_get(uint32_t x, uint32_t y, float out[3])
{
    float vertices[4][3];
    float first[3];
    float second[3];
    float nx, ny, nz, inverse_length;
    size_t index;
    if (out == NULL || x >= s_terrain_quad_width || y >= s_terrain_quad_height
        || s_terrain_quad_surface == NULL
        || s_terrain_quad_vertex_read == NULL) {
        return 0;
    }
    index = (size_t)y * (size_t)s_terrain_quad_width + (size_t)x;
    if (!s_terrain_quad_vertex_read(s_terrain_quad_vertex_context,
                                    s_terrain_quad_surface, index, vertices)) {
        return 0;
    }
    /* The polygon reader returns the four points in the order consumed by
     * FUN_00463590: triangles (0,1,2) and (2,3,0). */
    surface_triangle_normal_calculate(first, vertices[0], vertices[1],
                                      vertices[2]);
    surface_triangle_normal_calculate(second, vertices[2], vertices[3],
                                      vertices[0]);
    nx = first[0] + second[0];
    ny = first[1] + second[1];
    nz = first[2] + second[2];
    inverse_length = 1.0f / (float)sqrt((double)(nx * nx + ny * ny + nz * nz));
    /* The game's output frame swaps the second and third normal components. */
    out[0] = nx * inverse_length;
    out[1] = nz * inverse_length;
    out[2] = ny * inverse_length;
    return 1;
}

void terrain_dirty_quad_normal_emit_pass(TerrainDirtyQuadNormalPass* pass)
{
    static const int neighbor_offsets[4][4][2] = {
        { { 0, 0 }, { -1, 0 }, { 0, 1 }, { -1, 1 } },
        { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } },
        { { 0, 0 }, { 0, -1 }, { 1, -1 }, { 1, 0 } },
        { { 0, 0 }, { -1, -1 }, { 0, -1 }, { -1, 0 } }
    };
    uint32_t y;
    if (pass == NULL || pass->records == NULL || pass->dirty == NULL ||
        pass->emit_normals == NULL || *pass->dirty == 0u) return;
    *pass->dirty = 0u;
    for (y = 0; y < pass->height; ++y) {
        uint32_t x;
        for (x = 0; x < pass->width; ++x) {
            uint32_t index = y * pass->width + x;
            uint8_t* record = pass->records + index * 8u;
            float normals[4][3];
            uint32_t corner;
            int component;
            if ((record[4] & 2u) == 0u) continue;
            for (corner = 0; corner < 4u; ++corner) {
                unsigned int table_corner = (record[4] & 4u) != 0u
                    ? ((corner - 1u) & 3u) : corner;
                unsigned int neighbor;
                float length;
                for (component = 0; component < 3; ++component)
                    normals[corner][component] = 0.0f;
                for (neighbor = 0; neighbor < 4u; ++neighbor) {
                    const int nx = (int)x +
                        neighbor_offsets[table_corner][neighbor][0];
                    const int ny = (int)y +
                        neighbor_offsets[table_corner][neighbor][1];
                    float normal[3];
                    if (nx >= 0 && ny >= 0 &&
                        terrain_cell_surface_normal_get((uint32_t)nx,
                                                        (uint32_t)ny,
                                                        normal)) {
                        normals[corner][0] += normal[0];
                        normals[corner][1] += normal[1];
                        normals[corner][2] += normal[2];
                    }
                }
                length = (float)sqrt((double)(
                    normals[corner][0] * normals[corner][0] +
                    normals[corner][1] * normals[corner][1] +
                    normals[corner][2] * normals[corner][2]));
                length = 1.0f / length;
                normals[corner][0] *= length;
                normals[corner][1] *= length;
                normals[corner][2] *= length;
            }
            pass->emit_normals(pass->context, index, normals);
            record[4] = (uint8_t)(record[4] & ~2u);
        }
    }
}

void terrain_cell_quad_geometry_emit(TerrainCellQuadEmitGrid* grid,
                                     uint32_t x, uint32_t y,
                                     const float vertices[4][3])
{
    static const uint32_t triangle_indices[6] = {
        0u, 1u, 3u, 1u, 2u, 3u
    };
    uint32_t index;
    uint8_t* record;
    uint8_t flags;
    int quad;
    int flipped;
    float ordered[4][3];
    uint32_t i;
    if (grid == NULL || vertices == NULL || grid->records == NULL ||
        x >= grid->width || y >= grid->height) return;
    index = y * grid->width + x;
    record = grid->records + index * 8u;
    flags = record[4];
    quad = (int)*(uint32_t*)record;
    if ((flags & 1u) != 0u) {
        if (grid->hooks.create_quad == NULL) return;
        quad = grid->hooks.create_quad(grid->hooks.context, index,
                                       triangle_indices);
        *(uint32_t*)record = (uint32_t)quad;
        if (quad != -1) {
            if (grid->hooks.quad_position_apply != NULL)
                grid->hooks.quad_position_apply(grid->hooks.context, quad,
                    grid->position_parameter);
            if (grid->hooks.quad_post_create_update != NULL)
                grid->hooks.quad_post_create_update(grid->hooks.context,
                                                     quad, 1);
            if (grid->hooks.visibility_set != NULL)
                grid->hooks.visibility_set(grid->hooks.context, quad, 1);
            flags = (uint8_t)(flags & ~1u);
        }
    }
    if (grid->dirty != NULL) *grid->dirty = 1u;
    flags = (uint8_t)(flags | 2u);
    flipped = !((vertices[2][2] <= vertices[0][2]
                 ? vertices[2][2] : vertices[0][2]) <= vertices[1][2] &&
                (vertices[2][2] <= vertices[0][2]
                 ? vertices[2][2] : vertices[0][2]) <= vertices[3][2]);
    if (flipped) {
        const uint32_t order[4] = { 1u, 2u, 3u, 0u };
        for (i = 0; i < 4u; ++i) {
            ordered[i][0] = vertices[order[i]][0];
            ordered[i][1] = vertices[order[i]][1];
            ordered[i][2] = vertices[order[i]][2];
        }
        flags = (uint8_t)(flags | 4u);
    }
    else {
        for (i = 0; i < 4u; ++i) {
            ordered[i][0] = vertices[i][0];
            ordered[i][1] = vertices[i][1];
            ordered[i][2] = vertices[i][2];
        }
        flags = (uint8_t)(flags & ~4u);
    }
    record[4] = flags;
    if (quad != -1 && grid->hooks.vertices_emit != NULL)
        grid->hooks.vertices_emit(grid->hooks.context, quad, ordered);
}

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
static const char* K_ERODETRIGGER   = "ErodeTriggerTime";
static const char* K_ERODETIME      = "ErodeErodeTime";
static const char* K_ERODELOCK      = "ErodeLockTime";
static const char* K_LANDSLIDECOUNT = "NumberOfLandSlidesTillCaveIn";

/* Cheat-word bits (DAT_00556ed0) set by LevelLoad (finding 07/config): kept
 * accessible for the frame loop but not written by the load itself (the
 * corpus's DAT_00556ed0 is modelled in GameContext). */
#define LVL_OFF_BOULDER_ANIM 0x274   /* +0x9d */
#define LVL_OFF_INIT_CRYS    0x130   /* +0x4c scratch from post-load init */
#define LVL_OFF_INIT_ORE     0x134   /* +0x4d scratch from post-load init */

/* ------------------------------------------------------------------ *
 *  PRNG — VC5 LIBC.LIB rand.obj supplies the target's shared _srand/_rand
 *  entries and initial seed word (DAT_004afcc8 = 1). Portable builds retain
 *  the equivalent local LCG.
 * ------------------------------------------------------------------ */

// LIBRARY: LEGORR 0x0048e160 SYMBOL
// _srand
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
// GLOBAL: LEGORR 0x004afcc8
static uint32_t s_rand_state = 1;

void level_random_seed_set(uint32_t seed)
{
    s_rand_state = seed;
}

// LIBRARY: LEGORR 0x0048e170 SYMBOL
// _rand
int level_rand_u16(void)
{
    s_rand_state = s_rand_state * 214013u + 2531011u;
    return (((int32_t)s_rand_state) >> 16) & 0x7fff;
}
#endif

int level_random_draw15(void)
{
    return (int)level_rand_u16();
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
    return atoi(fields[1]);
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

// FUNCTION: LEGORR 0x0044e0c0
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

int terrain_surface_height_get(const void* handle, int x, int y)
{
    const TerrainState* t = (const TerrainState*)handle;
    if (t == NULL || t->blocks == NULL || x < 0 || y < 0
        || x >= t->width || y >= t->height) {
        return 0;
    }
    return t->blocks[((size_t)y * (size_t)t->width + (size_t)x)
                      * TERRAIN_BLOCK_STRIDE + 0x1a];
}

float terrain_blocksize_get(const void* handle)
{
    const TerrainState* t = (const TerrainState*)handle;
    return t != NULL ? t->blocksize : 0.0f;
}

/* MapCellToWorldCenter (FUN_0044f640): use the terrain's retained grid
 * transform and return the center of a cell in world x/y coordinates. */
int terrain_cell_world_center_xy_get(const void* handle, int x, int y,
                                     float position_xy[2])
{
    const TerrainState* t = (const TerrainState*)handle;
    if (t == NULL || position_xy == NULL || x < 0 || y < 0 ||
        x >= t->width || y >= t->height) return 0;
    position_xy[0] = t->blocksize * 0.5f + (float)x * t->blocksize
        + t->world_off_x;
    position_xy[1] = t->world_off_y -
        (t->blocksize * 0.5f + (float)y * t->blocksize);
    return 1;
}

/* WorldToTerrainCellBoundsCheck (FUN_0044f700): invert the center-based
 * terrain transform, round through the target CRT conversion, then check
 * the actual map dimensions. */
int terrain_world_xy_to_cell(const void* handle, float x, float y,
                             int* cell_x, int* cell_y)
{
    const TerrainState* t = (const TerrainState*)handle;
    int mapped_x, mapped_y;
    if (t == NULL || t->blocksize == 0.0f) return 0;
    mapped_x = crt_float_to_long(
        (x - t->world_off_x - t->blocksize * 0.5f) / t->blocksize);
    mapped_y = crt_float_to_long(
        (t->world_off_y - t->blocksize * 0.5f - y) / t->blocksize);
    if (cell_x != NULL) *cell_x = mapped_x;
    if (cell_y != NULL) *cell_y = mapped_y;
    return mapped_x >= 0 && mapped_y >= 0 &&
        mapped_x < t->width && mapped_y < t->height;
}

/* TerrainCellSurfaceTypeGet (FUN_0044f540, finding 226): the per-cell
 * surface-type byte in the physical block table — the port's terrain
 * handle is the TerrainState model, so the corpus's record fields
 * +0x8/+0x28 (stride, table) map to the modeled width/table pair. Bounds
 * guard added — the corpus indexes blind (documented deviation). */
// FUNCTION: LEGORR 0x0044f540
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

/* TerrainCellChangedFlagSet (FUN_0044efb0): the physical cell dirty bit is
 * available even though TerrainState does not yet own the renderer's global
 * dirty word or mesh handles. */
void terrain_cell_changed_flag_set(void* terrain, int x, int y)
{
    TerrainState* t = (TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL || x < 0 || y < 0 ||
        x >= t->width || y >= t->height) return;
    t->blocks[((size_t)y * (size_t)t->width + (size_t)x) *
              TERRAIN_BLOCK_STRIDE + 0x19] |= 4u;
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

/* The corpus keeps a bounded twenty-entry pool of the predug coordinates
 * encountered while loading the layer. */
// GLOBAL: LEGORR 0x004e844c
static int s_predug_coordinates[20][2];
// GLOBAL: LEGORR 0x004e853c
static unsigned int s_predug_coordinate_count;
unsigned int level_predug_coordinate_count_get(void)
{
    return s_predug_coordinate_count;
}
int level_predug_coordinate_get(unsigned int index, int cell_xy_out[2])
{
    if (cell_xy_out == NULL || index >= s_predug_coordinate_count) return 0;
    cell_xy_out[0] = s_predug_coordinates[index][0];
    cell_xy_out[1] = s_predug_coordinates[index][1];
    return 1;
}
void level_predug_coordinate_pool_clear(void)
{
    memset(s_predug_coordinates, 0, sizeof(s_predug_coordinates));
    s_predug_coordinate_count = 0u;
}
// FUNCTION: LEGORR 0x0044A4B0
void predug_coordinate_pool_append(const int cell_xy[2])
{
    volatile unsigned int* count = &s_predug_coordinate_count;
    unsigned int slot = *count;
    if (slot < 20u) {
        s_predug_coordinates[slot][0] = cell_xy[0];
        slot = *count;
        s_predug_coordinates[slot][1] = cell_xy[1];
        *count = slot + 1u;
    }
}

/* LoadPredugLayer @ 0x42bd20 */
// FUNCTION: LEGORR 0x0042bd20
static int load_predug(LevelState* s, const char* name, int bias)
{
    s_predug_coordinate_count = 0;
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
                fC |= 0x20u;
                const int cell_xy[2] = {x, y};
                predug_coordinate_pool_append(cell_xy);
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

struct TerrainActivityBuildRecordModel {
    LevelState* level;
    int x;
    int y;
    int enabled;
    uint8_t scene_flags_0c;
};
static TerrainActivityBuildRecordModel s_terrain_activity_build_records[256];

/* FUNCTION: LEGORR 0x004311d0. The scene object and linked record family
 * are not ported, so preserve the observable per-level cell membership and
 * enable state in a bounded sidecar table. */
static void terrain_activity_build_record_update(LevelState* s, int x, int y,
                                                  int enable)
{
    if (s == NULL) return;
    int free_slot = -1;
    for (int i = 0; i < 256; ++i) {
        TerrainActivityBuildRecordModel* record =
            &s_terrain_activity_build_records[i];
        if (record->level == s && record->x == x && record->y == y) {
            if (enable == 0) {
                record->level = NULL;
                record->enabled = 0;
                record->scene_flags_0c = 0u;
            } else {
                record->enabled = enable;
            }
            return;
        }
        if (record->level == NULL && free_slot < 0) free_slot = i;
    }
    if (enable != 0 && free_slot >= 0) {
        s_terrain_activity_build_records[free_slot].level = s;
        s_terrain_activity_build_records[free_slot].x = x;
        s_terrain_activity_build_records[free_slot].y = y;
        s_terrain_activity_build_records[free_slot].enabled = enable;
        s_terrain_activity_build_records[free_slot].scene_flags_0c = 0u;
    }
}

void* terrain_activity_build_record_get(LevelState* level, int x, int y)
{
    for (int i = 0; level != NULL && i < 256; ++i) {
        TerrainActivityBuildRecordModel* record =
            &s_terrain_activity_build_records[i];
        if (record->level == level && record->x == x && record->y == y) {
            return record;
        }
    }
    return NULL;
}

/* FUNCTION: LEGORR 0x004314c0 */
void terrain_activity_build_record_release(void* value)
{
    for (int i = 0; value != NULL && i < 256; ++i) {
        if (value == &s_terrain_activity_build_records[i]) {
            s_terrain_activity_build_records[i].level = NULL;
            s_terrain_activity_build_records[i].enabled = 0;
            return;
        }
    }
}

/* FUNCTION: LEGORR 0x00431530 */
void terrain_activity_build_records_release_all(LevelState* level)
{
    for (int i = 0; level != NULL && i < 256; ++i) {
        if (s_terrain_activity_build_records[i].level == level) {
            terrain_activity_build_record_release(
                &s_terrain_activity_build_records[i]);
        }
    }
}

/* LoadErodeCryoreLayers @ 0x42c480 — reads the TerrainMap (Surf materials),
 * writes the erosion/cryo byte at voxel +0x02. */
// FUNCTION: LEGORR 0x0042c480
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
                /* rock-monster / cavern spawn point */
                terrain_activity_build_record_update(s, x, y, 1);
            }
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadEmergeLayer @ 0x42bf40 — even nonzero values seed an erosion stage;
 * zero and odd values are rounded up before the voxel's +0x06 write. */
// FUNCTION: LEGORR 0x0042bf40
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
            int seed_stage = 0;
            rec[0x06] = (uint8_t)(c >> 1);
            if (c == 0 || (c & 1u) != 0u) {
                ++c;
                rec[0x06] = (uint8_t)(c >> 1);
            } else {
                seed_stage = 1;
            }
            if (seed_stage) {
                const int cell_xy[2] = { x, y };
                lava_erosion_stage_register(
                    cell_xy, (uint32_t)level_rand_u16() & 3u);
            }
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadPathAiLayers @ 0x42c6a0 — reads the CryOreMap, writes path/AI byte
 * (value − bias) at voxel +0x05. */
// FUNCTION: LEGORR 0x0042c6a0
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
// FUNCTION: LEGORR 0x0042c760
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
// FUNCTION: LEGORR 0x0042c9d0
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
// FUNCTION: LEGORR 0x0042cc90
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
// FUNCTION: LEGORR 0x0042c120
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
// FUNCTION: LEGORR 0x0042c060
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

/* CellInitPass @ 0x42f6f0. The target processes one cell coordinate at a
 * time. The port applies record flags/types, task-counter transitions,
 * erosion queue state, terrain dirty/material state, callbacks, and the
 * diagonal fixup available from the retained height field. Cell-kind scene
 * records, the descriptor-list veto, transient emitter handles, per-cell
 * corner deformation, and material-transition slots still need their
 * renderer/global owners. */
// FUNCTION: LEGORR 0x0042f6f0
static void cell_init_pass(LevelState* s, float cell_x, uint32_t cell_y)
{
    static const int neighbour_x[4] = { 0, 1, 1, 0 };
    static const int neighbour_y[4] = { 0, 0, 1, 1 };
    int x = (int)cell_x;
    int width;
    int height;
    uint8_t* rec;
    uint32_t primary;
    uint32_t original_primary;
    uint8_t empty_neighbours[4];
    int empty_count = 0;
    uint32_t secondary;
    int solid_count = 0;
    int last_solid = 0;
    int i;
    if (s == NULL) return;
    width = level_int(s, LEVEL_OFF_WIDTH);
    height = level_int(s, LEVEL_OFF_HEIGHT);
    if ((uint32_t)x >= (uint32_t)(width - 1) ||
        cell_y >= (uint32_t)(height - 1)) return;
    rec = voxel_at(s, x, (int)cell_y);
    if (rec == NULL) return;
    primary = *(uint32_t*)(rec + 0x08);
    original_primary = primary;
    secondary = *(uint32_t*)(rec + 0x0c);
    if ((primary & 0x20000000u) != 0u) {
        if ((secondary & 0x400u) == 0u) {
            entity_task_counter_increment_by_branch(1);
            secondary |= 0x400u;
            *(uint32_t*)(rec + 0x0c) = secondary;
        }
    }
    else if ((secondary & 0x400u) != 0u) {
        entity_task_counter_increment_by_branch(0);
        secondary &= ~0x400u;
        *(uint32_t*)(rec + 0x0c) = secondary;
    }
    for (i = 0; i < 4; ++i) {
        uint8_t* neighbour = voxel_at(s, x + neighbour_x[i],
                                      (int)cell_y + neighbour_y[i]);
        if (neighbour != NULL && neighbour[0] == 1) {
            ++solid_count;
            last_solid = i;
        }
        else {
            empty_neighbours[empty_count++] = (uint8_t)i;
        }
    }
    primary &= 0xffffdf27u;
    if (solid_count == 4) {
        primary = (primary & 0xffffdf07u) | 0x0cu;
        rec[3] = 0;
    } else if (solid_count == 1 || solid_count == 3) {
        primary = (primary & 0xffffdf07u) | 0x14u;
        if (solid_count == 1)
            rec[3] = (uint8_t)((last_solid + 2) & 3);
        switch (rec[2]) {
        case 1:
        case 7:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x35 : 0x55);
            break;
        case 2:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x34 : 0x54);
            break;
        case 3:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x33 : 0x53);
            break;
        case 4:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x32 : 0x52);
            break;
        case 5:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x31 : 0x51);
            break;
        default:
            break;
        }
        if (solid_count == 1)
            primary = (primary & 0xffffdf07u) | 0x54u;
        else
            primary |= 0x80u;
    } else {
        if (solid_count == 0) {
            rec[1] = 0x70;
            if ((primary & 4u) != 0u && rec[2] != 1 && rec[2] != 2 &&
                rec[2] != 3 && rec[2] != 4 && rec[2] != 5)
                rec[3] = 0;
        }
        else {
            static const uint8_t empty_pairs[6][2] = {
                { 0, 1 }, { 1, 2 }, { 2, 3 },
                { 0, 3 }, { 0, 2 }, { 1, 3 }
            };
            static const uint8_t partial_shapes[5] = {
                0x25, 0x24, 0x23, 0x22, 0x21
            };
            uint8_t orientation = (uint8_t)x;
            int pair;
            for (pair = 0; pair < 6; ++pair) {
                if ((empty_neighbours[0] == empty_pairs[pair][0] &&
                     empty_neighbours[1] == empty_pairs[pair][1]) ||
                    (empty_neighbours[1] == empty_pairs[pair][0] &&
                     empty_neighbours[0] == empty_pairs[pair][1])) {
                    orientation = (uint8_t)pair;
                    break;
                }
            }
            primary = (primary & 0xffffdf07u) | 0x14u;
            if (orientation > 3u) {
                if (rec[2] >= 1 && rec[2] <= 5)
                    rec[1] = 0x77;
                primary |= 0x2000u;
                rec[3] = (uint8_t)(orientation - 4u);
                terrain_block_fixup((TerrainState*)s->ptr_terrain, x,
                                    (int)cell_y,
                                    (int)((orientation - 4u) & 1u));
            }
            else if ((original_primary & 0x20u) != 0u) {
                if (rec[2] >= 1 && rec[2] <= 5)
                    rec[1] = partial_shapes[rec[2] - 1];
                rec[3] = orientation;
            }
            else {
                switch (rec[2]) {
                case 1:
                case 7: rec[1] = 5; break;
                case 2: rec[1] = 4; break;
                case 3: rec[1] = 3; break;
                case 4: rec[1] = 2; break;
                case 5: rec[1] = 1; break;
                case 8: rec[1] = 0x40; break;
                case 10: rec[1] = 0x20; break;
                case 11: rec[1] = 0x67; break;
                default: break;
                }
                rec[3] = orientation;
            }
        }
    }
    *(uint32_t*)(rec + 0x08) = primary;
    if (solid_count == 4 && (original_primary & 8u) == 0u) {
        int cell_xy[2];
        cell_xy[0] = x;
        cell_xy[1] = (int)cell_y;
        terrain_cell_refresh_dispatch_current(cell_xy, 0);
        terrain_neighbor_flag_refresh_xy(s, x, (int)cell_y, 1);
        if ((render_feature_flags_get() & 0x800000u) != 0u) {
            int cell_xy[2];
            cell_xy[0] = x;
            cell_xy[1] = (int)cell_y;
            cmd_produce(0x0e, NULL, rec[3], cell_xy);
        }
    }
    else if (solid_count == 1 || solid_count == 3) {
        terrain_neighbor_flag_refresh_xy(s, x, (int)cell_y, 1);
    }
    if (solid_count == 3) {
        int cell_xy[2];
        cell_xy[0] = x;
        cell_xy[1] = (int)cell_y;
        coordinate_callback_dispatch_current(cell_xy);
    }

    /* FUN_0040e1e0 / FUN_0040e350 use the renderer-owned DAT_004c7df8
     * registry. When that owner is absent, its create/remove operation has
     * no portable record to mutate; the level flags and command side effects
     * around it are still available here. */
    if (solid_count == 4 &&
        (*(uint32_t*)(rec + 0x08) & 0x400000u) == 0u) {
        int cell_xy[2];
        uint32_t* current_flags = (uint32_t*)(rec + 0x08);
        int descriptor_match = 0;
        void* descriptor;
        cell_xy[0] = x;
        cell_xy[1] = (int)cell_y;
        if ((*current_flags & 0x400u) == 0u &&
            (secondary & 4u) == 0u && (rec[9] & 0x80u) == 0u) {
            /* DAT_004b8a24 shares the path-record list root. Its pair keys
             * are at +0x0c/+0x10 and its next link is at +0x24. */
            descriptor = path_record_last_peek();
            while (descriptor != NULL) {
                const uint32_t* words = (const uint32_t*)descriptor;
                if (words[3] == (uint32_t)x &&
                    words[4] == (uint32_t)cell_y) {
                    descriptor_match = 1;
                    break;
                }
                descriptor = *(void**)((uint8_t*)descriptor + 0x24);
            }
            if (!descriptor_match) {
                GameContext* game = entity_pool_callback_context_get();
                *current_flags &= 0xffdfffffu;
                *current_flags &= 0xdfffffffu;
                *current_flags &= 0xbfffffffu;
                entity_activity_scene_state_update(game);
                coordinate_pair_callback_dispatch(cell_xy);
                flic_opcode27_fallback_noop();
                coordinate_pair_event_queue(cell_xy, (void*)0x28);
                coordinate_pair_event_queue(cell_xy, (void*)0x28);
                coordinate_pair_event_queue(cell_xy, (void*)0x28);
                coordinate_pair_event_queue(cell_xy, (void*)0x28);
                *current_flags |= 3u;
            }
        }
        *current_flags |= 0x400000u;
        selection_mode7_cancel_at_point((const uint32_t*)cell_xy);
    }

    /* The target selects erosion and flag-driven material variants only in
     * its four-solid-cell branch. Other neighbor counts retain the shape
     * variant selected above for the common terrain update below. */
    if (solid_count == 4) {
        primary = *(uint32_t*)(rec + 0x08);
        secondary = *(uint32_t*)(rec + 0x0c);
        if (rec[2] == 6 || (primary & 0x80000000u) != 0u) {
            int cell_xy[2];
            cell_xy[0] = x;
            cell_xy[1] = (int)cell_y;
            lava_erosion_pending_cell_add(cell_xy, 1);
            if ((secondary & 0x200u) == 0u) {
                if (rec[7] == 4u) {
                    rec[1] = 0x46u;
                    terrain_cell_dirty_queue_push_and_clear(
                        s->ptr_terrain, x, (int)cell_y, 1);
                }
                else {
                    terrain_cell_dirty_queue_push_and_clear(
                        s->ptr_terrain, x, (int)cell_y, 0);
                    switch (rec[7]) {
                    case 0: rec[1] = 6u; break;
                    case 1: rec[1] = 0x16u; break;
                    case 2: rec[1] = 0x26u; break;
                    case 3: rec[1] = 0x36u; break;
                    default: break;
                    }
                }
            }
            else {
                terrain_cell_dirty_queue_push_and_clear(
                    s->ptr_terrain, x, (int)cell_y, 0);
                rec[1] = 0x47u;
            }
        }
        else if (rec[2] == 9u) {
            rec[1] = 0x45u;
            terrain_cell_dirty_queue_push_and_clear(
                s->ptr_terrain, x, (int)cell_y, 1);
        }
        else if ((primary & 0x100000u) == 0u) {
            if ((primary & 0x20000000u) != 0u) {
                uint8_t kind = rec[3];
                rec[1] = (uint8_t)terrain_neighbor_coordinate_check(
                    s, x, (int)cell_y, &kind, (secondary & 0x100u) != 0u);
                rec[3] = kind;
            }
            else if ((secondary & 0x20u) != 0u) {
                rec[1] = 0x30u;
            }
            else if ((primary & 0x200000u) != 0u) {
                rec[1] = (uint8_t)(((primary & 0x40000000u) != 0u)
                                   ? 0x61u : 0u);
            }
            else {
                rec[1] = (uint8_t)(0x13u - (primary & 3u));
            }
        }
        else {
            rec[1] = (uint8_t)(((secondary & 0x100u) != 0u)
                               ? 0x66u : 0x76u);
        }
    }

    if (solid_count == 1 || solid_count == 3) {
        TerrainState* terrain = (TerrainState*)s->ptr_terrain;
        const int h00 = terrain_surface_height_get(terrain, x,
                                                   (int)cell_y);
        const int h11 = terrain_surface_height_get(terrain, x + 1,
                                                   (int)cell_y + 1);
        const int h10 = terrain_surface_height_get(terrain, x + 1,
                                                   (int)cell_y);
        const int h01 = terrain_surface_height_get(terrain, x,
                                                   (int)cell_y + 1);
        const int diagonal_a = h00 - h11;
        const int diagonal_b = h10 - h01;
        terrain_block_fixup(terrain, x, (int)cell_y,
                            abs(diagonal_b) < abs(diagonal_a));
    }

    /* CellInitPass clears the temporary color override for these cell
     * families before asking the renderer to rebuild the final surface. */
    if ((primary & 0x800004u) == 0x800004u &&
        (rec[2] == 1 || rec[2] == 7 || (primary & 8u) != 0u)) {
        primary &= ~0x800000u;
        *(uint32_t*)(rec + 0x08) = primary;
        if (s->ptr_terrain != NULL)
            terrain_cell_surface_type_reset(s->ptr_terrain, x, (int)cell_y);
    }

    /* The target ends in renderer-owned material transition calls. Update the
     * retained terrain record here; the preview grid returned by
     * path_record_surface_grid_get is not a level renderer and must not be
     * written from this global map sweep. */
    if (s->ptr_terrain != NULL) {
        (void)terrain_cell_surface_type_set(s->ptr_terrain, x,
                                            (int)cell_y, (int)rec[1]);
    }

    /* Target invalidates the 4x4 terrain patch around the updated cell. */
    {
        int dirty_x, dirty_y;
        for (dirty_y = (int)cell_y - 1; dirty_y <= (int)cell_y + 2;
             ++dirty_y) {
            for (dirty_x = x - 1; dirty_x <= x + 2; ++dirty_x) {
                terrain_cell_changed_flag_set(s->ptr_terrain, dirty_x,
                                              dirty_y);
            }
        }
    }
}

/* Keep the retail per-cell entry available to the path-record registration
 * and teardown routines. The map-load sweep remains on the private helper
 * above so it can preserve the original float coordinate call shape. */
void level_cell_init_pass(LevelState* level, int x, int y)
{
    cell_init_pass(level, (float)x, (uint32_t)y);
}

/* The level loader sweeps cells by calling the target-shaped per-cell entry.
 * Keep the sweep separate so it cannot be mistaken for the mapped function. */
static void cell_init_all(LevelState* s)
{
    int x, y;
    int width = level_int(s, LEVEL_OFF_WIDTH);
    int height = level_int(s, LEVEL_OFF_HEIGHT);
    for (y = 0; y < height - 1; ++y)
        for (x = 0; x < width - 1; ++x)
            cell_init_pass(s, (float)x, (uint32_t)y);
}

/* ------------------------------------------------------------------ *
 *  MapLoadPipeline @ 0x42b510.
 * ------------------------------------------------------------------ */

// FUNCTION: LEGORR 0x0042b510
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
    cell_init_all(s);
    return 1;
}

/* The source level loader's predug pool is the production source for the
 * camera selector's bounded copy. Keep the engine sidecar in step with the
 * just-loaded level before simulation starts. */
static int camera_predug_coordinate_resolve(int x, int y, float* world_x,
                                           float* world_y, void* context)
{
    LevelState* level = (LevelState*)context;
    float position[2];
    if (level == NULL || world_x == NULL || world_y == NULL ||
        !terrain_cell_world_center_xy_get(
            level_load_ptr(level, LEVEL_OFF_TERRAIN), x, y, position)) {
        return 0;
    }
    *world_x = position[0];
    *world_y = position[1];
    return 1;
}

static void camera_predug_spawn_state_seed(LevelState* level)
{
    int cell_xy[20 * 2];
    unsigned int i;
    unsigned int count = level_predug_coordinate_count_get();
    if (count > 20u) count = 20u;
    for (i = 0u; i < count; ++i) {
        int cell[2];
        if (!level_predug_coordinate_get(i, cell)) break;
        cell_xy[i * 2u] = cell[0];
        cell_xy[i * 2u + 1u] = cell[1];
    }
    camera_predug_spawn_state_install(cell_xy, i, i != 0u);
    camera_spawn_level_block_bind(level);
    camera_spawn_coordinate_hook_set(camera_predug_coordinate_resolve, level);
}

/* ------------------------------------------------------------------ *
 *  Level load @ 0x429890 — config read + pipeline + post-load init.
 *
 *  Config paths are rooted at the file's first record (upstream block
 *  parser keeps the `Lego` root, so a level key reads as
 *  "Lego* <levelname> <key>", matching the corpus's BlockQueryDefault
 *  path-building "%s%s%s").
 * ------------------------------------------------------------------ */

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
    const char* value = block_get_strptr(cfg, path);
    return (float)atof(value != NULL ? value : "");
}

static int cfg_typed(BlockFile* cfg, const char* level, const char* key, int dflt)
{
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    /* FUN_004790d0 delegates to ConfigBooleanGet @ 0x477710: missing or
     * unrecognized values answer 2 instead of parsing a numeric string. */
    return block_get_bool(cfg, path);
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

// FUNCTION: LEGORR 0x00429890
LevelState* level_load(BlockFile* cfg, const char* level_name)
{
    GameContext* game;
    LevelState* previous_level;
    if (cfg == NULL || level_name == NULL) {
        return NULL;
    }
    game = entity_pool_callback_context_get();
    previous_level = game != NULL ? game->level : NULL;

    /* PendingLevelName clears the cross-level pools before loading the new
     * level (FUN_004372a0); this is the shared production level-load path. */
    spawn_coordinate_pool_clear();

    LevelState* s = (LevelState*)calloc(1, sizeof(LevelState));
    if (s == NULL) {
        return NULL;
    }
    /* DAT_005560d4 is current throughout LevelLoad, including the map
     * loaders that register EmergeMap stages. Mirror that lifetime in the
     * GameContext instead of waiting for the caller's assignment. */
    if (game != NULL) game->level = s;
    /* The erosion tables are process globals in the reference, but their
     * active coordinates belong to this level's EmergeMap. Bind and clear
     * them before loading any sparse map that can register a stage. */
    lava_erosion_level_context_set(s);
    {
        float trigger_time = cfg_float(cfg, level_name,
                                       K_ERODETRIGGER, 0.0f);
        float erosion_time = cfg_float(cfg, level_name,
                                        K_ERODETIME, 0.0f);
        float lock_time = cfg_float(cfg, level_name, K_ERODELOCK, 0.0f);
        long mutation_count = cfg_int(cfg, level_name,
                                      K_LANDSLIDECOUNT, 0);
        if (trigger_time == 0.0f) trigger_time = 10.0f;
        if (erosion_time == 0.0f) erosion_time = 5.0f;
        if (lock_time == 0.0f) lock_time = 60.0f;
        if (mutation_count == 0) mutation_count = 3;
        erosion_timing_state_init(trigger_time, erosion_time, lock_time);
        lava_erosion_mutation_threshold_set((unsigned int)mutation_count);
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
                          strcasecmp_lite(boulder, "None") == 0 ? 0 : 1);
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
        if (game != NULL) game->level = previous_level;
        return NULL;
    }
    camera_predug_spawn_state_seed(s);

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
#if defined(_MSC_VER)
        char* dup = _strdup(next);
#else
        char* dup = (char*)malloc(strlen(next) + 1);
        if (dup != NULL) strcpy(dup, next);
#endif
        if (dup != NULL) level_store_ptr(s, LEVEL_OFF_NEXTLEVEL, dup);
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
    GameContext* game;
    if (s == NULL) {
        return;
    }
    game = entity_pool_callback_context_get();
    if (game != NULL && game->level == s) game->level = NULL;
    camera_spawn_level_block_bind(NULL);
    lava_erosion_level_context_release(s);
    /* Level teardown's corpus path teardown includes ReleaseAllGroupRecords
     * (0x4099d0), which owns the global descriptor and payload chain. */
    path_record_release_all();
    path_record_cell_links_release(s, NULL);
    path_record_surface_grid_bind(NULL);
    terrain_cell_mutation_context_bind(NULL, NULL);
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

/* TerrainDescriptorFindExact (FUN_004090c0): when a pair key is supplied,
 * both words must match; otherwise the optional single key is compared
 * against the node's single-key field. The global list head is threaded in
 * as `list`. */
// FUNCTION: LEGORR 0x004090C0
TerrainDescriptorNode* terrain_descriptor_find_exact(
    TerrainDescriptorNode* list, const uint32_t* pair_key,
    const uint32_t* single_key)
{
    for (TerrainDescriptorNode* node = list; node != NULL; node = node->next) {
        if (pair_key != NULL) {
            if (node->key_pair_first == pair_key[0]
                && node->key_pair_second == pair_key[1]) {
                return node;
            }
        }
        else if (single_key != NULL && node->key_single == *single_key) {
            return node;
        }
    }
    return NULL;
}

/* TerrainCellPredicateCheck (FUN_004090a0): the public path passes a pair
 * key and a NULL single-key alternative, then reports whether an exact
 * descriptor exists. */
// FUNCTION: LEGORR 0x004090A0
int terrain_cell_predicate_check(TerrainDescriptorNode* list,
                                 const uint32_t* pair_key)
{
    return terrain_descriptor_find_exact(list, pair_key, NULL) != NULL;
}

/* CellResourceEntryMatches (FUN_0040af40): entries backed by a matching
 * live task are accepted directly. Otherwise the first two words must
 * match, and a nonzero bounded-mode flag applies the unsigned cell limit
 * from entry word two to the cell record's +0xcc word. */
// FUNCTION: LEGORR 0x0040AF40
int terrain_cell_resource_entry_matches(const uint32_t* cell_record,
                                        const uint32_t* resource_entry)
{
    if (cell_record == NULL || resource_entry == NULL) {
        return 0;
    }
    if (task_kind_filtered_search((int)resource_entry[0]) != 0) {
        return 1;
    }
    if (resource_entry[0] != cell_record[0]
        || resource_entry[1] != cell_record[1]) {
        return 0;
    }
    if (resource_entry[3] != 0u
        && cell_record[0xcc / sizeof(uint32_t)] < resource_entry[2]) {
        return 0;
    }
    return 1;
}

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

/* Level grid coordinates use the CRT's x87 float-to-long rounding. */
int level_cell_of(float v)
{
    return crt_float_to_long(v);
}

/* EntityPositionToTerrainCell (FUN_004424f0): the corpus obtains the
 * entity's scene position, then passes its x/y pair through the current
 * terrain's grid picker. EntitySlot::x/y are the port's logical mirrors
 * for that scene position (deviation: no renderer transform is available). */
// FUNCTION: LEGORR 0x004424F0
void entity_position_to_terrain_cell(const struct EntitySlot* entity,
                                     const LevelState* s,
                                     int* cell_x, int* cell_y)
{
    if (entity == NULL || s == NULL || cell_x == NULL || cell_y == NULL) {
        return;
    }
    const int x = level_cell_of(entity->x);
    const int y = level_cell_of(entity->y);
    if (level_coord_pair_within_bounds(s, x, y) == 0) {
        return; /* deviation: the corpus grid picker owns this failure case */
    }
    *cell_x = x;
    *cell_y = y;
}

// FUNCTION: LEGORR 0x00432770
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
// FUNCTION: LEGORR 0x00432950
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
// FUNCTION: LEGORR 0x00432a20
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

/* Retail ABI: cell-coordinate pair plus the set/clear selector. */
// FUNCTION: LEGORR 0x00432e00
void level_tile_mark_current(const int cell_xy[2], int set)
{
    GameContext* game = entity_pool_callback_context_get();
    uint8_t* rec;
    uint32_t* flags;
    if (game == NULL || game->level == NULL || cell_xy == NULL) return;
    rec = level_voxel(game->level, cell_xy[0], cell_xy[1]);
    if (rec == NULL) return;
    flags = (uint32_t*)(rec + 0x08);
    if (set != 0) *flags |= TILE_FLAG_OCCUPIED;
    else *flags &= ~TILE_FLAG_OCCUPIED;
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

// FUNCTION: LEGORR 0x004319b0
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
// FUNCTION: LEGORR 0x00432bd0
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
// FUNCTION: LEGORR 0x00432c20
uint32_t level_cell_flag_wave(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x40000u;
}

/* FUN_00432c50: speed class 0 under the terrain block bit, else the low
 * terrain-type bits + 1. */
// FUNCTION: LEGORR 0x00432c50
int level_cell_speed_class(const LevelState* s, int cx, int cy)
{
    const uint32_t flags = cell_flag_word(s, cx, cy);
    if ((flags & 0x200000u) != 0) {
        return 0;
    }
    return (int)(flags & 3u) + 1;
}

/* FUN_00432e60: flag word's LOW BYTE (corpus byte read) masked 0xc0. */
// FUNCTION: LEGORR 0x00432e60
uint32_t level_cell_mask_c0(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (uint32_t)rec[8] & 0xc0u : 0u;
}

/* FUN_00432e90: predug edge-wall bit (finding 07, load-time 3×3 OR). */
// FUNCTION: LEGORR 0x00432e90
uint32_t level_cell_flag_edge_wall(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x4000000u;
}

/* FUN_00432ec0 / FUN_00432f00: state byte equals 1 / 6. */
// FUNCTION: LEGORR 0x00432ec0
int level_cell_state_is_1(const LevelState* s, int cx, int cy)
{
    return level_tile_type(s, cx, cy) == 1;
}

// FUNCTION: LEGORR 0x00432f00
int level_cell_state_is_6(const LevelState* s, int cx, int cy)
{
    return level_tile_type(s, cx, cy) == 6;
}

/* FUN_00432f40: terrain bits 0x18 clear; outside the gate counts as
 * clear, exactly as the corpus returns true for the early-out. */
// FUNCTION: LEGORR 0x00432f40
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
// FUNCTION: LEGORR 0x00432f90
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
// FUNCTION: LEGORR 0x00432fd0
uint32_t level_cell_flag_2000(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x2000u;
}

// FUNCTION: LEGORR 0x00433000
uint32_t level_cell_flag_80(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x80u;
}

// FUNCTION: LEGORR 0x00433030
uint32_t level_cell_flag_8000(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x8000u;
}

/* FUN_00433060: secondary flag word (record+12) bit 0. */
// FUNCTION: LEGORR 0x00433060
uint32_t level_cell_secondary_bit0(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 1u;
}

/* Port-facing coordinate helper retains explicit LevelState ownership. */
int level_cell_occupied_at(const LevelState* s, int cx, int cy)
{
    GameContext* game = entity_pool_callback_context_get();
    if (game != NULL && game->level == s) {
        const int cell_xy[2] = { cx, cy };
        return level_cell_occupied_at_current(cell_xy) != 0u;
    }
    return (cell_flag_word(s, cx, cy) & 0x20000000u) != 0;
}

/* TerrainCoordinateAndOccupancyCheck (FUN_00433090) uses the current level
 * global and returns the masked flag word (not a normalized boolean). */
// FUNCTION: LEGORR 0x00433090
uint32_t level_cell_occupied_at_current(const int cell_xy[2])
{
    GameContext* game = entity_pool_callback_context_get();
    if (cell_xy == NULL || game == NULL) return 0u;
    return cell_flag_word(game->level, cell_xy[0], cell_xy[1]) &
           0x20000000u;
}

/* FUN_004330e0: terrain/wave pair mask 0x180000. */
// FUNCTION: LEGORR 0x004330e0
int level_cell_mask_180000(const LevelState* s, int cx, int cy)
{
    return (cell_flag_word(s, cx, cy) & 0x180000u) != 0;
}

/* FUN_00433120: raise/clear flag bit 0x10000000 (bounds-guarded). */
// FUNCTION: LEGORR 0x00433120
void level_cell_flag_10000000_set(LevelState* s, int cx, int cy, int set)
{
    if (s == NULL || cx < 0 || cy < 0 ||
        cx >= level_int(s, LEVEL_OFF_WIDTH) ||
        cy >= level_int(s, LEVEL_OFF_HEIGHT)) {
        return;
    }
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    uint8_t* rec = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS)
                 + ((size_t)cy * (size_t)w + (size_t)cx) * LEVEL_VOXEL_STRIDE;
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
// FUNCTION: LEGORR 0x00432170
uint32_t level_cell_secondary_flag_100(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 0x100u;
}

/* FUN_004322d0: visited mark, secondary bit 1. */
// FUNCTION: LEGORR 0x004322d0
uint32_t level_cell_visited_flag(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 2u;
}

/* FUN_004323c0 / FUN_004325d0: clear/raise secondary bit 0. */
// FUNCTION: LEGORR 0x004323c0
void level_cell_secondary_bit0_clear(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word &= ~1u;
}

// FUNCTION: LEGORR 0x004325d0
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
// FUNCTION: LEGORR 0x004324d0
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

// FUNCTION: LEGORR 0x00432520
uint32_t level_cell_secondary_flag_8(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 8u;
}

/* FUN_00432aa0: mode 0 = masked 0x400 read; mode nonzero = OR test of
 * the 0x400 flag and the secondary BYTE's bit 4 (the byte read is the
 * low byte of the secondary word, so the word mask is equivalent). */
// FUNCTION: LEGORR 0x00432aa0
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

/* CellFourNeighborWalkCheck (FUN_0041f740): the tutorial path gate uses
 * this asymmetric cardinal-neighbor rule before submitting a cell action.
 * Neighbor order follows the corpus's local table: east, south, west,
 * north. */
// FUNCTION: LEGORR 0x0041f740
int cell_four_neighbor_walk_check(const LevelState* s, int cx, int cy)
{
    static const int8_t offsets[4][2] = {
        { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 },
    };
    const int marked = level_cell_mask_c0(s, cx, cy) != 0;
    int occupied_neighbors = 0;
    for (int i = 0; i < 4; ++i) {
        const int neighbor = level_cell_occupied_or_flag(
            s, cx + offsets[i][0], cy + offsets[i][1], 1) != 0;
        if (neighbor != 0) {
            if (marked == 0) {
                return 0;
            }
            ++occupied_neighbors;
        }
    }
    return marked == 0 || occupied_neighbors <= 1;
}

// FUNCTION: LEGORR 0x00432b00
uint32_t level_cell_flag_800_bounded(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint8_t* voxels = (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
        const size_t index = (size_t)cy * (size_t)w + (size_t)cx;
        uint32_t flags;
        __builtin_memcpy(&flags, voxels + index * LEVEL_VOXEL_STRIDE + 8,
                         sizeof(flags));
        return flags & 0x800u;
    }
    return 0u;
}

// FUNCTION: LEGORR 0x00432b50
uint32_t level_cell_flag_8_bounded(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint8_t* voxels = (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
        const size_t index = (size_t)cy * (size_t)w + (size_t)cx;
        uint32_t flags;
        __builtin_memcpy(&flags, voxels + index * LEVEL_VOXEL_STRIDE + 8,
                         sizeof(flags));
        return flags & 8u;
    }
    return 0u;
}

// FUNCTION: LEGORR 0x00432d90
uint32_t level_cell_flag_20_bounded(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint8_t* voxels = (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
        const size_t index = (size_t)cy * (size_t)w + (size_t)cx;
        uint32_t flags;
        __builtin_memcpy(&flags, voxels + index * LEVEL_VOXEL_STRIDE + 8,
                         sizeof(flags));
        return flags & 0x20u;
    }
    return 0u;
}

/* FUN_00432b90: signed state byte in {8, 10, 11}. No cell reads absent. */
// FUNCTION: LEGORR 0x00432b90
int level_cell_type_is_8_or_10_or_11(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    const int type = (int)(signed char)rec[2];
    return type == 8 || type == 10 || type == 11;
}

/* FUN_00432dd0: entity-reservation bit (the level_tile_mark flag). */
// FUNCTION: LEGORR 0x00432dd0
uint32_t level_cell_flag_reserved(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & TILE_FLAG_OCCUPIED;
}

/* FUN_00432710: raise/clear flag 0x80000 (bounds-guarded). */
// FUNCTION: LEGORR 0x00432710
void level_cell_flag_80000_set(LevelState* s, int cx, int cy, int set)
{
    if (s == NULL || cx < 0 || cy < 0 ||
        cx >= level_int(s, LEVEL_OFF_WIDTH) ||
        cy >= level_int(s, LEVEL_OFF_HEIGHT)) {
        return;
    }
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    uint8_t* rec = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS)
                 + ((size_t)cy * (size_t)w + (size_t)cx) * LEVEL_VOXEL_STRIDE;
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if (set != 0) {
        *flags |= 0x80000u;
    } else {
        *flags &= ~0x80000u;
    }
}

/* FUN_004332c0: record byte +3, unsigned. */
// FUNCTION: LEGORR 0x004332c0
int level_cell_record_byte3(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (int)rec[3] : 0;
}

/* Level-owned form used by map loading and simulation helpers. */
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

/* TerrainCellWorkFlagSet @ 0x004332f0 reads the current level global and
 * raises bit 2 only for coordinates strictly inside the final row/column. */
// FUNCTION: LEGORR 0x004332f0
void terrain_cell_work_flag_set_current(uint32_t cx, uint32_t cy)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint32_t width, height;
    uint8_t* rec;
    uint32_t* flags;
    if (game == NULL || game->level == NULL) {
        return;
    }
    s = game->level;
    width = (uint32_t)level_int(s, LEVEL_OFF_WIDTH);
    height = (uint32_t)level_int(s, LEVEL_OFF_HEIGHT);
    if (cx < width - 1u && cy < height - 1u) {
        rec = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS)
            + ((size_t)cy * (size_t)width + (size_t)cx)
                * LEVEL_VOXEL_STRIDE;
        flags = (uint32_t*)(rec + 0x08);
        *flags = *flags | 4u;
    }
}

/* TerrainCellWorkFlagClear (FUN_00430320): target-shaped implementation
 * below obtains the current level from GameContext. This explicit-level
 * worker is also used by the erosion path. The descriptor veto walks the
 * live path/descriptor list rooted at DAT_004b8a24; these records carry the
 * pair at +0x0c/+0x10 and next at +0x24. */
void terrain_cell_work_flag_clear(LevelState* s, const int coord[2],
                                  uint32_t selector)
{
    if (s == NULL || coord == NULL) {
        return;
    }
    uint8_t* rec = level_voxel(s, coord[0], coord[1]);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if ((*flags & 0x400u) != 0u || (rec[0x0c] & 4u) != 0u ||
        (*flags & 0x8000u) != 0u) {
        return;
    }

    {
        void* record = path_record_last_peek();
        while (record != NULL) {
            const uint32_t* words = (const uint32_t*)record;
            if (words[3] == (uint32_t)coord[0] &&
                words[4] == (uint32_t)coord[1]) {
                return;
            }
            record = *(void**)((uint8_t*)record + 0x24);
        }
    }

    *flags &= 0xffdfffffu;
    *flags &= 0xdfffffffu;
    *flags &= 0xbfffffffu;
    entity_activity_scene_state_update(entity_pool_callback_context_get());
    coordinate_callback_dispatch(coord, 0);
    flic_opcode27_fallback_noop();
    {
        int repeat;
        for (repeat = 0; repeat < 4; ++repeat) {
            coordinate_pair_event_queue(
                coord, (void*)(uintptr_t)selector);
        }
    }
    *flags |= 3u;
}

/* TerrainCellWorkFlagClear @ 0x00430320 takes only the coordinate pair and
 * event selector; DAT_005560d4 supplies its level. */
// FUNCTION: LEGORR 0x00430320
void terrain_cell_work_flag_clear_current(const int coord[2],
                                          uint32_t selector)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* flags;
    void* descriptor;
    if (game == NULL || game->level == NULL || coord == NULL) return;
    s = game->level;
    rec = level_voxel(s, coord[0], coord[1]);
    if (rec == NULL) return;
    flags = (uint32_t*)(rec + 0x08);
    if ((*flags & 0x400u) != 0u || (rec[0x0c] & 4u) != 0u ||
        (*flags & 0x8000u) != 0u) return;
    descriptor = path_record_last_peek();
    while (descriptor != NULL) {
        const uint32_t* words = (const uint32_t*)descriptor;
        if (words[3] == (uint32_t)coord[0] &&
            words[4] == (uint32_t)coord[1]) return;
        descriptor = *(void**)((uint8_t*)descriptor + 0x24);
    }
    *flags &= 0xffdfffffu;
    *flags &= 0xdfffffffu;
    *flags &= 0xbfffffffu;
    entity_activity_scene_state_update(game);
    coordinate_pair_callback_dispatch(coord);
    flic_opcode27_fallback_noop();
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    *flags |= 3u;
}

/* FUN_004502c0: unsigned pair membership (negative coords wrap). */
// FUNCTION: LEGORR 0x004502c0
int level_coord_pair_within_bounds(const LevelState* s, int cx, int cy)
{
    return (unsigned)cx < (unsigned)level_int(s, LEVEL_OFF_WIDTH) &&
           (unsigned)cy < (unsigned)level_int(s, LEVEL_OFF_HEIGHT);
}

/* FUN_004329d0: valid coordinate with the blocked bit (flag 8) set. */
// FUNCTION: LEGORR 0x004329d0
int level_cell_flag8_check(const LevelState* s, int cx, int cy)
{
    if (level_coord_pair_within_bounds(s, cx, cy)) {
        return (cell_flag_word(s, cx, cy) & 8u) != 0;
    }
    return 0;
}

/* --- map-file resource slots ---------------------------------------------
 * The corpus table interleaves (ptr, flag) pairs across 2000 entries as one
 * 8-byte stride record array; the port mirrors that layout. See level.hpp
 * for the deviation notes (the corpus's MapOpen2 opener rides the unported
 * wad seam; the claim and release ride the corpus's engine-heap allocator
 * pair, 0x48dbd0 / 0x48db80). */

/* One record array, stride 8: pointer at +0, used flag byte at +4
 * (corpus tests bit 0 of the byte, but claims with a dword load/or/store
 * and clears both fields as dwords — MapFileResourceSlot* @ 0x4893f0..). */
typedef struct MapFileResourceSlot {
    uint8_t* ptr;   /* +0x00 */
    uint8_t  used;  /* +0x04, bit 0 */
    uint8_t  pad[3];
} MapFileResourceSlot;

/* ptr 4 + used 1 + pad 3 = 8: a wrong stride fails this typedef. */
typedef char map_file_resource_slot_stride_check[sizeof(MapFileResourceSlot) == 8u ? 1 : -1];

// GLOBAL: LEGORR 0x00544a20
static MapFileResourceSlot s_map_slots[LEVEL_MAP_SLOT_COUNT];

/* FUN_004893f0: clear every pair (no frees — the corpus leak loop). The
 * corpus walks the used words from 0x544a24 up to 0x5488a4 (add 8 / cmp /
 * jb) and dword-zeroes both fields, so the clears are raw dword stores. */
// FUNCTION: LEGORR 0x004893f0
void level_map_slots_reset(void)
{
    /* Plain pointer operands keep the walk's loop-back unsigned (jb). */
    uint32_t* s = (uint32_t*)&s_map_slots[0].used;
    uint32_t* const end = s + LEVEL_MAP_SLOT_COUNT * 2;
    for (; s < end; s += 2) {
        s[-1] = 0;
        s[0] = 0;
    }
}

/* FUN_00489410: first free slot, engine-heap image, -1 when full. The
 * corpus allocates through the engine-heap wrapper with no zero-fill and
 * no failure check (an allocator NULL rides straight into the slot and
 * faults on use), keeps the scan index in a second register, and raises
 * bit 0 with a dword load/or/store. The port keeps the indexed scan —
 * the walk form tested lower (register-role roulette, b24 class). */
// FUNCTION: LEGORR 0x00489410
int level_map_slot_allocate(size_t size)
{
    /* Indexed, like the corpus's hit path (mov [esi*8+0x544a20]): the
     * scale-index operand rides the loop counter. */
    for (uint32_t i = 0; i < LEVEL_MAP_SLOT_COUNT; ++i) {
        if ((s_map_slots[i].used & 1u) == 0u) {
            /* Blind like the corpus: no zero-fill, no NULL check (the
             * retry carrier's zero-size quirk covers size 0). */
            void* img = engine_heap_alloc((long)size);
            s_map_slots[i].ptr = (uint8_t*)img;
            *(uint32_t*)&s_map_slots[i].used |= 1u;
            return (int)i;
        }
    }
    return -1; /* the corpus's or eax, 0xffffffff */
}

/* FUN_00489460: free + clear an in-use slot; bad/free ids are no-ops.
 * The corpus frees through the engine-heap free (0x48db80) and clears
 * both fields as dwords. */
// FUNCTION: LEGORR 0x00489460
void level_map_slot_release(uint32_t slot)
{
    if (slot < LEVEL_MAP_SLOT_COUNT && (s_map_slots[slot].used & 1u) != 0u) {
        MapFileResourceSlot* s = &s_map_slots[slot];
        engine_heap_free(s->ptr);
        *(uint32_t*)&s->ptr = 0;
        *(uint32_t*)&s->used = 0;
    }
}

/* FUN_004894a0: the image pointer, or NULL when free/out of range. */
// FUNCTION: LEGORR 0x004894a0
uint8_t* level_map_slot_get(uint32_t slot)
{
    if (slot < LEVEL_MAP_SLOT_COUNT && (s_map_slots[slot].used & 1u) != 0u) {
        return s_map_slots[slot].ptr;
    }
    return NULL;
}

/* FUN_004515a0: header dims at +0x08/+0x0C (port guards a NULL image). */
// FUNCTION: LEGORR 0x004515a0
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
// FUNCTION: LEGORR 0x004515c0
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
// FUNCTION: LEGORR 0x004334c0
void level_map_dimensions_get(uint32_t slot, uint32_t* width,
                              uint32_t* height)
{
    level_map_image_header(level_map_slot_get(slot), width, height);
}

/* FUN_004319e0: cell through the slot; the width comes from the header. */
// FUNCTION: LEGORR 0x004319e0
uint16_t level_map_cell_get(uint32_t slot, int x, int y)
{
    const uint8_t* img = level_map_slot_get(slot);
    uint32_t w = 0, h = 0;
    level_map_image_header(img, &w, &h);
    return level_map_image_cell(img, x, (int)w, y);
}

/* FUN_00433330: gated state-byte read through an out param. */
// FUNCTION: LEGORR 0x00433330
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
// FUNCTION: LEGORR 0x0042c5b0
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
// FUNCTION: LEGORR 0x00429850
uint32_t active_engine_field_b0_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, LEVEL_OFF_ENGINE_B0) : 0u;
}

/* EngineFloatField210GetByIndex (FUN_00429860, findings 295/392): the
 * indexed float at +0x210 + 4*index. */
// FUNCTION: LEGORR 0x00429860
float engine_float_field210_get_by_index(const LevelState* s, int index)
{
    if (s == NULL) {
        return 0.0f;
    }
    return level_float(s, LEVEL_OFF_ENGINE_FLOATS + index * 4);
}

/* EngineFloatField260Get (FUN_00429880, findings 295/392): the float at
 * active engine-state +0x260. */
// FUNCTION: LEGORR 0x00429880
float engine_float_field260_get(const LevelState* s)
{
    return s != NULL ? level_float(s, LEVEL_OFF_ENGINE_F260) : 0.0f;
}

/* AirSupplyAmountUpdate (FUN_00424560, finding 317): see level.hpp. The
 * stats read lands before the gate (corpus order); the corpus's float10
 * accumulation is modelled with the port's double — the documented
 * extended-precision deviation. */
// FUNCTION: LEGORR 0x00424560
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
// FUNCTION: LEGORR 0x0042b500
uint32_t global_state_field84_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, LEVEL_OFF_STATE84) : 0u;
}

/* UpgradeResourceCheck (FUN_0041f900, finding 475): the level's resource
 * counters — +0xa0 for selector 0, +0xac for any nonzero selector; and
 * zero when no level object is active (the corpus's own DAT-null rule,
 * which doubles as the port's NULL guard). */
// FUNCTION: LEGORR 0x0041f900
uint32_t upgrade_resource_check(const LevelState* s, int selector)
{
    if (s == NULL) {
        return 0u;
    }
    return (uint32_t)level_int(s, selector != 0 ? LEVEL_OFF_RES_B
                                               : LEVEL_OFF_RES_A);
}

/* AlwaysAcceptPlaceCommandCell (FUN_00432550, finding 455): always yes. */
// FUNCTION: LEGORR 0x00432550
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
#define LEVEL_CELL_VISITED_TRACK_CAP 100

// GLOBAL: LEGORR 0x0055684c
static int32_t s_cell_refresh_track[LEVEL_CELL_REFRESH_TRACK_CAP][2];
// GLOBAL: LEGORR 0x00556e8c
static int     s_cell_refresh_track_count;
// GLOBAL: LEGORR 0x00556528
static int32_t s_cell_visited_track[LEVEL_CELL_VISITED_TRACK_CAP][2];
// GLOBAL: LEGORR 0x00556848
static int     s_cell_visited_track_count;
static TerrainNeighborPresenceHook s_terrain_neighbor_presence_hook;
static TerrainNeighborRenderUpdateHook s_terrain_neighbor_render_hook;
static void* s_terrain_neighbor_hook_context;
static TerrainNeighborActionHook s_terrain_neighbor_build_action;
static TerrainNeighborActionHook s_terrain_neighbor_clear_action;
static void* s_terrain_neighbor_action_context;
static TerrainCellVariantCompleteHook s_terrain_cell_variant_complete_hook;
static void* s_terrain_cell_variant_complete_context;
static TerrainCellStateProjectionHook s_terrain_cell_state_projection_hook;
static void* s_terrain_cell_state_projection_context;
static DockDefinitionCellGetHook s_dock_definition_cell_get_hook;
static void* s_dock_definition_cell_get_context;
static SpatialObjectQueryHook s_spatial_object_query_hook;
static void* s_spatial_object_query_context;
static TerrainCellRecordCountIncrementHook s_terrain_cell_record_count_hook;
static void* s_terrain_cell_record_count_context;
static GridCellEligibilityHooks s_grid_cell_eligibility_hooks;

void grid_cell_eligibility_hooks_set(const GridCellEligibilityHooks* hooks)
{
    if (hooks == NULL) memset(&s_grid_cell_eligibility_hooks, 0,
                              sizeof(s_grid_cell_eligibility_hooks));
    else s_grid_cell_eligibility_hooks = *hooks;
}

// FUNCTION: LEGORR 0x00433e30
int grid_cell_entity_eligibility_check(int x, int y)
{
    GridCellEligibilityHooks* hooks = &s_grid_cell_eligibility_hooks;
    uint32_t flags = 0;
    if (hooks->cell_flags_get != NULL) {
        flags = hooks->cell_flags_get(x, y, hooks->context);
    }
    if ((flags & 0x100u) != 0u) return 0;
    return hooks->surface_entity_eligible != NULL &&
           hooks->surface_entity_eligible(x, y, hooks->context) != 0;
}

void spatial_object_query_hook_install(SpatialObjectQueryHook hook,
                                       void* context)
{
    s_spatial_object_query_hook = hook;
    s_spatial_object_query_context = context;
}

void* spatial_object_query_execute(const SpatialObjectQueryRequest* request)
{
    if (request == NULL || s_spatial_object_query_hook == NULL) return NULL;
    return s_spatial_object_query_hook(s_spatial_object_query_context,
                                      request);
}

void terrain_cell_record_count_increment_hook_install(
    TerrainCellRecordCountIncrementHook hook, void* context)
{
    s_terrain_cell_record_count_hook = hook;
    s_terrain_cell_record_count_context = context;
}

void terrain_cell_record_count_increment(int a, int b, int c)
{
    if (s_terrain_cell_record_count_hook != NULL) {
        s_terrain_cell_record_count_hook(a, b, c,
                                         s_terrain_cell_record_count_context);
    }
}

// FUNCTION: LEGORR 0x00403910
void terrain_cell_record_counts_prune(void* entity)
{
    int a, b, c;
    uint32_t* count = cmd_pending_command_counts;
    for (a = 0; a < 20; ++a) {
        for (b = 0; b < 15; ++b) {
            for (c = 0; c < 16; ++c, ++count) {
                uint32_t ordinal = 0;
                uint32_t removed = 0;
                while (ordinal < *count) {
                    if (unported_ledger_stubs::
                            object_task_target_filter_find((uint32_t)a,
                                (uint32_t)b, (uint32_t)c) == NULL ||
                        entity_action_kind_entity_create_and_command_issue(
                            (EntitySlot*)entity, a, b, c) != NULL) {
                        ++removed;
                    }
                    ++ordinal;
                }
                *count -= removed;
            }
        }
    }
}

// FUNCTION: LEGORR 0x004039a0
void task_record_create_and_scan_grid(
    void* entity)
{
    void* record;
    /* FUN_004039a0 always creates the timed kind-5 record with
     * (payload, 0, -1, 0), then changes its command kind to 9. */
    record = timed_task_record_initialize_and_link(entity, 0, -1, 0);
    if (record != NULL) cmd_set_kind(record, 9);
    terrain_cell_record_counts_prune(entity);
}

int terrain_cell_surface_update_dispatch(
    const uint8_t* cell, int x, int y, void* param_a, void* param_b,
    const TerrainCellSurfaceUpdateHooks* hooks)
{
    float coordinates[12];
    uint32_t flags;
    uint32_t packed_surface;
    void* resource;
    if (cell == NULL || hooks == NULL || hooks->resource_get == NULL ||
        hooks->geometry_get == NULL || hooks->dispatch == NULL) return 0;
    memcpy(&flags, cell + 8, sizeof(flags));
    if ((flags & 0x18u) == 0u) return 0;
    packed_surface = cell[1];
    resource = hooks->resource_get(hooks->context,
                                   packed_surface & 0x0fu,
                                   (packed_surface >> 4) & 0x0fu);
    if (resource == NULL) return 0;
    if (!hooks->geometry_get(hooks->context, x, y, coordinates)) return 0;
    hooks->dispatch(hooks->context, resource, coordinates, packed_surface,
                    cell[3], param_a, param_b);
    return 1;
}

int terrain_cell_mutation_progress(const TerrainCellMutationGrid* grid,
                                   uint32_t x, uint32_t y,
                                   const TerrainCellMutationHooks* hooks)
{
    const TerrainMutationCell* cells[4];
    float heights[4];
    size_t index;
    float impact;
    unsigned int i;
    if (grid == NULL || hooks == NULL || grid->cells == NULL ||
        hooks->corner_heights_get == NULL || hooks->surface_apply == NULL ||
        grid->width < 2u || grid->height < 2u ||
        x >= grid->width - 1u || y >= grid->height - 1u) return 0;
    if ((size_t)grid->width > ((size_t)-1) / (size_t)grid->height ||
        (size_t)grid->width * (size_t)grid->height >
            ((size_t)-1) / sizeof(TerrainMutationCell)) return 0;
    index = (size_t)y * (size_t)grid->width + (size_t)x;
    cells[0] = &grid->cells[index];
    cells[1] = &grid->cells[index + 1u];
    cells[2] = &grid->cells[index + grid->width + 1u];
    cells[3] = &grid->cells[index + grid->width];
    if (!hooks->corner_heights_get(hooks->context, x, y, heights)) return 0;
    impact = grid->impact_a + grid->impact_b;
    if ((cells[0]->flags & 8u) != 0u) {
        for (i = 0; i < 4; ++i) heights[i] -= impact;
    }
    else {
        if ((cells[0]->flags & 0x10u) == 0u) return 0;
        for (i = 0; i < 4; ++i) {
            if (cells[i]->material == 1u) heights[i] -= impact;
        }
    }
    hooks->surface_apply(hooks->context, x, y, heights);
    return 1;
}

static TerrainCellMutationGrid s_active_mutation_grid;
static TerrainCellMutationHooks s_active_mutation_hooks;
static int s_active_mutation_context_bound;

void terrain_cell_mutation_context_bind(
    const TerrainCellMutationGrid* grid,
    const TerrainCellMutationHooks* hooks)
{
    if (grid == NULL || hooks == NULL) {
        memset(&s_active_mutation_grid, 0, sizeof(s_active_mutation_grid));
        memset(&s_active_mutation_hooks, 0, sizeof(s_active_mutation_hooks));
        s_active_mutation_context_bound = 0;
        return;
    }
    s_active_mutation_grid = *grid;
    s_active_mutation_hooks = *hooks;
    s_active_mutation_context_bound = 1;
}

void terrain_cell_mutation_progress_current(int x, int y)
{
    if (s_active_mutation_context_bound && x >= 0 && y >= 0) {
        (void)terrain_cell_mutation_progress(
            &s_active_mutation_grid, (uint32_t)x, (uint32_t)y,
            &s_active_mutation_hooks);
    }
}

int terrain_stamp_impact_handle(uint32_t x, uint32_t y, int direction,
                                int mode,
                                const TerrainStampImpactHooks* hooks)
{
    static const int neighbor_x[4] = { 1, 0, -1, 0 };
    static const int neighbor_y[4] = { 0, 1, 0, -1 };
    static const int effect_x[4] = { 0, 1, 0, -1 };
    static const int effect_y[4] = { 1, 0, -1, 0 };
    int cell_x, cell_y;
    int neighbors = 0;
    int i;
    float base_height, effect_height;
    if (hooks == NULL || hooks->cell_flags_get == NULL ||
        hooks->impact_cell_resolve == NULL || hooks->height_get == NULL ||
        hooks->effect_create == NULL || direction < 0 || direction >= 4)
        return 0;
    for (i = 0; i < 4; ++i) {
        if (hooks->cell_flags_get(hooks->context,
                (int)x + neighbor_x[i], (int)y + neighbor_y[i]) & 8u)
            ++neighbors;
    }
    if (!hooks->impact_cell_resolve(hooks->context, x, y, mode,
                                    &cell_x, &cell_y)) return 0;
    base_height = hooks->height_get(hooks->context, cell_x, cell_y, 0, 0);
    if (hooks->impact_event != NULL)
        hooks->impact_event(hooks->context, cell_x, cell_y, 0, 4,
                            base_height);
    effect_height = hooks->height_get(hooks->context, cell_x, cell_y,
                                      (float)effect_x[direction],
                                      (float)effect_y[direction]);
    if (!hooks->effect_create(hooks->context, neighbors > 1, x, y,
                              cell_x, cell_y, effect_height))
        return 0;
    if (hooks->cell_flags_or != NULL)
        hooks->cell_flags_or(hooks->context, x, y, 0x800u);
    return 1;
}

/* SurfaceTypeDescriptionsConfigLoad owns the configured surface-name/value
 * tables consumed by the surface-type query helpers below. */
#define SURFACE_TYPE_SLOTS 18
static const char* s_surface_type_names[SURFACE_TYPE_SLOTS];
// GLOBAL: LEGORR 0x00556320
static uint32_t s_surface_type_values[SURFACE_TYPE_SLOTS];

void surface_type_descriptions_load(SurfaceTypeDescriptionEntry entries[18],
                                    const SurfaceTypeDescriptionHooks* hooks)
{
    unsigned int i;
    if (entries == NULL || hooks == NULL || hooks->value_get == NULL ||
        hooks->parse == NULL) return;
    for (i = 0; i < 18u; ++i) {
        const char* value = hooks->value_get(hooks->context,
                                             entries[i].config_key);
        uint32_t parsed = 0;
        uint32_t flags = 0;
        char buffer[1024];
        size_t length = 0;
        if (value == NULL) continue;
        while (length < sizeof(buffer) - 1u && value[length] != '\0') {
            buffer[length] = value[length];
            ++length;
        }
        buffer[length] = '\0';
        (void)hooks->parse(hooks->context, buffer, &parsed, &flags);
        entries[i].parsed_value = parsed;
        entries[i].flags = flags;
        s_surface_type_values[i] = parsed;
    }
}

/* TileModelEntityRegister (FUN_0040b5f0): allocate two model slots plus two
 * 100-word interface arrays, load the no-texture models, scale and configure
 * them, then copy/reverse the second model's UVs. */
// FUNCTION: LEGORR 0x0040B5F0
TileModelEntityRegistration* tile_model_entity_register(
    void* manager, const char* first_entry, const char* second_entry,
    float scale_percent, int parameter,
    const TileModelEntityRegistrationHooks* hooks)
{
    TileModelEntityRegistration* registration;
    const char* names[2];
    unsigned int model_index;
    if (hooks == NULL || hooks->model_load == NULL || first_entry == NULL)
        return NULL;
    names[0] = first_entry;
    names[1] = second_entry != NULL ? second_entry : first_entry;
    registration = (TileModelEntityRegistration*)calloc(
        1, sizeof(TileModelEntityRegistration));
    if (registration == NULL) return NULL;
    registration->scale_integer = (int32_t)scale_percent;
    registration->registration_flags = 0u;
    registration->parameter = parameter;
    for (model_index = 0; model_index < 2u; ++model_index) {
        registration->models[model_index] = hooks->model_load(
            hooks->context, manager, (int)model_index, names[model_index], 1);
        if (registration->models[model_index] == NULL && model_index == 1u)
            registration->models[model_index] = hooks->model_load(
                hooks->context, manager, 1, first_entry, 1);
        if (registration->models[model_index] == NULL) {
            tile_model_entity_registration_release(registration, hooks);
            return NULL;
        }
    }
    for (model_index = 0; model_index < 2u; ++model_index) {
        void* model = registration->models[model_index];
        float* uv_records = NULL;
        int count = 0;
        unsigned int i;
        float scale = scale_percent * 0.01f;
        if (hooks->model_scale != NULL)
            hooks->model_scale(hooks->context, model, scale, scale, scale);
        if (hooks->model_mode_set != NULL)
            hooks->model_mode_set(hooks->context, model, 0, 1);
        if (hooks->model_mode_set != NULL)
            hooks->model_mode_set(hooks->context, model, 1, 1);
        if (hooks->mesh_uv_get != NULL)
            count = hooks->mesh_uv_get(hooks->context, model, &uv_records);
        if (count < 0) count = 0;
        if (model_index != 0u && uv_records != NULL) {
            for (i = 0; i < (unsigned int)count; ++i) {
                uv_records[i * 9u + 7u] = 1.0f - uv_records[i * 9u + 7u];
                uv_records[i * 9u + 8u] = 1.0f - uv_records[i * 9u + 8u];
            }
        }
        if (hooks->mesh_uv_set != NULL)
            hooks->mesh_uv_set(hooks->context, model, 0, count, uv_records);
        if (hooks->uv_records_release != NULL && uv_records != NULL)
            hooks->uv_records_release(hooks->context, uv_records);
    }
    return registration;
}

void tile_model_entity_registration_release(
    TileModelEntityRegistration* registration,
    const TileModelEntityRegistrationHooks* hooks)
{
    unsigned int i;
    if (registration == NULL) return;
    if (hooks != NULL && hooks->model_release != NULL) {
        for (i = 0; i < 2u; ++i) {
            if (registration->models[i] != NULL)
                hooks->model_release(hooks->context,
                                     registration->models[i]);
        }
    }
    free(registration);
}

typedef struct TerrainProjectedCellCandidate {
    int x;
    int y;
    float distance;
    float metric;
} TerrainProjectedCellCandidate;

uint32_t terrain_projected_cell_state_refresh(
    const TerrainProjectedCellRefreshState* state,
    const TerrainProjectedCellRefreshHooks* hooks)
{
    TerrainProjectedCellCandidate* deferred;
    uint32_t deferred_count = 0, immediate_count = 0, accepted = 0;
    int x_min, x_max, y_min, y_max;
    int x, y;
    size_t capacity;
    if (state == NULL || hooks == NULL || hooks->cell_flags_get == NULL ||
        hooks->candidate_measure == NULL || state->map_width == 0u ||
        state->map_height == 0u || state->radius < 0) return 0;
    x_min = state->center_x - state->radius;
    y_min = state->center_y - state->radius;
    x_max = state->center_x + state->radius;
    y_max = state->center_y + state->radius;
    if (x_min < 0) x_min = 0;
    if (y_min < 0) y_min = 0;
    if (x_max >= (int)state->map_width) x_max = (int)state->map_width - 1;
    if (y_max >= (int)state->map_height) y_max = (int)state->map_height - 1;
    if (x_min >= x_max || y_min >= y_max) return 0;
    capacity = (size_t)(x_max - x_min) * (size_t)(y_max - y_min);
    deferred = (TerrainProjectedCellCandidate*)malloc(
        capacity * sizeof(*deferred));
    if (deferred == NULL) return 0;
    for (y = y_min; y < y_max; ++y) {
        for (x = x_min; x < x_max; ++x) {
            float distance, facing, metric;
            uint32_t flags = hooks->cell_flags_get(hooks->context, x, y);
            if ((flags & 0x18u) == 0u) continue;
            if (!hooks->candidate_measure(hooks->context, x, y,
                                          &distance, &facing, &metric) ||
                distance >= state->distance_limit ||
                facing <= state->facing_limit) continue;
            if (metric <= 0.0f) {
                ++immediate_count;
                continue;
            }
            deferred[deferred_count].x = x;
            deferred[deferred_count].y = y;
            deferred[deferred_count].distance = distance;
            deferred[deferred_count].metric = metric;
            ++deferred_count;
        }
    }
    for (x = 0; x < (int)deferred_count; ++x) {
        TerrainProjectedCellCandidate* candidate = &deferred[x];
        uint32_t flags = hooks->cell_flags_get(hooks->context,
                                               candidate->x, candidate->y);
        int forced = accepted >= state->deferred_limit ||
            candidate->distance >= state->distance_limit * 0.6666667f;
        int overlay = 0;
        if (!forced && state->overlay_enabled != 0u &&
            hooks->cell_overlay_update != NULL)
            overlay = hooks->cell_overlay_update(hooks->context,
                                                 candidate->x, candidate->y);
        if (!forced && overlay) {
            if (hooks->cell_flags_set != NULL)
                hooks->cell_flags_set(hooks->context, candidate->x,
                                      candidate->y, flags | 0x100u);
            ++accepted;
        }
        else {
            if (hooks->cell_flags_set != NULL)
                hooks->cell_flags_set(hooks->context, candidate->x,
                                      candidate->y, flags & ~0x100u);
            if (hooks->cell_refresh != NULL)
                hooks->cell_refresh(hooks->context, candidate->x,
                                    candidate->y);
            if ((flags & 0x4000u) != 0u && hooks->cell_sound != NULL)
                hooks->cell_sound(hooks->context, candidate->x,
                                  candidate->y);
        }
        if (hooks->frame_tick != NULL) hooks->frame_tick(hooks->context);
    }
    while (immediate_count-- != 0u) {
        if (hooks->frame_tick != NULL) hooks->frame_tick(hooks->context);
    }
    free(deferred);
    return accepted;
}

void dock_definition_cell_get_hook_install(DockDefinitionCellGetHook hook,
                                          void* context)
{
    s_dock_definition_cell_get_hook = hook;
    s_dock_definition_cell_get_context = context;
}

// FUNCTION: LEGORR 0x00438f60
int dock_definition_matches_cell(const int cell_xy[2])
{
    int dock_xy[2];
    if (cell_xy == NULL || s_dock_definition_cell_get_hook == NULL) return 0;
    if (!s_dock_definition_cell_get_hook(dock_xy,
                                         s_dock_definition_cell_get_context)) {
        return 0;
    }
    return dock_xy[0] == cell_xy[0] && dock_xy[1] == cell_xy[1];
}
static TerrainCellLinkedEntryRemoveHook s_terrain_cell_linked_entry_remove_hook;
static void* s_terrain_cell_linked_entry_remove_context;

void terrain_neighbor_flag_hooks_install(
    TerrainNeighborPresenceHook presence,
    TerrainNeighborRenderUpdateHook render_update, void* context)
{
    s_terrain_neighbor_presence_hook = presence;
    s_terrain_neighbor_render_hook = render_update;
    s_terrain_neighbor_hook_context = context;
}

void terrain_neighbor_refresh_hooks_install(
    TerrainNeighborActionHook build_action,
    TerrainNeighborActionHook clear_action, void* context)
{
    s_terrain_neighbor_build_action = build_action;
    s_terrain_neighbor_clear_action = clear_action;
    s_terrain_neighbor_action_context = context;
}

void terrain_cell_variant_complete_hook_install(
    TerrainCellVariantCompleteHook hook, void* context)
{
    s_terrain_cell_variant_complete_hook = hook;
    s_terrain_cell_variant_complete_context = context;
}

void terrain_cell_state_projection_hook_install(
    TerrainCellStateProjectionHook hook, void* context)
{
    s_terrain_cell_state_projection_hook = hook;
    s_terrain_cell_state_projection_context = context;
}

void terrain_cell_linked_entry_remove_hook_install(
    TerrainCellLinkedEntryRemoveHook hook, void* context)
{
    s_terrain_cell_linked_entry_remove_hook = hook;
    s_terrain_cell_linked_entry_remove_context = context;
}

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
    if (s == NULL) return;
    /* The level owns no scene manager for CellInitPass's activity, effects,
     * and mesh updates. Keep the live terrain's physical cell dirty so the
     * terrain renderer can consume the refresh when its owner is attached. */
    terrain_cell_changed_flag_set(s->ptr_terrain, cx, cy);
}

/* MapCellTrackingFlagsClearAndRefresh (FUN_004302b0): clear the cell's
 * tracking/variant bits, update the entity activity scene state, run the
 * coordinate callback over the live pool, and preserve the FLIC fallback
 * call (a pure return in the reference). */
// FUNCTION: LEGORR 0x004302B0
void map_cell_tracking_flags_clear_and_refresh(LevelState* s,
                                               const int cell_xy[2])
{
    uint8_t* rec;
    uint32_t* flags;
    if (s == NULL || cell_xy == NULL) {
        return;
    }
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    flags = (uint32_t*)(rec + 0x08);
    *flags &= 0xdfffffffu;
    *flags &= 0xbfffffffu;
    entity_activity_scene_state_update(entity_pool_callback_context_get());
    coordinate_callback_dispatch(cell_xy, 0);
    flic_opcode27_fallback_noop();
}

/* TerrainCellStateUpdate (FUN_00432c90): the retail entry takes only the
 * coordinate pair and reads the active level global. Keep the explicit-level
 * worker for portable callers; the mapped entry below supplies the live
 * GameContext owner. */
int terrain_cell_state_update(LevelState* s, const int cell_xy[2])
{
    uint8_t* rec;
    uint32_t* flags;
    uint32_t value;
    float position[3];
    if (s == NULL || cell_xy == NULL) {
        return 0;
    }
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return 0;
    }
    flags = (uint32_t*)(rec + 0x08);
    value = *flags;
    if ((value & 0x200000u) != 0u) {
        return 1;
    }
    {
        uint32_t state = value & 3u;
        if (state != 0u) {
            *flags = (*flags & ~3u) | ((state - 1u) & 3u);
        }
        else {
            *flags |= 0x200000u;
            if (s_terrain_cell_linked_entry_remove_hook != NULL) {
                s_terrain_cell_linked_entry_remove_hook(
                    cell_xy, s_terrain_cell_linked_entry_remove_context);
            }
            selection_mode5_cancel_at_point((const uint32_t*)cell_xy);
            coordinate_callback_dispatch_by_mode(cell_xy, 0);
            if (terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                                position) != 0) {
                position[2] = (float)terrain_surface_height_get(
                    s->ptr_terrain, cell_xy[0], cell_xy[1]);
                if (s_terrain_cell_state_projection_hook != NULL) {
                    s_terrain_cell_state_projection_hook(
                        cell_xy, position,
                        s_terrain_cell_state_projection_context);
                }
            }
        }
        cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
        return state == 0u;
    }
}

// FUNCTION: LEGORR 0x00432C90
int terrain_cell_state_update_current(const int cell_xy[2])
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* flags;
    uint32_t value;
    uint32_t state;
    float position[3];
    if (game == NULL || cell_xy == NULL) return 0;
    s = game->level;
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) return 0;
    flags = (uint32_t*)(rec + 0x08);
    value = *flags;
    if ((value & 0x200000u) != 0u) return 1;
    state = value & 3u;
    if (state != 0u) {
        *flags = (*flags & ~3u) | ((state - 1u) & 3u);
    }
    else {
        *flags |= 0x200000u;
        if (s_terrain_cell_linked_entry_remove_hook != NULL) {
            s_terrain_cell_linked_entry_remove_hook(
                cell_xy, s_terrain_cell_linked_entry_remove_context);
        }
        selection_mode5_cancel_at_point((const uint32_t*)cell_xy);
        coordinate_callback_dispatch_by_mode(cell_xy, 0);
        if (terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                            position) != 0) {
            position[2] = (float)terrain_surface_height_get(
                s->ptr_terrain, cell_xy[0], cell_xy[1]);
            if (s_terrain_cell_state_projection_hook != NULL) {
                s_terrain_cell_state_projection_hook(
                    cell_xy, position,
                    s_terrain_cell_state_projection_context);
            }
        }
    }
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
    return state == 0u;
}

/* TerrainNeighborFlagSet (FUN_00430470): for an interior, flagged cell,
 * query the spatial neighbor helper; on a match move the primary flag from
 * bit 3 to bit 4, refresh the cell, and update its render-side neighbor. */
// FUNCTION: LEGORR 0x00430470
void terrain_neighbor_flag_set(LevelState* s, int manager,
                               uint32_t x, uint32_t y)
{
    uint8_t* rec;
    uint32_t* flags;
    (void)manager;
    if (s == NULL || x == 0u || y == 0u || level_width(s) < 2 ||
        level_height(s) < 2 || x >= (uint32_t)level_width(s) - 2u ||
        y >= (uint32_t)level_height(s) - 2u) {
        return;
    }
    rec = level_voxel(s, (int)x, (int)y);
    if (rec == NULL) {
        return;
    }
    flags = (uint32_t*)(rec + 0x08);
    if ((*flags & 8u) == 0u || s_terrain_neighbor_presence_hook == NULL ||
        s_terrain_neighbor_presence_hook(x, y, 0,
                                         s_terrain_neighbor_hook_context) == 0) {
        return;
    }
    *flags &= ~8u;
    *flags |= 0x10u;
    cell_refresh_seam(s, (int)x, (int)y);
    if (s_terrain_neighbor_render_hook != NULL) {
        s_terrain_neighbor_render_hook(x, y,
                                       s_terrain_neighbor_hook_context);
    }
}

/* TerrainNeighborRefresh (FUN_00430df0): inspect the first eligible
 * bit-3 neighbor in north/east/south/west order. Current state 8 requests
 * the build action; state 10 requests the clear action. */
// FUNCTION: LEGORR 0x00430DF0
void terrain_neighbor_refresh(LevelState* s, const int cell_xy[2])
{
    static const int offsets[4][2] = {
        {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    uint8_t* current;
    int x, y, i;
    int neighbor[2];
    uint8_t* found = NULL;
    if (s == NULL || cell_xy == NULL) {
        return;
    }
    x = cell_xy[0];
    y = cell_xy[1];
    for (i = 0; i < 4; ++i) {
        neighbor[0] = x + offsets[i][0];
        neighbor[1] = y + offsets[i][1];
        found = level_voxel(s, neighbor[0], neighbor[1]);
        if (found != NULL &&
            ((*(uint32_t*)(found + 0x08) & 8u) != 0u) &&
            found[2] != 9 && found[2] != 6) {
            break;
        }
        found = NULL;
    }
    if (found == NULL) {
        return;
    }
    current = level_voxel(s, x, y);
    if (current == NULL) {
        return;
    }
    if (current[2] == 8 && s_terrain_neighbor_build_action != NULL) {
        s_terrain_neighbor_build_action(neighbor,
                                        s_terrain_neighbor_action_context);
    } else if (current[2] == 10 &&
               s_terrain_neighbor_clear_action != NULL) {
        s_terrain_neighbor_clear_action(neighbor,
                                        s_terrain_neighbor_action_context);
    }
}

/* TerrainCellVariantFlagPropagate (FUN_00432600) takes one coordinate pair
 * and reads the active level global. */
int terrain_cell_variant_flag_propagate(LevelState* s, const int cell_xy[2])
{
    static const int offsets[5][2] = {
        {0, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    uint8_t* rec;
    uint32_t* flags;
    uint32_t old_flags;
    int i;
    if (s == NULL || cell_xy == NULL) {
        return 0;
    }
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return 0;
    }
    flags = (uint32_t*)(rec + 0x08);
    old_flags = *flags;
    if ((old_flags & 0x200000u) == 0u || rec[2] == 6) {
        return 0;
    }
    if ((old_flags & 0x80000000u) == 0u) {
        *flags |= 0x20000000u;
    } else {
        lava_erosion_pending_cell_add(cell_xy, 0);
        rec[7] = 0;
    }
    *flags &= 0xbfffffffu;
    for (i = 0; i < 5; ++i) {
        cell_init_pass(s,
            (float)(cell_xy[0] + offsets[i][0]),
            (uint32_t)(cell_xy[1] + offsets[i][1]));
    }
    entity_activity_scene_state_update(entity_pool_callback_context_get());
    return 1;
}

// FUNCTION: LEGORR 0x00432600
int terrain_cell_variant_flag_propagate_current(const int cell_xy[2])
{
    static const int offsets[5][2] = {
        {0, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* flags;
    uint32_t old_flags;
    int i;
    if (game == NULL || cell_xy == NULL) return 0;
    s = game->level;
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) return 0;
    flags = (uint32_t*)(rec + 0x08);
    old_flags = *flags;
    if ((old_flags & 0x200000u) == 0u || rec[2] == 6u) return 0;
    if ((old_flags & 0x80000000u) == 0u) {
        *flags |= 0x20000000u;
    }
    else {
        lava_erosion_pending_cell_add(cell_xy, 0);
        rec[7] = 0;
    }
    *flags &= 0xbfffffffu;
    for (i = 0; i < 5; ++i) {
        cell_init_pass(s,
            (float)(cell_xy[0] + offsets[i][0]),
            (uint32_t)(cell_xy[1] + offsets[i][1]));
    }
    entity_activity_scene_state_update(game);
    return 1;
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
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

/* The target reads the current level from DAT_005560d4 and receives the
 * coordinate pair by pointer. GameContext is the live owner for that global
 * in this port; retain the explicit-LevelState adapter above for callers. */
// FUNCTION: LEGORR 0x00432100
void terrain_cell_flag100_set_and_track_current(const int cell_xy[2])
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* word;
    if (game == NULL || game->level == NULL || cell_xy == NULL) {
        return;
    }
    s = game->level;
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    word = (uint32_t*)(rec + 0x0C);
    *word = *word | 0x100u;
    cell_refresh_track_push(cell_xy[0], cell_xy[1]);
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
}

// FUNCTION: LEGORR 0x00432270
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

// FUNCTION: LEGORR 0x00432300
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
    /* Corpus 0x432385-0x43238d loads the dword into esi, masks it, then
     * stores it back. Keep that separate RMW sequence under /Od. */
    uint32_t primary_value = *primary;
    primary_value &= 0xdfef7bffu;
    *primary = primary_value;
    uint32_t* secondary = (uint32_t*)(rec + 0x0C);
    /* Same load/and/store shape for the secondary word at 0x4323a7-0x4323ac. */
    uint32_t secondary_value = *secondary;
    secondary_value &= 0xfffffffbu;
    *secondary = secondary_value;
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

// FUNCTION: LEGORR 0x00432360
void terrain_cell_primary_flags_mask_clear_and_refresh_current(
    const int cell_xy[2])
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* primary;
    uint32_t* secondary;
    uint32_t value;
    if (game == NULL || game->level == NULL || cell_xy == NULL) {
        return;
    }
    s = game->level;
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    primary = (uint32_t*)(rec + 0x08);
    value = *primary;
    value &= 0xdfef7bffu;
    *primary = value;
    secondary = (uint32_t*)(rec + 0x0C);
    value = *secondary;
    value &= 0xfffffffbu;
    *secondary = value;
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
}

void terrain_cell_flags_set_and_refresh(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    /* Corpus 0x432415-0x43241d uses load/or/store, then repeats the
     * operation for the secondary word. */
    uint32_t primary_value = *primary;
    primary_value |= 0x108000u;
    *primary = primary_value;
    uint32_t* secondary = (uint32_t*)(rec + 0x0C);
    uint32_t secondary_value = *secondary;
    secondary_value |= 4u;
    *secondary = secondary_value;
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

// FUNCTION: LEGORR 0x004323f0
void terrain_cell_flags_set_and_refresh_current(const int cell_xy[2])
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* primary;
    uint32_t* secondary;
    uint32_t value;
    if (game == NULL || game->level == NULL || cell_xy == NULL) {
        return;
    }
    s = game->level;
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    primary = (uint32_t*)(rec + 0x08);
    value = *primary;
    value |= 0x108000u;
    *primary = value;
    secondary = (uint32_t*)(rec + 0x0C);
    value = *secondary;
    value |= 4u;
    *secondary = value;
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
}

// FUNCTION: LEGORR 0x00432450
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

// FUNCTION: LEGORR 0x00432490
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
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

// FUNCTION: LEGORR 0x00432560
void terrain_cell_flag40000000_clear_or_set_current(const int cell_xy[2],
                                                     int set)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* primary;
    uint32_t value;
    if (game == NULL || game->level == NULL || cell_xy == NULL) {
        return;
    }
    s = game->level;
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    primary = (uint32_t*)(rec + 0x08);
    value = *primary;
    if (set == 0) {
        value &= 0xbfffffffu;
    } else {
        value |= 0x40000000u;
    }
    *primary = value;
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
}

/* CellNeighborDirectionCode (FUN_004508a0): eight neighbors, -1 else. */
// FUNCTION: LEGORR 0x004508a0
int cell_neighbor_direction_code(int x1, int y1, int x2, int y2)
{
    int result = -1;
    if (x1 == x2) {
        if (y2 == y1 - 1) {
            return 0;
        }
        if (y2 == y1 + 1) {
            return 2;
        }
    }
    else {
        const int x1_next = x1 + 1;
        if (y1 == y2) {
            if (x1_next == x2) {
                return 1;
            }
            if (x1 - 1 == x2) {
                return 3;
            }
        }
        else {
            if (x1_next == x2) {
                if (y1 - 1 == y2) {
                    return 4;
                }
                if (x1_next == x2 && y1 + 1 == y2) {
                    return 5;
                }
            }
            if (x1 - 1 == x2) {
                if (y1 + 1 == y2) {
                    return 6;
                }
                if (x1 - 1 == x2 && y1 - 1 == y2) {
                    result = 7;
                }
            }
        }
    }
    return result;
}

/* --- neighbor-refresh / circle / edge-flag cluster (see level.hpp) ------- */

// FUNCTION: LEGORR 0x00431450
void terrain_neighbor_flag_refresh(LevelState* s, const int cell_xy[2],
                                   int mode)
{
    TerrainActivityBuildRecordModel* build;
    int cx;
    int cy;
    cx = cell_xy[0];
    cy = cell_xy[1];
    if (level_voxel(s, cx, cy) == NULL) {
        return; /* bounds deviation (as across this module) */
    }
    build = (TerrainActivityBuildRecordModel*)
        terrain_activity_build_record_get(s, cx, cy);
    if (build == NULL) {
        return;
    }
    if (mode == 0 && build->enabled == 0) {
        /* SceneActionNameSet("Activity_Destroy") and SceneFrameSet(0)
         * require the scene-object handle owned by the corpus record.
         * The sidecar retains the decompiled record byte at +0x0c, but the
         * portable level has no scene object to receive those calls. */
        build->scene_flags_0c |= 4u;
        return;
    }
    build->scene_flags_0c |= 2u;
}

// FUNCTION: LEGORR 0x004310f0
void terrain_cell_flag20_clear_and_refresh(LevelState* s, int cx, int cy)
{
    terrain_neighbor_flag_refresh_xy(s, cx, cy, 0);
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec != NULL) {
        uint32_t* word = (uint32_t*)(rec + 0x08);
        *word &= 0xffffffdfu;
    }
    cell_refresh_seam(s, cx, cy);
}

// FUNCTION: LEGORR 0x0042f2e0
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

static TerrainEdgeMidpointUpdateFn s_terrain_edge_midpoint_update_hook;
static void* s_terrain_edge_midpoint_update_user;

void terrain_edge_midpoint_update_hook_set(TerrainEdgeMidpointUpdateFn hook,
                                           void* user)
{
    s_terrain_edge_midpoint_update_hook = hook;
    s_terrain_edge_midpoint_update_user = user;
}

/* FUNCTION: LEGORR 0x0046e610. The edge mesh/scene resource pass is
 * delegated to the renderer through its owning level hook. */
void terrain_edge_flag_midpoint_update(LevelState* level, float delta)
{
    if (s_terrain_edge_midpoint_update_hook != NULL) {
        s_terrain_edge_midpoint_update_hook(level, delta,
                                            s_terrain_edge_midpoint_update_user);
    }
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

/* Retail ABI: pointer to center pair, then radius; level comes from the
 * process-global current level. */
// FUNCTION: LEGORR 0x00435010
void terrain_cell_work_flag_circle_set_current(const int center_xy[2],
                                               int radius)
{
    GameContext* game = entity_pool_callback_context_get();
    int width, height, radius_squared;
    int row;
    if (game == NULL || game->level == NULL || center_xy == NULL) return;
    width = level_int(game->level, LEVEL_OFF_WIDTH);
    height = level_int(game->level, LEVEL_OFF_HEIGHT);
    radius_squared = radius * radius;
    for (row = -radius; row <= radius; ++row) {
        int column;
        for (column = -radius; column <= radius; ++column) {
            const int x = center_xy[0] + column;
            const int y = center_xy[1] + row;
            if (column * column + row * row <= radius_squared &&
                x >= 0 && x < width && y >= 0 && y < height) {
                terrain_cell_work_flag_set_current((uint32_t)x,
                                                   (uint32_t)y);
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
                if ((int64_t)row0 - 1 <= (int64_t)row0 + 1) { /* corpus's wrap guard */
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

/* Retail ABI is two unsigned coordinates. The level is the live global, not
 * an explicit first parameter; keep the three-argument form above as the
 * portable adapter for isolated level callers. */
// FUNCTION: LEGORR 0x00431580
void terrain_neighbor_work_flag_scan_current(unsigned int seed_x,
                                             unsigned int seed_y)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    int w;
    int h;
    int span;
    float* buf;
    float* page[2];
    unsigned int count[2];
    int cur;
    if (game == NULL || game->level == NULL) return;
    s = game->level;
    w = level_width(s);
    h = level_height(s);
    span = w + h;
    if (span <= 0) return;
    buf = (float*)engine_heap_alloc((long)span * 0x20);
    if (buf == NULL) return;
    page[0] = buf;
    page[1] = buf + span * 4;
    count[0] = 1u;
    count[1] = 0u;
    page[0][0] = (float)seed_x;
    page[0][1] = (float)seed_y;
    cur = 0;
    do {
        if (count[cur] != 0u) {
            const float* pf = page[cur];
            unsigned int idx;
            for (idx = 0; idx < count[cur]; ++idx, pf += 2) {
                const int col0 = level_cell_of(pf[0]);
                const int row0 = level_cell_of(pf[1]);
                int row;
                for (row = row0 - 1; row <= row0 + 1; ++row) {
                    int col;
                    for (col = col0 - 1; col <= col0 + 1; ++col) {
                        uint8_t* rec = level_voxel(s, col, row);
                        uint32_t word = 0;
                        if (rec != NULL)
                            __builtin_memcpy(&word, rec + 0x08, sizeof(word));
                        if ((word & 0x20000u) != 0u) {
                            /* The renderer/wave operation in FUN_00430530
                             * has no target-compatible owner here. Preserve
                             * the known work-bit clear and queue traversal. */
                            word &= 0xfffdffffu;
                            if (rec != NULL)
                                __builtin_memcpy(rec + 0x08, &word,
                                                 sizeof(word));
                            {
                                const unsigned int n = count[1 - cur];
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
        count[1 - cur] = 0u;
    } while (count[cur] != 0u);
    engine_heap_free(buf);
}

/* --- cell timestamp lock table (see level.hpp) --------------------------
 * Corpus regions: the (x,y) pair slots (stride 8) end at the per-entry
 * active flags (stride 4), the stamps live in their own stride-4 region,
 * and the scan bound is the first address past the flag region — all
 * 1000 entries. */
#define LEVEL_TS_COUNT 1000

typedef struct CellCoordinateTimestamp {
    int32_t x;
    int32_t y;
} CellCoordinateTimestamp;

typedef char cell_coordinate_timestamp_stride_check[
    sizeof(CellCoordinateTimestamp) == 8u ? 1 : -1];

// GLOBAL: LEGORR 0x004d3a34
static CellCoordinateTimestamp s_ts_coordinates[LEVEL_TS_COUNT];
// GLOBAL: LEGORR 0x004d5974
static int32_t  s_ts_stamps[LEVEL_TS_COUNT];
// GLOBAL: LEGORR 0x004d6914
static uint32_t s_ts_active[LEVEL_TS_COUNT];
// GLOBAL: LEGORR 0x004d78c0
static int32_t  s_ts_now; /* _DAT_004d78c0: ErodeLockTime * 25 float bits */

// FUNCTION: LEGORR 0x0040ee00
void cell_coordinate_timestamp_remember(const int xy[2])
{
    volatile uint32_t* active = s_ts_active;
    int index = 0;
    do {
        if (*active == 0u) {
            s_ts_coordinates[index].x = xy[0];
            s_ts_coordinates[index].y = xy[1];
            s_ts_stamps[index] = s_ts_now;
            *active = 1u;
            return;
        }
        ++active;
        ++index;
    } while (active < s_ts_active + LEVEL_TS_COUNT);
    /* full table: stores nothing (faithful) */
}

// FUNCTION: LEGORR 0x0040efb0
uint32_t cell_coordinate_timestamp_exists(const int xy[2])
{
    uint32_t* active = s_ts_active;
    CellCoordinateTimestamp* coordinate = s_ts_coordinates;
    if (xy == NULL) return 0u;
    while (*active == 0u || coordinate->x != xy[0] ||
           coordinate->y != xy[1]) {
        ++active;
        ++coordinate;
        if (active >= s_ts_active + LEVEL_TS_COUNT) return 0u;
    }
    return 1u;
}

/* --- terrain dirty queue (see level.hpp) ---------------------------------
 * The corpus's queue lives inside its terrain context (u16 pair entries,
 * stride 4, capacity (0x7198-0x4a88)/4 = 2500, count at +0x7198). */
#define LEVEL_DIRTY_QUEUE_CAP 2500

static int16_t s_dirty_queue_x[LEVEL_DIRTY_QUEUE_CAP];
static int16_t s_dirty_queue_y[LEVEL_DIRTY_QUEUE_CAP];
static int     s_dirty_queue_count;

// FUNCTION: LEGORR 0x00450960
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

// FUNCTION: LEGORR 0x00450a80
void cell_list_min_distance_scale_apply(int enable) /* FUN_00450a80 */
{
    if (enable != 0 && (s_cell_list_flags & 4u) == 0) {
        /* The queue count is nonnegative (zero-init, bounded append, and
         * remove-only-after-find), matching the corpus's unsigned gate. */
        for (uint32_t i = 0u; i < (uint32_t)s_dirty_queue_count; ++i) {
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
        /* Same bounded, nonnegative queue-count invariant as the apply arm. */
        for (uint32_t i = 0u; i < (uint32_t)s_dirty_queue_count; ++i) {
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

// GLOBAL: LEGORR 0x004b8bc8
static uint32_t s_grid_rec[GRID_REC_REGION_DWORDS];

/* Flat dword index of a block's record-pointer word, or -1 when the
 * coordinate cannot address the region (bounds deviation). */
static int64_t grid_rec_block_index(int x, int y)
{
    const int64_t idx = (int64_t)y + (int64_t)x * GRID_REC_COLS;
    const int64_t off = 32 + idx * GRID_REC_STRIDE;
    if (idx < 0 || off < 0 || off >= GRID_REC_REGION_DWORDS) {
        return -1;
    }
    return off;
}

/* Flat dword index of a block field word, or -1. */
static int64_t grid_rec_field_index(int x, int y, int field)
{
    const int64_t off = grid_rec_block_index(x, y);
    if (off < 0 || field < 0) {
        return -1;
    }
    const int64_t word = off + 1 + field;
    if (word >= GRID_REC_REGION_DWORDS) {
        return -1;
    }
    return word;
}

// FUNCTION: LEGORR 0x0040ab30
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

// FUNCTION: LEGORR 0x0040af90
void terrain_cell_record_get(int x, int y, int field,
                             uint32_t* value_a, uint32_t* value_b)
{
    const int64_t block = grid_rec_block_index(x, y);
    if (block < 0) {
        return;
    }
    const uint32_t variant = s_grid_rec[(size_t)block];
    const int64_t off = block + (variant != 0u ? field : 0);
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

/* The cell-entry readiness bits occupy bytes at DAT_004b8c4c, inside the
 * mapped grid-record region. This source model has no writer for nonempty
 * resource-entry lists, so the current record accessor supplies count zero;
 * nonempty lists reject until the cell resource records are modeled. */
#define CELL_ENTRY_SLOT_TYPES 16
#define CELL_ENTRY_SLOT_ROWS 15
#define CELL_ENTRY_SLOT_COUNT 49

// FUNCTION: LEGORR 0x0040AEA0
int ensure_cell_entries_loaded(uint32_t kind, int row, int slot)
{
    if (global_mode_zero_input_peek() != 0u) {
        return 1;
    }
    if (kind >= CELL_ENTRY_SLOT_TYPES || row < 0
        || row >= CELL_ENTRY_SLOT_ROWS || slot < 0
        || slot >= CELL_ENTRY_SLOT_COUNT) {
        return 0;
    }
    const size_t index = ((size_t)kind * CELL_ENTRY_SLOT_ROWS
                          + (size_t)row) * CELL_ENTRY_SLOT_COUNT
                         + (size_t)slot;
    uint8_t* const ready = (uint8_t*)s_grid_rec + 0x84u + index;
    if ((*ready & 2u) != 0u) {
        return 1;
    }
    uint32_t records = 0u;
    uint32_t count = 0u;
    terrain_cell_record_get((int)kind, row, slot, &records, &count);
    (void)records;
    if (count != 0u) {
        return 0;
    }
    if ((*ready & 1u) != 0u) {
        *ready |= 2u;
    }
    return 1;
}

// FUNCTION: LEGORR 0x0040b250
uint32_t cell_flag4_is_set(int x, int y, int field)
{
    const int64_t block = grid_rec_block_index(x, y);
    if (block < 0) {
        return 0;
    }
    const int64_t word = (s_grid_rec[(size_t)block] != 0u)
        ? grid_rec_field_index(x, y, field)
        : grid_rec_field_index(x, y, 0);
    if (word < 0) {
        return 0;
    }
    return s_grid_rec[(size_t)word] & 4u;
}

// FUNCTION: LEGORR 0x0040b2a0
void cell_flag100_set(int x, int y, int field)
{
    const int64_t word = grid_rec_field_index(x, y, field);
    if (word >= 0) {
        s_grid_rec[(size_t)word] |= 0x100u;
    }
}

// FUNCTION: LEGORR 0x0040b2e0
uint32_t cell_flag100_is_set(int x, int y, int field)
{
    const int64_t word = grid_rec_field_index(x, y, field);
    if (word < 0) {
        return 0;
    }
    return s_grid_rec[(size_t)word] & 0x100u;
}

/* The present table lands NULL: its heap owner (the grid-initialize
 * leaf, address deliberately unquoted) is unported. */
static uint32_t* s_grid_present_table;
// GLOBAL: LEGORR 0x004c7dfc
static const uint32_t* s_grid_record_manager;

// FUNCTION: LEGORR 0x0040e460
int grid_cell_record_present_check(int x, int y)
{
    if (s_grid_present_table == NULL || s_grid_record_manager == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    const int w = (int)s_grid_record_manager[5]; /* +0x14 */
    const int64_t idx = (int64_t)w * y + x;
    if (idx < 0) {
        return 0;
    }
    return s_grid_present_table[(size_t)idx * 3] != 0u; /* stride 0xc */
}

/* --- corner normal sampling (see level.hpp) ------------------------------- */
// FUNCTION: LEGORR 0x004512d0
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
            sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
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
// GLOBAL: LEGORR 0x005562d8
// GLOBAL: LEGORR 0x00556320

// FUNCTION: LEGORR 0x00435170
const char* terrain_surface_type_name_get(int index)
{
    static const char missing[] = "Need config for surface type";
    if (index < 0 || index >= SURFACE_TYPE_SLOTS
        || s_surface_type_names[index] == NULL) {
        return missing;
    }
    return s_surface_type_names[index];
}

// FUNCTION: LEGORR 0x00435190
uint32_t surface_type_configured_value_at_index(int index)
{
    if (index < 0 || index >= SURFACE_TYPE_SLOTS) {
        return 0; /* bounds deviation (corpus indexes blindly) */
    }
    return s_surface_type_values[index];
}

/* --- neighbor coordinate classification (see level.hpp) ------------------- */
// FUNCTION: LEGORR 0x0042f350
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
 * The corpus's two adjacent tables (DAT pairs 0x4e3944/0x4dee44) as two
 * zero-initialized module statics: no ported code feeds them. The first
 * table is at 0x4e3944; the second, read as the difference's minuend, is at
 * 0x4dee44. */
#define GRID_COUNT_TABLE_DWORDS 0x12c0 /* 300 cells * 16 slots */

// GLOBAL: LEGORR 0x004e3944
static uint32_t s_grid_count_first[GRID_COUNT_TABLE_DWORDS];
// GLOBAL: LEGORR 0x004dee44
static uint32_t s_grid_count_second[GRID_COUNT_TABLE_DWORDS];

static int64_t grid_count_index(int row, int column, int slot)
{
    const int64_t cell = (int64_t)row * 15 + column;
    const int64_t idx = cell * 16 + slot;
    if (cell < 0 || slot < 0 || idx >= GRID_COUNT_TABLE_DWORDS) {
        return -1; /* bounds deviation (corpus indexes blindly) */
    }
    return idx;
}

// FUNCTION: LEGORR 0x00437440
void grid_record_count_tables_clear(void)
{
    for (size_t k = 0; k < GRID_COUNT_TABLE_DWORDS; ++k) {
        s_grid_count_first[k] = 0u;
    }
    for (size_t second_index = 0; second_index < GRID_COUNT_TABLE_DWORDS;
         ++second_index) {
        s_grid_count_second[second_index] = 0u;
    }
}

// FUNCTION: LEGORR 0x00437460
int grid_record_count_difference_get(int row, int column, int slot,
                                     int subtract_first_table)
{
    const int64_t idx = grid_count_index(row, column, slot);
    if (idx < 0) {
        return 0;
    }
    const int v = (int)s_grid_count_second[(size_t)idx];
    return (subtract_first_table != 0)
        ? v - (int)s_grid_count_first[(size_t)idx]
        : v;
}

// FUNCTION: LEGORR 0x004374a0
uint32_t grid_record_count_get(int row, int column, int slot)
{
    const int64_t idx = grid_count_index(row, column, slot);
    return (idx < 0) ? 0u : s_grid_count_first[(size_t)idx];
}

/* FUNCTION: LEGORR 0x00455300
 * Resolve a configured object type to its grid-record table coordinates and
 * return the selected first-table count. */
uint32_t entity_type_grid_record_count_get(const char* type_name, int slot)
{
    int type = 0;
    int row = 0;
    if (!entity_type_row_for_name(type_name, &type, &row)) return 0u;
    return grid_record_count_get(type, row, slot);
}

int32_t grid_count_table_slot_peek(int row, int column, int slot, int table)
{
    const int64_t idx = grid_count_index(row, column, slot);
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

// FUNCTION: LEGORR 0x0044f5c0
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

// FUNCTION: LEGORR 0x0044f4e0
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

// FUNCTION: LEGORR 0x0044f570
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

/* Retail ABI: coordinate pair plus mode, current level fetched internally.
 * The level-taking helper above remains for portable callers with an
 * explicit context. */
// FUNCTION: LEGORR 0x00402ae0
void terrain_cell_refresh_dispatch_current(const int cell_xy[2], int mode)
{
    GameContext* ctx = entity_pool_callback_context_get();
    LevelState* lvl = ctx != NULL ? ctx->level : NULL;
    (void)mode;
    if (cell_xy == NULL || lvl == NULL || lvl->ptr_terrain == NULL) {
        return;
    }
    terrain_cell_surface_type_reset(lvl->ptr_terrain, cell_xy[0],
                                    cell_xy[1]);
}

static CoordinateEntityDispatchHook s_coordinate_direct_callback;
static CoordinateEntityDispatchHook s_coordinate_mode_zero_callback;
static CoordinateEntityDispatchHook s_coordinate_mode_nonzero_callback;
static void* s_coordinate_dispatch_context;

void coordinate_entity_dispatch_hooks_install(
    CoordinateEntityDispatchHook direct_callback,
    CoordinateEntityDispatchHook mode_zero_callback,
    CoordinateEntityDispatchHook mode_nonzero_callback, void* context)
{
    s_coordinate_direct_callback = direct_callback;
    s_coordinate_mode_zero_callback = mode_zero_callback;
    s_coordinate_mode_nonzero_callback = mode_nonzero_callback;
    s_coordinate_dispatch_context = context;
}

struct CoordinateEntityDispatchPayload {
    const void* coordinate_pair;
    int extra;
    CoordinateEntityDispatchHook callback;
};

static int coordinate_entity_dispatch_adapter(EntitySlot* entity,
                                              void* payload)
{
    CoordinateEntityDispatchPayload* const data =
        (CoordinateEntityDispatchPayload*)payload;
    if (data == NULL || data->callback == NULL) {
        return 0;
    }
    return data->callback(entity, data->coordinate_pair, data->extra,
                          s_coordinate_dispatch_context);
}

void coordinate_callback_dispatch(const void* coord_pair, int value)
{
    CoordinateEntityDispatchPayload payload;
    payload.coordinate_pair = coord_pair;
    payload.extra = value;
    payload.callback = s_coordinate_direct_callback;
    entity_pool_callback_search(coordinate_entity_dispatch_adapter, &payload);
}

// FUNCTION: LEGORR 0x00402b30
void coordinate_callback_dispatch_current(const void* coord_pair)
{
    CoordinateEntityDispatchPayload payload;
    payload.coordinate_pair = coord_pair;
    payload.extra = 0;
    payload.callback = s_coordinate_direct_callback;
    entity_pool_callback_search(coordinate_entity_dispatch_adapter, &payload);
}

// FUNCTION: LEGORR 0x00402b60
void coordinate_callback_dispatch_by_mode(const void* coord_pair, int mode)
{
    CoordinateEntityDispatchPayload payload;
    payload.coordinate_pair = coord_pair;
    payload.extra = 0;
    payload.callback = mode != 0 ? s_coordinate_mode_nonzero_callback
                                 : s_coordinate_mode_zero_callback;
    entity_pool_callback_search(coordinate_entity_dispatch_adapter, &payload);
}

/* --- the global-object field band (see level.hpp) ---------------------------
 * The corpus's `DAT_005560d4 != 0` guard lands as the port's NULL-level
 * guard; every other detail is the corpus's own arithmetic. */
// FUNCTION: LEGORR 0x0041f870
void global_object_field8c_increment(LevelState* s)
{
    if (s != NULL) {
        level_set_int(s, 0x8C, level_int(s, 0x8C) + 1);
    }
}

// FUNCTION: LEGORR 0x0041f880
void global_object_field8c_subtract(LevelState* s, int amount)
{
    if (s != NULL) {
        level_set_int(s, 0x8C, level_int(s, 0x8C) - amount);
    }
}

// FUNCTION: LEGORR 0x0041f8a0
void global_object_field98_add(LevelState* s, int amount)
{
    if (s != NULL) {
        level_set_int(s, 0x98, level_int(s, 0x98) + amount);
    }
}

// FUNCTION: LEGORR 0x0041f8c0
void global_object_field98_clear(LevelState* s)
{
    if (s != NULL) {
        level_set_int(s, 0x98, 0);
    }
}

// FUNCTION: LEGORR 0x0041f920
void level_state_field9c_add(LevelState* s, int amount)
{
    if (s != NULL) {
        level_set_int(s, 0x9C, level_int(s, 0x9C) + amount);
    }
}

// FUNCTION: LEGORR 0x0041f980
void global_object_field94_increment(LevelState* s)
{
    if (s != NULL) {
        level_set_int(s, 0x94, level_int(s, 0x94) + 1);
    }
}

// FUNCTION: LEGORR 0x0041f990
void global_object_fielda8_increment(LevelState* s)
{
    if (s != NULL) {
        level_set_int(s, 0xA8, level_int(s, 0xA8) + 1);
    }
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
// FUNCTION: LEGORR 0x004589a0
int global_option_low_bits_enabled(const LevelState* s)
{
    return s != NULL && (s->objective & 7u) != 0u;
}

/* GlobalFlagGet_004ffe30 (FUN_00458650): threaded objective-result read. */
// FUNCTION: LEGORR 0x00458650
int level_objective_result_get(const LevelState* s)
{
    return s != NULL ? s->objective_result : 0;
}

/* TerrainObjectTypeSfxAtCellGet (FUN_0044ad80) — see level.hpp. */
#define TERRAIN_SFX_GRID_CELLS 300
// GLOBAL: LEGORR 0x004de814
static int s_terrain_sfx_grid[TERRAIN_SFX_GRID_CELLS]; /* DAT_004de814 role */

// FUNCTION: LEGORR 0x0044ad80
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
 * pair initialized 20/15, but the resolver zeroes the row half at its
 * entry (0x42e8b8) — only the TYPE half carries across entries, and only
 * into skipped (`continue`) iterations; a resolved name therefore always
 * sees a fresh row (the port's resolver carries the entry zeroing); the corpus
 * strides a missing value string into the alias table (the port's lookup
 * guards NULL) and stores blind past the 300-entry window (the port
 * stores blind as well). The entry chain is the subgroup record followed
 * by its equal-key siblings, entry name = the object name (the corpus
 * reads the name blind at its record +4; the port record keeps the name
 * token in its first word, so the load lands there), entry value = the
 * sound alias reference. */
// FUNCTION: LEGORR 0x0044acc0
void terrain_object_sfx_grid_config_load(BlockFile* config,
                                         const char* root_path)
{
    memset(s_terrain_sfx_grid, 0, sizeof s_terrain_sfx_grid);
    int obj_x = 20;
    int init_count = 20;
    int obj_y;
    /* The corpus writes the row output on each pass through this 20-count
     * pre-loop. Keep those stores visible to VC5; final outputs remain 20/15. */
    do {
        --init_count;
        *(volatile int*)&obj_y = 15;
    } while (init_count != 0);
    const char* path = string_path_join(root_path, "S_ObjTtSFXs", NULL);
    for (BlockRec* entry = block_subgroup(config, path);
         entry != NULL; entry = block_next_equal_key(entry)) {
        /* The corpus loads the name blind (mov ecx,[esi+4]); the port
         * record keeps the name token in the first word. The chain
         * already proved entry non-NULL. */
        const char* name = *(const char* const*)((const char*)entry);
        if (entity_type_row_for_name(name, &obj_x,
                                     &obj_y) == 0) {
            continue;
        }
        int sfx = 0;
        /* The entry chain proves non-NULL; BlockRec stores value after name. */
        const char* value = *(const char* const*)((const char*)entry
                                                   + sizeof(void*));
        if (sound_alias_lookup(value, &sfx) == 0) {
            continue;
        }
        const int cell = obj_y + obj_x * 15;
        s_terrain_sfx_grid[cell] = sfx; /* the corpus's blind scale store */
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

// FUNCTION: LEGORR 0x0040e700
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
// FUNCTION: LEGORR 0x00438720
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
// FUNCTION: LEGORR 0x00409cd0
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
// FUNCTION: LEGORR 0x00451180
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

// FUNCTION: LEGORR 0x0040d3f0
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

/* TerrainCellHasOccupiedOrFlaggedNeighbor (FUN_0040d240): preserve the
 * corpus's center rejection and its two ordered cardinal rings. The
 * per-cell DAT_004c7df8 side table is threaded as a query callback. */
// FUNCTION: LEGORR 0x0040D240
int terrain_cell_has_occupied_or_flagged_neighbor(
    const LevelState* s, int cx, int cy,
    TerrainCellSidecarPresenceQuery sidecar_present)
{
    const uint8_t* center = level_voxel(s, cx, cy);
    if (center == NULL || (center[8] & 8u) == 0u
        || center[2] == 6u || center[2] == 7u) {
        return 0;
    }
    if ((sidecar_present != NULL && sidecar_present(s, cx, cy) != 0)
        || terrain_cell_special_state_predicate(s, cx, cy, 0) != 0) {
        return 0;
    }

    static const int8_t offsets[8][2] = {
        { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 },
        { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 },
    };
    for (int i = 0; i < 8; ++i) {
        const int x = cx + offsets[i][0];
        const int y = cy + offsets[i][1];
        if (level_coord_pair_within_bounds(s, x, y) == 0) {
            continue;
        }
        if ((sidecar_present != NULL && sidecar_present(s, x, y) != 0)
            || terrain_cell_special_state_predicate(s, x, y, 0) != 0) {
            return 1;
        }
    }
    return 0;
}

/* --- the path record window band (2026-10-25) ----------------------------- */

/* PathRecordAllocateAndLink (FUN_00409350): allocates the corpus's 0xd4
 * word window (53 dwords), zero-fills it the corpus way, stamps the
 * serial counter store at +0x08, copies the source coordinate pair to
 * +0x0c/+0x10, and links the previous most-recent record (corpus dword
 * slot 9). Deviations: the window is widened past the corpus stride so
 * the link holds a port pointer (port-layout convention), the corpus's
 * belt-and-braces zero stores are spelled explicitly after the fill, and
 * the allocator's failure word is guarded. */

#define PATH_RECORD_WINDOW 0xd4 /* the corpus's 53-dword window           */
#define PATH_RECORD_WORDS (PATH_RECORD_WINDOW / 4u)

/* DAT_004b8a20 role */
// GLOBAL: LEGORR 0x004b8a20
static uint32_t s_path_record_count;
// GLOBAL: LEGORR 0x004b8a24
static void* s_path_record_last;     /* DAT_004b8a24 role */

typedef struct PathRecordPayloadPort {
    const void* record;
    void* payload;
    uint32_t count;
    struct PathRecordPayloadPort* next;
} PathRecordPayloadPort;

typedef struct CellPathRecordPort {
    LevelState* level;
    int x;
    int y;
    void* record;
    struct CellPathRecordPort* next;
} CellPathRecordPort;

typedef struct PathRecordEntityPort {
    const void* record;
    uint32_t count;
    struct EntitySlot* entities[40];
    struct PathRecordEntityPort* next;
} PathRecordEntityPort;

static PathRecordPayloadPort* s_path_record_payload_ports;
static CellPathRecordPort* s_cell_path_record_ports;
static PathRecordEntityPort* s_path_record_entity_ports;
static TerrainSurfaceGrid* s_path_record_surface_grid;

static PathRecordPayloadPort* path_record_payload_port_find(
    const void* record)
{
    PathRecordPayloadPort* port;
    for (port = s_path_record_payload_ports; port != NULL; port = port->next) {
        if (port->record == record) return port;
    }
    return NULL;
}

static CellPathRecordPort* cell_path_record_port_find(
    const LevelState* level, int x, int y)
{
    CellPathRecordPort* port;
    for (port = s_cell_path_record_ports; port != NULL; port = port->next) {
        if (port->level == level && port->x == x && port->y == y)
            return port;
    }
    return NULL;
}

static PathRecordEntityPort* path_record_entity_port_find(
    const void* record)
{
    PathRecordEntityPort* port;
    for (port = s_path_record_entity_ports; port != NULL; port = port->next) {
        if (port->record == record) return port;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00409350
void* path_record_allocate_and_link(const uint32_t* coord_pair)
{
    const size_t bytes =
        PATH_RECORD_WINDOW + sizeof(void*) - sizeof(uint32_t);
    char* rec = (char*)engine_heap_alloc((long)bytes);
    if (rec == NULL) {
        return NULL; /* deviation: the corpus strides the allocator's word */
    }
    __builtin_memset(rec, 0, bytes); /* the corpus's rep stos zero-fill  */
    /* The corpus writes these two belt words after the sweep and MSVC5
     * keeps the stores (the disassembly shows them landing blind). */
    *(uint32_t*)(rec + 0x20) = 0u;
    *(uint32_t*)(rec + 0x1c) = 0u;
    /* Keep the corpus's two counter reads separate: the record receives the
     * old word at 0x40936e, then 0x409379 reloads and increments the global. */
    *(uint32_t*)(rec + 0x08) = s_path_record_count;
    s_path_record_count = s_path_record_count + 1u;
    if (coord_pair != NULL) {
        *(uint32_t*)(rec + 0x0c) = coord_pair[0];
        *(uint32_t*)(rec + 0x10) = coord_pair[1];
    }
    *(void**)(rec + 0x24) = s_path_record_last;         /* prev link */
    s_path_record_last = rec;
    return rec;
}

int path_record_payload_set(void* record, void* payload, uint32_t count)
{
#if defined(_WIN32)
    if (record == NULL) return 0;
    *(void**)((char*)record + 0x14) = payload;
    *(uint32_t*)((char*)record + 0x18) = count;
    return 1;
#else
    PathRecordPayloadPort* port;
    uintptr_t payload_address;
    if (record == NULL) return 0;
    port = path_record_payload_port_find(record);
    if (payload != NULL && port == NULL) {
        port = (PathRecordPayloadPort*)engine_heap_alloc(
            (long)sizeof(PathRecordPayloadPort));
        if (port == NULL) return 0;
        port->record = record;
        port->payload = NULL;
        port->count = 0u;
        port->next = s_path_record_payload_ports;
        s_path_record_payload_ports = port;
    }
    if (port != NULL) {
        port->payload = payload;
        port->count = count;
    }
    payload_address = (uintptr_t)payload;
    *(uint32_t*)((char*)record + 0x14) =
        (uint32_t)(payload_address & 0xffffffffu);
    *(uint32_t*)((char*)record + 0x18) = count;
    return 1;
#endif
}

void* path_record_payload_get(const void* record)
{
    PathRecordPayloadPort* port = path_record_payload_port_find(record);
    if (port != NULL) return port->payload;
#if defined(_WIN32)
    if (record != NULL) return *(void* const*)((const char*)record + 0x14);
#endif
    return NULL;
}

uint32_t path_record_payload_count_get(const void* record)
{
    PathRecordPayloadPort* port = path_record_payload_port_find(record);
    if (port != NULL) return port->count;
    return record != NULL ? *(const uint32_t*)((const char*)record + 0x18)
                          : 0u;
}

void path_record_surface_grid_bind(TerrainSurfaceGrid* grid)
{
    s_path_record_surface_grid = grid;
}

TerrainSurfaceGrid* path_record_surface_grid_get(void)
{
    return s_path_record_surface_grid;
}

int level_cell_path_record_set(LevelState* level, int x, int y,
                               void* record)
{
#if defined(_WIN32)
    uint8_t* cell;
    if (level == NULL) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    *(void**)(cell + 0x10) = record;
    return 1;
#else
    uint8_t* cell;
    CellPathRecordPort* port;
    CellPathRecordPort* previous;
    uintptr_t record_address;
    if (level == NULL) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    port = cell_path_record_port_find(level, x, y);
    if (record == NULL) {
        if (port != NULL) {
            previous = NULL;
            for (CellPathRecordPort* current = s_cell_path_record_ports;
                 current != NULL; current = current->next) {
                if (current == port) {
                    if (previous != NULL) previous->next = current->next;
                    else s_cell_path_record_ports = current->next;
                    engine_heap_free(current);
                    break;
                }
                previous = current;
            }
        }
    }
    else {
        if (port == NULL) {
            port = (CellPathRecordPort*)engine_heap_alloc(
                (long)sizeof(CellPathRecordPort));
            if (port == NULL) return 0;
            port->level = level;
            port->x = x;
            port->y = y;
            port->next = s_cell_path_record_ports;
            s_cell_path_record_ports = port;
        }
        port->record = record;
    }
    record_address = (uintptr_t)record;
    *(uint32_t*)(cell + 0x10) =
        (uint32_t)(record_address & 0xffffffffu);
    return 1;
#endif
}

void* level_cell_path_record_get(const LevelState* level, int x, int y)
{
    CellPathRecordPort* port = cell_path_record_port_find(level, x, y);
    if (port != NULL) return port->record;
#if defined(_WIN32)
    {
        const uint8_t* cell = level_voxel(level, x, y);
        return cell != NULL ? *(void* const*)(cell + 0x10) : NULL;
    }
#else
    return NULL;
#endif
}

int path_record_entity_append(void* record, struct EntitySlot* entity)
{
#if defined(_WIN32)
    uint32_t count;
    if (record == NULL || entity == NULL) return 0;
    count = *(uint32_t*)((char*)record + 0xcc);
    if (count >= 40u) return 0;
    *(struct EntitySlot**)((char*)record + 0x2c + count * 4u) = entity;
    *(uint32_t*)((char*)record + 0xcc) = count + 1u;
    return 1;
#else
    PathRecordEntityPort* port;
    uintptr_t entity_address;
    if (record == NULL || entity == NULL) return 0;
    port = path_record_entity_port_find(record);
    if (port == NULL) {
        port = (PathRecordEntityPort*)engine_heap_alloc(
            (long)sizeof(PathRecordEntityPort));
        if (port == NULL) return 0;
        port->record = record;
        port->count = 0u;
        port->next = s_path_record_entity_ports;
        s_path_record_entity_ports = port;
    }
    if (port->count >= 40u) return 0;
    port->entities[port->count] = entity;
    entity_address = (uintptr_t)entity;
    *(uint32_t*)((char*)record + 0x2c + port->count * 4u) =
        (uint32_t)(entity_address & 0xffffffffu);
    ++port->count;
    *(uint32_t*)((char*)record + 0xcc) = port->count;
    return 1;
#endif
}

uint32_t path_record_entity_count_get(const void* record)
{
    PathRecordEntityPort* port = path_record_entity_port_find(record);
    if (port != NULL) return port->count;
    return record != NULL ? *(const uint32_t*)((const char*)record + 0xcc)
                          : 0u;
}

struct EntitySlot* path_record_entity_get(const void* record, uint32_t index)
{
    PathRecordEntityPort* port = path_record_entity_port_find(record);
    if (port != NULL)
        return index < port->count ? port->entities[index] : NULL;
#if defined(_WIN32)
    if (record != NULL && index < path_record_entity_count_get(record))
        return *(struct EntitySlot* const*)((const char*)record + 0x2c +
                                            index * 4u);
#endif
    return NULL;
}

void path_record_cell_links_release(LevelState* level, const void* record)
{
    CellPathRecordPort* previous = NULL;
    CellPathRecordPort* port = s_cell_path_record_ports;
    while (port != NULL) {
        CellPathRecordPort* next = port->next;
        if ((level == NULL || port->level == level) &&
            (record == NULL || port->record == record)) {
            if (port->level != NULL) {
                uint8_t* cell = level_voxel(port->level, port->x, port->y);
                if (cell != NULL) *(uint32_t*)(cell + 0x10) = 0u;
            }
            if (previous != NULL) previous->next = next;
            else s_cell_path_record_ports = next;
            engine_heap_free(port);
        }
        else {
            previous = port;
        }
        port = next;
    }
}

static void path_record_storage_release(void* record)
{
    PathRecordPayloadPort* previous = NULL;
    PathRecordPayloadPort* port;
    PathRecordEntityPort* entity_previous = NULL;
    PathRecordEntityPort* entity_port;
    if (record == NULL) return;
    path_record_cell_links_release(NULL, record);
    for (port = s_path_record_payload_ports; port != NULL; port = port->next) {
        if (port->record == record) {
            if (previous != NULL) previous->next = port->next;
            else s_path_record_payload_ports = port->next;
            engine_heap_free(port->payload);
            engine_heap_free(port);
            break;
        }
        previous = port;
    }
    for (entity_port = s_path_record_entity_ports; entity_port != NULL;
         entity_port = entity_port->next) {
        if (entity_port->record == record) {
            if (entity_previous != NULL)
                entity_previous->next = entity_port->next;
            else
                s_path_record_entity_ports = entity_port->next;
            engine_heap_free(entity_port);
            break;
        }
        entity_previous = entity_port;
    }
    if (port == NULL) engine_heap_free(path_record_payload_get(record));
}

static void path_record_remove_and_free(void* record);

void path_record_release(void* record)
{
    path_record_remove_and_free(record);
}

/* FUNCTION: LEGORR 0x00409300
 * The original first checks whether the coordinate pair already has a
 * path descriptor, then creates and links a 0xd4-byte path record and
 * sets bit 1 in its final dword. This portable entry point models the
 * creation branch; callers that need duplicate suppression perform that
 * check before calling it. */
void* tutorial_path_descriptor_create(const uint32_t* coord_pair)
{
    char* record;
    if (coord_pair == NULL) return NULL;
    record = (char*)path_record_allocate_and_link(coord_pair);
    if (record == NULL) return NULL;
    *(uint32_t*)(record + 0xd0) |= 2u;
    return record;
}

uint32_t path_record_count_peek(void) { return s_path_record_count; }
void* path_record_last_peek(void) { return s_path_record_last; }

/* PathRecordRelease (FUN_004099f0): unlink the selected record from the
 * path-record chain, then release its optional owned payload and record. */
// FUNCTION: LEGORR 0x004099f0
static void path_record_remove_and_free(void* record)
{
    void* previous = NULL;
    void* current = s_path_record_last;

    /* The corpus enters a do/while and tests the link before comparing the
     * requested record.  Keep that shape: an empty list goes straight to the
     * common payload/record release path. */
    do {
        if (current == NULL) goto release_record;
        if (current == record) {
            if (previous != NULL) {
                *(void**)((char*)previous + 0x24) =
                    *(void**)((char*)current + 0x24);
            }
            else {
                s_path_record_last = *(void**)((char*)current + 0x24);
            }
            goto release_record;
        }
        previous = current;
        current = *(void**)((char*)current + 0x24);
    } while (1);

release_record:
#if defined(_WIN32)
    engine_heap_free(*(void**)((char*)record + 0x14));
    engine_heap_free(record);
#else
    path_record_storage_release(record);
    engine_heap_free(record);
#endif
}

/* ReleaseAllGroupRecords (FUN_004099d0) saves each successor before the
 * unlinking/freeing leaf, so the full list can be released head-first. */
// FUNCTION: LEGORR 0x004099d0
void path_record_release_all(void)
{
    void* current = s_path_record_last;
    while (current != NULL) {
        void* next = *(void**)((char*)current + 0x24);
        path_record_remove_and_free(current);
        current = next;
    }
}

/* TutorialPathCoordinateIndex (FUN_00456b80) — see level.hpp. The corpus
 * sweeps x-outer/y-inner over the grid dims and head-pushes each cell's
 * coordinate byte onto the chain table, clearing the byte as consumed. */
// FUNCTION: LEGORR 0x00456b80
void tutorial_path_coordinate_index(LevelState* s)
{
    if (s == NULL) {
        return; /* deviation: the corpus strides the block blindly */
    }
    for (int i = 0; i != 0x37; ++i) {
        s->pool_heads[i] = NULL;
    }
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if (level_load_ptr(s, LEVEL_OFF_VOXELS) == NULL) {
        return; /* deviation: the corpus strides the voxel global */
    }
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            uint8_t* cell = voxel_at(s, x, y) + 0x04;
            const int coord = *cell;
            if (coord != 0) {
                const uint32_t ux = (uint32_t)x;
                const uint32_t uy = (uint32_t)y;
                /* the corpus reads the superseded head and threads the
                 * node's next link after the allocation */
                void* old = (coord < 0x37) ? s->pool_heads[coord] : NULL;
                void* node = engine_heap_alloc(16); /* widened node image */
                if (node == NULL) {
                    continue; /* deviation: the corpus strides the alloc */
                }
                __builtin_memcpy((char*)node + 0x0, &ux, 4);
                __builtin_memcpy((char*)node + 0x4, &uy, 4);
                __builtin_memcpy((char*)node + 0x8, &old, sizeof(old));
                if (coord < 0x37) {
                    s->pool_heads[coord] = node;
                }
                *cell = 0;
            }
        }
    }
}

/* Test hook: stages a count-table slot (the corpus's table writers are
 * unported, so the faithful read shape is exercised through this hook —
 * the install-hook precedent). */
void grid_count_table_slot_poke(int row, int column, int slot, int table,
                                uint32_t value)
{
    const int64_t idx = grid_count_index(row, column, slot);
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
// FUNCTION: LEGORR 0x00431140
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
    terrain_activity_build_record_update((LevelState*)lvl, x, y, 0);
    const int pair[2] = { x, y };
    coordinate_callback_dispatch(pair, 0);      /* 0x402b30 seam */
    message_dispatch(0x1c, 0, 0, pair);         /* 0x419b80 */
}

/* RandomVariantCellScatter (FUN_00435230) — see level.hpp. Draws ride the
 * module's corpus-RNG model (the shared 15-bit word thunk). */
// FUNCTION: LEGORR 0x00435230
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
            const int candidate[2] = { cx, cy };
            /* FUN_0040f190 is the live erosion candidate/mutation path. */
            const int spawn = lava_erosion_cell_eligible_and_mutate(
                candidate, 1);
            if (spawn != 0 && mode != 0) {
                return;
            }
            dedup[key] = 1;
        }
    }
}

/* RandomThunkDraw15 (thunk_FUN_0048e170 at FUN_004798a0): the reference thunk
 * jumps to VC5's _rand entry. Keep the same direct tail-call shape on MSVC. */
// FUNCTION: LEGORR 0x004798a0
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
__declspec(naked) uint32_t rng_thunk_draw15(void)
{
    __asm { jmp rand }
}
#else
uint32_t rng_thunk_draw15(void)
{
    return (uint32_t)level_random_draw15();
}
#endif

/* --- erosion timing band (2026-10-25) -------------------------------------
 * These module statics mirror the reference timing, stage, and lock tables;
 * EmergeMap and the live ticker now own their initialization and updates. */
#define EROSION_MARK_COUNT 2000   /* DAT_004cfbb4 */
#define EROSION_TICKET_COUNT LEVEL_TS_COUNT /* DAT_004d6914 */

// GLOBAL: LEGORR 0x004d78b8
static float s_erosion_decay_scale;
// GLOBAL: LEGORR 0x004d78bc
static float s_erosion_delay_scale;
// GLOBAL: LEGORR 0x004cbd30
static uint32_t s_erosion_clear_word;
// GLOBAL: LEGORR 0x004cfbb4
static uint32_t s_erosion_marks[EROSION_MARK_COUNT];
static LevelState* s_lava_erosion_level_context;
struct LavaErosionPendingCell {
    int x;
    int y;
};
struct LavaErosionStageSlot {
    int x;
    int y;
    float timer;
};
static LavaErosionPendingCell s_lava_erosion_pending_cells[EROSION_MARK_COUNT];
static unsigned int s_lava_erosion_pending_count;
static LavaErosionStageSlot s_lava_erosion_stage_slots[EROSION_MARK_COUNT];
static float s_erosion_trigger_accumulator;
static unsigned int s_lava_erosion_mutation_threshold = 4u;
static LavaErosionEdgeApplyHook s_lava_erosion_edge_apply;
static LavaErosionCellActionHook s_lava_erosion_work_clear;
static LavaErosionCellActionHook s_lava_erosion_cell_change;
static LavaErosionSoundHook s_lava_erosion_sound;
static LavaErosionEventHook s_lava_erosion_event;
static void* s_lava_erosion_hook_context;

static void lava_erosion_runtime_tables_clear(void)
{
    memset(s_erosion_marks, 0, sizeof(s_erosion_marks));
    memset(s_lava_erosion_pending_cells, 0,
           sizeof(s_lava_erosion_pending_cells));
    memset(s_lava_erosion_stage_slots, 0,
           sizeof(s_lava_erosion_stage_slots));
    memset(s_ts_active, 0, sizeof(s_ts_active));
    memset(s_ts_coordinates, 0, sizeof(s_ts_coordinates));
    memset(s_ts_stamps, 0, sizeof(s_ts_stamps));
    s_lava_erosion_pending_count = 0u;
    s_erosion_trigger_accumulator = 0.0f;
    s_ts_now = 0;
}

// FUNCTION: LEGORR 0x0040e930
void erosion_timing_state_init(float value_a, float value_b, float value_c)
{
    s_erosion_decay_scale = value_a * 25.0f;
    s_erosion_delay_scale = value_b * 25.0f * 0.2f;
    const float tick = value_c * 25.0f;
    s_erosion_clear_word = 0u;
    lava_erosion_runtime_tables_clear();
    __builtin_memcpy(&s_ts_now, &tick, sizeof s_ts_now);
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
               ? (int)s_ts_active[index] : -1;
}

/* Find the first zero in the 2,000-dword DAT_004cfbb4 stage-active table.
 * Stage registration and the pending-cell ticker share this allocator. */
// FUNCTION: LEGORR 0x0040e990
int lava_erosion_stage_slot_find_free(int* slot_out)
{
    volatile uint32_t* cursor = s_erosion_marks;
    int slot = 0;
    do {
        if (*cursor == 0u) {
            *slot_out = slot;
            return 1;
        }
        ++cursor;
        ++slot;
    } while (cursor < s_erosion_marks + EROSION_MARK_COUNT);
    return 0;
}

// FUNCTION: LEGORR 0x0040e9c0
float lava_erosion_delay_from_cell(const LevelState* s, int x, int y)
{
    const uint8_t* rec = level_voxel(s, x, y);
    if (rec == NULL) {
        return 0.0f; /* deviation: the corpus strides the grid blindly */
    }
    const int steps = 6 - (int)rec[6];
    return (float)steps * s_erosion_delay_scale * 25.0f;
}

// FUNCTION: LEGORR 0x0040f000
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
            const int candidate_xy[2] = { nx, ny };
            if (cell_coordinate_timestamp_exists(candidate_xy) == 0u) {
                return 1;
            }
        }
    }
    return 0;
}

/* LavaErosionStageRegister (FUN_0040ea10): claim the first inactive stage
 * slot, seed the cell byte and countdown, then append it to the ordered
 * pending-candidate table. */
int lava_erosion_stage_register(const int xy[2], uint32_t stage)
{
    int slot;
    uint8_t* cell;
    if (xy == NULL || s_lava_erosion_level_context == NULL) return 0;
    cell = level_voxel(s_lava_erosion_level_context, xy[0], xy[1]);
    if (cell == NULL) return 0;
    if (!lava_erosion_stage_slot_find_free(&slot)) return 0;
    cell[7] = (uint8_t)stage;
    s_lava_erosion_stage_slots[slot].x = xy[0];
    s_lava_erosion_stage_slots[slot].y = xy[1];
    s_lava_erosion_stage_slots[slot].timer = lava_erosion_delay_from_cell(
        s_lava_erosion_level_context, xy[0], xy[1]) / (float)(stage + 1u);
    s_erosion_marks[slot] = 1u;
    lava_erosion_pending_cell_add(xy, 1);
    return 1;
}

unsigned int lava_erosion_pending_cell_count_get(void)
{
    return s_lava_erosion_pending_count;
}

int lava_erosion_pending_cell_get(unsigned int index, int xy_out[2])
{
    if (xy_out == NULL || index >= s_lava_erosion_pending_count) return 0;
    xy_out[0] = s_lava_erosion_pending_cells[index].x;
    xy_out[1] = s_lava_erosion_pending_cells[index].y;
    return 1;
}

int lava_erosion_stage_get(unsigned int slot, int xy_out[2],
                           float* timer_out, uint8_t* cell_stage_out)
{
    const LavaErosionStageSlot* stage;
    const uint8_t* cell;
    if (slot >= EROSION_MARK_COUNT || s_erosion_marks[slot] == 0u) return 0;
    stage = &s_lava_erosion_stage_slots[slot];
    if (xy_out != NULL) {
        xy_out[0] = stage->x;
        xy_out[1] = stage->y;
    }
    if (timer_out != NULL) *timer_out = stage->timer;
    cell = level_voxel(s_lava_erosion_level_context, stage->x, stage->y);
    if (cell_stage_out != NULL) *cell_stage_out = cell != NULL ? cell[7] : 0u;
    return 1;
}

void lava_erosion_level_context_set(LevelState* s)
{
    if (s_lava_erosion_level_context != s) {
        lava_erosion_runtime_tables_clear();
        if (s == NULL) {
            s_erosion_decay_scale = 0.0f;
            s_erosion_delay_scale = 0.0f;
            s_erosion_clear_word = 0u;
            s_lava_erosion_mutation_threshold = 4u;
        }
    }
    s_lava_erosion_level_context = s;
}

void lava_erosion_level_context_release(LevelState* s)
{
    if (s_lava_erosion_level_context == s) {
        lava_erosion_level_context_set(NULL);
    }
}

/* LavaErosionNeighborEligible (FUN_0040f2b0): bit 3 is the base candidate
 * gate; reserved cells and terrain types 6, 9, and 7 are excluded. The
 * reference checks an optional surface owner before honoring the edge-wall
 * flag. The portable level record does not expose that owner yet, so a raised
 * edge-wall flag is conservatively treated as a rejection. */
// FUNCTION: LEGORR 0x0040f2b0
int lava_erosion_neighbor_eligible(const LevelState* s, int x, int y)
{
    const int type = level_tile_type(s, x, y);
    if (s == NULL || level_cell_flag_8_bounded(s, x, y) == 0u ||
        level_cell_flag_reserved(s, x, y) != 0u || type == 6 || type == 9 ||
        type == 7) {
        return 0;
    }
    return level_int(s, LEVEL_OFF_SAFECAVERNS) == 0 ||
           level_cell_flag_edge_wall(s, x, y) == 0u;
}

int lava_erosion_neighbor_eligible_current(const int xy[2])
{
    if (xy == NULL) return 0;
    return lava_erosion_neighbor_eligible(s_lava_erosion_level_context,
                                          xy[0], xy[1]);
}

/* LavaErosionPendingCellAdd (FUN_0040ee50): maintain the compact 2,000-cell
 * candidate list and mirror membership in voxel flag bit 31. Removal uses
 * swap-with-last and retires the matching stage slot, as in the reference. */
// FUNCTION: LEGORR 0x0040ee50
void lava_erosion_pending_cell_add(const int xy[2], int add)
{
    uint8_t* const cell = xy != NULL
        ? level_voxel(s_lava_erosion_level_context, xy[0], xy[1]) : NULL;
    unsigned int i;
    uint32_t flags;
    if (cell == NULL) return;
    __builtin_memcpy(&flags, cell + 8, sizeof flags);
    if (add == 0) {
        for (i = 0; i < s_lava_erosion_pending_count; ++i) {
            if (s_lava_erosion_pending_cells[i].x == xy[0] &&
                s_lava_erosion_pending_cells[i].y == xy[1]) {
                --s_lava_erosion_pending_count;
                s_lava_erosion_pending_cells[i] =
                    s_lava_erosion_pending_cells[s_lava_erosion_pending_count];
                flags &= 0x7fffffffu;
                __builtin_memcpy(cell + 8, &flags, sizeof flags);
                break;
            }
        }
        for (i = 0; i < EROSION_MARK_COUNT; ++i) {
            if (s_erosion_marks[i] != 0u &&
                s_lava_erosion_stage_slots[i].x == xy[0] &&
                s_lava_erosion_stage_slots[i].y == xy[1]) {
                s_erosion_marks[i] = 0u;
                break;
            }
        }
        cell_coordinate_timestamp_remember(xy);
    }
    else if ((flags & 0x80000000u) == 0u &&
             s_lava_erosion_pending_count < EROSION_MARK_COUNT) {
        LavaErosionPendingCell* const pending =
            &s_lava_erosion_pending_cells[s_lava_erosion_pending_count++];
        pending->x = xy[0];
        pending->y = xy[1];
        map_cell_tracking_flags_clear_and_refresh(
            s_lava_erosion_level_context, xy);
        __builtin_memcpy(&flags, cell + 8, sizeof flags);
        flags |= 0x80000000u;
        __builtin_memcpy(cell + 8, &flags, sizeof flags);
    }
}

/* LavaErosionPendingCellTick (FUN_0040eab0): keep the engine-scale trigger
 * gate, walk the pending coordinates in their live insertion/swap order,
 * then advance all active stage timers. */
// FUNCTION: LEGORR 0x0040eab0
int lava_erosion_pending_cell_tick(float scale)
{
    int triggered = 0;
    LevelState* s = s_lava_erosion_level_context;
    s_erosion_trigger_accumulator += scale;
    if (s_erosion_decay_scale < s_erosion_trigger_accumulator) {
        int free_slot;
        unsigned int n;
        triggered = 1;
        if (s != NULL && s_lava_erosion_pending_count != 0u &&
            lava_erosion_stage_slot_find_free(&free_slot)) {
            const unsigned int slot = (unsigned int)free_slot;
            const unsigned int start =
                (unsigned int)level_rand_u16() %
                s_lava_erosion_pending_count;
            for (n = 0; n < s_lava_erosion_pending_count; ++n) {
                const unsigned int index =
                    (start + n) % s_lava_erosion_pending_count;
                const int seed_xy[2] = {
                    s_lava_erosion_pending_cells[index].x,
                    s_lava_erosion_pending_cells[index].y
                };
                int candidate_xy[2];
                if (lava_erosion_neighbor_check(
                        s, seed_xy[0], seed_xy[1], candidate_xy) != 0) {
                    uint8_t* cell = level_voxel(
                        s, candidate_xy[0], candidate_xy[1]);
                    if (cell == NULL) continue;
                    if (level_cell_occupied_at(s, candidate_xy[0],
                                               candidate_xy[1]) == 0 ||
                        (cell[0x0c] & 0x10u) != 0u) {
                        float delay;
                        uint32_t secondary;
                        __builtin_memcpy(&secondary, cell + 0x0c,
                                         sizeof secondary);
                        secondary &= ~0x10u;
                        __builtin_memcpy(cell + 0x0c, &secondary,
                                         sizeof secondary);
                        s_lava_erosion_stage_slots[slot].x =
                            candidate_xy[0];
                        s_lava_erosion_stage_slots[slot].y =
                            candidate_xy[1];
                        delay = lava_erosion_delay_from_cell(
                            s, candidate_xy[0], candidate_xy[1]);
                        s_lava_erosion_stage_slots[slot].timer = delay;
                        s_erosion_marks[slot] = 1u;
                        cell[7] = 0u;
                        lava_erosion_pending_cell_add(candidate_xy, 1);
                        message_dispatch(0x1e, 0, 0, candidate_xy);
                        break;
                    }
                    cell_coordinate_timestamp_remember(candidate_xy);
                    cell[0x0c] |= 0x10u;
                }
            }
        }
        s_erosion_trigger_accumulator = 0.0f;
    }

    if (s != NULL) {
        unsigned int slot;
        for (slot = 0; slot < EROSION_MARK_COUNT; ++slot) {
            LavaErosionStageSlot* stage;
            uint8_t* cell;
            int before, after;
            if (s_erosion_marks[slot] == 0u) continue;
            stage = &s_lava_erosion_stage_slots[slot];
            before = crt_float_to_long(stage->timer);
            stage->timer -= scale;
            after = crt_float_to_long(stage->timer);
            if (after == before) continue;
            cell = level_voxel(s, stage->x, stage->y);
            if (cell == NULL) {
                s_erosion_marks[slot] = 0u;
                continue;
            }
            cell[7] = (uint8_t)(4 - after);
            if (after == 0) {
                s_erosion_marks[slot] = 0u;
                cell[2] = 6u;
            }
            if (after >= 0 && after <= 3) {
                float position[3];
                float rotation[3];
                float xy[2];
                if (terrain_cell_world_center_xy_get(
                        s->ptr_terrain, stage->x, stage->y, xy)) {
                    position[0] = xy[0];
                    position[1] = xy[1];
                    position[2] = (float)terrain_surface_height_get(
                        s->ptr_terrain, stage->x, stage->y);
                    d3drm_vector_random(rotation);
                    lava_effect_select_and_place(after + 4, position,
                                                 rotation);
                }
            }
            cell_init_pass(s, (float)stage->x, (uint32_t)stage->y);
        }
    }

    /* The lock entries share the configured ErodeLockTime float word. */
    {
        unsigned int i;
        for (i = 0; i < EROSION_TICKET_COUNT; ++i) {
            if (s_ts_active[i] != 0u) {
                float remaining;
                __builtin_memcpy(&remaining, &s_ts_stamps[i],
                                 sizeof remaining);
                remaining -= scale;
                __builtin_memcpy(&s_ts_stamps[i], &remaining,
                                 sizeof remaining);
                if (remaining < 0.0f) s_ts_active[i] = 0u;
            }
        }
    }
    return triggered;
}

void lava_erosion_hooks_set(LavaErosionEdgeApplyHook edge_apply,
                            LavaErosionCellActionHook work_clear,
                            LavaErosionCellActionHook cell_change,
                            LavaErosionSoundHook sound,
                            LavaErosionEventHook event,
                            void* context)
{
    s_lava_erosion_edge_apply = edge_apply;
    s_lava_erosion_work_clear = work_clear;
    s_lava_erosion_cell_change = cell_change;
    s_lava_erosion_sound = sound;
    s_lava_erosion_event = event;
    s_lava_erosion_hook_context = context;
}

void lava_erosion_mutation_threshold_set(unsigned int threshold)
{
    s_lava_erosion_mutation_threshold = threshold;
}

/* LavaErosionNeighborMutate (FUN_0040f330). The available scene-slot API
 * claims the effect slot; cell flags, repeat count, dirty notification, and
 * recursive propagation stay local to the level model. */
// FUNCTION: LEGORR 0x0040f330
void lava_erosion_neighbor_mutate(const int xy[2], unsigned int direction,
                                  int propagate)
{
    static const int edge_for_type[15] = {
        -1, 0, 1, 1, 2, 2, 2, 3, 3, 0, 1, 2, 3, 1, 0
    };
    static const int group_for_type[15] = {
        -1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0
    };
    uint8_t* cell;
    uint32_t flags;
    int type;
    int edge;
    int changed = 0;
    if (xy == NULL || s_lava_erosion_level_context == NULL) return;
    cell = level_voxel(s_lava_erosion_level_context, xy[0], xy[1]);
    if (cell == NULL) return;
    /* FUN_0040f330 drains the cell transition state through 0x432c90 before
     * clearing its work flag and attempting the directional mutation. */
    while (terrain_cell_state_update(s_lava_erosion_level_context, xy) == 0) {
    }
    /* FUN_0040f330 switches on the complete direction mask.  The values
     * 1..14 map to one of four scene edges and one of two edge endpoints. */
    type = direction < 15u ? (int)direction : -1;
    edge = type < 15 ? edge_for_type[type] : -1;
    if (edge < 0) return;
    if (s_lava_erosion_work_clear != NULL) {
        s_lava_erosion_work_clear(xy[0], xy[1], 0x2f,
                                  s_lava_erosion_hook_context);
    } else {
        terrain_cell_work_flag_clear(s_lava_erosion_level_context, xy,
                                     0x2fu);
    }
    if (s_lava_erosion_edge_apply != NULL) {
        changed = s_lava_erosion_edge_apply(xy[0], xy[1], direction, edge,
                                             s_lava_erosion_hook_context);
    } else {
        float position[2];
        if (terrain_cell_world_center_xy_get(
                s_lava_erosion_level_context->ptr_terrain,
                xy[0], xy[1], position)) {
            const float height = (float)terrain_surface_height_get(
                s_lava_erosion_level_context->ptr_terrain, xy[0], xy[1]);
            changed = scene_slot_pose_scale_register(
                group_for_type[type], (uint32_t)xy[0], (uint32_t)xy[1],
                position[0], position[1], height, 0.0f, 0.0f);
        }
    }
    if (changed != 0) {
        __builtin_memcpy(&flags, cell + 8, sizeof flags);
        flags |= 0x800u | 0x1000u;
        __builtin_memcpy(cell + 8, &flags, sizeof flags);
        level_tile_mark(s_lava_erosion_level_context, xy[0], xy[1], 1);
        if (s_lava_erosion_cell_change != NULL) {
            s_lava_erosion_cell_change(xy[0], xy[1], 1,
                                       s_lava_erosion_hook_context);
        } else {
            terrain_cell_changed_flag_set(
                s_lava_erosion_level_context->ptr_terrain, xy[0], xy[1]);
        }
        if (s_lava_erosion_sound != NULL) {
            s_lava_erosion_sound(xy[0], xy[1], s_lava_erosion_hook_context);
        } else {
            float position[3];
            float cell_xy[2];
            if (terrain_cell_world_center_xy_get(
                    s_lava_erosion_level_context->ptr_terrain,
                    xy[0], xy[1], cell_xy)) {
                position[0] = cell_xy[0];
                position[1] = cell_xy[1];
                position[2] = (float)terrain_surface_height_get(
                    s_lava_erosion_level_context->ptr_terrain,
                    xy[0], xy[1]);
                sound_submission_route_wrapper(NULL, 0x28, 0, 0, position);
            }
        }
    }
    if (propagate != 0) {
        uint32_t count;
        __builtin_memcpy(&count, cell + 0x24, sizeof count);
        ++count;
        __builtin_memcpy(cell + 0x24, &count, sizeof count);
        if (count >= s_lava_erosion_mutation_threshold) {
            lava_erosion_eight_neighbor_propagate(xy, direction);
            if (s_lava_erosion_event != NULL) {
                s_lava_erosion_event(xy[0], xy[1],
                                     s_lava_erosion_hook_context);
            } else {
                message_dispatch(6, 0, 0, xy);
            }
        }
    }
    if (s_lava_erosion_cell_change != NULL) {
        s_lava_erosion_cell_change(xy[0], xy[1], 0,
                                   s_lava_erosion_hook_context);
    } else {
        terrain_cell_surface_type_reset(
            s_lava_erosion_level_context->ptr_terrain, xy[0], xy[1]);
        terrain_cell_changed_flag_set(
            s_lava_erosion_level_context->ptr_terrain, xy[0], xy[1]);
    }
    cell_init_pass(s_lava_erosion_level_context, (float)xy[0],
                   (uint32_t)xy[1]);
}

/* FUN_0040f190: gate an erosion candidate, form a four-direction mask from
 * its walkable neighbors, and mutate only when no neighbor carries the
 * protected state/flag and at least one has a clear 0xc0 flag mask. */
// FUNCTION: LEGORR 0x0040f190
int lava_erosion_cell_eligible_and_mutate(const int xy[2], int hint)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    unsigned int mask = 0;
    int valid = 1;
    int clear_mask_neighbors = 0;
    int i;
    if (xy == NULL || lava_erosion_neighbor_eligible_current(xy) == 0) {
        return 0;
    }
    for (i = 0; i < 4; ++i) {
        const int nx = xy[0] + dx[i];
        const int ny = xy[1] + dy[i];
        if (level_cell_walkable_flag(s_lava_erosion_level_context, nx, ny)) {
            if (level_cell_state_is_1(s_lava_erosion_level_context, nx, ny) ||
                level_cell_flag_20_bounded(s_lava_erosion_level_context,
                                           nx, ny) != 0u) {
                valid = 0;
            }
            mask |= 1u << i;
            if (level_cell_mask_c0(s_lava_erosion_level_context, nx, ny) == 0u) {
                clear_mask_neighbors++;
            }
        }
    }
    if (valid && mask != 0u && clear_mask_neighbors != 0) {
        lava_erosion_neighbor_mutate(xy, mask, hint);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040f5f0
void lava_erosion_eight_neighbor_propagate(const int xy[2],
                                           unsigned int direction)
{
    static const int dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    static const int dy[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    int i;
    if (xy == NULL || s_lava_erosion_level_context == NULL) return;
    for (i = 0; i < 8; ++i) {
        int neighbor[2];
        neighbor[0] = xy[0] + dx[i];
        neighbor[1] = xy[1] + dy[i];
        if (level_voxel(s_lava_erosion_level_context,
                        neighbor[0], neighbor[1]) != NULL) {
            if (lava_erosion_cell_eligible_and_mutate(neighbor, 0) == 0 &&
                lava_erosion_neighbor_eligible_current(neighbor) != 0) {
                lava_erosion_neighbor_mutate(neighbor, direction, 0);
            }
        }
    }
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
// GLOBAL: LEGORR 0x004de588
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

// FUNCTION: LEGORR 0x00431da0
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

// FUNCTION: LEGORR 0x00431c70
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

/* The six-dword render snapshot is a separate zero-filled global block in
 * the corpus (DAT_004ea7ac). Its individual values are unresolved; keep the
 * storage opaque and expose only the exact copy/clear operations. */
// GLOBAL: LEGORR 0x004ea7ac
static uint32_t s_render_config_snapshot[6];

// FUNCTION: LEGORR 0x004370b0
void render_config_six_field_copy_out(uint32_t out[6])
{
    if (out == NULL) {
        return; /* deviation: the corpus copies through the pointer blindly */
    }
    const uint32_t* source = s_render_config_snapshot;
    int remaining = 6;
    do {
        *out++ = *source++;
        --remaining;
    } while (remaining != 0);
}

// FUNCTION: LEGORR 0x004370d0
void render_config_six_field_copy_in_or_clear(const uint32_t in[6])
{
    if (in != NULL) {
        uint32_t* destination = s_render_config_snapshot;
        int remaining = 6;
        do {
            *destination++ = *in++;
            --remaining;
        } while (remaining != 0);
        return;
    }
    uint32_t* destination = s_render_config_snapshot;
    int remaining = 6;
    do {
        *destination++ = 0;
        --remaining;
    } while (remaining != 0);
}

// FUNCTION: LEGORR 0x00437100
int grid_record_count_difference_sum(int type, int skip_flagged_rows)
{
    int total = 0;
    unsigned row;
    unsigned slot;

    row = 0;
    do {
        slot = 0;
        do {
            if (skip_flagged_rows == 0
                || (stats_table_word_at_144(type, (int)row) & 8u) == 0u) {
                total += grid_record_count_difference_get(type, (int)row,
                                                           (int)slot, 1);
            }
            ++slot;
        } while (slot < 16);
        ++row;
    } while (row < 15u);
    return total;
}
