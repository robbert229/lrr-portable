// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_UI_ACTION_DISPATCH_HPP
#define LEGORRR_GAME_UI_ACTION_DISPATCH_HPP

#include <stdint.h>

/* Named effect seams for UiActionDispatch. The caller owns the runtime
 * objects; this dispatcher preserves the original branch order and state
 * writes while routing engine-specific operations through these callbacks. */
typedef struct UiActionDispatchOps {
    void* context;
    void* (*current_selection_get)(void* context);
    int (*action_already_held)(void* context, int action, int mode);
    uint32_t (*entity_flags_get)(void* context, void* entity, int flag_word);
    uint8_t (*entity_byte_flags_get)(void* context, void* entity,
                                    int byte_offset);
    int (*entity_type_get)(void* context, void* entity);
    uint32_t (*selected_cell_flags_get)(void* context, int x, int y);
    void (*ui_mode_request)(void* context, int mode, int argument);
    void (*input_request_enqueue)(void* context, int kind, void* entity,
                                  int argument_a, int argument_b);
    void (*selected_map_action_enqueue)(void* context, int action, int mode,
                                        int x, int y);
    void (*selection_clear_request_and_hud_reset)(void* context);
    /* TutorialPathDescriptorCreate @ 0x409300, used by actions 5 and 7. */
    void (*tutorial_path_descriptor_create)(void* context, int x, int y);
    void (*selected_cell_reveal_mask_clear)(void* context, int x, int y,
                                            uint32_t mask);
    void (*terrain_cell_update)(void* context, int x, int y);
    void (*terrain_cell_command_02510)(void* context, int x, int y);
    void (*terrain_cell_command_03030)(void* context, int x, int y);
    void (*terrain_cell_command_02d50)(void* context, int x, int y);
    void (*terrain_cell_command_02fb0)(void* context, int x, int y);
    void (*terrain_cell_command_02ae0)(void* context, int x, int y);
    void (*terrain_cell_action_mode_refresh)(void* context, int x, int y);
    void (*terrain_cell_command_complete)(void* context, int x, int y);
    void (*terrain_descriptor_tiles_reset_and_free)(void* context,
                                                     int x, int y);
    void (*selection_group_clear)(void* context, int x, int y);
    void (*selected_entity_action_02ec0)(void* context, void* entity);
    void (*global_state_low_byte_clear_and_refresh)(void* context);
    void (*selected_entities_detach_or_pose)(void* context);
    void (*terrain_cell_to_world_position)(void* context, void* entity,
                                           int mode);
    void (*selected_entity_action_1f)(void* context);
    void (*selected_entity_action_20_group)(void* context);
    void (*selected_entity_action_21)(void* context);
    void (*selected_entity_action_24)(void* context);
    void (*selected_entity_action_2f)(void* context, void* entity);
    void (*selected_entity_action_31_group)(void* context, int argument);
    /* ActionKindEntityCreateAndCommandIssue @ 0x43a650. */
    void (*entity_action_kind_create)(void* context, void* entity, int kind);
    void (*entity_flag_80000000_set_clear)(void* context, void* entity,
                                           int set);
    int (*selected_entity_predicate)(void* context, void* entity,
                                     int predicate_id);
    void (*selected_entity_slot_set)(void* context, int slot, void* entity);
    void (*building_upgrade_commit)(void* context, void* entity);
    void (*selected_entity_field_f6_clear)(void* context, void* entity);
    void (*object_command_task_create)(void* context, void* entity,
                                      int mode);
    void (*entity_kind_command_issue)(void* context, void* entity);
    void (*ui_action_helper_00452be0)(void* context);
    void (*cursor_action_pair_request)(void* context, int action, int value);
    void* (*entity_action_record_find)(void* context, void* entity,
                                       int mode, int mask);
    void (*entity_action_record_apply)(void* context, void* record,
                                       int mask, int enabled);
    int (*cursor_action_rejected)(void* context, int first, int second,
                                  int mode);
} UiActionDispatchOps;

typedef struct UiActionDispatchState {
    const UiActionDispatchOps* ops;
    int current_mode;             /* DAT_004ddccc */
    int selected_cell_x;          /* DAT_004ddcc4 */
    int selected_cell_y;          /* DAT_004ddcc8 */
    int cursor_action_x;           /* DAT_004ddcd8 */
    int cursor_action_y;           /* DAT_004ddcdc */
    uint32_t action_flags;         /* DAT_00556ed4 */
    uint32_t cursor_flags;         /* DAT_004c7ea8 */
    void* selected_entity_slots[3]; /* DAT_0055644c/50/54 */
} UiActionDispatchState;

void ui_action_dispatch_state_install(UiActionDispatchState* state);
int ui_action_dispatch(int action);

#endif
