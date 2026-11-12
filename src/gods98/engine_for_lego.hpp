// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from engine.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_ENGINE_FOR_LEGO_HPP
#define LRR_GODS98_ENGINE_FOR_LEGO_HPP

extern "C" int32_t s_context_mode;
extern uint32_t s_panel_highlight_flags;
extern float s_panel_highlight_timer;
extern EngineWorldArrowCaptureHook s_world_arrow_capture_hook;
extern void* s_world_arrow_capture_context;
extern EngineTutorialDispatchHook s_tutorial_dispatch_hook;
extern void* s_tutorial_dispatch_context;
extern unported_ledger_stubs::GlobalSubsystemResetState* s_global_subsystem_reset_state;
extern const char* s_engine_config_root;
extern EngineContext* s_callback_registry_context;
extern EngineConfigRegistryResetHook s_callback_registry_reset_hook;
extern EngineGameConfigInitHook s_game_config_init_hook;
extern void* s_callback_registry_user;
extern uint32_t s_triple_first;
extern uint32_t s_triple_second;
extern uint32_t s_triple_third;
extern float s_task_text_origin_x;
extern float s_task_text_origin_y;
extern float s_score_text_origin_x;
extern float s_score_text_origin_y;
extern uint32_t s_global_low_word;
extern void* s_refresh_record;
extern void* s_position_handle;
extern int s_position_seam_calls;
extern void* s_position_seam_last_handle;
extern int s_position_seam_last_x;
extern int s_position_seam_last_y;
extern float s_scroll_scale;

#endif /* LRR_GODS98_ENGINE_FOR_LEGO_HPP */
