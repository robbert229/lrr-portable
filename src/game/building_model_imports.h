// SPDX-License-Identifier: LGPL-3.0-or-later
/* Building.c (GAME-006) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6; finding 592). Included only by
 * building_model.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, with the port's
 * parameter types; building_model.c casts its original-layout records to
 * them. */
#ifndef LEGORRR_GAME_BUILDING_MODEL_IMPORTS_H
#define LEGORRR_GAME_BUILDING_MODEL_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/container.h"
#include "gods98/entity.h"
#include "gods98/record.h"
#include "gods98/texture.h"
#include "gods98/utils.h"
#include "gods98/view.h"
#include "animation_object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SceneIndexedRecordList;
struct Upgrade_Model;
struct RenderResourceBinding;

/* GAME-023 AI.c's activity names (004decc4); legoobject.cpp defines them. */
extern const char* s_activity[79];

/* GAME-046 (tooltip.cpp): set a part's activity, falling back to
 * Activity_Stand. The port's entry takes the part as its binding record
 * (the container at +4). */
void scene_object_set_animation_with_fallback(
    struct RenderResourceBinding* binding, void* name);           /* 0046c130 */

/* GAME-047 Upgrade.c (upgrade.c; finding 617). */
void upgrade_load(struct Upgrade_Model* upgrades, BlockFile* config,
                  const char* gameName);                          /* 0046c160 */
void scene_indexed_record_list_rebuild_for_value(
    struct SceneIndexedRecordList* list, int value);              /* 0046c340 */

/* G98-002 Container.c (container_unit.cpp). */
uint32_t scene_object_flag_bit3_get_target(const void* scene_object); /* 00473ba0 */
void scene_object_owner_set(void* scene_object, uint32_t owner); /* 00473da0 */
void render_object_set_identity_transform(void* object);         /* 00475610 */
void ddiscene_scene_object_attach_or_unlink(void* pair_record,
                                            void* source_pair_record); /* 00475710 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_BUILDING_MODEL_IMPORTS_H */
