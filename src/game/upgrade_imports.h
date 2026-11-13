// SPDX-License-Identifier: LGPL-3.0-or-later
/* Upgrade.c (GAME-047) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6; finding 617). Included only by
 * upgrade.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage. */
#ifndef LEGORRR_GAME_UPGRADE_IMPORTS_H
#define LEGORRR_GAME_UPGRADE_IMPORTS_H

#include "gods98/container.h"
#include "gods98/config.h"
#include "gods98/entity.h"
#include "gods98/utils.h"
#include "gods98/view.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

/* GAME-021 Lego.c (lego_unit.c): Lego_GetUpgradeData's lookup by name. */
void* lego_upgrade_name_entry_find(const char* name);            /* 0042cd50 */

/* GAME-023 LegoObject.c (legoobject_unit.c). */
int legoobject_remove(struct Lego_Object* obj);                  /* 00437540 */
struct Lego_Object* legoobject_create(void* model, int type,
                                      int id);                   /* 00437d00 */

/* GAME-023 AI.c's activity names (004decc4); legoobject_port.cpp defines
 * them. */
extern const char* s_activity[79];

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_UPGRADE_IMPORTS_H */
