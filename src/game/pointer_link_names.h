// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-033 Pointer.c (finding 612): the port's link names of the unit's
 * functions (as findings 597, 600 and 607).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: a Pointers config item, a pointer
 * name and its type, a pointer type (with a timer), the mouse position or
 * the elapsed time. So the unit's definitions take the port's names, and
 * the C callers (lego_unit.c, info_unit.c, reward.c) and the C++ ones
 * (frontend.cpp, ui_list.cpp, mission.cpp, reward_port.cpp) reach them with
 * no caller edit. The port tail pointer_port.cpp compiles its definitions
 * of these names out of the matching build (LRR_POINTER_PORT_NAMES). Every
 * port name here has C linkage.
 *
 * Included only by pointer.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_POINTER_LINK_NAMES_H
#define LEGORRR_GAME_POINTER_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define pointer_initialise ui_pointer_cursor_name_registry_init          /* 0045c830 */
#define pointer_load_pointers pointer_cursor_config_load_target          /* 0045ca70 */
#define pointer_get_type pointer_cursor_config_parse_target              /* 0045cbd0 */
#define pointer_get_image indexed_global_value_at_get                    /* 0045cc10 */
#define pointer_set_current_if_timer_finished media_pointer_cursor_select /* 0045cc20 */
#define pointer_set_current media_pointer_cursor_override_set            /* 0045cc40 */
#define pointer_get_current_type global_cursor_word_get                  /* 0045cc60 */
#define pointer_draw_pointer pointer_cursor_draw_target                  /* 0045cc70 */
#define pointer_update media_pointer_cursor_override_tick                /* 0045cd90 */
#endif

#endif /* LEGORRR_GAME_POINTER_LINK_NAMES_H */
