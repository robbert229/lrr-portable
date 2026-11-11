// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/tutorial_hud.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_TUTORIAL_HUD_SHARED_HPP
#define LRR_GAME_TUTORIAL_HUD_SHARED_HPP

#define NERP_TUTORIAL_CELL_CAPACITY 2048

typedef struct NerpTutorialCellState {
    int32_t x;
    int32_t y;
    uint32_t block_clicks;
    uint32_t crystal_count;
    uint32_t ore_count;
    uint32_t flags;
} NerpTutorialCellState;

typedef struct NerpRuntimeState {
    /* Reference history[0] aliases the frontend's last inline sound index.
     * Only entries 1..8 live here; always use tutorial_message_history_get. */
    uint32_t message_history[8];
    uint32_t message_maximum;
    uint32_t message_offset;
    uint32_t message_ack_count;
    uint32_t message_pending_ack;
    uint32_t message_previous_ack;
    uint32_t message_flags;
    uint32_t message_wait;
    uint32_t message_suppress_arrow;
    uint32_t message_sound_active;
    float message_timer;
    float icon_position[2];
    uint32_t icon_space;
    uint32_t icon_width;
    uint32_t tutorial_flags;
    uint32_t tutorial_block_clicks;
    uint32_t tutorial_crystals;
    uint32_t tutorial_block_type_flags;
    uint32_t tutorial_record_pointer;
    uint32_t camera_control_flags;
    int32_t camera_target_cells[2][2];
    uint32_t camera_target_valid[2];
    uint32_t hidden_objects_found;
    uint32_t objective_switch;
    uint32_t pointer_release_count;
    int32_t pointer_action;
    int32_t pointer_x;
    int32_t pointer_y;
    NerpTutorialCellState cells[NERP_TUTORIAL_CELL_CAPACITY];
    uint32_t cell_count;
} NerpRuntimeState;

extern TutorialScriptOps s_tutorial_script_ops;
extern GameContext* s_tutorial_game_context;
extern EngineContext* s_tutorial_engine_context;
extern void* s_tutorial_script_data;
extern uint32_t s_tutorial_script_size;
extern uint32_t* s_tutorial_program_words;
extern uint32_t s_tutorial_program_count;
extern uint32_t s_tutorial_nerp_registers[8];
extern float s_tutorial_nerp_timers[4];
extern NerpRuntimeState s_nerp_runtime;
extern uint32_t s_tutorial_555d40_state[45];
extern uint32_t s_tutorial_focus_id;
extern uint32_t s_tutorial_focus_valid;
extern float s_tutorial_focus_offset_target;
extern float s_tutorial_focus_angle_target;
extern uint32_t s_tutorial_focus_offset_active;
extern uint32_t s_tutorial_focus_angle_active;
extern float s_tutorial_focus_offset;
extern float s_tutorial_focus_angle;
extern float s_tutorial_focus_angle_velocity;
int tutorial_builtin_command(uint32_t command, const uint32_t* args, uint32_t argc, uint32_t* result_out);
uint32_t tutorial_builtin_command_adapter(uint32_t command, const uint32_t* args, uint32_t arg_count, void* context);
void tutorial_scroll_runtime_state_reset(void);
void tutorial_camera_focus_state_reset(void);
void tutorial_script_data_release(void* data);
void tutorial_tokens_clear(void);
void tutorial_script_program_clear(void);
int tutorial_script_file_load_target(const char* path);

#endif /* LRR_GAME_TUTORIAL_HUD_SHARED_HPP */
