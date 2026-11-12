// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — selection manager + pointer input (finding 06).
 *
 * Reconstructs the corpus's selection cluster up to the pieces the command
 * queue feeds: DAT_004fef2c (selection array), DAT_004ff298 (count /
 * SelectedEntity gate), the add-to-selection path (dedupe, cap, selected
 * flag), opcode-8 selected-set movement, and opcode-5 individual movement
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
typedef void (*SelectedObjectOutlineDrawFn)(void* context, EntitySlot* entity,
                                            float red, float green,
                                            float blue);
void selected_object_outlines_draw(
    const GameContext* ctx, SelectedObjectOutlineDrawFn draw,
    void* context); /* FUN_00424830 */

/* Context-explicit selection array helper. */
EntitySlot* const* selection_record_array(const GameContext* ctx);

/* SelectionRecordCountReset (FUN_00452600): zeroes only the count — the
 * corpus leaves entries and their selected flags in place (snapshot paths
 * overwrite the array); `selection_clear` is the flag-clean variant. */
void selection_count_reset(GameContext* ctx);

/* Bind the active application context used by the retail global-selection
 * entry point. */
void selection_active_context_bind(GameContext* ctx);
void selection_active_context_sync(GameContext* ctx);
extern "C" void selection_clear_active(void);
int selection_entity_remove_active(EntitySlot* entity);
int selection_candidate_filter_active(void* entity, int mode, int param_3);

/* Retail active-selection global accessors. */
extern "C" EntitySlot* const* selection_active_context_record_get(int* count_out);
extern "C" EntitySlot* const* selection_active_records_get(void);
extern "C" uint32_t selection_active_count_get(void);
extern "C" int selection_active_has_any_get(void);
void selection_active_count_reset(void);

/* SelectedEntity (0x4525c0): zero-argument retail getter over the active
 * selection globals. */
extern "C" EntitySlot* selection_active_entity_get(void);
extern "C" uint32_t selection_contains_entity_active_target(
    EntitySlot* entity, uint32_t* index_out);

/* Context-explicit helper for portable callers. */
EntitySlot* selection_first(const GameContext* ctx);

/* SelectedEntitiesAnyMatch (FUN_0041f420): call `predicate` in selection
 * order with its shared 32-bit argument; stop at the first nonzero result.
 * A NULL context, callback, or entry is treated as no match (the corpus
 * uses a global selection and dereferences its entries directly). */
int selected_entities_any_match(const GameContext* ctx,
                                int (*predicate)(EntitySlot*, uint32_t),
                                uint32_t argument);

/* ObjectInteractionFlagsAllow (FUN_0041f100): map the first matching
 * stats-row capability bit to its required entity-stats bit, then search
 * the filtered live-entity pool for a match. */
int object_interaction_flags_allow(GameContext* ctx, int type, int row);

/* Entity-pool request predicates used by TutorialPathMenuAction. */
int queue_action_request_type2(GameContext* ctx);
int queue_action_request_value(GameContext* ctx, uint32_t state_mask);
int queue_action_request_flag100000(GameContext* ctx);
int queue_action_request_pair(GameContext* ctx, int entity_type,
                              uint32_t entity_subtype);
int queue_action_request_index(GameContext* ctx, uint32_t flags_mask);

/* QueueCallbackRequest3E0 (FUN_0041f400): test whether any filtered live
 * entity passes TerrainCellAllowsPlacement at the given cell. */
int queue_callback_request_3e0(GameContext* ctx, int x, int y);

/* QueueCallbackRequest390 (FUN_0041f3c0): search live entities, skipping
 * +0x3e8 bit 0x80000, for one whose requested cell is walkable. The corpus
 * forwards to TerrainCellWalkabilityQuery in simple mode and applies its
 * entity flags, owner, and per-terrain speed gates. */
int queue_callback_request_390(GameContext* ctx, int x, int y);

/* TutorialPathMenuAction (FUN_0041d040). All explicit switch branches,
 * default no-op behavior, and the final global-mode gate are implemented.
 * Callbacks override the live entity/config defaults where supplied. The
 * unnamed 0x14/0x15 route predicate remains an external runtime dependency. */
int tutorial_path_menu_action(
    GameContext* ctx, const struct EngineContext* engine,
    struct TerrainDescriptorNode* descriptors,
    int (*sidecar_query)(const struct LevelState*, int, int),
    int (*target_cell_valid)(EntitySlot*),
    int (*route_marker_candidate)(EntitySlot*, int),
    int (*vehicle_unload_eligible)(EntitySlot*, int),
    int (*entity_unload_cell_available)(EntitySlot*, int),
    /* Route/pick tail for the capability-1 branch of action 0x14/0x15. */
    int (*selected_action_14_15_route_predicate)(EntitySlot*),
    int (*building_upgrade_allowed)(EntitySlot*),
    int (*entity_action_kind_prerequisite)(EntitySlot*, int),
    int (*building_upgrade_limit_allows)(EntitySlot*),
    int action, int x, int y);

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
void selection_state_tick_active(void);
void selection_state_tick(GameContext* ctx);

/* Pick the placed entity nearest grid cell (cx,cy) within `tol` cells.
 * Returns NULL when nothing is close enough. */
EntitySlot* selection_pick(GameContext* ctx, float cx, float cy,
                           float tol);

/* Enqueue opcode 5 with signed integer CELL words, truncating the renderer's
 * corner-based fractional grid hit to its containing cell. Entity/scene
 * positions stay in world space. Returns the number enqueued. */
int selection_cmd_move(GameContext* ctx, float tx, float ty);

/* Enqueue opcode 1 through the selection eligibility/filter owner. */
void selection_cmd_add(GameContext* ctx, EntitySlot* e);

#ifdef __cplusplus
}
#endif

/* One port-side world action owner. The frame UI arbiter must first tag
 * the ordered sample WORLD. Its admission token is shared with the later
 * simtick caller. World actions use an owned release. The ordinary floor
 * branch follows the reference's selected-set movement/clear order; other
 * world action modes retain their existing reconstruction policy. */
struct InputPointerSample;
int selection_pointer_world_dispatch(GameContext* ctx,
    struct EngineContext* engine, struct InputPointerSample* sample,
    int width, int height);

/* Host convenience: pointer membership in the selection array. */
int selection_contains_entity(const GameContext* ctx, const EntitySlot* e);
/* SelectionContainsEntity (FUN_00452610): when index_out is non-NULL, scan
 * the live selection array in order and write the first matching 32-bit
 * index; when NULL, test the entity's +0x3e8 selected bit. */
int selection_contains_entity_target(const GameContext* ctx,
                                     const EntitySlot* e,
                                     uint32_t* index_out);

/* Coordinate-matched UI action cancellation helpers (FUN_0041cf80,
 * FUN_0041cfb0 and FUN_0041cfe0). `coordinate` points to two map words. */
extern "C" void selection_mode7_cancel_at_point(const uint32_t coordinate[2]);
extern "C" void cancel_action_for_matching_coordinates(const uint32_t coordinate[2]);
extern "C" void selection_mode5_cancel_at_point(const uint32_t coordinate[2]);

/* Selection-mode HUD command updates (FUN_0041cea0/0041cf20). */
void selection_object_command_update(GameContext* ctx, EntitySlot* entity);
void selection_target_command_update(GameContext* ctx, EntitySlot* entity);

/* SelectionCancelIfCursorHitsObject (FUN_0041d010): in gameplay context,
 * clear the current command/HUD selection request when the candidate is
 * selected or carries the cursor-hit flag (+0x3e8 bit 0x200000). */
void selection_cancel_if_cursor_hits_object(struct EngineContext* engine,
                                             GameContext* ctx,
                                             EntitySlot* entity);

/* SelectionCandidateAlreadySelectedOrMarked / SelectionCandidateEligibilityCheck
 * (FUN_00452690 / FUN_00452870, findings 77/85/468): the selection filter's
 * candidate leaves. Already-selected-or-marked tests the +0x3e8 selected
 * bit or +0x3ec byte bit 1. The
 * eligibility leaf requires the +0x3e8 bit-0x100 state gate, the ported
 * shared-eligibility predicate, the +0x3e4/+0x3e0 clear-bit gates, and
 * the type-1 +0x3e0 bit-0x40000 rejection. NULL entities reject both. */
int selection_candidate_already_selected_or_marked(const GameContext* ctx,
                                                   const EntitySlot* e);
int selection_active_candidate_already_selected_or_marked(
    EntitySlot* entity);
/* The eligibility leaf is declared in entity.hpp (implemented there). */

/* CurrentLevelStateFlagsContainMask (FUN_0041f720): ANDs the selected
 * entity's +0x3f0 level-state mask word. Deviation: with an empty
 * selection the corpus dereferences its NULL accessor blindly; the port
 * answers false (no flags contained). The +0x3f0 word itself is a
 * zero-init mirror — see game.hpp EntitySlot. */
int current_level_state_flags_contain_mask(const GameContext* ctx,
                                           uint32_t mask);

/* Retail SelectionClearMarkerAndResetHud takes one entity and writes the
 * active DAT_00556ecc mirror when its state/stat gates pass. */
void selection_clear_marker_and_reset_hud(EntitySlot* e); /* FUN_0044c4a0 */
/* Explicit-context compatibility entry used by tests and context owners. */
void selection_clear_marker_and_reset_hud(struct EngineContext* engine,
                                          EntitySlot* e);

#endif /* LEGORRR_GAME_SELECT_HPP */
