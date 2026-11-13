// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-007 Collision.c (finding 603): the port's link names of the unit's
 * functions, for the matching build only (as finding 599's
 * fallin_link_names.h).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: pointers to 2D points (two
 * floats). So the unit's definitions take the port's names (view.hpp, C
 * linkage): legoobject_unit.c and weapons.c reach them without a caller
 * edit, and the port tail (placement_port.cpp) compiles its own definitions
 * of these names out of the matching build. Natively the port tail keeps
 * serving them.
 *
 * 004089d0 keeps the unit's name: its only caller is 00408b00, and the
 * port's segment_2d_point_distance has C++ linkage.
 *
 * Included only by collision.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_COLLISION_LINK_NAMES_H
#define LEGORRR_GAME_COLLISION_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define collision_polyline_point_distance point_to_polyline_minimum_segment_distance /* 00408b00 */
#define collision_project_onto_line point_project_onto_line2d /* 00408b60 */
#define collision_project_onto_direction point2d_project_onto_direction /* 00408bf0 */
#endif

#endif /* LEGORRR_GAME_COLLISION_LINK_NAMES_H */
