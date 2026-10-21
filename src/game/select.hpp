// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — selection manager + pointer input (finding 06).
 *
 * Reconstructs the corpus's selection cluster up to the pieces the command
 * queue feeds: DAT_004fef2c (selection array), DAT_004ff298 (count /
 * SelectedEntity gate), the add-to-selection path (dedupe, cap, selected
 * flag) and the opcode-8 (-add N to selection) + opcode-5 (position) routes
 * through FUN_004520d0 (CommandQueueTick, ported in src/game/cmd.cpp).
 *
 * `selection_pick` maps a viewport/cell position to the nearest placed
 * entity — the "click the grid → select" half the reconstruction demo
 * drives with the tracked mouse (the platform event layer carries motion,
 * not buttons, so a key triggers the pick).
 */
#ifndef LEGORRR_GAME_SELECT_HPP
#define LEGORRR_GAME_SELECT_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Add `e` to the selection (dedupe, cap SELECTION_MAX, mark selected).
 * Returns 1 when newly added. */
int selection_add(GameContext* ctx, EntitySlot* e);

/* Remove `e` from the selection (clears its selected flag). */
void selection_remove(GameContext* ctx, const EntitySlot* e);

void selection_clear(GameContext* ctx);
int  selection_count(const GameContext* ctx);

/* SelectedEntity (0x4525c0): first selection entry when nonzero, else NULL. */
EntitySlot* selection_first(const GameContext* ctx);

/* Pick the placed entity nearest grid cell (cx,cy) within `tol` cells.
 * Returns NULL when nothing is close enough. */
EntitySlot* selection_pick(GameContext* ctx, float cx, float cy,
                           float tol);

/* Enqueue an opcode-5 (position) command for every selected entity toward
 * grid cell (tx,ty) — the pointer-driven "move" action (finding 06, op 5/6
 * → CmdCreateMove). Returns the number enqueued. */
int selection_cmd_move(GameContext* ctx, float tx, float ty);

/* Enqueue the opcode-8 selection-add for `e` (flags op path). */
void selection_cmd_add(GameContext* ctx, EntitySlot* e);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SELECT_HPP */
