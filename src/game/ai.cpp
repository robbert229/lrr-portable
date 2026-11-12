// SPDX-License-Identifier: LGPL-3.0-or-later
/* AI execution receivers — the 0x43c810 per-entity callback is called by
 * EntityFrameUpdateDispatch in gods98/simtick.cpp. This module also owns the
 * separate game-layer command/task executor. See ai.hpp for the anatomy;
 * branch comments carry corpus addresses. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ai.hpp"
#include "cmd.hpp"
#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "mission.hpp"
#include "select.hpp"
#include "gods98/view.hpp"
#include "gods98/sound.hpp"
#include "crt/crt.hpp"
#include "gods98/block.hpp"
#include "gods98/string.hpp"
#include "gods98/message.hpp"
#include "util.hpp"
#include "ai_shared.hpp"
#include "ai_from_lego.hpp"
#include "ai_for_legoobject.hpp"
#include "ai_from_legoobject.hpp"

/* The command record's mutable `now` doubles as the task countdown for
 * task commands (the link primitives seed it with the 1500.0 "now"
 * timestamp; the executor resets it to AI_TASK_DURATION on first run). */
#define AI_TOMBSTONE 1000.0f

/* Flag bits the callback branches on (corpus +0x3ec / +0x3e4 words). */
#define ENT_FLAG3EC_ACTIVITY_DONE  0x00200000u  /* activity ended (0x43c845) */
#define ENT_FLAG3EC_TARGET_LINK    0x00040000u  /* acquire link pending (0x43c8b2) */
#define ENT_FLAG3EC_LINKED         0x00080000u  /* link acquired (writes +0x354) */
#define ENT_FLAG3EC_IDLE_REGISTER  0x00000040u  /* type-1 idle check (0x43c94d) */
#define ENT_FLAG3EC_VULNERABLE     0x00000020u  /* cheat-gated flag (0x43c90b) */
#define ENT_FLAG3E4_WAKE_TIMER     0x08000000u  /* +0x368 runs, tick early-return */
#define ENT_FLAG3E4_ACTIVITY_GRANT 0x00000040u  /* +0x3e4 bit 0x40: deferred
                                                 * command-grant install
                                                 * (callback 0x43da62) */

static const void* rec_payload(const void* rec) { return cmd_rec_payload(rec); }

// GLOBAL: LEGORR 0x004a48b3
const char s_tool_name_freezergun[] = "_LegoObject_ToolType_FreezerGun";

// GLOBAL: LEGORR 0x005561fc
const char* s_tool_type_names[AI_TOOL_TYPE_COUNT] = {
    s_tool_name_drill, s_tool_name_spade,
    s_tool_name_hammer, s_tool_name_spanner,
    s_tool_name_laser, s_tool_name_pushergun,
    s_tool_name_birdscarer, s_tool_name_freezergun + 1
};

const char* ai_activity_name(int id)
{
    return (id >= 0 && id < AI_ACTIVITY_COUNT) ? s_activity[id] : NULL;
}

const char* ai_ability_type_name_get(int id)
{
    return (id >= 0 && id < AI_ABILITY_TYPE_COUNT)
        ? s_ability_type_names[id] : NULL;
}

const char* ai_tool_type_name_get(int id)
{
    return (id >= 0 && id < AI_TOOL_TYPE_COUNT) ? s_tool_type_names[id] : NULL;
}

uint32_t ai_activity_resource_slot_get(int id)
{
    return (id >= 0 && id < AI_ACTIVITY_RESOURCE_SLOT_COUNT)
        ? s_activity_resource_slots[id] : 0;
}

char s_bubble_action_names[BUBBLE_ACTION_NAME_COUNT]
                                  [BUBBLE_ACTION_NAME_CAPACITY];
// GLOBAL: LEGORR 0x00557864
uint32_t s_bubble_texture_handles[BUBBLE_ACTION_NAME_COUNT];
BubbleTextureResourceLoadFn s_bubble_texture_load;
BubbleTextureInitializeFn s_bubble_texture_initialize;
void* s_bubble_texture_context;
uint32_t s_bubble_config_global_flags;

int bubble_action_name_register(int index, const char* name)
{
    if (index < 0 || index >= BUBBLE_ACTION_NAME_COUNT || name == NULL) {
        return 0;
    }
    size_t n = strlen(name);
    if (n >= BUBBLE_ACTION_NAME_CAPACITY) {
        n = BUBBLE_ACTION_NAME_CAPACITY - 1u;
    }
    memcpy(s_bubble_action_names[index], name, n);
    s_bubble_action_names[index][n] = '\0';
    return 1;
}

void bubble_texture_load_hooks_set(BubbleTextureResourceLoadFn load,
                                   BubbleTextureInitializeFn initialize,
                                   void* context)
{
    s_bubble_texture_load = load;
    s_bubble_texture_initialize = initialize;
    s_bubble_texture_context = context;
}

void bubble_config_global_flags_set(uint32_t flags)
{
    s_bubble_config_global_flags = flags;
}

uint32_t bubble_texture_handle_peek(int index)
{
    return (index >= 0 && index < BUBBLE_ACTION_NAME_COUNT)
        ? s_bubble_texture_handles[index] : 0u;
}

void bubble_texture_handle_set(int index, uint32_t handle)
{
    if (index >= 0 && index < BUBBLE_ACTION_NAME_COUNT) {
        s_bubble_texture_handles[index] = handle;
    }
}

/* FUN_00403f30: when the chain head is absent or is not kind 0xe, allocate
 * an idle record, link the former head through +0x60, and install it. */
// FUNCTION: LEGORR 0x00403F30
void ai_idle_chain_record_attach(EntitySlot* entity)
{
    if (entity->cmd_head == NULL || cmd_rec_type(entity->cmd_head) != 0xe) {
        void* rec = cmd_alloc(0xe);
        if (rec != NULL) {
            cmd_rec_set_next(rec, entity->cmd_head);
            entity->cmd_head = rec;
        }
    }
}

int ai_idle_count(void) { return s_idle100_n; }

const LevelState* grid_of(const GameContext* ctx);

/* The runner scale is the single-precision value at reference address
 * 0x0049e3b4. Its bytes in LegoRR.exe are 00 00 00 3f (0.5f). */
#define AI_RUNNER_SECONDS_SCALE 0.5f
#define AI_RUNNER_FLAG           0x02000000u

/* Legacy target/work-path seconds-per-cell fallback. Ordinary kind-0
 * routes use the selected Stats RouteSpeed through entity_route_speed. */
float ai_stats_speed(const GameContext* ctx, int tile_type)
{
    const float def = 1.0f / AI_MOVE_SPEED_CPS;
    if (ctx == NULL || tile_type < 0 || tile_type >= AI_STATS_TERRAIN_TYPES ||
        ctx->stats_speed_sec[tile_type] <= 0.0f) {
        return def;
    }
    return ctx->stats_speed_sec[tile_type];
}

/* --- permitted-activity list (+0x370 ids / +0x384 count) -------------------
 * Corpus shape, pinned from the wake/riding decompile bundles:
 *   FUN_0044b480 scan -> FUN_0043c440 (4/5/7) -> FUN_0044c530 (type-2 gate)
 *   FUN_0044b370 setter — append/replace/recycle, callers 0x437eb6 (spawn
 *   dispatch, type-2 arm) and 0x43da62 (deferred command install). */

/* The 0x438150 spawn-dispatch switch (corpus FUN_00437d00 default arm):
 * type − 8 indexes the byte selector at 0x4382ac — the shipped bytes are
 * {00 01 04 04 02 02 02 04 03 03 03 03} for types 8..0x13 — and jumps
 * through the table at 0x438298. Arms 0 (type 8) and 1 (type 9) pin the
 * +0x334 stats block to DAT_004a98f0 / DAT_004a9a40, selectors 2 and 3
 * (types 0xc, 0xd, 0xe, 0x10, 0x11, 0x12, 0x13) to DAT_004a9b90, and
 * selector 4 falls back to the instance-stats attacher FUN_00469c10.
 * These static task-state templates are 336 bytes each (stride 0x150),
 * all-zero in the file apart from the leading dword 0x30. The port maps a
 * matching named Stats row to +0xcc when available; the retail decompile
 * does not show this copy, and other template fields have no identified
 * config owner. */
int ai_minion_state_for(int type)
{
    static const uint8_t selector[12] = {
        0x00, 0x01, 0x04, 0x04, 0x02, 0x02, 0x02, 0x04, 0x03, 0x03, 0x03, 0x03,
    };
    int key = type - 8;
    if (key < 0 || key > 11) {
        return AI_TASK_STATE_NONE;
    }
    switch (selector[key]) {
    case 0x00: return AI_TASK_STATE_MINION_1; /* 0x438163 -> DAT_004a98f0 */
    case 0x01: return AI_TASK_STATE_MINION_2; /* 0x43816f -> DAT_004a9a40 */
    case 0x02:                                /* 0x43817b -> DAT_004a9b90 */
    case 0x03:
        return AI_TASK_STATE_MINION_3;
    default:  return AI_TASK_STATE_NONE;      /* 0x438187: instance stats */
    }
}

/* Portable capacity selection used by activity-grant and command gates.
 * This keeps the reconstruction's static task-state and context fallbacks
 * outside the address-mapped raw reader above. */
int ai_activity_cap_for_context(const GameContext* ctx, const EntitySlot* e)
{
    int cap;
    if (e == NULL) return 0;
    if (e->task_state != AI_TASK_STATE_NONE) {
        cap = e->activity_cap;
    }
    else if (e->stats_record != NULL) {
        cap = ai_activity_cap(e);
    }
    else {
        cap = (ctx != NULL) ? ctx->stats_activity_cap : 0;
    }
    if (cap > AI_ACTIVITY_SLOTS) {
        cap = AI_ACTIVITY_SLOTS; /* port bound (corpus stores inline) */
    }
    return cap;
}

AiRegSlot s_reg_flagged[AI_REG_FLAGGED_SLOTS];
AiRegSlot s_reg_type4[AI_REG_TYPE4_SLOTS];
AiRegSlot s_reg_type2[AI_REG_TYPE2_SLOTS];

static AiRegSlot* s_reg_window(int window, int* count)
{
    if (window == 0) {
        *count = AI_REG_FLAGGED_SLOTS;
        return s_reg_flagged;
    }
    if (window == 1) {
        *count = AI_REG_TYPE4_SLOTS;
        return s_reg_type4;
    }
    *count = AI_REG_TYPE2_SLOTS;
    return s_reg_type2;
}

void ai_object_registry_slot_peek(int window, int slot, void** record,
                                  uint32_t* timer)
{
    int count = 0;
    AiRegSlot* w = s_reg_window(window, &count);
    if (slot < 0 || slot >= count) {
        return;
    }
    if (record != NULL) {
        *record = w[slot].record;
    }
    if (timer != NULL) {
        *timer = w[slot].timer;
    }
}

void ai_object_registry_slot_set(int window, int slot, void* record,
                                 uint32_t timer)
{
    int count = 0;
    AiRegSlot* w = s_reg_window(window, &count);
    if (slot < 0 || slot >= count) {
        return;
    }
    w[slot].record = record;
    w[slot].timer = timer;
}

int ai_object_registry_window_count(int window)
{
    int count = 0;
    (void)s_reg_window(window, &count);
    return count;
}

void ai_object_registries_reset(void)
{
    memset(s_reg_flagged, 0, sizeof s_reg_flagged);
    memset(s_reg_type4, 0, sizeof s_reg_type4);
    memset(s_reg_type2, 0, sizeof s_reg_type2);
}

/* Finish the chain-head command on `e`: unlink from both structures, free,
 * advance the chain. */
static int finish_head(GameContext* ctx, EntitySlot* e)
{
    void* rec = e->cmd_head;
    e->cmd_head = cmd_rec_next(rec);
    cmd_global_unlink(rec);
    cmd_free(rec, 0);
    (void)ctx;
    return 1;
}

/* Append a waypoint to the entity path list (the corpus +0x24 buffer is
 * heap-grown while the path is built — the appender at 0x44693b reallocs
 * `count*20` bytes and writes {x, y, 0.5f} per walked cell). This mirrors
 * one append; the builder is ai_path_build. */
int ai_path_append(EntitySlot* e, float x, float y)
{
    if (e == NULL) {
        return -1;
    }
    if (e->wp_index >= e->wp_count) {
        e->wp_count = 0; /* previous path popped whole: reuse the buffer */
        e->wp_index = 0;
    }
    if (e->wp_count < 0 || e->wp_count == 0x7fffffff ||
        !entity_waypoint_storage_reserve(
            e, (unsigned int)e->wp_count + 1u)) {
        return -1;
    }
    AiWaypoint* waypoint = &ENTITY_WAYPOINTS(e)[e->wp_count];
    waypoint->x = x;
    waypoint->y = y;
    waypoint->weight_y = 0.5f;
    waypoint->weight_x = 0.5f;
    waypoint->pending = 1;
    waypoint->step_kind = 0;
    waypoint->reserved[0] = 0;
    waypoint->reserved[1] = 0;
    return e->wp_count++;
}

/* ------------------------------------------------------------------ *
 *  Wave-flood pathfinder core — the port of the corpus system behind
 *  the level+4 grid handle: the seed FUN_00430ee0 and the traceback /
 *  spread probe FUN_00430530 (decompile evidence:
 *  docs/evidence/legorr-pathfinder-build.bundle; finding 10).
 *
 *  Corpus state lives in the handle's voxel records (stride 0x48): the
 *  wave-state byte record+0 (0 = unwalked, 1 = flooded/walked, 5 =
 *  traceback-stamped) and the record+8 flag word, which carries the
 *  wave flag 0x40000 the seed raises over its northwest 2x2 footprint
 *  including the seed itself (FUN_00430ee0, record 12's offset table)
 *  and the 8 bit FUN_00430530 gates rejected cells on (`flags & 8`).
 *  The port-level equivalent of that handle is the per-build wave state
 *  below: the same state bytes and flag bits over the port grid, created
 *  by the build's install step and released by its finish reset, so the
 *  flood work state is scoped per path build and never leaks between
 *  commands. The only map-side authority it consults is the ported
 *  queries level_walkable (FUN_00432770) and level_tile_occupied
 *  (FUN_00432e00 read): an unwalkable or entity-reserved cell takes the
 *  blocked bit 8 and the wave refuses it, exactly as the corpus gate.
 *
 *  The corpus traceback roots are the routing-destination tiles — cells
 *  whose record+2 type is 8 or 10 (FUN_00430530's '\b'/'\n' branches).
 *  The port routes to a cell, not a destination tile, so the seeded
 *  target cell takes that role: traceback starts there and walks back
 *  through the wave-marked cells to the start cell. The wave pump and
 *  value-propagation queue behind the handle (FUN_0044efb0/FUN_0044f380)
 *  is modelled by the spread FIFO below; the corpus's message/flag
 *  side effects inside FUN_00430530 (FUN_00419b80, FUN_00431450, the
 *  +0x334-gated branches) belong to the unported state objects and stay
 *  TODO(corpus) like the rest of that family.
 * ------------------------------------------------------------------ */
#define WAVE_STATE_WALKED    1u        /* flooded: record+0 = 1          */
#define WAVE_STATE_TRACEBACK 5u        /* record+0 = 5 (traceback pass)  */
#define WAVE_FLAG_BLOCKED    0x00000008u /* FUN_00430530 flags & 8 gate  */
#define WAVE_FLAG_WAVE       0x00040000u /* FUN_00430ee0 wave flag       */

typedef struct WaveFlood {
    const LevelState* grid;
    int w, h;
    uint8_t*  cell;   /* wave-state byte per cell          */
    uint32_t* flags;  /* per-cell flag overlay (8 / 0x40000) */
    int*      dist;   /* wave steps from target (traceback gradient) */
    int*      queue;  /* spread FIFO; reused as the traceback stack   */
    uint8_t*  scan;   /* per-cell traceback scan cursor    */
    int       marks;  /* cells carrying the wave flag (diagnostic)    */
    int       goal;   /* seeded target cell index          */
    int       self;   /* the mover's own cell: its reservation is exempt
                         from the occupancy gate (the walker marks its
                         current cell every tick, 0x43deed) */
} WaveFlood;

/* The four spread directions the flood advances through. */
static const int s_wave_off[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

/* The seed's flag footprint (record 12's local_20 table): a northwest
 * 2x2 block including the seed cell — (-1,-1), (0,-1), (0,0), (-1,0). */
static const int s_seed_off[4][2] = { { -1, -1 }, { 0, -1 }, { 0, 0 }, { -1, 0 } };

static int s_wave_marks; /* wave-flag count from the last path build */

int ai_wave_flood_count(void) { return s_wave_marks; }

static void wave_mark(WaveFlood* wf, int cell)
{
    if ((wf->flags[cell] & WAVE_FLAG_WAVE) == 0) {
        wf->flags[cell] |= WAVE_FLAG_WAVE;
        ++wf->marks;
    }
}

/* The blocked gate: an unwalkable or entity-reserved cell takes the
 * corpus's flags & 8 rejection bit; the ported queries are the only
 * state authority the wave consults. The mover's own cell is exempt
 * from the occupancy check: the walker raises that reservation on its
 * current cell every tick (callback 0x43deed), so rejecting it would
 * strand every retarget mid-move (PR #87 feedback). */
static int wave_gate_blocked(WaveFlood* wf, int x, int y, int cell)
{
    if (cell == wf->self) {
        return !level_walkable(wf->grid, x, y);
    }
    if (!level_walkable(wf->grid, x, y) || level_tile_occupied(wf->grid, x, y)) {
        wf->flags[cell] |= WAVE_FLAG_BLOCKED;
        return 1;
    }
    return 0;
}

/* Temporary path-search seed inspired by the terrain initializer: this
 * routine gates only on its interior-coordinate
 * bounds and the destination record's state byte being zero. It does not
 * query walkability or occupancy: those are path-search policy, not part
 * of the seed operation. Then it stamps the seed cell walked (1) and raises
 * the wave flag 0x40000 over the northwest 2x2 footprint. The bytes and
 * flag words here are per-build replacements for record+0 and record+8. */
static int wave_seed(WaveFlood* wf, int tx, int ty)
{
    if (tx == 0 || ty == 0 ||
        (unsigned)tx >= (unsigned)wf->w - 2u ||
        (unsigned)ty >= (unsigned)wf->h - 2u) {
        return 0;
    }
    int goal = ty * wf->w + tx;
    if (wf->cell[goal] != 0) {
        return 0;
    }
    wf->cell[goal] = WAVE_STATE_WALKED;
    wf->goal = goal;
    for (int i = 0; i < 4; ++i) {
        int nx = tx + s_seed_off[i][0], ny = ty + s_seed_off[i][1];
        if (nx >= 0 && ny >= 0 && nx < wf->w && ny < wf->h) {
            wave_mark(wf, ny * wf->w + nx);
        }
    }

    /* TerrainCellInitialize refreshes its northwest 2x2 cell-init region,
     * then marks the surrounding 3x3 physical terrain patch dirty. The
     * LevelState APIs own those same operations in this port. The seed's
     * strict interior gate guarantees the 2x2 coordinates are valid; the
     * dirty setter retains the terrain handle's own boundary checks. */
    level_cell_init_pass((LevelState*)wf->grid, tx - 1, ty - 1);
    level_cell_init_pass((LevelState*)wf->grid, tx,     ty - 1);
    level_cell_init_pass((LevelState*)wf->grid, tx - 1, ty);
    level_cell_init_pass((LevelState*)wf->grid, tx,     ty);
    for (int y = ty - 1; y <= ty + 1; ++y) {
        for (int x = tx - 1; x <= tx + 1; ++x) {
            terrain_cell_changed_flag_set(wf->grid->ptr_terrain, x, y);
        }
    }
    return 1;
}

/* The wave spread: the corpus pumps the wave flag through the handle's
 * queue (FUN_0044efb0); the port models the same four-direction spread
 * with a FIFO from the seed cell. Entering a cell stamps it walked (1)
 * and raises the wave flag 0x40000 — the mark of a walked cell — while
 * blocked cells take flag 8 and the spread refuses them. The spread
 * stops once the start cell floods; that is the corpus's traceback root
 * condition (FUN_00430530 returning nonzero onto the mover). */
static void wave_spread(WaveFlood* wf, int start)
{
    int w = wf->w, h = wf->h;
    int head = 0, tail = 1;
    wf->queue[0] = wf->goal;
    while (head < tail && wf->cell[start] == 0) {
        int cell = wf->queue[head++];
        int cx = cell % w, cy = cell / w;
        for (int i = 0; i < 4 && wf->cell[start] == 0; ++i) {
            int nx = cx + s_wave_off[i][0], ny = cy + s_wave_off[i][1];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                continue;
            }
            int ncell = ny * w + nx;
            if (wf->cell[ncell] != 0) {
                continue;
            }
            if (wave_gate_blocked(wf, nx, ny, ncell) != 0) {
                continue;
            }
            wf->cell[ncell] = WAVE_STATE_WALKED;
            wave_mark(wf, ncell);
            wf->dist[ncell] = wf->dist[cell] + 1;
            wf->queue[tail++] = ncell;
        }
    }
}

/* FUN_00430530 traceback: rooted at the flooded target cell, it walks
 * back over the wave's cells to the start cell, stamping every walked
 * cell with record+0 = 5 and refusing cells carrying the blocked flag 8.
 * The port follows the wave-distance gradient with backtracking, so
 * dead-end branches get stamped 5 too — the corpus stamps its explored
 * neighbours 5 in exactly the same "tried and refused" pass. Writes the
 * goal->start sequence into the reused stack (wf->queue) and returns the
 * cell count before the start cell, or 0 when the start never flooded
 * (unreachable or blocked target). */
static int wave_traceback(WaveFlood* wf, int start)
{
    if (wf->cell[start] != WAVE_STATE_WALKED) {
        return 0;
    }
    int depth = 0;
    wf->queue[0] = wf->goal;
    wf->cell[wf->goal] = WAVE_STATE_TRACEBACK;
    while (depth >= 0) {
        if (depth >= wf->w * wf->h) {
            return 0; /* safety: a walk-back longer than the grid is broken */
        }
        int cell = wf->queue[depth];
        if (cell == start) {
            return depth;
        }
        int cx = cell % wf->w, cy = cell / wf->w;
        int advanced = 0;
        for (int i = wf->scan[cell]; i < 4; ++i) {
            wf->scan[cell] = (uint8_t)(i + 1);
            int nx = cx + s_wave_off[i][0], ny = cy + s_wave_off[i][1];
            if (nx < 0 || ny < 0 || nx >= wf->w || ny >= wf->h) {
                continue;
            }
            int ncell = ny * wf->w + nx;
            if (wf->cell[ncell] != WAVE_STATE_WALKED) {
                continue; /* flooded-only: never unwalked, never traceback */
            }
            if ((wf->flags[ncell] & WAVE_FLAG_BLOCKED) != 0) {
                continue; /* the flags & 8 gate */
            }
            if (wf->dist[ncell] != wf->dist[cell] + 1) {
                continue; /* follow the wave gradient out from the target */
            }
            wf->queue[++depth] = ncell;
            wf->cell[ncell] = WAVE_STATE_TRACEBACK;
            advanced = 1;
            break;
        }
        if (advanced == 0) {
            --depth; /* backtrack: this branch is a dead end */
        }
    }
    return 0;
}

/* Build a command's path into the entity buffer through the wave-flood
 * core. Corpus shape: the command dispatch installs the list per command
 * (0x4418d0: buffer set, count/index and +0x28c/+0x290 reset, mover flag
 * raised), the builder appends one entry per walked cell (0x44693b's
 * per-cell {x, y, 0.5f} writes — the port's pending=1 mirror), and the
 * command end releases the list (0x441a50). Port-side the finish reset
 * also releases the wave state, so no flood work survives a build.
 * Returns the entry count, or 0 when the build yields no path (no grid,
 * off-handle or blocked target, unreachable, equal cells) or cannot grow
 * the route buffer. */
int ai_path_build(GameContext* ctx, EntitySlot* e, float tx, float ty)
{
    s_wave_marks = 0; /* the build owns the whole flood state: start blank */
    const LevelState* grid = grid_of(ctx);
    if (e == NULL || grid == NULL) {
        return 0; /* no terrain transform or route without a bound grid */
    }
    int w = level_width(grid);
    int h = level_height(grid);
    float start_xy[2];
    int sx, sy;
    entity_transform_position_get(e, start_xy);
    if (!terrain_world_xy_to_cell(grid->ptr_terrain, start_xy[0], start_xy[1],
                                  &sx, &sy)) return 0;
    int gx = level_cell_of(tx);
    int gy = level_cell_of(ty);
    if (sx < 0 || sy < 0 || gx < 0 || gy < 0 || sx >= w || sy >= h ||
        gx >= w || gy >= h) {
        return 0;
    }

    WaveFlood wf;
    memset(&wf, 0, sizeof(wf));
    wf.grid = grid;
    wf.w = w;
    wf.h = h;
    wf.self = sy * w + sx; /* the mover's own cell: exempt from its own
                              reservation, the gate still sees the map */
    int size = w * h;
    wf.cell = (uint8_t*)calloc((size_t)size, 1);
    wf.flags = (uint32_t*)calloc((size_t)size, sizeof(uint32_t));
    wf.dist = (int*)calloc((size_t)size, sizeof(int));
    wf.queue = (int*)calloc((size_t)size, sizeof(int));
    wf.scan = (uint8_t*)calloc((size_t)size, 1);

    int len = 0;
    if (wf.cell != NULL && wf.flags != NULL && wf.dist != NULL &&
        wf.queue != NULL && wf.scan != NULL) {
        /* FUN_00430ee0 itself only checks the seed record's state byte.
         * The path builder separately rejects an unusable destination;
         * its caller is a route search, whereas the mapped seed primitive
         * is a terrain-cell initializer and has no walkability gate. */
        if (wave_gate_blocked(&wf, gx, gy, gy * w + gx) == 0 &&
            wave_seed(&wf, gx, gy) != 0) {
            int start = sy * w + sx;
            if (wf.cell[start] == 0) {
                wave_spread(&wf, start); /* equal cells need no wave */
            }
            len = wave_traceback(&wf, start);
        }
        /* The traceback stack is wf->queue; cells [0, len) run
         * target -> start-adjacent (the start itself is excluded: the
         * entity is already there). */
        s_wave_marks = wf.marks;
    }

    int rc = 0;
    if (len > 0 && entity_waypoint_storage_reserve(e, (unsigned int)len)) {
        e->wp_count = 0; /* corpus install: fresh list per command (0x4418d0) */
        e->wp_index = 0;
        for (int i = len - 1; i >= 0; --i) {
            int cell = wf.queue[i];
            if (ai_path_append(e, (float)(cell % w),
                               (float)(cell / w)) < 0) {
                e->wp_count = 0;
                break;
            }
        }
        if (e->wp_count == len) rc = len;
    }
    free(wf.cell); /* finish reset (0x441a50): the flood state is released */
    free(wf.flags);
    free(wf.dist);
    free(wf.queue);
    free(wf.scan);
    return rc;
}

/* The bound level's voxel grid, or NULL for an absent/sub-margin map.
 * Ordinary kind-0 admission waits while its terrain is unavailable. */
const LevelState* grid_of(const GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL || level_width(ctx->level) < 3 ||
        level_height(ctx->level) < 3) {
        return NULL;
    }
    return ctx->level;
}

/* Legacy target/work-path rate adapter, retained for non-kind-0 behavior.
 * +0x24..34 actually contain drill times in reference ticks; this historical
 * reciprocal is NOT ordinary walking speed. Kind 0 uses entity_route_speed.
 * Correcting the other work-path semantics needs its own evidence/scope. */
static float underfoot_seconds(const GameContext* ctx, const EntitySlot* e,
                               int type)
{
    float seconds;
    if (type >= 0 && type < AI_STATS_TERRAIN_TYPES && e != NULL &&
        e->stats_record != NULL) {
        seconds = entity_stats_float_at24_indexed(e, type);
        if (seconds > 0.0f) return seconds;
    }
    return ai_stats_speed(ctx, type);
}

/* Historical target/work adapter, preserved for non-kind-0 commands.
 * Its +0x24 reciprocal is not the ordinary route speed authority. */
static float underfoot_cps(const GameContext* ctx, const LevelState* grid,
                           const EntitySlot* e, float x, float y)
{
    if (grid == NULL) {
        return AI_MOVE_SPEED_CPS;
    }
    int cell_x, cell_y;
    if (!terrain_world_xy_to_cell(grid->ptr_terrain, x, y, &cell_x, &cell_y))
        return AI_MOVE_SPEED_CPS;
    int type = level_tile_type(grid, cell_x, cell_y);
    float seconds = underfoot_seconds(ctx, e, type);
    if (e != NULL && (e->flags_3e8 & AI_RUNNER_FLAG) != 0u)
        seconds *= AI_RUNNER_SECONDS_SCALE;
    return 1.0f / seconds;
}

/* Keep the portable simulation coordinates and the entity's already-bound
 * DDiScene object in step. The retail movement callback advances the scene
 * pose as it consumes the path; entity_transform_position_get reads that
 * scene pose when one is installed. */
static void ai_entity_position_set(EntitySlot* entity, float x, float y)
{
    if (entity == NULL) return;
    entity_world_position_set(entity, x, y, entity->rot, 0);
}

/* Move/target step: move `e` toward (tx,ty) by speed*dt, where speed is
 * the terrain-under-foot rate (see underfoot_cps); 1 when arrived. */
static int step_toward(GameContext* ctx, EntitySlot* e, float tx, float ty,
                       float dt)
{
    float position[2];
    entity_transform_position_get(e, position);
    float dx = tx - position[0];
    float dy = ty - position[1];
    float len = (float)__builtin_sqrtf(dx * dx + dy * dy);
    if (len <= 0.001f) {
        ai_entity_position_set(e, tx, ty);
        return 1;
    }
    const LevelState* grid = grid_of(ctx);
    float blocksize = grid != NULL
        ? terrain_blocksize_get(grid->ptr_terrain) : 1.0f;
    float step = underfoot_cps(ctx, grid, e, position[0], position[1])
        * blocksize * dt;
    if (step >= len) {
        ai_entity_position_set(e, tx, ty);
        return 1;
    }
    ai_entity_position_set(e, position[0] + (dx / len) * step,
                           position[1] + (dy / len) * step);
    return 0;
}

/* AI owns admitted kind-0 and kind19 world motion. Numeric cell waypoints;
 * zero flags are valid (0x441700), and wp_index alone owns progression.
 * entity_route_speed reuses 0x446610's WORLD units/reference tick. AI takes
 * seconds: multiply the rate by 25, with NO blocksize factor. The legacy
 * underfoot_cps work/target path above is not the ordinary route contract. */
static int step_path(GameContext* ctx, EntitySlot* e, float dt)
{
    const LevelState* grid = grid_of(ctx);
    float position[2];
    float remaining = dt;
    float blocksize;
    if (grid == NULL || grid->ptr_terrain == NULL || e->wp_count <= 0 ||
        (e->flags_3e8 & 0x4000u) != 0u) return 0;
    blocksize = terrain_blocksize_get(grid->ptr_terrain);
    if (blocksize <= 0.0f) return 0;
    entity_transform_position_get(e, position);
    while (e->wp_index >= 0 && e->wp_index < e->wp_count) {
        int cx, cy;
        float target[2];
        float dx, dy, length, speed, step;
        AiWaypoint* waypoint;
        if (!terrain_world_xy_to_cell(grid->ptr_terrain,
                position[0], position[1], &cx, &cy)) return 0;
        waypoint = &ENTITY_WAYPOINTS(e)[e->wp_index];
        /* Use the same actor-aware cell/edge authority as route admission.
         * The separate work-cell predicate 0x432770 requires flags not set
         * by the OList creator and does not own ordinary route motion. */
        if (!terrain_cell_route_and_occupancy_check(e, grid,
                (int)waypoint->x, (int)waypoint->y, cx, cy, 0)) return 0;
        if (!entity_route_waypoint_world_get(e, (unsigned int)e->wp_index,
                                              target)) return 0;
        dx = target[0] - position[0];
        dy = target[1] - position[1];
        length = (float)sqrt((double)(dx * dx + dy * dy));
        speed = entity_route_speed(e) * 25.0f; /* seconds -> reference ticks */
        if (!(speed > 0.0f)) return 0; /* absent/invalid rate retains the head */
        step = speed * remaining;
        if (length > step && length > blocksize * 0.00001f) {
            ai_entity_position_set(e, position[0] + dx / length * step,
                                    position[1] + dy / length * step);
            return 0;
        }
        ai_entity_position_set(e, target[0], target[1]);
        position[0] = target[0];
        position[1] = target[1];
        if (speed > 0.0f) remaining -= length / speed;
        if (remaining < 0.0f) remaining = 0.0f;
        waypoint->pending &= (uint8_t)~1u;
        ++e->wp_index;
    }
    /* Do not finish at the penultimate waypoint or spend this frame's time
     * again on the next command. Command ownership releases the route/head. */
    return e->wp_index == e->wp_count;
}

/* Yield a task's deposit into the mission counters by priority kind, and
 * report the incident on the message bus (finding 22 §1). */
static void task_deposit(GameContext* ctx, int kind)
{
    switch (kind) {
    case 0: /* Crystal */
        mission_set_crystals(ctx, ctx->mission.crystals + AI_TASK_YIELD);
        mission_incident(ctx, 0); /* CrystalFound */
        break;
    case 1: /* Ore */
    case 2: /* DefaultCollect */
        mission_set_ore(ctx, ctx->mission.ore + AI_TASK_YIELD);
        mission_incident(ctx, 1); /* OreSeamFound */
        break;
    case 4: /* Construction */
        ctx->mission.constructions += AI_TASK_YIELD;
        mission_incident(ctx, 8); /* Constructed */
        break;
    default:
        break; /* other priorities have no counter effect in this subset */
    }
}

/* 43d0a6: turn completion owns this callback before route/grant. */
static int ai_turn_completion_frame_update(EntitySlot* e)
{
    void* component;
    float angle;
    if ((e->flags_3e4 & 0x4000u) != 0u) return 1;
    if ((e->flags_3e0 & 4u) == 0u) return 0;
    if (!e->activity_advance_valid ||
        e->activity_advance_activity != e->activity ||
        e->activity_advance_generation != e->activity_generation ||
        e->activity_advance_overflow != e->activity_timer ||
        !(e->activity_advance_overflow > 0.0f)) return 1;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    component = actor_scene_motion_component(e);
    if (component == NULL) return 1;
    if ((e->flags_3e0 & 0x80u) != 0u) {
        render_object_rotate_axis(component, 1u, 0.0f, 1.0f, 0.0f, 1.57079637f);
        if (!actor_scene_clock_reset(e)) return 1;
        e->flags_3e0 &= ~0x80u;
        return 1;
    }
    angle = (e->activity == 9 || e->activity == 20) ? -1.57079637f : 1.57079637f;
    render_object_rotate_axis(component, 1u, 0.0f, 1.0f, 0.0f, angle);
    e->flags_3e0 = (e->flags_3e0 & ~4u) | 1u;
    if (e->wp_count == 0) ai_set_activity(e, 0, 0);
    else if ((e->flags_3e0 & 0x800000u) != 0u && e->type == 2)
        ai_set_activity(e, 3, 0);
    else ai_set_activity(e, 1, 0);
#else
    (void)component; (void)angle;
#endif
    return 1;
}

/* Work callback 43cb40..43ce69. */
static void* ai_drill_component(EntitySlot* e)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (e->type == 2 || e->type == 3)
        return actor_scene_motion_component(e);
#endif
    return e->type_component_handle;
}
static void ai_drill_sound_cleanup(EntitySlot* e)
{
    sound_handle_stop_forward(e->drill_sound_handle_350);
    e->flags_3ec &= ~0x100000u;
    (void)sound_submission_route_wrapper(ai_drill_component(e),
        (int)entity_stats_word_at_c0_or_c4(e, 1), 0, 1, NULL);
    /* 484c90 is the mapped no-op. */
    e->flags_3e8 &= ~0x4000u;
}

static void ai_drill_work_frame_update(EntitySlot* e, float dt)
{
    AiWaypoint* route;
    LevelState* level;
    int gx, gy, wx, wy, mode;
    float position[2], divisor;
    if ((e->flags_3e0 & 8u) == 0u) return;
    if ((e->flags_3e0 & 0x10u) != 0u) {
        e->drill_sound_handle_350 = sound_submission_route_wrapper(
            ai_drill_component(e),
            (int)entity_stats_word_at_c0_or_c4(e, 0), 1, 1, NULL);
        e->flags_3ec |= 0x100000u;
        e->flags_3e0 &= ~0x10u;
    }
    route = ENTITY_WAYPOINTS(e);
    if (e->wp_count <= 0 || e->wp_index < 0 ||
        e->wp_index >= e->wp_count ||
        (e->wp_dynamic == NULL && e->wp_count > AI_WAYPOINTS_MAX) ||
        (e->wp_dynamic != NULL &&
         (unsigned int)e->wp_count > e->wp_capacity)) {
        e->flags_3e0 &= ~8u;
        e->flags_3e8 &= ~0x02000000u;
        ai_drill_sound_cleanup(e);
        return;
    }
    gx = (int)e->pos_pair_2f4;
    gy = (int)e->pos_pair_2f8;
    wx = crt_float_to_long(route[e->wp_index].x);
    wy = crt_float_to_long(route[e->wp_index].y);
    mode = (int)(e->flags_3e8 & 0x02000000u);
    if (!terrain_cell_walkability_query_current(e, (uint32_t)gx,
                                                 (uint32_t)gy, mode, 1)) {
        e->flags_3e0 &= ~8u;
        e->flags_3e8 &= ~0x02000000u;
        ai_drill_sound_cleanup(e);
        return;
    }
    if (!entity_work_object_position_get(e, position)) return;
    /* 484c90 on type 2 is a no-op in this port. */
    divisor = entity_stats_float_at24_indexed(e,
        (int)terrain_cell_material_get_current(gx, gy));
    if ((entity_stats_word_get(e) & 1u) != 0u && mode != 0)
        divisor *= 0.5f;
    if (!terrain_cell_drill_work_add_current(gx, gy, divisor, dt)) {
        int cx, cy;
        level = level_current_owner_get();
        if (level != NULL && level->ptr_terrain != NULL &&
            terrain_world_xy_to_cell(level->ptr_terrain, position[0],
                                     position[1], &cx, &cy) &&
            cx == wx && cy == wy)
            e->flags_3e8 |= 0x4000u;
        return;
    }
    if (!terrain_cell_walkability_query_current(e, (uint32_t)gx,
                                                 (uint32_t)gy, mode, 1))
        return;
    level = level_current_owner_get();
    if (level == NULL) return;
    if (mode != 0) {
        if (!terrain_cell_initialize(level, (uint32_t)gx, (uint32_t)gy))
            return;
    }
    else if (!terrain_cell_mutate(level, (uint32_t)wx, (uint32_t)wy, 0))
        return;
    entity_command_unlink_by_kind(e, 8, NULL, 1);
    e->flags_3e0 &= ~8u;
    {
        int goal[2];
        goal[0] = gx; goal[1] = gy;
        level_tile_mark_current(goal, 0);
    }
    route = ENTITY_WAYPOINTS(e);
    if (e->wp_count > 0 &&
        (e->wp_dynamic == NULL ? e->wp_count <= AI_WAYPOINTS_MAX :
            (unsigned int)e->wp_count <= e->wp_capacity) &&
        route[e->wp_count - 1].step_kind == 1u)
        route[e->wp_count - 1].step_kind = 0u;
    ai_drill_sound_cleanup(e);
}

/* The verified callback branches, in corpus order. Returns 1 when the
 * corpus's callback would have returned early (walker moves on). */
static int callback_branches(GameContext* ctx, EntitySlot* e, float dt)
{
    /* 0x43c845 — activity completion: +0x2e8 zeroed, then FUN_00447c40's
     * bookkeeping (early return when current == last), then flag clear. */
    if ((e->flags_3ec & ENT_FLAG3EC_ACTIVITY_DONE) != 0) {
        e->activity = AI_ACTIVITY_NONE;
        activity_register_bookkeeping(e); /* FUN_00447c40, including runner */
        e->flags_3ec &= ~ENT_FLAG3EC_ACTIVITY_DONE;
    }

    /* 0x43c870 — wake timer: if +0x3e4 bit 0x8000000 is set, tick +0x368
     * down and RETURN — no further behaviour runs this frame. Once the
     * timer has expired, FUN_0044c110 wakes the attached target: unless
     * it is in the attached special state (+0x3e0 bit 0x200000), it takes
     * +0x340 = 100.0 and sets +0x3e8 bit 0x800000. The corpus's +0x364
     * pointer is filled by the attach-pair writer (0x44c0e2, not ported),
     * so the port mirrors the field as-is. */
    if ((e->flags_3e4 & ENT_FLAG3E4_WAKE_TIMER) != 0) {
        if (!(e->wake_timer > 0.0f) && e->wake_target != NULL) {
            ai_attached_entity_wake(e);
        }
        e->wake_timer -= dt;
        return 2; /* direct early return; no shared activity tail */
    }

    /* 0x43c8b2 — target acquisition: +0x3ec bit 0x40000 asks FUN_0044a200
     * for the work object and then links it. The source corpus's FUN_00469fc0
     * is a stats-record +0xc8 getter (Finding 457), not a linkability test;
     * the corpus link helper itself remains unported. The simplified port
     * mirrors the acquired target directly in +0x354 and sets bit 0x80000. */
    if ((e->flags_3ec & ENT_FLAG3EC_TARGET_LINK) != 0) {
        const void* obj = (e->cmd_head != NULL)
                              ? rec_payload(e->cmd_head) : NULL;
        if (obj != NULL) {
            e->link_target = (void*)obj; /* +0x354 mirror */
            e->flags_3ec |= ENT_FLAG3EC_LINKED;
        }
        e->flags_3ec &= ~ENT_FLAG3EC_TARGET_LINK;
    }

    /* 0x43c90b — cheat word DAT_00556ed0 bit 0 (testb $0x1): when set, the
     * type-2 "can do" probe FUN_0044c530 decides flag bit 0x20; otherwise
     * the bit is cleared. FUN_0044c530 = "type 2 and FUN_0043c440 is
     * non-zero"; FUN_0043c440 asks FUN_0044b480's scan over the entity's
     * real permitted-activity list (+0x370 slots, count +0x384) for
     * activities 4 (drill), 5 (teleport) or 7 (reinforce) — list-driven,
     * not the single current-activity field. */
    if ((ctx->cheat & 1u) != 0 && ai_can_do(e)) {
        e->flags_3ec |= ENT_FLAG3EC_VULNERABLE;
    } else {
        e->flags_3ec &= ~ENT_FLAG3EC_VULNERABLE;
    }

    /* 0x43c94d — type-1 idle registration: flag bit 0x40 + empty work and
     * link state -> FUN_0043a960. The extracted helper owns its tile probe
     * and capped idle-table insertion. */
    if (e->type == 1 && (e->flags_3ec & ENT_FLAG3EC_IDLE_REGISTER) != 0 &&
        e->cmd_head == NULL && e->link_target == NULL) {
        e->flags_3ec &= ~ENT_FLAG3EC_IDLE_REGISTER;
        ai_lava_crust_activity_register(ctx, e);
    }

    /* 0x43c9d7..43ca1b precedes action routes and the pickup transition.
     * A mounted type-2 actor has a separate vehicle clock-sync branch; do
     * not advance its animation independently of that owner. */
    entity_building_deposit_state_monitor(e);
    if (!(e->type == 2 && (e->flags_3e4 & 8u) != 0u &&
          e->owner_36c != NULL && ((const EntitySlot*)e->owner_36c)->type == 1))
        (void)entity_activity_tick(e, dt);

    ai_drill_work_frame_update(e, dt);

    if (ai_turn_completion_frame_update(e)) return 1;

    /* 0x43da62 — deferred command grant: +0x3e4 bit 0x40 marks "the command
     * at the chain head grants an activity". The branch consumes the flag,
     * installs the record's +0x38 activity onto the entity's permitted-
     * activity list through the FUN_0044b370 setter (append, or in-place
     * replace when the grant is a special 4/5/7), drops the grant records
     * (FUN_00403d10(ent, 0x13, 0, 1) unlinks every chain record of type
     * 0x13 with no per-type side effects — cmd_chain_drop), and RETURNS:
     * both corpus paths end at 0x43ed80. The corpus writer of the +0x3e4
     * grant bit is terminal kind0xd in the path owner, not the 0x438150
     * Stats switch. cmd_gettool_route_finish raises it only after the ordinary
     * mover reaches the full weighted endpoint. The ported list receives
     * the numeric selector from the actual kind19 head. */
    if ((e->flags_3e4 & ENT_FLAG3E4_ACTIVITY_GRANT) != 0) {
        e->flags_3e4 &= ~ENT_FLAG3E4_ACTIVITY_GRANT;
        if (e->cmd_head != NULL) {
            if (cmd_rec_type(e->cmd_head) == 19) {
                if (!cmd_gettool_route_arrived(e)) return 1;
                /* Retail43da7e reads the NUMERIC head+38, not the port's
                 * staged grant_act (which is zero in actual19 producers). */
                ai_activity_grant(ctx, e, (int)cmd_rec_value_38(e->cmd_head));
            }
            else ai_activity_grant(ctx, e, cmd_rec_activity(e->cmd_head));
        }
        cmd_chain_drop(e, 0x13); /* corpus 0x43da92: FUN_00403d10(ent, 0x13, 0, 1) */
        return 1;
    }
    return 0;
}

/* EntityFrameUpdateDispatch's LAB_0043c810 receiver. The pool walker owns
 * liveness and +0x3e8 filtering; this adapter carries the current GameContext
 * into the recovered behavior branches and refreshes the activity timer. */
static int ai_action_route_frame_update(EntitySlot* entity, float dt)
{
    EntitySlot* target;
    int index;
    if ((entity->flags_3e0 & 1u) == 0u) {
        return 0;
    }
    /* The application runs ai_tick before this callback. Its Move/GetTool
     * owners already advance and retire these command routes in seconds. */
    if (entity->cmd_head != NULL &&
        (cmd_rec_type(entity->cmd_head) == 0 ||
         cmd_rec_type(entity->cmd_head) == 19)) {
        return 0;
    }

    /* 0x43de28..47: this receiver already gets reference frame units from
     * the EngineTick scaled carrier; do not multiply by 25 again. */
    (void)entity_path_step_advance_and_arrival_check(entity, dt);
    if ((entity->flags_3e0 & 0x200u) == 0u) return 0;
    target = (EntitySlot*)entity->link_2fc;
    if (target == NULL || target->attach_304 != NULL) {
        entity->flags_3e0 &= ~0x200u;
        return 1;
    }
    index = entity->carry_state_324;
    /* The reference slots end immediately before +0x324. Refuse corrupt
     * counts before mutating a relationship; upstream capacity is separate. */
    if (index < 0 || index >= 7) return 1;
    ai_set_activity(entity, 0x0f, 0);
    if (entity->type == 3) {
        if (target->type == 5) ai_set_activity(entity, 0x10, 0);
        else if (target->type == 6) ai_set_activity(entity, 0x23, 0);
    }
    if (index == 0) entity->carry_link_308 = target;
    else entity->carry_extra_links[index - 1] = target;
    entity->link_2fc = 0u;
    target->attach_304 = entity;
    target->attached_scene_slot_index = index;
    /* 0x43e23c retains arrival 0x200 while starting the pickup activity.
     * Later activity completion owns its clearing, not this attachment. */
    entity->flags_3e0 |= 0x400u;
    entity_command_unlink_by_kind(entity, 3, target, 1);
    ai_idle_chain_record_attach(entity);
    ++entity->carry_state_324;
    return 1;
}

static int ai_pickup_completion_frame_update(EntitySlot* entity, float dt)
{
    EntitySlot* carried;
    LevelState* level;
    int neighbor, motion, count;
    if (entity->type != 2 || (entity->flags_3e0 & 0x600u) != 0x600u)
        return 0;
    /* The mounted clock-sync branch has a separate owner. Do not use an
     * old independent actor result while that branch is unavailable. */
    if ((entity->flags_3e4 & 8u) != 0u && entity->owner_36c != NULL &&
        ((EntitySlot*)entity->owner_36c)->type == 1) return 0;
    /* 43ea00..43ebf1: neither missing nor unimplemented motion ownership
     * is a zero result. Both actual gates must establish the idle path. */
    neighbor = entity_neighbor_step_idle_checked(entity, 0);
    if (neighbor != 0) return 0;
    motion = entity_motion_idle_gate_checked(entity, dt);
    if (motion != 0 || !entity->activity_advance_valid ||
        entity->activity_advance_activity != entity->activity ||
        entity->activity_advance_generation != entity->activity_generation ||
        entity->activity_advance_overflow != entity->activity_timer ||
        !(entity->activity_advance_overflow > 0.0f)) return 0;
    count = entity->carry_state_324;
    if (count < 1 || count > 7) return 0;
    carried = count == 1 ? (EntitySlot*)entity->carry_link_308
                         : entity->carry_extra_links[count - 2];
    level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
    if (carried == NULL || level == NULL) return 0;
    entity->activity_advance_valid = 0u;
    /* 43ec17/43eca0: the last carried kind owns the one-shot pickup count.
     * Retain carry/backlink and bit400; the scheduler owns later delivery. */
    entity->flags_3e0 &= ~0x200u;
    if (carried->type == 6) global_object_field94_increment_for_level(level);
    else if (carried->type == 7) {
        global_object_fielda8_increment_for_level(level);
        message_dispatch(0x1a, NULL, (uint32_t)(uintptr_t)entity, NULL);
    }

    return 1;
}

int ai_entity_frame_update(EntitySlot* entity, float dt)
{
    GameContext* ctx;
    int handled;
    if (entity == NULL) return 0;
    entity->activity_advance_valid = 0u;
    ctx = entity->owner_ctx != NULL ? entity->owner_ctx
                                    : entity_pool_callback_context_get();
    if (ctx == NULL) {
        return 0;
    }
    handled = callback_branches(ctx, entity, dt);
    if (handled == 2) {
        return 1;
    }
    if (handled || ai_action_route_frame_update(entity, dt)) {
        (void)activity_register_bookkeeping(entity);
        return 1;
    }
    (void)ai_pickup_completion_frame_update(entity, dt);
    (void)activity_register_bookkeeping(entity);
    return 0;
}

int ai_tick(GameContext* ctx, float dt)
{
    if (ctx == NULL || !(dt > 0.0f)) {
        return 0;
    }
    int done = 0;
    /* EntityFrameUpdateDispatch owns the flag callback walk. This game-layer
     * pass runs BEFORE EngineTick's command scheduler, entity callback and
     * operation-queue drain in the application frame. */
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            EntitySlot* e = &ctx->entity[t][i];
            if (e->type == 0) {
                continue; /* walker liveness *ent+0x408==ent mirror */
            }
            while (e->cmd_head != NULL) {
                void* head = e->cmd_head;
                int type = cmd_rec_type(head);
                if (type == 0) {
                    /* Queue drain is later in EngineTick. On the next frame
                     * this shares admission with the installed scheduler, which
                     * may have different entity gates. One route, one world
                     * mover, one retirement owner in either call order. */
                    if ((cmd_rec_state(head) & CMD_STATE_DONE) != 0u ||
                        (cmd_move_route_prepare(e, ctx->level) &&
                         step_path(ctx, e, dt))) {
                        done += cmd_move_route_finish(e);
                    }
                    break;
                } else if (type == 5 || type == 8) {
                    /* The command/entity callbacks own delivery and Drill
                     * tasks. This earlier mover cannot retire an attached
                     * task before those owners receive it. */
                    break;
                } else if (type == 19) {
                    /* Retain the scheduler's actual GetTool head. Only its
                     * admitted terminal-d route uses this ordinary mover. */
                    if (cmd_gettool_route_moving(e) && step_path(ctx, e, dt))
                        (void)cmd_gettool_route_finish(e);
                    break;
                } else if (type == 2) {
                    /* Target: step toward the payload entity. */
                    const EntitySlot* tgt = (const EntitySlot*)rec_payload(head);
                    float target_position[2];
                    int have_target = tgt != NULL;
                    if (have_target)
                        entity_transform_position_get(tgt, target_position);
                    if (!have_target || step_toward(ctx, e,
                            target_position[0], target_position[1], dt)) {
                        done += finish_head(ctx, e);
                    } else {
                        break;
                    }
                } else if (type == 0xe) {
                    break; /* idle record: poller territory (finding 10) */
                } else {
                    /* Unknown record types: drop them (the task type-3 runs
                     * in the global pass below, matching CmdLinkHead). */

                    done += finish_head(ctx, e);
                }
            }
        }
    }
    /* 2. Global command list: task records (CmdLinkHead) run their yield.
     *    Move/target records in the global list are the same objects as the
     *    chain heads handled above — already unlinked when completed. */
    {
        void* rec = cmd_global_head();
        while (rec != NULL) {
            void* next = cmd_rec_next(rec);
            int type = cmd_rec_type(rec);
            if (type == 3) {
                float now = cmd_rec_now(rec);
                if (now <= 0.0f || now >= AI_TOMBSTONE) {
                    now = AI_TASK_DURATION; /* first run: seed the timer */
                }
                now -= dt;
                if (now <= 0.0f) {
                    task_deposit(ctx, cmd_rec_kind(rec));
                    cmd_global_unlink(rec);
                    cmd_free(rec, 0);
                    ++done;
                } else {
                    cmd_rec_set_now(rec, now);
                }
            }
            rec = next;
        }
    }
    return done;
}

int ai_commands_active(const GameContext* ctx)
{
    int n = 0;
    if (ctx == NULL) {
        return 0;
    }
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            const EntitySlot* e = &ctx->entity[t][i];
            for (void* rec = e->cmd_head; rec != NULL; rec = cmd_rec_next(rec)) {
                ++n;
            }
        }
    }
    return n;
}

void entity_activity_scene_state_update(GameContext* ctx)
{
    if ((s_entity_state_flags & 0x10u) != 0u) {
        s_entity_state_flags |= 8u;
        return;
    }
    s_entity_state_flags |= 4u;
    global_object_field98_clear(ctx != NULL ? ctx->level : NULL);
}

uint32_t entity_state_flags_peek(void) { return s_entity_state_flags; }
void     entity_state_flags_set(uint32_t value) { s_entity_state_flags = value; }

/* --- the entity activity gate band (see ai.hpp) ---------------------------- */

/* --- random activity attempt (2026-10-24) --------------------------------- */

/* --- the entity counter + link band (see ai.hpp) --------------------------- */

/* --- the idle-tile update band (2026-10-25) -------------------------------- */

/* Reset the port mirror of the idle-registration slice cleared by
 * ActivityNameTableInit and SpawnCoordinatePoolClear. */
void ai_idle_table_reset(void)
{
    for (int i = 0; i < AI_IDLE_TABLE_MAX; ++i) {
        s_idle100[i] = NULL;
    }
    s_idle100_n = 0;
}

/* ActiveEntityListContainsCode (FUN_004031e0): a permitted-activity scan
 * (FUN_0044b480) over the selection when it is non-empty, else over the
 * corpus's global active-entity list. The selection branch caches the
 * count (the corpus's ebx), walks the record array with a pointer and an
 * unsigned counter (add reg,4 / inc / cmp-jb) and passes entries BLIND to
 * the scan — the corpus has no NULL-entry test there, the scan itself
 * answers 0; the fallback branch skips NULL entries (the corpus's
 * test/je) and re-reads the count every iteration. The selection branch
 * answers on the first hit and never falls back to the list
 * (pinned branch isolation).
 * The global list mirrors the entity pool's create/teardown lifecycle; the
 * explicit install adapter remains available to replace it for callers that
 * have a more specific active set. The 50-entry host bound matches the
 * corpus region; the corpus strides a NULL selection entry into the activity
 * scan, while the port's scan answers 0 for it (guarded). */
#define ACTIVE_ENTITY_LIST_MAX 50 /* corpus region DAT_004b335c..3424 */
// GLOBAL: LEGORR 0x004b335c
static EntitySlot* s_active_list[ACTIVE_ENTITY_LIST_MAX];
// GLOBAL: LEGORR 0x004b3424
static int s_active_list_used;

void active_entity_list_install(const EntitySlot* const* entries, int count)
{
    if (entries == NULL || count < 0) {
        count = 0;
    }
    if (count > ACTIVE_ENTITY_LIST_MAX) {
        count = ACTIVE_ENTITY_LIST_MAX; /* deviation: window guard */
    }
    int i;
    for (i = 0; i < count; ++i) {
        s_active_list[i] = (EntitySlot*)entries[i];
    }
    for (i = count; i < ACTIVE_ENTITY_LIST_MAX; ++i) {
        s_active_list[i] = NULL;
    }
    s_active_list_used = count;
}

int active_entity_list_count(void)
{
    return s_active_list_used;
}

const EntitySlot* active_entity_list_entry_get(int index)
{
    return index >= 0 && index < s_active_list_used ? s_active_list[index] : NULL;
}

void active_entity_list_entity_add(const EntitySlot* entity)
{
    int i;
    if (entity == NULL) return;
    for (i = 0; i < s_active_list_used; ++i) {
        if (s_active_list[i] == entity) return;
    }
    if (s_active_list_used < ACTIVE_ENTITY_LIST_MAX)
        s_active_list[s_active_list_used++] = (EntitySlot*)entity;
}

void active_entity_list_entity_remove(const EntitySlot* entity)
{
    int i;
    if (entity == NULL) return;
    for (i = 0; i < s_active_list_used; ++i) {
        if (s_active_list[i] == entity) {
            int j;
            for (j = i; j + 1 < s_active_list_used; ++j)
                s_active_list[j] = s_active_list[j + 1];
            s_active_list[--s_active_list_used] = NULL;
            return;
        }
    }
}

void active_entity_list_clear(void)
{
    int i;
    for (i = 0; i < ACTIVE_ENTITY_LIST_MAX; ++i)
        s_active_list[i] = NULL;
    s_active_list_used = 0;
}

// FUNCTION: LEGORR 0x004031e0
int active_entity_list_contains_code(GameContext* ctx, int code)
{
    unsigned count;
    EntitySlot* const* records;
    EntitySlot* const* walk;
    unsigned i;

    if (selection_has_any(ctx) == 0) {
        /* the fallback re-reads the count (the corpus's DAT_004b3424
         * load) on every iteration and skips NULL entries (test/je). */
        for (i = 0; i < (unsigned) s_active_list_used; ++i) {
            if (s_active_list[i] != NULL
                && ai_activity_scan(s_active_list[i], code) != 0) {
                return 1;
            }
        }
        return 0;
    }
    /* selection branch: cached count (corpus ebx), unsigned counter and
     * a walk pointer (inc / add reg,4 / cmp-jb); entries go BLIND to
     * the scan — the corpus has no NULL test here and the scan
     * (FUN_0044b480) answers 0 for NULL. */
    count = (unsigned) selection_count(ctx);
    records = selection_record_array(ctx);
    for (i = 0, walk = records; records != NULL && i < count; ++i, ++walk) {
        if (ai_activity_scan(*walk, code) != 0) {
            return 1;
        }
    }
    return 0;
}

/* DAT_004eabc8, LegoObject.c's tool-tip gate that Panel.c reads (game/ai.h).
 * The port has no owner of it yet; this port global stands in for it
 * (finding 554). */
#include "ai.h"

int s_lego_object_tool_tip_gate;
