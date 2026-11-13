// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-012, the effects unit (finding 589): the port's link names of the
 * unit's functions, for the matching build only (as finding 585's
 * sfx_link_names.h).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: rock-fall style and type indices,
 * block coordinates, world positions and directions, the root container,
 * the config and the game name. So the unit's definitions take the port's
 * names: C and C++ callers reach them without an alias or a caller edit,
 * and the port tail (rock_fall_styles_port.cpp, and lego_port.cpp's
 * adapter for 0040bfd0) compiles its own definitions of these names out of
 * the matching build. Natively the port tail keeps
 * serving them through its hooks.
 *
 * Three names are port-only adapters that already had the original's
 * contract for lego_unit.c: rock_fall_initialise_adapter (0040bfd0),
 * global_resource_handle_load_and_enable_original (0040c720) and
 * misc_objects_effect_slots_bind_original (0040c990).
 *
 * The other twelve functions keep the unit's names:
 * - 0040bde0 and 0040beb0, whose port names a C++ port model
 *   (legoobject.cpp's 0x00441ea0, and the port's own 0040bde0) calls with a
 *   scene object, not a LegoObject;
 * - 0040c3a0, whose port model is defined in gods98/resource.cpp (left
 *   alone) and called by simtick.cpp on the port's state;
 * - nine whose port names have C++ linkage or another contract and which
 *   only this unit calls in the original.
 * aitask_port_aliases.h points the C callers of the first three at them.
 *
 * Included only by rock_fall_styles.c, before any other header. A macro
 * renames a declaration or a definition one for one, so MSVC's symbol
 * numbering (finding 538) does not change. */
#ifndef LEGORRR_GAME_ROCK_FALL_STYLES_LINK_NAMES_H
#define LEGORRR_GAME_ROCK_FALL_STYLES_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define effect_remove_all ledger_stub_0040bdc0_SetTimeoutTo10000 /* 0040bdc0 */
#define effect_spawn_boulder_explode_at scene_light_create_at_position /* 0040be10 */
#define effect_get_rock_fall_style rock_fall_style_name_lookup /* 0040bf70 */
#define effect_set_rock_fall_style rockfall_style_table_base_set /* 0040bfc0 */
#define effect_load_rock_fall_styles_all rock_fall_initialise_adapter /* 0040bfd0 */
#define effect_load_electric_fence_beam indexed_resource_handle_set_create /* 0040c1b0 */
#define effect_spawn_rock_fall scene_slot_pose_scale_register /* 0040c230 */
#define effect_spawn_electric_fence_beam transient_effect_render_slot_configure /* 0040c2f0 */
#define effect_get_block_pos_rock_fall scene_cell_table_pair_read /* 0040c570 */
#define effect_find_rock_fall_part scene_resource_slot_register_name /* 0040c5b0 */
#define effect_remove_all_boulder_explode scene_resource_singleton_pool_release /* 0040c690 */
#define effect_remove_all_rock_falls rock_fall_style_scene_resources_release /* 0040c6c0 */
#define effect_load_explosion global_resource_handle_load_and_enable_original /* 0040c720 */
#define effect_load_misc_objects misc_objects_effect_slots_bind_original /* 0040c990 */
#define effect_spawn_particle lava_effect_select_and_place /* 0040cce0 */
#endif

#endif /* LEGORRR_GAME_ROCK_FALL_STYLES_LINK_NAMES_H */
