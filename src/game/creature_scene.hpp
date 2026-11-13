// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_CREATURE_SCENE_HPP
#define LEGORRR_CREATURE_SCENE_HPP

#include <stdint.h>

#if defined(LEGORRR_BACKEND_MINIWIN)

struct EntitySlot;
struct AnimationObject;
struct Model;

/* Native creature scenes: the type-row AnimationObjects (minifigure type 2,
 * rock monster type 3) and the per-entity clones that the animation-object
 * unit (game/animation_object.c, GAME-004) drives.
 *
 * The reference loads each type row with Creature_Load (0x406990) and
 * EntityCreateByType clones it with Creature_Clone (0x406cb0). The port's
 * type rows are 32-bit word tables, so the AnimationObject rows live here
 * instead, keyed by (type, row), and load through 0x406990 on first use.
 * Entities get their clone in attached_direction_record_type2/3. */
int creature_scene_type_path_set(int type, uint32_t row, const char* path,
                                 uint32_t feature);
void creature_scene_types_clear(int type);

/* Clone the entity's type row (EntityCreateByType). Returns 0 when the type
 * has no loadable scene; the entity then keeps the port's stand-ins. */
int creature_scene_entity_create(struct EntitySlot* entity);
/* Creature_Remove plus the record's own memory (EntityTeardown). */
void creature_scene_entity_release(struct EntitySlot* entity);
struct AnimationObject* creature_scene_model(const struct EntitySlot* entity);

/* The call sites' native arms (legoobject_port.cpp, entity.cpp), beside the
 * WinAPI actor_scene ones. */
/* EntityWorldPositionSet's creature branch: Creature_SetPosition at the
 * terrain height, then Creature_SetOrientation along the heading. */
void creature_scene_pose_apply(struct EntitySlot* entity);
/* EntityActivityTick (0x447b30): Creature_UpdateAnimation. Returns 0 when
 * the entity has no creature scene. */
int creature_scene_advance(struct EntitySlot* entity, float delta,
                           uint32_t repeats, float* overflow);
/* FUN_00447c40's type 2/3 change: Creature_SetActivity (0x406e40) with the
 * carried time, or the port's activity runner when the entity has no
 * creature scene. */
int creature_scene_activity_set(struct EntitySlot* entity,
                                const char* activity, float time);

/* Visit the creature's visible meshes with model-to-creature matrices
 * (LightWave axes: x right, y up, z forward). Returns the mesh count. */
typedef void (*CreatureSceneMeshVisitFn)(void* context,
                                         const struct Model* model,
                                         const float model_to_creature[16],
                                         float opacity);
unsigned int creature_scene_meshes_visit(const struct EntitySlot* entity,
                                         CreatureSceneMeshVisitFn visit,
                                         void* context);

#endif /* LEGORRR_BACKEND_MINIWIN */
#endif /* LEGORRR_CREATURE_SCENE_HPP */
