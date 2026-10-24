// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — entity command primitives + the opcode queue executor
 * (finding 10).
 *
 * Port of the corpus's command subsystem (0x401–0x407) and the per-frame
 * queue executor FUN_004520d0:
 *
 *   CmdAlloc @ 0x406440  — 0x68-byte records from a doubling pool
 *   CmdPoolGrow @ 0x4064c0 — malloc(2^N * 0x68), linked into the free list
 *   CmdFree @ 0x406480  — cancel-ref, owned-buffer free, free-list push
 *   CmdRegistryInit @ 0x401cf0 — 31 task-type names, 27 priority names,
 *                          the 27-entry behaviour table
 *   CmdKindByName @ 0x401cb0 — priority name -> kind index
 *   CmdCreateMove/Target/Gated/Task @ 0x4035c0/0x4036c0/0x403610/0x402720
 *   MinionCompanionCmd @ 0x402850, CmdLinkHead/Tail @ 0x404030/0x404070,
 *   CmdChainWalk @ 0x404190, CmdChainPurge @ 0x402310,
 *   IdleQueueRegister @ 0x407450
 *   FUN_004520d0 — the 5-dword-record opcode switch (the queue executor
 *   ticker_004520d0 fills in tickers.cpp).
 *
 * Command record (0x68 bytes, finding 10 §"Command record"):
 *   +0x00 type (0 move / 2 target / 3 task / 5 gated / 7 zone / 0x13 ref)
 *   +0x04/0x08/0x0C params (coords for move)
 *   +0x10 payload ptr (entity / target), +0x14 "now" (timeGetTime on alloc)
 *   +0x1C behaviour id = behaviour[kind], +0x3C extra (task type-8 -> 8)
 *   +0x40 owned buffer, +0x4C kind index, +0x5C state flags, +0x60 next,
 *   +0x64 free-list link
 *
 * The AI *execution* receivers (which read +0x4c/+0x1c and run the task
 * switch — finding 10 §"Still open") are stubs here; this tranche covers
 * allocation, the registry, creation, linking, and queue dispatch.
 */
#ifndef LEGORRR_GAME_CMD_HPP
#define LEGORRR_GAME_CMD_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_RECORD_SIZE 0x68

/* Record state flags (+0x5c, finding 10). */
#define CMD_STATE_PENDING   0x10u
#define CMD_STATE_DONE      0x20u
#define CMD_STATE_BUFOUT    0x200u  /* +0x40 is external (do not free) */
#define CMD_STATE_MINION    0x2000u
#define CMD_STATE_CANCEL    0x10000u

/* --- pool + registry --------------------------------------------------- */
void cmd_init(void);            /* CmdRegistryInit */

void* cmd_alloc(int type);      /* CmdAlloc — a zeroed 0x68 record */
void  cmd_free(void* rec, int cancel); /* CmdFree */

/* Priority-name lookup (CmdKindByName): scans the 27-priority table,
 * returns 1 and sets *out when `name` matches, else 0. */
int  cmd_kind_by_name(const char* name, int* out);

/* Behaviour (AI task) id for a kind index (behaviour[kind], default 0x31). */
int  cmd_kind_behaviour(int kind);

/* Registry table names (slot = address order CmdRegistryInit writes them). */
const char* cmd_task_name(int slot);     /* 31 AI task types */
const char* cmd_priority_name(int slot); /* 27 AI priorities  */

/* --- entity-bound creators (findings 10) -------------------------------- */
void cmd_create_move(EntitySlot* e, float x, float y, int flag);
void cmd_create_target(EntitySlot* e, void* target);
void cmd_create_gated(EntitySlot* e, void* target);
void cmd_create_task(EntitySlot* e);   /* kind derived from the entity type */

/* --- chain ops ---------------------------------------------------------- */
/* CmdChainWalk: return the tail record of the entity's chain (or NULL). */
void* cmd_chain_walk(const EntitySlot* e);

/* FUN_00403d10 (generic drop): unlink and free every command record of the
 * given type from the entity's chain (the corpus walks the +0xbc record
 * list and matches record +0x00 == type; type-0x13 grant records have no
 * per-type side effects, so the drop is a plain unlink+free). Returns the
 * number of records dropped. */
int cmd_chain_drop(EntitySlot* e, int type);
void  cmd_chain_purge(EntitySlot* e);

/* Current global command-list head (DAT_004b3354) — the most recent
 * link_head'd record (tasks land here via CmdLinkHead). */
void* cmd_global_head(void);
/* Advance along the global list; cmd_rec_next advances an entity chain. */
void* cmd_global_next(const void* rec);

/* Unlink `rec` from the global command list (a record is also chained on
 * its entity; completion unlinks here before cmd_free). */
void cmd_global_unlink(const void* rec);

/* --- global tick list ---------------------------------------------------- */
void cmd_tick_global(void);     /* CmdTickGlobal @ 0x402220 (scoped) */

/* --- record accessors ---------------------------------------------------- */
int         cmd_rec_type(const void* rec);
const void* cmd_rec_payload(const void* rec);
int         cmd_rec_kind(const void* rec);
int         cmd_rec_behaviour(const void* rec);
/* The +0x38 command-granted activity id: the callback's deferred install
 * branch (0x43da62) appends it onto the entity's permitted-activity list
 * when the +0x3e4 bit-0x40 grant flag is up, then drops type-0x13 grant
 * records (cmd_chain_drop). The corpus writer of +0x38 is the task-state
 * granter — config/runtime-driven, not a shipped immediate (finding 10
 * §"Spawn-dispatch task-state switch"); port-side grants stage through
 * cmd_rec_set_activity. */
int         cmd_rec_activity(const void* rec);
void        cmd_rec_set_activity(void* rec, int act);
uint32_t    cmd_rec_state(const void* rec);
void*       cmd_rec_next(const void* rec);
void        cmd_rec_set_next(void* rec, void* next);

/* Move-target coords and the "now" timer on a record (semantic accessors;
 * the corpus stores these at +0x08/+0x0C and +0x14). */
float cmd_rec_move_x(const void* rec);
float cmd_rec_move_y(const void* rec);
float cmd_rec_now(const void* rec);
void  cmd_rec_set_now(void* rec, float v);

/* --- queue executor ------------------------------------------------------ */
/* The command layer's queue executor runs from GameContext-owned state; the
 * frame loop calls cmd_context_set once at startup. */
void cmd_context_set(GameContext* ctx);

/* Producer (FUN_00452060): append a 5-dword command record to the active
 * buffer; silent drop when the buffer is full. */
void cmd_produce(int op, const void* a1, int a2, int a3, int a4);

/* Queue a command for an entity (opcode, entity, param, extra…) — the
 * entity-aware convenience over cmd_produce. */
void cmd_enqueue(int opcode, const EntitySlot* e, int param,
                 int p1, int p2);

/* Cheat/debug keybinds (FUN_00452010): bind `key` to a command; the
 * executor's held-key pre-pass produces it whenever `keyheld(key)` is set. */
void cmd_bind_key(int key, int op, int a1, int a2, int a3, int a4);
void cmd_keybind_tick(int (*keyheld)(int key));

/* The per-frame executor (FUN_004520d0): dispatch pending records through
 * the opcode switch into the cmd primitives / manager stubs. */
void cmd_queue_tick(void);

/* Entity chain head on the slot (chain links live in the cmd pool). */
void cmd_entity_head(EntitySlot* e);

/* --- queue-adjacent leaves (2026-10-24) -----------------------------------
 * CommandQueueCancelOpcode (FUN_00451f60, findings 06/10/468): removes
 * every pending entry of the active bank whose second field matches the
 * argument, swap-with-last removals. The corpus's forward index skips a
 * swapped-in match (rechecked only at the next slot) — the port keeps
 * that shape faithfully.
 * ActiveContextRecordGet (FUN_00452580, finding 386): hands out the
 * active selection array and its count through the cmd context; clears
 * the output and answers NULL when nothing is selected.
 * ArrayFindValueIndex (FUN_00452650, finding 374): the linear value
 * search over an int array; the matched index lands when a non-NULL
 * output was given. NULL arrays skip (documented deviation). */
void cmd_queue_cancel_match(int value);
const void* active_context_record_get(int* count_out);
int cmd_array_find_value_index(const int* array, unsigned count, int value,
                               unsigned* index_out);
/* Tests/diagnostics view over the active-bank record count (the
 * message_active_count precedent). */
int cmd_queue_active_count_peek(void);

/* --- selection snapshot / dispatch (2026-10-24) -----------------------------
 * SelectionSnapshotCreate (FUN_00452520, findings 06/10/468): when the
 * selection is nonempty, allocates a copy through the port's heap model,
 * copies the entity pointers, and answers count + array with 1; the empty
 * selection answers 0 without writing. (The corpus copies `count &
 * 0x3fffffff` dwords and trails a dead zero-iteration byte loop — the
 * dead tail is simply absent; the copy is pointer-sized on the port.)
 * SelectionCommandDispatch (FUN_00452d60, findings 06/10, opcode 9's
 * helper): gate the current-selection context (flag word + bit 0x10 and
 * type-2/owner), else scan the selection array through the owner
 * indirection, and drive the sound-minimum-distance leaf with mode 0 and
 * the dispatch parameter as the scalar selector. NULL array output and
 * failed allocations skip (documented deviations). */
int selection_snapshot_create(void** array_out, unsigned* count_out);
int selection_command_dispatch(int param);

/* --- random-cell producer / entity remove (2026-10-24) ----------------------
 * EntityRandomValidCellCommandCreate (FUN_00452430, findings 06/10/468):
 * samples up to 1000 random map cells through the port's module-local
 * 15-bit LCG, and when the walkable-cell helper (its +0x3e8 cell-bit
 * bounded read) accepts one, queues operation 0x24 with the entity. The
 * corpus threads the coordinate pair as a stack pointer the dispatch may
 * not survive; the port stores the pair by value (documented deviation),
 * and zero/absent grid dimensions skip (the corpus would modulo by zero).
 * SelectionEntityRemove (FUN_00452c50, findings 06/10/468): the removal
 * half — scan for the entity's index, the index-zero HUD post is the
 * unported HUD-chain seam (no state effect here), swap-with-last removal,
 * and the selected-bit clear on both the reconstruction flag word and the
 * corpus's +0x3e8 mirror. Answers 1 only when it removed. */
void entity_random_valid_cell_command_create(void* entity);
int selection_entity_remove(void* entity);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_CMD_HPP */
