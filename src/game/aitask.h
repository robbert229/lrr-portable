// SPDX-License-Identifier: LGPL-3.0-or-later
/* AITask.c (original translation unit GAME-002, finding 505).
 *
 * C-compatible header (ADR 0005). aitask.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp checks it against
 * the C++ declarations of the same functions.
 */
#ifndef LEGORRR_GAME_AITASK_H
#define LEGORRR_GAME_AITASK_H

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;

/* The original command record (AITask, 0x68 bytes; finding 10, finding 570).
 * This is the unit's own declaration of the original layout. The port's
 * CmdRec (cmd_for_aitask.hpp, a frozen header) keeps its port-only tail and
 * is used only by the port tail aitask_port.cpp. Unlike the port's CmdRec
 * before finding 570, there is one link: +0x60 serves both the entity chain
 * (entity +0x2f0) and the two global lists (0x4b3354, 0x4b3358). */
typedef struct AITask AITask;
struct AITask {
    int type;                   /* +0x00 task type (0 goto, 2 target, 3 task, ...) */
    int param;                  /* +0x04 */
    int blockX;                 /* +0x08 cell x (raw word; floats for some types) */
    int blockY;                 /* +0x0c cell y */
    struct EntitySlot* object;  /* +0x10 payload object / target */
    float time;                 /* +0x14 timeGetTime on alloc, 1500.0f on link */
    float timeIn;               /* +0x18 timer (task parameter) */
    int priorityValue;          /* +0x1c behaviour value = behaviour[priority] */
    int field_20;               /* +0x20 cell-command action id */
    int field_24;               /* +0x24 cell-command referenced entity */
    int field_28;               /* +0x28 cell-command entry count */
    int zone;                   /* +0x2c zone-command id */
    float field_30;             /* +0x30 random offset in [0.4, 0.6] (type 7) */
    float field_34;             /* +0x34 random offset in [0.4, 0.6] (type 7) */
    void* field_38;             /* +0x38 raw word / payload */
    int field_3c;               /* +0x3c task-kind selector (type 8 sets 8) */
    void* buffer;               /* +0x40 owned buffer (freed by the pool free) */
    unsigned int count;         /* +0x44 the buffer's entry count */
    void* entityRef;            /* +0x48 pending entity reference */
    int priority;               /* +0x4c priority (kind) index */
    unsigned int field_50;      /* +0x50 timeGetTime() at alloc; unsigned sort key of 00403f80 */
    void* eventPayload;         /* +0x54 event payload */
    AITask* ref;                /* +0x58 referenced record (type 0x13 pair) */
    unsigned int flags;         /* +0x5c state flags */
    AITask* next;               /* +0x60 entity-chain and global-list link */
    AITask* nextFree;           /* +0x64 pool free-list link */
};

/* Port-only accessors of the priority tables shared with the port tail
 * (finding 570); defined last in aitask.c. */
int* aitask_port_priority_values(void);  /* 004b32e8 */
int* aitask_port_priority_states(void);  /* 004b7ff4 */

/* FUN_00401cb0 (cmd_kind_by_name). */
int aitask_cmd_kind_by_name(const char* name, int* priority);

/* FUN_00401cf0 (cmd_registry_init_target). */
void aitask_cmd_registry_init_target(void);

/* FUN_00402000 (cmd_mark_matching). */
void aitask_cmd_mark_matching(int type, struct EntitySlot* object, const int* blockPos);

/* FUN_00402060 (cmd_set_kind). */
void aitask_cmd_set_kind(AITask* task, int priority);

/* FUN_00402090 (priority_entry_order_value_set). */
void priority_entry_order_value_set(int kind, int value);

/* FUN_004020a0 (priority_entry_state_set). */
void priority_entry_state_set(int kind, int value);

/* FUN_004020b0 (priority_entry_state_get). */
int priority_entry_state_get(int kind);

/* FUN_004020c0 (handle_array_release_and_reset). */
void aitask_handle_array_release_and_reset(void);

/* FUN_00402100 (cmd_flush_all). */
void aitask_cmd_flush_all(int disable);

/* FUN_00402170 (level_no_gather_object_setup). */
void aitask_level_no_gather_object_setup(int noGather);

/* FUN_004021e0 (engine_memory_allocate). */
void* engine_memory_allocate(long size);

/* FUN_00402210 (engine_memory_free). */
void engine_memory_free(void* ptr);

/* FUN_00402220 (cmd_tick_global_delta). */
void aitask_cmd_tick_global_delta(float elapsed);

/* FUN_00402310 (cmd_global_list_purge). */
void aitask_cmd_global_list_purge(AITask** list);

/* FUN_00402510 (terrain_cell_command_create). */
void aitask_terrain_cell_command_create(const int* blockPos, int noHighlight, int cancel);

/* FUN_00402600 (entity_record_array_apply_zero_state). */
void aitask_entity_record_array_apply_zero_state(AITask* task, int detach);

/* FUN_00402670 (entity_command15_validated_record_queue). */
void aitask_entity_command15_validated_record_queue(struct EntitySlot* object);

/* FUN_004026c0 (entity_deferred_work_schedule). */
void aitask_entity_deferred_work_schedule(struct EntitySlot* object);

/* FUN_00402720 (cmd_create_task). */
void aitask_cmd_create_task(struct EntitySlot* object, float timeIn);

/* FUN_004027a0 (bobbin_clear_delay). */
void aitask_bobbin_clear_delay(const int* blockPos);

/* FUN_00402800 (entity_command14_batch_issue). */
void aitask_entity_command14_batch_issue(unsigned int count);

/* FUN_00402850 (cmd_minion_companion). */
void aitask_cmd_minion_companion(struct EntitySlot* object);

/* FUN_00402890 (task_type23_create_for_category_mask). */
void aitask_task_type23_create_for_category_mask(struct EntitySlot* object, int mask, int detach);

/* FUN_00402960 (entity_kind_command_issue). */
void aitask_entity_kind_command_issue(struct EntitySlot* object);

/* FUN_00402a40 (coordinate_pair_callback_dispatch). */
void aitask_coordinate_pair_callback_dispatch(const int* blockPos);

/* FUN_00402aa0 (argument_callback_dispatch). */
void aitask_argument_callback_dispatch(struct EntitySlot* object);

/* FUN_00402ae0 (terrain_cell_refresh_dispatch_current). */
void aitask_terrain_cell_refresh_dispatch_current(const int* blockPos, int mode);

/* FUN_00402b30 (coordinate_callback_dispatch_current). */
void aitask_coordinate_callback_dispatch_current(const int* blockPos);

/* FUN_00402b60 (coordinate_callback_dispatch_by_mode). */
void aitask_coordinate_callback_dispatch_by_mode(const int* blockPos, int mode);

/* FUN_00402cd0 (cmd_cancel_ref). */
void aitask_cmd_cancel_ref(AITask* task);

/* FUN_00402d50 (kind_nine_cell_command_issue). */
int aitask_kind_nine_cell_command_issue(const int* blockPos, int noDetach);

/* FUN_00402e40 (cell_command_type13_issue). */
void aitask_cell_command_type13_issue(const int* blockPos);

/* FUN_00402ec0 (entity_conditional_task_schedule). */
void aitask_entity_conditional_task_schedule(struct EntitySlot* object);

/* FUN_00402f30 (entity_threshold_progress_event_queue). */
void aitask_entity_threshold_progress_event_queue(struct EntitySlot* object,
                                                  int urgent);

/* FUN_00402f70 (entity_target_command_queue). */
void aitask_entity_target_command_queue(struct EntitySlot* object,
                                        struct EntitySlot* target);

/* FUN_00402fb0 (cell_command_type11_issue). */
void aitask_cell_command_type11_issue(const int* blockPos);

/* FUN_00403030 (cell_command_type3_issue). */
void aitask_cell_command_type3_issue(const int* blockPos);

/* FUN_004030b0 (coordinate_pair_event_queue). */
void aitask_coordinate_pair_event_queue(const int* blockPos, void* payload);

/* FUN_004030e0 (cmd_validated_record_queue). */
void aitask_cmd_validated_record_queue(AITask* source);

/* FUN_00403190 (object_event_type2_queue). */
int aitask_object_event_type2_queue(void* payload);

/* FUN_004031e0 (active_entity_list_contains_code). */
int aitask_active_entity_list_contains_code(int code);

/* FUN_00403270 (selection_drag_area_command_create). */
void aitask_selection_drag_area_command_create(struct EntitySlot** objects,
                                               unsigned int count,
                                               const int* blockPos,
                                               int replace);

/* FUN_00403320 (object_command_task_create). */
void aitask_object_command_task_create(struct EntitySlot* object, int param);

/* FUN_004033c0 (object_attached_record_allocate). */
void aitask_object_attached_record_allocate(struct EntitySlot* object);

/* FUN_004033f0 (coordinate_pair_task_record_queue). */
void aitask_coordinate_pair_task_record_queue(const int* blockPos);

/* FUN_00403430 (task_registry_remove_type0_entry_by_pair). */
void aitask_task_registry_remove_type0_entry_by_pair(const int* blockPos);

/* FUN_004034a0 (scalar_task_record_queue). */
void aitask_scalar_task_record_queue(struct EntitySlot* object);

/* FUN_004034e0 (task_pair_record_enqueue). */
void aitask_task_pair_record_enqueue(const int* blockPos);

/* FUN_00403520 (task_record_kind10_queue). */
void aitask_task_record_kind10_queue(struct EntitySlot* object);

/* FUN_00403560 (task_registry_remove_type10_entries_for_entity). */
void aitask_task_registry_remove_type10_entries_for_entity(
    struct EntitySlot* object);

/* FUN_004035c0 (cmd_create_move). */
void aitask_cmd_create_move(struct EntitySlot* object, const int* blockPos,
                            int param);

/* FUN_00403610 (cmd_create_gated). */
void aitask_cmd_create_gated(struct EntitySlot* object,
                             struct EntitySlot* target);

/* FUN_00403650 (entity_task_batch_append). */
void aitask_entity_task_batch_append(struct EntitySlot** objects,
                                     unsigned int count,
                                     struct EntitySlot* target);

/* FUN_004036c0 (cmd_create_target). */
void aitask_cmd_create_target(struct EntitySlot* object,
                              struct EntitySlot* target);

/* FUN_00403700 (cmd_cancel_by_coords). */
void aitask_cmd_cancel_by_coords(int zone, int matchCell, int field20,
                                 int field24);

/* FUN_00403750 (cmd_command_kind_record_issue). */
void aitask_cmd_command_kind_record_issue(int field20, int field24, int field28,
                                          const int* blockPos, int zone,
                                          const float* randPair, int flag800);

/* FUN_00403810 (terrain_cell_record_count_update). */
void aitask_terrain_cell_record_count_update(const int* blockPos, int a, int b,
                                             int c);

/* FUN_004038c0 (timed_task_record_initialize_and_link). */
AITask* aitask_timed_task_record_initialize_and_link(struct EntitySlot* object,
                                                     int field20, int field24,
                                                     int field28);

/* FUN_00403910 (terrain_cell_record_counts_prune). */
void aitask_terrain_cell_record_counts_prune(struct EntitySlot* object);

/* FUN_004039a0 (task_record_create_and_scan_grid). */
void aitask_task_record_create_and_scan_grid(struct EntitySlot* object);

/* FUN_004039d0 (task_record_kind11_timed_queue). */
void aitask_task_record_kind11_timed_queue(struct EntitySlot* object);

/* FUN_00403a10 (task_callback_dispatch). */
void aitask_task_callback_dispatch(struct EntitySlot* object);

/* FUN_00403a50 (entity_drain_command_chain). */
void aitask_entity_drain_command_chain(struct EntitySlot* object);

/* FUN_00403af0 (entity_task_state_update_after_cleanup). */
void aitask_entity_task_state_update_after_cleanup(struct EntitySlot* object,
                                                   int remove);

/* FUN_00403b40 (entity_pending_task_flag_clear). */
void aitask_entity_pending_task_flag_clear(struct EntitySlot* object);

/* FUN_00403b60 (entity_activity_records_refresh). */
void aitask_entity_activity_records_refresh(struct EntitySlot* object);

/* FUN_00403c00 (entity_activity_record_remove_by_type). */
void aitask_entity_activity_record_remove_by_type(struct EntitySlot* object,
                                                  int type,
                                                  struct EntitySlot* target);

/* FUN_00403d10 (entity_command_unlink_by_kind). */
void aitask_entity_command_unlink_by_kind(struct EntitySlot* object, int type,
                                          struct EntitySlot* target, int unlink);

/* FUN_00403ef0 (pending_record_callback_dispatch). */
void aitask_pending_record_callback_dispatch(struct EntitySlot* object);

/* FUN_00403f30 (ai_idle_chain_record_attach). */
void aitask_ai_idle_chain_record_attach(struct EntitySlot* object);

/* FUN_00403f60 (task_record_kind14_remove_if_head). */
void aitask_task_record_kind14_remove_if_head(struct EntitySlot* object);

/* FUN_00404030 (cmd_link_head). */
void aitask_cmd_link_head(AITask* task, int priority);

/* FUN_00404070 (cmd_link_tail). */
void aitask_cmd_link_tail(AITask* task);

/* FUN_004040a0 (entity_task_queue_flush). */
void aitask_entity_task_queue_flush(struct EntitySlot* object);

/* FUN_00404100 (command_queue_sweep_relink). */
void aitask_command_queue_sweep_relink(struct EntitySlot* object);

/* FUN_00404190 (cmd_chain_walk). */
AITask* aitask_cmd_chain_walk(struct EntitySlot* object);

/* FUN_004041b0 (entity_task_chain_contains_kind). */
int aitask_entity_task_chain_contains_kind(struct EntitySlot* object, int type);

/* FUN_004041e0 (entity_command_chain_head_remove). */
AITask* aitask_entity_command_chain_head_remove(struct EntitySlot* object);

/* FUN_00404e00 (object_path_submit_to_grid_target). */
int aitask_object_path_submit_to_grid_target(struct EntitySlot* object, int* blockPos,
                                             float* wPos);

/* FUN_00404ed0 (ledger_stub_00404ed0_CheckObjectCandidate). */
int aitask_check_object_candidate(struct EntitySlot* object);

/* FUN_00404f10 (object_task_target_filter_find). */
int aitask_object_task_target_filter_find(int type, int id, int level);

/* FUN_00404f50 (object_filter_matches_action_target). */
int aitask_object_filter_matches_action_target(AITask* task, void* data);

/* FUN_00404fc0 (ledger_stub_00404fc0_PlaceCommandEligibilityCheck). */
int aitask_place_command_eligibility_check(AITask* task, struct EntitySlot* object,
                                           float* outWPos, int* outBPos, int* outFlag,
                                           int checkCommand, int checkAbility);

/* FUN_00405950 (pending_command_timer_fire_scan). */
void aitask_pending_command_timer_fire_scan(void);

/* FUN_00405c10 (PendingCommandEntityAssignmentTick). */
void aitask_pending_command_entity_assignment_tick(void);

/* FUN_00406360 (entity_record_attach_and_relink). */
void aitask_entity_record_attach_and_relink(AITask* task, AITask* prev,
                                            struct EntitySlot* object);

/* FUN_004063e0 (pending_record_entity_reference_clear). */
void aitask_pending_record_entity_reference_clear(struct EntitySlot* object);

/* FUN_00406400 (cmd_pending_record_copy). */
AITask* aitask_cmd_pending_record_copy(AITask* task);

/* FUN_00406440 (cmd_alloc). */
AITask* aitask_cmd_alloc(int type);

/* FUN_00406480 (cmd_free). */
void aitask_cmd_free(AITask* task, int noCancel);

/* FUN_004064c0 (cmd_pool_grow). */
void aitask_cmd_pool_grow(void);

/* FUN_00406540 (EntityPoolCallbackSearch). */
int aitask_entity_pool_callback_search(int (*callback)(AITask* task, void* data),
                                       void* data);

/* FUN_004065d0 (renderer_global_dword_get). */
unsigned int renderer_global_dword_get(void);

/* FUN_004065e0 (render_surface_secondary_get). */
unsigned int render_surface_secondary_get(void);

/* FUN_004021f0 (callback or merged entry). */
int aitask_callback_004021f0(AITask* task, void* data);

/* FUN_00402700 (callback or merged entry). */
int aitask_callback_00402700(AITask* task, void* data);

/* FUN_00402a70 (callback or merged entry). */
int aitask_callback_00402a70(AITask* task, void* data);

/* FUN_00402ac0 (callback or merged entry). */
int aitask_callback_00402ac0(AITask* task, void* data);

/* FUN_00402bb0 (callback or merged entry). */
int aitask_callback_00402bb0(AITask* task, void* data);

/* FUN_00402c20 (callback or merged entry). */
int aitask_callback_00402c20(AITask* task, void* data);

/* FUN_00402c70 (callback or merged entry). */
int aitask_callback_00402c70(AITask* task, void* data);

/* FUN_00402ca0 (callback or merged entry). */
int aitask_callback_00402ca0(AITask* task, void* data);

/* FUN_00402d30 (callback or merged entry). */
int aitask_callback_00402d30(AITask* task, void* data);

/* FUN_00403a30 (callback or merged entry). */
int aitask_callback_00403a30(AITask* task, void* data);

/* FUN_00403ad0 (callback or merged entry). */
int aitask_callback_00403ad0(AITask* task, void* data);

/* FUN_00403f10 (callback or merged entry). */
int aitask_callback_00403f10(AITask* task, void* data);

/* FUN_00403f80 (callback or merged entry). */
int aitask_callback_00403f80(const void* a, const void* b);

/* FUN_00404250 (callback or merged entry). */
int aitask_cmd_pool_timer_delta_apply(AITask* task, void* data);

/* FUN_00404280 (callback or merged entry). */
int aitask_cmd_entity_command_chain_tick(struct EntitySlot* object, void* data);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AITASK_H */
