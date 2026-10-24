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

/* ObjectAttachedRecordAllocate (FUN_004033c0, findings 251/270): allocates a
 * kind-0x1C attached-object record through the ported allocator and hangs it
 * on the entity — after the chain's last record (the walk result's +0x60
 * link), or on the entity's chain head at +0x2F0 when the chain is empty.
 * Deviation: the corpus strides the entity blindly; the port guards it. */
void object_attached_record_allocate(EntitySlot* e); /* FUN_004033c0 */
/* PendingRecordCopyToEntityAndFlag (FUN_00406400): copies a command record
 * into a fresh allocation of the same type — the corpus's 26-field image
 * copy, then the pending flag (bit 0x200 of the state word) raised and the
 * chain link cleared so the copy starts unattached. (The corpus name's
 * "to entity" half never appears in its body: the copy is allocated, not
 * attached.) Deviations: the corpus strides both the source and the
 * allocation blindly; the port guards them and answers the copy pointer
 * for observability (the corpus answers nothing; the copy leaks the same). */
void* cmd_pending_record_copy(const void* rec); /* FUN_00406400 */

void  cmd_chain_purge(EntitySlot* e);

/* Current global command-list head (DAT_004b3354) — the most recent
 * link_head'd record (tasks land here via CmdLinkHead). */
void* cmd_global_head(void);
/* Advance along the global list; cmd_rec_next advances an entity chain. */
void* cmd_global_next(const void* rec);

/* Unlink `rec` from the global command list (a record is also chained on
 * its entity; completion unlinks here before cmd_free). */
void cmd_global_unlink(const void* rec);

/* PendingRecordEntityReferenceClear (FUN_004063e0): walks the global
 * command list and zeroes every record whose +0x48 pending-entity
 * reference equals the given reference (the walk continues past a hit).
 * Deviations: the reference rides the port's native-size pointer slot,
 * and the corpus writers of that word (the spawn-dispatch band) are
 * unported, so the slot stays zero until one installs a value. */
void pending_record_entity_reference_clear(void* reference);
void* cmd_rec_entity_ref(const void* rec);              /* +0x48 read */
void  cmd_rec_entity_ref_install(void* rec, void* ref); /* port hook */

/* EntityRecordAttachAndRelink (FUN_00406360): attaches a record to the
 * entity's command head (entity +0x2f0). A record without the +0x5c bit
 * 0x40 flag unlinks from the global list (head slot if prev is null,
 * else prev's link) and attaches in place; otherwise the entity attaches
 * to a cmd_pending_record_copy copy, and the +0x5c bit 0x40000 stamp
 * writes the corpus's 25.0-float bits into the copy-source record's
 * +0x18. Both lanes end with the entity-reference sweep and the idle
 * registration. Deviations: NULL record/entity guards and the
 * port-split global-list link (see the .cpp). */
void entity_record_attach_and_relink(void* record, void* previous,
                                     EntitySlot* entity);

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

#ifdef __cplusplus
}
#endif

/* --- the kind-7 zone-command word + cancel (2026-10-24; finding 252) ---------
 * CmdCancelByCoords (FUN_00403700): walks the global command list and
 * completes (state bit 0x20) every type-7 record whose zone word `+0x2c`
 * matches `zone`; when `probe` is zero the coordinate test is skipped,
 * otherwise the record's +0x20/+0x24 pair must equal (x, y). The corpus
 * writer of the zone word (the kind-7 issue-and-link helper) is
 * unported, so the field lands zero-initialized with accessors standing
 * in (documented deviation). */
void cmd_cancel_by_coords(int zone, int probe, int x, int y);
int  cmd_rec_zone_get(const void* rec);
void cmd_rec_zone_set(void* rec, int zone);
/* Public global-head link — CmdLinkHead @ 0x404030, exported for the
 * list-walking leaves (the corpus's own entry point). */
void cmd_link_global_head(void* rec, int kind);
/* Port-side staged writers for the cell-command slots (the corpus's
 * kind-7 issuer is unported). */
void cmd_rec_action_set(void* rec, int action);
void cmd_rec_target_set(void* rec, int target);

/* port hook: drop an entity from the idle-queue table (the window B model
 * of the object-registry band; EntityPointerRegistriesRemove calls it). */
void idle_queue_clear(EntitySlot* e);

/* SelectedEntitiesAnyPredicateMatch / AnyObjectCanOccupyTerrainCell
 * (FUN_00431a30 / FUN_00431ab0): the any-hit sweeps over the active
 * selection context's record array. Both pull the array through the
 * context-record getter and answer the first true predicate; an empty
 * array answers zero.
 * Deviations: the corpus strides the array blindly (the port guards the
 * count and the state block); the walkability predicate's entity-gated
 * mode-1 clauses are the documented seam pass — only the cell walkability
 * actually discriminates the sweep. */
int selected_entities_any_predicate_match(int cx, int cy);
                                                /* FUN_00431a30 */
int any_object_can_occupy_terrain_cell(int cx, int cy);
                                                /* FUN_00431ab0 */

/* CellCommandType3Issue (FUN_00403030): the kind-0xb cell-command issuer —
 * issues while the cell is walkable with the bounded flag 0x20 word clear;
 * stages the coordinate dwords (no act staging), applies the zero-state,
 * links onto the global head at kind 6, and rides the surface-type tail the
 * corpus's register-losing shape (deviations: guarded level/allocator). */
void cell_command_type3_issue(int x, int y);   /* FUN_00403030 */

/* EntityTaskBatchAppend (FUN_00403650): per entity in the array, allocates
 * a type-1 command, stages the payload word with a zero time word, sweeps
 * and relinks the entity's chain, then lands the record on the chain tail's
 * +0x60 link (or the entity +0x2f0 head when the chain is empty). Deviation:
 * guarded entity array where the corpus strides the words blindly. */
void entity_task_batch_append(EntitySlot* const* entities, int count,
                              void* payload);     /* FUN_00403650 */

/* DeferredOListEntityQueueAdd (FUN_00437c20): appends one entry into the
 * corpus's deferred ordered-list entity queue band — a 200-slot array of
 * 11-dword entries whose counter rides the band's own end word. Entry
 * words: 0/1 the grid-pick cell pair (ftol of the record's float pair,
 * FUN_0044f6d0 boundary), 2/3 the record pair, then params 5/1/2/3/6 and
 * two duplicated names (FUN_00477550). The hooks peek the band. */
void deferred_olist_entity_queue_add(GameContext* ctx, const uint32_t record[2],
                                     int p1, int p2, int p3, int p5, int p6,
                                     const char* name1, const char* name2);
uint32_t deferred_olist_entry_word(int slot, int word);
char*    deferred_olist_entry_string(int slot, int which);
uint32_t deferred_olist_counter_peek(void);
void     deferred_olist_reset(void);

/* --- HUD post / menu-selection chain (2026-10-25) --------------------------
 * CommandContextPostToHud (FUN_0041b2d0) refreshes the global state, updates
 * the menu selection's surface state and stores the map coordinate.
 * MenuSelectionResourceUpdate (FUN_0041b300) gates on the corpus's kind set
 * {1,2,3,4,5,7} and routes the surface-type work over the last tracked
 * coordinates through the frontend.* map-store model (DAT_004ddcf4/f8/cfc
 * live there; DAT_004de1f0 lands as a module static — the unowned-global
 * deviation). The corpus call sites lose the packer's third register: the
 * pair shape is pinned by the cmd.cpp tail precedent, so the tracked word
 * is the surface-type setter's type argument on the tracked half, and the
 * parameter half applies type 4 and tracks the prior value when it is
 * not 4. Guards: no-level and NULL-coordinate paths stay inert
 * (deviations: the corpus dereferences the current level blindly). */
void menu_selection_resource_update(int kind, const uint32_t coord[2]);
                                                  /* FUN_0041b300 */
void command_context_post_to_hud(int kind, const uint32_t coord[2]);
                                                  /* FUN_0041b2d0 */
void selection_clear_request_and_hud_reset(void); /* FUN_0041e750 */
uint32_t menu_surface_track_peek(void);           /* port hook: DAT_004de1f0 */

/* --- resource/ore counter pair (2026-10-25) --------------------------------
 * LevelResourceBalanceAddAndSound (FUN_0041f9a0) bumps the current level's
 * +0xa0 (selector 0) or +0xac (nonzero) counter when one is active, then
 * steps the overlay animation float once (selector 0) or five times
 * (nonzero); the corpus's sound-half is absent (the ledger name's sound
 * rides elsewhere in the callers). SimulationModeCounterAdd (FUN_0043a910)
 * is the loop form: run the zero-selector half `count` times, its first
 * parameter rides through with no observable effect (register residue).
 * Deviation: the corpus dereferences the current level blindly; the port
 * guards the no-level path (the counters and the overlay float still
 * ride — the corpus runs the advance outside the level gate). */
void level_resource_balance_add_and_sound(int selector);
                                                /* FUN_0041f9a0 */
void simulation_mode_counter_add(int mode, int count);
                                                /* FUN_0043a910 */

/* UpgradeResourceDebit (FUN_0041f9e0): subtract `amount` from the current
 * level's +0xa0 (selector 0) or +0xac (nonzero) under the level gate, then
 * step the overlay animation OUTSIDE the gate — pinned quirk: mode 0 here
 * (the add half is mode 1), and the nonzero selector steps the constant 5
 * regardless of the amount while selector 0 steps the amount.
 * UpgradeNameEntryFind (FUN_0042cd50): case-insensitive lookup over the
 * loader name table answering the matching 16-byte record's address, zero
 * on a cold table or miss. The loader-owned globals (record block,
 * name-pointer array, count) have no ported owner — the table rides
 * zero-init own storage behind install (window 36, deviations in the
 * module notes); NULL names skip the compare. */
void upgrade_resource_debit(int selector, int amount);
                                                /* FUN_0041f9e0 */
void upgrade_name_table_install(void* records, const char* const* names,
                                int count);     /* port hook */
const void* upgrade_name_entry_find(const char* name);
                                                /* FUN_0042cd50 */

/* KindSixRandomMessageDispatchSweep (FUN_0040f0e0): subtract the frame
 * delta from the sweep timer (DAT_004d78c4 own storage); once it reaches
 * zero, draw up to one hundred random grid pairs modulo the current
 * level's dimensions — the grid-spawn route is the quote-only seam
 * precedent (never lands, so the message-6 dispatch cannot fire) — and
 * re-arm the timer at (draw % 50) + 750.0 from one final draw, answering
 * draw / 50. A positive timer answers zero through the corpus's flag fold
 * (pinned); a missing level skips the cell loop but the re-arm draw runs
 * (deviation: the corpus strides a null handle and modulo zero blindly). */
uint32_t kind_six_random_sweep_dispatch(float delta);
                                                /* FUN_0040f0e0 */
void kind_six_sweep_timer_install(float value); /* port hook */
float kind_six_sweep_timer_peek(void);          /* port hook */

/* CommandSampleStatsAccumulate (FUN_0046b660): per-kind sliding-window
 * statistics over own storage. A null sample clears the head and total;
 * a sample slides the window when the head falls below its +8 word and
 * adds its +4 word to the running total, raising the high-water mark;
 * the pointer rides the kind's log and the index increments. Deviations
 * (kind guard, log-edge stop) in the module notes. Fields 20..24 are
 * index/head/total/sum/high. */
void command_sample_stats_accumulate(int kind, const void* sample);
                                                /* FUN_0046b660 */
uint32_t command_sample_stats_peek(int kind, int field); /* port hook */

#endif /* LEGORRR_GAME_CMD_HPP */
