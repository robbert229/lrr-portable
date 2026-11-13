// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-005 Bubbles.c (finding 598): the port's link names of the unit's
 * functions (as findings 581, 585, 590, 592 and 597).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: a config, an object, the call to
 * arms timer or the game's elapsed time. So the unit's definitions take
 * the port's names, and the C callers (lego_unit.c, legoobject_unit.c,
 * aitask.c) and the C++ ones reach them with no caller edit. The port tail
 * bubbles_port.cpp compiles its own definitions of these names out of the
 * matching build (LRR_BUBBLES_PORT_NAMES). Every port name here has C
 * linkage.
 *
 * Four functions keep the unit's names in every build:
 * bubble_get_bubble_type (00407300), bubble_live_object_update_bubble_image
 * (004078c0), bubble_callback_draw_obj_info (00407960) and
 * bubble_live_object_get_bubble_image (00407a10). Only the unit calls
 * them in the original. Their port names (bubble_action_name_to_index,
 * pending_type_value_slot_select) have C++ linkage and the port's
 * contracts, and the port tail keeps serving them.
 *
 * Included only by bubbles.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_BUBBLES_LINK_NAMES_H
#define LEGORRR_GAME_BUBBLES_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define bubble_initialise bubble_action_strings_initialize          /* 004070b0 */
#define bubble_load_bubbles bubble_config_texture_load              /* 00407240 */
#define bubble_toggle_object_uis_always_visible state_flag_toggle_when_zero /* 00407340 */
#define bubble_get_object_uis_always_visible global_task_refresh_flag_get   /* 00407360 */
#define bubble_reset_object_bubble_image entity_reset_auxiliary_state      /* 00407370 */
#define bubble_remove_object entity_pointer_registries_remove       /* 004073a0 */
#define bubble_show_power_off type4_object_registry_insert          /* 00407410 */
#define bubble_show_bubble idle_register                            /* 00407450 */
#define bubble_show_call_to_arms type2_timed_registry_insert        /* 004074b0 */
#define bubble_set_call_to_arms_timer set_type2_object_record_value /* 00407510 */
#define bubble_show_health_bar flagged_object_timer_registry_insert /* 00407540 */
#define bubble_draw_all_obj_infos screen_effects_tick               /* 004075a0 */
#endif

#endif /* LEGORRR_GAME_BUBBLES_LINK_NAMES_H */
