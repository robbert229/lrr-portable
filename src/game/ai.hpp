// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — AI execution receivers (finding 10 §"Still open").
 *
 * The corpus's entity-manager tick (FUN_0043b270, the fifth subsystem
 * ticker in the EngineTick dispatch) that reads the command records created
 * by src/game/cmd.cpp and *runs* them is not decompiled (the evidence bundle
 * covers the allocation/registry/creator side only). This module is the
 * reconstruction of that receiver, ported from the documented semantics:
 *
 *   - move   (record type 0): step the entity toward rec->x/rec->y
 *   - target (record type 2): step toward rec->payload (an EntitySlot*)
 *   - task   (record type 3): the behaviour→effect subset that drives the
 *             mission counters (finding 09): priority kinds Crystal (0) /
 *             Ore (1) / DefaultCollect (2) / Construction (4) deposit their
 *             yield into GameContext.mission on completion.
 *
 * The exact handler mapping inside FUN_0043b270 is unverified — the port
 * keeps a deterministic subset with the constants below factored so a later
 * byte-exact pass can slot the real switch in. When the chain head's
 * command finishes (state |= CMD_STATE_DONE) it is unlinked and freed, so
 * the next queued command becomes active — the corpus's completion/cancel
 * flow (CmdChainPurge).
 *
 * This closes the loop: selection input (select.*) -> queue (cmd.*) ->
 * execution (here) -> mission counters (mission.*) -> objectives/save.
 */
#ifndef LEGORRR_GAME_AI_HPP
#define LEGORRR_GAME_AI_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Movement speed (cells/second) and task timings. */
#define AI_MOVE_SPEED_CPS  2.0f
#define AI_TASK_DURATION   2.0f
#define AI_TASK_YIELD      2    /* deposit per completed gather task */

/* Run the entity-manager tick for `dt` seconds: step every entity whose
 * chain head is a move/target, run task yields, unlink finished commands.
 * Returns the number of commands completed this tick. */
int ai_tick(GameContext* ctx, float dt);

/* Number of live command records across all entity chains (for logs/tests). */
int ai_commands_active(const GameContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AI_HPP */
