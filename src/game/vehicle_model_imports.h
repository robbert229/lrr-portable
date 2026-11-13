// SPDX-License-Identifier: LGPL-3.0-or-later
/* Vehicle.c (GAME-048) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6; finding 590). Included only by
 * vehicle_model.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, with the port's
 * parameter types; vehicle_model.c casts its original-layout records to
 * them. */
#ifndef LEGORRR_GAME_VEHICLE_MODEL_IMPORTS_H
#define LEGORRR_GAME_VEHICLE_MODEL_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/container.h"
#include "gods98/entity.h"
#include "gods98/mesh.h"
#include "gods98/model.h"
#include "gods98/record.h"
#include "gods98/texture.h"
#include "gods98/utils.h"
#include "gods98/view.h"
#include "entity.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SceneIndexedRecordList;
struct SceneChannelSlotsState;

/* GAME-001 Advisor.c: Maths_Vector3DScale, out of line (advisor.c). */
float* vec_scale(float* dst, const float* src, float s);          /* 00401720 */

/* GAME-047 Upgrade.c (upgrade_levels.cpp). The port's entries take its own
 * record list and channel state; the unit passes the vehicle's (finding
 * 590). */
void scene_indexed_record_list_rebuild_for_value(
    struct SceneIndexedRecordList* list, int value);              /* 0046c340 */
void scene_channel_slots_rebuild(struct SceneChannelSlotsState* state,
                                 int mode);                       /* 0046c510 */
/* Upgrade_Load (0046c160) has no annotated port entry with its contract;
 * PORT-ONLY adapter at the end of vehicle_model_port.cpp. */
void vehicle_upgrade_load_adapter(void* upgrades, BlockFile* config,
                                  const char* gameName);          /* 0046c160 */

/* G98-002 Container.c (container_unit.cpp). */
void scene_object_owner_set(void* scene_object, uint32_t owner); /* 00473da0 */
/* Container_EnableSoundTriggers (00473ec0): the port's entry takes a hook
 * table; PORT-ONLY adapter at the end of vehicle_model_port.cpp. */
void vehicle_container_enable_sound_triggers_adapter(void* cont,
                                                     int on);    /* 00473ec0 */
uint32_t scene_object_bounds_union(void* scene_object,
                                   float bounds_min_max[6]);     /* 00474f10 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);              /* 004754c0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3],
                                    float second[3]);             /* 00475500 */
void render_object_rotate_axis(void* object, uint32_t mode, float x, float y,
                               float z, float angle);             /* 00475580 */
void scene_object_camera_teleport_or_offset(void* object, uint32_t mode,
                                            float x, float y,
                                            float z);             /* 004755e0 */
void ddiscene_scene_object_attach_or_unlink(void* pair_record,
                                            void* source_pair_record); /* 00475710 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_VEHICLE_MODEL_IMPORTS_H */
