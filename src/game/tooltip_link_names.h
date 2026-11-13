// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-046 ToolTip.c (finding 597): the port's link names of the unit's
 * functions (as findings 581, 585, 590 and 592).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: a tool tip type, a font and
 * colours, an Image, a sound effect id, or the mouse position and the
 * frame time. So the unit's definitions take the port's names, and the C
 * callers (lego_unit.c, interface_unit.c, panel_unit.c, priorities.c,
 * reward.c) and the C++ ones reach them with no caller edit. The port tail
 * tooltip_port.cpp compiles its own definitions of these names out of the
 * matching build (LRR_TOOLTIP_PORT_NAMES).
 *
 * Two of the unit's own names, tooltip_set_content and tooltip_draw_box,
 * are already the port's names: frontend.cpp's ToolTip_SetContent adapter
 * and tooltip_port.cpp's ToolTip_DrawBox adapter, which reward.c calls.
 * The matching build compiles those adapters out, so the unit's
 * definitions serve the names there. Natively the adapters keep them, and
 * the unit's definitions are renamed out of their way: the unit's state is
 * not the port's, so native callers must not reach it.
 *
 * Three functions keep the unit's names in every build. tooltip_draw
 * (0046bc30): its port name, timed_effect_slot_render_record, takes the
 * port's record and hooks. tooltip_set_flag10 (0046b730) and
 * tooltip_reset_timer (0046b7a0): their port names have C++ linkage in the
 * frozen frontend_for_*.hpp headers, so the port tail keeps serving them,
 * and interface_unit.c, their one caller with the original's contract,
 * calls the unit's names.
 *
 * Included only by tooltip.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_TOOLTIP_LINK_NAMES_H
#define LEGORRR_GAME_TOOLTIP_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define tooltip_initialise tooltip_overlay_state_initialize /* 0046b1d0 */
#define tooltip_get_type case_insensitive_name_table_lookup /* 0046b4d0 */
#define tooltip_add_icon command_sample_stats_accumulate /* 0046b660 */
#define tooltip_set_sfx timed_effect_cue_request_set_corpus /* 0046b700 */
#define tooltip_activate timed_effect_update_request_set_corpus /* 0046b770 */
#define tooltip_update timed_effect_runtime_update /* 0046b7c0 */
#else
#define tooltip_set_content tooltip_unit_set_content /* 0046b520 */
#define tooltip_draw_box tooltip_unit_draw_box /* 0046b8b0 */
#endif

#endif /* LEGORRR_GAME_TOOLTIP_LINK_NAMES_H */
