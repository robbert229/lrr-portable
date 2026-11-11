// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what reward uses from mission.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_MISSION_FOR_REWARD_HPP
#define LRR_GAME_MISSION_FOR_REWARD_HPP

typedef struct MissionTaskInitBinding {
    GameContext* game;
    BlockFile* config;
    const char* level_name;
} MissionTaskInitBinding;

extern const char s_task_name_crystals[];
extern RewardScreenState* s_reward_screen_state;
extern const RewardScreenOps* s_reward_screen_ops;
extern RewardScreenState s_reward_screen_default_state;
extern GameContext* s_reward_font_owner;
extern void* s_reward_font_base_record;
extern void* s_reward_font_title_record;
extern void* s_reward_font_back_record;
extern MissionTaskInitBinding s_task_init_binding;
void rw_path(char* out, size_t out_sz, BlockFile* cfg, const char* level, const char* sub, const char* name);
void task_put_int(GameTask* t, int off, int32_t v);
int task_get_int(const GameTask* t, int off);
char* mission_replace_char(char* text, char from, char to);
extern uint32_t s_reward_success_words[8];
extern int s_reward_success_words_valid;
extern GameContext* s_checkpoint_refresh_context_override;
void mission_reward_font_records_release(void);
GameTask* reward_task_get(RewardScreenState* state, int index);
void reward_task_flic_reload_record(RewardScreenState* state, GameTask* task, int index);
void reward_task_flics_reload_all(RewardScreenState* state);

#endif /* LRR_GAME_MISSION_FOR_REWARD_HPP */
