// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-045 Text.c (finding 607): the port's link names of the unit's
 * functions (as findings 590, 597, 598 and 600).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: the two text windows and the image
 * position, a message type and its text, a NERPs message line, or the
 * elapsed time. So the unit's definitions take the port's names, and the C
 * callers (lego_unit.c, legoobject_unit.c, aitask.c, advisor.c,
 * priorities.c, tutorial_script.c) and the C++ ones (frontend.cpp,
 * frameloop.cpp, lego_port.cpp, legoobject_port.cpp, entity.cpp,
 * aitask_port.cpp, tutorial_script_port.cpp) reach them with no caller
 * edit. The port tail text_port.cpp compiles its definitions of these names
 * out of the matching build (LRR_TEXT_PORT_NAMES), and lego_port.cpp its
 * PORT-ONLY no-op adapter for Text_Initialise. Every port name here has C
 * linkage.
 *
 * Text_Clear (0046a9e0) keeps the unit's name: only the unit calls it.
 *
 * Included only by text.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_TEXT_LINK_NAMES_H
#define LEGORRR_GAME_TEXT_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define text_load ui_message_strings_initialize                          /* 0046a7f0 */
#define text_initialise text_message_panel_initialise_adapter            /* 0046a950 */
#define text_restart ui_message_overlay_state_initialize                 /* 0046aa90 */
#define text_get_type_from_name ui_message_label_index_find              /* 0046aad0 */
#define text_set_message ui_message_text_slot_assign                     /* 0046ab10 */
#define text_set_message_with_image ui_message_text_slot_assign_with_image /* 0046abb0 */
#define text_set_nerps_message advisor_text_request_setup                /* 0046ac20 */
#define text_display_message advisor_text_animation_update               /* 0046ac60 */
#define text_update advisor_text_display_worker                          /* 0046ad00 */
#endif

#endif /* LEGORRR_GAME_TEXT_LINK_NAMES_H */
