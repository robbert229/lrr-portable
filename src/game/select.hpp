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
                                     /* FUN_004525f0 SelectionRecordCountGet */

/* SelectedEntitySetHasAny (FUN_004525b0): nonzero when the selection is
 * non-empty — the corpus's gate on DAT_004ff298. */
int selection_has_any(const GameContext* ctx);

/* SelectionRecordArrayGet (FUN_004525e0): the DAT_004fef2c selection
 * array itself. NULL GameContext yields NULL (the port has no static). */
EntitySlot* const* selection_record_array(const GameContext* ctx);

/* SelectionRecordCountReset (FUN_00452600): zeroes only the count — the
 * corpus leaves entries and their selected flags in place (snapshot paths
 * overwrite the array); `selection_clear` is the flag-clean variant. */
void selection_count_reset(GameContext* ctx);

/* SelectedEntity (0x4525c0): first selection entry when nonzero, else NULL. */
EntitySlot* selection_first(const GameContext* ctx);

/* SelectionStateTick (FUN_004528c0, finding 468): the selection filter
 * pass. Walk the whole selection and classify it — kind 2 without the
 * +0x36c owner (context 9, scan ends there); kind 1 split clean/flagged
 * by the stats word's bits 0x20000/0x2000000 and the +0x3ec bit 0x40;
 * kind 4 (context 0xd); kind 0xb with the +0x3e4 bit 0x2000000 (context
 * 0xe); and the bit-0x20000-with-no-owner class (context 0xc). The class
 * ladder posts one HUD context (CommandContextPostToHud ported), silences
 * the lower classes, and a second walk keeps only entries in the chosen
 * class — swap-with-last compaction that revisits the swapped-in slot —
 * clearing the selected bit (the +0x3e8 bit 0x200000 mirror) on every
 * dropped entity. Quirks pinned: the class-none shape posts 0 and drops
 * every entry, and the swap compaction revisits the swapped-in slot
 * (an entry removed twice loses the selected bit twice — inert).
 * Deviations: the widened owner mirror (pointer convention), the +0x3ec
 * low-byte read, the NULL-slot guard. */
void selection_state_tick(GameContext* ctx);

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

/* SelectionContainsEntity (FUN_00452610): the dedupe scan — nonzero when
 * `e` is in the selection array below the count. NULL context answers 0
 * (the corpus reads its global array blindly). */
int selection_contains_entity(const GameContext* ctx, const EntitySlot* e);

/* SelectionCandidateAlreadySelectedOrMarked / SelectionCandidateEligibilityCheck
 * (FUN_00452690 / FUN_00452870, findings 77/85/468): the selection filter's
 * candidate leaves. Already-selected-or-marked answers through the
 * selection array or the +0x3ec word's bit 2 (the corpus reads that word
 * as a byte; the flag-word read is the documented deviation). The
 * eligibility leaf requires the +0x3e8 bit-0x100 state gate, the ported
 * shared-eligibility predicate, the +0x3e4/+0x3e0 clear-bit gates, and
 * the type-1 +0x3e0 bit-0x40000 rejection. NULL entities reject both. */
int selection_candidate_already_selected_or_marked(const GameContext* ctx,
                                                   const EntitySlot* e);
/* The eligibility leaf is declared in entity.hpp (implemented there). */

/* CurrentLevelStateFlagsContainMask (FUN_0041f720): ANDs the selected
 * entity's +0x3f0 level-state mask word. Deviation: with an empty
 * selection the corpus dereferences its NULL accessor blindly; the port
 * answers false (no flags contained). The +0x3f0 word itself is a
 * zero-init mirror — see game.hpp EntitySlot. */
int current_level_state_flags_contain_mask(const GameContext* ctx,
                                           uint32_t mask);

#endif /* LEGORRR_GAME_SELECT_HPP */
