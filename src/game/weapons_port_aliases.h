// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-050 Weapons.c (finding 582): matching-build-only aliases from the
 * port's Weapons.c entry points to the original-shape functions in
 * weapons.c, for the C units that call them.
 *
 * The port's implementations stay in the port tail weapons_port.cpp,
 * unannotated, and native callers keep calling them: they work through the
 * port's hook tables and EntitySlot, not the original's records, so the
 * unit's functions cannot replace them natively. In the matching build
 * (MSVC 5.0, _MSC_VER 1100) these C callers call the unit's functions
 * instead, so their call operands name the original functions again.
 *
 * Include it first in a caller's file, before any declaration of these
 * names (as aitask_port_aliases.h). Each macro renames a declaration or a
 * call one for one, so MSVC's symbol numbering (finding 538) does not
 * change. Every aliased entry has C linkage. Only C units include it; not
 * weapons_port.cpp, which defines the port models. The entries weapons.c
 * shares with the port under the port's names (0046f0d0-0046f1a0) need no
 * alias; weapons.h aliases 0046f270 itself, for terrain_cell_objects.c, and
 * aitask_port_aliases.h aliases 0046eb80 for lego_unit.c (a new #include
 * would move those units' symbol counters, finding 538). */
#ifndef LEGORRR_GAME_WEAPONS_PORT_ALIASES_H
#define LEGORRR_GAME_WEAPONS_PORT_ALIASES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define see_through_walls_ray_check weapons_see_through_walls_check /* 00471960 */
#define entity_centers_within_object_radius weapons_object_in_touch_range /* 00472130 */
/* GAME-028 NERPs.c (finding 586), for aitask.c, which includes this header
 * and not aitask_port_aliases.h. */
#define system_state_flags_get_original nerpfunc_get_tutorial_flags /* 00456240 */
#endif

#endif /* LEGORRR_GAME_WEAPONS_PORT_ALIASES_H */
