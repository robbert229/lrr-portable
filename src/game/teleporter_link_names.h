// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-044 Teleporter.c (finding 621): the port's link names of the
 * unit's functions (as findings 598, 614 and 617).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments, so the unit's definitions take
 * the port's names: Lego.c (lego_unit.c, lego_unit_imports.h) and
 * Objective.c (objective_unit.cpp) reach them with no caller edit. The
 * port tail record_chains_port.cpp compiles its definitions of these names
 * out of the matching build (LRR_TELEPORTER_PORT_NAMES). All three port
 * names have C linkage.
 *
 * Teleporter_Update (0046a730) keeps the unit's name: Lego.c's port name
 * for it, camera_move_pipeline_original, is an adapter in lego_port.cpp,
 * so lego_unit_imports.h points Lego.c at teleporter_update instead.
 *
 * Included only by teleporter.c, before any other header. A macro renames
 * a definition one for one, so MSVC's symbol numbering (finding 538) does
 * not change. */
#ifndef LEGORRR_GAME_TELEPORTER_LINK_NAMES_H
#define LEGORRR_GAME_TELEPORTER_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define teleporter_restart global_linked_record_chain_clear           /* 0046a390 */
#define teleporter_service_all entity_flag_mask_callbacks_all_pass    /* 0046a4c0 */
#define teleporter_start scene_object_state_dispatch_original         /* 0046a510 */
#endif

#endif /* LEGORRR_GAME_TELEPORTER_LINK_NAMES_H */
