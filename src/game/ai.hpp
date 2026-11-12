// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — AI execution receivers (finding 10, closed 2026-10-21).
 * The entity-manager frame dispatcher is owned by gods98/simtick.cpp.
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
 *   - DAT_004decc4[0..78] is the activity-name table the initializer at
 *     0x436c2f fills: Activity_Stand, Activity_Route, ..., Activity_TeleportIn
 *     (the 79-entry dump is documented in findings 10 and 464).
 *
 * This module ports the verified subset of the callback against the port's
 * entity mirror fields (flags_3e4/flags_3ec/activity/...), in corpus branch
 * order, with each branch naming its corpus address. Branches whose
 * subjects the port does not model yet (projectile arcs, screen-shake,
 * screen-space effects) are listed as TODOs in
 * finding 10 rather than guessed. Move commands now flow through the
 * ported path-list mirror (EntitySlot.wp, corpus +0x24/+0x28/+0x2c) and its
 * +0x3e0 bit-0x1 route flag. Kind-0 admission belongs to
 * cmd_move_route_prepare and the existing 0x404e00/0x441700 cell route APIs.
 * AI alone interpolates these routes in world space, converting its seconds
 * to the RouteSpeed helper's 25-Hz reference ticks. Failed admission waits
 * without a straight-line fallback.
 * ai_path_build remains a separate wave-flood diagnostic/convenience API.
 * Wake-timer expiry mirrors FUN_0044c110's writes onto the +0x364 target;
 * FUN_0044c030 creates and links its paired child through entity.cpp. The
 * cheat-word branch runs the real FUN_0044c530 can-do probe.
 */
#ifndef LEGORRR_GAME_AI_HPP
#define LEGORRR_GAME_AI_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Legacy target/work-path fallback (cells/second) and task timings.
 * Ordinary kind-0 motion requires terrain and loaded RouteSpeed; it does
 * not use this default or reinterpret drill times as walking speed. */
#define AI_MOVE_SPEED_CPS  2.0f
#define AI_TASK_DURATION   2.0f
#define AI_TASK_YIELD      2    /* deposit per completed gather task */

/* +0x3e0 bit 0x8 is the separate work-cell mover state (0x43cd66).
 * Ordinary kind-0 routes use the installer's bit 0x1, not this flag. */
#define ENT_FLAG3E0_MOVING 0x00000008u

/* --- activity state (FUN_004409e0 + DAT_004decc4) ---------------------- */
#define AI_ACTIVITY_COUNT 79
#define AI_ABILITY_TYPE_COUNT 6
#define AI_TOOL_TYPE_COUNT 8
#define AI_ACTIVITY_RESOURCE_SLOT_COUNT 11
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
    AI_ACT_OPEN,        AI_ACT_WAITING1,
    /* ... + AI_ACT_WAITING2..21 fill through index 76. */
    AI_ACT_CRUMBLE = 77, AI_ACT_TELEPORT_IN = 78
};

/* DAT_004decc4 lookup: activity name for `id` (NULL when out of range). */
const char* ai_activity_name(int id);
const char* ai_ability_type_name_get(int id);
const char* ai_tool_type_name_get(int id);
uint32_t ai_activity_resource_slot_get(int id);
/* FUN_00436c20: clear the entity-pool/grid-count state and install the
 * activity/ability/tool names and selectors. The separate idle-registration
 * table and global animation word are outside its cleared range. */
extern "C" void ai_activity_name_table_initialize(void);
extern "C" void ai_idle_chain_record_attach(EntitySlot* entity); /* FUN_00403f30 */

/* FUN_004409e0 semantics: set the entity's activity (name-table index),
 * store the param, reset the activity timer. Out-of-range ids ignored. */
extern "C" void ai_set_activity(EntitySlot* e, int id, int param);

/* EntityActivityToggle (FUN_004373d0): the activity on/off fold. Off with
 * the +0x3ec bit 13 raised drops it and raises bit 14, requesting the
 * 0x36 activity; on with bit 13 clear raises bit 12 and requests 0x35;
 * the corpus's activity-reregistration tail folds into the same call (the
 * port's last-name mirror is synced by ai_set_activity). Deviation: the
 * corpus strides the entity blindly. */
void entity_activity_toggle(EntitySlot* e, int on);

/* Tick-down marker flow (FUN_004489a0 / FUN_0041f840): the corpus creates
 * a kind-13 TickDown scene object when activity 6 is available. The port
 * models this as a type-13 entity at the source entity's pose. */
int entity_kind6_tick_down_marker_spawn(GameContext* ctx, EntitySlot* e);
int entity_remove_object_if_class6(GameContext* ctx, EntitySlot* e);

/* BubbleActionNameToIndex (FUN_00407300): the 39-entry case-insensitive
 * name table and its registration hook; on a miss it returns the final
 * case-insensitive comparison result, not a -1 sentinel. BubbleConfigTextureLoad
 * reads the Bubbles child list, honors the global !-entry gate, and routes
 * resource load/initialize operations through the host renderer. */
typedef struct BlockRec BlockFile;
int bubble_action_name_register(int index, const char* name);
int bubble_action_name_to_index(const char* name);
typedef uint32_t (*BubbleTextureResourceLoadFn)(
    void* context, const char* resource_name, int x, int y);
typedef void (*BubbleTextureInitializeFn)(void* context, uint32_t handle);
void bubble_texture_load_hooks_set(BubbleTextureResourceLoadFn load,
                                   BubbleTextureInitializeFn initialize,
                                   void* context);
void bubble_config_global_flags_set(uint32_t flags);
extern "C" void bubble_config_texture_load(BlockFile* cfg);
uint32_t bubble_texture_handle_peek(int index);
void bubble_texture_handle_set(int index, uint32_t handle);

/* Append a waypoint to the entity's path list (corpus +0x24 buffer, entry
 * stride 5 dwords — each entry stores x/y, two 0.5 interpolation weights,
 * and its pending/kind bytes; see AiWaypoint). Storage grows beyond the
 * inline span as needed. Returns the entry index or -1 on allocation failure.
 * Coordinates are numeric cells. A fully walked list restarts. This helper
 * does not admit a command or transfer movement ownership. */
int ai_path_append(EntitySlot* e, float x, float y);

/* Build a move command's path into the entity buffer through the ported
 * wave-flood pathfinder core (the corpus's FUN_00430ee0 seed + spread +
 * FUN_00430530 traceback on the port's level-grid handle — see ai.cpp).
 * The list is install-reset per command (corpus dispatch start, 0x4418d0)
 * and appended one entry per walked cell (builder appends, 0x44693b); the
 * flood work state is created and released inside the build, never shared
 * across commands. Targets are numeric cells; the actor start is world space.
 * Returns 0 when no path exists or waypoint allocation fails. Production
 * kind-0 commands use the shared command admission owner instead. */
int ai_path_build(GameContext* ctx, EntitySlot* e, float tx, float ty);

/* Wave-flood diagnostic: how many cells carried the wave flag 0x40000
 * during the last ai_path_build's spread (finding 10's FUN_00430ee0 mark
 * of a walked cell). Reset when a build starts, so it never reflects an
 * earlier command; 0 before any build, or when that build had no grid. */
int ai_wave_flood_count(void);

/* Run the game-layer command/task executor for `dt` seconds. The separate
 * 0x43b270 pool walk applies entity behavior and activity-timer updates. */
/* Per-entity LAB_0043c810 receiver, called by the 0x43b270 pool walk. */
int ai_entity_frame_update(EntitySlot* entity, float dt);
/* Game-layer command/task executor; EntityFrameUpdateDispatch owns the
 * separate entity behavior callback and activity timer updates. */
int ai_tick(GameContext* ctx, float dt);

/* Number of live command records across all entity chains (for logs/tests). */
int ai_commands_active(const GameContext* ctx);

/* The DAT_004eabcc idle table FUN_0043a960 registers into (100 max). */
int ai_idle_count(void);

/* --- legacy target/work-path rate fallback ---------------------------- */
/* Values are seconds/cell for the historical target/work adapter only.
 * Stats +0x24..34 are drill times in reference ticks, not walking rates.
 * Ordinary kind-0 walking uses entity_route_speed (Stats RouteSpeed +0x20). */
float ai_stats_speed(const GameContext* ctx, int tile_type);

/* --- permitted-activity list (+0x370 ids / +0x384 count) ----------------- *
 * The corpus's real can-do answer: FUN_0044b480 scans the entity's
 * permitted-activity list, FUN_0043c440 asks it about activities 4 (drill),
 * 5 (teleport) and 7 (reinforce), and FUN_0044c530 wraps that behind the
 * type-2 gate. Installs land through FUN_0044b370 (ai_activity_grant). */

/* FUN_0044b480: 1 when `activity` is on the entity's list (+0x370 scanned
 * through count +0x384), else 0; a zero count always answers 0. */
extern "C" int ai_activity_scan(const EntitySlot* e, int activity);

/* FUN_0043c440: which can-do activity the list carries — 1 = drill (4),
 * 2 = teleport (5), 3 = reinforce (7), 0 = none. */
int ai_activity_probe(const EntitySlot* e);

/* FUN_0044c530: the "can do" probe — 1 for a type-2 entity whose list
 * permits drill/teleport/reinforce, else 0. */
extern "C" int ai_can_do(const EntitySlot* e);

/* --- static task-state selector (spawn-dispatch switch 0x438150) --------- *
 * EntityCreateByType routes types 8, 9, 0xc..0xe, and 0x10..0x13 to one of
 * three static +0x334 templates. The port uses a matching named Stats row as
 * a +0xcc capacity adapter when available; retail's static template word is
 * zero, and no decompiled copy into the template is known. */
#define AI_TASK_STATE_NONE      (-1) /* no static task state (instance stats) */
#define AI_TASK_STATE_MINION_1   0  /* type 8   -> DAT_004a98f0 template */
#define AI_TASK_STATE_MINION_2   1  /* type 9   -> DAT_004a9a40 template */
#define AI_TASK_STATE_MINION_3   2  /* types 0xc..0xe, 0x10..0x13 -> 0x4a9b90 */

/* The 0x438150 selector: entity type -> static task-state id, or
 * AI_TASK_STATE_NONE for types the switch sends to the instance-stats arm
 * (the corpus falls back to FUN_00469c10 for everything else). */
int ai_minion_state_for(int type);

/* FUN_0046a060: the permitted-activity capacity the appender sizes and
 * recycles each entity's list from — corpus `*(ent+0x334 stats block) +
 * 0xcc`. Instance Stats records supply this directly. Static task-state
 * entities use the port's same-name Stats-row adapter, or zero when that row
 * is unavailable. Only entities with neither source use the context
 * fallback. The result is clamped to the inline span. */
extern "C" int ai_activity_cap(const EntitySlot* e); /* 0x0046a060 raw Stats +0xcc read */
int ai_activity_cap_for_context(const GameContext* ctx, const EntitySlot* e);

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
/* Retail's contiguous DAT_00557864..DAT_00557900 bubble-handle table. */
extern uint32_t s_bubble_texture_handles[39];
#endif

/* EntityQueueForRockfallProcessingIfEligible (FUN_0043a9e0): the corpus's
 * pure forwarder to FUN_0043a960, which owns every gate; the port mirrors
 * that — the type-1 test runs inside the carrier and the entity lands in
 * the idle table only when its stats gate passes and its current level tile
 * is type 9. Duplicate registrations consume slots, as in the reference;
 * the 100-entry extent is a high-water bound (findings 41, 457, 480). */
extern "C" void entity_queue_for_rockfall_processing_if_eligible(EntitySlot* e);
/* Ported activity-change tail (FUN_00447c40): attach the idle-chain record
 * when the activity changes, then refresh the activity timer. */
extern "C" int activity_register_bookkeeping(EntitySlot* e);

/* --- activity-request wrappers (2026-10-24; findings 448/223/377) -----------
 * EntityDispatchEvent5AndSetFlag (FUN_00447b00): requests corpus activity
 * index 5 through the activity setter — whose corpus sibling folds carry the
 * scene-activity dispatch bookkeeping — and raises bit 0x200000 of the
 * entity's +0x3e0 word. EntitySetActivityPlace (FUN_00448c50): the same for
 * corpus activity index 30 against bit 0x200 of +0x3e4, answering true.
 * (The corpus names suggest place-semantics for the second, but the name
 * table's index 30 entry is "Activity_Clear" — no semantic claim.) */
void entity_dispatch_event5_and_set_flag(EntitySlot* e); /* FUN_00447b00 */
extern "C" int  entity_set_activity_place(EntitySlot* e);           /* FUN_00448c50 */

/* --- entity activity scene-state leaf (2026-10-24; findings 251/469) -------
 * EntityActivitySceneStateUpdate (FUN_00439990): when the manager flag
 * word's bit 0x10 is raised it raises bit 8 and returns; otherwise it
 * raises bit 4 and clears the level object's +0x98 field through the
 * ported global-object mutator. The shared DAT_004dee14 word owns these
 * bits and the scatter-spawn gate. The corpus's global level pointer is
 * threaded as a GameContext argument (the module's ctx convention). */
void entity_activity_scene_state_update(GameContext* ctx); /* FUN_00439990 */
extern "C" void entity_activity_scene_state_update_retail(void); /* FUN_00439990 */
uint32_t entity_state_flags_peek(void);                    /* port hook */
void     entity_state_flags_set(uint32_t value);           /* port hook */
extern "C" uint32_t s_entity_state_flags; /* DAT_004dee14 shared state word */

/* SceneObjectHasTaskType4_5_7 (FUN_004493e0, finding 469): a type-2 entity
 * answers 1 when its activity list carries drill (4), teleport (5), or
 * reinforce (7) — the corpus's short-circuit chain, reproduced in order. */
extern "C" int scene_object_has_task_type_4_5_7(const EntitySlot* e); /* FUN_004493e0 */

/* --- entity activity gate band (2026-10-24; findings 251/469) -------------
 * EntityDispatchEvent31ForPendingFlag (FUN_004437f0): a type-3 entity whose
 *   +0x3e8 pending bit 0x80000000 is raised and whose +0x3e4 bit 0x20000000
 *   is not raises that bit and re-registers through the ported activity set
 *   with name-table index 0x31 (the dispatch bookkeeping folds there).
 * EntityActivity23StateTransition (FUN_00449a70): a type-2 entity raises
 *   +0x3e4 bit 0x800; then while the +0x3e0 bit 0x400 is clear it raises
 *   bit 0x40000000, re-registers through the activity set with index 0x23,
 *   and attaches the idle chain record (the ported 0x403f30 half).
 * Deviation: the corpus dereferences the entity blindly. */
void entity_dispatch_event31_for_pending_flag(EntitySlot* e);
                                                /* FUN_004437f0 */
extern "C" void entity_activity23_state_transition(EntitySlot* e); /* FUN_00449a70 */

/* TryRandomEntityActionFromCodeRange (FUN_00443670): while the +0x3e0 bit
 *   0x400 is clear and the mask-word test (FUN_0043baf0) answers 0, draw
 *   the shared random word and map it into 0x38..0x4c (% 0x15 + 0x38),
 *   set the activity, then use ActivityRegisterBookkeeping's return as the
 *   success test. Success raises +0x3e0 bit 0x8000; after five failed
 *   dispatches the helper answers 0. Type-2/type-3 scene runner callbacks
 *   are not installed by the current backend, so this path can remain
 *   unsuccessful at runtime. */
extern "C" int entity_random_activity_attempt(EntitySlot* e); /* FUN_00443670 */

/* --- entity counter + link band (2026-10-24; findings 251/469/435) --------
 * SimulationUpdateCounterAdd (FUN_0043a8f0): bumps the global object's
 *   +0x8C word `count` times through the ported mutator (the corpus's first
 *   argument is calling-convention residue), then runs the scene-state
 *   update.
 * EntityFlag80000000SetOrClear (FUN_00438580): the +0x3e8 high bit is raised
 *   when `param` is zero and cleared otherwise, then the scene-state update
 *   runs.
 * EntityNestedFloatAddUnlessBlocked (FUN_0043c280): a type-4 entity failing
 *   the shared eligibility gate skips the add; otherwise the entity's +0xDC
 *   stats float folds into `*value` (the corpus's float10 fold lands as a
 *   float add; the answer is zero on both paths).
 * LinkedEntityFlagAndCommandUpdate (FUN_004440a0): when the +0x2fc link
 *   lands on a type-4 entity, both entities raise the +0x3ec bit 0x40 and
 *   the parent's kind-0x1C command records are dropped through the ported
 *   chain drop (the trailing zero/one arguments are calling-convention
 *   residue).
 * Deviation: the corpus dereferences entities blindly; the guards and the
 * threaded GameContext stand in (the scene-state update's global level
 *   word is the module's ctx convention). */
extern "C" void simulation_update_counter_add(GameContext* ctx, int count);
                                                /* FUN_0043a8f0 */
void entity_flag80000000_set_or_clear(GameContext* ctx, EntitySlot* e,
                                      int param); /* FUN_00438580 */
int entity_nested_float_add_unless_blocked(EntitySlot* e, float* value);
                                                /* FUN_0043c280 */
void linked_entity_flag_and_command_update(EntitySlot* e);
                                                /* FUN_004440a0 */

/* --- object registry windows (2026-10-25) ---------------------------------
 * The 0x407xxx band registers entity records into four pointer/timer pair
 * windows (corpus DAT_005579e4 / 7a84 / 7b24 / 7bc4). The windows have no
 * other corpus owner, so they ride module storage; the type-1/2 idle queue
 * (window B) stays modeled by the cmd module's idle table. The record type
 * gates read EntitySlot::type; the port guards null records. */

/* EntityPointerRegistriesRemove (FUN_004073a0): drop the record pointer from
 * every window whose slot matches (the timer words ride on). */
extern "C" void entity_pointer_registries_remove(EntitySlot* record); /* FUN_004073a0 */

/* Type4ObjectRegistryInsert (FUN_00407410): a type-4 record goes into the
 * type-4 window when absent — first empty slot, timer word untouched. */
extern "C" void type4_object_registry_insert(EntitySlot* record);     /* FUN_00407410 */

/* Type2TimedRegistryInsert (FUN_004074b0): the type-2 timed window has 50
 * two-word slots. A hit re-seeds its timer; a miss stores into the first
 * empty slot and seeds it, while a full table drops the record. */
extern "C" void type2_timed_registry_insert(EntitySlot* record);      /* FUN_004074b0 */

/* SetType2ObjectRecordValue (FUN_00407510): a type-2 record broadcasts its
 * value into the timer word of every occupied timed-window slot. */
extern "C" void set_type2_object_record_value(EntitySlot* record, uint32_t value);
                                                /* FUN_00407510 */

/* FlaggedObjectTimerRegistryInsert (FUN_00407540): with the stats word's
 * bit 0x80 raised (the ported capability reader), register into the
 * flagged-timer window — hit re-seeds, miss appends at the first empty
 * slot with the seed, and a full window drops the record. */
extern "C" void flagged_object_timer_registry_insert(EntitySlot* record);
                                                /* FUN_00407540 */

/* port hooks: peek/set windows (0 = flagged, 1 = type-4, 2 = type-2 timed);
 * out-of-range slots guarded, counts via ai_object_registry_window_count. */
void ai_object_registry_slot_peek(int window, int slot, void** record,
                                  uint32_t* timer);
void ai_object_registry_slot_set(int window, int slot, void* record,
                                 uint32_t timer);
int  ai_object_registry_window_count(int window);
void ai_object_registries_reset(void);  /* port hook: clear (unowned) */

/* --- the idle-tile update band (2026-10-25) -------------------------------- */

/* EntityUpdateAndUnregisterAtTile (FUN_00440130): stride the idle-registration
 * table for entities standing on cell (x,y); each hit stores the link word
 * through the +0x2fc mirror, runs the linked-entity flag/command update,
 * and unregisters the slot. Deviation: the port scans its NULL-skipping
 * table where the corpus strides the global blindly. */
void entity_update_and_unregister_at_tile(int x, int y, void* link);
                                                /* FUN_00440130 */
void ai_idle_table_reset(void);  /* ActivityNameTableInit's idle-table slice */

/* TrackedEntityPointerArrayRemove (FUN_0043a930): walks the idle table's
 * used window zeroing every slot holding the given pointer (the walk
 * continues past a hit). Deviation: the corpus strides the global table
 * and its word count blindly; the port rides the ported model's guard. */
void ai_idle_table_pointer_remove(void* pointer);

/* EntityActivityTerrainEligibilityTransition (FUN_00449980): with the +0x3e0
 * bit 0x40 raised, probe whether the entity may still be placed at the
 * float cell after the CRT __ftol conversion.
 * On an ineligible cell: fold activity 11 in with argument 1, clear the
 * gate bit raising the eligibility-fail bit, and clear the cell's occupied
 * mark. On an eligible one with the activity timer spent: answer 1 without
 * touching state. Otherwise: fold activity 0 out, queue the cell's terrain
 * work transition (FUN_00431140), blank the +0x2f4/+0x2f8 pair to -1.0,
 * write 1 through the result word, and clear the occupied mark. */
extern "C" int entity_activity_terrain_eligibility_transition(GameContext* ctx,
                                                              EntitySlot* e, float x,
                                                              float y, int* out);
                                                /* FUN_00449980 */

/* --- pair predicate + active-list scan (2026-10-25) -----------------------
 * EntityStatePairPredicate (FUN_0043c200): reads stats table (type1, row1)
 *   (FUN_00469e00); a zero first read answers 1 without walking (pinned).
 *   Otherwise the filtered pool walk (FUN_004377b0's path: liveness mirror
 *   + raised +0x3e8 bit-0x80000 gate) folds each live entity's +0xDC stats
 *   float (FUN_0043c280) into an accumulator, then the re-read of
 *   (type1, row1) plus (type2, row2) plus the fold answers 0 when the sum
 *   is <= 0, else 1. Deviations: row/accumulator ride corpus-registered
 *   together (the port keeps them apart, acc resets to 0.0 first); the
 *   float10 fold lands as double; the walk composes the corpus's global
 *   type tables over the ctx blocks; NULL ctx skips the walk (reads still
 *   answer).
 * ActiveEntityListContainsCode (FUN_004031e0): the permitted-activity
 *   scan (FUN_0044b480) over the selection when it is non-empty (first-hit
 *   answer, branch isolation pinned), else over the corpus's global active
 *   entity list — DAT_004b335c/DAT_004b3424 are maintained from live pool
 *   create/teardown events and can be replaced through the install adapter
 *   (window guard 50). */
int entity_state_pair_predicate(GameContext* ctx, int type1, int row1,
                                int type2, int row2);
                                                /* FUN_0043c200 */
int active_entity_list_contains_code(GameContext* ctx, int code);
                                                /* FUN_004031e0 */
void active_entity_list_install(const EntitySlot* const* entries, int count);
int  active_entity_list_count(void);            /* port hooks */
const EntitySlot* active_entity_list_entry_get(int index);
void active_entity_list_entity_add(const EntitySlot* entity);
void active_entity_list_entity_remove(const EntitySlot* entity);
void active_entity_list_clear(void);

#endif /* LEGORRR_GAME_AI_HPP */
