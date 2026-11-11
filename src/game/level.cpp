// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>

#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/record.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/simtick.hpp"
#include "gods98/string.hpp"
#include "gods98/surface.hpp"
#include "crt/crt.hpp"
#include "gods98/view.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "entity.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"
#include "gods98/flic.hpp"
#include "terrain_grid.hpp"
#include "select.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"

extern "C" float random_float_range(float minimum, float maximum);
/* Cell mutation's cheat branch calls the existing entity-side implementation
 * of SmallSpiderCellSwarmSpawn. Its runtime prerequisites are checked by that
 * owner (active entity context, surface grid, and configured entity type). */

/* ------------------------------------------------------------------ *
 *  Corpus .rdata constants (identities from finding 07/22 and the
 *  decompile corpus; numeric values are best-effort re-derivations — the
 *  shipped data section is under copyright and not reproduced here).
 * ------------------------------------------------------------------ */

#define LVL_SEC_TO_TICK   25.0f       /* _DAT_0049e2b4: 25 game ticks/second */
#define LVL_OXYGEN_SCALE  0.001f      /* LevelLoad converts OxygenRate to a per-tick rate. */
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
static TerrainCellOverlayHeightFillHook
    s_terrain_cell_overlay_height_fill;
static void* s_terrain_cell_overlay_height_fill_context;
static LevelTerrainMaterialTransitionMeshHook
    s_material_transition_mesh_hook;
static void* s_material_transition_mesh_context;
static TerrainSurfaceGrid* level_surface_grid_get(const LevelState* level);
LevelState* level_current_owner_get(void);
static LevelState* s_lava_erosion_level_context;
static EngineState s_level_field_grid_state;
static LevelState* s_level_field_grid_owner;
static int s_debug_scene_transform_enabled;
static int s_debug_camera_pose_mode;
static LevelDebugRenderOps s_level_debug_render_ops;
static void terrain_cell_refresh_dispatch_level(LevelState* level,
                                                const int cell_xy[2],
                                                int mode);
static void map_cell_tracking_activity_state_update(GameContext* game,
                                                     LevelState* level);

typedef struct CellTransientEffectPort {
    LevelState* level;
    void* record;
    void* emitter;
    struct CellTransientEffectPort* next;
} CellTransientEffectPort;
static CellTransientEffectPort* s_cell_transient_effect_ports;

static CellTransientEffectPort* cell_transient_effect_port_find(
    const LevelState* level, const void* record)
{
    CellTransientEffectPort* port;
    for (port = s_cell_transient_effect_ports; port != NULL;
         port = port->next) {
        if (port->level == level && port->record == record) return port;
    }
    return NULL;
}

static CellTransientEffectPort* cell_transient_effect_port_ensure(
    LevelState* level, void* record)
{
    CellTransientEffectPort* port =
        cell_transient_effect_port_find(level, record);
    if (port != NULL) return port;
    port = (CellTransientEffectPort*)malloc(sizeof(*port));
    if (port == NULL) return NULL;
    port->level = level;
    port->record = record;
    port->emitter = NULL;
    port->next = s_cell_transient_effect_ports;
    s_cell_transient_effect_ports = port;
    return port;
}

static void* cell_transient_effect_get(LevelState* level, void* record)
{
    CellTransientEffectPort* port =
        cell_transient_effect_port_find(level, record);
    if (port != NULL) return port->emitter;
    return (void*)(uintptr_t)*(uint32_t*)((uint8_t*)record + 0x1c);
}

static void cell_transient_effect_set(CellTransientEffectPort* port,
                                      void* emitter)
{
    if (port == NULL) return;
    port->emitter = emitter;
    /* Keep the retail pointer word for x86 builds and layout observability;
     * the sidecar holds the native pointer on wider hosts. */
    *(uint32_t*)((uint8_t*)port->record + 0x1c) =
        (uint32_t)(uintptr_t)emitter;
}

static void cell_transient_effect_ports_release(LevelState* level)
{
    CellTransientEffectPort** link = &s_cell_transient_effect_ports;
    while (*link != NULL) {
        CellTransientEffectPort* port = *link;
        if (port->level == level) {
            *link = port->next;
            free(port);
        }
        else {
            link = &port->next;
        }
    }
}

namespace unported_ledger_stubs {
int ledger_stub_00466250_ExpandCellEntitySpawn(int x, int y);
}

/* MapLoadPipeline calls GridCellRecordBufferReplace(level) after rebuilding
 * the separate 20-byte map-cell pool. EngineState is a portable view, so
 * keep its dimensions in persistent storage instead of casting LevelState
 * to the non-layout-compatible EngineState struct. */
static void level_field_grid_buffer_replace(LevelState* level)
{
    if (level == NULL) return;
    memset(&s_level_field_grid_state, 0, sizeof(s_level_field_grid_state));
    s_level_field_grid_state.grid_w = (uint32_t)level_int(
        level, LEVEL_OFF_WIDTH);
    s_level_field_grid_state.grid_h = (uint32_t)level_int(
        level, LEVEL_OFF_HEIGHT);
    s_level_field_grid_owner = level;
    grid_cell_record_buffer_replace(&s_level_field_grid_state);
}

static void level_field_grid_buffer_release(LevelState* level)
{
    if (level == NULL || s_level_field_grid_owner != level) return;
    entity_field_grid_buffer_release();
    s_level_field_grid_owner = NULL;
    s_level_field_grid_state.grid_w = 0u;
    s_level_field_grid_state.grid_h = 0u;
}

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

void terrain_cell_overlay_height_fill_hook_set(
    TerrainCellOverlayHeightFillHook hook, void* context)
{
    s_terrain_cell_overlay_height_fill = hook;
    s_terrain_cell_overlay_height_fill_context = context;
}

// FUNCTION: LEGORR 0x00463590
int terrain_cell_surface_normal_get(uint32_t x, uint32_t y, float out[3])
{
    float vertices[4][3];
    float first[3];
    float second[3];
    float nx, ny, nz, inverse_length;
    size_t index;
    if (out == NULL) return 0;
    /* The retail helper also gates on bit 0 of its renderer-owned 8-byte quad
     * record. This standalone API has no record pointer; the dirty-normal
     * pass enforces that gate at its call site. Do not substitute the
     * surface orientation bit here. */
    if (s_terrain_quad_surface != NULL &&
        s_terrain_quad_vertex_read != NULL &&
        x < s_terrain_quad_width && y < s_terrain_quad_height) {
        index = (size_t)y * (size_t)s_terrain_quad_width + (size_t)x;
        if (!s_terrain_quad_vertex_read(s_terrain_quad_vertex_context,
                s_terrain_quad_surface, index, vertices)) return 0;
    }
    else {
        GameContext* game = entity_pool_callback_context_get();
        LevelState* level = game != NULL ? game->level : NULL;
        if (level == NULL) level = level_current_owner_get();
        if (level == NULL || level->ptr_terrain == NULL ||
            !scene_render_terrain_cell_vertices_get(level, x, y,
                                                     vertices)) return 0;
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

/* TerrainDirtyCellQuadNormalEmitPass has the same renderer quad table that
 * FUN_00463590 reads from DAT_00552198. Preserve its per-cell bit-0 veto at
 * the call boundary, where the port has the corresponding record pointer. */
static int terrain_cell_surface_normal_get_for_pass(
    uint32_t x, uint32_t y, const uint8_t* records,
    uint32_t width, uint32_t height, float out[3])
{
    size_t index;
    if (records == NULL || x >= width || y >= height) return 0;
    index = (size_t)y * (size_t)width + (size_t)x;
    if ((records[index * 8u + 4u] & 1u) != 0u) return 0;
    return terrain_cell_surface_normal_get(x, y, out);
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
                        terrain_cell_surface_normal_get_for_pass(
                            (uint32_t)nx, (uint32_t)ny, pass->records,
                            pass->width, pass->height, normal)) {
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
    {
        static const uint8_t clear_order[4] = { 3u, 2u, 1u, 0u };
        static const uint8_t set_order[4] = { 0u, 3u, 2u, 1u };
        const uint8_t* order = flipped ? set_order : clear_order;
        for (i = 0u; i < 4u; ++i)
            memcpy(ordered[i], vertices[order[i]], 3u * sizeof(float));
        flags = (uint8_t)((flags & ~4u) | (flipped ? 4u : 0u));
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
static const char* K_EMERGE_TIMEOUT = "EmergeTimeOut";
static const char* K_EMERGE_CREATURE = "EmergeCreature";
static const char* K_SELBOXH        = "selboxheight";
static const char* K_OLISTFILE      = "olistfile";
static const char* K_SAFECAVERNS    = "SafeCaverns";
static const char* K_STARTFP        = "StartFP";
static const char* K_NODRAIN        = "NoDrain";
static const char* K_OXYGENRATE     = "OxygenRate";
static const char* K_UPGRADETIME    = "UpgradeTime";
static const char* K_MINIFIG_UPGRADE_TIME = "MinifigureUpgradeTime";
static const char* K_VEHICLE_UPGRADE_TIME = "VehicleUpgradeTime";
static const char* K_BUILDING_UPGRADE_TIME = "BuildingUpgradeTime";
static const char* K_NO_MULTI_SELECT = "NoMultiSelect";
static const char* K_NO_AUTO_EAT     = "NoAutoEat";
static const char* K_RECALL_OL_OBJECTS = "RecallOLObjects";
static const char* K_GENERATE_SPIDERS = "GenerateSpiders";
static const char* K_DISABLE_TOOLTIP_SOUND = "DisableToolTipSound";
static const char* K_DISABLE_START_TELEPORT = "DisableStartTeleport";
static const char* K_TRAIN_TIME     = "TrainTime";
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
#define LVL_OFF_UPGRADE_TIMES 0x210  /* +0x84, twenty float slots */
#define LVL_OFF_VEHICLE_UPGRADE_TIME 0x214 /* +0x85 */
#define LVL_OFF_MINIFIG_UPGRADE_TIME 0x218 /* +0x86 */
#define LVL_OFF_BUILDING_UPGRADE_TIME 0x220 /* +0x88 */
#define LVL_OFF_TRAIN_TIME 0x260     /* +0x98 */

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

/* MapOpen2 (FUN_00480170): preserve the retail open, seek/tell, progress,
 * allocate, rewind/read, and close order against the level-owned map slots. */
// GLOBAL: LEGORR 0x004a2770
static const char s_map_open_binary_mode[] = "rb";
// FUNCTION: LEGORR 0x00480170
int map_open2(const char* path, int32_t* size_out)
{
    GameFileHandle* handle = game_file_open_dispatch(
        path, s_map_open_binary_mode);
    if (handle != NULL) {
        int size;
        int slot;
        void* map_data;
        (void)wad_file_seek(handle, 0, 2);
        size = wad_file_cursor_get(handle);
        if (s_loader_progress_callback != NULL) {
            typedef void (*ProgressProc)(const char*, uint32_t, void*);
            ((ProgressProc)s_loader_progress_callback)(
                path, (uint32_t)size, s_loader_progress_context);
        }
        slot = level_map_slot_allocate((size_t)size);
        if (slot != -1) {
            map_data = level_map_slot_get((uint32_t)slot);
            (void)wad_file_seek(handle, 0, 0);
            (void)wad_file_read(map_data, 1, size, handle);
            if (size_out != NULL) *size_out = size;
            (void)game_file_close(handle);
            return slot;
        }
        (void)game_file_close(handle);
    }
    return -1;
}

MapFile* level_map_open(const char* name)
{
    int slot;
    int32_t size = 0;
    uint8_t* data;
    MapFile* m;
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }
    slot = map_open2(name, &size);
    if (slot < 0) {
        return NULL;
    }
    data = level_map_slot_get((uint32_t)slot);
    if (data == NULL || size < 0x10) {
        level_map_slot_release((uint32_t)slot);
        return NULL;
    }
    m = (MapFile*)calloc(1, sizeof(MapFile));
    if (m == NULL) {
        level_map_slot_release((uint32_t)slot);
        return NULL;
    }
    m->data = data;
    m->size = size;
    m->slot = (uint32_t)slot;
    __builtin_memcpy(&m->width, data + 0x08, sizeof(uint32_t));
    __builtin_memcpy(&m->height, data + 0x0c, sizeof(uint32_t));
    return m;
}

void level_map_close(MapFile* m)
{
    if (m != NULL) {
        level_map_slot_release(m->slot);
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
 *  Builds the terrain struct (grid dimensions + world scale), the parallel
 *  physical block table (per-cell stride 0x28), and a port-owned indexed
 *  mesh record window whose vertex reads use the same 0x82470 copier as the
 *  retail DDi mesh path. Shader and COM surface ownership remain in the
 *  renderer boundary.
 * ------------------------------------------------------------------ */

#define TERRAIN_BLOCK_STRIDE 0x28
#define LEVEL_DIRTY_QUEUE_CAP 2500

typedef struct TerrainIndexedMeshObject {
    void* mesh_record;
} TerrainIndexedMeshObject;

typedef struct TerrainState {
    uint32_t dim[4];      /* [0]=w-1 [1]=h-1 [2]=w [3]=h */
    float    blocksize;
    float    terrain_value; /* TerrainCreate's fourth source parameter */
    float    world_off_x; /* -(w-1)*blocksize*0.5 */
    float    world_off_y; /*  (h-1)*blocksize*0.5 */
    uint32_t dirty_flags; /* TerrainCreate +0x73d0 sidecar word */
    uint32_t detail_mesh_token; /* retail +0x24 mode-2 indexed mesh object */
    uint8_t* blocks;      /* w*h*0x28 physical block table */
    uint8_t* variant_window; /* cell variant relation tables + header */
    float*   cell_vertices; /* four XYZ vertices per terrain quad */
    uint32_t* cell_detail_flags; /* indexed mesh +0x6c bit-0 sidecar */
    struct TerrainIndexedMeshRecord {
        uint32_t block_count;
        uint32_t reserved;
        void* blocks;
    } indexed_mesh;
    TerrainIndexedMeshObject detail_mesh_object;
    uint8_t* indexed_mesh_blocks; /* 0x70-byte indexed mesh entries */
    float* indexed_mesh_vertices; /* four 0x20-byte vertices per quad */
    /* GraphicsOverlayResources' separate quad list (0x552180/0x552198).
     * It owns derived output; cell_vertices remains the mutable base mesh. */
    TerrainCellQuadEmitGrid derived_mesh;
    uint32_t derived_mesh_dirty;
    float* derived_vertices; /* separate ordered retail X,Z,height quads */
    void* roof_scene;        /* genuine pooled native frame owner */
    void* roof_root;
    TerrainSurfaceGrid* detail_surface; /* physical-quad clear/restore owner */
    float    minimum_y;
    int      width, height;
    LevelState* owner_level; /* live current-level SceneRender quad owner */
    uint16_t dirty_queue_x[LEVEL_DIRTY_QUEUE_CAP];
    uint16_t dirty_queue_y[LEVEL_DIRTY_QUEUE_CAP];
    uint32_t dirty_queue_count;
    void* scene_context; /* portable sidecar for the input scene root */
} TerrainState;

int terrain_roof_owner_initialize(LevelState* level)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    TerrainState* terrain = level != NULL ? (TerrainState*)level->ptr_terrain : NULL;
    void* root = scene_root_object_get();
    if (terrain == NULL || terrain->owner_level != level || root == NULL ||
        terrain->roof_scene != NULL ||
        indexed_surface_scene_owner_get() != NULL) return 0;
    if (!indexed_surface_resource_allocate(root, terrain->width,
                                           terrain->height)) return 0;
    void* scene = indexed_surface_scene_owner_get();
    if (scene == NULL) { graphics_overlay_resources_release(); return 0; }
    free(terrain->derived_mesh.records);
    terrain->derived_mesh.records = indexed_surface_geometry_records_get();
    terrain->roof_scene = scene;
    terrain->roof_root = root;
    graphics_overlay_visibility_set(1u);
    return 1;
#else
    (void)level;
    return 0;
#endif
}

int terrain_roof_visibility_get(const LevelState* level)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    typedef long (__stdcall *ParentGet)(void*, void**);
    typedef unsigned long (__stdcall *Release)(void*);
    const TerrainState* t = level != NULL ? (const TerrainState*)level->ptr_terrain : NULL;
    void* channels[3];
    void* parent = NULL;
    if (t == NULL || t->roof_scene == NULL ||
        t->roof_scene != indexed_surface_scene_owner_get() ||
        t->roof_root != scene_root_object_get()) return -1;
    memcpy(channels, t->roof_scene, sizeof(channels));
    if (((ParentGet)(*(void***)channels[1])[0x5c / 4])(
            channels[1], &parent) != 0 || parent == NULL) return -1;
    int result = parent == channels[0] ? 1 :
                 (parent == channels[2] ? 0 : -1);
    ((Release)(*(void***)parent)[2])(parent);
    return result;
#else
    (void)level;
    return -1;
#endif
}

int terrain_cell_roof_vertices_get(const LevelState* level, uint32_t x,
                                    uint32_t y, float out[12])
{
    static const uint8_t clear_order[4] = { 3u, 2u, 1u, 0u };
    static const uint8_t set_order[4] = { 0u, 3u, 2u, 1u };
    const TerrainState* t = level != NULL ? (const TerrainState*)level->ptr_terrain : NULL;
    const uint8_t* order;
    size_t index;
    uint32_t corner;
    if (t == NULL || out == NULL || t->derived_vertices == NULL ||
        t->derived_mesh.records == NULL || x >= t->dim[0] || y >= t->dim[1]) return 0;
    index = (size_t)y * t->derived_mesh.width + x;
    if ((t->derived_mesh.records[index * 8u + 4u] & 1u) != 0u) return 0;
    order = (t->derived_mesh.records[index * 8u + 4u] & 4u) != 0u
        ? set_order : clear_order;
    for (corner = 0u; corner < 4u; ++corner) {
        const float* src = t->derived_vertices + index * 12u + corner * 3u;
        out[order[corner] * 3u] = src[0];
        out[order[corner] * 3u + 1u] = src[2];
        out[order[corner] * 3u + 2u] = src[1];
    }
    return 1 + ((t->derived_mesh.records[index * 8u + 4u] & 4u) != 0u);
}

typedef struct TerrainVertexReadRequest {
    int block_index;
    int first_vertex;
    int count;
    void* output;
} TerrainVertexReadRequest;

typedef struct TerrainIndexedMeshLayerOwner {
    void* reserved[3];
    void* mesh;
} TerrainIndexedMeshLayerOwner;

typedef struct TerrainIndexedMeshLayerObject {
    void* reserved[3];
    TerrainIndexedMeshLayerOwner* owner;
} TerrainIndexedMeshLayerObject;

static void terrain_indexed_mesh_layer_object_init(
    TerrainState* terrain, TerrainIndexedMeshLayerObject* object,
    TerrainIndexedMeshLayerOwner* owner)
{
    memset(object, 0, sizeof(*object));
    memset(owner, 0, sizeof(*owner));
    owner->mesh = &terrain->indexed_mesh;
    object->owner = owner;
}

static void terrain_indexed_mesh_cell_sync(TerrainState* terrain,
                                           size_t quad_index)
{
    static const uint8_t flipped_order[4] = { 3u, 0u, 1u, 2u };
    size_t quad_count;
    const uint8_t* order = NULL;
    uint8_t* cell;
    float* destination;
    uint32_t corner;
    if (terrain == NULL || terrain->cell_vertices == NULL ||
        terrain->indexed_mesh_vertices == NULL || terrain->blocks == NULL ||
        terrain->dim[0] == 0u || terrain->dim[1] == 0u) return;
    quad_count = (size_t)terrain->dim[0] * (size_t)terrain->dim[1];
    if (quad_index >= quad_count) return;
    cell = terrain->blocks +
        ((quad_index / (size_t)terrain->dim[0]) * terrain->dim[2] +
         quad_index % (size_t)terrain->dim[0]) * TERRAIN_BLOCK_STRIDE;
    if ((cell[0x19] & 1u) != 0u) order = flipped_order;
    destination = terrain->indexed_mesh_vertices + quad_index * 32u;
    for (corner = 0u; corner < 4u; ++corner) {
        const uint32_t source_corner = order != NULL
            ? order[corner] : corner;
        const float* source = terrain->cell_vertices +
            quad_index * 12u + source_corner * 3u;
        float* vertex = destination + corner * 8u;
        /* The retail indexed reader copies position, normal, and UV words
         * from 0x20-byte vertices. TerrainCreate owns positions here; the
         * other five words have no producer in the portable terrain path. */
        vertex[0] = source[0];
        vertex[1] = source[2];
        vertex[2] = source[1];
    }
}

static int terrain_derived_quad_create(void* context, uint32_t index,
                                       const uint32_t triangles[6])
{
    TerrainState* terrain = (TerrainState*)context;
    (void)triangles;
    if (terrain == NULL || terrain->owner_level == NULL) return -1;
    if (terrain->derived_vertices == NULL || index >=
        terrain->derived_mesh.width * terrain->derived_mesh.height ||
        index > 0x7fffffffu) return -1;
    return (int)index; /* software mesh slot, not a native DDi handle */
}

static int terrain_derived_quad_vertices_emit(void* context, int quad,
                                               const float vertices[4][3])
{
    TerrainState* terrain = (TerrainState*)context;
    if (terrain == NULL || terrain->derived_vertices == NULL || quad < 0 ||
        (uint32_t)quad >= terrain->derived_mesh.width *
                           terrain->derived_mesh.height) return 0;
    memcpy(terrain->derived_vertices + (size_t)quad * 12u,
           vertices, 12u * sizeof(float));
    return 1;
}

static int terrain_indexed_mesh_create(TerrainState* terrain,
                                       size_t quad_count)
{
    size_t index;
    if (terrain == NULL || quad_count == 0u ||
        quad_count > ((size_t)-1) / 0x70u ||
        quad_count > ((size_t)-1) / (32u * sizeof(float)) ||
        quad_count > 0xffffffffu) return 0;
    terrain->indexed_mesh_blocks = (uint8_t*)calloc(quad_count, 0x70u);
    terrain->indexed_mesh_vertices = (float*)calloc(
        quad_count * 32u, sizeof(float));
    terrain->derived_mesh.records = (uint8_t*)calloc(
        (size_t)terrain->dim[2] * terrain->dim[3], 8u);
    terrain->derived_vertices = (float*)calloc(
        (size_t)terrain->dim[2] * terrain->dim[3] * 12u, sizeof(float));
    if (terrain->indexed_mesh_blocks == NULL ||
        terrain->indexed_mesh_vertices == NULL ||
        terrain->derived_mesh.records == NULL ||
        terrain->derived_vertices == NULL) return 0;
    terrain->derived_mesh.width = terrain->dim[2];
    terrain->derived_mesh.height = terrain->dim[3];
    for (index = 0u; index < (size_t)terrain->dim[2] * terrain->dim[3];
         ++index) terrain->derived_mesh.records[index * 8u + 4u] = 1u;
    terrain->derived_mesh.dirty = &terrain->derived_mesh_dirty;
    terrain->derived_mesh.hooks.context = terrain;
    terrain->derived_mesh.hooks.create_quad = terrain_derived_quad_create;
    terrain->derived_mesh.hooks.vertices_emit =
        terrain_derived_quad_vertices_emit;
    terrain->indexed_mesh.block_count = (uint32_t)quad_count;
    terrain->indexed_mesh.reserved = 0u;
    terrain->indexed_mesh.blocks = terrain->indexed_mesh_blocks;
    terrain->detail_mesh_object.mesh_record = &terrain->indexed_mesh;
    terrain->detail_mesh_token =
        (uint32_t)(uintptr_t)&terrain->detail_mesh_object;
    for (index = 0u; index < quad_count; ++index) {
        uint8_t* block = terrain->indexed_mesh_blocks + index * 0x70u;
        uint32_t vertex_count = 4u;
        uint32_t detail_enabled = 1u;
        void* vertices = terrain->indexed_mesh_vertices + index * 32u;
        memcpy(block + 0x04u, &vertex_count, sizeof(vertex_count));
        memcpy(block + 0x0cu, &vertices, sizeof(vertices));
        memcpy(block + 0x6cu, &detail_enabled, sizeof(detail_enabled));
        terrain_indexed_mesh_cell_sync(terrain, index);
    }
    return 1;
}

static void* terrain_indexed_mesh_get(void* context, void* object)
{
    TerrainIndexedMeshObject* mesh_object =
        (TerrainIndexedMeshObject*)object;
    (void)context;
    if (mesh_object == NULL) return NULL;
    return mesh_object->mesh_record;
}

static uint32_t terrain_indexed_mesh_entry_get(void* context, void* mesh,
                                               unsigned int index)
{
    TerrainState::TerrainIndexedMeshRecord* record =
        (TerrainState::TerrainIndexedMeshRecord*)mesh;
    uint32_t flags;
    uint8_t* block;
    (void)context;
    if (record == NULL || record->blocks == NULL ||
        index >= record->block_count) return 0u;
    block = (uint8_t*)record->blocks + (size_t)index * 0x70u;
    memcpy(&flags, block + 0x6cu, sizeof(flags));
    return flags & 1u;
}

static const TerrainDetailLookupOps s_terrain_detail_lookup_ops = {
    NULL,
    terrain_indexed_mesh_get,
    terrain_indexed_mesh_entry_get,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static uint32_t terrain_indexed_mesh_vertex_data_read(
    void* context, void* mesh, const void* request_pointer)
{
    const TerrainVertexReadRequest* request =
        (const TerrainVertexReadRequest*)request_pointer;
    (void)context;
    if (mesh == NULL || request == NULL) return 0u;
    indexed_geometry_data_copy_to_output(mesh, request->block_index,
        request->first_vertex, request->count, request->output);
    return 0u;
}

static const LwoLayerDispatchOps s_terrain_indexed_mesh_dispatch_ops = {
    NULL,
    terrain_indexed_mesh_get,
    NULL,
    NULL,
    NULL,
    terrain_indexed_mesh_vertex_data_read,
    NULL,
    NULL,
    NULL
};

static void terrain_cell_orientation_set_level(LevelState* level, int x, int y,
                                              int flipped)
{
    TerrainState* terrain;
    TerrainSurfaceGrid* surface;
    if (level == NULL || x < 0 || y < 0) return;
    terrain = (TerrainState*)level->ptr_terrain;
    surface = level_surface_grid_get(level);
    if (surface != NULL)
        terrain_cell_surface_orientation_set(surface, (uint32_t)x,
                                             (uint32_t)y, flipped);
    if (terrain != NULL && terrain->blocks != NULL && x < terrain->width &&
        y < terrain->height) {
        uint8_t* record = terrain->blocks +
            ((size_t)y * (size_t)terrain->width + (size_t)x) *
                TERRAIN_BLOCK_STRIDE;
        record[0x19] = (uint8_t)((record[0x19] & (uint8_t)~1u) |
                                 (flipped != 0 ? 1u : 0u));
        if (x + 1 < terrain->width && y + 1 < terrain->height) {
            size_t quad_index = (size_t)y * (size_t)terrain->dim[0] +
                                (size_t)x;
            terrain_indexed_mesh_cell_sync(terrain, quad_index);
            if (terrain->derived_mesh.records != NULL &&
                (terrain->derived_mesh.records[((size_t)y *
                    terrain->derived_mesh.width + (uint32_t)x) * 8u + 4u] &
                 1u) == 0u) {
                terrain_cell_mutation_progress(level, (uint32_t)x,
                                                (uint32_t)y);
            }
        }
    }
    scene_render_terrain_cell_orientation_apply(level, (uint32_t)x,
                                               (uint32_t)y, flipped);
}

static void terrain_create_vertices_write(void* context, uint32_t cell,
                                          const float vertices[12])
{
    TerrainState* terrain = (TerrainState*)context;
    float* output;
    uint32_t corner;
    if (terrain == NULL || terrain->cell_vertices == NULL) return;
    output = terrain->cell_vertices + (size_t)cell * 12u;
    for (corner = 0u; corner < 4u; ++corner) {
        /* FUN_0044e4d0 emits retail records as X,Z,height. TerrainState's
         * SceneRender-facing cache stores X,height,Z. */
        output[corner * 3u] = vertices[corner * 3u];
        output[corner * 3u + 1u] = vertices[corner * 3u + 2u];
        output[corner * 3u + 2u] = vertices[corner * 3u + 1u];
        if (vertices[corner * 3u + 2u] < terrain->minimum_y)
            terrain->minimum_y = vertices[corner * 3u + 2u];
    }
}

static void terrain_create_cell_surface_vectors_rebuild(TerrainState* terrain,
                                                         uint32_t quad_index,
                                                         uint32_t record_index,
                                                         int flipped)
{
    float* vertices;
    float ordered_vertices[12];
    float first[3];
    float second[3];
    uint8_t* record;
    static const uint8_t forward[4] = { 3u, 0u, 1u, 2u };
    static const uint8_t identity[4] = { 0u, 1u, 2u, 3u };
    const uint8_t* order = flipped != 0 ? forward : identity;
    uint32_t corner;
    if (terrain == NULL || terrain->blocks == NULL ||
        terrain->cell_vertices == NULL ||
        quad_index >= terrain->dim[0] * terrain->dim[1] ||
        record_index >= terrain->dim[2] * terrain->dim[3]) return;
    vertices = terrain->cell_vertices + (size_t)quad_index * 12u;
    record = terrain->blocks + (size_t)record_index * TERRAIN_BLOCK_STRIDE;
    for (corner = 0u; corner < 4u; ++corner) {
        const float* source = vertices + order[corner] * 3u;
        ordered_vertices[corner * 3u] = source[0];
        ordered_vertices[corner * 3u + 1u] = source[2];
        ordered_vertices[corner * 3u + 2u] = source[1];
    }
    surface_triangle_normal_calculate(first, ordered_vertices,
                                      ordered_vertices + 3,
                                      ordered_vertices + 9);
    surface_triangle_normal_calculate(second, ordered_vertices + 3,
                                      ordered_vertices + 6,
                                      ordered_vertices + 9);
    memcpy(record, first, sizeof(first));
    memcpy(record + 0x0cu, second, sizeof(second));
}

// FUNCTION: LEGORR 0x0044e0c0
void* terrain_create_retail(void* engine_context, const char* surface_map,
                            float blocksize, float terrain_value)
{
    /* The retail entry receives DAT_005560d8 as its scene/resource owner.
     * Retain that owner with the CPU terrain even when its DDi mesh callbacks
     * are unavailable. */
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
    t->terrain_value = terrain_value;
    t->scene_context = engine_context;
    t->world_off_x = -(float)(m->width - 1) * blocksize * 0.5f;
    t->world_off_y = (float)(m->height - 1) * blocksize * 0.5f;

    size_t n = (size_t)m->width * m->height;
    t->blocks = (uint8_t*)calloc(n, TERRAIN_BLOCK_STRIDE);
    if (t->blocks == NULL) {
        level_map_close(m);
        free(t);
        return NULL;
    }
    t->variant_window = (uint8_t*)calloc(1u, CELL_VARIANT_WINDOW_SIZE);
    if (t->variant_window == NULL) {
        free(t->blocks);
        level_map_close(m);
        free(t);
        return NULL;
    }
    /* The retail TerrainState begins with the dimensions/stride and stores
     * its block table at +0x28; the compatibility lists occupy +0x34 and
     * +0x4034. Keep that view in a native sidecar because host pointers make
     * the compact 32-bit object layout unsuitable for inline tables. */
    memcpy(t->variant_window + 0x00, &t->dim[0], sizeof(uint32_t));
    memcpy(t->variant_window + 0x04, &t->dim[1], sizeof(uint32_t));
    memcpy(t->variant_window + 0x08, &t->dim[2], sizeof(uint32_t));
    memcpy(t->variant_window + 0x28, &t->blocks, sizeof(t->blocks));
    int x, y;
    for (y = 0; y < m->height; ++y) {
        for (x = 0; x < m->width; ++x) {
            uint8_t* rec = t->blocks
                           + ((size_t)y * m->width + x) * TERRAIN_BLOCK_STRIDE;
            rec[0x18] = 0;
            rec[0x19] = 0x60;
            rec[0x1A] = (uint8_t)level_map_cell(m, x, y);
            /* FUN_0044e0c0 draws two shared-stream values, normalizes their
             * vector, and stores it at +0x1c/+0x20 with magnitude 0.3. */
            float nx = random_float_range(0.0f, 1.0f);
            float ny = random_float_range(0.0f, 1.0f);
            float inv = 1.0f / (float)sqrt((double)(nx * nx + ny * ny));
            nx *= inv * 0.3f;
            ny *= inv * 0.3f;
            memcpy(rec + 0x1C, &nx, sizeof(nx));
            memcpy(rec + 0x20, &ny, sizeof(ny));
        }
    }
    /* TerrainGridVertexBuild creates the CPU copy of each retail quad. The
     * render backend can later mirror these records into its native mesh. */
    t->minimum_y = 0.0f;
    if (m->width > 1 && m->height > 1) {
        size_t quad_count = (size_t)(m->width - 1) * (size_t)(m->height - 1);
        if (quad_count > ((size_t)-1) / (12u * sizeof(float))) {
            terrain_destroy(t);
            level_map_close(m);
            return NULL;
        }
        t->cell_vertices = (float*)calloc(quad_count * 12u,
                                          sizeof(float));
        t->cell_detail_flags = (uint32_t*)calloc(quad_count,
                                                 sizeof(uint32_t));
        if (t->cell_vertices == NULL || t->cell_detail_flags == NULL) {
            terrain_destroy(t);
            level_map_close(m);
            return NULL;
        }
        /* TerrainCreate allocates one 4-vertex mesh record per quad, then
         * LwoLayerEnabledSet(scene, index, 1) sets its indexed +0x6c bit.
         * MeshRecordAllocateAndFill has zeroed that word first, so this is
         * the complete initial per-cell detail state. */
        {
            size_t detail_index;
            for (detail_index = 0u; detail_index < quad_count;
                 ++detail_index)
                t->cell_detail_flags[detail_index] = 1u;
        }
        {
            TerrainVertexBuildGrid build;
            memset(&build, 0, sizeof(build));
            build.width = (uint32_t)(m->width - 1);
            build.height = (uint32_t)(m->height - 1);
            build.map_width = (uint32_t)m->width;
            build.map_height = (uint32_t)m->height;
            /* FUN_0044e4d0 uses context +0x14 (TerrainCreate's fourth
             * argument) for height, while +0x10 controls X/Z spacing. */
            build.vertical_scale = terrain_value;
            build.spacing = blocksize;
            build.x_origin = t->world_off_x;
            build.z_origin = t->world_off_y;
            build.minimum_y = 0.0f;
            build.cell_records = t->blocks;
            build.context = t;
            build.vertices_write = terrain_create_vertices_write;
            terrain_grid_vertex_build(&build);
            t->minimum_y = build.minimum_y;
        }
    }
    /* Choose each quad's diagonal by comparing the two opposite-corner
     * height deltas. The lower delta is orientation bit 0 (finding 114).
     * Retail rebuilds both stored triangle vectors for every quad; choosing
     * a new orientation also marks the cell and its three neighboring
     * records dirty through TerrainCellOrientationToggleAndRebuild. */
    for (y = 0; y + 1 < m->height; ++y) {
        for (x = 0; x + 1 < m->width; ++x) {
            uint8_t* top_left = t->blocks +
                ((size_t)y * (size_t)m->width + (size_t)x) *
                    TERRAIN_BLOCK_STRIDE;
            uint8_t* top_right = top_left + TERRAIN_BLOCK_STRIDE;
            uint8_t* bottom_left = top_left +
                (size_t)m->width * TERRAIN_BLOCK_STRIDE;
            uint8_t* bottom_right = bottom_left + TERRAIN_BLOCK_STRIDE;
            int diagonal_ad = (int)top_left[0x1A] -
                              (int)bottom_right[0x1A];
            int diagonal_bc = (int)top_right[0x1A] -
                              (int)bottom_left[0x1A];
            if (diagonal_ad < 0) diagonal_ad = -diagonal_ad;
            if (diagonal_bc < 0) diagonal_bc = -diagonal_bc;
            if (diagonal_ad < diagonal_bc) {
                top_left[0x19] |= 1u;
                terrain_cell_changed_flag_set(t, x, y);
                terrain_cell_changed_flag_set(t, x + 1, y);
                terrain_cell_changed_flag_set(t, x + 1, y + 1);
                terrain_cell_changed_flag_set(t, x, y + 1);
            }
            terrain_create_cell_surface_vectors_rebuild(
                t, (uint32_t)y * t->dim[0] + (uint32_t)x,
                (uint32_t)y * t->dim[2] + (uint32_t)x,
                (top_left[0x19] & 1u) != 0u);
        }
    }
    if (m->width > 1 && m->height > 1) {
        size_t quad_count = (size_t)t->dim[0] * (size_t)t->dim[1];
        if (!terrain_indexed_mesh_create(t, quad_count)) {
            terrain_destroy(t);
            level_map_close(m);
            return NULL;
        }
    }
    level_map_close(m);
    return t;
}

void* terrain_create(const char* surface_map, float blocksize)
{
    return terrain_create_retail(NULL, surface_map, blocksize, 0.0f);
}

void terrain_destroy(void* handle)
{
    TerrainState* t = (TerrainState*)handle;
    if (t != NULL) {
        if (t->owner_level != NULL &&
            t->owner_level->ptr_tile_mesh_grid != NULL) {
            TerrainTileMeshGrid* tile_grid =
                t->owner_level->ptr_tile_mesh_grid;
            if (tile_grid->surface_text_map == t->variant_window) {
                tile_grid->surface_text_map = NULL;
                tile_grid->surface_text_map_insert = NULL;
            }
            if (tile_grid->surface_grid != NULL &&
                tile_grid->surface_grid->terrain_window ==
                    t->variant_window) {
                terrain_surface_grid_terrain_window_set(
                    tile_grid->surface_grid, NULL);
            }
        }
        {
            TerrainSurfaceGrid* surface = path_record_surface_grid_get();
            if (surface != NULL &&
                surface->terrain_window == t->variant_window) {
                terrain_surface_grid_terrain_window_set(surface, NULL);
            }
        }
        if (t->roof_scene != NULL) {
            if (indexed_surface_scene_owner_get() == t->roof_scene)
                graphics_overlay_resources_release();
            t->derived_mesh.records = NULL; /* released global owner table */
            t->roof_scene = t->roof_root = NULL;
        }
        t->owner_level = NULL;
        if (t->detail_surface != NULL) {
            free(t->detail_surface->surface_values);
            free(t->detail_surface);
        }
        free(t->derived_mesh.records);
        free(t->derived_vertices);
        free(t->indexed_mesh_vertices);
        free(t->indexed_mesh_blocks);
        free(t->cell_vertices);
        free(t->cell_detail_flags);
        free(t->variant_window);
        free(t->blocks);
        free(t);
    }
}

int terrain_cell_base_vertices_get(const void* handle, uint32_t x,
                                   uint32_t y, float vertices[12])
{
    const TerrainState* terrain = (const TerrainState*)handle;
    if (terrain == NULL || vertices == NULL ||
        terrain->cell_vertices == NULL || x >= terrain->dim[0] ||
        y >= terrain->dim[1]) return 0;
    memcpy(vertices, terrain->cell_vertices +
        ((size_t)y * terrain->dim[0] + x) * 12u, 12u * sizeof(float));
    return 1;
}

/* The retail geometry buffer stores four vertex records per quad in
 * row-major order. GridCellReadSurfaceValues copies the first three words of
 * each record verbatim; these are the mesh's X, Z, height words. The
 * +0x19 orientation bit reflects the cyclic vertex rotation already
 * performed on the retail geometry object. */
int terrain_cell_surface_values_available(const void* handle, int x, int y)
{
    const TerrainState* terrain = (const TerrainState*)handle;
    if (terrain == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= terrain->dim[0] ||
        (uint32_t)y >= terrain->dim[1]) return 0;
    if (terrain->cell_vertices != NULL) return 1;
    if (terrain->owner_level != NULL) {
        float vertices[4][3];
        return scene_render_terrain_cell_vertices_get(
            terrain->owner_level, (uint32_t)x, (uint32_t)y, vertices);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00450220
void grid_cell_read_surface_values(void* terrain, int x, int y,
                                   float out_values[12])
{
    TerrainState* state = (TerrainState*)terrain;
    float indexed_output[4][9];
    TerrainIndexedMeshLayerObject object;
    TerrainIndexedMeshLayerOwner owner;
    size_t quad_index;
    uint32_t corner;
    uint32_t component;
    if (state == NULL || out_values == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= state->dim[0] ||
        (uint32_t)y >= state->dim[1]) return;
    quad_index = (size_t)(uint32_t)y * state->dim[0] + (uint32_t)x;
    if (state->indexed_mesh_blocks == NULL ||
        state->indexed_mesh_vertices == NULL) return;
    terrain_indexed_mesh_cell_sync(state, quad_index);
    memset(indexed_output, 0, sizeof(indexed_output));
    terrain_indexed_mesh_layer_object_init(state, &object, &owner);
    (void)lwo_layer_polygon_vertex_data_read(&object, (int)quad_index, 0,
                                            4, indexed_output);
    for (corner = 0u; corner < 4u; ++corner)
        for (component = 0u; component < 3u; ++component)
            out_values[corner * 3u + component] =
                indexed_output[corner][component];
}

/* Retail GridCellSetPayloadIndex asks the TerrainCreate scene object at
 * +0x24 for the detail entry at width * y + x, then discards the result.
 * Keep this target wrapper separate from the host eligibility predicate
 * below, which reads the same mirrored enable bit. */
// FUNCTION: LEGORR 0x0044e860
void grid_cell_set_payload_index(void* terrain_handle, int x, int y)
{
    TerrainState* terrain = (TerrainState*)terrain_handle;
    TerrainIndexedMeshLayerObject object;
    TerrainIndexedMeshLayerOwner owner;
    unsigned int entry_index;
    if (terrain == NULL) return;
    entry_index = (unsigned int)((int64_t)terrain->dim[2] * y + x);
    terrain_indexed_mesh_layer_object_init(terrain, &object, &owner);
    (void)terrain_detail_entry_get_at_index(&object, entry_index);
}

/* TerrainCreate initializes each indexed record's +0x6c bit, and
 * GridSurfaceWriteAtXY updates that bit. This host predicate is used by
 * entity eligibility and does not alter GridCellSetPayloadIndex's void ABI. */
int grid_cell_detail_enabled_get(void* terrain_handle, int x, int y)
{
    const TerrainState* terrain = (const TerrainState*)terrain_handle;
    size_t cell_index;
    uint32_t flags;
    if (terrain == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= terrain->dim[0] || (uint32_t)y >= terrain->dim[1])
        return 0;
    if (terrain->cell_detail_flags == NULL) return 0;
    cell_index = (size_t)(uint32_t)y * (size_t)terrain->dim[0]
        + (uint32_t)x;
    if (terrain->indexed_mesh_blocks != NULL) {
        memcpy(&flags, terrain->indexed_mesh_blocks + cell_index * 0x70u +
               0x6cu, sizeof(flags));
        return (flags & 1u) != 0u;
    }
    return (terrain->cell_detail_flags[cell_index] & 1u) != 0u;
}

void terrain_detail_enable_set(void* terrain_handle, uint32_t index,
                               int enabled)
{
    TerrainState* terrain = (TerrainState*)terrain_handle;
    size_t quad_count;
    if (terrain == NULL || terrain->cell_detail_flags == NULL ||
        terrain->dim[0] == 0u || terrain->dim[1] == 0u)
        return;
    quad_count = (size_t)terrain->dim[0] * (size_t)terrain->dim[1];
    if ((size_t)index >= quad_count) return;
    if (enabled != 0)
        terrain->cell_detail_flags[index] |= 1u;
    else
        terrain->cell_detail_flags[index] &= ~1u;
    if (terrain->indexed_mesh_blocks != NULL) {
        uint8_t* block = terrain->indexed_mesh_blocks +
            (size_t)index * 0x70u;
        uint32_t flags;
        memcpy(&flags, block + 0x6cu, sizeof(flags));
        if (enabled != 0) flags |= 1u;
        else flags &= ~1u;
        memcpy(block + 0x6cu, &flags, sizeof(flags));
    }
}

TerrainSurfaceGrid* terrain_camera_detail_grid_get(LevelState* level)
{
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    TerrainSurfaceGrid* grid;
    size_t count;
    if (terrain == NULL || terrain->owner_level != level ||
        terrain->indexed_mesh_blocks == NULL ||
        terrain->cell_detail_flags == NULL || terrain->dim[0] == 0u ||
        terrain->dim[1] == 0u) {
        fprintf(stderr, "camera-diag: grid prerequisites level=%p terrain=%p owner=%p mesh=%p flags=%p dims=%u,%u\n",
            level, terrain, terrain ? terrain->owner_level : NULL,
            terrain ? terrain->indexed_mesh_blocks : NULL,
            terrain ? terrain->cell_detail_flags : NULL,
            terrain ? terrain->dim[0] : 0u, terrain ? terrain->dim[1] : 0u);
        fflush(stderr);
        return NULL;
    }
    if (terrain->detail_surface != NULL) return terrain->detail_surface;
    count = (size_t)terrain->dim[0] * terrain->dim[1];
    grid = (TerrainSurfaceGrid*)calloc(1u, sizeof(TerrainSurfaceGrid));
    if (grid == NULL) { fprintf(stderr, "camera-diag: grid allocation failed\n"); fflush(stderr); return NULL; }
    grid->surface_values = (uint8_t*)malloc(count);
    if (grid->surface_values == NULL) { fprintf(stderr, "camera-diag: grid values allocation failed count=%lu\n", (unsigned long)count); fflush(stderr); free(grid); return NULL; }
    memset(grid->surface_values, 1, count);
    grid->width = terrain->dim[0];
    grid->height = terrain->dim[1];
    grid->record_width = terrain->dim[2];
    grid->cell_records = terrain->blocks;
    grid->terrain_window = terrain->variant_window;
    grid->owner_level = level;
    terrain->detail_surface = grid;
    return grid;
}

/* LAB_00429110 is the transient-effect callback, not palette helper429160.
 * The native callback is synchronous between clearing and queueing a cell. */
static LevelState* s_camera_detail_callback_level;
static int s_camera_detail_callback_ready;
static void terrain_camera_detail_cell_callback(int x, int y)
{
    uint8_t* cell = level_voxel(s_camera_detail_callback_level, x, y);
    uint32_t flags;
    void* effect;
    if (cell == NULL) return;
    memcpy(&flags, cell + 8, sizeof(flags));
    if ((flags & 0x4000u) == 0u) return;
    effect = cell_transient_effect_get(s_camera_detail_callback_level, cell);
    if (effect == NULL) { fprintf(stderr, "camera-diag: effect missing cell=%d,%d flags=%08lx\n", x, y, (unsigned long)flags); fflush(stderr); s_camera_detail_callback_ready = 0; return; }
    transient_effect_enable_set(effect, 0);
}

int terrain_camera_detail_collect(LevelState* level, const float position[3],
                                  const float forward[3], int primary)
{
    TerrainSurfaceGrid* grid = terrain_camera_detail_grid_get(level);
    const TerrainState* terrain = level != NULL
        ? (const TerrainState*)level->ptr_terrain : NULL;
    float world_x, world_y;
    int center_x, center_y, dx, dy;
    const int radius = primary ? 8 : 2;
    uint32_t required = 0u;
    LevelState* prior_callback_level;
    int ready;
    if (grid == NULL || position == NULL ||
        (primary && forward == NULL)) { fprintf(stderr, "camera-diag: collect arguments grid=%p position=%p forward=%p primary=%d\n", grid, position, forward, primary); fflush(stderr); return 0; }
    /* Native42921d reads TerrainCreate+10 spacing, then exact49e2d4=3.
     * It uses the follow position plus the actual eye's world forward. */
    world_x = position[0]; world_y = position[1];
    if (primary) {
        float lead = terrain->blocksize * 3.0f;
        world_x += forward[0] * lead;
        world_y += forward[1] * lead;
    }
    /* 44f6d0 writes cell coordinates even when outside physical bounds;
     * the radius scan performs the per-cell admission. */
    (void)terrain_world_xy_to_cell(terrain, world_x, world_y,
                                    &center_x, &center_y);
    if (center_x < INT_MIN + radius || center_x > INT_MAX - radius ||
        center_y < INT_MIN + radius || center_y > INT_MAX - radius) {
        fprintf(stderr, "camera-diag: center bounds x=%d y=%d radius=%d\n", center_x, center_y, radius); fflush(stderr);
        return 0;
    }
    for (dy = -radius; dy <= radius; ++dy)
        for (dx = -radius; dx <= radius; ++dx)
            if (dx * dx + dy * dy <= radius * radius &&
                center_x + dx >= 0 && center_y + dy >= 0 &&
                (uint32_t)(center_x + dx) < grid->width &&
                (uint32_t)(center_y + dy) < grid->height) ++required;
    /* Never clear a cell whose restoration cannot be retained. The real
     * radius8 plus radius2 passes fit the native queue's512 entries. */
    if (grid->changed_count > TERRAIN_GRID_CHANGE_CAPACITY ||
        required > TERRAIN_GRID_CHANGE_CAPACITY - grid->changed_count) {
        fprintf(stderr, "camera-diag: queue capacity changed=%lu required=%lu\n", (unsigned long)grid->changed_count, (unsigned long)required); fflush(stderr);
        return 0;
    }
    prior_callback_level = s_camera_detail_callback_level;
    s_camera_detail_callback_level = level;
    s_camera_detail_callback_ready = 1;
    terrain_surface_clear_circle(grid, center_x, center_y, radius,
        primary ? terrain_camera_detail_cell_callback : NULL);
    ready = s_camera_detail_callback_ready;
    s_camera_detail_callback_level = prior_callback_level;
    return ready;
}

void terrain_camera_detail_restore(LevelState* level)
{
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    if (terrain != NULL && terrain->owner_level == level &&
        terrain->detail_surface != NULL)
        terrain_cell_change_queue_flush(terrain->detail_surface);
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

/* Presentation fallback uses TerrainCreate's actual fourth argument, including
 * zero and negative scales; block spacing is an independent quantity. */
float terrain_height_scale_get(const void* handle)
{
    const TerrainState* t = (const TerrainState*)handle;
    return t != NULL ? t->terrain_value : 0.0f;
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
    return terrain_cell_world_center_xy_transform(handle, x, y, position_xy);
}

int terrain_cell_world_center_xy_transform(const void* handle, int x, int y,
                                           float position_xy[2])
{
    const TerrainState* t = (const TerrainState*)handle;
    if (t == NULL || position_xy == NULL) return 0;
    position_xy[0] = t->blocksize * 0.5f + (float)x * t->blocksize
        + t->world_off_x;
    position_xy[1] = t->world_off_y -
        (t->blocksize * 0.5f + (float)y * t->blocksize);
    return 1;
}

/* WorldToTerrainCellBoundsCheck (FUN_0044f700): invert the vertex-origin
 * transform, convert through the target CRT, then check physical quads.
 * The half-block offset belongs only to MapCellToWorldCenter. */
int terrain_world_xy_to_cell(const void* handle, float x, float y,
                             int* cell_x, int* cell_y)
{
    const TerrainState* t = (const TerrainState*)handle;
    int mapped_x, mapped_y;
    if (t == NULL || t->blocksize == 0.0f) return 0;
    mapped_x = crt_float_to_long(
        (x - t->world_off_x) / t->blocksize);
    mapped_y = crt_float_to_long(
        (t->world_off_y - y) / t->blocksize);
    if (cell_x != NULL) *cell_x = mapped_x;
    if (cell_y != NULL) *cell_y = mapped_y;
    /* 0x44f700 bounds physical quads (dim[0/1] = width/height - 1),
     * after writing the truncated results, even when outside the grid. */
    return mapped_x >= 0 && mapped_y >= 0 &&
        (uint32_t)mapped_x < t->dim[0] &&
        (uint32_t)mapped_y < t->dim[1];
}

/* TerrainSurfacePointProjectOffset (FUN_0044fb90): choose the cell edge
 * intersected by the supplied horizontal normal, intersect that edge with
 * the point's normal line, then apply the requested block-scaled offset.
 * The reference receives its TerrainState and four-corner data from the
 * active renderer; this path uses TerrainState's retained CPU quad and the
 * two triangle normals stored in its 0x28-byte cell record. */
// FUNCTION: LEGORR 0x0044fb90
int terrain_state_surface_point_project_offset(
    void* terrain_handle, float world_x, float world_z, int orientation,
    float offset, float* out_x, float* out_z)
{
    static const uint8_t flipped_order[4] = { 1u, 2u, 3u, 0u };
    TerrainState* terrain = (TerrainState*)terrain_handle;
    int cell_x, cell_y;
    size_t quad_index, record_index;
    uint8_t* record;
    float vertices[4][3];
    float normal_x, normal_z;
    float edge_point[2], edge_direction[2];
    float point[2], normal_direction[2], intersection[2];
    float length, scaled_offset;
    uint32_t corner;
    int flipped, first_corner, second_corner;
    if (terrain == NULL || terrain->blocks == NULL ||
        terrain->cell_vertices == NULL || out_x == NULL || out_z == NULL ||
        !terrain_world_xy_to_cell(terrain, world_x, world_z,
                                  &cell_x, &cell_y) ||
        cell_x < 0 || cell_y < 0 || cell_x >= terrain->width - 1 ||
        cell_y >= terrain->height - 1) return 0;

    quad_index = (size_t)(uint32_t)cell_y * terrain->dim[0] +
                 (uint32_t)cell_x;
    record_index = (size_t)(uint32_t)cell_y * terrain->dim[2] +
                   (uint32_t)cell_x;
    record = terrain->blocks + record_index * TERRAIN_BLOCK_STRIDE;
    flipped = (record[0x19] & 1u) != 0u;
    for (corner = 0u; corner < 4u; ++corner) {
        const uint32_t source = flipped ? flipped_order[corner] : corner;
        memcpy(vertices[corner], terrain->cell_vertices +
               quad_index * 12u + source * 3u, 3u * sizeof(float));
    }

    /* FUN_0044fb90 selects the stored triangle normal using the same
     * Manhattan-distance tests against the first two sampled corners. */
    if (!flipped) {
        if ((world_x - vertices[0][0]) +
            (vertices[1][2] - world_z) < terrain->blocksize)
            record += 0u;
        else
            record += 0x0cu;
    }
    else {
        if ((vertices[1][0] - world_x) +
            (vertices[1][2] - world_z) < terrain->blocksize)
            record += 0x0cu;
        else
            record += 0u;
    }
    normal_x = *(float*)record;
    normal_z = *(float*)(record + 4u);

    if (orientation == 0) {
        if ((float)fabs((double)normal_x) <=
            (float)fabs((double)normal_z)) {
            if (normal_z >= 0.0f) {
                first_corner = 0;
                second_corner = 1;
            }
            else {
                first_corner = 2;
                second_corner = 3;
            }
        }
        else if (normal_x >= 0.0f) {
            first_corner = 1;
            second_corner = 2;
        }
        else {
            first_corner = 0;
            second_corner = 3;
        }
    }
    else if (flipped) {
        first_corner = 1;
        second_corner = 3;
    }
    else {
        first_corner = 0;
        second_corner = 2;
    }

    edge_point[0] = vertices[first_corner][0];
    edge_point[1] = vertices[first_corner][2];
    edge_direction[0] = vertices[second_corner][0] - edge_point[0];
    edge_direction[1] = vertices[second_corner][2] - edge_point[1];
    point[0] = world_x;
    point[1] = world_z;
    normal_direction[0] = normal_x;
    normal_direction[1] = normal_z;
    if (line_intersection_2d_compute(intersection, edge_point,
            edge_direction, point, normal_direction) == NULL) return 0;

    length = (float)sqrt((double)(normal_x * normal_x +
                                  normal_z * normal_z));
    if (length == 0.0f) return 0;
    scaled_offset = offset * terrain->blocksize;
    *out_x = intersection[0] - (normal_x / length) * scaled_offset;
    *out_z = intersection[1] - (normal_z / length) * scaled_offset;
    return 1;
}

static int terrain_surface_triangle_height(const float a[3], const float b[3],
                                           const float c[3], float x, float z,
                                           float* height_out)
{
    const float denominator = (b[2] - c[2]) * (a[0] - c[0]) +
                              (c[0] - b[0]) * (a[2] - c[2]);
    float wa, wb, wc;
    if (height_out == NULL || denominator == 0.0f) return 0;
    wa = ((b[2] - c[2]) * (x - c[0]) +
          (c[0] - b[0]) * (z - c[2])) / denominator;
    wb = ((c[2] - a[2]) * (x - c[0]) +
          (a[0] - c[0]) * (z - c[2])) / denominator;
    wc = 1.0f - wa - wb;
    if (wa < -0.0001f || wb < -0.0001f || wc < -0.0001f) return 0;
    *height_out = wa * a[1] + wb * b[1] + wc * c[1];
    return 1;
}

/* TerrainSurfaceHeightInterpolate (FUN_0044f940): choose the cell under the
 * world point, use its live four-vertex CPU quad, and interpolate in the
 * target's two triangles (0,1,3) and (1,2,3). TerrainCreate's two stored
 * triangle records use this 1-to-3 diagonal; the cell orientation bit rotates
 * the four vertices before interpolation. Heights are already in world units
 * after TerrainGridVertexBuild, so the target returns the interpolated value
 * without another scale. */
// FUNCTION: LEGORR 0x0044f940
float terrain_surface_height_interpolate(const void* handle, float world_x,
                                         float world_z)
{
    static const uint8_t flipped_order[4] = { 1u, 2u, 3u, 0u };
    const TerrainState* terrain = (const TerrainState*)handle;
    int cell_x, cell_y;
    size_t cell_index;
    uint8_t* record;
    float vertices[4][3];
    float height;
    uint32_t corner;
    int flipped;
    if (terrain == NULL || terrain->blocks == NULL ||
        terrain->cell_vertices == NULL ||
        !terrain_world_xy_to_cell(handle, world_x, world_z,
                                  &cell_x, &cell_y) ||
        cell_x < 0 || cell_y < 0 || cell_x >= terrain->width - 1 ||
        cell_y >= terrain->height - 1) return 0.0f;
    cell_index = (size_t)cell_y * (size_t)(terrain->width - 1) +
                 (size_t)cell_x;
    record = terrain->blocks +
        ((size_t)cell_y * (size_t)terrain->width + (size_t)cell_x) *
            TERRAIN_BLOCK_STRIDE;
    flipped = (record[0x19] & 1u) != 0u;
    for (corner = 0u; corner < 4u; ++corner) {
        const uint32_t source = flipped
            ? flipped_order[corner] : corner;
        memcpy(vertices[corner],
               terrain->cell_vertices + cell_index * 12u + source * 3u,
               3u * sizeof(float));
    }
    if (terrain_surface_triangle_height(vertices[0], vertices[1],
                                        vertices[3], world_x, world_z,
                                        &height) ||
        terrain_surface_triangle_height(vertices[1], vertices[2],
                                        vertices[3], world_x, world_z,
                                        &height)) return height;
    return 0.0f;
}

/* 0x44fe70 samples the authored SurfaceMap plane, independently of the
 * live/mutated quad used by 0x44f940. Byte decoding and RoughLevel scaling
 * stay inside the terrain owner; the camera receives a world-space height. */
// FUNCTION: LEGORR 0x0044fe70
float terrain_authored_surface_height_interpolate(const void* handle,
                                                  float world_x,
                                                  float world_y)
{
    const TerrainState* terrain = (const TerrainState*)handle;
    static const int corner_x[4] = { 0, 1, 1, 0 };
    static const int corner_y[4] = { 0, 0, 1, 1 };
    float height[4];
    float fx, fy;
    int x, y, corner;
    if (terrain == NULL || terrain->blocks == NULL ||
        terrain->blocksize == 0.0f ||
        !terrain_world_xy_to_cell(handle, world_x, world_y, &x, &y))
        return 0.0f;
    fx = (world_x - terrain->world_off_x) / terrain->blocksize - (float)x;
    fy = (terrain->world_off_y - world_y) / terrain->blocksize - (float)y;
    for (corner = 0; corner < 4; ++corner) {
        size_t index = ((size_t)(y + corner_y[corner]) *
            (size_t)terrain->width + (size_t)(x + corner_x[corner])) *
            TERRAIN_BLOCK_STRIDE;
        height[corner] = -(float)terrain->blocks[index + 0x1a] *
            terrain->terrain_value;
    }
    /* The reference uses the fixed 00-to-11 diagonal and does not consult
     * the live mesh's orientation flag, vertex offsets or mutation state. */
    if (fy >= fx)
        return height[0] + (height[2] - height[3]) * fx +
            (height[3] - height[0]) * fy;
    return height[0] + (height[1] - height[0]) * fx +
        (height[2] - height[1]) * fy;
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
    t->dirty_flags |= 2u;
}

/* First half of TerrainDirtyCellUpdate (FUN_0044eff0). The ticker owns the
 * later dirty-list interpolation (FUN_00450b60) and transition dispatch
 * (FUN_0044f090); this level-owned pass uses the actual TerrainState records
 * instead of the detached EngineState::sim_grid mirror. */
void terrain_dirty_cell_vectors_refresh(LevelState* level)
{
    TerrainState* terrain;
    TerrainSurfaceGrid* surface;
    uint32_t y;
    if (level == NULL) level = level_current_owner_get();
    terrain = level != NULL ? (TerrainState*)level->ptr_terrain : NULL;
    if (terrain == NULL || terrain->blocks == NULL ||
        (terrain->dirty_flags & 2u) == 0u) return;
    surface = level_surface_grid_get(level);
    /* FUN_00450f30 writes renderer-owned surface vectors. Keep the dirty
     * records pending if that owner has not been installed yet. */
    if (surface == NULL) return;
    for (y = 0u; y < (uint32_t)terrain->height; ++y) {
        uint32_t x;
        for (x = 0u; x < (uint32_t)terrain->width; ++x) {
            uint8_t* record = terrain->blocks +
                ((size_t)y * (size_t)terrain->width + x) *
                    TERRAIN_BLOCK_STRIDE;
            if ((record[0x19] & 4u) == 0u) continue;
            terrain_cell_surface_vectors_initialize(surface, x, y);
            record[0x19] &= (uint8_t)~4u;
        }
    }
    terrain->dirty_flags &= ~2u;
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
int s_predug_coordinates[20][2];
// GLOBAL: LEGORR 0x004e853c
unsigned int s_predug_coordinate_count;
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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void predug_coordinate_pool_append(const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [s_predug_coordinate_count]
        cmp ecx, 14h
        jae predug_append_return
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [eax]
        mov dword ptr [ecx*8 + s_predug_coordinates], edx
        mov ecx, dword ptr [s_predug_coordinate_count]
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx*8 + s_predug_coordinates + 4], eax
        mov eax, dword ptr [s_predug_coordinate_count]
        inc eax
        mov dword ptr [s_predug_coordinate_count], eax
    predug_append_return:
        ret
    }
}
#else
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
#endif

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
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            int value = (int)level_map_cell_get((uint32_t)slot,
                                                (int)x, (int)y) - bias;
            uint8_t* record = voxels + index * LEVEL_VOXEL_STRIDE;
            if (value == 5) {
                value = 4;
            }
            record[0x02] = (uint8_t)value;
            if ((int8_t)record[0x02] == 6) {
                record[0x07] = 4;
            } else if ((int8_t)record[0x02] == 11) {
                /* rock-monster / cavern spawn point */
                int cell_xy[2];
                cell_xy[0] = (int)x;
                cell_xy[1] = (int)y;
                unported_ledger_stubs::type11_cell_coordinate_append(
                    cell_xy);
                terrain_activity_build_record_update(s, (int)x, (int)y, 1);
            }
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* LoadEmergeLayer @ 0x42bf40 — even nonzero values seed an erosion stage;
 * zero and odd values are rounded up before the voxel's +0x06 write. */
// FUNCTION: LEGORR 0x0042bf40
static int load_emerge(LevelState* s, const char* name)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            uint16_t value = level_map_cell_get((uint32_t)slot,
                                                (int)x, (int)y);
            int seed_stage = 0;
            if (value == 0u || (value & 1u) != 0u) {
                ++value;
            } else {
                seed_stage = 1;
            }
            voxels[index * LEVEL_VOXEL_STRIDE + 0x06] =
                (uint8_t)(value >> 1);
            if (seed_stage != 0) {
                int cell_xy[2];
                int16_t random_value = (int16_t)level_rand_u16();
                int sign = random_value >> 31;
                int magnitude = ((int)random_value ^ sign) - sign;
                int phase = ((magnitude & 3) ^ sign) - sign;
                cell_xy[0] = (int)x;
                cell_xy[1] = (int)y;
                lava_erosion_stage_register(cell_xy, phase);
            }
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* LoadPathAiLayers @ 0x42c6a0 — reads the CryOreMap, writes path/AI byte
 * (value − bias) at voxel +0x05. */
// FUNCTION: LEGORR 0x0042c6a0
static int load_path_ai(LevelState* s, const char* name, int bias)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    int8_t bias_byte = (int8_t)bias;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            int8_t value = (int8_t)(uint8_t)
                level_map_cell_get((uint32_t)slot, (int)x, (int)y);
            voxels[index * LEVEL_VOXEL_STRIDE + 0x05] =
                (uint8_t)(value - bias_byte);
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* LoadFallinLayer @ 0x42c760 — reads the PathMap, ORs fallin flag bits into
 * voxel +0x08 over an existing cave flag (the corpus's `(flags & 8)` gate). */
// FUNCTION: LEGORR 0x0042c760
static int load_fallin(LevelState* s, const char* name, int bias)
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
            uint8_t* rec = voxel_at(s, x, y);
            uint32_t flags;
            __builtin_memcpy(&flags, rec + 0x08, 4);
            if ((flags & 8) != 0) {
                if (v == 2) {
                    flags |= 0x20000000u;
                } else if (v == 1) {
                    /* LoadFallinLayer runs TerrainCellWorkFlagClear with
                     * selector 0x3d for this branch. That helper owns the
                     * descriptor veto, activity update, coordinate callback,
                     * and four queued events before restoring flag bits 0/1. */
                    const int cell_xy[2] = { x, y };
                    terrain_cell_work_flag_clear_current(cell_xy, 0x3du);
                    continue;
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
static int load_block_pointers(LevelState* s, const char* name)
{
    /* DAT_00556f18 is also the camera's block-pointer-grid enable word. */
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot;
    uint8_t* voxels;
    g_camera_grid_enabled = 0;
    if (name == NULL) {
        return 0;
    }
    slot = map_open2(name, &image_size);
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
#if defined(_MSC_VER) && defined(_M_IX86)
    voxels = (uint8_t*)(uintptr_t)
        level_cptr(s)[LEVEL_OFF_VOXELS / 4];
#else
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
#endif
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            uint8_t* record = voxels + index * LEVEL_VOXEL_STRIDE;
            uint32_t v = level_map_cell_get((uint32_t)slot,
                                            (int)x, (int)y);
            *(uint32_t*)(record + 0x3C) = 0u;
            if (v != 0) {
                int16_t seed = *(int16_t*)(record + 0x20);
                uint32_t hash = (uint32_t)(int32_t)seed %
                    (v * (uint32_t)g_camera_scale_14 * 0x19u);
                *(uint32_t*)(record + 0x3C) = v;
                *(float*)(record + 0x40) = (float)hash;
                *(uint32_t*)(record + 0x38) = 0u;
                if (v > 4) {
                    *(uint32_t*)(record + 0x3C) = v - 4u;
                    *(uint32_t*)(record + 0x38) = 1u;
                }
            }
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    g_camera_grid_enabled = 1;
    return 1;
}

/* LoadTerrainMap @ 0x42cc90 — reads the BlockPointersMap, writes surface
 * type byte (value − bias) at voxel +0x04 (mapping note: level.hpp). */
// FUNCTION: LEGORR 0x0042cc90
static int load_terrain_map(LevelState* s, const char* name, int bias)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    int8_t bias_byte = (int8_t)bias;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            int8_t value = (int8_t)(uint8_t)
                level_map_cell_get((uint32_t)slot, (int)x, (int)y);
            voxels[index * LEVEL_VOXEL_STRIDE + 0x04] =
                (uint8_t)(value - bias_byte);
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* LoadLayerL @ 0x42c120 — reads the EmergeMap; ORs `0x40`/`0x80` flag bits
 * at voxel +0x0C and appends 0x4C-stride records into level+0x08. */
// FUNCTION: LEGORR 0x0042c120
static int load_layer_l(LevelState* s, const char* name)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot;
    uint8_t* voxels;
    uint8_t* records = (uint8_t*)level_load_ptr(s, LEVEL_OFF_LAYERL_RECORDS);
    int      count   = level_int(s, LEVEL_OFF_LAYERL_COUNT);

    /* MapOpen2's retail entry takes only (path, size_out), with the map-slot
     * owner installed separately. Layer L then uses the same slot helpers as
     * the decompile instead of allocating a host MapFile wrapper. */
    slot = map_open2(name, &image_size);
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint16_t c = level_map_cell_get((uint32_t)slot, (int)x, (int)y);
            if (c == 0) {
                continue;
            }
            int record_slot = ((int)c - 1) % 6;
            int rec  = ((int)c - 1) / 6;
            if (rec >= count) {
                int next_count = rec + 1;
                size_t old_size = (size_t)count * 0x4Cu;
                size_t new_size = (size_t)next_count * 0x4Cu;
                uint8_t* grown;
                /* The array is released with free() by level_destroy and
                 * grown with realloc(); on native builds engine_heap_alloc
                 * is the CRT pool allocator, whose blocks free()/realloc()
                 * cannot take, so allocate with malloc (the same call on the
                 * reference build, where engine_heap_alloc is a macro). */
                if (records == NULL)
                    grown = (uint8_t*)malloc(new_size);
                else
                    grown = (uint8_t*)realloc(records, new_size);
                if (grown == NULL) {
                    level_store_ptr(s, LEVEL_OFF_LAYERL_RECORDS, records);
                    level_set_int(s, LEVEL_OFF_LAYERL_COUNT, count);
                    level_map_slot_release((uint32_t)slot);
                    return 0;
                }
                records = grown;
                __builtin_memset(records + old_size, 0, new_size - old_size);
                count = next_count;
                level_store_ptr(s, LEVEL_OFF_LAYERL_RECORDS, records);
                level_set_int(s, LEVEL_OFF_LAYERL_COUNT, count);
            }
            uint8_t* recp = records + (size_t)rec * 0x4C;
            uint8_t* vox = voxels + ((size_t)y * width + x) * 0x48u;
            if (record_slot == 0) {
                *(uint32_t*)(vox + 0x0C) |= 0x80u;
                *(uint32_t*)(recp + 4) = x;
                *(uint32_t*)(recp + 8) = y;
            } else {
                *(uint32_t*)(vox + 0x0C) |= 0x40u;
                *(uint32_t*)(recp + (size_t)record_slot * 0xC + 4) = x;
                *(uint32_t*)(recp + (size_t)record_slot * 0xC + 8) = y;
                *(uint32_t*)(recp +
                    (size_t)(record_slot + 1) * 0xC) = 1u;
            }
        }
    }
    level_store_ptr(s, LEVEL_OFF_LAYERL_RECORDS, records);
    level_set_int(s, LEVEL_OFF_LAYERL_COUNT, count);
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* LoadLayerM @ 0x42c060 — reads the AIMap, writes raw byte at voxel +0x34. */
// FUNCTION: LEGORR 0x0042c060
static int load_layer_m(LevelState* s, const char* name)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            voxels[index * LEVEL_VOXEL_STRIDE + 0x34] =
                (uint8_t)level_map_cell_get((uint32_t)slot, (int)x, (int)y);
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* CellInitPass @ 0x42f6f0. The target processes one cell coordinate at a
 * time. The port applies record flags/types, task-counter transitions,
 * descriptor veto, erosion queue state, terrain dirty/material state,
 * callbacks, and diagonal updates to the installed per-level surface grid
 * and CPU terrain cache. Cell-kind records are level-owned; only their DDi
 * object factory and scene attachment remain behind renderer hooks. */
// FUNCTION: LEGORR 0x0042f6f0
/* Callers in the reference often push a fourth zero that FUN_0042f6f0 does
 * not inspect. Keep the worker variadic so those target-shaped call sites can
 * preserve that stack shape while older portable callers omit it. */
static void cell_init_pass(LevelState* s, float cell_x, uint32_t cell_y, ...)
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
    map_cell_record_grid_activate(s);
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
            if ((primary & 4u) == 0u ||
                (rec[2] >= 1u && rec[2] <= 5u)) {
                rec[1] = 0x70u;
            }
            rec[3] = 0u;
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
            if (solid_count == 3) {
                /* The retail loop stores each non-solid corner into the
                 * first byte of local_38; with three solid corners that is
                 * the sole empty index, copied directly to record +3. */
                orientation = empty_neighbours[0];
            }
            else {
                for (pair = 0; pair < 6; ++pair) {
                    if ((empty_neighbours[0] == empty_pairs[pair][0] &&
                         empty_neighbours[1] == empty_pairs[pair][1]) ||
                        (empty_neighbours[1] == empty_pairs[pair][0] &&
                         empty_neighbours[0] == empty_pairs[pair][1])) {
                        orientation = (uint8_t)pair;
                        break;
                    }
                }
            }
            /* The two-solid branch preserves bit 0x20 from uVar6; the
             * one/three-solid branches use the stricter 0xffffdf07 mask. */
            primary |= 0x14u;
            if (orientation > 3u) {
                if (rec[2] >= 1 && rec[2] <= 5)
                    rec[1] = 0x77;
                primary |= 0x2000u;
                rec[3] = (uint8_t)(orientation - 4u);
                terrain_cell_orientation_set_level(s, x, (int)cell_y,
                    (int)((orientation - 4u) & 1u));
            }
            else if ((original_primary & 0x20u) != 0u) {
                if (rec[2] >= 1u && rec[2] <= 5u) {
                    rec[1] = partial_shapes[rec[2] - 1];
                }
                /* The retail switch defaults directly to the common label,
                 * which writes the orientation even for unsupported types. */
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

    /* FUN_0040e1e0 / FUN_0040e350 use this level's 0x40cdc0 sidecar. The
     * eligibility and table state are native; only scene-object creation is
     * delegated to the typed renderer hook. */
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
                map_cell_tracking_activity_state_update(game, s);
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

    if (solid_count == 4) {
        uint32_t has_special_neighbor =
            unported_ledger_stubs::ledger_stub_0040e1e0_CellCornerOccupancyScan(
                s, x, (int)cell_y);
        terrain_cell_scene_object_update(s, (uint32_t)x, cell_y,
                                         has_special_neighbor != 0u);
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
                    if ((primary & 0x4000u) == 0u) {
                        if ((int)(int16_t)*(uint16_t*)(rec + 0x20) % 3 == 0) {
                            /* CellInitPass passes these raw words directly
                             * through the emitter's int* ABI. Preserve the
                             * exact stack representation used by the target. */
                            uint32_t direction_words[3];
                            CellTransientEffectPort* effect_port;
                            void* emitter;
                            direction_words[0] = 0x3dcccccdU;
                            direction_words[1] = 0x3dcccccdU;
                            direction_words[2] = 0xbf800000U;
                            effect_port =
                                cell_transient_effect_port_ensure(s, rec);
                            emitter = NULL;
                            if (effect_port != NULL) {
                                emitter = transient_effect_emitter_create(
                                    NULL, (uint32_t)x, cell_y,
                                    (int*)direction_words,
                                    0.6f, 0.4f, 0.0f, 1.0f,
                                    0x3e99999a, 0,
                                    (uint32_t)(int)(int16_t)
                                        *(uint16_t*)(rec + 0x20), 0x26);
                                cell_transient_effect_set(effect_port,
                                                          emitter);
                            }
                            primary |= 0x4000u;
                            *(uint32_t*)(rec + 0x08) = primary;
                        }
                    }
                    else {
                        transient_effect_enable_set(
                            cell_transient_effect_get(s, rec), 0);
                    }
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
                if ((primary & 0x4000u) != 0u) {
                    transient_effect_enable_set(
                        cell_transient_effect_get(s, rec), 1);
                }
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
        TerrainSurfaceGrid* surface = level_surface_grid_get(s);
        {
            float v00[3], v11[3], v10[3], v01[3];
            int have_vertices = 0;
            uint32_t height_component = 0u;
            if (surface != NULL &&
                terrain_cell_edge_vector_query(surface, (uint32_t)x,
                                               cell_y, v00) &&
                terrain_cell_edge_vector_query(surface,
                    (uint32_t)x + 1u, cell_y + 1u, v11) &&
                terrain_cell_edge_vector_query(surface,
                    (uint32_t)x + 1u, cell_y, v10) &&
                terrain_cell_edge_vector_query(surface, (uint32_t)x,
                    cell_y + 1u, v01)) {
                have_vertices = 1;
                height_component = 2u;
            }
            else {
                float vertices[4][3];
                if (scene_render_terrain_cell_vertices_get(
                        s, (uint32_t)x, cell_y, vertices)) {
                    memcpy(v00, vertices[0], sizeof(v00));
                    memcpy(v10, vertices[1], sizeof(v10));
                    memcpy(v11, vertices[2], sizeof(v11));
                    memcpy(v01, vertices[3], sizeof(v01));
                    have_vertices = 1;
                    height_component = 1u;
                }
            }
            if (have_vertices) {
                /* FUN_004507d0's third result is the raw surface height.
                 * SceneRender presents height in component one instead. */
                terrain_cell_orientation_set_level(s, x, (int)cell_y,
                    fabs((double)(v10[height_component] -
                                  v01[height_component])) <
                    fabs((double)(v00[height_component] -
                                  v11[height_component])));
            }
        }
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

    /* CellInitPass selects the ten-slot transition path when render flag
     * 0x8000 is enabled; both paths finish through TerrainCellMaterialSet
     * (FUN_0044f230), which updates the cell material and corner rotation. */
    if (s->ptr_terrain != NULL) {
        TerrainSurfaceGrid* surface = level_surface_grid_get(s);
        TerrainState* terrain = (TerrainState*)s->ptr_terrain;
        if ((render_feature_flags_get() & 0x8000u) != 0u) {
            terrain_cell_material_transition_begin(
                surface, (uint32_t)x, cell_y, (int)rec[1], (int)rec[3]);
        }
        else if (surface != NULL) {
            terrain_cell_material_set(surface, (uint32_t)x, cell_y,
                                      rec[1], rec[3]);
        }
        /* TerrainCreate's per-cell CPU record mirrors the renderer's +0x18
         * material byte even when no native surface owner is installed. */
        if (terrain->blocks != NULL && x >= 0 && cell_y <
            (uint32_t)terrain->height) {
            const size_t index = ((size_t)cell_y * (size_t)terrain->width +
                                  (size_t)x) * TERRAIN_BLOCK_STRIDE;
            terrain->blocks[index + 0x18u] = rec[1];
        }
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

    /* FUN_0042f6f0 ends with this gate after the material setter/transition:
     * LevelState +0x2c is the loaded `useroof` field. The progress routine
     * applies the level's dig-depth/roof-height displacement to the live
     * cell vertices, so keep it conditional on that retail field. */
    if (level_int(s, LEVEL_OFF_USEROOF) != 0) {
        terrain_cell_mutation_progress(s, (uint32_t)x, cell_y);
    }
}

/* Keep the retail per-cell entry available to the path-record registration
 * and teardown routines. The map-load sweep remains on the private helper
 * above so it can preserve the original float coordinate call shape. */
void level_cell_init_pass(LevelState* level, int x, int y)
{
    map_cell_record_grid_activate(level);
    cell_init_pass(level, (float)x, (uint32_t)y);
}

/* The level loader sweeps cells by calling the target-shaped per-cell entry.
 * Keep the sweep separate so it cannot be mistaken for the mapped function. */
static void cell_init_all(LevelState* s)
{
    int x, y;
    int width = level_int(s, LEVEL_OFF_WIDTH);
    int height = level_int(s, LEVEL_OFF_HEIGHT);
    for (y = 0; y < height; ++y)
        for (x = 0; x < width; ++x)
            cell_init_pass(s, (float)x, (uint32_t)y);
}

/* ------------------------------------------------------------------ *
 *  MapLoadPipeline @ 0x42b510.
 * ------------------------------------------------------------------ */

// FUNCTION: LEGORR 0x0042b510
static int map_pipeline(LevelState* s, const char** names, const int* biases,
                        const char* texture_set_name)
{
    /* `names` follows the ten config keys in LevelLoad. The corpus then
     * forwards them in a different positional order: Predug, Terrain, Path,
     * Erode, Fallin, Emerge, AI, BlockPointers, then the TerrainMap pass. */
    TerrainState* terrain = (TerrainState*)terrain_create_retail(
        scene_root_object_get(), names[0], level_float(s, LEVEL_OFF_BLOCKSIZE),
        level_float(s, LEVEL_OFF_ROUGHLEVEL));
    if (terrain == NULL) {
        return 0;
    }
    terrain->owner_level = s;
    level_store_ptr(s, LEVEL_OFF_TERRAIN, terrain);
    {
        uint32_t slot_index;
        /* TerrainCreate initializes all ten transition mesh handles to -1.
         * MapLoadPipeline's six IndexedFlagValueSetOne(0x70) calls all set
         * this same material suppression entry; their result is idempotent. */
        for (slot_index = 0u; slot_index < 10u; ++slot_index)
            s->material_transition_slots[slot_index].mesh_token = 0xffffffffu;
        s->material_transition_suppress_flags[0x70u] = 1u;
    }
    level_set_int(s, LEVEL_OFF_WIDTH, terrain->width);
    level_set_int(s, LEVEL_OFF_HEIGHT, terrain->height);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (level_int(s, LEVEL_OFF_USEROOF) != 0 &&
        !terrain_roof_owner_initialize(s)) return 0;
#endif
    /* TerrainCreate's geometry buffer is represented by TerrainState's CPU
     * quad records. Seed the native scene cache from those records before
     * CellInitPass can mutate orientations or heights. */
    if (terrain->cell_vertices != NULL) {
        int cell_x, cell_y;
        for (cell_y = 0; cell_y + 1 < terrain->height; ++cell_y) {
            for (cell_x = 0; cell_x + 1 < terrain->width; ++cell_x) {
                size_t index = ((size_t)cell_y *
                                (size_t)(terrain->width - 1) +
                                (size_t)cell_x) * 12u;
                float cached[12];
                uint8_t* cell_record = terrain->blocks +
                    ((size_t)cell_y * (size_t)terrain->width +
                     (size_t)cell_x) * TERRAIN_BLOCK_STRIDE;
                /* Retain geometry Y = -height byte * RoughLevel, exactly as
                 * the later CPU-quad mutation producer does. */
                memcpy(cached, terrain->cell_vertices + index,
                       sizeof(cached));
                scene_render_terrain_cell_vertices_update(
                    s, (uint32_t)cell_x, (uint32_t)cell_y, cached);
                if ((cell_record[0x19] & 1u) != 0u)
                    scene_render_terrain_cell_orientation_apply(
                        s, (uint32_t)cell_x, (uint32_t)cell_y, 1);
            }
        }
    }
    /* TerrainCellMutation's 0x46e220 step records the live cell's four
     * geometry-space heights in SceneRender's row-major CPU quad cache. */
    terrain_cell_overlay_height_fill_hook_set(
        scene_render_terrain_cell_overlay_height_fill, NULL);
    /* MapCellStateBufferRebuild @ 0x40ce80 is called as soon as the terrain
     * dimensions are known; own its per-cell links and pool on this level. */
    map_cell_state_buffer_rebuild(s);
    /* GridCellRecordBufferReplace @ 0x466230 follows the map-cell pool
     * rebuild in MapLoadPipeline and owns a distinct entity/timer/flag row
     * for every map coordinate. */
    level_field_grid_buffer_replace(s);

    /* TextureSetConfigLoad is a required pipeline gate: the reference does
     * not allocate cell records until config dimensions, texture resources,
     * tile meshes, and the roof texture have all been installed. */
    if (texture_set_name == NULL || texture_set_name[0] == '\0') return 0;
    {
        TerrainTileMeshGrid* tile_grid = (TerrainTileMeshGrid*)calloc(
            1, sizeof(TerrainTileMeshGrid));
        if (tile_grid == NULL) return 0;
        tile_grid->owner_level = s;
        /* LoadTileMeshes receives DAT_005560d8 as its scene manager. Terrain
         * creation captured that same root before the texture-set pass. */
        tile_grid->scene = terrain->scene_context;
        tile_grid->width = terrain->width;
        tile_grid->height = terrain->height;
        tile_grid->editor_mode =
            (engine_feature_word_peek() & 0x800000u) != 0u;
        tile_grid->cell_projection_scale =
            level_float(s, LEVEL_OFF_BLOCKSIZE);
        s->ptr_tile_mesh_grid = tile_grid;
        if (!terrain_texture_set_config_load_level(s, texture_set_name))
            return 0;
    }

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
    load_block_pointers(s, names[9]);
    ok = load_terrain_map(s, names[3], biases[3]);
    if (ok) {
        level_set_int(s, LEVEL_OFF_TERRAIN_LOADED, 1);
    }
    cell_init_all(s);
    {
        TerrainSurfaceGrid* surface = level_surface_grid_get(s);
        if (surface != NULL)
            terrain_grid_cells_initialize(surface);
    }
    terrain_cell_quad_run_stage_level(NULL, s);
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
 *  "Lego*::<levelname>::<key>", matching the corpus's BlockQueryDefault
 *  path-building "%s%s%s").
 * ------------------------------------------------------------------ */

/* Build the config path "<root>::<level>::<key>", rooted at the file's first
 * record name (`util_root_name`) — the block parser keeps the top-level
 * `Lego {…}` record as the root, and the corpus's LevelLoad paths are
 * relative to it (finding 12, FUN_004792e0). */

/* The shipped Lego.cfg nests every level group under the root's `Levels`
 * container (`Lego* { … Levels { … Level01 { … } } }`), so the flat
 * `root::level::key` path misses. Resolve through `Levels` first, then fall
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
    return value != NULL ? (float)atof(value) : dflt;
}

static int cfg_typed(BlockFile* cfg, const char* level, const char* key, int dflt)
{
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    /* FUN_004790d0 delegates to ConfigBooleanGet @ 0x477710: missing or
     * unrecognized values answer 2 instead of parsing a numeric string. */
    return block_get_bool_path(cfg, path);
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

static BlockRec* level_config_block_find(BlockFile* cfg, const char* level)
{
    char path[512];
    const char* root;
    BlockRec* record;
    if (cfg == NULL || level == NULL) return NULL;
    root = util_root_name(cfg);
    /* The level's container record (NULL when missing or empty). */
    snprintf(path, sizeof(path), "%s::Levels::%s", root, level);
    record = block_find(cfg, path);
    if (block_has_children(record)) return record;
    snprintf(path, sizeof(path), "%s::%s", root, level);
    record = block_find(cfg, path);
    return block_has_children(record) ? record : NULL;
}

/* LevelIdentifierRemapTableLoad @ 0x45d7e0 opens PTLFile, selects the
 * executable-root subgroup, and stores each child name/value as a pair of
 * message identifiers. Unknown names retain the final case-insensitive
 * comparison result returned by the retail lookup. */
static int level_identifier_remap_table_load(const char* path,
                                              const char* root_name)
{
    enum { LEVEL_REMAP_LOCAL_CAPACITY = 128 };
    uint32_t pairs[LEVEL_REMAP_LOCAL_CAPACITY * 2];
    BlockFile* file;
    BlockRec* group;
    BlockRec* entry;
    unsigned int count = 0u;
    unsigned int i;
    if (path == NULL || path[0] == '\0' || root_name == NULL) {
        level_identifier_remap_table_install(NULL, 0u);
        return 0;
    }
    file = block_open(path);
    if (file == NULL) {
        level_identifier_remap_table_install(NULL, 0u);
        return 0;
    }
    group = block_subgroup(file, root_name);
    if (group == NULL) {
        level_identifier_remap_table_install(NULL, 0u);
        block_close(file);
        return 1;
    }
    message_string_table_initialize();
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* name = block_name(entry);
        const char* value = block_value(entry);
        int resolved[2] = { -1, -1 };
        const char* tokens[2];
        unsigned int token;
        if (name == NULL || value == NULL) continue;
        tokens[0] = name;
        tokens[1] = value;
        for (token = 0u; token < 2u; ++token) {
            int comparison = -1;
            for (i = 0u; i < 65u; ++i) {
                const unsigned char* left = (const unsigned char*)
                    message_string_table_get(i);
                const unsigned char* right =
                    (const unsigned char*)tokens[token];
                if (left == NULL) {
                    comparison = 1;
                    continue;
                }
                /* MessageIdentifierFind compares the authored name with
                 * the table entry after its eight-byte Message_ prefix. */
                left += 8;
                do {
                    unsigned char a = *left++;
                    unsigned char b = *right++;
                    if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
                    if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
                    comparison = (int)a - (int)b;
                } while (comparison == 0 && left[-1] != 0u);
                if (comparison == 0) {
                    resolved[token] = (int)i;
                    break;
                }
            }
            if (resolved[token] < 0) resolved[token] = comparison;
        }
        if (count < LEVEL_REMAP_LOCAL_CAPACITY) {
            pairs[count * 2u] = (uint32_t)resolved[0];
            pairs[count * 2u + 1u] = (uint32_t)resolved[1];
            ++count;
        }
    }
    level_identifier_remap_table_install(pairs, count);
    block_close(file);
    return 1;
}

// FUNCTION: LEGORR 0x00429890
LevelState* level_load(BlockFile* cfg, const char* level_name)
{
    GameContext* game;
    LevelState* previous_level;
    if (cfg == NULL || level_name == NULL) {
        return NULL;
    }
    /* GameConfigInit loads SurfaceTypeDescriptions from the same Lego.cfg
     * root before a retail level is entered. The portable level-load path
     * owns the parsed config tree and is the earliest shared production
     * seam available for those process-wide description tables. */
    surface_type_descriptions_config_load(cfg, util_root_name(cfg));
    /* The object-type SFX lookup table is process-global in retail. The
     * config loader has no startup caller in this checkout, so seed it from
     * the same live Lego.cfg root used by the level's terrain descriptions. */
    terrain_object_sfx_grid_config_load(cfg, util_root_name(cfg));
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
    /* LevelLoad 0x42ad16 initializes the real air and comparison values. */
    level_set_float(s, LEVEL_OFF_AIR_SUPPLY, 100.0f);
    level_set_float(s, 0x204, 100.0f);
    /* DependencyGridConfigLoad builds its 20x15 rule table from the global
     * Dependencies records.  Bind those records to this LevelState before
     * the map's later dependency/upgrade queries can run. */
    if (game != NULL && game->level == s) {
        unported_ledger_stubs::dependency_grid_config_load(
            level_config_block_find(cfg, level_name),
            game->cfg != NULL ? game->cfg : cfg);
    }
    else {
        unported_ledger_stubs::dependency_grid_level_load(
            s, level_config_block_find(cfg, level_name), cfg);
    }

    float blocksize = cfg_float(cfg, level_name, K_BLOCKSIZE, 0.0f);
    level_set_float(s, LEVEL_OFF_BLOCKSIZE, blocksize);
    level_set_float(s, LEVEL_OFF_DIGDEPTH, cfg_float(cfg, level_name, K_DIGDEPTH, 0.0f));
    level_set_int(s, LEVEL_OFF_USEROOF, (int32_t)cfg_typed(cfg, level_name, K_USEROOF, 0));
    level_set_float(s, LEVEL_OFF_ROOFHEIGHT, cfg_float(cfg, level_name, K_ROOFHEIGHT, 0.0f));
    level_set_float(s, LEVEL_OFF_ROUGHLEVEL, cfg_float(cfg, level_name, K_ROUGHLEVEL, 0.0f));
    level_set_int(s, LEVEL_OFF_SAFECAVERNS, (int32_t)cfg_typed(cfg, level_name, K_SAFECAVERNS, 0));
    level_set_float(s, LEVEL_OFF_SELBOXHEIGHT, cfg_float(cfg, level_name, K_SELBOXH, 0.0f));

    /* LevelLoad maps Rock/default to 0, Lava to 1, and Ice to 2. */
    {
        char boulder[64] = { 0 };
        if (cfg_string(cfg, level_name, K_BOULDER_ANIM, boulder, sizeof(boulder)) != NULL) {
            int mode = 0;
            if (strcasecmp_lite(boulder, "Lava") == 0) mode = 1;
            else if (strcasecmp_lite(boulder, "Ice") == 0) mode = 2;
            level_set_int(s, LVL_OFF_BOULDER_ANIM, mode);
        }
    }

    /* LevelLoad stores rates and upgrade durations before opening map data.
     * Its twenty default upgrade slots are overwritten by the three
     * specialized values below when those keys are nonzero. */
    level_set_int(s, LEVEL_OFF_STARTP,
        cfg_typed(cfg, level_name, K_STARTFP, 0) == 1 ? 1 : 0);
    level_set_int(s, LEVEL_OFF_NODRAIN,
        cfg_typed(cfg, level_name, K_NODRAIN, 0) == 1 ? 1 : 0);
    level_set_float(s, LEVEL_OFF_OXYGEN_RATE,
                    cfg_float(cfg, level_name, K_OXYGENRATE, 0.0f) * LVL_OXYGEN_SCALE);
    {
        float base_ticks = cfg_float(cfg, level_name, K_UPGRADETIME,
                                     0.0f) * 25.0f;
        float vehicle_ticks;
        float minifigure_ticks;
        float building_ticks;
        int slot;
        for (slot = 0; slot < 20; ++slot)
            level_set_float(s, LVL_OFF_UPGRADE_TIMES + slot * 4,
                            base_ticks);

        vehicle_ticks = cfg_float(cfg, level_name,
                                   K_VEHICLE_UPGRADE_TIME, 0.0f);
        if (vehicle_ticks == 0.0f) vehicle_ticks = base_ticks;
        else vehicle_ticks *= 25.0f;
        level_set_float(s, LVL_OFF_VEHICLE_UPGRADE_TIME, vehicle_ticks);

        minifigure_ticks = cfg_float(cfg, level_name,
                                     K_MINIFIG_UPGRADE_TIME, 0.0f);
        if (minifigure_ticks == 0.0f) minifigure_ticks = base_ticks;
        else minifigure_ticks *= 25.0f;
        level_set_float(s, LVL_OFF_MINIFIG_UPGRADE_TIME,
                        minifigure_ticks);

        building_ticks = cfg_float(cfg, level_name,
                                   K_BUILDING_UPGRADE_TIME, 0.0f);
        if (building_ticks == 0.0f) building_ticks = base_ticks;
        else building_ticks *= 25.0f;
        level_set_float(s, LVL_OFF_BUILDING_UPGRADE_TIME, building_ticks);

        level_set_float(s, LVL_OFF_TRAIN_TIME,
            cfg_float(cfg, level_name, K_TRAIN_TIME, 0.0f) * 25.0f);
    }

    /* LevelLoad applies these configuration flags to DAT_00556ed0 after
     * the upgrade-time fields are initialized. Preserve unrelated bits in
     * the live GameContext word. NoMultiSelect is intentionally set-only in
     * the reference; the other four flags explicitly set or clear their bit. */
    if (game != NULL) {
        if (cfg_typed(cfg, level_name, K_NO_MULTI_SELECT, 0) == 1)
            game->cheat |= 0x00002000u;
        if (cfg_typed(cfg, level_name, K_NO_AUTO_EAT, 0) == 1)
            game->cheat |= 0x00200000u;
        else
            game->cheat &= ~0x00200000u;
        if (cfg_typed(cfg, level_name, K_RECALL_OL_OBJECTS, 0) == 1)
            game->cheat |= 0x00040000u;
        else
            game->cheat &= ~0x00040000u;
        if (cfg_typed(cfg, level_name, K_GENERATE_SPIDERS, 0) == 1)
            game->cheat |= 0x00080000u;
        else
            game->cheat &= ~0x00080000u;
        if (cfg_typed(cfg, level_name, K_DISABLE_TOOLTIP_SOUND, 0) == 1)
            game->cheat |= 0x00100000u;
        else
            game->cheat &= ~0x00100000u;
    }

    level_set_float(s, LEVEL_OFF_TOLERANCE,
                    cfg_float(cfg, level_name, K_BUILDTOL, 0.0f));
    level_set_float(s, LEVEL_OFF_MAXVARIATION,
                    cfg_float(cfg, level_name, K_BUILDMAXVAR, 0.0f));
    /* The target stores this inverted config boolean in its +0xa0 word
     * before the map pipeline begins. That word is reused for the live
     * resource balance later in the level lifetime. */
    level_set_int(s, LEVEL_OFF_DISABLE_START_TELEPORT,
        cfg_typed(cfg, level_name, K_DISABLE_START_TELEPORT, 0) == 1 ? 0 : 1);

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
    char texture_set[256] = { 0 };
    const int has_texture_set = cfg_string(cfg, level_name, K_TEXTURESET,
                                            texture_set,
                                            sizeof(texture_set)) != NULL;
    char ptl_file[256] = { 0 };
    const char* ptl_path = cfg_string(cfg, level_name, "PTLFile", ptl_file,
                                      sizeof(ptl_file));
    long fallin_mult = cfg_int(cfg, level_name, K_FALLIN_MULT, 1);
    if (fallin_mult == 0) {
        fallin_mult = 1;
    }
    /* LevelLoad stores the configured multiplier in DAT_00556f14 before the
     * map pipeline; LoadBlockPointers and the camera grid timer share it. */
    g_camera_scale_14 = (int)fallin_mult;
    {
        float emerge_timeout = cfg_float(cfg, level_name,
                                         K_EMERGE_TIMEOUT, 0.0f);
        if (emerge_timeout == 0.0f) emerge_timeout = 1500.0f;
        level_set_float(s, LEVEL_OFF_EMERGE_TIMEOUT, emerge_timeout);
    }
    {
        char emerge_creature[256] = { 0 };
        int type = 0;
        int row = 0;
        (void)cfg_string(cfg, level_name, K_EMERGE_CREATURE,
                         emerge_creature, sizeof(emerge_creature));
        (void)entity_type_row_for_name(emerge_creature, &type, &row);
        /* LevelLoad stores the resolver's row output at +0xb0; the temporary
         * type output is used only to validate/resolve the configured name. */
        level_set_int(s, LEVEL_OFF_ENGINE_B0, row);
    }

    /* Retail LevelLoad initializes objectives, NERP state, and the advisor
     * token table before invoking MapLoadPipeline. Keep those owners ready
     * for map/cell initialization callbacks that can consult level state. */
    objective_load(cfg, level_name, s);
    {
        char nerp_file[256] = { 0 };
        char nerp_message_file[256] = { 0 };
        const char* script_path = cfg_string(
            cfg, level_name, "NERPFile", nerp_file, sizeof(nerp_file));
        const char* message_path = cfg_string(
            cfg, level_name, "NERPMessageFile", nerp_message_file,
            sizeof(nerp_message_file));
        (void)tutorial_script_file_load(script_path);
        (void)tutorial_objective_token_load(message_path);
    }

    /* These level-owned display/navigation strings are installed before
     * the map pipeline in LevelLoad and are released by level_destroy if a
     * later mandatory map stage fails. */
    {
        char next[256] = { 0 };
        if (cfg_string(cfg, level_name, K_NEXTLEVEL, next,
                       sizeof(next)) != NULL) {
#if defined(_MSC_VER)
            char* dup = _strdup(next);
#else
            char* dup = (char*)malloc(strlen(next) + 1u);
            if (dup != NULL) strcpy(dup, next);
#endif
            if (dup != NULL) level_store_ptr(s, LEVEL_OFF_NEXTLEVEL, dup);
        }
    }
    {
        char full_name[256] = { 0 };
        if (cfg_string(cfg, level_name, K_FULLNAME, full_name,
                       sizeof(full_name)) != NULL) {
            size_t i;
            char* dup;
#if defined(_MSC_VER)
            dup = _strdup(full_name);
#else
            dup = (char*)malloc(strlen(full_name) + 1u);
            if (dup != NULL) strcpy(dup, full_name);
#endif
            if (dup != NULL) {
                for (i = 0; dup[i] != '\0'; ++i) {
                    if (dup[i] == '_') dup[i] = ' ';
                }
                level_store_ptr(s, LEVEL_OFF_FULLNAME, dup);
            }
        }
    }

    {
        int load_ok = level_identifier_remap_table_load(
            ptl_path, util_root_name(cfg));
        if (load_ok) {
            /* TextureSetConfigLoad uses this temporary config context while
             * MapLoadPipeline resolves the named texture set. Keep the
             * borrowed BlockFile pointer out of the global slot during all
             * earlier failure paths and clear it as soon as that load ends. */
            terrain_texture_set_config_context_bind(cfg);
            load_ok = map_pipeline(s, names, biases,
                                   has_texture_set ? texture_set : NULL);
            terrain_texture_set_config_context_bind(NULL);
        }
        if (!load_ok) {
            level_destroy(s);
            if (game != NULL) game->level = previous_level;
            return NULL;
        }
    }
    camera_predug_spawn_state_seed(s);

    /* LevelLoad calls ObjectiveVoiceAliasPrepare(0) then ObjectiveTick with
     * checker/frame deltas of 1 before parsing the object list. The level
     * state machine and alias bookkeeping are represented; DAT_00556140's
     * renderer owner is not, so use its explicit-level adapter with NULL. */
    objective_announce_current(0);
    objective_tick_target(NULL, s, 1.0f, 1.0f);

    /* LevelLoad calls OListEntityRecordParse after MapLoadPipeline and its
     * viewport/no-gather setup, before initial resource counters and the
     * level intro. At this point the active GameContext, Lego.cfg tree, and
     * loaded terrain are all available to the target parser. */
    if (game == NULL || object_list_load(game, cfg, level_name) <= 0) {
        level_destroy(s);
        if (game != NULL) game->level = previous_level;
        return NULL;
    }

    /* Initial-collection counters read by the entity layer (post-load). */
    level_set_int(s, LVL_OFF_INIT_CRYS, (int32_t)cfg_int(cfg, level_name, K_INITCRYSTALS, 0));
    level_set_int(s, LVL_OFF_INIT_ORE, (int32_t)cfg_int(cfg, level_name, K_INITORE, 0));

    /* LevelLoad (0x429890) activates the coordinate lists after OList and
     * initial counters, before the first ordinary tutorial dispatch. */
    tutorial_path_coordinate_index(s);

    return s;
}

/* Retail applies NoGather after the level intro. Keep the config decision
 * shared by the frameloop and next-level caller, which own that post-intro
 * point in the portable runtime. */
void level_no_gather_config_setup(BlockFile* cfg, const char* level_name)
{
    int mode = cfg_typed(cfg, level_name, "NoGather", 0) == 1 ? 1 : 0;
    level_no_gather_object_setup(mode);
}

void level_destroy(LevelState* s)
{
    GameContext* game;
    int was_current;
    uint32_t transition_index;
    if (s == NULL) {
        return;
    }
    deferred_olist_entity_level_end(s);
    game = entity_pool_callback_context_get();
    was_current = game != NULL && game->level == s;
    objective_active_level_clear(s);
    if (was_current) {
        /* PendingLevelName calls AdvisorDisplayStateResourcesRelease before
         * freeing the active level. This owns tutorial script and advisor
         * token resources, whose pointers are process-wide in the port. */
        (void)tutorial_display_resources_release();
        game->level = NULL;
    }
    unported_ledger_stubs::dependency_grid_level_release(s);
    terrain_cell_quad_run_stage_level_release(s);
    cell_transient_effect_ports_release(s);
    map_cell_record_grid_release(s);
    level_field_grid_buffer_release(s);
    /* PendingLevelName releases the level's TerrainActivityBuildRecords
     * before freeing the level. These records are kept in a process-wide
     * sidecar, so clear the entries tagged with this owner here as well. */
    terrain_activity_build_records_release_all(s);
    for (transition_index = 0u; transition_index < 10u;
         ++transition_index) {
        LevelTerrainMaterialTransitionSlot* slot =
            &s->material_transition_slots[transition_index];
        if ((slot->flags & 1u) != 0u &&
            s_material_transition_mesh_hook != NULL) {
            (void)s_material_transition_mesh_hook(
                s_material_transition_mesh_context, s, transition_index, 2,
                slot->cell_x, slot->cell_y, NULL, 0.0f);
        }
        slot->flags &= ~1u;
    }
    scene_render_terrain_level_release(s);
    camera_spawn_level_block_bind(NULL);
    lava_erosion_level_context_release(s);
    /* Level teardown's corpus path teardown includes ReleaseAllGroupRecords
     * (0x4099d0), which owns the global descriptor and payload chain. */
    path_record_release_all();
    path_record_cell_links_release(s, NULL);
    path_record_surface_grid_bind(NULL);
    /* PendingLevelName releases the 55 coordinate chains before the level.
     * These heads are typed sidecars, not the raw retail +0x114 window. */
    for (int coordinate_list = 0; coordinate_list < 0x37;
         ++coordinate_list) {
        void* node = s->pool_heads[coordinate_list];
        while (node != NULL) {
            void* next = *(void**)((char*)node + 8);
            engine_heap_free(node);
            node = next;
        }
        s->pool_heads[coordinate_list] = NULL;
    }
    if (s->ptr_tile_mesh_grid != NULL) {
        if (s->ptr_tile_mesh_grid->surface_grid != NULL &&
            s->ptr_tile_mesh_grid->surface_grid->owner_level == s) {
            s->ptr_tile_mesh_grid->surface_grid->owner_level = NULL;
            terrain_surface_grid_terrain_window_set(
                s->ptr_tile_mesh_grid->surface_grid, NULL);
        }
        terrain_texture_set_release(s->ptr_tile_mesh_grid);
        free(s->ptr_tile_mesh_grid);
        s->ptr_tile_mesh_grid = NULL;
    }
    free(s->ptr_voxels);
    free(s->ptr_layerl);
    free(s->ptr_full_name);
    terrain_destroy(s->ptr_terrain);
    free(s->ptr_next);
    /* ptr_name is borrowed from the caller (the level *name* passed to
     * level_load); never freed here. */
    __builtin_memset(s, 0, sizeof(*s));
    free(s);
    /* PendingLevelName resets the shared interface state after releasing
     * the active level. Discarding an uninstalled load must not reset it. */
    if (was_current) lego::level_state_counters_clear();
}

int level_width(const LevelState* s) { return s ? level_int(s, LEVEL_OFF_WIDTH) : 0; }
int level_height(const LevelState* s) { return s ? level_int(s, LEVEL_OFF_HEIGHT) : 0; }

/* TerrainDescriptorFindExact (FUN_004090c0): when a pair key is supplied,
 * both words must match; otherwise the optional single key is compared
 * against the node's single-key field. The global list head is threaded in
 * as `list`. */
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
int terrain_cell_predicate_check(TerrainDescriptorNode* list,
                                 const uint32_t* pair_key)
{
    return terrain_descriptor_find_exact(list, pair_key, NULL) != NULL;
}

/* Retail's descriptor chain consists of the raw path-record windows. */
// FUNCTION: LEGORR 0x004090c0
void* terrain_descriptor_find_exact_current(const uint32_t* pair_key,
                                            const uint32_t* single_key)
{
    uint8_t* record = (uint8_t*)path_record_last_peek();
    while (record != NULL) {
        uint32_t first, second;
        void* previous;
        if (pair_key != NULL) {
            memcpy(&first, record + 0x0c, sizeof first);
            memcpy(&second, record + 0x10, sizeof second);
            if (first == pair_key[0] && second == pair_key[1]) return record;
        }
        else if (single_key != NULL) {
            memcpy(&first, record + 0x08, sizeof first);
            if (first == *single_key) return record;
        }
        memcpy(&previous, record + 0x24, sizeof previous);
        record = (uint8_t*)previous;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004090a0
int terrain_cell_predicate_check_current(const uint32_t pair_key[2])
{
    return terrain_descriptor_find_exact_current(pair_key, NULL) != NULL;
}

/* CellResourceEntryMatches (FUN_0040af40): entries backed by a matching
 * live task are accepted directly. Otherwise the first two words must
 * match, and a nonzero bounded-mode flag applies the unsigned cell limit
 * from entry word two to the cell record's word index 0xcc (+0x330 bytes). */
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
        && cell_record[0xcc] < resource_entry[2]) {
        return 0;
    }
    return 1;
}

const char* level_next_name(const LevelState* s)
{
    return s != NULL ? (const char*)s->ptr_next : NULL;
}

const char* level_full_name(const LevelState* s)
{
    return s != NULL ? (const char*)s->ptr_full_name : NULL;
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
 * entity's scene position, then passes its world x/y pair through the
 * terrain grid picker. EntitySlot::x/y are the portable position values. */
// FUNCTION: LEGORR 0x004424F0
void entity_position_to_terrain_cell(const struct EntitySlot* entity,
                                     const LevelState* s,
                                     int* cell_x, int* cell_y)
{
    if (entity == NULL || s == NULL || cell_x == NULL || cell_y == NULL) {
        return;
    }
    if (!terrain_world_xy_to_cell(s->ptr_terrain, entity->x, entity->y,
                                  cell_x, cell_y)) return;
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

/* The mapped retail predicate carries an entity and two mode arguments and
 * reads the active level.  Keep level_walkable above as the simpler grid
 * convenience used by path-search code; this is the full address-level
 * behavior from FUN_00432770. */
// FUNCTION: LEGORR 0x00432770
int terrain_cell_walkability_query_current(const struct EntitySlot* e,
                                            uint32_t cx, uint32_t cy,
                                            int require_stat_bit,
                                            int gated_mode)
{
    LevelState* s = level_current_owner_get();
    const uint8_t* rec;
    uint32_t stats;
    int width, height;
    if (s == NULL || e == NULL) return 0;
    width = level_width(s);
    height = level_height(s);
    rec = level_voxel(s, (int)cx, (int)cy);
    if (rec == NULL) return 0;
    stats = entity_stats_word_get(e);
    if ((stats & 0x20000u) != 0u && e->owner_36c == NULL) return 0;
    if (require_stat_bit != 0 && (entity_stats_word_get(e) & 1u) == 0u)
        return 0;
    if (gated_mode == 0 || (e->flags_3e0 & 0x400u) == 0u) {
        if ((e->flags_3e8 & 2u) != 0u &&
            entity_stats_float_at24_indexed(e, (int)(signed char)rec[2]) != 0.0f &&
            cx < (uint32_t)width - 2u && cy < (uint32_t)height - 2u) {
            uint32_t flags;
            const int type = (int)(signed char)rec[2];
            __builtin_memcpy(&flags, rec + 8, sizeof flags);
            if (require_stat_bit == 0) {
                return (flags & 0x10u) != 0u && (flags & 0x40u) == 0u &&
                       type != 1 && type != 7 && type != 11;
            }
            if (rec[0] == 0) {
                static const int offsets[16] = {
                    0,-1, 1,0, 0,1, -1,0,
                    -1,-1, 0,-1, 0,0, -1,0
                };
                unsigned i;
                int any_cell = 0;
                for (i = 0; i < 4; ++i) {
                    const uint8_t* near_rec = level_voxel(
                        s, (int)cx + offsets[i * 2],
                        (int)cy + offsets[i * 2 + 1]);
                    if (near_rec != NULL && near_rec[0] != 0) {
                        any_cell = 1;
                        break;
                    }
                }
                if (any_cell) {
                    for (i = 0; i < 4; ++i) {
                        const uint8_t* near_rec = level_voxel(
                            s, (int)cx + offsets[8 + i * 2],
                            (int)cy + offsets[9 + i * 2]);
                        if (near_rec != NULL) {
                            const int near_type = (int)(signed char)near_rec[2];
                            if (near_type != 1 && near_type != 7) return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
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
    if (s == NULL) s = level_current_owner_get();
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

int entity_move_destination_cell_usable_current(const struct EntitySlot* e,
                                                uint32_t cx, uint32_t cy)
{
    return entity_move_destination_cell_usable(e, level_current_owner_get(),
                                               (int)cx, (int)cy);
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
    if (s == NULL) s = level_current_owner_get();
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

int terrain_cell_allows_placement_current(const struct EntitySlot* e,
                                          uint32_t cx, uint32_t cy)
{
    return terrain_cell_allows_placement(e, level_current_owner_get(),
                                         (int)cx, (int)cy);
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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void level_tile_mark_current_target(const int cell_xy[2],
                                                      int set)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        push esi
        test eax, eax
        mov eax, dword ptr [ecx + 0x14]
        je level_tile_mark_current_clear
        imul eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        mov ecx, dword ptr [ecx + 0xb8]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 8]
        or dword ptr [eax], 0x08000000
        pop esi
        ret
    level_tile_mark_current_clear:
        imul eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        add eax, esi
        pop esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0xb8]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 0xf7ffffff
        mov dword ptr [eax], ecx
        ret
        nop
        nop
        nop
        nop
    }
}
#else
void level_tile_mark_current_target(const int cell_xy[2], int set)
{
    LevelState* level = g_active_level_retail;
    if (set != 0) {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cell_xy[1] + cell_xy[0];
        uint32_t* flags = (uint32_t*)(voxels + 8 + cell_index * 0x48);
        *flags |= 0x8000000u;
        return;
    }
    {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cell_xy[1] + cell_xy[0];
        uint32_t* flags = (uint32_t*)(voxels + 8 + cell_index * 0x48);
        *flags &= 0xf7ffffffu;
    }
}
#endif

void level_tile_mark_current(const int cell_xy[2], int set)
{
    level_tile_mark_current_target(cell_xy, set);
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

/* TerrainCellMaterialGet (FUN_004319b0) reads byte +2 from the active
 * level's 0x48-byte row-major cell record. Like retail it has no coordinate
 * or active-level validation; callers must have an active level and in-range
 * cell coordinates. */
// FUNCTION: LEGORR 0x004319b0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint8_t terrain_cell_material_get_current(int x, int y)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        mov eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        imul eax, dword ptr [esp + 8]
        add eax, edx
        xor edx, edx
        lea eax, dword ptr [eax + eax*8]
        mov dl, byte ptr [ecx + eax*8 + 2]
        mov eax, edx
        ret
    }
}
#else
uint8_t terrain_cell_material_get_current(int x, int y)
{
    LevelState* level = g_active_level_retail;
    const uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    const uint8_t* voxels = (const uint8_t*)level->ptr_voxels;
    const uint32_t index = (uint32_t)y * width + (uint32_t)x;
    const uint8_t* record = voxels + index * LEVEL_VOXEL_STRIDE;
    return record[2];
}
#endif

/* --- terrain-cell flag primitives ----------------------------------------
 * Core readers over the voxel record. The corpus family computes the record
 * address with no bounds check at all. Bounded variants below preserve
 * their own dim-minus-one checks; unchecked target leaves use the raw
 * row-major helper where they do not have that gate. */

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

/* The target's coordinate-pair leaf readers do not apply grid bounds. Keep
 * their 32-bit row-major address calculation distinct from level_voxel(),
 * whose checked access is useful to higher-level portable code. */
static uint8_t* cell_record_unchecked(LevelState* s, int cx, int cy)
{
    uint8_t* voxels;
    uint32_t index;
    uint32_t byte_offset;
    if (s == NULL) return NULL;
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    if (voxels == NULL) return NULL;
    index = (uint32_t)cy * (uint32_t)level_int(s, LEVEL_OFF_WIDTH) +
            (uint32_t)cx;
    byte_offset = index * (uint32_t)LEVEL_VOXEL_STRIDE;
    return voxels + byte_offset;
}

/* 0x434570: the +0x14 float belongs to the level voxel record. */
int terrain_cell_drill_work_add(LevelState* level, int x, int y,
                                float divisor, float elapsed)
{
    uint8_t* record;
    float work;
    if (divisor == 0.0f || divisor != divisor || level == NULL) return 0;
    record = cell_record_unchecked(level, x, y);
    if (record == NULL) return 0;
    memcpy(&work, record + 0x14, sizeof(work));
    work += elapsed / divisor;
    memcpy(record + 0x14, &work, sizeof(work));
    return work > 1.0f ? 1 : 0;
}

// FUNCTION: LEGORR 0x00434570
int terrain_cell_drill_work_add_current(int x, int y, float divisor,
                                        float elapsed)
{
    if (divisor == 0.0f || divisor != divisor) return 0;
    return terrain_cell_drill_work_add(level_current_owner_get(), x, y,
                                       divisor, elapsed);
}

static const uint8_t* cell_record_unchecked_const(const LevelState* s,
                                                   int cx, int cy)
{
    return cell_record_unchecked((LevelState*)s, cx, cy);
}

#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x00431560
__declspec(naked) LevelState* level_current_owner_get(void)
{
    __asm {
        mov eax, dword ptr [g_active_level_retail]
        ret
    }
}
#else
LevelState* level_current_owner_get(void)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    return level;
}
#endif

// FUNCTION: LEGORR 0x00431570
void* level_current_terrain_get(int cell_x, int cell_y)
{
    (void)cell_x;
    (void)cell_y;
#if !defined(_MSC_VER) || !defined(_M_IX86)
    LevelState* level = level_current_owner_get();
    return level != NULL ? level->ptr_terrain : NULL;
#else
    return *(void**)((char*)level_current_owner_get() + 4);
#endif
}

/* Retail erosion entry points read DAT_005560d4 on every call. Keep the
 * level-load sidecar as a fallback for explicit offline work, while letting
 * address-mapped current-level paths follow the live game/map owner. */
static LevelState* lava_erosion_current_level_get(void)
{
    LevelState* level = level_current_owner_get();
    return level != NULL ? level : s_lava_erosion_level_context;
}

static uint32_t cell_flag_word_unchecked(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = cell_record_unchecked_const(s, cx, cy);
    uint32_t flags = 0u;
    if (rec != NULL) __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    return flags;
}

static uint32_t cell_secondary_word_unchecked(const LevelState* s,
                                              int cx, int cy)
{
    const uint8_t* rec = cell_record_unchecked_const(s, cx, cy);
    uint32_t flags = 0u;
    if (rec != NULL) __builtin_memcpy(&flags, rec + 0x0c, sizeof(flags));
    return flags;
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

// FUNCTION: LEGORR 0x00432bd0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int terrain_cell_walkable_flag_current(uint32_t cx,
                                                          uint32_t cy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        push edi
        mov edx, dword ptr [ecx + 14h]
        lea eax, dword ptr [edx - 1]
        cmp esi, eax
        jae terrain_cell_walkable_flag_outside
        mov edi, dword ptr [ecx + 18h]
        mov eax, dword ptr [esp + 10h]
        dec edi
        cmp eax, edi
        jae terrain_cell_walkable_flag_outside
        imul eax, edx
        add eax, esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov eax, dword ptr [eax + edx*8 + 8]
        test al, 10h
        je terrain_cell_walkable_flag_outside
        test al, 8
        jne terrain_cell_walkable_flag_outside
        mov eax, 1
        pop edi
        pop esi
        ret
    terrain_cell_walkable_flag_outside:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
int terrain_cell_walkable_flag_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
    if (cx >= width - 1u) {
        return 0;
    }
    uint32_t height = level->word[LEVEL_OFF_HEIGHT / 4];
    if (cy >= height - 1u) {
        return 0;
    }
    uint32_t index = cy * width + cx;
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    uint32_t flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
    if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
        return 1;
    }
    return 0;
}
#endif

/* FUN_00432c20: wave-spawn flag bit (finding 10's wave flag). */
uint32_t level_cell_flag_wave(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x40000u;
}

// FUNCTION: LEGORR 0x00432c20
uint32_t terrain_cell_flag_wave_current(int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cy + cx;
    uint32_t flags = *(uint32_t*)(voxels + 8 + cell_index * 0x48);
    return flags & 0x40000u;
}

/* FUN_00432c50: speed class 0 under the terrain block bit, else the low
 * terrain-type bits + 1. */
int level_cell_speed_class(const LevelState* s, int cx, int cy)
{
    /* FUN_00432c50 addresses the voxel row directly. In particular, it does
     * not use the bounded LevelCellFlag helpers: callers establish their own
     * cell gate before asking for the speed class. */
    const uint32_t flags = cell_flag_word_unchecked(s, cx, cy);
    if ((flags & 0x200000u) != 0) {
        return 0;
    }
    return (int)(flags & 3u) + 1;
}

// FUNCTION: LEGORR 0x00432c50
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int terrain_cell_speed_class_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        test eax, 200000h
        je terrain_cell_speed_class_available
        xor eax, eax
        pop esi
        ret
    terrain_cell_speed_class_available:
        and eax, 3
        pop esi
        inc eax
        ret
    }
}
#else
int terrain_cell_speed_class_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cell_xy[1] + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 8 + cell_index * 0x48);
    if ((flags & 0x200000u) != 0u) {
        return 0;
    }
    return (int)(flags & 3u) + 1;
}
#endif

/* Explicit-level adapter; unlike the mapped target, return the masked byte
 * so callers that need the individual flag bits can inspect them. */
uint32_t level_cell_mask_c0(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (uint32_t)rec[8] & 0xc0u : 0u;
}

/* Retail target reads the active level and normalizes either flag to 1. */
// FUNCTION: LEGORR 0x00432e60
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_mask_c0_current(int cx, int cy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        mov eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        imul eax, dword ptr [esp + 8]
        add eax, edx
        lea eax, dword ptr [eax + eax*8]
        test byte ptr [ecx + eax*8 + 8], 0c0h
        jne terrain_cell_mask_c0_set
        xor eax, eax
        ret
    terrain_cell_mask_c0_set:
        mov eax, 1
        ret
    }
}
#else
uint32_t terrain_cell_mask_c0_current(int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cy + cx;
    uint8_t flags = voxels[8 + cell_index * 0x48];
    return (flags & 0xc0u) != 0u ? 1u : 0u;
}
#endif

/* FUN_00432e90: predug edge-wall bit (finding 07, load-time 3×3 OR). */
uint32_t level_cell_flag_edge_wall(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x4000000u;
}

/* Resolve the portable owner for targets which read DAT_005560d4. Their
 * row-major record calculations remain in each mapped target body. */
static LevelState* terrain_current_level_resolve(void)
{
    LevelState* level = (LevelState*)map_cell_record_grid_level_get();
    return level != NULL ? level : level_current_owner_get();
}

// FUNCTION: LEGORR 0x00432e90
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_edge_wall_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        and eax, 4000000h
        ret
    }
}
#else
uint32_t terrain_cell_edge_wall_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 8 + cell_index * 0x48);
    return flags & 0x4000000u;
}
#endif

/* FUN_00432ec0 / FUN_00432f00: state byte equals 1 / 6. */
int level_cell_state_is_1(const LevelState* s, int cx, int cy)
{
    return level_tile_type(s, cx, cy) == 1;
}

int level_cell_state_is_6(const LevelState* s, int cx, int cy)
{
    return level_tile_type(s, cx, cy) == 6;
}

/* Retail predicates use the current level and one coordinate-pair argument. */
// FUNCTION: LEGORR 0x00432ec0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int terrain_cell_state_equals_one_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push ebx
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        xor edx, edx
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov bl, byte ptr [ecx + eax*8 + 2]
        cmp bl, 1
        pop ebx
        sete dl
        mov eax, edx
        ret
    }
}
#else
int terrain_cell_state_equals_one_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = cell_xy[1] * width + cell_xy[0];
    return voxels[2 + cell_index * 0x48] == 1;
}
#endif

// FUNCTION: LEGORR 0x00432f00
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int terrain_cell_state_equals_six_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push ebx
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        xor edx, edx
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov bl, byte ptr [ecx + eax*8 + 2]
        cmp bl, 6
        pop ebx
        sete dl
        mov eax, edx
        ret
    }
}
#else
int terrain_cell_state_equals_six_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = cell_xy[1] * width + cell_xy[0];
    return voxels[2 + cell_index * 0x48] == 6;
}
#endif

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

// FUNCTION: LEGORR 0x00432f40
#if defined(_WIN32) && defined(_M_IX86)
__declspec(naked) int terrain_cell_mask18_clear_current(uint32_t cx,
                                                         uint32_t cy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        push edi
        mov edx, dword ptr [ecx + 14h]
        lea eax, dword ptr [edx - 1]
        cmp esi, eax
        jae terrain_cell_mask18_clear_outside
        mov edi, dword ptr [ecx + 18h]
        mov eax, dword ptr [esp + 10h]
        dec edi
        cmp eax, edi
        jae terrain_cell_mask18_clear_outside
        imul eax, edx
        add eax, esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov eax, dword ptr [eax + edx*8 + 8]
        and al, 18h
        neg al
        sbb eax, eax
        inc eax
        pop edi
        pop esi
        ret
    terrain_cell_mask18_clear_outside:
        pop edi
        mov eax, 1
        pop esi
        ret
    }
}
#else
int terrain_cell_mask18_clear_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = terrain_current_level_resolve();
    uint32_t width;
    uint32_t height;
    const uint8_t* voxels;
    uint32_t index;
    uint32_t flags;
    if (level == NULL) return 1;
    width = level->word[LEVEL_OFF_WIDTH / 4u];
    height = level->word[LEVEL_OFF_HEIGHT / 4u];
    if (cx >= width - 1u || cy >= height - 1u) return 1;
    voxels = (const uint8_t*)level->ptr_voxels;
    if (voxels == NULL) return 1;
    index = cy * width + cx;
    __builtin_memcpy(&flags, voxels + 8u + index * 0x48u, sizeof(flags));
    return (flags & 0x18u) == 0u;
}
#endif

/* Explicit-level adapter for code which already owns a level instance. */
uint32_t level_cell_flag_4(const LevelState* s, int cx, int cy)
{
    const int w = level_int(s, LEVEL_OFF_WIDTH);
    const int h = level_int(s, LEVEL_OFF_HEIGHT);
    if ((unsigned)cx < ((unsigned)w - 1u) &&
        (unsigned)cy < ((unsigned)h - 1u)) {
        const uint8_t* voxels =
            (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
        const uint32_t index = (uint32_t)cy * (uint32_t)w + (uint32_t)cx;
        const uint32_t flags =
            *(const uint32_t*)(voxels + index * LEVEL_VOXEL_STRIDE + 8u);
        return flags & 4u;
    }
    return 0u;
}

/* TerrainCellFlag4TestInBounds (FUN_00432f90) reads the active level. */
// FUNCTION: LEGORR 0x00432f90
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t level_cell_flag_4_current(uint32_t cx,
                                                      uint32_t cy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        push edi
        mov edx, dword ptr [ecx + 14h]
        lea eax, dword ptr [edx - 1]
        cmp esi, eax
        jae terrain_cell_flag_4_outside
        mov edi, dword ptr [ecx + 18h]
        mov eax, dword ptr [esp + 10h]
        dec edi
        cmp eax, edi
        jae terrain_cell_flag_4_outside
        imul eax, edx
        add eax, esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov eax, dword ptr [eax + edx*8 + 8]
        and eax, 4
        pop edi
        pop esi
        ret
    terrain_cell_flag_4_outside:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
uint32_t level_cell_flag_4_current(uint32_t cx, uint32_t cy)
{
    const LevelState* s;
    int w;
    int h;
    const uint8_t* voxels;
    uint32_t index;
    uint32_t flags;
    /* The target reads DAT_005560d4 directly. The map-cell registry is the
     * same retained current-level owner when no GameContext is installed. */
    s = terrain_current_level_resolve();
    if (s == NULL) return 0u;
    w = level_int(s, LEVEL_OFF_WIDTH);
    h = level_int(s, LEVEL_OFF_HEIGHT);
    if (cx >= ((uint32_t)w - 1u) ||
        cy >= ((uint32_t)h - 1u)) {
        return 0u;
    }
    voxels = (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    if (voxels == NULL) return 0u;
    index = (uint32_t)cy * (uint32_t)w + (uint32_t)cx;
    flags = *(const uint32_t*)(voxels + index * LEVEL_VOXEL_STRIDE + 8u);
    return flags & 4u;
}
#endif

/* Explicit-level adapters for the two scalar mask leaves. */
uint32_t level_cell_flag_2000(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x2000u;
}

uint32_t level_cell_flag_80(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x80u;
}

// FUNCTION: LEGORR 0x00432fd0
uint32_t terrain_cell_flag_2000_current(int cx, int cy)
{
#if defined(_WIN32) && defined(_M_IX86)
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
    int index = width * cy + cx;
    uint32_t flags = *(uint32_t*)(voxels + 8 + index * 0x48);
    return flags & 0x2000u;
#else
    LevelState* level = terrain_current_level_resolve();
    const uint8_t* voxels;
    uint32_t index;
    uint32_t flags;
    if (level == NULL) return 0u;
    voxels = (const uint8_t*)level_load_ptr(level, LEVEL_OFF_VOXELS);
    if (voxels == NULL) return 0u;
    index = (uint32_t)cy * (uint32_t)level_int(level, LEVEL_OFF_WIDTH) +
            (uint32_t)cx;
    __builtin_memcpy(&flags,
        voxels + index * (uint32_t)LEVEL_VOXEL_STRIDE + 8u,
        sizeof(flags));
    return flags & 0x2000u;
#endif
}

// FUNCTION: LEGORR 0x00433000
uint32_t terrain_cell_flag_80_current(int cx, int cy)
{
#if defined(_WIN32) && defined(_M_IX86)
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
    int index = width * cy + cx;
    uint32_t flags = *(uint32_t*)(voxels + 8 + index * 0x48);
    return flags & 0x80u;
#else
    LevelState* level = terrain_current_level_resolve();
    const uint8_t* voxels;
    uint32_t index;
    uint32_t flags;
    if (level == NULL) return 0u;
    voxels = (const uint8_t*)level_load_ptr(level, LEVEL_OFF_VOXELS);
    if (voxels == NULL) return 0u;
    index = (uint32_t)cy * (uint32_t)level_int(level, LEVEL_OFF_WIDTH) +
            (uint32_t)cx;
    __builtin_memcpy(&flags,
        voxels + index * (uint32_t)LEVEL_VOXEL_STRIDE + 8u,
        sizeof(flags));
    return flags & 0x80u;
#endif
}

uint32_t level_cell_flag_8000(const LevelState* s, int cx, int cy)
{
    return cell_flag_word(s, cx, cy) & 0x8000u;
}

/* Retail pair-pointer ABI; this target is an unchecked masked-word read. */
// FUNCTION: LEGORR 0x00433030
#if defined(_WIN32) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_flag_8000_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        and eax, 8000h
        ret
    }
}
#else
uint32_t terrain_cell_flag_8000_current(const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    if (cell_xy == NULL || level == NULL) return 0u;
    return cell_flag_word_unchecked(level, cell_xy[0], cell_xy[1]) & 0x8000u;
}
#endif

/* FUN_00433060: secondary flag word (record+12) bit 0. */
uint32_t level_cell_secondary_bit0(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 1u;
}

// FUNCTION: LEGORR 0x00433060
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_secondary_bit0_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 0ch]
        and eax, 1
        ret
    }
}
#else
uint32_t terrain_cell_secondary_bit0_current(const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    if (cell_xy == NULL || level == NULL) return 0u;
    return cell_secondary_word_unchecked(level, cell_xy[0], cell_xy[1]) & 1u;
}
#endif

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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t level_cell_occupied_at_current(
    const int cell_xy[2])
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [g_active_level_retail]
        push esi
        mov ecx, dword ptr [eax]
        test ecx, ecx
        jl level_cell_occupied_at_failed
        mov esi, dword ptr [eax + 4]
        test esi, esi
        jl level_cell_occupied_at_failed
        mov eax, dword ptr [edx + 14h]
        cmp ecx, eax
        jge level_cell_occupied_at_failed
        cmp esi, dword ptr [edx + 18h]
        jge level_cell_occupied_at_failed
        imul eax, esi
        add eax, ecx
        mov ecx, dword ptr [edx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        and eax, 20000000h
        pop esi
        ret
    level_cell_occupied_at_failed:
        xor eax, eax
        pop esi
        ret
    }
}
#else
uint32_t level_cell_occupied_at_current(const int cell_xy[2])
{
    LevelState* level;
    int x;
    int y;
    if (cell_xy == NULL) return 0u;
    /* FUN_00433090 reads the process-wide active level directly. The portable
     * owner may be the map-cell registry when no entity-pool GameContext is
     * installed, so use the same current-owner resolution as the other
     * coordinate-pair flag readers. */
    level = level_current_owner_get();
    if (level == NULL) return 0u;
    x = cell_xy[0];
    y = cell_xy[1];
    return cell_flag_word(level, x, y) & 0x20000000u;
}
#endif

/* FUN_004330e0: terrain/wave pair mask 0x180000. */
int level_cell_mask_180000(const LevelState* s, int cx, int cy)
{
    return (cell_flag_word(s, cx, cy) & 0x180000u) != 0;
}

// FUNCTION: LEGORR 0x004330e0
#if defined(_WIN32) && defined(_M_IX86)
__declspec(naked) int terrain_cell_mask_180000_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        test dword ptr [ecx + eax*8 + 8], 180000h
        jne terrain_cell_mask_180000_true
        xor eax, eax
    terrain_cell_mask_180000_true:
        pop esi
        ret
    }
}
#else
int terrain_cell_mask_180000_current(const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    if (cell_xy == NULL || level == NULL) return 0;
    return (cell_flag_word_unchecked(level, cell_xy[0], cell_xy[1]) &
            0x180000u) != 0u;
}
#endif

/* FUN_00433120: raise/clear flag bit 0x10000000 (bounds-guarded). */
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

// FUNCTION: LEGORR 0x00433120
#if defined(_WIN32) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flag_10000000_set_current(
    const int cell_xy[2], int set)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        test eax, eax
        mov eax, dword ptr [edx + 4]
        je terrain_cell_flag_10000000_clear
        imul eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 8]
        or dword ptr [eax], 10000000h
        pop esi
        ret
    terrain_cell_flag_10000000_clear:
        imul eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        add eax, esi
        pop esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 0efffffffh
        mov dword ptr [eax], ecx
        ret
        nop
        nop
        nop
        nop
    }
}
#else
void terrain_cell_flag_10000000_set_current(const int cell_xy[2], int set)
{
    LevelState* level = level_current_owner_get();
    if (level != NULL && cell_xy != NULL) {
        level_cell_flag_10000000_set(level, cell_xy[0], cell_xy[1], set);
    }
}
#endif

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

/* Explicit-level adapter. */
uint32_t level_cell_secondary_flag_100(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 0x100u;
}

// FUNCTION: LEGORR 0x00432170
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_secondary_flag_100_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 0ch]
        and eax, 100h
        ret
    }
}
#else
uint32_t terrain_cell_secondary_flag_100_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    return flags & 0x100u;
}
#endif

/* Explicit-level adapter. */
uint32_t level_cell_visited_flag(const LevelState* s, int cx, int cy)
{
    return cell_secondary_word(s, cx, cy) & 2u;
}

// FUNCTION: LEGORR 0x004322d0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_visited_flag_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 0ch]
        and eax, 2
        ret
    }
}
#else
uint32_t terrain_cell_visited_flag_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = width * cell_xy[1] + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    return flags & 2u;
}
#endif

/* Explicit-level secondary bit 0 clear helper. */
void level_cell_secondary_bit0_clear(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word &= ~1u;
}

// FUNCTION: LEGORR 0x004323c0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_secondary_bit0_clear_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 0ch]
        and dword ptr [eax], 0fffffffeh
        ret
    }
}
#else
void terrain_cell_secondary_bit0_clear_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t* flags = (uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    *flags = *flags & 0xfffffffeu;
}
#endif

/* Explicit-level secondary bit 0 set helper. */
void level_cell_secondary_bit0_set(LevelState* s, int cx, int cy)
{
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return;
    }
    uint32_t* word = (uint32_t*)(rec + 0x0C);
    *word |= 1u;
}

// FUNCTION: LEGORR 0x004325d0
#if defined(_WIN32) && defined(_M_IX86)
__declspec(naked) void terrain_cell_secondary_bit0_set_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 0ch]
        or dword ptr [eax], 1
        ret
    }
}
#else
void terrain_cell_secondary_bit0_set_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t* flags = (uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    *flags = *flags | 1u;
}
#endif

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

// FUNCTION: LEGORR 0x004324d0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_secondary_flag_8_set_current(
    int cx, int cy, int set)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov eax, dword ptr [esp + 0ch]
        test eax, eax
        mov eax, dword ptr [ecx + 14h]
        je terrain_cell_secondary_flag_8_clear
        imul eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, edx
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 0ch]
        or dword ptr [eax], 8
        ret
    terrain_cell_secondary_flag_8_clear:
        imul eax, dword ptr [esp + 8]
        add eax, dword ptr [esp + 4]
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 0ch]
        lea eax, dword ptr [eax + edx*8 + 0ch]
        and ecx, 0fffffff7h
        mov dword ptr [eax], ecx
        ret
    }
}
#else
void terrain_cell_secondary_flag_8_set_current(int cx, int cy, int set)
{
    if (set != 0) {
        LevelState* level = g_active_level_retail;
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        volatile uint32_t* words = (volatile uint32_t*)level->word;
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            words[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cy + cx;
        uint32_t* flags = (uint32_t*)(voxels + 0x0c + cell_index * 0x48);
        *flags |= 8u;
        return;
    }
    {
        LevelState* level = g_active_level_retail;
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        volatile uint32_t* words = (volatile uint32_t*)level->word;
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            words[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cy + cx;
        uint32_t* flags = (uint32_t*)(voxels + 0x0c + cell_index * 0x48);
        *flags &= ~8u;
    }
}
#endif

// FUNCTION: LEGORR 0x00432520
uint32_t terrain_cell_secondary_flag_8_current(int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cy + cx;
    uint32_t flags = *(uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    return flags & 8u;
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

// FUNCTION: LEGORR 0x00432aa0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_occupied_or_flag_current(
    int cx, int cy, int mode)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov eax, dword ptr [esp + 0ch]
        test eax, eax
        mov eax, dword ptr [ecx + 14h]
        je occupied_flag_mode_zero
        imul eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, edx
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8]
        mov ecx, dword ptr [eax + 8]
        test ch, 4
        jne occupied_flag_set
        test byte ptr [eax + 0ch], 4
        jne occupied_flag_set
        xor eax, eax
        ret
    occupied_flag_set:
        mov eax, 1
        ret
    occupied_flag_mode_zero:
        imul eax, dword ptr [esp + 8]
        add eax, dword ptr [esp + 4]
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov eax, dword ptr [eax + edx*8 + 8]
        and eax, 400h
        ret
    }
}
#else
uint32_t terrain_cell_occupied_or_flag_current(int cx, int cy, int mode)
{
    LevelState* level = g_active_level_retail;
    if (mode != 0) {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        volatile uint32_t* words = (volatile uint32_t*)level->word;
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            words[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cy + cx;
        uint8_t* record = voxels + cell_index * 0x48;
        uint32_t flags = *(uint32_t*)(record + 8);
        if ((flags & 0x400u) == 0u && (record[0x0c] & 4u) == 0u) {
            return 0u;
        }
        return 1u;
    }
    {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cy + cx;
        uint8_t* record = voxels + cell_index * 0x48;
        uint32_t flags = *(uint32_t*)(record + 8);
        return flags & 0x400u;
    }
}
#endif

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

// FUNCTION: LEGORR 0x00432b00
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_flag_800_bounded_current(
    uint32_t cx, uint32_t cy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        push edi
        mov edx, dword ptr [ecx + 14h]
        lea eax, dword ptr [edx - 1]
        cmp esi, eax
        jae terrain_cell_flag_800_bounded_outside
        mov edi, dword ptr [ecx + 18h]
        mov eax, dword ptr [esp + 10h]
        dec edi
        cmp eax, edi
        jae terrain_cell_flag_800_bounded_outside
        imul eax, edx
        add eax, esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov eax, dword ptr [eax + edx*8 + 8]
        and eax, 800h
        pop edi
        pop esi
        ret
    terrain_cell_flag_800_bounded_outside:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
uint32_t terrain_cell_flag_800_bounded_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
    if (cx < width - 1u &&
        cy < level->word[LEVEL_OFF_HEIGHT / 4] - 1u) {
        uint32_t index = cy * width + cx;
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        uint32_t flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
        return flags & 0x800u;
    }
    return 0u;
}
#endif

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

// FUNCTION: LEGORR 0x00432b50
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_flag_8_bounded_current(
    uint32_t cx, uint32_t cy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        push edi
        mov edx, dword ptr [ecx + 14h]
        lea eax, dword ptr [edx - 1]
        cmp esi, eax
        jae terrain_cell_flag_8_bounded_outside
        mov edi, dword ptr [ecx + 18h]
        mov eax, dword ptr [esp + 10h]
        dec edi
        cmp eax, edi
        jae terrain_cell_flag_8_bounded_outside
        imul eax, edx
        add eax, esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov eax, dword ptr [eax + edx*8 + 8]
        and eax, 8
        pop edi
        pop esi
        ret
    terrain_cell_flag_8_bounded_outside:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
uint32_t terrain_cell_flag_8_bounded_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
    if (cx < width - 1u &&
        cy < level->word[LEVEL_OFF_HEIGHT / 4] - 1u) {
        uint32_t index = cy * width + cx;
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        uint32_t flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
        return flags & 8u;
    }
    return 0u;
}
#endif

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

// FUNCTION: LEGORR 0x00432d90
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_flag_20_bounded_current(
    uint32_t cx, uint32_t cy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        push edi
        mov edx, dword ptr [ecx + 14h]
        lea eax, dword ptr [edx - 1]
        cmp esi, eax
        jae terrain_cell_flag_20_bounded_outside
        mov edi, dword ptr [ecx + 18h]
        mov eax, dword ptr [esp + 10h]
        dec edi
        cmp eax, edi
        jae terrain_cell_flag_20_bounded_outside
        imul eax, edx
        add eax, esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov eax, dword ptr [eax + edx*8 + 8]
        and eax, 20h
        pop edi
        pop esi
        ret
    terrain_cell_flag_20_bounded_outside:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
uint32_t terrain_cell_flag_20_bounded_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
    if (cx < width - 1u &&
        cy < level->word[LEVEL_OFF_HEIGHT / 4] - 1u) {
        uint32_t index = cy * width + cx;
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        uint32_t flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
        return flags & 0x20u;
    }
    return 0u;
}
#endif

/* FUN_00432b90: unchecked signed state byte in {8, 10, 11}. */
int level_cell_type_is_8_or_10_or_11(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = cell_record_unchecked_const(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    const int type = (int)(signed char)rec[2];
    return type == 8 || type == 10 || type == 11;
}

// FUNCTION: LEGORR 0x00432b90
int terrain_cell_type_is_8_or_10_or_11_current(int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cy + cx;
    signed char type = *(signed char*)(voxels + 2 + cell_index * 0x48);
    if (type != 8 && type != 10 && type != 11) {
        return 0;
    }
    return 1;
}

/* FUN_00432dd0: entity-reservation bit (the level_tile_mark flag). */
uint32_t level_cell_flag_reserved(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = cell_record_unchecked_const(s, cx, cy);
    uint32_t flags;
    if (rec == NULL) return 0u;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    return flags & TILE_FLAG_OCCUPIED;
}

// FUNCTION: LEGORR 0x00432dd0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_flag_reserved_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        and eax, 8000000h
        ret
    }
}
#else
uint32_t terrain_cell_flag_reserved_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 8 + cell_index * 0x48);
    return flags & 0x8000000u;
}
#endif

/* FUN_00432710: raise/clear flag 0x80000 (bounds-guarded). */
void level_cell_flag_80000_set(LevelState* s, int cx, int cy, int set)
{
    uint8_t* rec = cell_record_unchecked(s, cx, cy);
    uint32_t flags;
    if (rec == NULL) return;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    if (set != 0) {
        flags |= 0x80000u;
    } else {
        flags &= ~0x80000u;
    }
    __builtin_memcpy(rec + 0x08, &flags, sizeof(flags));
}

/* Retail entry uses an unchecked coordinate-pair ABI; retain the explicit
 * LevelState helper above for portable callers. */
// FUNCTION: LEGORR 0x00432710
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flag_80000_clear_or_set_current(
    const int cell_xy[2], int set)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        push esi
        test eax, eax
        mov eax, dword ptr [ecx + 14h]
        je terrain_cell_flag_80000_clear
        imul eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 8]
        or dword ptr [eax], 80000h
        pop esi
        ret
    terrain_cell_flag_80000_clear:
        imul eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        add eax, esi
        pop esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 0fff7ffffh
        mov dword ptr [eax], ecx
        ret
    }
}
#else
void terrain_cell_flag_80000_clear_or_set_current(const int cell_xy[2],
                                                   int set)
{
    LevelState* level = g_active_level_retail;
    uint8_t* record = (uint8_t*)level->ptr_voxels +
        (cell_xy[1] * (int)level->word[LEVEL_OFF_WIDTH / 4] + cell_xy[0]) *
            LEVEL_VOXEL_STRIDE;
    uint32_t* flags = (uint32_t*)(record + 8);
    if (set != 0) *flags |= 0x80000u;
    else *flags &= ~0x80000u;
}
#endif

/* FUN_004332c0: record byte +3, unsigned. */
int level_cell_record_byte3(const LevelState* s, int cx, int cy)
{
    const uint8_t* rec = level_voxel(s, cx, cy);
    return rec != NULL ? (int)rec[3] : 0;
}

// FUNCTION: LEGORR 0x004332c0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint8_t terrain_cell_record_byte3_current(int x, int y)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        mov eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        imul eax, dword ptr [esp + 8]
        add eax, edx
        xor edx, edx
        lea eax, dword ptr [eax + eax*8]
        mov dl, byte ptr [ecx + eax*8 + 3]
        mov eax, edx
        ret
    }
}
#else
uint8_t terrain_cell_record_byte3_current(int x, int y)
{
    LevelState* level = level_current_owner_get();
    const uint8_t* rec;
    if (level == NULL) return 0u;
    rec = cell_record_unchecked_const(level, x, y);
    return rec != NULL ? rec[3] : 0u;
}
#endif

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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_work_flag_set_current(uint32_t cx,
                                                            uint32_t cy)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        push edi
        mov edx, dword ptr [ecx + 14h]
        lea eax, dword ptr [edx - 1]
        cmp esi, eax
        jae terrain_cell_work_flag_set_done
        mov edi, dword ptr [ecx + 18h]
        mov eax, dword ptr [esp + 10h]
        dec edi
        cmp eax, edi
        jae terrain_cell_work_flag_set_done
        imul eax, edx
        add eax, esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        or ecx, 4
        mov dword ptr [eax], ecx
    terrain_cell_work_flag_set_done:
        pop edi
        pop esi
        ret
    }
}
#else
void terrain_cell_work_flag_set_current(uint32_t cx, uint32_t cy)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s = game != NULL ? game->level : NULL;
    uint32_t width, height;
    uint8_t* rec;
    uint32_t* flags;
    /* FUN_00431560 is the authoritative current-level source; the map-cell
     * registry retains that owner when no entity-pool context is active. */
    if (s == NULL) s = (LevelState*)map_cell_record_grid_level_get();
    if (s == NULL) return;
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
#endif

/* TerrainCellWorkFlagClear (FUN_00430320): target-shaped implementation
 * below obtains the current level from GameContext. This explicit-level
 * worker is also used by the erosion path. The descriptor veto walks the
 * live path/descriptor list rooted at DAT_004b8a24; these records carry the
 * pair at +0x0c/+0x10 and next at +0x24. */
void terrain_cell_work_flag_clear(LevelState* s, const int coord[2],
                                  uint32_t selector)
{
    GameContext* current = entity_pool_callback_context_get();
    if (current != NULL && current->level == s) {
        /* Route live-level mutations through the retail current-level entry
         * point; the explicit-level path below remains for off-screen level
         * maintenance that is not the active DAT_005560d4 equivalent. */
        terrain_cell_work_flag_clear_current(coord, selector);
        return;
    }
    if (s == NULL || coord == NULL) {
        return;
    }
    /* FUN_00430320 calculates the cell record directly from the supplied
     * pair; the caller owns coordinate validation. */
    uint8_t* rec = cell_record_unchecked(s, coord[0], coord[1]);
    if (rec == NULL) {
        return;
    }
    uint32_t* flags = (uint32_t*)(rec + 0x08);
    if ((*flags & 0x400u) != 0u || (rec[0x0c] & 4u) != 0u ||
        (*flags & 0x8000u) != 0u) {
        return;
    }

    if (terrain_cell_predicate_check_current(
            (const uint32_t*)coord) != 0) return;

    *flags &= 0xffdfffffu;
    *flags &= 0xdfffffffu;
    *flags &= 0xbfffffffu;
    map_cell_tracking_activity_state_update(
        entity_pool_callback_context_get(), s);
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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_work_flag_clear_current(
    const int coord[2], uint32_t selector)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push ebx
        push esi
        mov esi, dword ptr [esp + 0ch]
        mov eax, dword ptr [ecx + 14h]
        mov ebx, dword ptr [ecx + 0b8h]
        mov edx, dword ptr [esi + 4]
        push edi
        imul eax, edx
        mov edi, dword ptr [esi]
        add eax, edi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ebx + eax*8]
        mov ebx, dword ptr [eax + 8]
        test bh, 4
        jne terrain_cell_work_flag_clear_done
        test byte ptr [eax + 0ch], 4
        jne terrain_cell_work_flag_clear_done
        mov eax, dword ptr [ecx + 14h]
        imul eax, edx
        add eax, edi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        test ch, 80h
        jne terrain_cell_work_flag_clear_done
        push esi
        call terrain_cell_predicate_check_current
        add esp, 4
        test eax, eax
        jne terrain_cell_work_flag_clear_done
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 0ffdfffffh
        mov dword ptr [eax], ecx
        mov ecx, dword ptr [g_active_level_retail]
        mov ebx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, ebx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov edi, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and edi, 0dfffffffh
        mov dword ptr [eax], edi
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ebx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ebx, 0bfffffffh
        mov dword ptr [eax], ebx
        call entity_activity_scene_state_update_retail
        push esi
        call coordinate_pair_callback_dispatch
        add esp, 4
        push esi
        call flic_opcode27_fallback_noop
        mov ebx, dword ptr [esp + 18h]
        add esp, 4
        mov edi, 4
    terrain_cell_work_flag_clear_queue_loop:
        push ebx
        push esi
        call coordinate_pair_event_queue
        add esp, 8
        dec edi
        jne terrain_cell_work_flag_clear_queue_loop
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        or ecx, 3
        mov dword ptr [eax], ecx
    terrain_cell_work_flag_clear_done:
        pop edi
        pop esi
        pop ebx
        ret
    }
}
#else
void terrain_cell_work_flag_clear_current(const int coord[2],
                                          uint32_t selector)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* flags;
    if (coord == NULL) return;
    /* Retail reads DAT_005560d4 directly. The active map-cell registry is the
     * portable owner when entity-pool context is not installed. */
    s = game != NULL ? game->level : NULL;
    if (s == NULL) s = level_current_owner_get();
    if (s == NULL) return;
    /* The retail entry has no coordinate bounds gate. */
    rec = cell_record_unchecked(s, coord[0], coord[1]);
    if (rec == NULL) return;
    flags = (uint32_t*)(rec + 0x08);
    if ((*flags & 0x400u) != 0u || (rec[0x0c] & 4u) != 0u ||
        (*flags & 0x8000u) != 0u) return;
    if (terrain_cell_predicate_check_current(
            (const uint32_t*)coord) != 0) return;
    *flags &= 0xffdfffffu;
    *flags &= 0xdfffffffu;
    *flags &= 0xbfffffffu;
    map_cell_tracking_activity_state_update(game, s);
    coordinate_pair_callback_dispatch(coord);
    flic_opcode27_fallback_noop();
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    *flags |= 3u;
}
#endif

/* FUN_004502c0: unsigned pair membership (negative coords wrap). */
// FUNCTION: LEGORR 0x004502c0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) static int level_coord_pair_within_bounds_target(
    const uint32_t* dimensions, uint32_t cx, uint32_t cy)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        cmp ecx, dword ptr [eax]
        jb coordinate_x_in_bounds
        xor eax, eax
        ret
    coordinate_x_in_bounds:
        mov edx, dword ptr [esp + 0ch]
        mov ecx, dword ptr [eax + 4]
        cmp edx, ecx
        sbb eax, eax
        neg eax
        ret
    }
}
#else
static int level_coord_pair_within_bounds_target(const uint32_t* dimensions,
                                                uint32_t cx, uint32_t cy)
{
    if (cx < dimensions[0]) {
        return cy < dimensions[1];
    }
    return 0;
}
#endif

int level_coord_pair_within_bounds(const LevelState* s, int cx, int cy)
{
    if (s == NULL) return 0;
    return level_coord_pair_within_bounds_target(
        &s->word[LEVEL_OFF_WIDTH / 4], (uint32_t)cx, (uint32_t)cy);
}

/* FUN_004329d0 ignores its first argument and reads the active level and its
 * terrain dimensions, then tests the voxel's +8 flag word. */
// FUNCTION: LEGORR 0x004329d0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) static int level_cell_flag8_check_target(
    const LevelState* unused, int cx, int cy)
{
    __asm {
        push ebx
        mov ebx, dword ptr [esp + 0ch]
        push esi
        mov esi, dword ptr [g_active_level_retail]
        push edi
        mov edi, dword ptr [esp + 18h]
        mov eax, dword ptr [esi + 4]
        push edi
        push ebx
        push eax
        call level_coord_pair_within_bounds_target
        add esp, 0ch
        test eax, eax
        je flag8_not_set
        mov eax, dword ptr [esi + 14h]
        mov edx, dword ptr [esi + 0b8h]
        imul eax, edi
        add eax, ebx
        lea ecx, dword ptr [eax + eax*8]
        test byte ptr [edx + ecx*8 + 8], 8
        je flag8_not_set
        mov eax, 1
        pop edi
        pop esi
        pop ebx
        ret
    flag8_not_set:
        pop edi
        pop esi
        xor eax, eax
        pop ebx
        ret
    }
}
#else
static int level_cell_flag8_check_target(const LevelState* unused,
                                         int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    TerrainState* terrain;
    uint8_t* voxels;
    uint32_t index;
    (void)unused;
    if (level == NULL) return 0;
    terrain = (TerrainState*)level->ptr_terrain;
    if (terrain == NULL ||
        !level_coord_pair_within_bounds_target(terrain->dim,
            (uint32_t)cx, (uint32_t)cy)) return 0;
    voxels = (uint8_t*)level->ptr_voxels;
    if (voxels == NULL) return 0;
    index = (uint32_t)cy * level->word[LEVEL_OFF_WIDTH / 4u] +
            (uint32_t)cx;
    return (voxels[index * LEVEL_VOXEL_STRIDE + 8u] & 8u) != 0u;
}
#endif

/* Explicit-level adapter for staged callers. The retail target above is
 * selected when the requested level is the active one. */
int level_cell_flag8_check(const LevelState* s, int cx, int cy)
{
    if (s == NULL) return 0;
    if (s == g_active_level_retail)
        return level_cell_flag8_check_target(s, cx, cy);
    if (level_coord_pair_within_bounds(s, cx, cy))
        return (cell_flag_word(s, cx, cy) & 8u) != 0;
    return 0;
}

/* --- map-file resource slots ---------------------------------------------
 * The corpus table interleaves (ptr, flag) pairs across 2000 entries as one
 * 8-byte stride record array; the port mirrors that layout. MapOpen2 reaches
 * this owner through its typed slot callbacks; the claim and release use the
 * corpus's engine-heap allocator pair, 0x48dbd0 / 0x48db80. */

/* One record array, stride 8: pointer at +0, used flag byte at +4
 * (corpus tests bit 0 of the byte, but claims with a dword load/or/store
 * and clears both fields as dwords — MapFileResourceSlot* @ 0x4893f0..). */
typedef struct MapFileResourceSlot {
    uint32_t ptr;   /* +0x00; retail pointer word */
    uint8_t  used;  /* +0x04, bit 0 */
    uint8_t  pad[3];
} MapFileResourceSlot;

/* Native pointers are wider in the portable x64 build. Keep them separately
 * while preserving the retail pointer-word/active-word table layout. */
static uint8_t* s_map_slot_native_ptrs[LEVEL_MAP_SLOT_COUNT];

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
    uint32_t i;
    for (; s < end; s += 2) {
        s[-1] = 0;
        s[0] = 0;
    }
    /* Retail reset clears the pointer words without freeing the payloads. */
    for (i = 0; i < LEVEL_MAP_SLOT_COUNT; ++i)
        s_map_slot_native_ptrs[i] = NULL;
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
            s_map_slots[i].ptr = (uint32_t)(unsigned long)img;
            s_map_slot_native_ptrs[i] = (uint8_t*)img;
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
        engine_heap_free(s_map_slot_native_ptrs[slot]);
        s_map_slot_native_ptrs[slot] = NULL;
        *(uint32_t*)&s_map_slots[slot].ptr = 0;
        *(uint32_t*)&s_map_slots[slot].used = 0;
    }
}

/* FUN_004894a0: the image pointer, or NULL when free/out of range. */
// FUNCTION: LEGORR 0x004894a0
uint8_t* level_map_slot_get(uint32_t slot)
{
    if (slot < LEVEL_MAP_SLOT_COUNT && (s_map_slots[slot].used & 1u) != 0u) {
        return s_map_slot_native_ptrs[slot];
    }
    return NULL;
}

/* FUN_004515a0: header dims at +0x08/+0x0C (port guards a NULL image). */
// FUNCTION: LEGORR 0x004515a0
void level_map_image_header(const uint8_t* image, uint32_t* width,
                            uint32_t* height)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    *width = *(const uint32_t*)(image + 0x08);
    *height = *(const uint32_t*)(image + 0x0c);
#else
    *width = 0;
    *height = 0;
    if (image != NULL) {
        __builtin_memcpy(width, image + 0x08, sizeof(*width));
        __builtin_memcpy(height, image + 0x0C, sizeof(*height));
    }
#endif
}

/* FUN_004515c0: zero-extended u16 cell at +0x10 + (y*width+x)*2. */
// FUNCTION: LEGORR 0x004515c0
uint16_t level_map_image_cell(const uint8_t* image, int x, int width, int y)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const uint16_t*)(image + 0x10 + (y * width + x) * 2);
#else
    if (image == NULL) {
        return 0;
    }
    /* MapFileGetCell performs this multiply/add in 32-bit unsigned
     * arithmetic before doubling the cell index. Keep that behavior on
     * 64-bit hosts as well; the target accessor has no coordinate check. */
    const uint32_t index = (uint32_t)y * (uint32_t)width + (uint32_t)x;
    const uint32_t byte_offset = index * 2u;
    uint16_t cell;
    __builtin_memcpy(&cell, image + 0x10 + byte_offset, sizeof(cell));
    return cell;
#endif
}

/* FUN_004334c0: resolve the resource slot, then read its two header dwords. */
// FUNCTION: LEGORR 0x004334c0
void level_map_dimensions_get(uint32_t slot, uint32_t* width,
                              uint32_t* height)
{
    uint8_t* image = level_map_slot_get(slot);
    image += 0x08;
    *width = *(uint32_t*)image;
    *height = *(uint32_t*)(image + 4);
}

/* FUN_004319e0: cell through the slot; dimensions come from its resource. */
// FUNCTION: LEGORR 0x004319e0
uint16_t level_map_cell_get(uint32_t slot, int x, int y)
{
    const uint8_t* img = level_map_slot_get(slot) + 0x10u;
    uint32_t width, height;
    uint32_t index;
    level_map_dimensions_get(slot, &width, &height);
    index = (uint32_t)y * width + (uint32_t)x;
    return *(const uint16_t*)(img + index * 2u);
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

// FUNCTION: LEGORR 0x00433330
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int terrain_cell_type_get_at_current(
    uint32_t cx, uint32_t cy, uint32_t* out_type)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        push edi
        mov edx, dword ptr [ecx + 14h]
        lea eax, dword ptr [edx - 1]
        cmp esi, eax
        jae terrain_cell_type_get_at_failed
        mov edi, dword ptr [ecx + 18h]
        mov eax, dword ptr [esp + 10h]
        dec edi
        cmp eax, edi
        jae terrain_cell_type_get_at_failed
        imul eax, edx
        add eax, esi
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        xor ecx, ecx
        mov cl, byte ptr [eax + edx*8 + 2]
        mov edx, dword ptr [esp + 14h]
        mov eax, 1
        mov dword ptr [edx], ecx
        pop edi
        pop esi
        ret
    terrain_cell_type_get_at_failed:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
int terrain_cell_type_get_at_current(uint32_t cx, uint32_t cy,
                                     uint32_t* out_type)
{
    LevelState* level = level_current_owner_get();
    const int width = level_int(level, LEVEL_OFF_WIDTH);
    const int height = level_int(level, LEVEL_OFF_HEIGHT);
    if (cx < ((unsigned)width - 1u) && cy < ((unsigned)height - 1u)) {
        const uint8_t* record = level_voxel(level, (int)cx, (int)cy);
        if (record != NULL) *out_type = (uint32_t)record[0x02];
        return 1;
    }
    return 0;
}
#endif

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

/* Retail ABI: coordinate pair plus four output words, with level ownership
 * obtained through DAT_005560d4. Existing game code uses the explicit-level
 * helper above so noncurrent loads can classify cells without changing the
 * target entry's ownership contract. */
// FUNCTION: LEGORR 0x0042c5b0
int terrain_cell_byte_state_classify_current(const int xy[2],
    uint32_t* out_a, uint32_t* out_b, uint32_t* out_c, uint32_t* out_d)
{
    uint32_t values[5];
    uint32_t value;
    uint32_t index;
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    uint32_t cell_index = (uint32_t)xy[1] * width + (uint32_t)xy[0];
    const uint8_t* voxels;
#if defined(_WIN32) && defined(_M_IX86)
    voxels = (const uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4u];
#else
    voxels = (const uint8_t*)level->ptr_voxels;
#endif
    value = (uint32_t)voxels[(size_t)cell_index * 0x48u + 5u];
    values[0] = 1u;
    values[1] = 3u;
    values[2] = 5u;
    values[3] = 10u;
    values[4] = 25u;
    index = value - 1u;
    *out_d = 0u;
    *out_c = 0u;
    *out_b = 0u;
    *out_a = 0u;
    if (value == 0u) return 0;
    value = *(uint32_t*)((uint8_t*)values + (index & 0xfffffffcu));
    switch (index & 3u) {
        case 0u: *out_a = value; return 1;
        case 1u: *out_c = value; return 1;
        case 2u: *out_b = value; return 1;
        default: *out_d = value; return 1;
    }
}

/* ActiveEngineFieldB0Get (FUN_00429850, findings 295/392): the active
 * engine-state dword at level-block +0xb0. */
uint32_t active_engine_field_b0_get(const LevelState* s)
{
    return s != NULL ? (uint32_t)level_int(s, LEVEL_OFF_ENGINE_B0) : 0u;
}

// FUNCTION: LEGORR 0x00429850
uint32_t active_engine_field_b0_get_current(void)
{
    return *(const uint32_t*)((const uint8_t*)g_active_level_retail +
                              LEVEL_OFF_ENGINE_B0);
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

// FUNCTION: LEGORR 0x00429860
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float engine_float_field210_get_by_index_current(int index)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        fld dword ptr [ecx + eax*4 + 210h]
        ret
    }
}
#else
float engine_float_field210_get_by_index_current(int index)
{
    const uint8_t* active = (const uint8_t*)g_active_level_retail;
    return *(const float*)(active + LEVEL_OFF_ENGINE_FLOATS + index * 4);
}
#endif

/* EngineFloatField260Get (FUN_00429880, findings 295/392): the float at
 * active engine-state +0x260. */
// FUNCTION: LEGORR 0x00429880
static float engine_float_field260_get_current_target(void)
{
    return *(const float*)((const uint8_t*)g_active_level_retail +
                           LEVEL_OFF_ENGINE_F260);
}

float engine_float_field260_get(const LevelState* s)
{
    if (s != NULL && s == g_active_level_retail)
        return engine_float_field260_get_current_target();
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

void air_supply_amount_update_current(const struct EntitySlot* e, float delta)
{
    air_supply_amount_update(level_current_owner_get(), e, delta);
}

/* GlobalStateField84Get (FUN_0042b500, findings 324/392): the retail getter
 * reads +0x84 from the active level global and takes no arguments. */
// FUNCTION: LEGORR 0x0042b500
static uint32_t global_state_field84_get_current_target(void)
{
    return *(const uint32_t*)((const uint8_t*)g_active_level_retail +
                              LEVEL_OFF_STATE84);
}

uint32_t global_state_field84_get(const LevelState* s)
{
    if (s != NULL && s == g_active_level_retail)
        return global_state_field84_get_current_target();
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

uint32_t upgrade_resource_check_current(int selector)
{
    return upgrade_resource_check(level_current_owner_get(), selector);
}

/* AlwaysAcceptPlaceCommandCell (FUN_00432550, finding 455): always yes. */
// FUNCTION: LEGORR 0x00432550
int always_accept_place_command_cell(void)
{
    return 1;
}

/* --- tracked/refreshing cell-flag set family ------------------------------
 * Retail's 0x432100 producer writes 100 coordinate pairs at 0x0055684c;
 * 0x432200 snapshots them to 0x00556b6c and clears the secondary 0x100 bit,
 * then 0x4321a0 consumes that snapshot after the entity walk. The separate
 * visited list at 0x00556528 is also 100 coordinate pairs. */

#define LEVEL_CELL_REFRESH_TRACK_CAP 100
#define LEVEL_CELL_VISITED_TRACK_CAP 100

// GLOBAL: LEGORR 0x0055684c
static int32_t s_cell_refresh_track[LEVEL_CELL_REFRESH_TRACK_CAP][2];
// GLOBAL: LEGORR 0x00556e8c
static int     s_cell_refresh_track_count;
// GLOBAL: LEGORR 0x00556b6c
static int32_t s_cell_previous_track[LEVEL_CELL_REFRESH_TRACK_CAP][2];
// GLOBAL: LEGORR 0x00556e90
static int     s_cell_previous_track_count;
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
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    uint32_t flags = 0;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (hooks->cell_flags_get != NULL) {
        flags = hooks->cell_flags_get(x, y, hooks->context);
    }
    else {
        const uint8_t* cell;
        if (level == NULL) return 0;
        cell = level_voxel(level, x, y);
        if (cell == NULL) return 0;
        memcpy(&flags, cell + 8, sizeof(flags));
    }
    if ((flags & 0x100u) != 0u) return 0;
    if (hooks->surface_entity_eligible != NULL)
        return hooks->surface_entity_eligible(x, y, hooks->context) != 0;
    return level != NULL && grid_cell_detail_enabled_get(
        level->ptr_terrain, x, y) != 0;
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

/* TerrainCellRecordCountUpdate (FUN_00403810): query at the cell center,
 * issue the requested entity action when found, and otherwise increment the
 * flattened pending-count table. The spatial callback predicate is still
 * owned by the spatial-query adapter; this routine preserves the exact
 * query/action/fallback order around it. */
// FUNCTION: LEGORR 0x00403810
void terrain_cell_record_count_update(const int cell_xy[2], int a, int b,
                                      int c)
{
    EntitySlot* entity;
    entity = (EntitySlot*)
        unported_ledger_stubs::
            ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
                cell_xy, (uint32_t)a, (uint32_t)b, (uint32_t)c);
    if (entity == NULL) {
        if (c != 0 && b == 7) {
            entity = (EntitySlot*)
                unported_ledger_stubs::
                    ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
                        cell_xy, 7u, 0u, (uint32_t)c);
            if (entity != NULL) {
                EntitySlot* count_entity = (EntitySlot*)
                    unported_ledger_stubs::
                        ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
                            NULL, cell_xy, 7u, 0u);
                uint32_t count = stats_instance_word_at_ec(count_entity);
                if (count == 0u) return;
                do {
                    (void)entity_action_kind_entity_create_and_command_issue(
                        entity, 7, 0, c);
                    --count;
                } while (count != 0u);
                return;
            }
        }
    }
    else if (entity_action_kind_entity_create_and_command_issue(
                 entity, a, b, c) != NULL) {
        return;
    }
    /* DAT_004b34f4 is the 20 x 15 x 16 counter table also swept by
     * CellRecordCountsPruneForEntity. Keep the retail flattening expression;
     * the bounds guard protects the host table when callers provide an
     * invalid selector (the reference writes blindly). */
    if (a >= 0 && a < 20 && b >= 0 && b < 15 && c >= 0 && c < 16)
        ++cmd_pending_command_counts[b * 16 + a * 0xf0 + c];
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

/* CellSurfaceUpdateDispatch (FUN_00434480): resolve the packed surface's
 * per-level tile-model record, sample the live four-corner geometry, then
 * run the two persistent visual channels through the scene resource API. */
// FUNCTION: LEGORR 0x00434480
int terrain_cell_surface_update_dispatch_level(LevelState* level,
                                                int x, int y,
                                                float scale,
                                                float uniform_scale)
{
    uint8_t* cell;
    uint32_t flags;
    TerrainTileMeshGrid* tile_grid;
    SceneCellVisualRelayRecord* visual;
    TerrainSurfaceGrid* surface;
    float vertices[12];
    if (level == NULL) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    memcpy(&flags, cell + 8, sizeof(flags));
    if ((flags & 0x18u) == 0u) return 0;
    tile_grid = level->ptr_tile_mesh_grid;
    visual = terrain_tile_mesh_visual_channels_get(tile_grid, cell[1]);
    if (visual == NULL) return 0;
    surface = level_surface_grid_get(level);
    if (surface == NULL || surface->surface_vectors == NULL || x < 0 ||
        y < 0 || (uint32_t)x >= surface->width ||
        (uint32_t)y >= surface->height) return 0;
    /* FUN_004500d0 returns the cell's diagonal orientation, not a success
     * flag: zero is the normal (unflipped) cell and still has valid vertices. */
    (void)terrain_cell_surface_vertices_get(surface, (uint32_t)x,
                                             (uint32_t)y, vertices);
    /* The record's lookup pointer is the +0x7c texture-variant owner passed
     * by TextureSetConfigLoad into TileModelEntityRegister. */
    if (visual->lookup_table == NULL &&
        tile_grid->visual_lookup_table != NULL)
        visual->lookup_table = (SceneCellVisualLookupTable*)
            tile_grid->visual_lookup_table;
    scene_cell_visual_channels_apply(visual, vertices, cell[1], cell[3],
                                     scale, uniform_scale);
    return 1;
}

/* 0x433380 reads the mutable base mesh through 0x4500d0, adjusts a
 * local copy, then publishes it to the distinct 0x462f20 quad owner. */
static int terrain_cell_mutation_progress_level(LevelState* level,
                                                TerrainSurfaceGrid* surface,
                                                int x, int y)
{
    static const int corner_x[4] = { 0, 1, 1, 0 };
    static const int corner_y[4] = { 0, 0, 1, 1 };
    TerrainState* terrain;
    uint8_t* cell;
    uint32_t flags;
    float base[12];
    float derived[4][3];
    float impact;
    int corner;
    (void)surface;
    if (level == NULL || x < 0 || y < 0 ||
        x >= level_width(level) - 1 || y >= level_height(level) - 1)
        return 0;
    terrain = (TerrainState*)level->ptr_terrain;
    cell = level_voxel(level, x, y);
    if (cell == NULL || terrain == NULL ||
        terrain->derived_mesh.records == NULL ||
        !terrain_cell_base_vertices_get(terrain, (uint32_t)x, (uint32_t)y,
                                         base)) return 0;
    flags = *(uint32_t*)(cell + 8);
    if ((flags & (8u | 0x10u)) == 0u) return 0;
    impact = level_float(level, LEVEL_OFF_DIGDEPTH) +
             level_float(level, LEVEL_OFF_ROOFHEIGHT);
    for (corner = 0; corner < 4; ++corner) {
        uint8_t* vertex = level_voxel(level, x + corner_x[corner],
                                      y + corner_y[corner]);
        derived[corner][0] = base[corner * 3];
        derived[corner][1] = base[corner * 3 + 2];
        derived[corner][2] = base[corner * 3 + 1];
        if ((flags & 8u) != 0u || (vertex != NULL && vertex[0] == 1u))
            derived[corner][2] -= impact;
    }
    terrain_cell_quad_geometry_emit(&terrain->derived_mesh,
        (uint32_t)x, (uint32_t)y, derived);
    return 1;
}

/* TerrainCellMutationProgress has the retail level/x/y ABI. It samples and
 * derives its separate SceneRender quad without writing back into the base. */
// FUNCTION: LEGORR 0x00433380
void terrain_cell_mutation_progress(LevelState* level, uint32_t x,
                                    uint32_t y)
{
    TerrainSurfaceGrid* surface = level_surface_grid_get(level);
    if (level == NULL || x > 0x7fffffffu || y > 0x7fffffffu) return;
    (void)terrain_cell_mutation_progress_level(level, surface,
                                               (int)x, (int)y);
}

void terrain_cell_mutation_progress_current(int x, int y)
{
    if (x >= 0 && y >= 0) {
        GameContext* game = entity_pool_callback_context_get();
        LevelState* level = game != NULL ? game->level : NULL;
        if (level == NULL)
            level = (LevelState*)map_cell_record_grid_level_get();
        if (level != NULL)
            terrain_cell_mutation_progress(level, (uint32_t)x,
                                           (uint32_t)y);
    }
}

static void terrain_level_vertex_offset(LevelState* level, int vertex_x,
                                        int vertex_y, float delta)
{
    static const int cell_x[4] = { 0, -1, -1, 0 };
    static const int cell_y[4] = { 0, 0, -1, -1 };
    static const uint8_t corner[4] = { 0u, 1u, 2u, 3u };
    TerrainState* terrain;
    uint32_t index;
    if (level == NULL || vertex_x < 0 || vertex_y < 0) return;
    terrain = (TerrainState*)level->ptr_terrain;
    if (terrain == NULL || terrain->cell_vertices == NULL ||
        terrain->width < 2 || terrain->height < 2) return;
    for (index = 0u; index < 4u; ++index) {
        int x = vertex_x + cell_x[index];
        int y = vertex_y + cell_y[index];
        uint32_t cell_corner = corner[index];
        size_t cell_index;
        if (x < 0 || y < 0 || x >= terrain->width - 1 ||
            y >= terrain->height - 1) continue;
        cell_index = (size_t)y * (size_t)(terrain->width - 1) +
                     (size_t)x;
        /* cell_vertices is canonical. The indexed mesh applies the retail
         * rotation; applying it here again selects the wrong shared corner.
         * 0x44f380 supplies (0,0,delta) in retail X,Z,height order. */
        terrain->cell_vertices[cell_index * 12u + cell_corner * 3u + 1u]
            += delta;
        terrain_indexed_mesh_cell_sync(terrain, cell_index);
        terrain_create_cell_surface_vectors_rebuild(terrain,
            (uint32_t)cell_index, (uint32_t)y * terrain->dim[2] +
                (uint32_t)x,
            (terrain->blocks[((size_t)y * terrain->dim[2] +
                (size_t)x) * TERRAIN_BLOCK_STRIDE + 0x19u] & 1u) != 0u);
    }
    scene_render_terrain_vertex_offset(level, vertex_x, vertex_y, delta);
}

/* 0x430ee0: initialize one actual level cell, including terrain refresh. */
// FUNCTION: LEGORR 0x00430ee0
int terrain_cell_initialize(LevelState* level, uint32_t x, uint32_t y)
{
    static const int cardinal[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    uint8_t* cell;
    int ix, iy, i, count, last;
    int xy[2];
    int width, height;
    if (level == NULL) return 0;
    width = level_width(level); height = level_height(level);
    if (width < 3 || height < 3 || x == 0u || y == 0u ||
        x >= (uint32_t)(width - 2) || y >= (uint32_t)(height - 2))
        return 0;
    cell = level_voxel(level, (int)x, (int)y);
    if (cell == NULL) return 0;
    if (cell[0] != 0u) return 1;
    terrain_level_vertex_offset(level, (int)x, (int)y,
                                level_float(level, LEVEL_OFF_DIGDEPTH));
    cell[0] = 1u;
    for (iy = (int)y - 1; iy <= (int)y; ++iy) {
        for (ix = (int)x - 1; ix <= (int)x; ++ix) {
            uint8_t* neighbor = level_voxel(level, ix, iy);
            uint32_t flags;
            __builtin_memcpy(&flags, neighbor + 8, sizeof(flags));
            flags |= 0x40000u;
            __builtin_memcpy(neighbor + 8, &flags, sizeof(flags));
        }
    }
    for (iy = (int)y - 1; iy <= (int)y; ++iy)
        for (ix = (int)x - 1; ix <= (int)x; ++ix)
            cell_init_pass(level, (float)ix, (uint32_t)iy);
    for (iy = (int)y - 1; iy <= (int)y + 1; ++iy)
        for (ix = (int)x - 1; ix <= (int)x + 1; ++ix)
            terrain_cell_changed_flag_set(level->ptr_terrain, ix, iy);
    count = 0; last = 0;
    for (i = 0; i < 4; ++i) {
        const uint8_t* neighbor = level_voxel(level,
            (int)x + cardinal[i][0], (int)y + cardinal[i][1]);
        if (neighbor[0] == 1u) { ++count; last = i; }
    }
    xy[0] = (int)x; xy[1] = (int)y;
    if (count == 1)
        cmd_produce(0x0e, NULL, ((last - 2) & 3) | 0x10000, xy);
    terrain_cell_refresh_dispatch_level(level, xy, 1);
    return 1;
}

/* TerrainCellMutation's mode-1 path is used by the tutorial ground visitor as
 * well as the predug-cell sweep. CPU-visible state from both modes is owned
 * here; the indexed DDi height-overlay slot remains a renderer operation. */
// FUNCTION: LEGORR 0x00430530
int terrain_cell_mutate(LevelState* level, uint32_t x, uint32_t y, int mode)
{
    static const int corner_offset[4][2] = {
        { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
    };
    static const int neighbor_offset[8][2] = {
        { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 },
        { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
    };
    uint8_t* target;
    uint32_t* target_flags;
    int cell_xy[2];
    int width, height;
    int index;
    if (level == NULL || x > 0x7fffffffu || y > 0x7fffffffu) return 0;
    width = level_width(level);
    height = level_height(level);
    if (width < 3 || height < 3) return 0;
    if (x == 0u || y == 0u || x >= (uint32_t)(width - 2) ||
        y >= (uint32_t)(height - 2)) return 0;
    target = level_voxel(level, (int)x, (int)y);
    if (target == NULL) return 0;
    target_flags = (uint32_t*)(target + 0x08);
    if ((*target_flags & 8u) != 0u) return 0;
    cell_xy[0] = (int)x;
    cell_xy[1] = (int)y;

    if (target[2] == 8u || target[2] == 10u) {
        uint16_t* steps = (uint16_t*)(target + 0x22);
        memset(target + 0x14, 0, 4u);
        for (index = 0; index < 4; ++index) {
            const int nx = (int)x + corner_offset[index][0];
            const int ny = (int)y + corner_offset[index][1];
            uint8_t* neighbor = level_voxel(level, nx, ny);
            if (neighbor == NULL) continue;
            if (neighbor[0] == 0u) {
                neighbor[0] = 5u;
                message_dispatch(0x25, 0u, 0u, cell_xy);
                if (target[2] == 8u)
                    message_dispatch(1, 0u, 0u, cell_xy);
                else
                    message_dispatch(0x24, 0u, 0u, cell_xy);
            }
            if (neighbor[0] != 1u) {
                --neighbor[0];
                terrain_level_vertex_offset(level, nx, ny,
                    level_float(level, LEVEL_OFF_DIGDEPTH) * 0.25f);
            }
        }
        terrain_neighbor_refresh(level, cell_xy);
        ++*steps;
        if (*steps != 4u) {
            terrain_cell_mutation_progress(level, x, y);
            for (index = 0; index < 8; ++index) {
                terrain_cell_mutation_progress(
                    level,
                    (uint32_t)((int)x + neighbor_offset[index][0]),
                    (uint32_t)((int)y + neighbor_offset[index][1]));
            }
            return 0;
        }
        target[2] = 5u;
        for (index = 0; index < 4; ++index) {
            uint8_t* neighbor = level_voxel(
                level, (int)x + corner_offset[index][0],
                (int)y + corner_offset[index][1]);
            if (neighbor != NULL) neighbor[0] = 1u;
        }
        return terrain_cell_mutate(level, x, y, mode);
    }

    for (index = 0; index < 4; ++index) {
        const int vx = (int)x + corner_offset[index][0];
        const int vy = (int)y + corner_offset[index][1];
        uint8_t* corner_cell = level_voxel(level, vx, vy);
        if (corner_cell != NULL && corner_cell[0] == 0u) {
            terrain_level_vertex_offset(level, vx, vy,
                level_float(level, LEVEL_OFF_DIGDEPTH));
            corner_cell[0] = 1u;
        }
    }
    for (index = 0; index < 8; ++index) {
        uint8_t* neighbor = level_voxel(
            level, (int)x + neighbor_offset[index][0],
            (int)y + neighbor_offset[index][1]);
        if (neighbor != NULL) {
            uint32_t* flags = (uint32_t*)(neighbor + 0x08);
            if ((*flags & 8u) == 0u) {
                *flags |= 0x10u;
                *flags &= ~0x40000u;
            }
        }
    }
    *target_flags |= 8u;
    terrain_neighbor_flag_refresh(level, cell_xy, 1);
    *target_flags &= ~0x20u;
    *target_flags |= 0x400000u;
    if (((*target_flags & 0x4000000u) == 0u ||
         (*target_flags & 0x20000u) != 0u) &&
        (*target_flags & 0x800000u) != 0u)
        global_float_at_00554adc_add_one();
    if (mode == 0) {
        int scan_y;
        for (scan_y = (int)y - 1; scan_y <= (int)y + 1; ++scan_y) {
            int scan_x;
            for (scan_x = (int)x - 1; scan_x <= (int)x + 1; ++scan_x) {
                uint8_t* scan_cell = level_voxel(level, scan_x, scan_y);
                if (scan_cell != NULL && (scan_cell[0x0a] & 2u) != 0u) {
                    GameContext* game =
                        entity_pool_callback_context_get();
                    if (game != NULL && game->level == level)
                        terrain_neighbor_work_flag_scan_current(
                            (unsigned int)scan_x, (unsigned int)scan_y);
                    else
                        terrain_neighbor_work_flag_scan(level,
                            (unsigned int)scan_x, (unsigned int)scan_y);
                    if (system_state_flags_get() == 0u) {
                        message_dispatch(9, 0u, 0u, cell_xy);
                        advisor_text_animation_update(0x12, 1, 0);
                        global_float_at_00554efc_add_one();
                    }
                }
            }
        }
        *target_flags |= 0x800u;
        cmd_produce(0x0e, NULL, target[3], cell_xy);
        terrain_cell_surface_quad_height_fill_current(level, (int)x, (int)y);
        /* Mirror the live cell heights into SceneRender's row-major CPU
         * record. The separate staged DDi object/edge-slot mutation still has
         * no native scene-resource owner. */
        if (s_terrain_cell_overlay_height_fill != NULL) {
            s_terrain_cell_overlay_height_fill(
                s_terrain_cell_overlay_height_fill_context, level, x, y);
        }
        if (((*target_flags & 0x400u) == 0u) &&
            ((target[0x0c] & 4u) == 0u) &&
            ((*target_flags & 0x8000u) == 0u)) {
            terrain_cell_work_flag_clear(level, cell_xy, 0x28u);
            selection_mode7_cancel_at_point((const uint32_t*)cell_xy);
        }
    }
    else {
        *target_flags |= 0x200000u;
    }
    /* Retail's TerrainCellRefreshDispatch always walks the live command pool
     * before resetting the terrain surface type. Keep that state update even
     * when this explicit LevelState is active without a GameContext; the
     * callback walk itself is owned by CmdRec and does not require one. */
    terrain_cell_refresh_dispatch_level(level, cell_xy, 0);
    for (index = -1; index <= 1; ++index) {
        int dx;
        for (dx = -1; dx <= 1; ++dx)
            cell_init_pass(level, (float)((int)x + dx),
                           (uint32_t)((int)y + index));
    }
    *target_flags &= ~0x800000u;
    for (index = -1; index <= 2; ++index) {
        int dx;
        for (dx = -1; dx <= 2; ++dx)
            terrain_cell_changed_flag_set(level->ptr_terrain,
                                           (int)x + dx, (int)y + index);
    }
    if (mode == 0) {
        GameContext* game = entity_pool_callback_context_get();
        if (game != NULL && (game->cheat & 0x80000u) != 0u) {
            const int16_t seed = *(const int16_t*)(target + 0x20);
            const unsigned int magnitude = seed < 0
                ? (unsigned int)(-(int)seed) : (unsigned int)seed;
            if ((magnitude & 3u) == 0u) {
                int variation = (int)seed % 0x549;
                unported_ledger_stubs::
                    ledger_stub_0044b140_SmallSpiderCellSwarmSpawn(
                    (int)x, (int)y, (uint32_t)variation);
            }
        }
    }
    {
        GameContext* game = entity_pool_callback_context_get();
        if (game != NULL && game->level == level)
            deferred_olist_entity_create_and_link(cell_xy);
    }
    {
        uint32_t* secondary = (uint32_t*)(target + 0x0c);
        if ((*secondary & 0x200u) != 0u) {
            *secondary &= ~0x200u;
            *secondary |= 0x20u;
            predug_coordinate_pool_append(cell_xy);
            selection_mode7_cancel_at_point((const uint32_t*)cell_xy);
            cell_init_pass(level, (float)x, y);
        }
    }
    if (mode == 0) {
        message_dispatch(0x1b, 0u, 0u, cell_xy);
        return 1;
    }
    return 1;
}

/* TerrainStampImpactHandler (FUN_00431780) has four arguments and reads the
 * current level plus renderer-owned slot tables from globals. This target
 * entry uses the live LevelState, terrain, sound, and scene-slot owners that
 * the portable runtime exposes. */
// FUNCTION: LEGORR 0x00431780
int terrain_stamp_impact_handle_current(uint32_t x, uint32_t y,
                                        int direction, int mode)
{
    static const int neighbor_x[4] = { 1, 0, -1, 0 };
    static const int neighbor_y[4] = { 0, 1, 0, -1 };
    LevelState* level = level_current_owner_get();
    TerrainSurfaceGrid* surface;
    float position_xy[2];
    float position[3];
    uint32_t neighbors = 0u;
    uint32_t flags;
    uint8_t* cell;
    int i;
    if (direction < 0 || direction >= 4) return 0;
    if (level == NULL || level->ptr_terrain == NULL ||
        x >= (uint32_t)level_int(level, LEVEL_OFF_WIDTH) ||
        y >= (uint32_t)level_int(level, LEVEL_OFF_HEIGHT)) return 0;
    surface = level_surface_grid_get(level);
    if (mode == 0) {
        if (!terrain_cell_world_center_xy_get(level->ptr_terrain,
                (int)x, (int)y, position_xy)) return 0;
    }
    else {
        float edge[3];
        if (surface == NULL ||
            !terrain_cell_edge_vector_query(surface, x, y, edge)) return 0;
        position_xy[0] = edge[0];
        position_xy[1] = edge[2];
    }
    for (i = 0; i < 4; ++i) {
        if ((cell_flag_word_unchecked(level,
                (int)x + neighbor_x[i], (int)y + neighbor_y[i]) & 8u)
            != 0u) ++neighbors;
    }
    position[0] = position_xy[0];
    position[1] = position_xy[1];
    position[2] = terrain_surface_height_interpolate(
        level->ptr_terrain, position_xy[0], position_xy[1]);
    sound_positional_sound_submit(0, 4, 0, 0, position);
    {
        const float effect_height = terrain_surface_height_interpolate(
            level->ptr_terrain, position_xy[0], position_xy[1]);
        /* The decompile's FUN_0040c230 call omits its last two pose slots;
         * the adapter requires them, so retain the neutral pose. */
        if (!scene_slot_pose_scale_register(
                neighbors > 1u, x, y, position_xy[0], position_xy[1],
                effect_height, 0.0f, 0.0f)) return 0;
    }
    flags = cell_flag_word_unchecked(level, (int)x, (int)y);
    flags |= 0x800u;
    cell = cell_record_unchecked(level, (int)x, (int)y);
    if (cell != NULL) __builtin_memcpy(cell + 0x08, &flags, sizeof(flags));
    return 1;
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
    if (direction < 0 || direction >= 4) return 0;
    if (hooks == NULL || hooks->cell_flags_get == NULL ||
        hooks->impact_cell_resolve == NULL || hooks->height_get == NULL ||
        hooks->effect_create == NULL)
        return terrain_stamp_impact_handle_current(x, y, direction, mode);
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
// GLOBAL: LEGORR 0x005562d8
static const char* s_surface_type_names[SURFACE_TYPE_SLOTS];
// GLOBAL: LEGORR 0x00556320
static uint32_t s_surface_type_values[SURFACE_TYPE_SLOTS];

/* These are the keys initialized by GameConfigInit at DAT_00556290. The
 * second token in each SurfaceTypeDescriptions value is a sound alias; the
 * first token is escape-decoded localized text. */
static const char* const s_surface_type_config_keys[SURFACE_TYPE_SLOTS] = {
    "Lego_SurfaceType_Tunnel", "Lego_SurfaceType_Immovable",
    "Lego_SurfaceType_Hard", "Lego_SurfaceType_Medium",
    "Lego_SurfaceType_Loose", "Lego_SurfaceType_Soil",
    "Lego_SurfaceType_Lava", "Lego_SurfaceType_Water",
    "Lego_SurfaceType_OreSeam", "Lego_SurfaceType_Lake",
    "Lego_SurfaceType_CrystalSeam", "Lego_SurfaceType_RechargeSeam",
    "Lego_SurfaceType_Rubble", "Lego_SurfaceType_Reinforcement",
    "Lego_SurfaceType_Path", "Lego_SurfaceType_SlugHole",
    "Lego_SurfaceType_Undiscovered", "Lego_SurfaceType_Cavern"
};

static void surface_type_config_path_build(char* path, size_t capacity,
                                           const char* root,
                                           const char* key)
{
    static const char section[] = "::SurfaceTypeDescriptions::";
    size_t used = 0u;
    size_t i;
    if (path == NULL || capacity == 0u) return;
    path[0] = '\0';
    if (root != NULL) {
        for (i = 0u; root[i] != '\0' && used + 1u < capacity; ++i)
            path[used++] = root[i];
    }
    for (i = 0u; section[i] != '\0' && used + 1u < capacity; ++i)
        path[used++] = section[i];
    if (key != NULL) {
        for (i = 0u; key[i] != '\0' && used + 1u < capacity; ++i)
            path[used++] = key[i];
    }
    path[used] = '\0';
}

/* SurfaceTypeDescriptionsConfigLoad @ 0x4350a0. The caller supplies the
 * loaded Lego.cfg tree and its config-root prefix (DAT_005560c4 in retail).
 * Missing entries leave their corresponding zero/null global values alone. */
// FUNCTION: LEGORR 0x004350a0
void surface_type_descriptions_config_load(BlockFile* config,
                                           const char* config_root)
{
    unsigned int i;
    if (config == NULL) return;
    for (i = 0u; i < SURFACE_TYPE_SLOTS; ++i) {
        char path[512];
        const char* value;
        char value_copy[1024];
        char* alias_name;
        char* value_end;
        char* split;
        char* decoded;
        int alias = 0;
        size_t length = 0u;
        surface_type_config_path_build(path, sizeof(path), config_root,
                                       s_surface_type_config_keys[i]);
        value = block_value(block_find(config, path));
        if (value == NULL) continue;
        while (length < sizeof(value_copy) - 1u && value[length] != '\0') {
            value_copy[length] = value[length];
            ++length;
        }
        value_copy[length] = '\0';
        split = strchr(value_copy, ',');
        if (split == NULL) continue;
        *split++ = '\0';
        alias_name = value_copy;
        while (*alias_name == ' ' || *alias_name == '\t') ++alias_name;
        value_end = alias_name + strlen(alias_name);
        while (value_end > alias_name &&
               (value_end[-1] == ' ' || value_end[-1] == '\t'))
            *--value_end = '\0';
        while (*split == ' ' || *split == '\t') ++split;
        value_end = split + strlen(split);
        while (value_end > split &&
               (value_end[-1] == ' ' || value_end[-1] == '\t'))
            *--value_end = '\0';
        decoded = formatted_text_escape_decode_and_emit("%s", alias_name);
        if (decoded != NULL) s_surface_type_names[i] = decoded;
        if (sound_alias_lookup(split, &alias) == 0) alias = 0;
        s_surface_type_values[i] = (uint32_t)alias;
    }
}

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
    if (hooks == NULL || hooks->model_load == NULL ||
        hooks->model_release == NULL || first_entry == NULL)
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

/* FUN_00439990 clears the level's +0x98 field only on its non-0x10 state
 * path. When the level comes from the map registry rather than the threaded
 * GameContext, reproduce that clear against the active level explicitly. */
static void map_cell_tracking_activity_state_update(GameContext* game,
                                                     LevelState* level)
{
    GameContext* matching_game = game != NULL && game->level == level
        ? game : NULL;
    entity_activity_scene_state_update(matching_game);
    if (matching_game == NULL &&
        (entity_state_flags_peek() & 0x10u) == 0u) {
        global_object_field98_clear(level);
    }
}

/* Explicit-level adapter for the current-level retail entry below. */
void map_cell_tracking_flags_clear_and_refresh(LevelState* s,
                                               const int cell_xy[2])
{
    GameContext* game;
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
    game = entity_pool_callback_context_get();
    map_cell_tracking_activity_state_update(game, s);
    /* FUN_004302b0 calls FUN_00402a40, the coordinate-pair callback walk.
     * FUN_00402b30 is a different callback registry and must not be used for
     * this refresh. */
    coordinate_pair_callback_dispatch(cell_xy);
    flic_opcode27_fallback_noop();
}

/* MapCellTrackingFlagsClearAndRefresh (FUN_004302b0) reads the active level
 * global and receives only the coordinate pair in the retail ABI. */
// FUNCTION: LEGORR 0x004302B0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void map_cell_tracking_flags_clear_and_refresh_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        imul eax, dword ptr [esi + 4]
        add eax, dword ptr [esi]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 8]
        mov ecx, dword ptr [eax]
        and ecx, 0dfffffffh
        mov dword ptr [eax], ecx
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esi]
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 0bfffffffh
        mov dword ptr [eax], ecx
        call entity_activity_scene_state_update_retail
        push esi
        call coordinate_pair_callback_dispatch
        add esp, 4
        push esi
        call flic_opcode27_fallback_noop
        add esp, 4
        pop esi
        ret
    }
}
#else
void map_cell_tracking_flags_clear_and_refresh_current(
    const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    const int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cell_xy[1] + cell_xy[0];
    uint8_t* record = voxels + cell_index * 0x48;
    volatile uint32_t* flags = (volatile uint32_t*)(record + 8);
    uint32_t value = *flags;
    value &= 0xdfffffffu;
    *flags = value;

    cell_index = width * cell_xy[1] + cell_xy[0];
    record = voxels + cell_index * 0x48;
    flags = (volatile uint32_t*)(record + 8);
    value = *flags;
    value &= 0xbfffffffu;
    *flags = value;

    entity_activity_scene_state_update_retail();
    coordinate_pair_callback_dispatch(cell_xy);
    ((void (*)(const int*))flic_opcode27_fallback_noop)(cell_xy);
}
#endif

/* TerrainCellStateUpdate (FUN_00432c90): the retail entry takes only the
 * coordinate pair and reads the active level global. Keep the explicit-level
 * worker for portable callers; the mapped entry below supplies the live
 * GameContext owner. */
static void terrain_cell_state_effect_spawn(LevelState* level,
                                            const int cell_xy[2])
{
    float position[3];
    float center[2];
    float rotation[3];
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    if (terrain == NULL || cell_xy == NULL ||
        !terrain_cell_world_center_xy_get(terrain, cell_xy[0], cell_xy[1],
                                          center))
        return;
    position[0] = center[0];
    position[1] = center[1];
    position[2] = terrain_surface_height_interpolate(
        terrain, position[0], position[1]);
    if (s_terrain_cell_state_projection_hook != NULL) {
        s_terrain_cell_state_projection_hook(
            cell_xy, position, s_terrain_cell_state_projection_context);
    }
    /* FUN_00432c90 selects effect slot 3 with a D3DRM random orientation
     * after resolving the cell center and interpolated terrain height. */
    d3drm_vector_random(rotation);
    (void)lava_effect_select_and_place(3, position, rotation);
}

int terrain_cell_state_update(LevelState* s, const int cell_xy[2])
{
    uint8_t* rec;
    uint32_t* flags;
    uint32_t value;
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
            terrain_cell_state_effect_spawn(s, cell_xy);
        }
        cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
        return state == 0u;
    }
}

// FUNCTION: LEGORR 0x00432C90
int terrain_cell_state_update_current(const int cell_xy[2])
{
    LevelState* s = level_current_owner_get();
    uint8_t* rec;
    uint32_t* flags;
    uint32_t value;
    uint32_t state;
    if (s == NULL) s = (LevelState*)map_cell_record_grid_level_get();
    if (s == NULL || cell_xy == NULL) return 0;
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
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
        terrain_cell_state_effect_spawn(s, cell_xy);
    }
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
    return state == 0u;
}

/* TerrainNeighborFlagSet (FUN_00430470): for an interior, flagged cell,
 * require a staged spatial record at that coordinate, move the primary flag
 * from bit 3 to bit 4, run CellInitPass, and clear its staged render slot. */
// FUNCTION: LEGORR 0x00430470
void terrain_neighbor_flag_set(LevelState* s, int manager,
                               uint32_t x, uint32_t y)
{
#if defined(_WIN32) && defined(_M_IX86)
    uint32_t width;
    uint32_t height;
    uint32_t index;
    uint8_t* voxels;
    uint8_t* rec;
    (void)manager;
    if (x != 0u && y != 0u) {
        width = s->word[LEVEL_OFF_WIDTH / 4];
        if (x < width - 2u) {
            height = s->word[LEVEL_OFF_HEIGHT / 4];
            if (y < height - 2u) {
                index = y * width + x;
                voxels = (uint8_t*)(uintptr_t)
                    s->word[LEVEL_OFF_VOXELS / 4];
                rec = voxels + index * 0x48u;
                if ((rec[8] & 8u) != 0u &&
                    terrain_cell_quad_run_slot_state_get(
                        s, (int)x, (int)y, NULL, NULL) != 0) {
                    *(uint32_t*)(rec + 8) =
                        *(uint32_t*)(rec + 8) & 0xfffffff7u;
                    *(uint32_t*)(rec + 8) =
                        *(uint32_t*)(rec + 8) | 0x10u;
                    cell_init_pass(s, (float)x, y, 0);
                    terrain_cell_surface_quad_reset_current(
                        (int)x, (int)y);
                }
            }
        }
    }
#else
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
    /* Retail checks for a staged cell quad at this coordinate through
     * WorldSpatialRecordFindAtGridCoordinate (0x46e8a0). The LevelState
     * sidecar is that same per-level table; the old hook-only gate prevented
     * the production table from participating when no renderer adapter was
     * installed. */
    if ((*flags & 8u) == 0u ||
        !terrain_cell_quad_run_slot_state_get(s, (int)x, (int)y,
                                               NULL, NULL)) {
        return;
    }
    *flags &= ~8u;
    *flags |= 0x10u;
    /* The retail path runs CellInitPass here, then clears the staged run's
     * selected overlay slot. A dirty-bit mark alone misses its terrain and
     * material-state updates. */
    cell_init_pass(s, (float)x, y);
    terrain_cell_surface_quad_reset_level(s, (int)x, (int)y);
    if (s_terrain_neighbor_render_hook != NULL) {
        s_terrain_neighbor_render_hook(x, y,
                                       s_terrain_neighbor_hook_context);
    }
#endif
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
    if (current[2] == 8) {
        if (s_terrain_neighbor_build_action != NULL) {
            s_terrain_neighbor_build_action(
                neighbor, s_terrain_neighbor_action_context);
        } else {
            /* Retail FUN_00430df0 uses the alternate kind-7 placement setup
             * for a type-8 target: FUN_004394b0(cell, 0, 0, 1). */
            (void)entity_build_placement_command_setup(
                neighbor, 0, NULL, 1, 1);
        }
    } else if (current[2] == 10) {
        if (s_terrain_neighbor_clear_action != NULL) {
            s_terrain_neighbor_clear_action(
                neighbor, s_terrain_neighbor_action_context);
        } else {
            /* Type 10 uses FUN_00439370(cell, 0, 0, 1), the kind-6 form. */
            (void)entity_build_placement_command_setup(
                neighbor, 0, NULL, 1, 0);
        }
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
    map_cell_tracking_activity_state_update(
        entity_pool_callback_context_get(), s);
    return 1;
}

// FUNCTION: LEGORR 0x00432600
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int terrain_cell_variant_flag_propagate_current(
    const int cell_xy[2])
{
    __asm {
        sub esp, 28h
        push ebx
        push ebp
        mov ebp, dword ptr [g_active_level_retail]
        push esi
        mov esi, dword ptr [esp + 38h]
        push edi
        mov eax, dword ptr [ebp + 14h]
        mov ecx, dword ptr [ebp + 0b8h]
        imul eax, dword ptr [esi + 4]
        add eax, dword ptr [esi]
        lea eax, dword ptr [eax + eax*8]
        lea edx, dword ptr [ecx + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 8]
        test eax, 200000h
        je terrain_cell_variant_flag_propagate_failed
        cmp byte ptr [edx + 2], 6
        je terrain_cell_variant_flag_propagate_failed
        xor ecx, ecx
        or ebx, -1
        mov edi, 1
        mov dword ptr [esp + 10h], ecx
        test eax, 80000000h
        mov dword ptr [esp + 14h], ecx
        mov dword ptr [esp + 18h], ecx
        mov dword ptr [esp + 1ch], ebx
        mov dword ptr [esp + 20h], edi
        mov dword ptr [esp + 24h], ecx
        mov dword ptr [esp + 28h], ecx
        mov dword ptr [esp + 2ch], edi
        mov dword ptr [esp + 30h], ebx
        mov dword ptr [esp + 34h], ecx
        je terrain_cell_variant_flag_propagate_mark_refresh
        push ecx
        push esi
        call lava_erosion_pending_cell_add
        mov eax, dword ptr [ebp + 14h]
        mov ecx, dword ptr [esi]
        imul eax, dword ptr [esi + 4]
        add esp, 8
        add eax, ecx
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [ebp + 0b8h]
        mov byte ptr [eax + edx*8 + 7], 0
        jmp terrain_cell_variant_flag_propagate_refresh
    terrain_cell_variant_flag_propagate_mark_refresh:
        or eax, 20000000h
        mov dword ptr [edx + 8], eax
    terrain_cell_variant_flag_propagate_refresh:
        mov eax, dword ptr [ebp + 14h]
        mov edx, dword ptr [esi]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        mov edx, dword ptr [ebp + 0b8h]
        lea edi, dword ptr [esp + 10h]
        mov ebx, 5
        lea ecx, dword ptr [eax + eax*8]
        lea eax, dword ptr [edx + ecx*8 + 8]
        mov ecx, dword ptr [edx + ecx*8 + 8]
        and ecx, 0bfffffffh
        mov dword ptr [eax], ecx
    terrain_cell_variant_flag_propagate_cell_init_loop:
        mov eax, dword ptr [edi + 4]
        mov edx, dword ptr [esi + 4]
        mov ecx, dword ptr [esi]
        add eax, edx
        push 0
        push eax
        add ecx, dword ptr [edi]
        push ecx
        push ebp
        call cell_init_pass
        add esp, 10h
        add edi, 8
        dec ebx
        jne terrain_cell_variant_flag_propagate_cell_init_loop
        call entity_activity_scene_state_update_retail
        mov eax, 1
        pop edi
        pop esi
        pop ebp
        pop ebx
        add esp, 28h
        ret
    terrain_cell_variant_flag_propagate_failed:
        pop edi
        pop esi
        pop ebp
        xor eax, eax
        pop ebx
        add esp, 28h
        ret
    }
}
#else
int terrain_cell_variant_flag_propagate_current(const int cell_xy[2])
{
    static const int offsets[5][2] = {
        {0, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    LevelState* s;
    uint8_t* rec;
    uint32_t* flags;
    uint32_t old_flags;
    int i;
    if (cell_xy == NULL) return 0;
    s = level_current_owner_get();
    if (s == NULL) return 0;
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
    map_cell_tracking_activity_state_update(
        entity_pool_callback_context_get(), s);
    return 1;
}
#endif

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

/* The target sets the secondary +0x0c flag, appends the coordinate to the
 * refresh queue, and invokes the one-cell initializer. */
// FUNCTION: LEGORR 0x00432100
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flag100_set_and_track_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [g_active_level_retail]
        mov ecx, dword ptr [esp + 4]
        push esi
        push 0
        mov eax, dword ptr [edx + 14h]
        mov esi, dword ptr [ecx]
        imul eax, dword ptr [ecx + 4]
        add eax, esi
        mov esi, dword ptr [edx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 0ch]
        mov esi, dword ptr [eax]
        or esi, 100h
        mov dword ptr [eax], esi
        mov esi, dword ptr [s_cell_refresh_track_count]
        mov eax, dword ptr [ecx]
        mov dword ptr [esi*8 + s_cell_refresh_track], eax
        mov esi, dword ptr [s_cell_refresh_track_count]
        mov eax, dword ptr [ecx + 4]
        mov dword ptr [esi*8 + s_cell_refresh_track + 4], eax
        mov esi, dword ptr [s_cell_refresh_track_count]
        inc esi
        mov dword ptr [s_cell_refresh_track_count], esi
        mov eax, dword ptr [ecx + 4]
        mov ecx, dword ptr [ecx]
        push eax
        push ecx
        push edx
        call cell_init_pass
        add esp, 10h
        pop esi
        ret
    }
}
#else
void terrain_cell_flag100_set_and_track_current(const int cell_xy[2])
{
    LevelState* level = terrain_current_level_resolve();
    const uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    const size_t index = (size_t)cell_xy[1] * width +
                         (size_t)cell_xy[0];
    uint8_t* record = (uint8_t*)level->ptr_voxels +
                      index * LEVEL_VOXEL_STRIDE;
    uint32_t* word = (uint32_t*)(record + 0x0cu);
    *word |= 0x100u;
    /* Retail appends directly at DAT_0055684c[count] and increments the
     * global count without a capacity test. Keep this target entry's write
     * sequence direct; the explicit-LevelState adapter keeps its bounded
     * host-side helper. */
    s_cell_refresh_track[s_cell_refresh_track_count][0] = cell_xy[0];
    s_cell_refresh_track[s_cell_refresh_track_count][1] = cell_xy[1];
    ++s_cell_refresh_track_count;
    cell_init_pass(level, (float)cell_xy[0], (uint32_t)cell_xy[1]);
}
#endif

/* TerrainCellPreviousStateTrack (FUN_00432200): copy the just-collected
 * coordinates into the second 100-pair buffer, clear each cell's secondary
 * 0x100 mark, then publish that buffer as the post-walk update queue. The
 * order is significant: the entity walk runs after these marks are cleared
 * and may set them again to suppress the later CellInitPass. */
// FUNCTION: LEGORR 0x00432200
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_previous_state_track_current(void)
{
    __asm {
        mov eax, dword ptr [s_cell_refresh_track_count]
        xor edx, edx
        push esi
        mov esi, dword ptr [g_active_level_retail]
        test eax, eax
        jbe terrain_cell_previous_state_track_done
        push edi
        mov ecx, offset s_cell_refresh_track
    terrain_cell_previous_state_track_loop:
        mov eax, dword ptr [ecx]
        mov dword ptr [ecx + 320h], eax
        mov eax, dword ptr [ecx + 4]
        mov dword ptr [ecx + 324h], eax
        mov eax, dword ptr [ecx + 4]
        imul eax, dword ptr [esi + 14h]
        mov edi, dword ptr [ecx]
        add ecx, 8
        add eax, edi
        mov edi, dword ptr [esi + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [edi + eax*8 + 0ch]
        mov edi, dword ptr [eax]
        and edi, 0fffffeffh
        inc edx
        mov dword ptr [eax], edi
        mov eax, dword ptr [s_cell_refresh_track_count]
        cmp edx, eax
        jb terrain_cell_previous_state_track_loop
        pop edi
    terrain_cell_previous_state_track_done:
        mov dword ptr [s_cell_previous_track_count], eax
        mov dword ptr [s_cell_refresh_track_count], 0
        pop esi
        ret
    }
}
#else
void terrain_cell_previous_state_track_current(void)
{
    LevelState* level = terrain_current_level_resolve();
    uint32_t i;
    if (level == NULL) return;

    for (i = 0u; i < (uint32_t)s_cell_refresh_track_count; ++i) {
        const int x = s_cell_refresh_track[i][0];
        const int y = s_cell_refresh_track[i][1];
        uint8_t* record = cell_record_unchecked(level, x, y);
        s_cell_previous_track[i][0] = x;
        s_cell_previous_track[i][1] = y;
        if (record != NULL) {
            uint32_t* secondary = (uint32_t*)(record + 0x0c);
            *secondary &= ~0x100u;
        }
    }
    s_cell_previous_track_count = s_cell_refresh_track_count;
    s_cell_refresh_track_count = 0;
}
#endif

/* TerrainCellUpdateQueueFlush (FUN_004321a0): after the entity walk, run
 * CellInitPass for each snapshotted cell whose 0x100 secondary mark remains
 * clear, then discard the snapshot count. */
// FUNCTION: LEGORR 0x004321A0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_update_queue_flush_current(void)
{
    __asm {
        mov eax, dword ptr [s_cell_previous_track_count]
        push ebx
        xor ebx, ebx
        push esi
        mov esi, dword ptr [g_active_level_retail]
        test eax, eax
        jbe terrain_cell_update_queue_flush_done
        push edi
        push ebp
        mov edi, offset s_cell_previous_track
    terrain_cell_update_queue_flush_loop:
        mov ecx, dword ptr [edi + 4]
        mov eax, dword ptr [esi + 14h]
        imul eax, ecx
        mov edx, dword ptr [edi]
        mov ebp, dword ptr [esi + 0b8h]
        add eax, edx
        lea eax, dword ptr [eax + eax*8]
        test byte ptr [ebp + eax*8 + 0dh], 1
        jne terrain_cell_update_queue_flush_next
        push 0
        push ecx
        push edx
        push esi
        call cell_init_pass
        add esp, 10h
    terrain_cell_update_queue_flush_next:
        mov eax, dword ptr [s_cell_previous_track_count]
        inc ebx
        add edi, 8
        cmp ebx, eax
        jb terrain_cell_update_queue_flush_loop
        pop ebp
        pop edi
    terrain_cell_update_queue_flush_done:
        pop esi
        mov dword ptr [s_cell_previous_track_count], 0
        pop ebx
        ret
    }
}
#else
void terrain_cell_update_queue_flush_current(void)
{
    LevelState* level = terrain_current_level_resolve();
    uint32_t i;
    if (level == NULL) return;

    for (i = 0u; i < (uint32_t)s_cell_previous_track_count; ++i) {
        const int x = s_cell_previous_track[i][0];
        const int y = s_cell_previous_track[i][1];
        const uint8_t* record = cell_record_unchecked_const(level, x, y);
        if (record != NULL && (record[0x0d] & 1u) == 0u) {
            cell_init_pass(level, (float)x, (uint32_t)y);
        }
    }
    s_cell_previous_track_count = 0;
}
#endif

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

/* The retail entry point reads DAT_005560d4 and appends without checking the
 * 100-pair table's capacity. Keep the explicit-level adapter bounded. */
// FUNCTION: LEGORR 0x00432270
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_visited_flag_set_and_track_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [g_active_level_retail]
        mov ecx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [edx + 14h]
        mov esi, dword ptr [ecx]
        imul eax, dword ptr [ecx + 4]
        mov edx, dword ptr [edx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [edx + eax*8 + 0ch]
        mov edx, dword ptr [eax]
        or edx, 2
        mov dword ptr [eax], edx
        mov edx, dword ptr [s_cell_visited_track_count]
        mov eax, dword ptr [ecx]
        mov dword ptr [edx*8 + s_cell_visited_track], eax
        mov eax, dword ptr [ecx + 4]
        mov ecx, dword ptr [s_cell_visited_track_count]
        mov dword ptr [ecx*8 + s_cell_visited_track + 4], eax
        mov eax, dword ptr [s_cell_visited_track_count]
        inc eax
        mov dword ptr [s_cell_visited_track_count], eax
        ret
    }
}
#else
void terrain_cell_visited_flag_set_and_track_current(const int cell_xy[2])
{
    LevelState* s = terrain_current_level_resolve();
    uint8_t* rec;
    uint32_t* word;
    if (s == NULL || cell_xy == NULL) return;
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) return;
    word = (uint32_t*)(rec + 0x0c);
    *word |= 2u;
    s_cell_visited_track[s_cell_visited_track_count][0] = cell_xy[0];
    s_cell_visited_track[s_cell_visited_track_count][1] = cell_xy[1];
    ++s_cell_visited_track_count;
}
#endif

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

/* The retail entry point is parameterless and reads the current level from
 * DAT_005560d4. Keep the LevelState-taking form above as a portable adapter
 * for callers that already own an explicit level. */
// FUNCTION: LEGORR 0x00432300
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_visited_flags_clear_all_current(void)
{
    __asm {
        mov eax, dword ptr [s_cell_visited_track_count]
        xor edx, edx
        push esi
        mov esi, dword ptr [g_active_level_retail]
        test eax, eax
        jbe terrain_cell_visited_flags_clear_all_done
        push edi
        mov ecx, offset s_cell_visited_track
    terrain_cell_visited_flags_clear_all_loop:
        mov eax, dword ptr [ecx + 4]
        mov edi, dword ptr [ecx]
        imul eax, dword ptr [esi + 14h]
        add eax, edi
        mov edi, dword ptr [esi + 0b8h]
        add ecx, 8
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [edi + eax*8 + 0ch]
        mov edi, dword ptr [eax]
        and edi, 0fffffffdh
        inc edx
        mov dword ptr [eax], edi
        mov eax, dword ptr [s_cell_visited_track_count]
        cmp edx, eax
        jb terrain_cell_visited_flags_clear_all_loop
        pop edi
    terrain_cell_visited_flags_clear_all_done:
        mov dword ptr [s_cell_visited_track_count], 0
        pop esi
        ret
    }
}
#else
void terrain_cell_visited_flags_clear_all_current(void)
{
    LevelState* s = terrain_current_level_resolve();
    int i;
    if (s == NULL) return;
    for (i = 0; i < s_cell_visited_track_count; ++i) {
        uint8_t* rec = cell_record_unchecked(s, s_cell_visited_track[i][0],
                                             s_cell_visited_track[i][1]);
        if (rec != NULL) {
            uint32_t* word = (uint32_t*)(rec + 0x0C);
            *word &= ~2u;
        }
    }
    s_cell_visited_track_count = 0;
}
#endif

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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_primary_flags_mask_clear_and_refresh_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        push 0
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        add eax, esi
        mov esi, dword ptr [ecx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 8]
        mov esi, dword ptr [eax]
        and esi, 0dfef7bffh
        mov dword ptr [eax], esi
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        add eax, esi
        mov esi, dword ptr [ecx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 0ch]
        mov esi, dword ptr [eax]
        and esi, 0fffffffbh
        mov dword ptr [eax], esi
        mov eax, dword ptr [edx + 4]
        mov edx, dword ptr [edx]
        push eax
        push edx
        push ecx
        call cell_init_pass
        add esp, 10h
        pop esi
        ret
    }
}
#else
void terrain_cell_primary_flags_mask_clear_and_refresh_current(
    const int cell_xy[2])
{
    LevelState* s = terrain_current_level_resolve();
    uint8_t* rec;
    uint32_t* primary;
    uint32_t* secondary;
    uint32_t value;
    if (s == NULL || cell_xy == NULL) return;
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
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
#endif

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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flags_set_and_refresh_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        push 0
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        add eax, esi
        mov esi, dword ptr [ecx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 8]
        mov esi, dword ptr [eax]
        or esi, 108000h
        mov dword ptr [eax], esi
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        add eax, esi
        mov esi, dword ptr [ecx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 0ch]
        mov esi, dword ptr [eax]
        or esi, 4
        mov dword ptr [eax], esi
        mov eax, dword ptr [edx + 4]
        mov edx, dword ptr [edx]
        push eax
        push edx
        push ecx
        call cell_init_pass
        add esp, 10h
        pop esi
        ret
    }
}
#else
void terrain_cell_flags_set_and_refresh_current(const int cell_xy[2])
{
    LevelState* s = terrain_current_level_resolve();
    uint8_t* rec;
    uint32_t* primary;
    uint32_t* secondary;
    uint32_t value;
    if (s == NULL || cell_xy == NULL) return;
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
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
#endif

void terrain_cell_flags_set_mask100400(LevelState* s, int x, int y)
{
    uint8_t* rec = level_voxel(s, x, y);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    *primary |= 0x100400u;
    cell_init_pass(s, (float)x, (uint32_t)y);
}

void terrain_cell_flags_set_mask20108000(LevelState* s, int x, int y)
{
    uint8_t* rec = level_voxel(s, x, y);
    if (rec == NULL) {
        return;
    }
    uint32_t* primary = (uint32_t*)(rec + 0x08);
    *primary |= 0x20108000u;
    cell_init_pass(s, (float)x, (uint32_t)y);
}

// FUNCTION: LEGORR 0x00432450
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flags_set_mask100400_current(int x,
                                                                  int y)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 8]
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [ecx + 14h]
        push edi
        imul eax, edx
        mov edi, dword ptr [ecx + 0b8h]
        add eax, esi
        push 0
        push edx
        lea eax, dword ptr [eax + eax*8]
        push esi
        push ecx
        lea eax, dword ptr [edi + eax*8 + 8]
        or dword ptr [eax], 100400h
        call cell_init_pass
        add esp, 10h
        pop edi
        pop esi
        ret
    }
}
#else
void terrain_cell_flags_set_mask100400_current(int x, int y)
{
    LevelState* s = level_current_owner_get();
    uint8_t* rec;
    if (s == NULL) return;
    rec = cell_record_unchecked(s, x, y);
    if (rec == NULL) return;
    *(uint32_t*)(rec + 0x08) |= 0x100400u;
    cell_init_pass(s, (float)x, (uint32_t)y);
}
#endif

// FUNCTION: LEGORR 0x00432490
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flags_set_mask20108000_current(int x,
                                                                     int y)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 8]
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [ecx + 14h]
        push edi
        imul eax, edx
        mov edi, dword ptr [ecx + 0b8h]
        add eax, esi
        push 0
        push edx
        lea eax, dword ptr [eax + eax*8]
        push esi
        push ecx
        lea eax, dword ptr [edi + eax*8 + 8]
        or dword ptr [eax], 20108000h
        call cell_init_pass
        add esp, 10h
        pop edi
        pop esi
        ret
    }
}
#else
void terrain_cell_flags_set_mask20108000_current(int x, int y)
{
    LevelState* s = level_current_owner_get();
    uint8_t* rec;
    if (s == NULL) return;
    rec = cell_record_unchecked(s, x, y);
    if (rec == NULL) return;
    *(uint32_t*)(rec + 0x08) |= 0x20108000u;
    cell_init_pass(s, (float)x, (uint32_t)y);
}
#endif

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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flag40000000_clear_or_set_current(
    const int cell_xy[2], int set)
{
    __asm {
        mov edx, dword ptr [g_active_level_retail]
        mov eax, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 4]
        push esi
        test eax, eax
        mov eax, dword ptr [edx + 14h]
        je terrain_cell_flag40000000_clear
        imul eax, dword ptr [ecx + 4]
        mov esi, dword ptr [ecx]
        add eax, esi
        mov esi, dword ptr [edx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 8]
        mov esi, dword ptr [eax]
        or esi, 40000000h
        jmp terrain_cell_flag40000000_store
    terrain_cell_flag40000000_clear:
        imul eax, dword ptr [ecx + 4]
        mov esi, dword ptr [ecx]
        add eax, esi
        mov esi, dword ptr [edx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 8]
        mov esi, dword ptr [eax]
        and esi, 0bfffffffh
    terrain_cell_flag40000000_store:
        mov dword ptr [eax], esi
    terrain_cell_flag40000000_refresh:
        mov eax, dword ptr [ecx + 4]
        mov ecx, dword ptr [ecx]
        push 0
        push eax
        push ecx
        push edx
        call cell_init_pass
        add esp, 10h
        pop esi
        ret
    }
}
#else
void terrain_cell_flag40000000_clear_or_set_current(const int cell_xy[2],
                                                     int set)
{
    LevelState* s = level_current_owner_get();
    uint8_t* rec;
    uint32_t* primary;
    uint32_t value;
    if (s == NULL || cell_xy == NULL) return;
    /* The retail entry addresses the cell without a grid-bounds check. */
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
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
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1], 0u);
}
#endif

/* LevelDebugInputCellMutatorDispatch (FUN_004288e0), wired stateful branches.
 * The engine calls this dispatcher only while DAT_00556ed0 bit 0x10 is set.
 * Unsupported branches that need the engine's hovered-object pointer or
 * renderer-owned DDi object remain at that owner boundary. */
void level_debug_render_ops_set(const LevelDebugRenderOps* ops)
{
    if (ops == NULL) memset(&s_level_debug_render_ops, 0,
                            sizeof(s_level_debug_render_ops));
    else s_level_debug_render_ops = *ops;
}

void level_debug_input_cell_mutator_dispatch(
    EngineContext* engine, GameContext* ctx, const InputState* input,
    int cell_x, int cell_y)
{
    EntitySlot* entity;
    if (engine == NULL || (engine->debug & 0x10u) == 0u || input == NULL)
        return;

    /* DAT_0076ac4f: selected record's +0x148 bit 0x400 toggle. */
    if (input_key_edge(input, 0xcf) != 0 && selection_has_any(ctx) != 0) {
        entity = selection_first(ctx);
        if (entity != NULL) entity_record_flag400_toggle_and_refresh(entity);
    }

    /* DAT_0076abae is slot 0x2e; the retail body performs the same nested
     * type-8 clear/detach call twice on the first selected entity. */
    if (input_key_edge(input, 0x2e) != 0 && selection_has_any(ctx) != 0) {
        entity = selection_first(ctx);
        if (entity != NULL) {
            entity_clear_nested_state_8_fields(entity);
            entity_clear_nested_state_8_fields(entity);
        }
    }

    /* DAT_0076abb1 (slot 0x31) resets the first selected entity's action
     * sequence with preserve_mode 0. */
    if (input_key_edge(input, 0x31) != 0 && selection_has_any(ctx) != 0) {
        entity = selection_first(ctx);
        if (entity != NULL) entity_action_sequence_reset(entity, 0);
    }

    /* DAT_0076abd8: set the engine 0x01000000 state bit and reset the
     * separate SystemState word only when entering this mode. */
    if (input_key_edge(input, 0x58) != 0) {
        if ((engine->state & 0x01000000u) == 0u) {
            uint32_t zero = 0u;
            engine->state |= 0x01000000u;
            (void)system_state_flags_set(&zero);
        } else {
            engine->state &= ~0x01000000u;
        }
    }

    /* DAT_0076abd7 calls GlobalModeZeroInputSet(0) on every held frame. */
    if (input_key_down(input, 0x57) != 0) {
        unported_ledger_stubs::dependency_grid_mode_zero_input_set(
            ctx != NULL ? ctx->level : NULL, 0);
    }

    /* DAT_0076abc3 flips DAT_004de57c, then forwards that value to the
     * existing scene-transform state owner. */
    if (input_key_edge(input, 0x43) != 0) {
        s_debug_scene_transform_enabled =
            s_debug_scene_transform_enabled == 0 ? 1 : 0;
        scene_transform_enable_set(s_debug_scene_transform_enabled);
    }

    /* DAT_0076abc4: toggle the renderer's debug pose, then reset the paired
     * transform/effect motion snapshot. The callback pair is the owner seam
     * for DAT_005560f0 and DAT_005560e4+0x2c. */
    if (input_key_edge(input, 0x44) != 0) {
        float position_x, position_y, position_z, pose_z;
        s_debug_camera_pose_mode = s_debug_camera_pose_mode == 0 ? 1 : 0;
        if (s_debug_camera_pose_mode == 0) {
            position_x = 200.0f;
            position_y = 140.0f;
            position_z = -130.0f;
            pose_z = 0.75f;
        } else {
            position_x = 250.0f;
            position_y = 190.0f;
            position_z = 20.0f;
            pose_z = 0.0f;
        }
        if (s_level_debug_render_ops.object_position_set != NULL) {
            (void)s_level_debug_render_ops.object_position_set(
                s_level_debug_render_ops.context, position_x, position_y,
                position_z);
        }
        if (s_level_debug_render_ops.object_pose_method_dispatch != NULL) {
            (void)s_level_debug_render_ops.object_pose_method_dispatch(
                s_level_debug_render_ops.context, -1.0f, -0.8f, pose_z,
                0.0f, 1.0f, 0.0f);
        }
        simtick_scene_position_reset_and_effect_motion_cancel();
    }

    if (ctx == NULL || ctx->level == NULL) return;

    /* DAT_0076ab91 is held-key TerrainNeighborFlagSet(level, 0, x, y). */
    if (input_key_down(input, 0x11) != 0) {
        terrain_neighbor_flag_set(ctx->level, 0u, (uint32_t)cell_x,
                                  (uint32_t)cell_y);
    }

    /* DAT_0076ab8e issues one type-0x14 entity command batch. */
    if (input_key_edge(input, 0x0e) != 0) {
        entity_command14_batch_issue(5);
    }

    /* DAT_0076abd1 is a held-key path to TerrainCellInitialize. */
    if (input_key_down(input, 0x51) != 0) {
        level_cell_init_pass(ctx->level, cell_x, cell_y);
    }

    /* DAT_0076abd3: edge-triggered TerrainCellMutation(mode 0). */
    if (input_key_edge(input, 0x53) != 0) {
        (void)terrain_cell_mutate(ctx->level, (uint32_t)cell_x,
                                  (uint32_t)cell_y, 0);
    }

    /* DAT_0076aba4: toggle the cell-object entry selected by the retail
     * presence query. The create/delete helpers own their full pool and
     * entity teardown effects. */
    if (input_key_edge(input, 0x24) != 0) {
        if (grid_cell_record_present_check(cell_x, cell_y) == 0)
            (void)create_cell_object_if_available(cell_x, cell_y);
        else
            (void)delete_cell_object(cell_x, cell_y);
    }

    /* DAT_0076aba3: expand a cell into its configured type-12 entity. */
    if (input_key_edge(input, 0x23) != 0) {
        (void)unported_ledger_stubs::
            ledger_stub_00466250_ExpandCellEntitySpawn(cell_x, cell_y);
    }
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
    /* FUN_004310f0 finishes with CellInitPass(level, x, y, 0). The level
     * module owns that transition, so a dirty-bit mark alone would omit the
     * cell flags, task-counter transitions, and material/variant updates. */
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

// FUNCTION: LEGORR 0x0042f2e0
void terrain_cell_edge_flag_set_and_update(LevelState* s, int cx, int cy,
                                           int set)
{
    uint32_t index = s->word[LEVEL_OFF_WIDTH / 4u] * (uint32_t)cy +
                     (uint32_t)cx;
    uint32_t* flags = (uint32_t*)((uint8_t*)s->ptr_voxels +
                                  (size_t)index * 0x48u + 8u);
    if (set != 0)
        *flags |= 0x200u;
    else
        *flags &= 0xfffffdffu;
    cell_init_pass(s, (float)cx, (uint32_t)cy, 0);
}

static TerrainEdgeMidpointUpdateFn s_terrain_edge_midpoint_update_hook;
static void* s_terrain_edge_midpoint_update_user;

void terrain_edge_midpoint_update_hook_set(TerrainEdgeMidpointUpdateFn hook,
                                           void* user)
{
    s_terrain_edge_midpoint_update_hook = hook;
    s_terrain_edge_midpoint_update_user = user;
}

/* FUNCTION: LEGORR 0x0046e610. Update the LevelState-owned staged-record
 * cell flags from their two edge midpoints; the optional hook performs only
 * the associated native mesh/scene updates. */
void terrain_edge_flag_midpoint_update(LevelState* level)
{
    terrain_world_spatial_edge_midpoints_update(level);
    if (s_terrain_edge_midpoint_update_hook != NULL) {
        s_terrain_edge_midpoint_update_hook(level,
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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_work_flag_circle_set_current(
    const int center_xy[2], int radius)
{
    __asm {
        sub esp, 0ch
        mov eax, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 14h]
        mov dword ptr [esp], eax
        mov eax, edx
        imul eax, edx
        push esi
        mov esi, edx
        neg esi
        push edi
        mov edi, esi
        cmp edi, edx
        mov dword ptr [esp + 0ch], eax
        mov dword ptr [esp + 10h], esi
        jg terrain_cell_work_flag_circle_set_early_done
        push ebp
        mov ebp, dword ptr [esp + 1ch]
        push ebx
    terrain_cell_work_flag_circle_set_outer_loop:
        mov ebx, edi
        imul ebx, edi
        cmp esi, edx
        jg terrain_cell_work_flag_circle_set_outer_next
    terrain_cell_work_flag_circle_set_inner_loop:
        mov ecx, esi
        mov eax, dword ptr [esp + 14h]
        imul ecx, esi
        add ecx, ebx
        cmp ecx, eax
        jg terrain_cell_work_flag_circle_set_next_column
        mov eax, dword ptr [ebp]
        lea ecx, dword ptr [esi + eax]
        test ecx, ecx
        jl terrain_cell_work_flag_circle_set_next_column
        mov eax, dword ptr [esp + 10h]
        cmp ecx, dword ptr [eax + 14h]
        jge terrain_cell_work_flag_circle_set_next_column
        mov eax, dword ptr [ebp + 4]
        add eax, edi
        test eax, eax
        jl terrain_cell_work_flag_circle_set_next_column
        mov edx, dword ptr [esp + 10h]
        cmp eax, dword ptr [edx + 18h]
        jge terrain_cell_work_flag_circle_set_after_mark
        push eax
        push ecx
        call terrain_cell_work_flag_set_current
        add esp, 8
    terrain_cell_work_flag_circle_set_after_mark:
        mov edx, dword ptr [esp + 24h]
    terrain_cell_work_flag_circle_set_next_column:
        inc esi
        cmp esi, edx
        jle terrain_cell_work_flag_circle_set_inner_loop
        mov esi, dword ptr [esp + 18h]
    terrain_cell_work_flag_circle_set_outer_next:
        inc edi
        cmp edi, edx
        jle terrain_cell_work_flag_circle_set_outer_loop
        pop ebx
        pop ebp
    terrain_cell_work_flag_circle_set_early_done:
        pop edi
        pop esi
        add esp, 0ch
        ret
    }
}
#else
void terrain_cell_work_flag_circle_set_current(const int center_xy[2],
                                               int radius)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level;
    if (center_xy == NULL) return;
    level = game != NULL ? game->level : NULL;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return;
    const int width = (int)level->word[LEVEL_OFF_WIDTH / 4u];
    const int height = (int)level->word[LEVEL_OFF_HEIGHT / 4u];
    const int radius_squared = radius * radius;
    int row;
    for (row = -radius; row <= radius; ++row) {
        int column;
        for (column = -radius; column <= radius; ++column) {
            const int x = column + center_xy[0];
            const int y = center_xy[1] + row;
            if (column * column + row * row <= radius_squared &&
                -1 < x && x < width && -1 < y && y < height) {
                terrain_cell_work_flag_set_current((uint32_t)x,
                                                   (uint32_t)y);
            }
        }
    }
}
#endif

/* TerrainNeighborWorkFlagScan (FUN_00431580): the work-flag breadth-first
 * sweep. A double-buffered float-pair queue (two pages of (w+h)*2 entries
 * carved from one (w+h)*0x20 engine-heap block — FUN_0048dbd0/FUN_0048db80)
 * starts at the seed coordinate; each round drains the current page and, for
 * every neighbour (row = y±1, col = x±1, indexed row*w + col in the stride-
 * 0x48 cell records at DAT+0xb8, port: level_voxel), processes the cells
 * carrying work flag bit 0x20000 by calling TerrainCellMutation in mode 1,
 * then clears the bit and queues each neighbour at its own coordinate unless
 * it is the coordinate currently being scanned. Rounds alternate pages until
 * the new page arrives empty. The source indexes out-of-bounds cells and
 * queue capacity blindly; level_voxel safely rejects invalid cells, while
 * the queue retains the source's (w+h)-derived capacity assumption. */
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
                                terrain_cell_mutate(s, (uint32_t)col,
                                                    (uint32_t)row, 1);
                                /* Mutation can publish deferred building
                                 * footprint flags. Retail reloads the live
                                 * record before clearing only the work bit. */
                                rec = level_voxel(s, col, row);
                                if (rec != NULL) {
                                    __builtin_memcpy(&word, rec + 0x08,
                                                     sizeof(word));
                                    word &= 0xfffdffffu;
                                    __builtin_memcpy(rec + 0x08, &word,
                                                     sizeof(word));
                                }
                                if (col != col0 || row != row0) {
                                    unsigned int n = count[1 - cur];
                                    float* dst = page[1 - cur];
                                    dst[2 * n] = (float)col;
                                    dst[2 * n + 1] = (float)row;
                                    count[1 - cur] = n + 1u;
                                }
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
    s = game != NULL ? game->level : NULL;
    if (s == NULL) s = (LevelState*)map_cell_record_grid_level_get();
    if (s == NULL) return;
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
                            terrain_cell_mutate(s, (uint32_t)col,
                                                (uint32_t)row, 1);
                            /* Preserve flags published by mutation,
                             * including a deferred building footprint. */
                            rec = level_voxel(s, col, row);
                            if (rec != NULL) {
                                __builtin_memcpy(&word, rec + 0x08,
                                                 sizeof(word));
                                word &= 0xfffdffffu;
                                __builtin_memcpy(rec + 0x08, &word,
                                                 sizeof(word));
                            }
                            if (col != col0 || row != row0) {
                                const unsigned int n = count[1 - cur];
                                float* dst = page[1 - cur];
                                dst[2 * n] = (float)col;
                                dst[2 * n + 1] = (float)row;
                                count[1 - cur] = n + 1u;
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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void cell_coordinate_timestamp_remember(const int xy[2])
{
    __asm {
        xor ecx, ecx
        mov eax, offset s_ts_active
    cell_coordinate_timestamp_remember_scan:
        cmp dword ptr [eax], 0
        je cell_coordinate_timestamp_remember_store
        add eax, 4
        inc ecx
        cmp eax, 004d78b4h
        jb cell_coordinate_timestamp_remember_scan
        ret
    cell_coordinate_timestamp_remember_store:
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [eax]
        mov dword ptr [ecx*8 + s_ts_coordinates], edx
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx*8 + s_ts_coordinates + 4], eax
        fld dword ptr [s_ts_now]
        fstp dword ptr [ecx*4 + s_ts_stamps]
        mov dword ptr [ecx*4 + s_ts_active], 1
        ret
    }
}
#else
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
#endif

// FUNCTION: LEGORR 0x0040efb0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t cell_coordinate_timestamp_exists(const int xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        push esi
        push edi
        mov ecx, offset s_ts_coordinates
        mov eax, offset s_ts_active
    cell_coordinate_timestamp_exists_scan:
        cmp dword ptr [eax], 0
        je cell_coordinate_timestamp_exists_next
        mov esi, dword ptr [edx]
        mov edi, dword ptr [ecx]
        cmp esi, edi
        jne cell_coordinate_timestamp_exists_next
        mov esi, dword ptr [edx + 4]
        mov edi, dword ptr [ecx + 4]
        cmp esi, edi
        je cell_coordinate_timestamp_exists_found
    cell_coordinate_timestamp_exists_next:
        add eax, 4
        add ecx, 8
        cmp eax, 004d78b4h
        jae cell_coordinate_timestamp_exists_missing
        jmp cell_coordinate_timestamp_exists_scan
    cell_coordinate_timestamp_exists_found:
        mov eax, 1
        pop edi
        pop esi
        ret
    cell_coordinate_timestamp_exists_missing:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
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
#endif

/* --- terrain dirty queue (see level.hpp) ---------------------------------
 * The corpus's queue lives inside its terrain context (u16 pair entries,
 * stride 4, capacity (0x7198-0x4a88)/4 = 2500, count at +0x7198). */
// FUNCTION: LEGORR 0x00450960
void terrain_cell_dirty_queue_push_and_clear(void* terrain, int x, int y,
                                             int push)
{
    TerrainState* t = (TerrainState*)terrain;
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
        if (t->dirty_queue_count == 0u) {
            return; /* corpus's empty-queue path: flag stays set */
        }
        int i;
        for (i = 0; i < (int)t->dirty_queue_count; ++i) {
            if ((uint32_t)t->dirty_queue_x[i] == (uint32_t)x &&
                (uint32_t)t->dirty_queue_y[i] == (uint32_t)y) {
                break;
            }
        }
        if ((uint32_t)i >= t->dirty_queue_count) {
            return; /* corpus's not-found path: flag stays set */
        }
        /* overwrite the found entry with the last one, then drop it */
        t->dirty_queue_x[i] = t->dirty_queue_x[t->dirty_queue_count - 1u];
        t->dirty_queue_y[i] = t->dirty_queue_y[t->dirty_queue_count - 1u];
        --t->dirty_queue_count;
        *flag &= 0xef;
        return;
    }
    if ((*flag & 0x10) != 0) {
        return; /* already queued (faithful) */
    }
    /* The corpus has no capacity branch, but its queue owns exactly 2500
     * entries. Keep the flag clear if a corrupt/overfull state cannot retain
     * the coordinate; otherwise the cell would become permanently stuck. */
    if (t->dirty_queue_count >= LEVEL_DIRTY_QUEUE_CAP) return;
    t->dirty_queue_x[t->dirty_queue_count] = (uint16_t)x;
    t->dirty_queue_y[t->dirty_queue_count] = (uint16_t)y;
    ++t->dirty_queue_count;
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
 * ported owner, so its scale word, scene-object slot, and applied-flag bit
 * land as zero-init own storage behind install/peek hooks. Dirty coordinates
 * and their count are retained by the terrain instance, matching +0x4a88
 * and +0x7198 instead of sharing one queue across levels. */
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
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    TerrainState* terrain;
    if (level == NULL) level = level_current_owner_get();
    terrain = level != NULL ? (TerrainState*)level->ptr_terrain : NULL;
    uint32_t dirty_count = terrain != NULL
        ? terrain->dirty_queue_count : 0u;
    if (enable != 0 && (s_cell_list_flags & 4u) == 0) {
        for (uint32_t i = 0u; i < dirty_count; ++i) {
            /* The corpus strides the u16 pair entries unsigned: the
             * index folds the y-entry through the scale word. */
            const uint32_t x = (uint32_t)terrain->dirty_queue_x[i];
            const uint32_t y = (uint32_t)terrain->dirty_queue_y[i];
            scene_mesh_color_set(s_cell_list_object,
                                 (int)(y * (uint32_t)s_cell_list_scale + x),
                                 0.3f, 0.3f, 0.3f);
        }
        s_cell_list_flags |= 4u;
        return;
    }
    if (enable == 0 && (s_cell_list_flags & 4u) != 0) {
        for (uint32_t i = 0u; i < dirty_count; ++i) {
            const uint32_t x = (uint32_t)terrain->dirty_queue_x[i];
            const uint32_t y = (uint32_t)terrain->dirty_queue_y[i];
            scene_mesh_color_set(s_cell_list_object,
                                 (int)(y * (uint32_t)s_cell_list_scale + x),
                                 0.0f, 0.0f, 0.0f);
        }
        s_cell_list_flags &= ~4u;
    }
}

/* --- dependency and terrain grid table fallback (see level.hpp) -----------
 * DAT 0x4b8bc8..0x4c71fc is a 300-cell, 0x31-dword-stride region addressed
 * by y + x * 15. For each cell, the block at dword 32 + cell * 0x31 holds
 * the selector, then sixteen field-flag words. Parallel counts start at
 * cell * 0x31 and pointer words at cell * 0x31 + 16. The active-level
 * DependencyGridConfigLoad path stores native pointer/count pairs in its
 * LevelState sidecar; this zeroed region is the no-level fallback. */
#define GRID_REC_REGION_DWORDS 14712 /* (0x4c71fc - 0x4b8bc8) / 4 */
#define GRID_REC_COLS          15    /* 20 rows by 15 columns */
#define GRID_REC_STRIDE        0x31  /* dwords per record block */
#define GRID_REC_FIELD_WORDS   16    /* cleared by the mask sweep */

// GLOBAL: LEGORR 0x004b8bc8
static uint32_t s_grid_rec[GRID_REC_REGION_DWORDS];

/* Flat dword index of a cell's selector word, or -1 when the coordinate
 * cannot address the region (bounds deviation). */
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

/* The mapped target walks the retail count/pointer tables directly. */
// FUNCTION: LEGORR 0x0040af90
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) void terrain_cell_record_get_target(
    int x, int y, int field, uint32_t* value_a, uint32_t* value_b)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        lea eax, dword ptr [eax + eax*2]
        lea edx, dword ptr [ecx + eax*4]
        add eax, edx
        lea ecx, dword ptr [eax + eax*2]
        shl ecx, 4
        add ecx, eax
        shl ecx, 2
        mov edx, dword ptr [s_grid_rec + ecx + 80h]
        test edx, edx
        je terrain_cell_record_get_default
        mov edx, dword ptr [esp + 0ch]
        lea ecx, dword ptr [eax + eax*2]
        shl ecx, 4
        add edx, ecx
        add eax, edx
        mov edx, dword ptr [esp + 10h]
        shl eax, 2
        mov ecx, dword ptr [s_grid_rec + eax + 40h]
        mov dword ptr [edx], ecx
        mov ecx, dword ptr [esp + 14h]
        mov eax, dword ptr [s_grid_rec + eax]
        mov dword ptr [ecx], eax
        ret
    terrain_cell_record_get_default:
        mov eax, dword ptr [esp + 10h]
        mov edx, dword ptr [s_grid_rec + ecx + 40h]
        mov dword ptr [eax], edx
        mov edx, dword ptr [esp + 14h]
        mov ecx, dword ptr [s_grid_rec + ecx]
        mov dword ptr [edx], ecx
        ret
    }
}
#else
static void terrain_cell_record_get_target(int x, int y, int field,
                                           uint32_t* value_a,
                                           uint32_t* value_b)
{
    const uint32_t cell = (uint32_t)(y + x * GRID_REC_COLS);
    const uint32_t offset = cell * GRID_REC_STRIDE;
    const uint32_t base = offset +
        (s_grid_rec[offset + 32u] != 0u ? (uint32_t)field : 0u);
    *value_a = s_grid_rec[base + 16u];
    *value_b = s_grid_rec[base];
}
#endif

void terrain_cell_record_get(int x, int y, int field,
                             uint32_t* value_a, uint32_t* value_b)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    const unported_ledger_stubs::DependencyGridTuple* native_records = NULL;
    uint32_t native_count = 0u;
    int64_t block;
    int64_t table_index;
    int64_t pointer_index;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (x < 0 || x >= (int)DEPENDENCY_GRID_ROWS || y < 0 ||
        y >= (int)DEPENDENCY_GRID_COLUMNS) return;
    if (unported_ledger_stubs::dependency_grid_cell_record_get_native(
            level, x, y, field,
            &native_records, &native_count)) {
        /* Preserve the retail 32-bit image word for callers that inspect
         * the output; native dereferences use the sidecar API directly. */
        if (value_a != NULL)
            *value_a = (uint32_t)(uintptr_t)native_records;
        if (value_b != NULL) *value_b = native_count;
        return;
    }
    block = grid_rec_block_index(x, y);
    if (block < 0) return;
    table_index = (block - 32) +
        (s_grid_rec[(size_t)block] != 0u ? field : 0);
    pointer_index = table_index + 16;
    if (table_index < 0 || pointer_index >= GRID_REC_REGION_DWORDS ||
        (s_grid_rec[(size_t)block] != 0u && field < 0)) return;
    if (value_a != NULL && value_b != NULL) {
        terrain_cell_record_get_target(x, y, field, value_a, value_b);
        return;
    }
    if (value_a != NULL) *value_a = s_grid_rec[(size_t)pointer_index];
    if (value_b != NULL) *value_b = s_grid_rec[(size_t)table_index];
}

/* Cell-entry readiness bits occupy bytes at DAT_004b8c4c. DependencyGridConfigLoad
 * produces the parallel record/count lists in the active LevelState sidecar;
 * its flags seed the retail ready-bit word here. */
#define CELL_ENTRY_SLOT_TYPES 20
#define CELL_ENTRY_SLOT_ROWS 15
#define CELL_ENTRY_SLOT_VARIANTS 16
#define CELL_ENTRY_SLOT_STRIDE 49

// FUNCTION: LEGORR 0x0040AEA0
int ensure_cell_entries_loaded(uint32_t kind, int row, int slot)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    if (unported_ledger_stubs::dependency_grid_mode_zero_input_get(level)
        != 0u) {
        return 1;
    }
    if (kind >= CELL_ENTRY_SLOT_TYPES || row < 0
        || row >= CELL_ENTRY_SLOT_ROWS || slot < 0
        || slot >= CELL_ENTRY_SLOT_VARIANTS) {
        return 0;
    }
    const size_t index = ((size_t)kind * CELL_ENTRY_SLOT_ROWS
                          + (size_t)row) * CELL_ENTRY_SLOT_STRIDE
                         + (size_t)slot;
    uint8_t* const ready = (uint8_t*)s_grid_rec + 0x84u + index;
    *ready |= (uint8_t)(unported_ledger_stubs::
        dependency_grid_cell_flags_get(level, kind, (uint32_t)row,
                                       (uint32_t)slot) & 1u);
    if ((*ready & 2u) != 0u) {
        return 1;
    }
    if (!unported_ledger_stubs::dependency_grid_cell_entries_validate(
            level, kind, (uint32_t)row, (uint32_t)slot)) {
        return 0;
    }
    if ((*ready & 1u) != 0u) {
        *ready |= 2u;
    }
    return 1;
}

// CellFlag4IsSet uses the global selector and flag tables directly.
// FUNCTION: LEGORR 0x0040b250
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) uint32_t cell_flag4_is_set_target(int x, int y,
                                                            int field)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        lea eax, dword ptr [eax + eax*2]
        lea edx, dword ptr [ecx + eax*4]
        lea ecx, dword ptr [eax + edx]
        lea eax, dword ptr [ecx + ecx*2]
        shl eax, 4
        add eax, ecx
        shl eax, 2
        mov edx, dword ptr [s_grid_rec + eax + 80h]
        test edx, edx
        je cell_flag4_is_set_default
        mov edx, dword ptr [esp + 0ch]
        lea eax, dword ptr [ecx + ecx*2]
        shl eax, 4
        add edx, eax
        add ecx, edx
        mov eax, dword ptr [s_grid_rec + ecx*4 + 84h]
        and eax, 4
        ret
    cell_flag4_is_set_default:
        mov eax, dword ptr [s_grid_rec + eax + 84h]
        and eax, 4
        ret
    }
}
#else
static uint32_t cell_flag4_is_set_target(int x, int y, int field)
{
    const uint32_t cell = (uint32_t)(y + x * GRID_REC_COLS);
    const uint32_t offset = cell * GRID_REC_STRIDE;
    if (s_grid_rec[offset + 32u] != 0u)
        return s_grid_rec[offset + 33u + (uint32_t)field] & 4u;
    return s_grid_rec[offset + 33u] & 4u;
}
#endif

uint32_t cell_flag4_is_set(int x, int y, int field)
{
    const int64_t block = grid_rec_block_index(x, y);
    const uint32_t dependency_flag =
        unported_ledger_stubs::dependency_grid_flag4_get(x, y, field);
    if (dependency_flag != 0xffffffffu) return dependency_flag;
    if (block < 0) {
        return 0;
    }
    const int64_t word = (s_grid_rec[(size_t)block] != 0u)
        ? grid_rec_field_index(x, y, field)
        : grid_rec_field_index(x, y, 0);
    if (word < 0) {
        return 0;
    }
    return cell_flag4_is_set_target(x, y, field);
}

// The mapped bodies address the retail flag table directly; public helpers
// retain the dependency override and portable bounds checks.
// FUNCTION: LEGORR 0x0040b2a0
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) void cell_flag100_set_target(int x, int y, int field)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        lea eax, dword ptr [eax + eax*2]
        lea edx, dword ptr [ecx + eax*4]
        add eax, edx
        mov edx, dword ptr [esp + 0ch]
        lea ecx, dword ptr [eax + eax*2]
        shl ecx, 4
        add edx, ecx
        add eax, edx
        mov ecx, dword ptr [s_grid_rec + eax*4 + 84h]
        lea eax, dword ptr [s_grid_rec + eax*4 + 84h]
        or ch, 1
        mov dword ptr [eax], ecx
        ret
    }
}
#else
static void cell_flag100_set_target(int x, int y, int field)
{
    const uint32_t cell = (uint32_t)(y + x * GRID_REC_COLS);
    const uint32_t word = cell * GRID_REC_STRIDE + (uint32_t)field + 33u;
    s_grid_rec[word] |= 0x100u;
}
#endif

void cell_flag100_set(int x, int y, int field)
{
    if (unported_ledger_stubs::dependency_grid_flag100_set(x, y, field))
        return;
    const int64_t word = grid_rec_field_index(x, y, field);
    if (word >= 0) cell_flag100_set_target(x, y, field);
}

// FUNCTION: LEGORR 0x0040b2e0
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) uint32_t cell_flag100_is_set_target(int x, int y,
                                                             int field)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        lea eax, dword ptr [eax + eax*2]
        lea edx, dword ptr [ecx + eax*4]
        add eax, edx
        mov edx, dword ptr [esp + 0ch]
        lea ecx, dword ptr [eax + eax*2]
        shl ecx, 4
        add edx, ecx
        add eax, edx
        mov eax, dword ptr [s_grid_rec + eax*4 + 84h]
        and eax, 100h
        ret
    }
}
#else
static uint32_t cell_flag100_is_set_target(int x, int y, int field)
{
    const uint32_t cell = (uint32_t)(y + x * GRID_REC_COLS);
    const uint32_t word = cell * GRID_REC_STRIDE + (uint32_t)field + 33u;
    return s_grid_rec[word] & 0x100u;
}
#endif

uint32_t cell_flag100_is_set(int x, int y, int field)
{
    const uint32_t dependency_flag =
        unported_ledger_stubs::dependency_grid_flag100_get(x, y, field);
    if (dependency_flag != 0xffffffffu) return dependency_flag;
    const int64_t word = grid_rec_field_index(x, y, field);
    if (word < 0) {
        return 0;
    }
    return cell_flag100_is_set_target(x, y, field);
}

/* The active table and its dimension owner are mirrored from the cell-kind
 * registry. The Reccmp GLOBAL alias for this address is already attached to
 * the shared descriptor view in crt.cpp; do not annotate a second mirror. */
static uint32_t* s_grid_present_table;
// GLOBAL: LEGORR 0x004c7dfc
static const uint32_t* s_grid_record_manager;

// FUNCTION: LEGORR 0x0040e460
int grid_cell_record_present_check(int x, int y)
{
    size_t index;
    s_grid_present_table = (uint32_t*)map_cell_record_grid_get();
    s_grid_record_manager =
        (const uint32_t*)map_cell_record_grid_level_get();
    if (s_grid_present_table == NULL || s_grid_record_manager == NULL ||
        x < 0 || y < 0) return 0;
    index = (size_t)s_grid_record_manager[5] * (size_t)y + (size_t)x;
    /* MapCellRecordGridInitialize allocates 0x14 bytes per cell, but the
     * retail accessors address the first 0x0c-byte record by this stride. */
    return s_grid_present_table[index * 3u] != 0u;
}

/* --- corner normal sampling (see level.hpp) ------------------------------- */
// FUNCTION: LEGORR 0x004512d0
#if defined(_MSC_VER) && defined(_M_IX86)
int terrain_cell_normal_sample(const void* terrain, uint32_t x, uint32_t y,
                               uint32_t corner, float out[3])
{
    const TerrainState* t = (const TerrainState*)terrain;
    const uint8_t* p;
    float* second;
    float length;
    float inverse;
    if (x < t->dim[0] && y < t->dim[1]) {
        p = t->blocks + (y * t->dim[2] + x) * TERRAIN_BLOCK_STRIDE;
        if ((p[0x19] & 1u) != 0u) corner = (corner + 1u) & 3u;
        if (corner == 0u) {
            out[0] = *(const float*)(p + 0x00);
            out[1] = *(const float*)(p + 0x04);
            out[2] = *(const float*)(p + 0x08);
        }
        if (corner == 2u) {
            second = (float*)(p + 0x0c);
            out[0] = second[0];
            out[1] = second[1];
            out[2] = second[2];
        }
        if (corner == 1u || corner == 3u) {
            second = (float*)(p + 0x0c);
            out[0] = second[0] + *(const float*)(p + 0x00);
            out[1] = second[1] + *(const float*)(p + 0x04);
            out[2] = second[2] + *(const float*)(p + 0x08);
            length = (float)sqrt(out[0] * out[0] + out[2] * out[2] +
                                 out[1] * out[1]);
            inverse = 1.0f / length;
            out[0] = inverse * out[0];
            out[1] = inverse * out[1];
            out[2] = inverse * out[2];
        }
        return 1;
    }
    out[2] = 0.0f;
    out[1] = 0.0f;
    out[0] = 0.0f;
    return 0;
}
#else
int terrain_cell_normal_sample(const void* terrain, uint32_t x, uint32_t y,
                               uint32_t corner, float out[3])
{
    const TerrainState* t = (const TerrainState*)terrain;
    uint32_t sel;
    const uint8_t* p;
    const float* fa;
    const float* fb;
    float v[3];
    if (out == NULL) return 0;
    if (t == NULL || t->blocks == NULL ||
        x >= t->dim[0] || y >= t->dim[1]) {
        for (int i = 0; i < 3; ++i) {
            out[i] = 0.0f;
        }
        return 0;
    }
    p = t->blocks + ((size_t)y * (size_t)t->dim[2] + (size_t)x) *
        TERRAIN_BLOCK_STRIDE;
    sel = corner;
    if ((p[0x19] & 1u) != 0u) {
        sel = (sel + 1u) & 3u;
    }
    fa = (const float*)(p + 0x00);
    fb = (const float*)(p + 0x0c);
    if (sel == 0u) {
        v[0] = fa[0]; v[1] = fa[1]; v[2] = fa[2];
    } else if (sel == 2u) {
        v[0] = fb[0]; v[1] = fb[1]; v[2] = fb[2];
    } else if (sel == 1u || sel == 3u) {
        v[0] = fb[0] + fa[0];
        v[1] = fb[1] + fa[1];
        v[2] = fb[2] + fa[2];
        /* FUN_004512d0's x87 expression evaluates x*x + z*z + y*y. */
        const float inv = 1.0f / (float)
            sqrt(v[0] * v[0] + v[2] * v[2] + v[1] * v[1]);
        v[0] *= inv;
        v[1] *= inv;
        v[2] *= inv;
    }
    else {
        /* The retail switch leaves the output untouched for other corner
         * selectors, while still returning success for the in-bounds cell. */
        return 1;
    }
    out[0] = v[0];
    out[1] = v[1];
    out[2] = v[2];
    return 1;
}
#endif

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
static uint32_t terrain_neighbor_coordinate_check_explicit(
    const LevelState* s, int x, int y, uint8_t* kind, int mode)
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

/* The retail entry uses the active level and returns the classification via
 * a four-argument ABI. Keep the explicit-level table helper above for tools
 * and callers operating on an inactive level. */
// FUNCTION: LEGORR 0x0042f350
static uint8_t terrain_neighbor_coordinate_check_current_target(
    int x, int y, uint8_t* kind, int mode)
{
    int offsets[8];
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    uint32_t height = level->word[LEVEL_OFF_HEIGHT / 4u];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#if defined(_WIN32) && defined(_M_IX86)
    voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4u];
#endif
    offsets[0] = 0;
    offsets[1] = -1;
    offsets[2] = 1;
    offsets[3] = 0;
    offsets[4] = 0;
    offsets[5] = 1;
    offsets[6] = -1;
    offsets[7] = 0;
    uint32_t mask = 0u;
    uint32_t i;
    for (i = 0u; i < 4u; ++i) {
        const int nx = x + offsets[i * 2u];
        const int ny = y + offsets[i * 2u + 1u];
        int in_bounds = 0;
        int occupied = 0;
        size_t cell_index = 0u;
        if (nx >= 0 && ny >= 0 && (uint32_t)nx < width &&
            (uint32_t)ny < height) {
            in_bounds = 1;
            cell_index = (size_t)(uint32_t)ny * width + (uint32_t)nx;
            uint32_t flags;
            __builtin_memcpy(&flags,
                voxels + cell_index * LEVEL_VOXEL_STRIDE + 8u,
                sizeof(flags));
            occupied = (flags & 0x20000000u) != 0u;
        }
        /* Retail performs this second flag probe even after the first probe
         * rejects an out-of-map neighbor. Keep the port's edge guard. */
        if (!occupied && in_bounds) {
            uint32_t flags;
            __builtin_memcpy(&flags,
                voxels + cell_index * LEVEL_VOXEL_STRIDE + 8u,
                sizeof(flags));
            if ((flags & 0x400u) != 0u) occupied = 1;
        }
        if (occupied) mask |= 1u << i;
    }

    if (mode == 0) {
        switch (mask) {
        case 0: *kind = 0; return 0x65;
        case 1: *kind = 2; return 0x65;
        case 2: *kind = 3; return 0x65;
        case 3: *kind = 3; return 99;
        case 4: *kind = 0; return 0x65;
        case 5: *kind = 0; return 0x62;
        case 6: *kind = 0; return 99;
        case 7: *kind = 0; return 100;
        case 8: *kind = 1; return 0x65;
        case 9: *kind = 2; return 99;
        case 10: *kind = 1; return 0x62;
        case 11: *kind = 3; return 100;
        case 12: *kind = 1; return 99;
        case 13: *kind = 2; return 100;
        case 14: *kind = 1; return 100;
        default: *kind = 0; return 0x60;
        }
    }
    switch (mask) {
    case 0: *kind = 0; return 0x75;
    case 1: *kind = 2; return 0x75;
    case 2: *kind = 3; return 0x75;
    case 3: *kind = 3; return 0x73;
    case 4: *kind = 0; return 0x75;
    case 5: *kind = 0; return 0x72;
    case 6: *kind = 0; return 0x73;
    case 7: *kind = 0; return 0x74;
    case 8: *kind = 1; return 0x75;
    case 9: *kind = 2; return 0x73;
    case 10: *kind = 1; return 0x72;
    case 11: *kind = 3; return 0x74;
    case 12: *kind = 1; return 0x73;
    case 13: *kind = 2; return 0x74;
    case 14: *kind = 1; return 0x74;
    default: *kind = 0; return 0x71;
    }
}

uint32_t terrain_neighbor_coordinate_check(const LevelState* s, int x, int y,
                                           uint8_t* kind, int mode)
{
    if (s != NULL && s == g_active_level_retail && kind != NULL) {
        return (uint32_t)terrain_neighbor_coordinate_check_current_target(
            x, y, kind, mode);
    }
    return terrain_neighbor_coordinate_check_explicit(s, x, y, kind, mode);
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
 * feed the level-owned SceneRender cell-color cache; alpha is fixed at 1.0
 * in every target call. */
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
        + ((size_t)y * (size_t)t->dim[2] + (size_t)x) *
            TERRAIN_BLOCK_STRIDE;
    if (t->owner_level != NULL) {
        scene_render_terrain_cell_surface_color_set(
            t->owner_level, (uint32_t)x, (uint32_t)y,
            mode != 0 ? r : 1.0f,
            mode != 0 ? g : 1.0f,
            mode != 0 ? b : 1.0f);
    }
    if (mode == 0) {
        /* corpus: the mesh half gets (1,1,1) and (1,1,1) alpha, and the
         * byte is cleared. */
        blk[0x24] = 0u;
        (void)r; (void)g; (void)b;
    }
    /* FUN_00475020 writes the cell mesh's RGB and fixed opaque alpha. The
     * SceneRender cache stores the RGB under the same width-minus-one quad
     * stride; TerrainState's +0x24 appearance byte remains separate from the
     * material byte at +0x18. */
    (void)r; (void)g; (void)b;
}

// FUNCTION: LEGORR 0x0044f4e0
uint8_t terrain_cell_surface_type_set(void* terrain, int x, int y, int type)
{
    TerrainState* t = (TerrainState*)terrain;
    if (t == NULL || t->blocks == NULL) {
        return 0;
    }
    if (x < 0 || y < 0 || x >= t->width || y >= t->height) {
        return 0; /* deviation: the corpus indexes blind */
    }
    uint8_t* blk = t->blocks
        + ((size_t)y * (size_t)t->dim[2] + (size_t)x) *
            TERRAIN_BLOCK_STRIDE;
    const uint8_t prior = blk[0x24];
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
 * The target walks EntityPoolCallbackSearch with LAB_00402bb0, whose body is
 * absent from the recovered corpus, then resets the terrain surface type.
 * Preserve the known traversal and ordering while leaving the callback's
 * effects to an explicit runtime binding. */
typedef struct TerrainCellRefreshCallbackPayload {
    const int* cell_xy;
    int mode;
} TerrainCellRefreshCallbackPayload;

static TerrainCellRefreshEntityCallback s_terrain_cell_refresh_callback;
static void* s_terrain_cell_refresh_callback_context;

static int terrain_cell_refresh_callback_adapter(EntitySlot* entity,
                                                  void* payload)
{
    TerrainCellRefreshCallbackPayload* data =
        (TerrainCellRefreshCallbackPayload*)payload;
    if (data == NULL || s_terrain_cell_refresh_callback == NULL) return 0;
    return s_terrain_cell_refresh_callback(entity, data->cell_xy, data->mode,
        s_terrain_cell_refresh_callback_context);
}

void terrain_cell_refresh_entity_callback_install(
    TerrainCellRefreshEntityCallback callback, void* context)
{
    s_terrain_cell_refresh_callback = callback;
    s_terrain_cell_refresh_callback_context = context;
}

static void terrain_cell_refresh_dispatch_level(LevelState* lvl,
                                                const int cell_xy[2],
                                                int mode)
{
    TerrainCellRefreshCallbackPayload payload;
    if (lvl == NULL || cell_xy == NULL) return;
    payload.cell_xy = cell_xy;
    payload.mode = mode;
    (void)entity_pool_callback_search(
        terrain_cell_refresh_callback_adapter, &payload);
    if (lvl->ptr_terrain != NULL) {
        terrain_cell_surface_type_reset(lvl->ptr_terrain, cell_xy[0],
                                        cell_xy[1]);
    }
}

void terrain_cell_refresh_dispatch(const struct GameContext* ctx, int x,
                                   int y)
{
    LevelState* lvl = (ctx != NULL) ? ctx->level : NULL;
    const int cell_xy[2] = { x, y };
    /* Explicit-context callers correspond to the observed mode-zero call
     * sites. Keep the same callback-before-surface-reset ordering as the
     * retail entrypoint. */
    terrain_cell_refresh_dispatch_level(lvl, cell_xy, 0);
}

/* Retail ABI: coordinate pair plus mode, current level fetched internally.
 * The level-taking helper above remains for portable callers with an
 * explicit context. */
// FUNCTION: LEGORR 0x00402ae0
void terrain_cell_refresh_dispatch_current(const int cell_xy[2], int mode)
{
    LevelState* lvl = level_current_owner_get();
    TerrainCellRefreshCallbackPayload payload;
    void* terrain;
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (lvl == NULL || cell_xy == NULL) return;
#endif
    payload.cell_xy = cell_xy;
    payload.mode = mode;
    (void)entity_pool_callback_search(
        terrain_cell_refresh_callback_adapter, &payload);
#if defined(_MSC_VER) && defined(_M_IX86)
    terrain = (void*)lvl->word[1];
    terrain_cell_surface_type_reset(terrain, cell_xy[0], cell_xy[1]);
#else
    terrain = lvl->ptr_terrain;
    if (terrain != NULL)
        terrain_cell_surface_type_reset(terrain, cell_xy[0], cell_xy[1]);
#endif
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

/* The retail entries below operate on DAT_005560d4 directly. Keep the
 * explicit-level variants separate for portable callers with owned state. */
// FUNCTION: LEGORR 0x0041f870
void global_object_field8c_increment(void)
{
    LevelState* s = g_active_level_retail;
    if (s != NULL) {
        s->word[0x8C / 4] = s->word[0x8C / 4] + 1u;
    }
}

void global_object_field8c_increment_for_level(LevelState* s)
{
    if (s != NULL) level_set_int(s, 0x8C, level_int(s, 0x8C) + 1);
}

// FUNCTION: LEGORR 0x0041f880
void global_object_field8c_subtract(int amount)
{
    LevelState* s = g_active_level_retail;
    if (s != NULL) {
        s->word[0x8C / 4] = s->word[0x8C / 4] - (uint32_t)amount;
    }
}

void global_object_field8c_subtract_for_level(LevelState* s, int amount)
{
    if (s != NULL) level_set_int(s, 0x8C, level_int(s, 0x8C) - amount);
}

// FUNCTION: LEGORR 0x0041f8a0
void global_object_field98_add(int amount)
{
    LevelState* s = g_active_level_retail;
    if (s != NULL) {
        s->word[0x98 / 4] = s->word[0x98 / 4] + (uint32_t)amount;
    }
}

void global_object_field98_add_for_level(LevelState* s, int amount)
{
    if (s != NULL) level_set_int(s, 0x98, level_int(s, 0x98) + amount);
}

void global_object_field98_clear(LevelState* s)
{
    if (s != NULL) {
        level_set_int(s, 0x98, 0);
    }
}

// FUNCTION: LEGORR 0x0041f8c0
void global_object_field98_clear_current(void)
{
    LevelState* level = g_active_level_retail;
    if (level != NULL) {
        level->word[0x98 / 4] = 0u;
    }
}

// FUNCTION: LEGORR 0x0041f920
void level_state_field9c_add(int amount)
{
    LevelState* s = g_active_level_retail;
    if (s != NULL) {
        s->word[0x9C / 4] = s->word[0x9C / 4] + (uint32_t)amount;
    }
}

void level_state_field9c_add_for_level(LevelState* s, int amount)
{
    if (s != NULL) level_set_int(s, 0x9C, level_int(s, 0x9C) + amount);
}

// FUNCTION: LEGORR 0x0041f980
void global_object_field94_increment(void)
{
    LevelState* s = g_active_level_retail;
    if (s != NULL) {
        s->word[0x94 / 4] = s->word[0x94 / 4] + 1u;
    }
}

void global_object_field94_increment_for_level(LevelState* s)
{
    if (s != NULL) level_set_int(s, 0x94, level_int(s, 0x94) + 1);
}

// FUNCTION: LEGORR 0x0041f990
void global_object_fielda8_increment(void)
{
    LevelState* s = g_active_level_retail;
    if (s != NULL) {
        s->word[0xA8 / 4] = s->word[0xA8 / 4] + 1u;
    }
}

void global_object_fielda8_increment_for_level(LevelState* s)
{
    if (s != NULL) level_set_int(s, 0xA8, level_int(s, 0xA8) + 1);
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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int terrain_object_type_sfx_at_cell_get(
    const void* position_record)
{
    __asm {
        push ecx
        mov edx, dword ptr [esp + 8]
        lea eax, dword ptr [esp]
        lea ecx, dword ptr [esp + 8]
        push eax
        push ecx
        push edx
        call copy_two_dword_pair_to_outputs
        mov eax, dword ptr [esp + 14h]
        mov ecx, dword ptr [esp + 0ch]
        add esp, 0ch
        lea eax, dword ptr [eax + eax*2]
        lea edx, dword ptr [ecx*4]
        lea eax, dword ptr [eax + eax*4]
        mov eax, dword ptr [edx + eax*4 + s_terrain_sfx_grid]
        pop ecx
        ret
    }
}
#else
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
#endif

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
 * reads the name blind at its record +4, the BlockRec key), entry value =
 * the sound alias reference. */
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
    const char* path = string_path_join(root_path, "ObjTtSFXs", NULL);
    for (BlockRec* entry = block_subgroup(config, path);
         entry != NULL; entry = block_next_equal_key(entry)) {
        /* The corpus loads the name blind (mov ecx,[esi+4]), the record
         * key. The chain already proved entry non-NULL. */
        const char* name = entry->key;
        if (entity_type_row_for_name(name, &obj_x,
                                     &obj_y) == 0) {
            continue;
        }
        int sfx = 0;
        /* The entry chain proves non-NULL; read its value blind. */
        const char* value = entry->value;
        if (sound_alias_lookup(value, &sfx) == 0) {
            continue;
        }
        const int cell = obj_y + obj_x * 15;
        s_terrain_sfx_grid[cell] = sfx; /* the corpus's blind scale store */
    }
}

/* --- terrain choice resource select (2026-10-25) ---------------------------
 * The first record word selects one of four independently allocated arrays;
 * the second word indexes that array. Kinds 6/7 select scalar resources.
 * An installer exposes array contents because the encyclopedia config
 * loader has no production owner in the portable startup path. */
typedef struct TerrainChoiceWindow {
    uint32_t* values;
    uint32_t count;
} TerrainChoiceWindow;
static TerrainChoiceWindow s_choice_windows[4];
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
        const TerrainChoiceWindow* window = &s_choice_windows[kind - 1u];
        s_choice_active = index < window->count
            ? window->values[index] : 0u;
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
    if (word >= 0)
        terrain_choice_window_array_install(word % 4, word / 4, value);
}

uint32_t terrain_choice_window_peek(int word)
{
    int kind, index;
    if (word < 0) return 0u;
    kind = word % 4;
    index = word / 4;
    if (index >= (int)s_choice_windows[kind].count) return 0u;
    return s_choice_windows[kind].values[index];
}

void terrain_choice_window_array_install(int kind, int index,
                                         uint32_t value)
{
    TerrainChoiceWindow* window;
    uint32_t count;
    uint32_t* values;
    if (kind < 0 || kind >= 4 || index < 0) return;
    window = &s_choice_windows[kind];
    count = (uint32_t)index + 1u;
    if (count > window->count) {
        if ((size_t)count > 0xffffffffu / sizeof(*values)) return;
        values = (uint32_t*)realloc(window->values,
                                    (size_t)count * sizeof(*values));
        if (values == NULL) return;
        memset(values + window->count, 0,
               (size_t)(count - window->count) * sizeof(*values));
        window->values = values;
        window->count = count;
    }
    window->values[index] = value;
}

void terrain_choice_scalar_install(int scalar, uint32_t value)
{
    if (scalar >= 0 && scalar < 2) {
        s_choice_scalar[scalar] = value;
    }
}

uint32_t terrain_choice_active_peek(void) { return s_choice_active; }
uint32_t terrain_choice_flags_peek(void) { return s_choice_flags; }
void terrain_choice_flags_set(uint32_t flags) { s_choice_flags = flags; }
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
    global_object_field98_add_for_level(level, amount);
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

// FUNCTION: LEGORR 0x00409cd0
void cell_cardinal_neighbor_apply_current(const int coord_pair[2])
{
    static const int offsets[8] = { 0,-1, 1,0, 0,1, -1,0 };
    int i;
    if (coord_pair == NULL) return;
    for (i = 0; i < 4; ++i) {
        const int pair[2] = { coord_pair[0] + offsets[i * 2],
                              coord_pair[1] + offsets[i * 2 + 1] };
        terrain_cell_refresh_dispatch_current(pair, 0);
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

/* TerrainCreate's 0x34/0x4034 reciprocal variant lists are hosted in a
 * separate allocation on native builds. This accessor supplies the same
 * window to the surface-vector pass and its 0x451180 predicate. */
const void* terrain_cell_variant_window_get(const void* terrain)
{
    const TerrainState* state = (const TerrainState*)terrain;
    return state != NULL ? state->variant_window : NULL;
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
 * corpus's center rejection and adjacent/distance-two checks per axis. The
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

    static const int8_t offsets[2][4][2] = {
        { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } },
        { { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 } },
    };
    for (int direction = 0; direction < 4; ++direction) {
        int distance;
        /* FUN_0040d240 tests the adjacent cell, then the distance-two cell
         * on that same cardinal axis before advancing to the next axis. */
        for (distance = 0; distance < 2; ++distance) {
            const int x = cx + offsets[distance][direction][0];
            const int y = cy + offsets[distance][direction][1];
            if (level_coord_pair_within_bounds(s, x, y) == 0) {
                continue;
            }
            if ((sidecar_present != NULL &&
                 sidecar_present(s, x, y) != 0) ||
                terrain_cell_special_state_predicate(s, x, y, 0) != 0) {
                return 1;
            }
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
    if (s_path_record_surface_grid != NULL &&
        s_path_record_surface_grid != grid) {
        LevelState* prior_owner = s_path_record_surface_grid->owner_level;
        s_path_record_surface_grid->owner_level = NULL;
        if (prior_owner != NULL)
            terrain_surface_grid_terrain_window_set(
                s_path_record_surface_grid, NULL);
        if (prior_owner != NULL && prior_owner->ptr_tile_mesh_grid != NULL &&
            prior_owner->ptr_tile_mesh_grid->surface_grid ==
                s_path_record_surface_grid) {
            prior_owner->ptr_tile_mesh_grid->surface_grid = NULL;
        }
    }
    s_path_record_surface_grid = grid;
    if (grid != NULL) {
        GameContext* game = entity_pool_callback_context_get();
        LevelState* current_level = game != NULL ? game->level : NULL;
        if (current_level == NULL)
            current_level = (LevelState*)map_cell_record_grid_level_get();
        if (current_level != NULL) {
            if (grid->owner_level == NULL)
                grid->owner_level = current_level;
            if (grid->owner_level == current_level)
                grid->record_width = (uint32_t)level_width(current_level);
            if (grid->owner_level == current_level &&
                current_level->ptr_tile_mesh_grid != NULL)
                current_level->ptr_tile_mesh_grid->surface_grid = grid;
            if (grid->owner_level == current_level &&
                current_level->ptr_terrain != NULL)
                terrain_surface_grid_terrain_window_set(
                    grid, terrain_cell_variant_window_get(
                        current_level->ptr_terrain));
        }
        else if (grid->owner_level != NULL) {
            grid->record_width = (uint32_t)level_width(grid->owner_level);
            if (grid->owner_level->ptr_terrain != NULL)
                terrain_surface_grid_terrain_window_set(
                    grid, terrain_cell_variant_window_get(
                        grid->owner_level->ptr_terrain));
        }
    }
}

TerrainSurfaceGrid* path_record_surface_grid_get(void)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* current_level = game != NULL ? game->level : NULL;
    if (current_level == NULL)
        current_level = (LevelState*)map_cell_record_grid_level_get();
    if (current_level != NULL &&
        current_level->ptr_tile_mesh_grid != NULL &&
        current_level->ptr_tile_mesh_grid->surface_grid != NULL) {
        return current_level->ptr_tile_mesh_grid->surface_grid;
    }
    return s_path_record_surface_grid;
}

static TerrainSurfaceGrid* level_surface_grid_get(const LevelState* level)
{
    TerrainSurfaceGrid* surface;
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    if (level != NULL && level->ptr_tile_mesh_grid != NULL &&
        level->ptr_tile_mesh_grid->surface_grid != NULL) {
        surface = level->ptr_tile_mesh_grid->surface_grid;
        if (terrain != NULL) {
            terrain_surface_grid_terrain_window_set(
                surface, terrain_cell_variant_window_get(terrain));
            /* FUN_0044f230's +0x28 table is TerrainCreate's 0x28-byte
             * physical block image. Bind that live image when no explicit
             * surface record owner was supplied. */
            if (surface->cell_records == NULL)
                surface->cell_records = terrain->blocks;
            if (surface->record_width == 0u)
                surface->record_width = terrain->dim[2];
        }
        return surface;
    }
    surface = path_record_surface_grid_get();
    if (surface != NULL && surface->owner_level != NULL &&
        surface->owner_level != level) return NULL;
    if (surface != NULL && terrain != NULL) {
        terrain_surface_grid_terrain_window_set(
            surface, terrain_cell_variant_window_get(terrain));
        if (surface->cell_records == NULL)
            surface->cell_records = terrain->blocks;
        if (surface->record_width == 0u)
            surface->record_width = terrain->dim[2];
    }
    return surface;
}

void level_terrain_material_transition_mesh_hook_set(
    LevelTerrainMaterialTransitionMeshHook hook, void* context)
{
    s_material_transition_mesh_hook = hook;
    s_material_transition_mesh_context = context;
}

static LevelState* terrain_material_transition_owner_get(
    TerrainSurfaceGrid* grid)
{
    if (grid != NULL && grid->owner_level != NULL)
        return grid->owner_level;
    return level_current_owner_get();
}

/* The retail transition path reads the four current mesh vertices. Use the
 * indexed mesh first and SceneRender's live quad cache when no TerrainState
 * geometry is available. TerrainSurfaceGrid::surface_vectors stores the
 * separate corner vectors and is not vertex-position data. */
static int terrain_material_transition_vertices_get(
    LevelState* level, TerrainState* terrain, TerrainSurfaceGrid* grid,
    uint32_t x, uint32_t y, float out[12])
{
    uint32_t corner;
    if (terrain_cell_surface_values_available(terrain, (int)x, (int)y)) {
        grid_cell_read_surface_values(terrain, (int)x, (int)y, out);
        return 1;
    }
    if (grid == NULL || x >= grid->width || y >= grid->height) return 0;
    if (level != NULL) {
        float scene_vertices[4][3];
        if (!scene_render_terrain_cell_vertices_get(
                level, x, y, scene_vertices)) return 0;
        for (corner = 0u; corner < 4u; ++corner) {
            out[corner * 3u] = scene_vertices[corner][0];
            out[corner * 3u + 1u] = scene_vertices[corner][2];
            out[corner * 3u + 2u] = scene_vertices[corner][1];
        }
        return 1;
    }
    return 0;
}

/* FUN_0044e6d0 copies words 6 and 7 from each nine-word vertex output slot
 * into its 0x38-byte transition record. Keep the full indexed-mesh payload
 * here so the saved words remain the source UV pair; the twelve-word
 * GridCellReadSurfaceValues result contains positions only. */
static int terrain_material_transition_vertex_payload_get(
    TerrainState* terrain, uint32_t x, uint32_t y, float out[4][9])
{
    TerrainIndexedMeshLayerObject object;
    TerrainIndexedMeshLayerOwner owner;
    size_t quad_index;
    if (terrain == NULL || out == NULL ||
        x >= terrain->dim[0] || y >= terrain->dim[1] ||
        terrain->indexed_mesh_blocks == NULL ||
        terrain->indexed_mesh_vertices == NULL) return 0;
    quad_index = (size_t)y * terrain->dim[0] + x;
    terrain_indexed_mesh_cell_sync(terrain, quad_index);
    memset(out, 0, sizeof(float) * 4u * 9u);
    terrain_indexed_mesh_layer_object_init(terrain, &object, &owner);
    (void)lwo_layer_polygon_vertex_data_read(&object, (int)quad_index, 0,
                                            4, out);
    return 1;
}

// FUNCTION: LEGORR 0x0044e6d0
void terrain_cell_material_transition_begin(TerrainSurfaceGrid* grid,
                                            uint32_t x, uint32_t y,
                                            int material,
                                            int rotation)
{
    LevelState* level = terrain_material_transition_owner_get(grid);
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    uint32_t index = 0u;
    uint32_t record_width;
    size_t record_index;
    uint8_t prior_material;
    float vertices[12];
    float vertex_payload[4][9];
    int have_vertices = 0;
    if (level == NULL || (grid == NULL && terrain == NULL)) return;
    /* The retail routine receives the terrain renderer object directly. The
     * port may enter through a helper that has only the level, so recover its
     * live surface owner before applying the final material operation. */
    if (grid == NULL) grid = level_surface_grid_get(level);
    if (grid != NULL) {
        if (x >= grid->width || y >= grid->height) return;
        index = y * grid->width + x;
    }
    else if (terrain == NULL || x >= terrain->dim[0] ||
             y >= terrain->dim[1]) return;
    record_width = grid != NULL && grid->record_width != 0u
        ? grid->record_width
        : (uint32_t)level_width(level);
    if (record_width == 0u && grid != NULL) record_width = grid->width;
    record_index = (size_t)y * record_width + x;
    if (terrain != NULL && terrain->blocks != NULL &&
        record_index < (size_t)terrain->dim[2] * terrain->dim[3]) {
        prior_material = terrain->blocks[record_index * 0x28u + 0x18u];
    }
    else if (grid != NULL && grid->cell_records != NULL) {
        prior_material = grid->cell_records[record_index * 0x28u + 0x18u];
    }
    else {
        prior_material = (uint8_t)material;
    }

    if (prior_material != (uint8_t)material &&
        level->material_transition_suppress_flags[prior_material] == 0u) {
        have_vertices = terrain_material_transition_vertices_get(
            level, terrain, grid, x, y, vertices) &&
            terrain_material_transition_vertex_payload_get(
                terrain, x, y, vertex_payload);
    }
    if (have_vertices) {
        uint32_t slot_index;
        for (slot_index = 0u; slot_index < 10u; ++slot_index) {
            LevelTerrainMaterialTransitionSlot* slot =
                &level->material_transition_slots[slot_index];
            uint32_t corner;
            if ((slot->flags & 1u) != 0u) continue;
            slot->timer = 0.05f;
            slot->cell_x = (int32_t)x;
            slot->cell_y = (int32_t)y;
            for (corner = 0u; corner < 4u; ++corner) {
                slot->saved_uv[corner][0] = vertex_payload[corner][6];
                slot->saved_uv[corner][1] = vertex_payload[corner][7];
            }
            slot->reserved = 0u;
            slot->flags |= 1u;
            if (s_material_transition_mesh_hook != NULL) {
                (void)s_material_transition_mesh_hook(
                    s_material_transition_mesh_context, level, slot_index, 0,
                    (int)x, (int)y, vertices, 1.0f);
            }
            break;
        }
    }

    /* The reference invokes the material setter whether or not a transition
     * slot was available. */
    if (grid != NULL)
        terrain_cell_material_set(grid, x, y, (uint8_t)material,
                                  (uint8_t)rotation);
    if (terrain != NULL && terrain->blocks != NULL &&
        record_index < (size_t)terrain->dim[2] * terrain->dim[3])
        terrain->blocks[record_index * 0x28u + 0x18u] = (uint8_t)material;
}

// FUNCTION: LEGORR 0x0044f090
void terrain_material_transition_tick(TerrainSurfaceGrid* grid, float delta)
{
    LevelState* level = terrain_material_transition_owner_get(grid);
    TerrainState* terrain = level != NULL
        ? (TerrainState*)level->ptr_terrain : NULL;
    uint32_t slot_index;
    if (level == NULL) return;
    if (grid == NULL) grid = level_surface_grid_get(level);
    for (slot_index = 0u; slot_index < 10u; ++slot_index) {
        LevelTerrainMaterialTransitionSlot* slot =
            &level->material_transition_slots[slot_index];
        float vertices[12];
        int have_vertices = 0;
        if ((slot->flags & 1u) == 0u) continue;
        if (slot->cell_x < 0 || slot->cell_y < 0) {
            slot->flags &= ~1u;
            continue;
        }
        have_vertices = terrain_material_transition_vertices_get(
            level, terrain, grid, (uint32_t)slot->cell_x,
            (uint32_t)slot->cell_y, vertices);
        if (!have_vertices) {
            slot->flags &= ~1u;
            continue;
        }
        if (s_material_transition_mesh_hook != NULL) {
            (void)s_material_transition_mesh_hook(
                s_material_transition_mesh_context, level, slot_index, 1,
                slot->cell_x, slot->cell_y, vertices, 1.0f - slot->timer);
        }
        slot->timer += delta * 0.04f;
        if (slot->timer > 1.0f) {
            if (s_material_transition_mesh_hook != NULL) {
                (void)s_material_transition_mesh_hook(
                    s_material_transition_mesh_context, level, slot_index, 2,
                    slot->cell_x, slot->cell_y, vertices,
                    1.0f - slot->timer);
            }
            slot->flags &= ~1u;
        }
    }
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

/* The two terrain path-record operations share this exact voxel transition:
 * registration sets the active bit and record link, while teardown clears
 * them; an adjacent duplicate point controls the paired-point bit. */
int level_cell_path_record_state_apply(LevelState* level, int x, int y,
                                       void* record, int is_registering,
                                       int duplicate_point)
{
    uint8_t* cell = level_voxel(level, x, y);
    uint32_t* flags;
    if (cell == NULL) return 0;
    flags = (uint32_t*)(cell + 0x08);
    if (is_registering) {
        if (!level_cell_path_record_set(level, x, y, record)) return 0;
        if (duplicate_point) *flags |= 0x8000u;
        *flags |= 0x100000u;
    }
    else {
        if (!level_cell_path_record_set(level, x, y, NULL)) return 0;
        if (duplicate_point) *flags &= ~0x8000u;
        *flags &= ~0x100000u;
    }
    level_cell_init_pass(level, x, y);
    return 1;
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
    /* CountMatchingCoordinatesInList (0x408e10) walks this same pointer
     * array using the independent progress/count word at +0x20. Retail's
     * OperationGroupAppendObject advances both words for every appended
     * entity. */
    *(uint32_t*)((char*)record + 0x20) =
        *(uint32_t*)((char*)record + 0x20) + 1u;
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
    *(uint32_t*)((char*)record + 0x20) = port->count;
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

void path_record_remove_and_free(void* record);

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
void path_record_remove_and_free(void* record)
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
    path_record_storage_release(record);
    engine_heap_free(record);
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
                void* node = engine_heap_alloc(8u + sizeof(void*));
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

/* Explicit-level adapter retained for callers that already own a context.
 * The mapped retail entry point below uses the current global level. */
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

    cell_init_pass((LevelState*)lvl, (float)x, (uint32_t)y);
    /* FUN_004311d0 receives param3 == 0 here; it initializes local matrix
     * data and returns without creating or changing the build record. */
    const int pair[2] = { x, y };
    coordinate_callback_dispatch(pair, 0);      /* 0x402b30 seam */
    message_dispatch(0x1c, 0, 0, pair);         /* 0x419b80 */
}

// FUNCTION: LEGORR 0x00431140
void terrain_cell_queued_work_transition_current(int x, int y)
{
    LevelState* lvl = level_current_owner_get();
    uint8_t* rec;
    uint32_t* word;
    int pair[2];
    uint32_t flags;
    if (lvl == NULL) return;
    rec = cell_record_unchecked(lvl, x, y);
    if (rec == NULL) return;
    word = (uint32_t*)(rec + 0x08);
    flags = *word;
    if ((flags & 0x10u) == 0u || (flags & 0xe0u) != 0u) return;
    *word = flags | 0x20u;
    cell_init_pass(lvl, (float)x, (uint32_t)y);
    /* FUN_004311d0(lvl, pair, 0) has no state effect on this branch. */
    pair[0] = x;
    pair[1] = y;
    coordinate_callback_dispatch_current(pair);
    message_dispatch(0x1c, 0, 0, pair);
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

/* --- erosion timing band (2026-10-25) -------------------------------------
 * These module statics mirror the reference timing, stage, and lock tables;
 * EmergeMap and the live ticker now own their initialization and updates. */
#define EROSION_MARK_COUNT 2000   /* DAT_004cfbb4 */
#define EROSION_TICKET_COUNT LEVEL_TS_COUNT /* DAT_004d6914 */

// GLOBAL: LEGORR 0x0049e124
static const float s_erosion_ticks_per_second = 25.0f;
// GLOBAL: LEGORR 0x0049e128
static const float s_erosion_delay_fraction = 0.2f;
// GLOBAL: LEGORR 0x0049e130
static const float s_erosion_stage_quanta = 4.0f;
// GLOBAL: LEGORR 0x004d78b8
static float s_erosion_decay_scale;
// GLOBAL: LEGORR 0x004d78bc
static float s_erosion_delay_scale;
struct LavaErosionPendingCell {
    int x;
    int y;
};
typedef char lava_erosion_pending_cell_stride_check[
    sizeof(LavaErosionPendingCell) == 8u ? 1 : -1];
// GLOBAL: LEGORR 0x004c7eb0
static LavaErosionPendingCell s_lava_erosion_pending_cells[EROSION_MARK_COUNT];
// GLOBAL: LEGORR 0x004cbd30
static uint32_t s_lava_erosion_pending_count;
// GLOBAL: LEGORR 0x004cbd34
static LavaErosionPendingCell s_lava_erosion_stage_coordinates[
    EROSION_MARK_COUNT];
// GLOBAL: LEGORR 0x004cfbb4
static uint32_t s_erosion_marks[EROSION_MARK_COUNT];
// GLOBAL: LEGORR 0x004d1af4
static float s_lava_erosion_stage_timers[EROSION_MARK_COUNT];
// GLOBAL: LEGORR 0x004d78b4
static float s_erosion_trigger_accumulator;
static int s_erosion_triggered_this_tick;
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
    memset(s_lava_erosion_stage_coordinates, 0,
           sizeof(s_lava_erosion_stage_coordinates));
    memset(s_lava_erosion_stage_timers, 0,
           sizeof(s_lava_erosion_stage_timers));
    memset(s_ts_active, 0, sizeof(s_ts_active));
    memset(s_ts_coordinates, 0, sizeof(s_ts_coordinates));
    memset(s_ts_stamps, 0, sizeof(s_ts_stamps));
    s_lava_erosion_pending_count = 0u;
    s_erosion_trigger_accumulator = 0.0f;
    s_ts_now = 0;
}

// FUNCTION: LEGORR 0x0040e930
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void erosion_timing_state_init(float value_a,
                                                  float value_b,
                                                  float value_c)
{
    __asm {
        fld dword ptr [esp + 8]
        fmul dword ptr [s_erosion_ticks_per_second]
        fld dword ptr [esp + 0ch]
        fld dword ptr [esp + 4]
        fmul dword ptr [s_erosion_ticks_per_second]
        fxch st(2)
        fmul dword ptr [s_erosion_delay_fraction]
        fxch st(1)
        fmul dword ptr [s_erosion_ticks_per_second]
        fxch st(2)
        fstp dword ptr [s_erosion_decay_scale]
        push edi
        xor eax, eax
        fstp dword ptr [s_erosion_delay_scale]
        mov ecx, 7d0h
        mov edi, offset s_erosion_marks
        fstp dword ptr [s_ts_now]
        mov dword ptr [s_lava_erosion_pending_count], eax
        rep stosd
        mov ecx, 3e8h
        mov edi, offset s_ts_active
        rep stosd
        pop edi
        ret
    }
}
#else
void erosion_timing_state_init(float value_a, float value_b, float value_c)
{
    s_erosion_decay_scale = value_a * s_erosion_ticks_per_second;
    s_erosion_delay_scale = value_b * s_erosion_ticks_per_second *
                            s_erosion_delay_fraction;
    const float tick = value_c * s_erosion_ticks_per_second;
    s_lava_erosion_pending_count = 0u;
    memset(s_erosion_marks, 0, sizeof(s_erosion_marks));
    memset(s_ts_active, 0, sizeof(s_ts_active));
    s_erosion_trigger_accumulator = 0.0f;
    __builtin_memcpy(&s_ts_now, &tick, sizeof s_ts_now);
}
#endif

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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int lava_erosion_stage_slot_find_free(int* slot_out)
{
    __asm {
        xor ecx, ecx
        mov eax, offset s_erosion_marks
    lava_erosion_stage_slot_find_free_scan:
        cmp dword ptr [eax], 0
        je lava_erosion_stage_slot_find_free_found
        add eax, 4
        inc ecx
        cmp eax, offset s_lava_erosion_stage_timers
        jb lava_erosion_stage_slot_find_free_scan
        xor eax, eax
        ret
    lava_erosion_stage_slot_find_free_found:
        mov eax, dword ptr [esp + 4]
        mov dword ptr [eax], ecx
        mov eax, 1
        ret
    }
}
#else
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
#endif

// FUNCTION: LEGORR 0x0040e9c0
float lava_erosion_delay_from_cell(const int xy[2])
{
#if defined(_MSC_VER) && defined(_M_IX86)
    LevelState* level = g_active_level_retail;
    const uint8_t* voxels =
        (const uint8_t*)level->word[LEVEL_OFF_VOXELS / 4u];
#else
    LevelState* level = lava_erosion_current_level_get();
    const uint8_t* voxels = level != NULL
        ? (const uint8_t*)level->ptr_voxels : NULL;
#endif
    const uint8_t* record;
    uint32_t width;
    uint32_t index;
    int steps;
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (xy == NULL || level == NULL) return 0.0f;
#endif
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (voxels == NULL) return 0.0f;
#endif
    width = level->word[LEVEL_OFF_WIDTH / 4u];
    index = (uint32_t)xy[1] * width + (uint32_t)xy[0];
    record = voxels + index * LEVEL_VOXEL_STRIDE;
    steps = 6 - (int)record[6];
    return (float)steps * s_erosion_delay_scale *
           s_erosion_ticks_per_second;
}

int lava_erosion_neighbor_check_level(const LevelState* s, int seed_x,
                                     int seed_y, int out_xy[2])
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

/* The retail entry receives seed/output coordinate pairs and reads the
 * active level global. Keep the explicit-level helper above for tests and
 * noncurrent-level queries; the complete target walk lives in this body. */
// FUNCTION: LEGORR 0x0040f000
int lava_erosion_neighbor_check(const int seed_xy[2], int out_xy[2])
{
    LevelState* level = lava_erosion_current_level_get();
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    const uint8_t* seed;
    int direction;
    if (seed_xy == NULL || level == NULL) return 0;
    seed = level_voxel(level, seed_xy[0], seed_xy[1]);
    if (seed == NULL || seed[2] != 6) return 0;
    for (direction = 0; direction < 4; ++direction) {
        const int nx = seed_xy[0] + dx[direction];
        const int ny = seed_xy[1] + dy[direction];
        const uint8_t* record;
        uint32_t flags = 0u;
        int candidate_xy[2];
        if (out_xy != NULL) {
            out_xy[0] = nx;
            out_xy[1] = ny;
        }
        record = level_voxel(level, nx, ny);
        if (record == NULL) continue;
        __builtin_memcpy(&flags, record + 8, sizeof(flags));
        if ((flags & 0x80000000u) != 0u || (flags & 8u) == 0u ||
            record[6] == 0u) continue;
        candidate_xy[0] = nx;
        candidate_xy[1] = ny;
        if (cell_coordinate_timestamp_exists(candidate_xy) == 0u)
            return 1;
    }
    return 0;
}

/* LavaErosionStageRegister (FUN_0040ea10): claim the first inactive stage
 * slot, seed the cell byte and countdown, then append it to the ordered
 * pending-candidate table. */
int lava_erosion_stage_register(const int xy[2], int stage)
{
    int slot;
    uint8_t* cell;
    if (xy == NULL || s_lava_erosion_level_context == NULL) return 0;
    cell = level_voxel(s_lava_erosion_level_context, xy[0], xy[1]);
    if (cell == NULL) return 0;
    if (!lava_erosion_stage_slot_find_free(&slot)) return 0;
    cell[7] = (uint8_t)stage;
    s_lava_erosion_stage_coordinates[slot].x = xy[0];
    s_lava_erosion_stage_coordinates[slot].y = xy[1];
    s_lava_erosion_stage_timers[slot] = lava_erosion_delay_from_cell(xy) /
                                        (float)(stage + 1);
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
    const uint8_t* cell;
    if (slot >= EROSION_MARK_COUNT || s_erosion_marks[slot] == 0u) return 0;
    if (xy_out != NULL) {
        xy_out[0] = s_lava_erosion_stage_coordinates[slot].x;
        xy_out[1] = s_lava_erosion_stage_coordinates[slot].y;
    }
    if (timer_out != NULL) *timer_out = s_lava_erosion_stage_timers[slot];
    cell = level_voxel(s_lava_erosion_level_context,
        s_lava_erosion_stage_coordinates[slot].x,
        s_lava_erosion_stage_coordinates[slot].y);
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

// FUNCTION: LEGORR 0x0040f2b0
int lava_erosion_neighbor_eligible_current(const int xy[2])
{
    LevelState* level;
    if (xy == NULL) return 0;
    level = lava_erosion_current_level_get();
    if (level == NULL) return 0;
    const uint32_t x = (uint32_t)xy[0];
    const uint32_t y = (uint32_t)xy[1];
    const uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    const uint32_t height = level->word[LEVEL_OFF_HEIGHT / 4u];
    const uint32_t index = y * width + x;
    const uint8_t* record;
    uint32_t flags;
    int8_t type;

    /* Retail ABI: one coordinate-pair pointer and the global current level.
     * FUN_00432b50 performs only these upper-bound tests and the bit-3 test;
     * FUN_00432dd0 then reads the selected record without bounds checks. */
    if (!(x < width - 1u && y < height - 1u)) return 0;
    record = (const uint8_t*)level->ptr_voxels +
             index * LEVEL_VOXEL_STRIDE;
    __builtin_memcpy(&flags, record + 8u, sizeof(flags));
    if ((flags & 8u) == 0u || (flags & 0x8000000u) != 0u) return 0;
    type = (int8_t)record[2];
    if (type == 6 || type == 9 || type == 7) return 0;
    if (level->word[LEVEL_OFF_SAFECAVERNS / 4u] != 0u &&
        (flags & 0x4000000u) != 0u) return 0;
    return 1;
}


/* LavaErosionPendingCellAdd (FUN_0040ee50): maintain the compact 2,000-cell
 * candidate list and mirror membership in voxel flag bit 31. Removal uses
 * swap-with-last and retires the matching stage slot, as in the reference. */
// FUNCTION: LEGORR 0x0040ee50
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void lava_erosion_pending_cell_add(const int xy[2], int add)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        push ebx
        push ebp
        push esi
        push edi
        mov edi, dword ptr [g_active_level_retail]
        test eax, eax
        je lava_erosion_pending_cell_add_remove
        mov esi, dword ptr [esp + 14h]
        mov eax, dword ptr [edi + 14h]
        mov ecx, dword ptr [edi + 0b8h]
        imul eax, dword ptr [esi + 4]
        mov ebx, dword ptr [esi]
        add eax, ebx
        lea eax, dword ptr [eax + eax*8]
        test dword ptr [ecx + eax*8 + 8], 80000000h
        jne lava_erosion_pending_cell_add_done
        mov eax, dword ptr [s_lava_erosion_pending_count]
        cmp eax, 7d0h
        jae lava_erosion_pending_cell_add_done
        mov edx, ebx
        push esi
        mov dword ptr [eax*8 + s_lava_erosion_pending_cells], edx
        mov ecx, dword ptr [s_lava_erosion_pending_count]
        mov eax, dword ptr [esi + 4]
        mov dword ptr [ecx*8 + s_lava_erosion_pending_cells + 4], eax
        mov eax, dword ptr [s_lava_erosion_pending_count]
        inc eax
        mov dword ptr [s_lava_erosion_pending_count], eax
        call map_cell_tracking_flags_clear_and_refresh_current
        mov eax, dword ptr [edi + 14h]
        mov edx, dword ptr [esi]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        add esp, 4
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [edi + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        or ecx, 80000000h
        mov dword ptr [eax], ecx
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    lava_erosion_pending_cell_add_remove:
        mov edx, dword ptr [s_lava_erosion_pending_count]
        mov esi, dword ptr [esp + 14h]
        xor eax, eax
        test edx, edx
        jbe lava_erosion_pending_cell_add_stage_scan
        mov ebx, dword ptr [esi]
        mov ecx, offset s_lava_erosion_pending_cells
    lava_erosion_pending_cell_add_list_scan:
        cmp ebx, dword ptr [ecx]
        jne lava_erosion_pending_cell_add_list_next
        mov ebp, dword ptr [esi + 4]
        cmp ebp, dword ptr [ecx + 4]
        je lava_erosion_pending_cell_add_list_found
    lava_erosion_pending_cell_add_list_next:
        inc eax
        add ecx, 8
        cmp eax, edx
        jae lava_erosion_pending_cell_add_stage_scan
        jmp lava_erosion_pending_cell_add_list_scan
    lava_erosion_pending_cell_add_list_found:
        dec edx
        mov dword ptr [s_lava_erosion_pending_count], edx
        lea ecx, dword ptr [edx*8 + s_lava_erosion_pending_cells]
        mov edx, dword ptr [edx*8 + s_lava_erosion_pending_cells]
        mov dword ptr [eax*8 + s_lava_erosion_pending_cells], edx
        mov ecx, dword ptr [ecx + 4]
        mov dword ptr [eax*8 + s_lava_erosion_pending_cells + 4], ecx
        mov eax, dword ptr [edi + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, dword ptr [esi]
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [edi + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 7fffffffh
        mov dword ptr [eax], ecx
    lava_erosion_pending_cell_add_stage_scan:
        mov edx, dword ptr [esi]
        xor ecx, ecx
        mov eax, offset s_lava_erosion_stage_coordinates
    lava_erosion_pending_cell_add_stage_loop:
        cmp edx, dword ptr [eax]
        jne lava_erosion_pending_cell_add_stage_next
        mov edi, dword ptr [esi + 4]
        mov ebx, dword ptr [eax + 4]
        cmp edi, ebx
        je lava_erosion_pending_cell_add_stage_found
    lava_erosion_pending_cell_add_stage_next:
        add eax, 8
        inc ecx
        cmp eax, offset s_erosion_marks
        jae lava_erosion_pending_cell_add_timestamp
        jmp lava_erosion_pending_cell_add_stage_loop
    lava_erosion_pending_cell_add_stage_found:
        mov dword ptr [ecx*4 + s_erosion_marks], 0
    lava_erosion_pending_cell_add_timestamp:
        push esi
        call cell_coordinate_timestamp_remember
        add esp, 4
    lava_erosion_pending_cell_add_done:
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    }
}
#else
void lava_erosion_pending_cell_add(const int xy[2], int add)
{
    LevelState* level = lava_erosion_current_level_get();
    uint8_t* const cell = xy != NULL
        ? level_voxel(level, xy[0], xy[1]) : NULL;
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
                s_lava_erosion_stage_coordinates[i].x == xy[0] &&
                s_lava_erosion_stage_coordinates[i].y == xy[1]) {
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
        /* The retail caller invokes the coordinate-only entry, which reads
         * DAT_005560d4. Use that mapped path whenever this is the live owner;
         * retain the explicit-level adapter only for offline level work. */
        if (terrain_current_level_resolve() == level) {
            map_cell_tracking_flags_clear_and_refresh_current(xy);
        }
        else {
            map_cell_tracking_flags_clear_and_refresh(level, xy);
        }
        __builtin_memcpy(&flags, cell + 8, sizeof flags);
        flags |= 0x80000000u;
        __builtin_memcpy(cell + 8, &flags, sizeof flags);
    }
}
#endif

/* LavaErosionPendingCellTick (FUN_0040eab0): keep the engine-scale trigger
 * gate, walk the pending coordinates in their live insertion/swap order,
 * then advance all active stage timers. */
// FUNCTION: LEGORR 0x0040eab0
void lava_erosion_pending_cell_tick(float scale)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    LevelState* s = g_active_level_retail;
#else
    LevelState* s = lava_erosion_current_level_get();
#endif
    s_erosion_triggered_this_tick = 0;
    /* The retail thunk draws before it tests the interval, so even frames
     * with no pending trigger advance the shared random stream. */
    const int16_t random_start = (int16_t)rng_thunk_draw15();
    s_erosion_trigger_accumulator += scale;
    if (s_erosion_decay_scale < s_erosion_trigger_accumulator) {
        int free_slot;
        const int have_free_slot =
            lava_erosion_stage_slot_find_free(&free_slot);
        unsigned int n;
        s_erosion_triggered_this_tick = 1;
        if (s != NULL && s_lava_erosion_pending_count != 0u &&
            have_free_slot) {
            const unsigned int slot = (unsigned int)free_slot;
            const unsigned int start = (unsigned int)random_start;
            for (n = 0; n < s_lava_erosion_pending_count; ++n) {
                const unsigned int index =
                    (start + n) % s_lava_erosion_pending_count;
                const int seed_xy[2] = {
                    s_lava_erosion_pending_cells[index].x,
                    s_lava_erosion_pending_cells[index].y
                };
                int candidate_xy[2];
                if (lava_erosion_neighbor_check(seed_xy, candidate_xy) != 0) {
                    uint8_t* cell;
#if defined(_MSC_VER) && defined(_M_IX86)
                    const uint32_t cell_index =
                        (uint32_t)candidate_xy[1] *
                        s->word[LEVEL_OFF_WIDTH / 4u] +
                        (uint32_t)candidate_xy[0];
                    cell = (uint8_t*)s->word[LEVEL_OFF_VOXELS / 4u] +
                           cell_index * LEVEL_VOXEL_STRIDE;
#else
                    cell = level_voxel(s, candidate_xy[0], candidate_xy[1]);
                    if (cell == NULL) continue;
#endif
                    if (level_cell_occupied_at_current(candidate_xy) == 0u ||
                        (cell[0x0c] & 0x10u) != 0u) {
                        float delay;
                        uint32_t secondary;
                        __builtin_memcpy(&secondary, cell + 0x0c,
                                         sizeof secondary);
                        secondary &= ~0x10u;
                        __builtin_memcpy(cell + 0x0c, &secondary,
                                         sizeof secondary);
                        s_lava_erosion_stage_coordinates[slot].x =
                            candidate_xy[0];
                        s_lava_erosion_stage_coordinates[slot].y =
                            candidate_xy[1];
                        delay = lava_erosion_delay_from_cell(candidate_xy);
                        s_lava_erosion_stage_timers[slot] = delay;
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
            LavaErosionPendingCell* stage;
            uint8_t* cell;
            int8_t before, after;
            if (s_erosion_marks[slot] == 0u) continue;
            stage = &s_lava_erosion_stage_coordinates[slot];
            before = (int8_t)crt_float_to_long(
                (s_lava_erosion_stage_timers[slot] /
                 lava_erosion_delay_from_cell((const int*)stage)) *
                s_erosion_stage_quanta);
            s_lava_erosion_stage_timers[slot] -= scale;
            after = (int8_t)crt_float_to_long(
                (s_lava_erosion_stage_timers[slot] /
                 lava_erosion_delay_from_cell((const int*)stage)) *
                s_erosion_stage_quanta);
            if (after == before) continue;
 #if defined(_MSC_VER) && defined(_M_IX86)
            {
                const uint32_t cell_index =
                    (uint32_t)stage->y * s->word[LEVEL_OFF_WIDTH / 4u] +
                    (uint32_t)stage->x;
                cell = (uint8_t*)s->word[LEVEL_OFF_VOXELS / 4u] +
                       cell_index * LEVEL_VOXEL_STRIDE;
            }
#else
            cell = level_voxel(s, stage->x, stage->y);
            if (cell == NULL) {
                s_erosion_marks[slot] = 0u;
                continue;
            }
#endif
            cell[7] = (uint8_t)(4 - after);
            if (after == 0) {
                s_erosion_marks[slot] = 0u;
                cell[2] = 6u;
            }
            {
                float position[3];
                float xy[2] = { 0.0f, 0.0f };
                const int position_valid = terrain_cell_world_center_xy_get(
                    s->ptr_terrain, stage->x, stage->y, xy);
                position[0] = xy[0];
                position[1] = xy[1];
                position[2] = position_valid
                    ? terrain_surface_height_interpolate(
                        s->ptr_terrain, xy[0], xy[1]) : 0.0f;
                /* Retail consumes this random vector on every stage-byte
                 * transition, before its switch decides whether the stage
                 * has a visible erosion effect. */
                {
                    float rotation[3];
                    d3drm_vector_random(rotation);
                    if (position_valid && after >= 0 && after <= 3) {
                        lava_effect_select_and_place(after + 4, position,
                                                     rotation);
                    }
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
}

/* The engine adapter keeps a trigger result for its portable test counter;
 * the mapped retail entry above is void and leaves EAX unused. */
int lava_erosion_pending_cell_tick_result(float scale)
{
    lava_erosion_pending_cell_tick(scale);
    return s_erosion_triggered_this_tick;
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
 * claims the effect slot; voxel flags, repeat count, recursive propagation,
 * and CellInitPass stay in the level model. */
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
    LevelState* level = lava_erosion_current_level_get();
    uint8_t* cell;
    uint32_t flags;
    int type;
    int edge;
    int changed = 0;
    if (xy == NULL || level == NULL) return;
    cell = level_voxel(level, xy[0], xy[1]);
    if (cell == NULL) return;
    /* FUN_0040f330 drains the cell transition state through 0x432c90 before
     * clearing its work flag and attempting the directional mutation. */
    while (terrain_cell_state_update(level, xy) == 0) {
    }
    if (s_lava_erosion_work_clear != NULL) {
        s_lava_erosion_work_clear(xy[0], xy[1], 0x2f,
                                  s_lava_erosion_hook_context);
    } else {
        terrain_cell_work_flag_clear(level, xy, 0x2fu);
    }
    /* FUN_0040f330 switches on the complete direction mask.  The values
     * 1..14 map to one of four scene edges and one of two edge endpoints. */
    type = direction < 15u ? (int)direction : -1;
    edge = type < 15 ? edge_for_type[type] : -1;
    /* Retail reaches its repeat-count, propagation and CellInitPass tail for
     * every direction mask. Mask 15 has no recovered switch arm, so retain
     * those CPU effects while leaving only its unresolvable scene edge out. */
    if (edge >= 0) {
        if (s_lava_erosion_edge_apply != NULL) {
            changed = s_lava_erosion_edge_apply(xy[0], xy[1], direction, edge,
                                                 s_lava_erosion_hook_context);
        } else {
            float position[2];
            if (terrain_cell_world_center_xy_get(
                    level->ptr_terrain,
                    xy[0], xy[1], position)) {
                const float height = terrain_surface_height_interpolate(
                    level->ptr_terrain,
                    position[0], position[1]);
                changed = scene_slot_pose_scale_register(
                    group_for_type[type], (uint32_t)xy[0], (uint32_t)xy[1],
                    position[0], position[1], height, 0.0f, 0.0f);
            }
        }
    }
    if (changed != 0) {
        __builtin_memcpy(&flags, cell + 8, sizeof flags);
        flags |= 0x800u | 0x1000u;
        __builtin_memcpy(cell + 8, &flags, sizeof flags);
        level_tile_mark(level, xy[0], xy[1], 1);
        if (s_lava_erosion_cell_change != NULL) {
            s_lava_erosion_cell_change(xy[0], xy[1], 1,
                                       s_lava_erosion_hook_context);
        }
        if (s_lava_erosion_sound != NULL) {
            s_lava_erosion_sound(xy[0], xy[1], s_lava_erosion_hook_context);
        } else {
            float position[3];
            float cell_xy[2];
            if (terrain_cell_world_center_xy_get(
                    level->ptr_terrain,
                    xy[0], xy[1], cell_xy)) {
                position[0] = cell_xy[0];
                position[1] = cell_xy[1];
                position[2] = terrain_surface_height_interpolate(
                    level->ptr_terrain,
                    cell_xy[0], cell_xy[1]);
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
    /* The retail routine ends its CPU mutation path with CellInitPass; it
     * does not reset the surface type or set the terrain block dirty bit. */
    cell_init_pass(level, (float)xy[0], (uint32_t)xy[1]);
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
    LevelState* level = lava_erosion_current_level_get();
    int valid = 1;
    int clear_mask_neighbors = 0;
    int i;
    if (xy == NULL || lava_erosion_neighbor_eligible_current(xy) == 0) {
        return 0;
    }
    for (i = 0; i < 4; ++i) {
        const int nx = xy[0] + dx[i];
        const int ny = xy[1] + dy[i];
        if (level_cell_walkable_flag(level, nx, ny)) {
            if (level_cell_state_is_1(level, nx, ny) ||
                level_cell_flag_20_bounded(level, nx, ny) != 0u) {
                valid = 0;
            }
            mask |= 1u << i;
            if (level_cell_mask_c0(level, nx, ny) == 0u) {
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
    LevelState* level = lava_erosion_current_level_get();
    int i;
    if (xy == NULL || level == NULL) return;
    for (i = 0; i < 8; ++i) {
        int neighbor[2];
        neighbor[0] = xy[0] + dx[i];
        neighbor[1] = xy[1] + dy[i];
        if (level_voxel(level, neighbor[0], neighbor[1]) != NULL) {
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
 * in slot 0 reads the same as "no cardinal neighbour" (pinned quirk).
 * The mapped x86 entry uses the retail six-argument ABI and reads cell
 * records directly from the active level's +0xb8 voxel pointer. Its world
 * position check uses the port's entity-position adapter, whose explicit
 * level argument and return contract do not reproduce the renderer-owned
 * scene-position call exactly. The separate explicit-level helper retains
 * bounds and NULL guards for callers that operate on arbitrary levels.
 *
 * FirstFreeNeighborCellPick (FUN_00431c70) walks the four cardinal
 * neighbours in a rotation that advances by the corpus's DAT_004de588
 * pick counter (own storage here; peek hook), calling the route check
 * with source and destination pinned to the neighbour cell. In mode 1 a
 * neighbour that is in bounds, open (flag 0x10) and unblocked (flag 8
 * clear) is skipped; otherwise it is tried. The mapped x86 entry uses the
 * retail stack delta table and reloads the active-level global after each
 * route check. The portable fallback uses the guarded sidecar APIs. */
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
            /* Retail resolves the entity's world position through the active
             * terrain grid picker before applying this source-cell fast path. */
            int entity_cell_x = -1;
            int entity_cell_y = -1;
            if (s != NULL && s->ptr_terrain != NULL) {
                entity_position_to_terrain_cell(entity, s, &entity_cell_x,
                                                &entity_cell_y);
                if (entity_cell_x == from_x && entity_cell_y == from_y)
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
        if ((flags & 0x10u) != 0u) {
            const int source_type = route_cell_byte(s, from_x, from_y, 0x02);
            const int above = route_cell_byte(s, from_x, from_y - 1, 0x02);
            if (source_type == 6 || above == 6 ||
                source_type == 7 || above == 7) {
                return 0; /* source/above wall types block the open-cell route */
            }
        }
        if (delta_index == 0) {
            if ((flags & 0x2000u) != 0u ||
                (route_cell_flags(s, to_x, to_y) & 0x2000u) != 0u) {
                answer = 2;
            }
        }
    }

    if (to_x != from_x && to_y != from_y) {
        /* diagonal corner pair along the destination row */
        if ((route_cell_flags(s, from_x, to_y) & 8u) != 0u
            && (route_cell_flags(s, to_x, from_y) & 8u) != 0u) {
            return 2;
        }
    }
    return answer;
}

// FUNCTION: LEGORR 0x00431da0
int terrain_cell_route_and_occupancy_check_current(
    const struct EntitySlot* entity, int from_x, int from_y,
    int to_x, int to_y, int strict)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    int local_20[8];
    int delta_index;
    int delta_slot;
    int entity_cell_x;
    int entity_cell_y;
    uint32_t width;
    uint32_t source_index;
    uint32_t dest_index;
    uint32_t flags;
    uint32_t block_relief;
    uint32_t special;
    uint32_t idle_gate;
    uint32_t walk_gate;
    uint32_t state_mask;
    uint32_t answer;
    uint8_t* voxels;
    uint8_t* source;
    uint8_t* dest;
    LevelState* level;

    level = g_active_level_retail;
    width = level->word[LEVEL_OFF_WIDTH / 4u];
    voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4u];
    source_index = (uint32_t)from_y * width + (uint32_t)from_x;
    source = voxels + source_index * 0x48u;
    dest_index = (uint32_t)to_y * width + (uint32_t)to_x;
    dest = voxels + dest_index * 0x48u;
    special = 0u;
    walk_gate = 1u;
    idle_gate = 0u;
    state_mask = 0u;
    answer = 1u;
    local_20[0] = 0;
    local_20[3] = 0;
    local_20[1] = -1;
    local_20[2] = 1;
    local_20[4] = 0;
    local_20[5] = 1;
    local_20[6] = -1;
    local_20[7] = 0;
    if (entity != NULL &&
        (entity_stats_word_get(entity) & 0x20000u) != 0u &&
        entity->owner_36c == NULL) {
        return 0;
    }
    delta_slot = 0;
    delta_index = 0;
    for (;;) {
        if (local_20[delta_slot * 2] + to_x == from_x &&
            to_y + local_20[delta_slot * 2 + 1] == from_y) {
            break;
        }
        ++delta_slot;
        ++delta_index;
        if (delta_slot >= 4)
            break;
    }
    if (entity != NULL) {
        if (entity->type == 0x0b)
            return 0;
        block_relief = entity_capability_flags_get(entity) & 0x200u;
        special = entity_stats_word_get(entity) & 1u;
        idle_gate = entity_stats_word_get(entity) & 0x1000000u;
        walk_gate = entity_stats_word_get(entity) & 0x2000000u;
        state_mask = entity_state_flags_get(entity) & 0x200000u;
        if (strict == 0) {
            entity_cell_x = 0;
            entity_cell_y = 0;
            entity_position_to_terrain_cell(entity, level, &entity_cell_x,
                                            &entity_cell_y);
            if (from_x == entity_cell_x && from_y == entity_cell_y)
                return 1;
        }
    }
    else {
        block_relief = 0u;
    }

    if (source[2] == 9) {
        if (idle_gate == 0u)
            return 0;
    }
    else if (walk_gate == 0u) {
        return 0;
    }
    if (source[2] == 6 && state_mask == 0u &&
        (*(uint32_t*)(source + 8) & 0x200u) == 0u) {
        return 0;
    }
    flags = *(uint32_t*)(source + 8);
    if ((flags & 0x400u) != 0u)
        return 0;
    if (strict != 0 && (flags & 0x40u) != 0u)
        return 0;
    if (block_relief == 0u && (source[0x0c] & 4u) != 0u)
        return 0;
    if (special == 0u) {
        if ((flags & 8u) == 0u)
            return 0;
    }
    else {
        if ((flags & 0x10u) != 0u)
            answer = 3u;
        if (source[0] == 0u)
            return 0;
        if ((flags & 0x10u) != 0u) {
            uint32_t above_index = ((uint32_t)from_y - 1u) * width +
                                   (uint32_t)from_x;
            uint8_t* above = voxels + above_index * 0x48u;
            if (source[2] == 6 || above[2] == 6)
                return 0;
            if (source[2] == 7 || above[2] == 7)
                return 0;
        }
        if (delta_index == 0) {
            if ((flags & 0x2000u) != 0u ||
                (*(uint32_t*)(dest + 8) & 0x2000u) != 0u) {
                answer = 2u;
            }
        }
    }
    if (to_x != from_x && to_y != from_y) {
        uint32_t cross_a = (uint32_t)to_y * width + (uint32_t)from_x;
        uint32_t cross_b = (uint32_t)from_y * width + (uint32_t)to_x;
        uint8_t* cell_a = voxels + cross_a * 0x48u;
        uint8_t* cell_b = voxels + cross_b * 0x48u;
        if ((*(uint32_t*)(cell_a + 8) & 8u) != 0u &&
            (*(uint32_t*)(cell_b + 8) & 8u) != 0u) {
            return 2;
        }
    }
    return (int)answer;
#else
    return terrain_cell_route_and_occupancy_check(
        entity, level_current_owner_get(), from_x, from_y, to_x, to_y,
        strict);
#endif
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
        if (mode != 0
            && (unsigned)nx < (unsigned)level_width(s) - 1u
            && (unsigned)ny < (unsigned)level_height(s) - 1u) {
            const uint32_t flags = route_cell_flags(s, nx, ny);
            if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
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

// FUNCTION: LEGORR 0x00431c70
int first_free_neighbor_cell_pick_current(const struct EntitySlot* entity,
                                          const int* cell_in,
                                          int* cell_out, int mode)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    int local_20[8];
    uint32_t local_24;
    uint32_t direction;
    uint32_t width;
    uint32_t height;
    uint32_t index;
    uint32_t flags;
    int x;
    int y;
    int result;
    LevelState* level;
    uint8_t* voxels;

    local_20[1] = -1;
    local_20[6] = -1;
    ++s_neighbor_pick_rotation;
    local_20[0] = 0;
    local_20[2] = 1;
    local_20[3] = 0;
    local_20[4] = 0;
    local_20[5] = 1;
    local_20[7] = 0;
    local_24 = 0u;
    level = g_active_level_retail;
    do {
        direction = (s_neighbor_pick_rotation + local_24) & 3u;
        x = local_20[direction * 2u] + cell_in[0];
        y = local_20[direction * 2u + 1u] + cell_in[1];
        if (mode != 0) {
            width = level->word[LEVEL_OFF_WIDTH / 4u];
            height = level->word[LEVEL_OFF_HEIGHT / 4u];
            if ((uint32_t)x < width - 1u &&
                (uint32_t)y < height - 1u) {
                voxels = (uint8_t*)(uintptr_t)
                    level->word[LEVEL_OFF_VOXELS / 4u];
                index = (uint32_t)y * width + (uint32_t)x;
                flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
                if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
                    ++local_24;
                    if (local_24 > 3u)
                        return 0;
                    continue;
                }
            }
        }
        result = terrain_cell_route_and_occupancy_check_current(
            entity, x, y, x, y, 0);
        level = g_active_level_retail;
        if (result != 0) {
            cell_out[0] = x;
            cell_out[1] = y;
            return 1;
        }
        ++local_24;
        if (local_24 > 3u)
            return 0;
    } while (1);
#else
    LevelState* level;
    int probe;

    /* Keep the retail entry's four-neighbor walk in this mapped body. The
     * reference increments DAT_004de588 once per call, then indexes the
     * cardinal table with (counter + probe) & 3. */
    ++s_neighbor_pick_rotation;
    level = level_current_owner_get();
    for (probe = 0; probe < 4; ++probe) {
        const uint32_t direction =
            (s_neighbor_pick_rotation + (uint32_t)probe) & 3u;
        const int x = cell_in[0] + s_cardinal_deltas[direction][0];
        const int y = cell_in[1] + s_cardinal_deltas[direction][1];

        if (mode != 0 && level != NULL &&
            (unsigned)x < (unsigned)level_width(level) - 1u &&
            (unsigned)y < (unsigned)level_height(level) - 1u) {
            const uint32_t flags = route_cell_flags(level, x, y);
            if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
                continue;
            }
        }

        /* FUN_00431da0 resolves DAT_005560d4 itself, so reacquire the active
         * level for every probe just as the retail caller does. */
        if (terrain_cell_route_and_occupancy_check(
                entity, level_current_owner_get(), x, y, x, y, 0) != 0) {
            cell_out[0] = x;
            cell_out[1] = y;
            return 1;
        }
    }
    return 0;
#endif
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

namespace unported_ledger_stubs {

/* Adapter only: SurfaceTypeDescriptionsConfigLoad (FUN_004350a0) takes the
 * config and section owners, then writes the original global tables. The
 * caller-supplied entries/hooks seam does not reproduce those owners. */
void ledger_stub_004350a0_SurfaceTypeDescriptionsConfigLoad(
    SurfaceTypeDescriptionEntry entries[18],
    const SurfaceTypeDescriptionHooks* hooks)
{
    surface_type_descriptions_load(entries, hooks);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// TaskCountAtGridCoordinateQuery
int ledger_stub_0043b360_TaskCountAtGridCoordinateQuery(
    uint32_t x, uint32_t y, EntitySlot** out_entity)
{
    return entity_task_at_grid_coordinate_find(x, y, out_entity);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

/* ExpandCellEntitySpawn (FUN_00466250) creates a type-12 entity only when
 * the cell's expansion bit is clear and the surrounding terrain flags allow
 * its orientation. The scene action setter itself remains renderer-owned;
 * entity creation, cell state, and the cell object's live sidecar are
 * handled here. */
// FUNCTION: LEGORR 0x00466250
int ledger_stub_00466250_ExpandCellEntitySpawn(int x, int y)
{
    static const float half_pi = 1.57079632679f;
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    uint8_t* cell;
    uint32_t flags;
    uint32_t entity_word;
    float position[3];
    float orientation;
    EntitySlot* entity;

    if (level == NULL) level = level_current_owner_get();
    if (game == NULL || level == NULL || level->ptr_terrain == NULL)
        return 0;
    if ((unsigned)x >= (unsigned)level_width(level) ||
        (unsigned)y >= (unsigned)level_height(level)) return 0;
    map_cell_record_grid_activate(level);
    cell = cell_record_unchecked(level, x, y);
    if (cell == NULL) return 0;
    memcpy(&flags, cell + 0x08, sizeof(flags));
    if ((flags & 0x100u) != 0u || (flags & 8u) == 0u) return 0;

    /* FUN_004663f0 first tests the two horizontal neighbors. If either
     * lacks bit 0x10, both vertical neighbors must carry it and orientation
     * is zero; otherwise the expansion faces at pi/2. */
    if ((cell_flag_word(level, x + 1, y) & 0x10u) == 0u ||
        (cell_flag_word(level, x - 1, y) & 0x10u) == 0u) {
        if ((cell_flag_word(level, x, y + 1) & 0x10u) == 0u ||
            (cell_flag_word(level, x, y - 1) & 0x10u) == 0u) return 0;
        orientation = 0.0f;
    }
    else {
        orientation = half_pi;
    }
    if (!terrain_cell_world_position_get(x, y, position)) return 0;

    entity = entity_create_with_position(game, 0x0c, NULL, 0,
        position[0], position[1], orientation);
    if (entity == NULL) return 0;

    entity_word = (uint32_t)(uintptr_t)entity;
    memcpy(cell, &entity_word, sizeof(entity_word));
    {
        const float expansion_time = 100.0f;
        memcpy(cell + 0x04, &expansion_time, sizeof(expansion_time));
    }
    flags |= 0x100u;
    memcpy(cell + 0x08, &flags, sizeof(flags));
    entity->flags_3e0 |= 0x100u;
    /* The native owner keeps the full pointer in its cell-link sidecar; the
     * retail record word above remains the 32-bit image used by x86 callers. */
    (void)grid_cell_object_link_insert(entity, x, y);
    return 1;
}

} // namespace unported_ledger_stubs

/* CountMatchingCoordinatesInList (FUN_00408e10) walks the member pointer
 * array at +0x2c for the independent +0x20 comparison count. +0xcc is the
 * pointer append count used by entity teardown; append keeps the two equal,
 * while the direct helper follows +0x20 as retail does. */
uint32_t path_record_count_matching_coordinates(const void* record,
                                                int type, int row)
{
    uint32_t count, index, matches = 0u;
    if (record == NULL) return 0u;
    memcpy(&count, (const char*)record + 0x20, sizeof(count));
    for (index = 0u; index < count; ++index) {
        const EntitySlot* entity = path_record_entity_get(record, index);
        if (entity != NULL && entity->type == type && entity->etype == row)
            ++matches;
    }
    return matches;
}

// FUNCTION: LEGORR 0x00408d70
int terrain_descriptor_coordinate_count_limit_check(
    uint32_t descriptor_key, int type, int row)
{
    uint8_t* record = (uint8_t*)path_record_last_peek();
    uint32_t descriptor_flags;
    uint32_t descriptor_value;
    uint32_t matching_count;
    uint32_t limit;
    int selector;

    /* TerrainDescriptorFindExact(0, &key) walks the path-record chain and
     * compares the key against record +8.  The port stores that same chain
     * through the widened +0x24 link. */
    while (record != NULL) {
        uint32_t key;
        memcpy(&key, record + 0x08, sizeof(key));
        if (key == descriptor_key) break;
        record = (uint8_t*)*(void**)(record + 0x24);
    }
    if (record == NULL) return 0;

    /* Retail's 0x408e10 walks the +0x2c pointer list for +0x20 entries;
     * each pointed-to entity begins with the type/row pair. The port keeps
     * the live pointer list in its entity sidecar on non-32-bit builds, but
     * maintains the retail +0x20 count alongside +0xcc in append(). */
    matching_count = path_record_count_matching_coordinates(record, type,
                                                             row);
    memcpy(&descriptor_value, record, sizeof(descriptor_value));
    memcpy(&descriptor_flags, record + 0xd0, sizeof(descriptor_flags));
    /* Retail's masked-negation expression yields 15 when bit 1 is set. */
    selector = (descriptor_flags & 2u) != 0u ? 15 : 4;
    if (type == 6) {
        limit = entity_indexed_record_word_at_b4_get(
            selector, (int)descriptor_value, 0);
    }
    else if (type == 7) {
        if (row == 0) {
            limit = entity_indexed_record_word_at_b0_get(
                selector, (int)descriptor_value, 0);
        }
        else {
            limit = stats_table_word_at_b8(selector,
                                           (int)descriptor_value, 0);
        }
    }
    else {
        return 1;
    }
    return matching_count < limit;
}

namespace unported_ledger_stubs {

int ledger_stub_00408d70_TerrainDescriptorCoordinateCountLimitCheck(
    uint32_t descriptor_key, int type, int row)
{
    return terrain_descriptor_coordinate_count_limit_check(
        descriptor_key, type, row);
}

} // namespace unported_ledger_stubs

uint32_t terrain_kind_nine_neighbor_emit_for_footprint_cells(
    const int cell_xy[2], const int* footprint_xy, uint32_t footprint_count)
{
    static const int neighbor_xy[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = context != NULL ? context->level : NULL;
    uint8_t* descriptor = (uint8_t*)path_record_last_peek();
    uint32_t emitted = 0u;
    float scale;
    float edge;
    float command_offsets[4][2];
    uint32_t segment;

    if (level == NULL) level = level_current_owner_get();
    if (cell_xy == NULL || footprint_xy == NULL || footprint_count == 0u ||
        level == NULL || level->ptr_terrain == NULL) return 0u;

    /* TerrainDescriptorFindExact(cell_xy, 0): records are head-linked at
     * +0x24 and key the requested cell at +0x0c/+0x10. */
    while (descriptor != NULL) {
        int descriptor_x, descriptor_y;
        memcpy(&descriptor_x, descriptor + 0x0c, sizeof(descriptor_x));
        memcpy(&descriptor_y, descriptor + 0x10, sizeof(descriptor_y));
        if (descriptor_x == cell_xy[0] && descriptor_y == cell_xy[1]) break;
        descriptor = (uint8_t*)*(void**)(descriptor + 0x24);
    }
    if (descriptor == NULL) return 0u;

    /* The original obtains the terrain handle from the current level and
     * uses its +0x10 block size to place the four edge command anchors. */
    scale = terrain_blocksize_get(level->ptr_terrain);
    if (scale == 0.0f) return 0u;
    edge = 8.8f / scale;
    command_offsets[0][0] = 0.5f;
    command_offsets[0][1] = 1.0f - edge;
    command_offsets[1][0] = 1.0f - edge;
    command_offsets[1][1] = 0.5f;
    command_offsets[2][0] = 0.5f;
    command_offsets[2][1] = edge;
    command_offsets[3][0] = edge;
    command_offsets[3][1] = 0.5f;

    for (segment = 0u; segment < footprint_count; ++segment) {
        const int* point = footprint_xy + segment * 2u;
        int is_duplicate = segment + 1u < footprint_count &&
            point[0] == point[2] && point[1] == point[3];
        int direction;
        if (is_duplicate) {
            /* Duplicate consecutive coordinates delimit the same interior
             * pair in the corpus loop; it advances over both entries. */
            ++segment;
            continue;
        }

        for (direction = 0; direction < 4; ++direction) {
            int neighbor_x = point[0] + neighbor_xy[direction][0];
            int neighbor_y = point[1] + neighbor_xy[direction][1];
            uint32_t candidate;
            int inside = 0;
            for (candidate = 0u; candidate < footprint_count;) {
                const int* candidate_xy = footprint_xy + candidate * 2u;
                if (candidate + 1u < footprint_count &&
                    candidate_xy[0] == candidate_xy[2] &&
                    candidate_xy[1] == candidate_xy[3]) {
                    /* The corpus's paired-coordinate walk advances past a
                     * duplicate pair without considering either entry. */
                    candidate += 2u;
                    continue;
                }
                if (candidate_xy[0] == neighbor_x &&
                    candidate_xy[1] == neighbor_y) {
                    inside = 1;
                    break;
                }
                ++candidate;
            }
            if (!inside) {
                uint32_t descriptor_key;
                memcpy(&descriptor_key, descriptor + 0x08,
                       sizeof(descriptor_key));
                cmd_command_kind_record_issue(
                    9u, 0u, 0u, point, descriptor_key,
                    command_offsets[direction], 1);
                ++(*(uint32_t*)(descriptor + 0x1c));
                ++emitted;
            }
        }
    }
    return emitted;
}

// FUNCTION: LEGORR 0x00408ec0
void terrain_kind_nine_neighbor_emit_for_footprint_cells_current(
    const int cell_xy[2], const int* footprint_xy, uint32_t footprint_count)
{
    (void)terrain_kind_nine_neighbor_emit_for_footprint_cells(
        cell_xy, footprint_xy, footprint_count);
}

namespace unported_ledger_stubs {

uint32_t ledger_stub_00408ec0_KindNineNeighborEmitForFootprintCells(
    const int cell_xy[2], const int* footprint_xy, uint32_t footprint_count)
{
    return terrain_kind_nine_neighbor_emit_for_footprint_cells(
        cell_xy, footprint_xy, footprint_count);
}

// FUNCTION: LEGORR 0x004093b0
static void placement_cell_search_expand(const int cell_xy[2])
{
    static const int offset[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = context != NULL ? context->level : NULL;
    int direction;
    if (level == NULL) level = level_current_owner_get();
    if (level == NULL || cell_xy == NULL) return;
    for (direction = 0; direction < 4; ++direction) {
        int next_xy[2];
        int nx = cell_xy[0] + offset[direction][0];
        int ny = cell_xy[1] + offset[direction][1];
        if (!level_cell_occupied_at(level, nx, ny) &&
            !level_cell_occupied_or_flag(level, nx, ny, 1)) continue;
        if (level_cell_secondary_flag_100(level, nx, ny) != 0u) continue;
        terrain_cell_flag100_set_and_track(level, nx, ny);
        next_xy[0] = nx;
        next_xy[1] = ny;
        placement_cell_search_expand(next_xy);
    }
}
void ledger_stub_004093b0_PassableAdjacentCellSearchExpand(
    const int cell_xy[2])
{
    placement_cell_search_expand(cell_xy);
}

int ledger_stub_00409470_NeighborCellEntitySearchRecurse(
    const int cell_xy[2], int resource_amount);
// FUNCTION: LEGORR 0x00409450
int ledger_stub_00409450_FindNearbyPlacementCandidate(
    const int start_xy[2], int resource_amount)
{
    int found = ledger_stub_00409470_NeighborCellEntitySearchRecurse(
        start_xy, resource_amount);
    terrain_cell_visited_flags_clear_all_current();
    return found;
}

// NeighborCellEntitySearchRecurse
// FUNCTION: LEGORR 0x00409470
int ledger_stub_00409470_NeighborCellEntitySearchRecurse(
    const int cell_xy[2], int resource_amount)
{
    static const int offset[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = context != NULL ? context->level : NULL;
    int direction;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL || cell_xy == NULL) return 0;
    for (direction = 0; direction < 4; ++direction) {
        int cell[2];
        EntitySlot* found;
        cell[0] = cell_xy[0] + offset[direction][0];
        cell[1] = cell_xy[1] + offset[direction][1];
        if (level_cell_secondary_bit0(level, cell[0], cell[1]) != 0u) {
            found = entity_at_grid_position_find(cell);
            if (found != NULL && entity_available_resource_spend(
                    level, resource_amount, found)) return 1;
        }
        if (!level_cell_occupied_at(level, cell[0], cell[1]) &&
            !level_cell_occupied_or_flag(level, cell[0], cell[1], 1)) continue;
        if (level_cell_visited_flag(level, cell[0], cell[1]) != 0u) continue;
        terrain_cell_visited_flag_set_and_track(level, cell[0], cell[1]);
        if (ledger_stub_00409470_NeighborCellEntitySearchRecurse(
                cell, resource_amount)) return 1;
    }
    return 0;
}

int nearby_placement_entity_search_current(const int cell_xy[2],
                                           int resource_amount)
{
    return ledger_stub_00409470_NeighborCellEntitySearchRecurse(
        cell_xy, resource_amount);
}

// FUNCTION: LEGORR 0x00409600
void ledger_stub_00409600_TerrainDescriptorTilesResetAndFree(
    const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    uint8_t* descriptor;
    int* coordinates;
    uint32_t count;
    uint32_t entity_count;
    uint32_t entity_index;
    uint32_t progress;
    uint32_t descriptor_key;
    if (cell_xy == NULL || level == NULL) return;

    /* This is the target's exact pair-key descriptor lookup. */
    descriptor = (uint8_t*)terrain_descriptor_find_exact_current(
        (const uint32_t*)cell_xy, NULL);
    if (descriptor == NULL) return;
    memcpy(&progress, descriptor + 0x1c, sizeof(progress));
    if (progress == *(uint32_t*)(descriptor + 0x20)) return;
    coordinates = (int*)path_record_payload_get(descriptor);
    count = path_record_payload_count_get(descriptor);
    memcpy(&descriptor_key, descriptor + 0x08, sizeof(descriptor_key));
    if (count != 0u && coordinates == NULL) return;

    progress = 0u;
    while (progress < count) {
        int x = coordinates[progress * 2u];
        int y = coordinates[progress * 2u + 1u];
        int duplicate = progress + 1u < count &&
            x == coordinates[(progress + 1u) * 2u] &&
            y == coordinates[(progress + 1u) * 2u + 1u];
        if (!level_cell_path_record_state_apply(
                level, x, y, NULL, 0, duplicate))
            level_cell_init_pass(level, x, y);
        /* FUN_0041cfb0 follows CellInitPass for every listed pair, even if
         * the portable cell-link adapter could not represent that cell. */
        cancel_action_for_matching_coordinates(
            (const uint32_t*)coordinates + progress * 2u);
        ++progress;
    }

    entity_count = path_record_entity_count_get(descriptor);
    for (entity_index = 0u; entity_index < entity_count; ++entity_index) {
        EntitySlot* entity = path_record_entity_get(descriptor, entity_index);
        if (entity == NULL) continue;
        if (entity->type == 9) {
            entity->flags_3e8 |= 0x800000u;
            entity->field_340 = -1.0f;
        }
        else {
            entity->flags_3ec &= ~0x100u;
            cmd_create_task(entity);
        }
    }

    cmd_cancel_by_coords((int)descriptor_key, 0, 0, 0);
    path_record_release(descriptor);
}

// CellPathRecordRegisterApply
// FUNCTION: LEGORR 0x00409790
uint32_t ledger_stub_00409790_CellPathRecordRegisterApply(
    uint32_t record_key, const int cell_xy[2], uint32_t direction,
    const int* footprint_xy, uint32_t footprint_count)
{
    LevelState* level = level_current_owner_get();
    void* record;
    int* coordinate_copy = NULL;
    uint32_t index = 0u;
    if (cell_xy == NULL ||
        (footprint_count != 0u && footprint_xy == NULL) ||
        (size_t)footprint_count > ((size_t)-1) / (2u * sizeof(int)))
        return 0u;
    if (level == NULL) return 0u;

    record = path_record_allocate_and_link((const uint32_t*)cell_xy);
    if (record == NULL) return 0u;
    memcpy((char*)record + 0x00, &record_key, sizeof(record_key));
    memcpy((char*)record + 0x04, &direction, sizeof(direction));
    if (footprint_count != 0u) {
        coordinate_copy = (int*)engine_heap_alloc((long)(
            (size_t)footprint_count * 2u * sizeof(int)));
        if (coordinate_copy == NULL) {
            path_record_release(record);
            return 0u;
        }
        memcpy(coordinate_copy, footprint_xy,
               (size_t)footprint_count * 2u * sizeof(int));
    }
    if (!path_record_payload_set(record, coordinate_copy, footprint_count)) {
        engine_heap_free(coordinate_copy);
        path_record_release(record);
        return 0u;
    }
    if (footprint_count != 0u) {
        /* FUN_00409790 passes a four-unit clamp width. */
        terrain_cell_surface_height_clamp(
            level_surface_grid_get(level),
            coordinate_copy, footprint_count, 4.0f);
    }

    while (index < footprint_count) {
        int x = coordinate_copy[index * 2u];
        int y = coordinate_copy[index * 2u + 1u];
        int duplicate = index + 1u < footprint_count &&
            x == coordinate_copy[(index + 1u) * 2u] &&
            y == coordinate_copy[(index + 1u) * 2u + 1u];
        if (level != NULL) {
            if (!level_cell_path_record_state_apply(
                    level, x, y, record, 1, duplicate))
                level_cell_init_pass(level, x, y);
            terrain_cell_refresh_eight_neighbors_current(
                coordinate_copy + index * 2u);
            cancel_action_for_matching_coordinates(
                (const uint32_t*)coordinate_copy + index * 2u);
        }
        /* CellPathRecordRegisterApply advances over a repeated adjacent
         * coordinate as one logical pair even when the cell could not be
         * updated. The decompile increments its list cursor in the duplicate
         * branch before the common increment below. */
        if (duplicate) ++index;
        ++index;
    }
    return *(uint32_t*)((char*)record + 0x08);
}

// FUNCTION: LEGORR 0x00409940
void terrain_cell_refresh_eight_neighbors_current(const int cell_xy[2])
{
    /* The retail local_40 table is ordered NW, N, NE, W, E, SW, S, SE.
     * Keep the walk in the mapped body: the reference calls the mutation
     * worker once for every offset and lets that worker apply edge checks. */
    static const int offset[8][2] = {
        { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 },
        { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
    };
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    int index;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL || cell_xy == NULL) return;
    for (index = 0; index < 8; ++index) {
        const int64_t x = (int64_t)cell_xy[0] + offset[index][0];
        const int64_t y = (int64_t)cell_xy[1] + offset[index][1];
        if (x < 0 || y < 0 || x > 0x7fffffff || y > 0x7fffffff)
            continue;
        terrain_cell_mutation_progress(level, (uint32_t)x, (uint32_t)y);
    }
}

static int entity_path_footprint_record_get(const EntitySlot* entity,
                                            const TypeConfigTable** table_out,
                                            const uint32_t** entries_out,
                                            uint32_t* count_out,
                                            uint32_t* token_out)
{
    const TypeConfigTable* table;
    const uint8_t* row_data;
    const uint32_t* entries;
    uint32_t row_kind, row, shape_token, count;
    if (table_out != NULL) *table_out = NULL;
    if (entries_out != NULL) *entries_out = NULL;
    if (count_out != NULL) *count_out = 0u;
    if (token_out != NULL) *token_out = 0u;
    /* Retail indexes DAT_005561e0 directly with the dword at entity +4.
     * EntitySlot::etype is that row index; resolving type_name here changes
     * behavior for renamed/aliased objects and needlessly depends on the
     * name table being loaded. */
    if (entity == NULL || entity->etype < 0) return 0;
    row_kind = 2u; /* DAT_005561e0 is the BuildingTypes table. */
    table = entity_type_config_table_get(row_kind);
    row = (uint32_t)entity->etype;
    if (table == NULL || table->rows == NULL || table->row_stride < 0x8cu ||
        row >= table->count) return 0;
    row_data = (const uint8_t*)table->rows + (size_t)row * table->row_stride;
    memcpy(&shape_token, row_data + 0x84u, sizeof(shape_token));
    memcpy(&count, row_data + 0x88u, sizeof(count));
    entries = type_config_shape_table_resolve(table, shape_token);
    if (entries == NULL || count == 0u || count > 10u) return 0;
    if (table_out != NULL) *table_out = table;
    if (entries_out != NULL) *entries_out = entries;
    if (count_out != NULL) *count_out = count;
    if (token_out != NULL) *token_out = shape_token;
    return 1;
}

static int entity_path_footprint_cell_get(const TypeConfigTable* table,
                                          const uint32_t* entries,
                                          uint32_t shape_token,
                                          uint32_t index, int cell_xy[2])
{
    const char* x_text;
    const char* y_text;
    uint32_t x_token, y_token;
    if (table == NULL || entries == NULL || cell_xy == NULL) return 0;
    /* The type parser reserves pair zero as the entity's anchor; subsequent
     * pairs are string tokens in the BuildingTypes Shape record. */
    if (index == 0u) {
        cell_xy[0] = 0;
        cell_xy[1] = 0;
        return 1;
    }
    x_token = entries[index * 2u];
    y_token = entries[index * 2u + 1u];
    x_text = type_config_shape_string_resolve(table, shape_token, x_token);
    y_text = type_config_shape_string_resolve(table, shape_token, y_token);
    if (x_text == NULL || y_text == NULL) return 0;
    cell_xy[0] = atoi(x_text);
    cell_xy[1] = atoi(y_text);
    return 1;
}

static void entity_cell_path_effects_apply_level(EntitySlot* entity,
                                                  LevelState* level)
{
    const TypeConfigTable* table;
    const uint32_t* footprint;
    uint32_t count;
    uint32_t shape_token;
    int source_cell[2];
    int direction;
    float direction_xy[2];
    void* component;
    uint32_t i;
    if (entity == NULL || level == NULL ||
        !entity_path_footprint_record_get(entity, &table, &footprint,
                                          &count, &shape_token)) {
        entity_placement_commands_build_once(entity);
        return;
    }
    entity_position_to_terrain_cell(entity, level, &source_cell[0],
                                    &source_cell[1]);
    /* Retail 0x4422a0 reads the type component via 0x4a200, then asks its
     * renderer surface for the normalized orientation vector. The host
     * component handle has no production writer yet; visual-resource cache
     * entries and velocity are not proven substitutes. */
    component = entity->type_component_handle;
    if (!entity_surface_vector_normalize_if_available(component,
                                                       direction_xy)) {
        /* The scene surface-vector provider is not installed in this build.
         * Preserve the decompiled final command setup without mutating cells
         * using a guessed direction. */
        entity_placement_commands_build_once(entity);
        return;
    }
    if (fabs((double)direction_xy[0]) <=
        fabs((double)direction_xy[1])) {
        direction = direction_xy[1] <= 0.0f ? 2 : 0;
    }
    else {
        direction = direction_xy[0] <= 0.0f ? 3 : 1;
    }
    for (i = 0; i < count; ++i) {
        int offset_xy[2];
        int dx, dy;
        int cell_xy[2];
        int repeated = 0;
        if (!entity_path_footprint_cell_get(table, footprint, shape_token,
                                            i, offset_xy)) continue;
        dx = offset_xy[0];
        dy = offset_xy[1];
        if (direction == 1) { int old_dx = dx; dx = -dy; dy = old_dx; }
        else if (direction == 2) { dx = -dx; dy = -dy; }
        else if (direction == 3) { int old_dx = dx; dx = dy; dy = -old_dx; }
        cell_xy[0] = source_cell[0] + dx;
        cell_xy[1] = source_cell[1] + dy;
        if (i + 1u < count) {
            int next_offset[2];
            int next_dx, next_dy;
            int next_x, next_y;
            if (!entity_path_footprint_cell_get(table, footprint, shape_token,
                                                i + 1u, next_offset)) {
                next_offset[0] = 0;
                next_offset[1] = 0;
            }
            next_dx = next_offset[0];
            next_dy = next_offset[1];
            if (direction == 1) {
                int old_dx = next_dx; next_dx = -next_dy; next_dy = old_dx;
            }
            else if (direction == 2) {
                next_dx = -next_dx; next_dy = -next_dy;
            }
            else if (direction == 3) {
                int old_dx = next_dx; next_dx = next_dy; next_dy = -old_dx;
            }
            next_x = source_cell[0] + next_dx;
            next_y = source_cell[1] + next_dy;
            repeated = cell_xy[0] == next_x && cell_xy[1] == next_y;
        }

        terrain_cell_primary_flags_mask_clear_and_refresh(
            level, cell_xy[0], cell_xy[1]);
        level_cell_secondary_bit0_clear(level, cell_xy[0], cell_xy[1]);
        if ((entity->flags_3ec & 8u) == 0u) {
            float position[3];
            float rotation[3];
            if (terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                                position)) {
                d3drm_vector_random(rotation);
                (void)lava_effect_select_and_place(3, position, rotation);
            }
            terrain_cell_work_flag_clear(level, cell_xy, 0x3bu);
        }
        terrain_cell_and_neighbors_update(cell_xy[0], cell_xy[1]);
        if (repeated) ++i;
    }
    entity_placement_commands_build_once(entity);
}

/* EntityCellPathEffectsApply (FUN_00409d50) uses one entity argument and the
 * active level/type tables; there is no caller-supplied hook bundle. */
// FUNCTION: LEGORR 0x00409d50
void entity_cell_path_effects_apply_current(EntitySlot* entity)
{
    GameContext* context = entity != NULL ? entity->owner_ctx : NULL;
    if (context == NULL) context = entity_pool_callback_context_get();
    entity_cell_path_effects_apply_level(
        entity, context != NULL ? context->level : NULL);
}

/* Explicit-level compatibility entry point retained for port callers. The
 * type record, rotation, terrain operations, and effect slots are resolved
 * internally; the former hook bundle is ignored. */
void entity_cell_path_effects_apply(
    EntitySlot* entity, LevelState* level,
    const EntityCellPathEffectHooks* hooks)
{
    (void)hooks;
    if (level == NULL && entity != NULL) {
        GameContext* context = entity->owner_ctx;
        if (context != NULL) level = context->level;
    }
    entity_cell_path_effects_apply_level(entity, level);
}

// EntityPlacementCommandsBuildOnce
void ledger_stub_00409f20_EntityPlacementCommandsBuildOnce(EntitySlot* entity)
{
    entity_placement_commands_build_once(entity);
}


// CellCornerOccupancyScan examines the center and ordered cardinal neighbors
// in the map-record sidecar and the level's special-state predicate.
// FUNCTION: LEGORR 0x0040e1e0
uint32_t ledger_stub_0040e1e0_CellCornerOccupancyScan(
    LevelState* level, int cell_x, int cell_y)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { 1, 0, -1, 0 };
    uint32_t i;
    if (level == NULL ||
        terrain_cell_object_record_present_for_level(level, cell_x, cell_y) ||
        terrain_cell_special_state_predicate(level, cell_x, cell_y, 0))
        return 0u;
    for (i = 0u; i < 4u; ++i) {
        int nx = cell_x + dx[i];
        int ny = cell_y + dy[i];
        int opposite = (int)((i - 2u) & 3u);
        int ox = cell_x + dx[opposite];
        int oy = cell_y + dy[opposite];
        int neighbor_record = terrain_cell_object_record_present_for_level(
            level, nx, ny);
        int neighbor_special = 0;
        if (!neighbor_record) {
            neighbor_special = terrain_cell_special_state_predicate(
                level, nx, ny, 0);
            if (!neighbor_special) continue;
        }
        if (terrain_cell_object_record_present_for_level(level, ox, oy))
            return 1u;
        if (terrain_cell_special_state_predicate(level, ox, oy, 0) &&
            !terrain_cell_special_state_predicate(level, nx, ny, 0))
            return 1u;
    }
    return 0u;
}


// LavaErosionNeighborMutate
void ledger_stub_0040f330_LavaErosionNeighborMutate(
    const int xy[2], unsigned int direction, int propagate)
{
    lava_erosion_neighbor_mutate(xy, direction, propagate);
}

// ErosionEightNeighborPropagate
void ledger_stub_0040f5f0_ErosionEightNeighborPropagate(
    const int xy[2], unsigned int direction)
{
    lava_erosion_eight_neighbor_propagate(xy, direction);
}

// LavaErosionCellEligibleAndMutate
int ledger_stub_0040f190_LavaErosionCellEligibleAndMutate(
    const int xy[2], int hint)
{
    return lava_erosion_cell_eligible_and_mutate(xy, hint);
}



} // namespace unported_ledger_stubs
