// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-025 AnimClone.c (finding 616): the port's link names of three of the
 * unit's functions (as findings 597, 600, 607 and 612).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments:
 * - scene_material_create: terrain_map.c (Map3D.c's Map3D_Create) and
 *   view.cpp's wrapper pass the seven colour and power floats;
 * - scene_object_owner_link_release: container_unit.cpp passes the clone;
 * - ddiscene_timed_hook_dispatch: container_unit.cpp passes the clone, the
 *   time and NULL, and view.cpp's wrapper passes its three arguments on.
 * So the unit's definitions take the port's names, and those callers reach
 * them with no caller edit. The port tail com_object_tree_port.cpp compiles
 * its definitions of these names out of the matching build
 * (LRR_ANIMCLONE_PORT_NAMES). Every port name here has C linkage (mesh.h,
 * view.hpp).
 *
 * Included only by animclone.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GODS98_ANIMCLONE_LINK_NAMES_H
#define LEGORRR_GODS98_ANIMCLONE_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define material_create scene_material_create                   /* 004894c0 */
#define anim_clone_remove scene_object_owner_link_release       /* 00489750 */
#define anim_clone_set_time ddiscene_timed_hook_dispatch        /* 004897e0 */
#endif

#endif /* LEGORRR_GODS98_ANIMCLONE_LINK_NAMES_H */
