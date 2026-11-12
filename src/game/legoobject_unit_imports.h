// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 LegoObject.c part 1 (issue #575): the C declarations of the
 * functions and globals of other modules that legoobject_unit.c calls or
 * reads.
 *
 * Every entry has C linkage. Where the owning module has a C++ header, its
 * declaration there carries extern "C" (in place, or as a pre-declaration
 * before a frozen <l>_for_<u>.hpp / <l>_from_<u>.hpp include) and
 * src/game/test_c_boundaries.cpp includes both, so a mismatch fails to
 * compile. They are kept here, not in the owners' C boundary headers, so
 * the other C units' symbol numbering does not change (finding 538); they
 * move to the owners' headers when the owners are converted. Where a
 * callee's contract differs from the original's, the port's contract is
 * used and noted. */
#ifndef LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H
#define LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct EntityAuxiliaryReleaseRecord;

/* ---- LegoObject.c's own state (objectGlobs, 004de790-004eadd0) --------
 * The port defines these in legoobject.cpp, each annotated at the field's
 * address, with C linkage. Parts 2 and 3 of the conversion still read
 * them there. */
extern void* s_pool_chunks[32];                     /* 004de790 listSet */
extern void* s_pool_record_head;                    /* 004de810 freeList */
extern int s_terrain_sfx_grid[300];                 /* 004de814 */
extern const char* s_activity[79];                  /* 004decc4 activityName */
extern uint32_t s_pool_chunk_count;                 /* 004dee10 listCount */
extern uint32_t s_entity_state_flags;               /* 004dee14 flags */
extern uint32_t s_activity_resource_slots[11];      /* 004dee18 */
extern uint32_t s_grid_count_second[0x12c0];        /* 004dee44 */
extern uint32_t s_task_counter_nonzero;             /* 004e2684 */
extern uint32_t s_grid_count_first[0x12c0];         /* 004e3944 */
extern uint32_t s_task_counter_zero;                /* 004e7184 */
extern int s_predug_coordinates[20][2];             /* 004e844c */
extern int s_type11_cell_coordinates[10][2];        /* 004e84ec */
extern unsigned int s_predug_coordinate_count;      /* 004e853c */
extern unsigned int s_type11_cell_coordinate_count; /* 004e8540 */
extern uint32_t s_render_config_snapshot[6];        /* 004ea7ac */
extern struct EntitySlot* s_idle100[100];           /* 004eabcc */
extern int s_idle100_n;                             /* 004ead5c */
extern const char* s_ability_type_names[6];         /* 004ead60 */
extern uint32_t s_global_animation_state;           /* 004eadc4 */
extern struct EntityAuxiliaryReleaseRecord* s_entity_auxiliary_release_records; /* 004eadd0 */

/* ---- group A ---- */

/* ---- group B ---- */

/* ---- group C ---- */
/* legoGlobs stand-ins defined in lego_unit.c (GAME-021). */
extern float legoGlobs_PowerCrystalRGB[3];                    /* 00556428 */
extern float legoGlobs_UnpoweredCrystalRGB[3];                /* 00556434 */
extern uint32_t legoGlobs_f_55644c;                           /* 0055644c */
extern uint32_t legoGlobs_f_556450;                           /* 00556450 */
struct ScenePairedAnimationState;
struct AnimationObject;
struct SceneObjectRecord;
struct RenderResourceBinding;
/* G98 (crt.hpp, sound.hpp, view.hpp, record.hpp, mesh.h,
 * scene_resources.hpp). */
void renderer_flag_bit6_set(int value);                       /* 004728e0 */
int sound_subsystem_enabled(void);                            /* 00465370 */
void sound_subsystem_enable_set(int enable, int stop_active); /* 00465260 */
void* entity_visual_resource_load(void* scene_object);        /* 00473690 */
void scene_object_owner_set(void* scene_object, uint32_t owner); /* 00473da0 */
int animated_texture_frame_count(const void* texture);        /* 00474a20 */
void scene_mesh_color_set(void* object, int block, float red, float green,
                          float blue);                        /* 00474ff0 */
void scene_mesh_color_and_alpha_set(void* object, int block, float red,
                                    float green, float blue,
                                    float alpha);             /* 00475020 */
/* FlicOpcode25SuccessStub (flic.hpp): six bytes that return 1. The
 * original's creature call (carry null frames) links to it with one
 * argument, so the unit calls it through a one-argument cast. */
uint32_t flic_opcode25_success_stub(void);                    /* 00484b90 */
/* Creature.c and Building.c (animation_object.h, record.hpp, view.hpp,
 * entity.hpp). */
void renderer_record_clone(struct AnimationObject* source,
                           struct AnimationObject* destination); /* 00406cb0 */
void scene_assign_entity_owner(const void* scene, uint32_t owner); /* 00408380 */
void scene_object_record_clone_retain_handles(
    const struct SceneObjectRecord* source,
    struct SceneObjectRecord* destination);                   /* 00408620 */
uint32_t scene_object_flags_get(const void* scene_object);    /* 00408850 */
void entity_payload_state_set(struct EntitySlot* e, int state); /* 004089a0 */
/* Upgrade.c (view.hpp). */
void render_resource_binding_copy_and_convert(
    const struct RenderResourceBinding* source,
    struct RenderResourceBinding* destination);               /* 0046c0b0 */
/* Vehicle.c (resource.hpp, record.hpp). Vehicle_SetUpgradeLevel's port
 * entry takes the record list and the channel state: the adapter at the
 * end of vehicle_model.cpp (GAME-021). */
void scene_record_indexed_entries_rebuild_original(void* vehicleModel,
                                                   int upgradeLevel); /* 0046cf90 */
void six_resource_slots_set_state(const void* record, uint32_t flag); /* 0046cfc0 */
int scene_record_template_instantiate(
    const struct ScenePairedAnimationState* source,
    struct ScenePairedAnimationState* destination);           /* 0046cff0 */
void scene_assign_owner_from_model(struct ScenePairedAnimationState* state,
                                   void* owner);              /* 0046d140 */
uint32_t object_record_field11c_get(const void* record);      /* 0046db60 */
/* Stats.c (entity_stats.cpp; entity.hpp, entity_from_entity_stats.hpp). */
int entity_building_index_set(struct EntitySlot* entity, uint32_t state); /* 00469c10 */
uint32_t entity_stats_word_get(const struct EntitySlot* e);   /* 00469e60 */
uint32_t entity_state_flags_get(const struct EntitySlot* e);  /* 00469e80 */
/* AITask.c (aitask.cpp; cmd.hpp, level.hpp). */
void entity_command15_validated_record_queue(struct EntitySlot* e); /* 00402670 */
void scalar_task_record_queue(void* scalar);                  /* 004034a0 */
void task_record_kind10_queue(void* payload);                 /* 00403520 */
void* timed_task_record_initialize_and_link(void* payload, int arg1,
                                            int arg2, int arg3); /* 004038c0 */
void task_record_create_and_scan_grid(void* entity);          /* 004039a0 */
void task_record_kind11_timed_queue(void* payload);           /* 004039d0 */
void task_record_kind14_remove_if_head(struct EntitySlot* e); /* 00403f60 */
/* HelpWindow.c (helpwindow.cpp, upgrades.hpp). */
void building_upgrade_state_changed_dispatch(int type, int row, int column,
                                             int state);      /* 00418830 */
/* LegoObject.c parts 2 and 3 (legoobject.cpp; entity.hpp, ai.hpp). The
 * port's 0043f560 and 0044b370 take a GameContext and 0044b680 is a
 * namespaced ledger stub with another contract: adapters at the end of
 * legoobject.cpp. 00449f50: GAME-021's adapter in lego.cpp. */
void entity_kind_four_post_create_resolve(struct EntitySlot* entity); /* 0043c570 */
int legoobject_gc_pool_find_with_local_predicate_adapter(struct EntitySlot* e); /* 0043f560 */
int activity_register_bookkeeping(struct EntitySlot* e);      /* 00447c40 */
void entity_type_operation_dispatch_adapter(struct EntitySlot* entity,
                                            int operation);   /* 00449f50 */
void legoobject_gc_activity_grant_adapter(struct EntitySlot* e, int value); /* 0044b370 */
int legoobject_gc_scene_object_tree_attach_adapter(struct EntitySlot* e); /* 0044b680 */

/* ---- group D ---- */

/* ---- group E ---- */

/* ---- group F ---- */

/* ---- group G ---- */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H */
