// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-015 Mesh.c (finding 625): the port's link names of six of the unit's
 * functions, for the matching build only (as viewports_link_names.h and
 * sfx_link_names.h).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: Container.c's port model
 * (container_unit.cpp) calls them at the original's call sites, with the
 * mesh, group and vertex records mesh_unit.c creates, and so do the C units
 * (sfx.c, weapons.c, screen_effect_quads.c) and the port tails that model
 * their emitters (sfx_port.cpp, weapons_port.cpp, level.cpp). So the unit's
 * definitions take the port's names: the callers reach them without an
 * edit, and the port tail mesh_port.cpp compiles its own definitions of
 * these names out of the matching build. Natively the port tail keeps
 * serving them.
 *
 * The unit's other functions keep its own names. gods98/mesh_unit.h,
 * screen_effect_quads_imports.h, lego_unit_imports.h and viewports_imports.h
 * point the C callers at them in the matching build; the port's C++ callers
 * of the other names keep the port's models.
 *
 * Included only by mesh_unit.c, before any other header. A macro renames a
 * declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. */
#ifndef LEGORRR_GODS98_MESH_LINK_NAMES_H
#define LEGORRR_GODS98_MESH_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define mesh_get_group indexed_record_entry_read /* 004821a0 */
#define mesh_add_group mesh_record_allocate_and_fill /* 00482220 */
#define mesh_get_vertices indexed_geometry_data_copy_to_output /* 00482470 */
#define mesh_is_group_hidden indexed_record_flag6c_bit0_get /* 00482780 */
#define mesh_set_group_texture animated_texture_frame_surface_set /* 004830c0 */
#define mesh_set_group_colour render_object_color_vector_set /* 00483270 */
#endif

#endif /* LEGORRR_GODS98_MESH_LINK_NAMES_H */
