// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-009 (finding 609): the port's link names of the unit's functions (as
 * findings 590, 597 and 607).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments, so the unit's definitions take the
 * port's names:
 * - scrolling_text_display: frontend_unit.cpp's MainMenuTextItem
 *   (004103d0), which declares it with C linkage in the matching build;
 * - screen_effect_flags_clear_and_release, preload_numbered_bitmaps and
 *   entity_quad_mesh_render_record_allowed: lego_unit.c (and the port's
 *   C++ callers through frontend.hpp, resource.hpp and entity.hpp);
 * - quad_mesh_render_record_create: legoobject_unit.c;
 * - simtick_table_dispatch_original: lego_unit.c's DamageFont_UpdateAll
 *   call, whose PORT-ONLY adapter lego_port.cpp compiles out of the
 *   matching build.
 * The port tail screen_effect_quads_port.cpp compiles its definitions of
 * these names out of the matching build (LRR_SEQ_PORT_NAMES). Every port
 * name here has C linkage.
 *
 * DamageFont_GetNextFree, DamageFont_SetDamageNumber and DamageFont_Update
 * keep the unit's names: only the unit calls them.
 *
 * Included only by screen_effect_quads.c, before any other header. A macro
 * renames a definition one for one, so MSVC's symbol numbering (finding
 * 538) does not change. */
#ifndef LEGORRR_GAME_SCREEN_EFFECT_QUADS_LINK_NAMES_H
#define LEGORRR_GAME_SCREEN_EFFECT_QUADS_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define credits_play scrolling_text_display                                   /* 0040a0c0 */
#define damage_font_cleanup screen_effect_flags_clear_and_release             /* 0040a3d0 */
#define damage_font_load_frames preload_numbered_bitmaps                      /* 0040a400 */
#define damage_font_display_damage_over_live_object quad_mesh_render_record_create /* 0040a4b0 */
#define damage_font_update_all simtick_table_dispatch_original                /* 0040aa10 */
#define damage_font_live_object_check_can_show_damage entity_quad_mesh_render_record_allowed /* 0040aae0 */
#endif

#endif /* LEGORRR_GAME_SCREEN_EFFECT_QUADS_LINK_NAMES_H */
