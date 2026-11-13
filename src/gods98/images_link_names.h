// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-011 Images.c (finding 632): the port's link names of two of the
 * unit's functions, for the matching build only (as mesh_link_names.h and
 * flic_link_names.h).
 *
 * Image_Initialise and Image_Shutdown are called only by Lego.c
 * (lego_unit.c), with the original's (empty) arguments, so in the matching
 * build (MSVC 5.0, _MSC_VER 1100) the unit's definitions take the port's
 * names and images_port.cpp compiles its models of them out there.
 *
 * The other functions keep the unit's names. The port's entries at their
 * addresses take the port's TextureSurface, and C++ port code calls them
 * with one, so the C units' headers (images.h, image_adapters.h,
 * font_imports.h, view_api.h and the units' imports headers) point only
 * the C callers at the unit, in the matching build.
 *
 * Included only by images_unit.c, before any other header. A macro renames
 * a declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. */
#ifndef LEGORRR_GODS98_IMAGES_LINK_NAMES_H
#define LEGORRR_GODS98_IMAGES_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define Image_Initialise renderer_record_table_initialize /* 0047d410 */
#define Image_Shutdown scene_object_registry_teardown /* 0047d430 */
#endif

#endif /* LEGORRR_GODS98_IMAGES_LINK_NAMES_H */
