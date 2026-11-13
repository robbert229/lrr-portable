// SPDX-License-Identifier: LGPL-3.0-or-later
/* Message.c (GAME-027) link names (finding 631). In the matching build the
 * unit's functions whose callers already pass the original's arguments
 * take the names those callers use: the C units (aitask.c, construction.c,
 * interface_unit.c, lego_unit.c, legoobject_unit.c, panel_unit.c) and the
 * port's C++ callers reach the unit with no caller edit. message_port.cpp,
 * lego_port.cpp and resource.cpp compile their own definitions of these
 * names out of that build (LRR_MESSAGE_PORT_NAMES). Natively the port's
 * definitions stay the live ones and the unit keeps its own names.
 *
 * Included only by message_unit.c, before any other header. A macro renames
 * a definition one for one, so MSVC's symbol numbering (finding 538) does
 * not change. */
#ifndef LEGORRR_GAME_MESSAGE_UNIT_LINK_NAMES_H
#define LEGORRR_GAME_MESSAGE_UNIT_LINK_NAMES_H

/* The callees' own link names: the names their definitions take in the
 * matching build. */
#include "game/entity_stats_link_names.h"
#include "game/sfx_link_names.h"
#include "gods98/container_link_names.h"

#if defined(_MSC_VER) && _MSC_VER < 1200
#define message_initialise message_string_table_initialize          /* 00451cd0 */
#define message_remove_events_with_object cmd_queue_cancel_match     /* 00451f60 */
#define message_add_message_action lego_key_callback_set_adapter     /* 00452010 */
#define message_post_event cmd_produce                              /* 00452060 */
#define message_update cmd_queue_tick                               /* 004520d0 */
#define message_pick_random_floor entity_random_valid_cell_command_create /* 00452430 */
#define message_remove_object_reference selection_object_detach      /* 004524b0 */
#define message_copy_selected_units selection_snapshot_create        /* 00452520 */
#define message_get_selected_units2 selection_active_context_record_get /* 00452580 */
#define message_any_unit_selected selection_active_has_any_get       /* 004525b0 */
#define message_get_primary_selected_unit selection_active_entity_get /* 004525c0 */
#define message_get_selected_units selection_active_records_get      /* 004525e0 */
#define message_get_num_selected_units selection_active_count_get    /* 004525f0 */
#define message_cleanup_selected_units_count selection_active_count_reset /* 00452600 */
#define message_is_unit_selected selection_contains_entity_active_target /* 00452610 */
#define message_find_index_of_object cmd_array_find_value_index      /* 00452650 */
#define message_is_selected_or_flags4_2 selection_active_candidate_already_selected_or_marked /* 00452690 */
#define message_select_object selection_candidate_filter_default     /* 004526c0 */
#define message_select_object2 selection_candidate_filter_active     /* 004526e0 */
#define message_can_select selection_candidate_eligibility_check     /* 00452870 */
#define message_reduce_selected_units selection_state_tick_active    /* 004528c0 */
#define message_clear_selected_units selection_clear_active          /* 00452be0 */
#define message_deselect_object selection_entity_remove_active       /* 00452c50 */
#define message_kill_all_selected_units selected_entity_state_monitor /* 00452cc0 */
#define message_enter_first_person_view selection_command_dispatch   /* 00452d60 */
#endif

#endif /* LEGORRR_GAME_MESSAGE_UNIT_LINK_NAMES_H */
