// SPDX-License-Identifier: LGPL-3.0-or-later
/* FrontEnd.c (GAME-017) link names (findings 623 and 628). In the matching
 * build the unit's Front_Callback_TriggerPlayCredits (004103d0) and most
 * of part 2 take the port's names, which Reward.c (reward.c), Lego.c
 * (lego_unit.c) and the port call; frontend_port.cpp compiles its own
 * definitions out of that build. Natively the port's definitions stay the
 * live ones. Included first by frontend_unit.c (a macro renames a
 * declaration one for one, so MSVC's symbol numbering does not change). */
#ifndef LEGORRR_GAME_FRONTEND_UNIT_LINK_NAMES_H
#define LEGORRR_GAME_FRONTEND_UNIT_LINK_NAMES_H

/* The callees' own link names: the names their definitions take in the
 * matching build. */
#include "game/pointer_link_names.h"
#include "game/screen_effect_quads_link_names.h"
#include "game/sfx_link_names.h"
#include "game/tooltip_link_names.h"
#include "gods98/viewports_link_names.h"

#if defined(_MSC_VER) && _MSC_VER < 1200
#define front_callback_trigger_play_credits main_menu_text_item /* 004103d0 */

/* Part 2 (finding 628): the unit's functions whose port names have C
 * linkage and the original's contract (a port prototype may add trailing
 * arguments the original ignores) take those names in the matching build,
 * and frontend_port.cpp compiles its own definitions of them out of that
 * build. Every caller, the C units' and the port's, then reaches the
 * original. Natively the port's definitions stay the live ones. */
#define front_callback_slider_music_volume audio_menu_volume_set       /* 00414df0 */
#define front_get_music_volume_slider option_music_default              /* 00414e10 */
#define front_update_game_speed_slider options_scroll_index_store       /* 00414f90 */
#define front_calc_slider_game_speed scroll_to_slider_index             /* 00415030 */
#define front_callback_select_load_save save_menu_refresh_target        /* 004150b0 */
#define front_refresh_game_speed_slider scroll_slider_refresh           /* 00415360 */
#define front_save_clear_save_number save_slot_selection_clear_target   /* 00415cf0 */
#define front_play_level_stream sound_stream_play_level                 /* 00416150 */
#define front_play_tutorial_stream sound_stream_play_tutorial           /* 004161a0 */
#define front_save_options_to_save_data options_write_to_slot_target    /* 00416910 */
#define front_restore_options options_load_or_default_target            /* 00416940 */
#define front_is_front_end_enabled frontend_enabled                     /* 00416d00 */
#define front_is_mission_selected save_slot_open                        /* 00416d50 */
#define front_is_tutorial_selected is_in_game                           /* 00416d60 */
#define front_is_quit_selected quit_menu_probe                          /* 00416dd0 */
#define front_is_quit_mission_selected level_start_flag                 /* 00416de0 */
#define front_is_restart_mission_selected menu_options_flag             /* 00416df0 */
#define front_get_current_level_set active_object_list_word             /* 00416e00 */
#define front_levels_reset_visited sav_clear_visited                    /* 00416e70 */
#define front_level_set_set_level_visited list_add_name                 /* 00416ed0 */
#define front_level_set_is_level_visited list_has_item                  /* 00416f40 */
#define front_level_set_set_level_link list_set_node                    /* 00416fb0 */
#define front_level_set_get_level_link list_get_node                    /* 00417020 */
#define front_level_link_run_through_links sav_tree_walk                /* 004171c0 */
#define front_level_link_find_by_link_index registry_value_lookup       /* 00417280 */
#define front_level_link_get_set_index registry_value_index_lookup_or_not_found /* 004172d0 */
#define front_levels_update_available_recurse sav_tree_walk2            /* 004172f0 */
#define front_save_get_level_score float_to_long_x87                    /* 00417430 */
#define front_save_read_save_file save_read_slot_target                 /* 00417a90 */
#define front_save_read_save_files sav_refresh_slots_target             /* 00417df0 */
#define front_save_get_save_data_at_index sav_slot_ctx_target           /* 00417e50 */
#define front_save_get_current_save_data sav_build_ctx_target           /* 00417e70 */
#define front_save_get_save_number save_slot_get_target                 /* 00417e90 */
#define front_save_set_save_number save_slot_set_target                 /* 00417ea0 */
#define front_save_set_save_struct_18 options_context_six_field_set_target /* 00417f20 */
#define front_save_set_reward_level save_write_record_target            /* 00417f40 */
#define front_save_get_reward_level sav_record_ptr_target               /* 00417f90 */
#define front_is_load_save_menu current_selection_context_get           /* 00417fe0 */
#define front_set_load_save_menu current_selection_context_set          /* 00417ff0 */
#define front_save_reset_current_save show_level_overlay_target         /* 00418000 */
#define front_save_copy_save_data front_campaign_snapshot_create        /* 00418040 */
#define front_save_restore_save_data options_context_copy_and_replace_resource_target /* 004180c0 */
#define front_levels_set_update save_state_event_context_set            /* 00418110 */
#define front_save_is_game_100_percented upgrade_average_is_above_99    /* 00418120 */
/* Front_Levels_UpdateAvailable (004173e0) and Front_Save_WriteCurrentSaveFile
 * (00417fc0) keep the unit's names: savegame.h's prototypes of the port's
 * names take another argument list, so savegame.h renames its callers
 * instead (outside this unit). */
#else
/* frontGlobs.startMissionLink (00556ff8) is Reward.c's g_front_save_tree
 * in the matching build, where frontend_port.cpp compiles its own
 * definition out; natively the port keeps that name. */
#define g_front_save_tree frontGlobs_startMissionLink
#endif

#endif /* LEGORRR_GAME_FRONTEND_UNIT_LINK_NAMES_H */
