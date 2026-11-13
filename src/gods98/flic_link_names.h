// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-016 Flic.c (finding 632): the port's link names of five of the unit's
 * functions, for the matching build only (as mesh_link_names.h).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments, so the unit's definitions take the
 * port's names: the callers reach them without an edit, and the port's own
 * definitions of these names (flic.cpp, flic_port.cpp) are compiled out of
 * the matching build. Natively the port keeps serving them.
 *
 * - flic_setup and flic_animate are the original-ABI adapters (flic.h) that
 *   the C units Panel.c, Pointer.c, Reward.c and FrontEnd.c call.
 * - 00484b90 and 00484c90 are the original's shared do-nothing functions:
 *   the linker folded identical functions from other units into them, and
 *   Lego.c, LegoObject.c, NERPs.c, Water.c and Files.c call them by these
 *   names.
 * - object_field10_get (Flic_GetHeight) is a plain field read that
 *   FrontEnd.c calls on its overlay flic.
 *
 * Flic_GetPaletteColour (004850c0) keeps the unit's name: only the port's
 * own decoders call the port's indexed_word_table_lookup. Flic_Close keeps
 * the unit's name too: the port's flic_close is its player's
 * close (mission.cpp's flics), so flic_api.h points only the C callers at
 * the unit.
 *
 * Included only by flic_unit.c, before any other header. A macro renames a
 * declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. */
#ifndef LEGORRR_GODS98_FLIC_LINK_NAMES_H
#define LEGORRR_GODS98_FLIC_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define Flic_Setup flic_setup /* 00483c80 */
#define Flic_Animate flic_animate /* 00484070 */
#define Flic_Unsupported flic_opcode25_success_stub /* 00484b90 */
#define Flic_Nothing flic_opcode27_fallback_noop /* 00484c90 */
#define Flic_GetHeight object_field10_get /* 004850e0 */
#endif

#endif /* LEGORRR_GODS98_FLIC_LINK_NAMES_H */
