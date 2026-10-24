// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — AI execution receivers (finding 10, closed 2026-10-21).
 *
 * The corpus entity-manager tick is FUN_0043b270: flag word DAT_004dee14
 * with pre/post passes, the entity walker FUN_004377d0(callback, &dt, 0),
 * the post-step ticker FUN_0044bf00, and the 125 s broadcast timer. The
 * callback (unnamed, entry 0x43c810 — Ghidra leaves it unattributed) is a
 * ~15 KB per-entity *behaviour evaluator*: it branches on entity flag words
 * (+0x3e0/+0x3e4/+0x3ec/+0x3e8), not on a command-record switch. The
 * earlier "record type 0=move / 2=target / 3=task handler" mapping was a
 * deterministic guess and is NOT corpus-faithful:
 *
 *   - No entity tick reads a command record's +0x1c behaviour id (verified
 *     across the whole .text: DAT_4b32e8 has xrefs only inside the command
 *     creation layer 0x401c00–0x4043ff). The task *names* are consumed by
 *     the creator side; the tick executes *flags*.
 *   - Behaviours are expressed on the entity as: flag bits (move = path
 *     list at +0x24 with +0x3e0 bit 0x8; follow = link handle at +0x354 and
 *     +0x3ec bit 0x80000), timers (+0x368 wake timer with +0x3e4 bit
 *     0x8000000, +0x2d4 activity timer), and the *activity* triple:
 *     +0x2e8 name pointer / +0x2d8 param / +0x2d4 timer, set by
 *     FUN_004409e0(ent, idx, p) = ent+0x2e8 = DAT_004decc4[idx];
 *     ent+0x2d8 = p; ent+0x2d4 = 0.
 *   - DAT_004decc4[0..76] is the activity-name table the initializer at
 *     0x436c2f fills: Activity_Stand, Activity_Route, ..., Activity_Waiting21
 *     (the full 77-entry dump lives in finding 10).
 *
 * This module ports the verified subset of the callback against the port's
 * entity mirror fields (flags_3e4/flags_3ec/activity/...), in corpus branch
 * order, with each branch naming its corpus address. Branches whose
 * subjects the port does not model yet (projectile arcs, screen-shake,
 * screen-space effects, the riding-attach writer) are listed as TODOs in
 * finding 10 rather than guessed. Move commands now flow through the
 * ported path-list mirror (EntitySlot.wp, corpus +0x24/+0x28/+0x2c), the
 * +0x3e0 bit-0x8 mover flag, and the tile walkability/occupancy queries
 * (#60); ai_path_build now runs the ported corpus wave-flood pathfinder
 * (FUN_00430ee0 seed + FUN_00430530 traceback on the port's level-grid
 * handle) per command, with the one-hop fallback kept for the no-path
 * and no-grid cases.
 * Wake-timer expiry mirrors FUN_0044c110's writes onto the +0x364 target,
 * and the cheat-word branch runs the real FUN_0044c530 can-do probe.
 */
#ifndef LEGORRR_GAME_AI_HPP
#define LEGORRR_GAME_AI_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Movement speed (cells/second) and task timings (port-side application
 * constants). AI_MOVE_SPEED_CPS is the *default* rate: with a grid bound
 * the mover steps at the speed of the terrain under foot instead, read
 * from the stats speed table behind FUN_00469cc0 (see below); without a
 * grid — or with unset table entries — this constant stands in. Corpus
 * movement is path-list driven, see the header note. */
#define AI_MOVE_SPEED_CPS  2.0f
#define AI_TASK_DURATION   2.0f
#define AI_TASK_YIELD      2    /* deposit per completed gather task */

/* +0x3e0 bit 0x8 — mover state: raised when a move command is applied
 * (creator side), cleared on path-list arrival (callback 0x43cd66). */
#define ENT_FLAG3E0_MOVING 0x00000008u

/* --- activity state (FUN_004409e0 + DAT_004decc4) ---------------------- */
#define AI_ACTIVITY_COUNT 77
#define AI_ACTIVITY_NONE  (-1)

/* Corpus indices into the DAT_004decc4 table (initializer @ 0x436c2f). */
enum AiActivity {
    AI_ACT_STAND = 0,   AI_ACT_ROUTE,        AI_ACT_ROUTE_RUBBLE,
    AI_ACT_RUN_PANIC,   AI_ACT_DRILL,        AI_ACT_TELEPORT,
    AI_ACT_WALK,        AI_ACT_REINFORCE,    AI_ACT_REVERSE,
    AI_ACT_TURN_LEFT,   AI_ACT_TURN_RIGHT,   AI_ACT_CANT_DO,
    AI_ACT_EMERGE,      AI_ACT_ENTER,        AI_ACT_ENTER_REIN,
    AI_ACT_COLLECT,     AI_ACT_GATHER,       AI_ACT_CARRY,
    AI_ACT_CARRY_RUBBLE, AI_ACT_THROW,       AI_ACT_CARRY_TURN_LEFT,
    AI_ACT_CARRY_TURN_RIGHT, AI_ACT_CARRY_STAND,
    AI_ACT_HIT_LEFT,    AI_ACT_HIT_RIGHT,    AI_ACT_HIT_FRONT,
    AI_ACT_HIT_BACK,    AI_ACT_HIT_HARD,     AI_ACT_DYNAMITE,
    AI_ACT_DEPOSIT,     AI_ACT_CLEAR,        AI_ACT_PLACE,
    AI_ACT_REPAIR,      AI_ACT_SLIP,         AI_ACT_REST,
    AI_ACT_EAT,         AI_ACT_STAMP,        AI_ACT_THROW_MAN,
    AI_ACT_THROWN_BY_ROCK_MONSTER,           AI_ACT_GET_UP,
    AI_ACT_BUILD_PATH,  AI_ACT_UPGRADE,      AI_ACT_EXPLODE,
    AI_ACT_UNPOWERED,   AI_ACT_FIRE_LASER,   AI_ACT_FREEZED,
    AI_ACT_FREEZE_START, AI_ACT_FREEZE_MELT, AI_ACT_RECHARGE,
    AI_ACT_WAKE_UP,     AI_ACT_TRAIN,        AI_ACT_FLOAT_ON,
    AI_ACT_FLOAT_OFF,   AI_ACT_OPENING,      AI_ACT_CLOSING,
    AI_ACT_OPEN,        AI_ACT_WAITING1
    /* ... + AI_ACT_WAITING2..21 fill up to AI_ACTIVITY_COUNT. */
};

/* DAT_004decc4 lookup: activity name for `id` (NULL when out of range). */
const char* ai_activity_name(int id);

/* FUN_004409e0 semantics: set the entity's activity (name-table index),
 * store the param, reset the activity timer. Out-of-range ids ignored. */
void ai_set_activity(EntitySlot* e, int id, int param);

/* Append a waypoint to the entity's path list (corpus +0x24 buffer, entry
 * stride 5 dwords — the port stores {x, y, pending} per entry; see
 * AiWaypoint). Returns the entry index or -1 when the port bound
 * (AI_WAYPOINTS_MAX) is reached. Move commands seed one entry via
 * cmd_create_move; a fully popped list restarts, as the corpus rebuilds
 * the buffer per path command. */
int ai_path_append(EntitySlot* e, float x, float y);

/* Build a move command's path into the entity buffer through the ported
 * wave-flood pathfinder core (the corpus's FUN_00430ee0 seed + spread +
 * FUN_00430530 traceback on the port's level-grid handle — see ai.cpp).
 * The list is install-reset per command (corpus dispatch start, 0x4418d0)
 * and appended one entry per walked cell (builder appends, 0x44693b); the
 * flood work state is created and released inside the build, never shared
 * across commands. Returns the entry count, or 0 when no path exists (no
 * grid, blocked target, unreachable, longer than AI_WAYPOINTS_MAX) and
 * the caller seeds a one-hop fallback. */
int ai_path_build(GameContext* ctx, EntitySlot* e, float tx, float ty);

/* Wave-flood diagnostic: how many cells carried the wave flag 0x40000
 * during the last ai_path_build's spread (finding 10's FUN_00430ee0 mark
 * of a walked cell). Reset when a build starts, so it never reflects an
 * earlier command; 0 before any build, or when that build had no grid. */
int ai_wave_flood_count(void);

/* Run the entity-manager tick for `dt` seconds: walk live entities and
 * apply the verified callback branches in corpus order, then step the
 * command chains. Returns the number of commands completed this tick. */
int ai_tick(GameContext* ctx, float dt);

/* Number of live command records across all entity chains (for logs/tests). */
int ai_commands_active(const GameContext* ctx);

/* The DAT_004eabcc idle table FUN_0043a960 registers into (100 max). */
int ai_idle_count(void);

/* --- per-terrain movement speed (FUN_00469cc0) -------------------------- */
/* Port of the `FUN_00469cc0(ent, tile_type)` speed probe: reads the stats
 * speed table (`GameContext.stats_speed_sec` — the port model of the
 * `+0x334` stats block's `+0x24 + terrain_type * 4` table) by terrain
 * index. Returns the corpus value: seconds to cross one cell of that
 * terrain. Zero entries and out-of-range types fall back to the
 * AI_MOVE_SPEED_CPS-equivalent default (the corpus indexes blindly and
 * gates movement on entry != 0 behind the unported +0x3e8 walker bit). */
float ai_stats_speed(const GameContext* ctx, int tile_type);

/* --- permitted-activity list (+0x370 ids / +0x384 count) ----------------- *
 * The corpus's real can-do answer: FUN_0044b480 scans the entity's
 * permitted-activity list, FUN_0043c440 asks it about activities 4 (drill),
 * 5 (teleport) and 7 (reinforce), and FUN_0044c530 wraps that behind the
 * type-2 gate. Installs land through FUN_0044b370 (ai_activity_grant). */

/* FUN_0044b480: 1 when `activity` is on the entity's list (+0x370 scanned
 * through count +0x384), else 0; a zero count always answers 0. */
int ai_activity_scan(const EntitySlot* e, int activity);

/* FUN_0043c440: which can-do activity the list carries — 1 = drill (4),
 * 2 = teleport (5), 3 = reinforce (7), 0 = none. */
int ai_activity_probe(const EntitySlot* e);

/* FUN_0044c530: the "can do" probe — 1 for a type-2 entity whose list
 * permits drill/teleport/reinforce, else 0. */
int ai_can_do(const EntitySlot* e);

/* --- minion task-state (spawn-dispatch switch 0x438150) ------------------ *
 * The corpus's entity spawn dispatcher (FUN_00437d00) routes minion types
 * through a selector at 0x438150: type − 8 indexes the byte table at
 * 0x4382ac and jumps through (0x438298) to a per-state arm that points the
 * entity's +0x334 stats block at a static template — the minion "task state".
 * The same stats block is what the permitted-activity appender reads for
 * its per-entity capacity (FUN_0046a060 = state +0xcc; see ai_activity_cap
 * and EntitySlot.activity_cap). Ids mirror the three template addresses. */
#define AI_TASK_STATE_NONE      (-1) /* no minion task state (instance stats) */
#define AI_TASK_STATE_MINION_1   0  /* type 8   -> DAT_004a98f0 template */
#define AI_TASK_STATE_MINION_2   1  /* type 9   -> DAT_004a9a40 template */
#define AI_TASK_STATE_MINION_3   2  /* types 0xc..0xe, 0x10..0x13 -> 0x4a9b90 */

/* The 0x438150 selector: entity type -> minion task-state id, or
 * AI_TASK_STATE_NONE for types the switch sends to the instance-stats arm
 * (the corpus falls back to FUN_00469c10 for everything else). */
int ai_minion_state_for(int type);

/* FUN_0046a060: the permitted-activity capacity the appender sizes and
 * recycles each entity's list from — corpus `*(ent+0x334 stats block) +
 * 0xcc`. The port models the per-entity value as `EntitySlot.activity_cap`
 * (0 = not set -> GameContext.stats_activity_cap, the game-wide default
 * from game_context_init), clamped to the inline span. */
int ai_activity_cap(const GameContext* ctx, const EntitySlot* e);

/* FUN_0044b370: install one activity grant onto the entity's list — the
 * command/dispatch install appender. Semantics (corpus-pinned): capacity
 * check (FUN_0046a060 via ai_activity_cap), +0x3ec bit
 * 0x4 reflects "a special (4/5/7) activity is permitted", a special value
 * REPLACES the first special entry already on the list (clearing the old
 * grant in place, 0x44b463), and otherwise the value appends — slot[count],
 * count++ while count != cap (0x44b44a), or the list shifts left by one and
 * the value lands in the last slot when full (0x44b417). */
void ai_activity_grant(GameContext* ctx, EntitySlot* e, int value);

#ifdef __cplusplus
}
#endif

/* EntityQueueForRockfallProcessingIfEligible (FUN_0043a9e0): the corpus's
 * tail-call wrapper registering a type-1 entity for rockfall processing
 * — it just runs FUN_0043a960's idle-registration half, so the port's
 * wrapper models that: the type gate runs and the entity lands in the
 * idle table through the dedupe/capacity rule (the stats-flag word and
 * the rockfall-tile probe stay unmodeled deviations — findings 41/457). */
void entity_queue_for_rockfall_processing_if_eligible(EntitySlot* e);

#endif /* LEGORRR_GAME_AI_HPP */
