// SPDX-License-Identifier: LGPL-3.0-or-later
/* Dependencies.c (GAME-010) imports: C declarations of the callees the unit
 * uses from other modules (ADR 0005 step 6; finding 612). Included only by
 * dependencies.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, by the name it has in
 * the matching build. */
#ifndef LEGORRR_GAME_DEPENDENCIES_IMPORTS_H
#define LEGORRR_GAME_DEPENDENCIES_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

/* Config's ":" (004a1cd4), which config.c defines (finding 581). */
extern const char s_config_rgb_separator[];

/* Utils.c (utils.c): Util_Tokenise. */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */

/* Lego.c (lego_unit.c): Lego_GetObjectByName. */
int lego_entity_type_pair_lookup(const char* objName, int* outType,
                                 unsigned int* outID,
                                 void** outModel);                  /* 0042e8b0 */

/* LegoObject.c (legoobject_unit.c): LegoObject_RunThroughListsSkipUpgradeParts
 * and the task-kind check its callback makes first. */
int legoobject_run_through_lists_skip_upgrade_parts(
    int (*callback)(struct Lego_Object* obj, void* data), void* data); /* 004377b0 */
int legoobject_task_kind_filtered_search(int selector);              /* 00439200 */

/* Stats.c (entity_stats.c): Stats_GetLevels, by the port's name that its
 * definition takes in the matching build (entity_stats_link_names.h). */
uint32_t building_upgrade_admission_check(int type, int row);        /* 00469f40 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_DEPENDENCIES_IMPORTS_H */
