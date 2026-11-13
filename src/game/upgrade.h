// SPDX-License-Identifier: LGPL-3.0-or-later
/* Upgrade.c (original translation unit GAME-047, finding 505): the upgrade
 * parts a vehicle or building shows for its upgrade level (finding 617).
 *
 * C-compatible header (ADR 0005). upgrade.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file: every include and declaration a header adds shifts MSVC's
 * symbol numbering (finding 506). src/game/test_c_boundaries.cpp includes
 * it.
 *
 * In the matching build upgrade_link_names.h, which upgrade.c includes
 * first, gives five of these functions the port's names (view.hpp,
 * resource.hpp), so every caller reaches them. Natively they keep these
 * names; the port tails tooltip_port.cpp and upgrade_levels_port.cpp serve
 * the port's names. */
#ifndef LEGORRR_GAME_UPGRADE_H
#define LEGORRR_GAME_UPGRADE_H

#include "gods98/config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Upgrade_PartModel;
struct Upgrade_Model;

void upgrade_part_load(struct Upgrade_PartModel* upgradeModel,
                       unsigned int objID, void* root,
                       const char* filename);                     /* 0046c030 */
void upgrade_part_clone(struct Upgrade_PartModel* srcUpgradeModel,
                        struct Upgrade_PartModel* destUpgradeModel); /* 0046c0b0 */
void upgrade_part_remove(struct Upgrade_PartModel* upgradeModel); /* 0046c0f0 */
float upgrade_part_move_animation(struct Upgrade_PartModel* upgradeModel,
                                  float elapsed,
                                  unsigned int repeat);           /* 0046c110 */
void upgrade_part_set_activity(struct Upgrade_PartModel* upgradeModel,
                               const char* activityName);         /* 0046c130 */
void upgrade_load(struct Upgrade_Model* upgrades, BlockFile* config,
                  const char* gameName);                          /* 0046c160 */
void upgrade_set_upgrade_level(struct Upgrade_Model* upgrades,
                               unsigned int objLevel);            /* 0046c340 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_UPGRADE_H */
