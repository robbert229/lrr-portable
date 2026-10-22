// SPDX-License-Identifier: LGPL-3.0-or-later
/* AI execution receivers — port of the FUN_0043b270 entity-tick callback
 * (entry 0x43c810), decompile-verified 2026-10-21 (finding 10 closed). See
 * ai.hpp for the anatomy; branch comments carry corpus addresses. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai.hpp"
#include "cmd.hpp"
#include "level.hpp"
#include "mission.hpp"

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
#define ENT_FLAG3EC_ACTIVITY_OK    0x00000004u  /* +0x3ec bit 0x4: the list
                                                 * permits a 4/5/7 activity;
                                                 * written by FUN_0044b370 */
#define ENT_FLAG3E4_WAKE_TIMER     0x08000000u  /* +0x368 runs, tick early-return */
#define ENT_FLAG3E4_ACTIVITY_GRANT 0x00000040u  /* +0x3e4 bit 0x40: deferred
                                                 * command-grant install
                                                 * (callback 0x43da62) */

static const void* rec_payload(const void* rec) { return cmd_rec_payload(rec); }

/* DAT_004decc4 activity-name table, per the initializer at 0x436c2f. */
static const char* const s_activity[AI_ACTIVITY_COUNT] = {
    "Activity_Stand", "Activity_Route", "Activity_RouteRubble",
    "Activity_RunPanic", "Activity_Drill", "Activity_Teleport",
    "Activity_Walk", "Activity_Reinforce", "Activity_Reverse",
    "Activity_TurnLeft", "Activity_TurnRight", "Activity_CantDo",
    "Activity_Emerge", "Activity_Enter", "Activity_EnterRein",
    "Activity_Collect", "Activity_Gather", "Activity_Carry",
    "Activity_CarryRubble", "Activity_Throw", "Activity_CarryTurnLeft",
    "Activity_CarryTurnRight", "Activity_CarryStand", "Activity_HitLeft",
    "Activity_HitRight", "Activity_HitFront", "Activity_HitBack",
    "Activity_HitHard", "Activity_Dynamite", "Activity_Deposit",
    "Activity_Clear", "Activity_Place", "Activity_Repair",
    "Activity_Slip", "Activity_Rest", "Activity_Eat",
    "Activity_Stamp", "Activity_ThrowMan", "Activity_ThrownByRockMonster",
    "Activity_GetUp", "Activity_BuildPath", "Activity_Upgrade",
    "Activity_Explode", "Activity_Unpowered", "Activity_FireLaser",
    "Activity_Freezed", "Activity_FreezeStart", "Activity_FreezeMelt",
    "Activity_Recharge", "Activity_WakeUp", "Activity_Train",
    "Activity_FloatOn", "Activity_FloatOff", "Activity_Opening",
    "Activity_Closing", "Activity_Open",
    "Activity_Waiting1", "Activity_Waiting2", "Activity_Waiting3",
    "Activity_Waiting4", "Activity_Waiting5", "Activity_Waiting6",
    "Activity_Waiting7", "Activity_Waiting8", "Activity_Waiting9",
    "Activity_Waiting10", "Activity_Waiting11", "Activity_Waiting12",
    "Activity_Waiting13", "Activity_Waiting14", "Activity_Waiting15",
    "Activity_Waiting16", "Activity_Waiting17", "Activity_Waiting18",
    "Activity_Waiting19", "Activity_Waiting20", "Activity_Waiting21"
};

const char* ai_activity_name(int id)
{
    return (id >= 0 && id < AI_ACTIVITY_COUNT) ? s_activity[id] : NULL;
}

void ai_set_activity(EntitySlot* e, int id, int param)
{
    if (e == NULL || id < 0 || id >= AI_ACTIVITY_COUNT) {
        return; /* corpus indexes the table blindly; clamp the mirror */
    }
    e->activity = id;
    /* Corpus +0x2ec mirrors "last name bookkept by FUN_00447c40"; every
     * corpus activity-set site re-registers through 447c40 in the same
     * step, so the fold lands here in the port (see finding 10). */
    e->activity_last = id;
    e->activity_param = param;
    e->activity_timer = 0.0f;
}

/* FUN_00403f30: if the chain head is not a live idle record (type 0xe),
 * prepend one so the idle poller sees this entity. */
static void attach_idle_record(EntitySlot* e)
{
    if (e->cmd_head == NULL || cmd_rec_type(e->cmd_head) != 0xe) {
        void* rec = cmd_alloc(0xe);
        if (rec != NULL) {
            cmd_rec_set_next(rec, e->cmd_head);
            e->cmd_head = rec;
        }
    }
}

/* FUN_0043a960 half: type-1 idle registration into DAT_004eabcc, max 100
 * (DAT_004ead5c). The tile == '\t' probe needs the path-grid mirror and is
 * TODO(corpus); the capacity rule and table are byte-exact. */
#define AI_IDLE_TABLE_MAX 100
static EntitySlot* s_idle100[AI_IDLE_TABLE_MAX];
static int s_idle100_n;

int ai_idle_count(void) { return s_idle100_n; }

/* FUN_00469cc0: `return *(float *)(*(ent + 0x334) + 0x24 + type * 4)` —
 * the stats speed table behind the entity's stats block, indexed by the
 * terrain type byte. The port models the stats block's table as the
 * one game-wide `ctx->stats_speed_sec` (see game.hpp), so the probe
 * becomes a guarded table read: zero entries and out-of-range types fall
 * back to the AI_MOVE_SPEED_CPS-equivalent seconds/cell. */
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

/* FUN_0044b480: scan the id slots at +0x370 through the +0x384 count;
 * a zero count answers 0 without reading. */
int ai_activity_scan(const EntitySlot* e, int activity)
{
    if (e == NULL || e->act_count <= 0) {
        return 0;
    }
    for (int i = 0; i < e->act_count; ++i) {
        if (e->act_list[i] == activity) {
            return 1;
        }
    }
    return 0;
}

/* FUN_0043c440: the can-do activities, checked in corpus order —
 * drill (1), teleport (2), reinforce (3), else 0. */
int ai_activity_probe(const EntitySlot* e)
{
    if (ai_activity_scan(e, AI_ACT_DRILL)) {
        return 1;
    }
    if (ai_activity_scan(e, AI_ACT_TELEPORT)) {
        return 2;
    }
    return ai_activity_scan(e, AI_ACT_REINFORCE) ? 3 : 0;
}

/* FUN_0044c530: "can do" — only type-2 entities answer, and only from
 * their permitted-activity list. */
int ai_can_do(const EntitySlot* e)
{
    return (e != NULL && e->type == 2) ? (ai_activity_probe(e) != 0) : 0;
}

/* The 0x438150 spawn-dispatch switch (corpus FUN_00437d00 default arm):
 * type − 8 indexes the byte selector at 0x4382ac — the shipped bytes are
 * {00 01 04 04 02 02 02 04 03 03 03 03} for types 8..0x13 — and jumps
 * through the table at 0x438298. Arms 0 (type 8) and 1 (type 9) pin the
 * +0x334 stats block to DAT_004a98f0 / DAT_004a9a40, selectors 2 and 3
 * (types 0xc, 0xd, 0xe, 0x10, 0x11, 0x12, 0x13) to DAT_004a9b90, and
 * selector 4 falls back to the instance-stats attacher FUN_00469c10.
 * These static templates are the minion "task states": 336 bytes each
 * (stride 0x150), all-zero in the file apart from the leading dword 0x30,
 * filled at runtime (config-driven, TODO(corpus) — the port models their
 * activity capacity per entity, see EntitySlot.activity_cap). */
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

/* FUN_0046a060: capacity = *(*(ent+0x334) + 0xcc). The port models the
 * stats block's +0xcc as EntitySlot.activity_cap (set by the minion
 * task-state spawn; 0 = unset) falling back to the game-wide default. */
int ai_activity_cap(const GameContext* ctx, const EntitySlot* e)
{
    int cap = (e != NULL && e->activity_cap > 0)
                  ? e->activity_cap
                  : ((ctx != NULL) ? ctx->stats_activity_cap : 0);
    if (cap > AI_ACTIVITY_SLOTS) {
        cap = AI_ACTIVITY_SLOTS; /* port bound (corpus stores inline) */
    }
    return cap;
}

static int activity_is_special(int v) /* FUN_0044b350's {4,5,7} predicate */
{
    return v == AI_ACT_DRILL || v == AI_ACT_TELEPORT || v == AI_ACT_REINFORCE;
}

/* FUN_0044b370: install one activity grant. Corpus order (the whole body,
 * see ai.hpp): cap gate, +0x3f4 = 0 (field not ported — no reader in the
 * ported surface, TODO(corpus)), +0x3ec bit 0x4 refresh, special-entry
 * replace, FIFO append/overwrite. The corpus compares count != cap with
 * cap = FUN_0046a060(ent) = *(state+0xcc) (the stats-block instance fill is
 * config-driven; the port models it as the per-entity activity_cap, and
 * defaults through ctx->stats_activity_cap clamped to the inline span). */
void ai_activity_grant(GameContext* ctx, EntitySlot* e, int value)
{
    if (e == NULL) {
        return;
    }
    int cap = ai_activity_cap(ctx, e);
    if (cap <= 0) {
        return; /* cap-0 entity: the corpus's grant runs nothing (0x44b38d) */
    }
    if (e->act_count > cap) {
        e->act_count = cap; /* guarded port mirror of corpus count/cap */
    }
    const int special = activity_is_special(value);
    if (ai_activity_probe(e) != 0 || special) {
        e->flags_3ec |= ENT_FLAG3EC_ACTIVITY_OK;   /* 0x44b3bf: or $0x4 */
    } else {
        e->flags_3ec &= ~ENT_FLAG3EC_ACTIVITY_OK;  /* 0x44b39c: and $0xfb */
    }
    if (special) {
        /* A special grant replaces the first special entry on the list —
         * the old grant is cleared in place (write at 0x44b463) and the
         * setter returns without touching count. */
        for (int i = 0; i < e->act_count; ++i) {
            if (activity_is_special(e->act_list[i])) {
                e->act_list[i] = value;
                return;
            }
        }
    }
    if (e->act_count != cap) {
        e->act_list[e->act_count++] = value;       /* 0x44b44a: append */
        return;
    }
    /* Full list: drop the oldest (left shift, 0x44b417) and write the last
     * slot (count is kept — the corpus stays at cap). */
    for (int i = 0; i + 1 < e->act_count; ++i) {
        e->act_list[i] = e->act_list[i + 1];
    }
    e->act_list[e->act_count - 1] = value;
}

static void idle100_register(EntitySlot* e)
{
    for (int i = 0; i < s_idle100_n; ++i) {
        if (s_idle100[i] == e) {
            return;
        }
    }
    if (s_idle100_n < AI_IDLE_TABLE_MAX) {
        s_idle100[s_idle100_n++] = e;
    }
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
    if (e->wp_count >= AI_WAYPOINTS_MAX) {
        return -1;
    }
    e->wp[e->wp_count].x = x;
    e->wp[e->wp_count].y = y;
    e->wp[e->wp_count].pending = 1;
    return e->wp_count++;
}

static const LevelState* grid_of(const GameContext* ctx);

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

/* FUN_00430ee0 seed: validate the target — the corpus drops row/column 0
 * and everything at or past its w-2/h-2 margin (unsigned), and its
 * record+0 == 0 gate is per-build state, fresh here — then stamps the
 * seed cell walked (1) and raises the wave flag 0x40000 over record 12's
 * footprint offsets, a northwest 2x2 block including the seed itself.
 * Returns 0 (and installs no state) when the target is off-handle or
 * blocked by the ported queries. */
static int wave_seed(WaveFlood* wf, int tx, int ty)
{
    if (tx == 0 || ty == 0 ||
        (unsigned)tx >= (unsigned)wf->w - 2u ||
        (unsigned)ty >= (unsigned)wf->h - 2u) {
        return 0;
    }
    int goal = ty * wf->w + tx;
    if (wave_gate_blocked(wf, tx, ty, goal) != 0) {
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
 * off-handle or blocked target, unreachable, equal cells, longer than
 * AI_WAYPOINTS_MAX) and the caller seeds the one-hop fallback. */
int ai_path_build(GameContext* ctx, EntitySlot* e, float tx, float ty)
{
    s_wave_marks = 0; /* the build owns the whole flood state: start blank */
    const LevelState* grid = grid_of(ctx);
    if (e == NULL || grid == NULL) {
        return 0; /* no grid handle: the one-hop fallback owns this move */
    }
    int w = level_width(grid);
    int h = level_height(grid);
    int sx = level_cell_of(e->x);
    int sy = level_cell_of(e->y);
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
        if (wave_seed(&wf, gx, gy) != 0) {
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
    if (len > 0 && len <= AI_WAYPOINTS_MAX) {
        e->wp_count = 0; /* corpus install: fresh list per command (0x4418d0) */
        e->wp_index = 0;
        for (int i = len - 1; i >= 0; --i) {
            int cell = wf.queue[i];
            ai_path_append(e, (float)(cell % w), (float)(cell / w));
        }
        rc = len;
    }
    free(wf.cell); /* finish reset (0x441a50): the flood state is released */
    free(wf.flags);
    free(wf.dist);
    free(wf.queue);
    free(wf.scan);
    return rc;
}

/* The bound level's voxel grid, or NULL when no map is loaded (the corpus
 * tick cannot walk an empty grid, so a sub-margin grid counts as absent and
 * the port-side fallback paths run instead). */
static const LevelState* grid_of(const GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL || level_width(ctx->level) < 3 ||
        level_height(ctx->level) < 3) {
        return NULL;
    }
    return ctx->level;
}

/* Cells/second the entity moves at *now*: the speed of the terrain under
 * foot, read the corpus way (move branch 0x43cc35–0x43cc5c: position →
 * cell via FUN_0048e060, cell → type via FUN_004319b0, type → stats entry
 * via FUN_00469cc0, where the corpus accumulates dt/entry per cell).
 * Without a bound grid the corpus cannot read terrain and the port default
 * applies. Corpus runner scaling (+0x3e8 bit 0x2000000 multiplies the
 * entry by the constant at 0x49e3b4 — value not recorded in the bundles)
 * is TODO(corpus). */
static float underfoot_cps(const GameContext* ctx, const LevelState* grid,
                           float x, float y)
{
    if (grid == NULL) {
        return AI_MOVE_SPEED_CPS;
    }
    int type = level_tile_type(grid, level_cell_of(x), level_cell_of(y));
    return 1.0f / ai_stats_speed(ctx, type);
}

/* Move/target step: move `e` toward (tx,ty) by speed*dt, where speed is
 * the terrain-under-foot rate (see underfoot_cps); 1 when arrived. */
static int step_toward(GameContext* ctx, EntitySlot* e, float tx, float ty,
                       float dt)
{
    float dx = tx - e->x;
    float dy = ty - e->y;
    float len = (float)__builtin_sqrtf(dx * dx + dy * dy);
    if (len <= 0.001f) {
        e->x = tx;
        e->y = ty;
        return 1;
    }
    float step = underfoot_cps(ctx, grid_of(ctx), e->x, e->y) * dt;
    if (step >= len) {
        e->x = tx;
        e->y = ty;
        return 1;
    }
    e->x += (dx / len) * step;
    e->y += (dy / len) * step;
    return 0;
}

/* Move step over the entity path list — the corpus move branch's shape
 * (callback 0x43cb98: walk +0x24 buffer at the +0x2c index; a null list
 * falls back to the command's coordinates at 0x43cbb1). With a grid bound,
 * the mover-state sync marks the entity's own cell occupied every tick
 * (callback 0x43deed) and the walker only steps while its *current* cell
 * is walkable (FUN_00432770, checked via 0x43cbe8; unwalkable = no step,
 * the command stays live). The step rate is the terrain-under-foot speed
 * (FUN_00469cc0, see underfoot_cps — 0x43cc35's probe on the same branch).
 * Arrival runs the corpus pop: the entry's pending
 * byte is cleared (0x43cd90), no entry removal. When the list runs out the
 * corpus arrives — +0x3e0 bit 0x8 is cleared (0x43cd66), the arrival cell's
 * occupancy flag is dropped (FUN_00432e00(cell, 0) at 0x43cd72), and the
 * command completes; FUN_00403d10(ent,8,0,1)'s record-list unlink stays
 * TODO(corpus) (the port has no +0xbc record list). Returns 1 only when
 * the whole path is walked. */
static int step_path(GameContext* ctx, EntitySlot* e, float fallback_x,
                     float fallback_y, float dt)
{
    const LevelState* grid = grid_of(ctx);
    if (grid != NULL && (e->flags_3e0 & ENT_FLAG3E0_MOVING) != 0) {
        int cx = level_cell_of(e->x), cy = level_cell_of(e->y);
        level_tile_mark((LevelState*)grid, cx, cy, 1);
        if (!level_walkable(grid, cx, cy)) {
            return 0; /* off-grid or blocked tile: no step this frame */
        }
    }
    int done;
    if (e->wp_count == 0) {
        done = step_toward(ctx, e, fallback_x, fallback_y, dt);
    } else {
        /* Path list: each entry is one cell crossed at the speed of the
         * terrain under foot (corpus 0x43cc35's read + 0x434570's per-cell
         * progress: a cell completes every speed seconds, so `dt` can
         * complete several cells on fast terrain and the overflow carries
         * into the next entries). Every completion pops in place. */
        float rem = dt;
        for (;;) {
            while (e->wp_index < e->wp_count &&
                   e->wp[e->wp_index].pending == 0) {
                ++e->wp_index; /* skip popped entries */
            }
            if (e->wp_index >= e->wp_count) {
                done = 1; /* path exhausted: arrived */
                break;
            }
            float wx = e->wp[e->wp_index].x, wy = e->wp[e->wp_index].y;
            float dx = wx - e->x, dy = wy - e->y;
            float len = (float)__builtin_sqrtf(dx * dx + dy * dy);
            if (len <= 0.001f) {
                e->wp[e->wp_index].pending = 0; /* already there: pop */
                ++e->wp_index;
                continue;
            }
            float cps = underfoot_cps(ctx, grid, e->x, e->y);
            float step = cps * rem;
            if (step >= len) {
                e->x = wx;
                e->y = wy;
                rem -= len / cps; /* time actually consumed */
                e->wp[e->wp_index].pending = 0; /* pop = flag clear (0x43cd90) */
                ++e->wp_index;
                if (e->wp_index >= e->wp_count) {
                    done = 1;
                    break;
                }
            } else {
                e->x += (dx / len) * step;
                e->y += (dy / len) * step;
                done = 0;
                break; /* still walking the current entry */
            }
        }
    }
    if (done && grid != NULL) {
        level_tile_mark((LevelState*)grid, level_cell_of(e->x),
                        level_cell_of(e->y), 0); /* arrival clear (0x43cd72) */
    }
    return done;
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

/* The verified callback branches, in corpus order. Returns 1 when the
 * corpus's callback would have returned early (walker moves on). */
static int callback_branches(GameContext* ctx, EntitySlot* e, float dt)
{
    /* 0x43c845 — activity completion: +0x2e8 zeroed, then FUN_00447c40's
     * bookkeeping (early return when current == last), then flag clear. */
    if ((e->flags_3ec & ENT_FLAG3EC_ACTIVITY_DONE) != 0) {
        int current = AI_ACTIVITY_NONE; /* +0x2e8 was zeroed pre-call */
        if (current != e->activity_last) {
            /* FUN_00403f30: the zeroed name is outside the eight-name
             * exception set, so the idle record attach always runs here. */
            attach_idle_record(e);
            /* per-type runner progress: object layer, TODO(corpus) */
            e->activity_last = AI_ACTIVITY_NONE; /* +0xbb = +0x2e8 (0) */
            e->activity_timer = 0.0f;            /* +0xb5 / +0x2d4 */
        }
        e->activity = AI_ACTIVITY_NONE;
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
            EntitySlot* t = (EntitySlot*)e->wake_target;
            if ((t->flags_3e0 & 0x00200000u) == 0) {
                t->field_340 = 100.0f;
                t->flags_3e8 |= 0x00800000u;
            }
        }
        e->wake_timer -= dt;
        return 1;
    }

    /* 0x43c8b2 — target acquisition: +0x3ec bit 0x40000 asks FUN_0044a200
     * for the work object and FUN_00469fc0 whether it is linkable; on
     * success FUN_00465050 creates the link (handle lands in +0x354),
     * bit 0x80000 is set, 0x40000 cleared. */
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
     * link state -> FUN_0043a960, which moves the entity into the
     * DAT_004eabcc idle table (cap 100) when it stands on a '\t' tile
     * (record+2 == 9 after the FUN_004424f0 -> cell round). The tile probe
     * runs with a bound grid; without a map the registration keeps its
     * port-side behavior (TODO(corpus)). */
    if (e->type == 1 && (e->flags_3ec & ENT_FLAG3EC_IDLE_REGISTER) != 0 &&
        e->cmd_head == NULL && e->link_target == NULL) {
        e->flags_3ec &= ~ENT_FLAG3EC_IDLE_REGISTER;
        const LevelState* grid = grid_of(ctx);
        if (grid == NULL ||
            level_tile_type(grid, level_cell_of(e->x),
                            level_cell_of(e->y)) == TILE_TYPE_IDLE) {
            idle100_register(e);
        }
    }

    /* 0x43da62 — deferred command grant: +0x3e4 bit 0x40 marks "the command
     * at the chain head grants an activity". The branch consumes the flag,
     * installs the record's +0x38 activity onto the entity's permitted-
     * activity list through the FUN_0044b370 setter (append, or in-place
     * replace when the grant is a special 4/5/7), drops the grant records
     * (FUN_00403d10(ent, 0x13, 0, 1) unlinks every chain record of type
     * 0x13 with no per-type side effects — cmd_chain_drop), and RETURNS:
     * both corpus paths end at 0x43ed80. The corpus writer of the +0x3e4
     * grant bit is NOT the 0x438150 switch (it writes only +0x334) — the
     * raiser is a computed-value store among the +0x3e4 write census
     * (finding 10 §"Spawn-dispatch task-state switch"), still TODO. The
     * ported list is where all these grants land. */
    if ((e->flags_3e4 & ENT_FLAG3E4_ACTIVITY_GRANT) != 0) {
        e->flags_3e4 &= ~ENT_FLAG3E4_ACTIVITY_GRANT;
        if (e->cmd_head != NULL) {
            ai_activity_grant(ctx, e, cmd_rec_activity(e->cmd_head));
        }
        cmd_chain_drop(e, 0x13); /* corpus 0x43da92: FUN_00403d10(ent, 0x13, 0, 1) */
        return 1;
    }
    return 0;
}

int ai_tick(GameContext* ctx, float dt)
{
    if (ctx == NULL || dt <= 0.0f) {
        return 0;
    }
    int done = 0;
    /* 1. Entity chains: the callback's flag branches, then the port-side
     *    application of chain heads (move/target stepping). Corpus order:
     *    the walker visits type blocks in table order, entities ascending. */
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            EntitySlot* e = &ctx->entity[t][i];
            if (e->type == 0) {
                continue; /* walker liveness *ent+0x408==ent mirror */
            }
            if (callback_branches(ctx, e, dt)) {
                continue; /* corpus early return (wake-timer state) */
            }
            e->activity_timer += dt;
            while (e->cmd_head != NULL) {
                void* head = e->cmd_head;
                int type = cmd_rec_type(head);
                if (type == 0) {
                    /* Move: walk the entity path list seeded by the creator.
                     * +0x3e0 bit 0x8 is the corpus mover-state marker — the
                     * arrival path clears it (callback 0x43cd66); the tick's
                     * exact gate condition is TODO(corpus), so it is
                     * mirrored, not enforced here. */
                    if (!step_path(ctx, e, cmd_rec_move_x(head),
                                   cmd_rec_move_y(head), dt)) {
                        break; /* still moving */
                    }
                    e->flags_3e0 &= ~ENT_FLAG3E0_MOVING;
                    done += finish_head(ctx, e);
                } else if (type == 2) {
                    /* Target: step toward the payload entity. */
                    const EntitySlot* tgt = (const EntitySlot*)rec_payload(head);
                    if (tgt == NULL || step_toward(ctx, e, tgt->x, tgt->y, dt)) {
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
