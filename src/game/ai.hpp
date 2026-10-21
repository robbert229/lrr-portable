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
 * screen-space effects, riding) are listed as TODOs in finding 10 rather
 * than guessed. Command records keep their role as the *application* layer
 * (they seed the path/link state in the real engine); the port's move/
 * target stepping below stays that application until the path lists are
 * ported — flagged below, no longer presented as "the handler switch".
 */
#ifndef LEGORRR_GAME_AI_HPP
#define LEGORRR_GAME_AI_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Movement speed (cells/second) and task timings (port-side application
 * constants — corpus movement is path-list driven, see header note). */
#define AI_MOVE_SPEED_CPS  2.0f
#define AI_TASK_DURATION   2.0f
#define AI_TASK_YIELD      2    /* deposit per completed gather task */

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

/* Run the entity-manager tick for `dt` seconds: walk live entities and
 * apply the verified callback branches in corpus order, then step the
 * command chains. Returns the number of commands completed this tick. */
int ai_tick(GameContext* ctx, float dt);

/* Number of live command records across all entity chains (for logs/tests). */
int ai_commands_active(const GameContext* ctx);

/* The DAT_004eabcc idle table FUN_0043a960 registers into (100 max). */
int ai_idle_count(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AI_HPP */
