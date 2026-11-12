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
 * The AI execution receiver and command/task executor are implemented in
 * ai.cpp. This module owns record allocation, registry, creation, linking,
 * pending assignment, and opcode queue dispatch.
 */
#ifndef LEGORRR_GAME_CMD_HPP
#define LEGORRR_GAME_CMD_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_RECORD_SIZE 0x68
#define CMD_PENDING_COMMAND_COUNT_TABLE_SIZE 0x12c0

/* Entity-type/action pending-command count table (DAT_004b34f4). */
extern uint32_t cmd_pending_command_counts[
    CMD_PENDING_COMMAND_COUNT_TABLE_SIZE];
/* Shared command-operation word (DAT_004b8060). */
extern int s_global_enabled;

/* Record state flags (+0x5c, finding 10). */
#define CMD_STATE_PENDING   0x10u
#define CMD_STATE_DONE      0x20u
#define CMD_STATE_BUFOUT    0x200u  /* +0x40 is external (do not free) */
#define CMD_STATE_MINION    0x2000u
#define CMD_STATE_CANCEL    0x10000u
/* Bit1 latches issuance, never final arrival. Kind0 uses the same bit. */
#define CMD_STATE_ISSUED     0x1u
#define CMD_STATE_MOVE_ROUTED CMD_STATE_ISSUED

/* --- pool + registry --------------------------------------------------- */
void cmd_init(void);            /* CmdRegistryInit */

void* cmd_alloc(int type);      /* zeroed retail window + private tail */
void  cmd_free(void* rec, int cancel); /* CmdFree */
void cmd_minion_companion(EntitySlot* entity);

/* Priority-name lookup (CmdKindByName): scans the 27-priority table,
 * returns 1 and sets *out when `name` matches, else 0. */
int  cmd_kind_by_name(const char* name, int* out);

/* Behaviour (AI task) id for a kind index (behaviour[kind], default 0x31). */
int  cmd_kind_behaviour(int kind);

/* Registry table names (slot = address order CmdRegistryInit writes them). */
const char* cmd_task_name(int slot);     /* 31 AI task types */
const char* cmd_priority_name(int slot); /* 27 AI priorities  */

/* --- entity-bound creators (findings 10) -------------------------------- */
/* Mapped ABI: the pair is two OPAQUE dwords, copied to +8/+c. For kind 0
 * those words encode signed integer cells, including queue opcodes 5/6. */
void cmd_create_move(EntitySlot* e, const float position_xy[2], int flag);
/* Shared kind-0 admission/lifecycle for the command scheduler and ai_tick.
 * Prepare installs at most one cell route; failure leaves the head pending.
 * AI alone advances that route. Finish returns 1 only for endpoint arrival
 * (or explicit DONE cancellation); malformed/failed routes retain the head. */
int cmd_move_route_prepare(EntitySlot* entity, struct LevelState* level);
int cmd_move_route_finish(EntitySlot* entity);
/* GetTool alone shares the ordinary mover. Scheduler issues; AI advances;
 * terminal verification raises bit40; the existing callback grants/removes. */
int cmd_gettool_route_moving(EntitySlot* entity);
int cmd_gettool_route_finish(EntitySlot* entity);
int cmd_gettool_route_arrived(EntitySlot* entity);
void cmd_gettool_route_reset(EntitySlot* entity);
void cmd_gettool_entity_removing(EntitySlot* entity);
void cmd_create_target(EntitySlot* e, void* target);
void cmd_create_gated(EntitySlot* e, void* target);
/* EntityActivityRecordRemoveByType (FUN_00403c00) passes 0x43160000 (150.0f
 * bits) for action-record removals; the port wires that through its +0x2f0
 * command-chain representation. */
extern "C" void cmd_create_task(EntitySlot* e, int32_t parameter = 0); /* +0x18 argument */
extern "C" void object_command_task_create(EntitySlot* entity, int parameter);
int object_path_submit_to_grid_target(EntitySlot* entity,
                                      struct LevelState* level,
                                      const int target_xy[2],
                                      const float* final_marker_position); /* 0x404e00 */
typedef struct CommandKindRecordHooks {
    void* context;
    float (*random_range)(float minimum, float maximum, void* context);
    void (*terrain_cell_update)(const int cell_xy[2], int a, int b, int c,
                                void* context);
} CommandKindRecordHooks;
void cmd_kind_record_hooks_set(const CommandKindRecordHooks* hooks);
void cmd_command_kind_record_issue(uint32_t a, uint32_t b, uint32_t c,
                                   const int cell_xy[2], uint32_t zone,
                                   const float* random_pair, int set_flag);
void cmd_validated_record_queue(void* record); /* FUN_004030e0 */

/* --- chain ops ---------------------------------------------------------- */
/* CmdChainWalk: return the tail record of the entity's chain (or NULL). */
void* cmd_chain_walk(const EntitySlot* e);
int cmd_chain_contains_kind(const EntitySlot* e, int kind);
typedef int (*CmdPurgeCellPredicate)(int x, int y, void* context);
typedef int (*CmdPurgeCompare)(const void* left, const void* right,
                              void* context);
void cmd_chain_purge_hooks_install(CmdPurgeCellPredicate cell_usable,
                                   CmdPurgeCellPredicate neighbor_eligible,
                                   CmdPurgeCompare compare, void* context);
/* Host helper for an entity-owned chain; retail CmdChainPurge operates on
 * a global command-list head and is implemented in cmd.cpp. */
void cmd_chain_purge(EntitySlot* e);

/* Convenience for callers that use the common unlink-and-count form of
 * FUN_00403d10. Returns the number of matching records removed. */
int cmd_chain_drop(EntitySlot* e, int type);
/* Full EntityCommandUnlinkByKind entry (FUN_00403d10). `extra` is the
 * type-specific payload (an EntitySlot for types 3 and 7).
 * `unlink` controls detachment from the entity chain and pool release. */
extern "C" void entity_command_unlink_by_kind(EntitySlot* e, int type,
                                   const void* extra, int unlink);

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
void* cmd_global_pending_head(void);
void* cmd_global_next(const void* rec);
/* Advance along the global list; cmd_rec_next advances an entity chain. */
void* cmd_global_next(const void* rec);

/* Unlink `rec` from the global command list (a record is also chained on
 * its entity; completion unlinks here before cmd_free). */
void cmd_global_unlink(const void* rec);
void pending_command_timer_fire_scan(void); /* 0x405950 */
void pending_command_entity_assignment_tick(void); /* 0x405c10 */
/* PlaceCommandEligibilityCheck (0x404fc0), evaluated against the live
 * command/entity and the operations bound from cmd_context_set. Position
 * outputs are world coordinates; cell outputs are grid coordinates. */
int cmd_place_command_eligibility_check(void* record, EntitySlot* entity,
                                        float position_xy[2],
                                        float cell_xy[2], uint32_t* out_flag,
                                        int require_command_gate,
                                        int relaxed_entity_gate);

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
void cmd_tick_global(void);     /* CmdTickGlobal @ 0x402220 */
/* Runtime ticker entry supplies the caller-stack delta consumed by 0x404250. */
void cmd_tick_global_delta(float delta);
int cmd_entity_command_chain_tick(EntitySlot* entity, void* delta_payload); /* 0x404280 */
typedef struct CmdEntityChainExternalOps {
    void* context;
    int (*object_position_step)(EntitySlot* entity, uint32_t selector,
                                void* context); /* kind 0x13, 0x44b500 */
    int (*waypoint_task_step)(EntitySlot* entity, const void* target_record,
                              uint32_t task_count, void* context); /* 0x444690 */
} CmdEntityChainExternalOps;
/* Kind19 defaults to the real 0x44b500 leaf; it pends without its scene
 * producer. Overrides must install a genuine terminal-d route. Kind21 pends
 * without its external owner. Negative=pending, zero=failure, positive=issued. */
void cmd_entity_chain_external_ops_set(const CmdEntityChainExternalOps* ops);

/* --- record accessors ---------------------------------------------------- */
int         cmd_rec_type(const void* rec);
const void* cmd_rec_payload(const void* rec);
int         cmd_rec_kind(const void* rec);
int         cmd_rec_behaviour(const void* rec);
/* Port-side granted-activity value used by the deferred install path; this
 * staged value is outside the retail +0x38 record word. */
int         cmd_rec_activity(const void* rec);
/* Raw +0x38 dword read by PendingTypeValueSlotSelect for kind 0x13. */
uint32_t    cmd_rec_value_38(const void* rec);
void        cmd_rec_set_activity(void* rec, int act);
uint32_t    cmd_rec_state(const void* rec);
void*       cmd_rec_next(const void* rec);
void        cmd_rec_set_next(void* rec, void* next);

/* Numeric cell adapters for kind 0 (never reinterpret its words as floats).
 * Other kinds retain their existing float-field semantics. */
float cmd_rec_move_x(const void* rec);
float cmd_rec_move_y(const void* rec);
void cmd_rec_move_xy_set(void* rec, float x, float y);
float cmd_rec_now(const void* rec);
void  cmd_rec_set_now(void* rec, float v);

/* --- queue executor ------------------------------------------------------ */
/* The command layer's queue executor runs from GameContext-owned state; the
 * frame loop calls cmd_context_set once at startup. */
void cmd_context_set(GameContext* ctx);

/* Producer (FUN_00452060): append a 5-dword command record to the active
 * buffer; the optional pair supplies record words 3 and 4, otherwise both
 * are zero. The corpus silently drops when the buffer is full. */
extern "C" void cmd_produce(int op, const void* a1, int a2, const int* pair);

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
 * samples up to 1000 random map cells through the shared VC5 CRT random stream
 * on the classic x86 build (a module-local 15-bit LCG on other builds), and
 * when the walkable-cell helper (its +0x3e8 cell-bit
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
/* SelectionObjectDetach (FUN_004524b0): cancel matching queued commands,
 * remove the entity from selection, and restore the media/min-distance
 * context when it points at the entity. The separate DAT_005560e8 owner
 * comparison/render-position side effect remains unavailable here. */
void selection_object_detach(void* entity);
int selected_entity_detach_callback(EntitySlot* entity); /* FUN_0041f6c0 */

/* --- selection filter / state monitor (2026-10-24) ---------------------------
 * SelectionCandidateFilter (FUN_004526e0, findings 06/10/77/85/468): the
 * insert half of the selection filter — follow the +0x304 attach pointer,
 * run the ported eligibility and already-selected leaves, and (below the
 * corpus's hundred-entry cap) append the candidate with the +0x3e8
 * selected bit set. The marker/HUD reset, the activity refresh / action
 * reset / turn-issue leaves, and the visual-resource route are unported
 * entity/render seams; the type-2 turn branch additionally needs the
 * engine view the cmd context does not carry.
 * SelectedEntityStateMonitor (FUN_00452cc0, findings 06/10): `count`
 * passes over the FIRST selection entry (the corpus counter never
 * advances — faithful), clearing the attached owner's +0x3e4 bit 3 and
 * owner word when its stats word bit 0x20000 is set. The per-type
 * teardown calls are the unported entity-layer teardown family, like the
 * media mode-leaf's hooks. NULL entries skip (deviation). */
int selection_candidate_filter(void* entity, int mode, int param_3);
/* SelectionCandidateFilterDefault (FUN_004526c0): forwards `mode` to the
 * filter with its third argument fixed at 1. */
void selection_candidate_filter_default(void* entity, int mode);
int selected_entity_state_monitor(void);

/* --- entity record schedulers (2026-10-24) ----------------------------------
 * EntityCommand15ValidatedRecordQueue (FUN_00402670, findings 362/465): with
 * the validator above, allocates a type-0x1a record, links it (kind 0xf),
 * stores the entity at +0x10 and raises the +0x5c bits 6/14/18 after
 * clearing bit 4. The corpus's first statement calls CurrentLevelGet for
 * its touch only; the port's link lands the record on the task list.
 * EntityDeferredWorkSchedule (FUN_004026c0, finding 465): with the +0x304
 * attach mirror clear and the +0x3ec bit-0x100 flag clear, searches live
 * command records for a type-3 entry whose payload is this entity before
 * creating a fallback task. NULL entity answers inert (deviation). */
void entity_command15_validated_record_queue(EntitySlot* e);
extern "C" void entity_deferred_work_schedule(EntitySlot* e);

/* --- registry accessors / match marker (2026-10-24) --------------------------
 * CmdMarkMatching (FUN_00402000, finding 466): walks the task list, raising
 * the record's +0x5c bit 0x10000 when its type matches and either the
 * payload equals the second argument or the record's x/y dwords equal the
 * supplied coordinate pair (the corpus compares int views; the port reads
 * the float bits through memcpy — same equality).
 * CmdSetKind (FUN_00402060, findings 10/466): records the kind at +0x4c and
 * the behaviour at +0x1c — or 99 when the record's state carries the
 * 0x30000 marker bits.
 * PriorityEntryOrderValueSet (FUN_00402090): writes the order table (the
 * port's s_behaviour, DAT_004b32e8) at the kind slot — guarded (deviation)
 * since the corpus writes blind; this feeds cmd_kind_behaviour.
 * PriorityEntryStateSet / PriorityEntryStateGet (FUN_004020a0 / FUN_004020b0,
 * finding 466): the 27-entry per-priority state table (DAT_004b7ff4, zeroed
 * by the registry initializer — a zero-init static here, the initializer
 * seam); guarded access (deviation). */
extern "C" void cmd_mark_matching(int type, int payload, const int coords[2]);
void cmd_set_kind(void* rec, int kind);
void priority_entry_order_value_set(int kind, int value);
void priority_entry_state_set(int kind, int value);
int  priority_entry_state_get(int kind);

/* Diagnostic: overwrite a record's +0x5c state word. */
void cmd_rec_state_set(void* rec, uint32_t state);

/* Diagnostic: read a record's +0x18 task-param word. */
int cmd_rec_param2_get(const void* rec);

/* --- registry flush / wrappers (2026-10-24) ---------------------------------
 * CmdFlushAll (FUN_00402100, finding 466): the nonzero argument clears bit
 * 1 of the registry word (the port's s_global_enabled view); zero runs the
 * entity callback sweep (the unported LAB seam — inert), sets the bit,
 * drops both list heads, and zeroes the 27-entry priority-state table.
 * The 0x12c0-entry cell-record counter block is the live
 * cmd_pending_command_counts table, shared with the terrain-cell task sweep.
 * EngineMemoryAllocate / EngineMemoryFree (FUN_004021e0 / FUN_00402210):
 * the engine-heap wrapper pair — allocate-and-lose (the corpus discards
 * the return) and free.
 * HandleArrayReleaseAndReset (FUN_004020c0, finding 393): releases each
 * nonzero handle of the 12-dword global handle array with the heap free,
 * then zeroes the array's companion counter and the registry word. No
 * ported code feeds the array, so the release half is inert over the
 * zero-init static (documented), and the reset halves are observable.
 * EntityCommand14BatchIssue (FUN_00402800, finding 10): `count` (zero
 * means one) type-4 records, kind 0x14 — the zero argument additionally
 * raises each record's +0x5c bit 6.
 * LevelNoGatherObjectSetup (FUN_00402170): optionally issues that batch,
 * then pushes the two level-setup records: the type-0x18 record bound to
 * kind 0x17 with bit 6 raised (corpus's manual tail push — ported through
 * the tail link, deviation), and the type-0x1b record head-linked at kind
 * 0 with bits 6/7/10 raised and bit 4 cleared. */
void cmd_flush_all(int disable_mode);
int  cmd_global_enabled_peek(void);
void* engine_memory_allocate(long size);
extern "C" void engine_memory_free(void* ptr);
extern "C" void handle_array_release_and_reset(void);
extern "C" void entity_command14_batch_issue(int count);
extern "C" void level_no_gather_object_setup(int mode);

/* TaskTableContentsRelease (FUN_00462960): drain-and-free the contents of a
 * task-table record window — count at +0x1c, entry-pointer array at +0x20,
 * each entry carrying a sub-record slot at +0x14. Every entry (and any
 * sub-record) goes through the heap free, then the array and the record
 * itself; the corpus answers 1 (kept). The window rides the port's widened
 * slot shape: entry and sub-record slots are host pointers read through
 * the record's byte window, and the count and array slots are re-read
 * every round, as the corpus's aliasing reloads do. Deviations: the port
 * guards NULL windows and NULL array slots (the corpus strides both
 * blindly); the caller frees nothing extra — this owns the whole tree. */
int task_table_contents_release(void* record);      /* FUN_00462960 */

/* --- cancellation-ref and conditional issuers (2026-10-24) --------------------
 * CmdCancelRef (FUN_00402cd0, findings 10/466): run by CmdFree unless the
 * second argument suppresses it. Kind0x13 with a reference searches the
 * live COMMAND pool with a pure predicate (native402d30); its discarded
 * result has no cancellation writes. Any
 * other type with a referenced record walks the global list and raises bit 5
 * (the completion/cancel flag) on each type-0x13 record that *is* that
 * referenced record. NULL is guarded (deviation: the corpus reads blindly).
 * EntityConditionalTaskSchedule (FUN_00402ec0, finding 466): with the
 * capability word's bit 0 raised and the +0x3e0 bit-0x400 gate and the
 * +0x3ec bit-0x10000 latch clear, allocates the type-0x1e record, latches
 * bit 16, head-links it at kind 0x11, and raises bits 4/20 and clears bit 6.
 * EntityThresholdProgressEventQueue (FUN_00402f30, finding 466): allocates
 * the type-10 record, stores the payload plus the +0x3c word 0x10, and
 * links it at kind 0x13 (raising bit 21) when the flag is set, else kind 7.
 * EntityTargetCommandQueue (FUN_00402f70, finding 466): allocates the
 * type-0x11 record for the target and chains it onto the entity command
 * chain through the CmdChainWalk tail (or the +0x2f0 head) — no link-head,
 * no timestamp write (the corpus writes only the chain pointer). */
void cmd_cancel_ref(void* rec);
extern "C" void entity_conditional_task_schedule(EntitySlot* e);
extern "C" void entity_threshold_progress_event_queue(void* payload, int flag);
void entity_target_command_queue(EntitySlot* e, void* target);

/* Diagnostics: overwrite a record's +0x58 reference slot and its +0x00 type
 * word (tests stage corpus-shaped records no ported producer creates). */
void cmd_rec_ref_set(void* rec, void* ref);
void* cmd_rec_ref_get(const void* rec);
void cmd_rec_type_set(void* rec, int type);

/* --- the selection-record array and the entity-kind issuer (2026-10-24) -----
 * EntityRecordArrayApplyZeroState (FUN_00402600, findings 06/10): when the
 * selected-entity set is non-empty, raises the record's bits 6/8, takes the
 * selection snapshot into the record's +0x40 owned buffer with the entry
 * count at +0x44, and walks it. Per entry the corpus runs
 * EntityActionSequenceReset and — with the mode argument set —
 * EntityAttachedObjectListDetach; both live in the unported entity-teardown
 * family, so the traversal is the seam half (documented) while the guard,
 * the bits, and the snapshot ownership are observable. NULL record guarded.
 * EntityKindCommandIssue (FUN_00402960, finding 466): once per entity — the
 * +0x3e4 bit-0x10 latch blocks the second pass. Allocates the type-0x12
 * record, and picks the +0x3c word from the capability word's bit 0x100 and
 * the stats word's bits 0x1000000/0x2000000 (1, or 2/4/1 — nothing when the
 * pair reads clear). With the capability bit 0x40 and the +0x3e0 bit-0x400
 * gate clear it raises bit 20, runs the record-array half with the inverted
 * bit-20 answer, clears bit 6, and head-links at kind 0xc.
 * CoordinatePairCallbackDispatch / ArgumentCallbackDispatch (FUN_00402a40 /
 * FUN_00402aa0, findings 362/465): the callback dispatchers — the pair
 * wrapper and the direct-argument dispatch. Their whole observable body is
 * the unported pool-callback machinery, so the port records the seams. */
void entity_record_array_apply_zero_state(void* rec, int mode);
extern "C" void entity_kind_command_issue(EntitySlot* e);
typedef int (*CoordinatePairEntityCallback)(EntitySlot* entity,
                                            const void* coordinate_pair,
                                            int extra, void* context);
typedef int (*ArgumentEntityCallback)(EntitySlot* entity, void* argument,
                                      void* context);
typedef int (*TaskCallbackEntityFn)(EntitySlot* entity, void* payload,
                                    void* context);
void cmd_callback_dispatch_hooks_install(
    CoordinatePairEntityCallback coordinate_callback,
    ArgumentEntityCallback argument_callback, void* context);
extern "C" void coordinate_pair_callback_dispatch(const void* coord_pair);
void argument_callback_dispatch(void* arg);
void task_callback_dispatch_hook_install(TaskCallbackEntityFn callback,
                                         void* context);
void task_callback_dispatch(void* payload); /* FUN_00403a10 */
void pending_record_callback_dispatch_hook_install(
    TaskCallbackEntityFn callback, void* context);
void pending_record_callback_dispatch(void* payload); /* FUN_00403ef0 */
typedef struct EntityTaskQueueFlushHooks {
    void* context;
    int (*scheduler_blocked)(void* context);
    void (*remove_activity)(EntitySlot* entity, int mode, void* command,
                            void* context);
    void (*queue_relink)(EntitySlot* entity, void* context);
    void (*state_recompute)(EntitySlot* entity, void* context);
} EntityTaskQueueFlushHooks;
void entity_task_queue_flush_hooks_set(
    const EntityTaskQueueFlushHooks* hooks);
void entity_task_queue_flush(EntitySlot* entity); /* FUN_004040a0 */

/* Diagnostics: the record's +0x44 buffer entry count and +0x3c word. */
unsigned cmd_rec_count_get(const void* rec);
int cmd_rec_extra_get(const void* rec);

/* --- terrain cell-command issuers (2026-10-24) ------------------------------
 * The cell-command family allocates a record for a grid cell: the pair lands
 * at the +0x08/+0x0c slots as integers (the corpus writes them as ints where
 * the move records store floats — the port reads the same slots through an
 * int view, deviation noted), followed by the action word, the -1 target
 * marker and the zero entry count. Each links at a cell-command kind and
 * runs the record-array half. The counter update that follows the link
 * belongs to the spatial-query/action-issue path; where that path has no
 * eligible entity it increments cmd_pending_command_counts.
 * TerrainCellCommandCreate (FUN_00402510): all three state guards must come
 * out clear — the state-equals-1 and state-equals-6 tests and the in-bounds
 * flag-bit-4 test. Then the type-8 record, the flag word's bit 3 with the
 * caller's flag, the record-array half, the kind-3 link, bit 7 (with bit 13
 * additionally when the second flag is set), and — with the caller's flag
 * clear — the surface-type apply and the cell's flag-bit-0x800000 mark.
 * KindNineCellCommandIssue (FUN_00402d50): the state byte must not be 1, the
 * walkable flag must answer and the reservation bit must be clear. Then the
 * type-9 record with action 8, the bit-10 raise, the record-array half with
 * the mode argument clear, the kind-3 link, the cell's occupied mark, and
 * the surface-type apply; answers 1 when issued (the corpus's tail loses the
 * surface setter's register arguments — the type follows the 0x402510 shape).
 * CellCommandType13Issue (FUN_00402e40): with the bounded flag-bit-8 read
 * answering, the type-0x14 record with action 0xd, the bit-10 raise, the
 * record-array half and the kind-3 link.
 * CellCommandType11Issue (FUN_00402fb0): no guard — the type-0xf record with
 * action 0xb, the bit-10 raise, the record-array half, the kind-4 link, and
 * the cell's secondary bit 3. */
void terrain_cell_command_create(int x, int y, int flag, int raise20);
int  kind_nine_cell_command_issue(int x, int y, int mode);
void cell_command_type13_issue(int x, int y);
void cell_command_type11_issue(int x, int y);

/* Diagnostics: the cell-command record's int views of the coordinate slots
 * and its action/target words. */
int cmd_rec_cell_xy_get(const void* rec, int* x, int* y);
int cmd_rec_action_get(const void* rec);
int cmd_rec_target_get(const void* rec);

/* TaskType23CreateForCategoryMask (FUN_00402890, findings 327/449/465):
 * when the entity's state word contains the selector's single bit the
 * type-0x17 task record is allocated for the entity with the selector
 * kept in the +0x3c word, the record-array half runs with the caller's
 * argument, and the task links with the category kind the selector maps
 * to — 1/2/4 to 0xc, 8 to 3, 0x10 to 7, 0x20 to 4, every other value to
 * 0xd — clearing bit 6 afterwards. NULL entity guarded (deviation). */
extern "C" void task_type23_create_for_category_mask(EntitySlot* e, int selector,
                                          int issue_records);

/* --- the task-kind filtered search (2026-10-24; findings 378/381/449) --------
 * EntityActionKindSupported (FUN_0043a7c0): with the stats word's bit 3 set,
 * the action kinds 9, 8, 0xb and 0xd answer unconditionally, kind 6 asks the
 * callable no-op (its return is corpus-undefined, so the port treats the arm
 * as always missing), and kind 7 asks the upgrade-resource counters (kind-7
 * argument; no active level answers zero). Kinds outside the set miss.
 * TaskEntityFlagsAndCellTaskMatch (FUN_00439240): the pool-search callback —
 * the same stats gate with the task kind and argument forwarded.
 * TaskKindFilteredSearch (FUN_00439200): selectors 1, 2 and 4 never search;
 * anything else walks the entity pool (table order, ascending index, live
 * slots) through the callback. */
int entity_action_kind_supported(EntitySlot* e, int kind, int arg);
int task_entity_flags_and_cell_task_match(const EntitySlot* e, int kind);
int task_kind_filtered_search(int selector);

/* --- the task-record queue producers (2026-10-24; findings 366/369/465) ------
 * The kind-tagged queue family: each allocates its record kind, stages the
 * corpus's words, assigns the command kind and lands the record. Coordinate
 * pairs ride the coordinate slots as integers (the port reads them back
 * through cmd_rec_cell_xy_get). DAT_004b3354 and DAT_004b3358 are independent
 * global-list roots sharing each record's +0x60 link. The timed pair
 * (kind 5/0x11) stores its arguments in the
 * cell-command slots and zeroes the timestamp slot before linking.
 *  CoordinatePairEventQueue (FUN_004030b0): kind 0xc, pair plus the +0x54
 *      event payload, head-linked as command 8.
 *  ObjectEventType2Queue (FUN_00403190): kind 0x13, the argument at the
 *      +0x38 slot, the reference slot zeroed, the record-array half,
 *      bit 6 cleared and bit 4 raised, head-linked as command 2.
 *  CoordinatePairTaskRecordQueue (FUN_004033f0): kind 0, pair, bit 4
 *      cleared and bit 6 raised, command kind 0x1a, tail-appended.
 *  ScalarTaskRecordQueue (FUN_004034a0): kind 0x1d, the scalar at +0x10,
 *      command 0x19, tail-appended, bit 6 raised after.
 *  TaskPairRecordEnqueue (FUN_004034e0): kind 0x19, pair, command 0x18,
 *      tail-appended.
 *  TaskRecordKind10Queue (FUN_00403520): kind 10, payload, command 0x16,
 *      tail-appended, bit 6 raised after.
 *  TaskRegistryRemoveType0EntryByPair (FUN_00403430): unlink-and-free the
 *      first kind-0 record whose pair matches (the free runs the cancel ref).
 *  TaskRegistryRemoveType10EntriesForEntity (FUN_00403560): unlink-and-free
 *      every kind-10 record whose +0x10 payload is the entity.
 *  TimedTaskRecordInitializeAndLink (FUN_004038c0): kind 5, payload, the
 *      timestamp word zeroed, the three arguments at +0x20/+0x24/+0x28,
 *      command 10, bit 4 cleared and bits 6/7/10 raised; returns the record.
 *  TaskRecordKind11TimedQueue (FUN_004039d0): kind 0x11, payload, the
 *      timestamp word zeroed, command 0, bit 4 cleared and bits 6/7 raised. */
extern "C" void coordinate_pair_event_queue(const int pair[2], void* payload);
extern "C" int  object_event_type2_queue(void* payload);
void coordinate_pair_task_record_queue(const int pair[2]);
void scalar_task_record_queue(void* scalar);
void task_pair_record_enqueue(const int pair[2]);
void task_record_kind10_queue(void* payload);
void task_registry_remove_type0_entry_by_pair(const int pair[2]);
void cmd_tutorial_congregation_set(int x, int y, int enable);
void task_registry_remove_type10_entries_for_entity(const EntitySlot* e);
void* timed_task_record_initialize_and_link(void* payload, int arg1,
                                            int arg2, int arg3);
void task_record_kind11_timed_queue(void* payload);

/* Diagnostic: the record's +0x54 event-payload slot. */
void* cmd_rec_event_payload_get(const void* rec);
int cmd_rec_cell_count_get(const void* rec); /* +0x28 word (timed arg 3) */
/* EntityPoolCallbackSearch's CmdRec-pool half of TerrainCellRefreshDispatch
 * (LAB_00402bb0): compare live type-8/9/11 task coordinates, set state bit
 * 0x20, and clear that cell's occupancy bit for each match. */
void cmd_pool_cell_refresh_dispatch(const int cell_xy[2], int mode);

/* --- the entity command-chain housekeeping (2026-10-24; findings 369/465) ----
 * EntityCommandChainHeadRemove (FUN_004041e0): pops the entity chain head.
 * Pending-but-undone heads move to the global head list; the corpus's second
 * arm (test $0x20,%ch — state bit 16, Ghidra's 0x2000 mislabel) undone heads
 * to the tail list — both raw inserts (no timestamp, no flag edits) —
 * and everything else is freed. Returns the entity's new chain head.
 * EntityDrainCommandChain (FUN_00403a50): gated on DAT_004b8060 bit 1 clear:
 * pending-undone records head-link at their own kind, bit-16 undone ones
 * tail-link, and everything else frees; the entity chain drops to NULL.
 * The corpus's pool-search tail half (the callback raising bit 5 on records
 * whose +0x10 payload is the entity) is the DAT_004b31c8 registry seam.
 * EntityTaskStateUpdateAfterCleanup (FUN_00403af0): with the argument clear
 * the chain head's bit 0 drops; with it set and the head type 0, the head
 * comes off and a kind-3 entity additionally queues operation 7. The corpus
 * reads the +0xbc record list — the port models it as the chain head
 * (deviation, same convention as cmd_chain_drop).
 * EntityPendingTaskFlagClear (FUN_00403b40): bit 0 off the chain head.
 * TaskRecordKind14RemoveIfHead (FUN_00403f60): a kind-0xe head comes off.
 * CommandQueueSweepRelink (FUN_00404100): the chain sweep — flag-bit-2
 * records free, pending-undone ones go raw onto the head list, bit-16
 * undone ones raw onto the tail list, everything else stays. The corpus
 * relinks every keeper through the HEAD keeper's link slot only, so an
 * intermediate keeper is orphaned (the corpus quirk, pinned).
 * EntityTaskChainContainsKind (FUN_004041b0): any chain record of the type. */
void* entity_command_chain_head_remove(EntitySlot* e);
void entity_drain_command_chain(EntitySlot* e);
void entity_task_state_update_after_cleanup(EntitySlot* e, int flag);
void entity_pending_task_flag_clear(EntitySlot* e);
void task_record_kind14_remove_if_head(EntitySlot* e);
void command_queue_sweep_relink(EntitySlot* e);
int  entity_task_chain_contains_kind(const EntitySlot* e, int type);

#ifdef __cplusplus
}
#endif

/* C++ convenience: numeric cell coordinates, truncated to integers before
 * entering the opaque mapped constructor. These are not world values.
 * Keep this overload outside the C-linkage block. */
void cmd_create_move(EntitySlot* e, float x, float y, int flag);

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
/* Retail IdleQueueRegister @ 0x407450; refreshes an existing type-2 slot or
 * claims the first empty slot with its 12.5 second timer. */
void idle_queue_register(EntitySlot* e);

/* SelectedEntitiesAnyPredicateMatch / AnyObjectCanOccupyTerrainCell
 * (FUN_00431a30 / FUN_00431ab0): the any-hit sweeps over the active
 * selection context's record array. Both pull the array through the
 * context-record getter and answer the first true predicate; an empty
 * array answers zero.
 * The walkability sweep retains the per-entity gates and forwards the
 * optional stats-bit requirement. The port guards the state block and
 * empty array, which the corpus strides blindly. */
extern "C" int selected_entities_any_predicate_match(int cx, int cy, int require_stat_bit = 0);
                                                /* FUN_00431a30 */
extern "C" int any_object_can_occupy_terrain_cell(int cx, int cy);
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
 * words: 0/1 the terrain-derived cell pair (FUN_0044f6d0), 2/3 the world
 * pair, then params 5/1/2/3/6 and
 * two duplicated names (FUN_00477550). The hooks peek the band. */
extern "C" void deferred_olist_entity_queue_add(GameContext* ctx, const uint32_t record[2],
                                     int p1, int p2, int p3, int p5, int p6,
                                     const char* name1, const char* name2);
uint32_t deferred_olist_entry_word(int slot, int word);
char*    deferred_olist_entry_string(int slot, int which);
uint32_t deferred_olist_counter_peek(void);
void     deferred_olist_reset(void);
struct DeferredOListEntityRecord;
unsigned int deferred_olist_entity_queue_take_cell(const int cell_xy[2],
    DeferredOListEntityRecord* records, unsigned int capacity);

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
extern "C" void command_context_post_to_hud(int kind, const uint32_t coord[2]);
                                                  /* FUN_0041b2d0 */
extern "C" void selection_clear_request_and_hud_reset(void); /* FUN_0041e750 */
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
extern "C" void simulation_mode_counter_add(int mode, int count);
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
 * level's dimensions, testing each through the erosion eligibility and
 * mutation path and dispatching message 6 for the first accepted pair — and
 * re-arm the timer at (draw % 50) + 750.0 from one final draw, answering
 * draw / 50. A positive timer answers zero through the corpus's flag fold
 * (pinned); a missing level skips the cell loop but the re-arm draw runs
 * (deviation: the corpus strides a null handle and modulo zero blindly). */
extern "C" uint32_t kind_six_random_sweep_dispatch(float delta);
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
extern "C" void command_sample_stats_accumulate(int kind, const void* sample);
                                                /* FUN_0046b660 */
uint32_t command_sample_stats_peek(int kind, int field); /* port hook */

#endif /* LEGORRR_GAME_CMD_HPP */
