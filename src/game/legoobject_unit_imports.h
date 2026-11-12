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
/* Config.c and Images.c records (BlockRec, Image). */
#include "gods98/config.h"
#include "gods98/images.h"

struct TextureSurface;

/* legoGlobs fields the port keeps as port globals. */
extern const char* s_engine_config_root;  /* engine.cpp, 005560c4 gameName */
extern const char* s_tool_type_names[];   /* ai.cpp, 005561fc toolName[11] */
/* objectGlobs fields defined elsewhere. */
extern float* s_terrain_neighbor_path_cost;      /* legoobject.cpp, 004dee00 */
extern void* s_object_toolTipIconAbilities[6];   /* lego_unit.c, 004ead78 */
extern void* s_object_toolTipIcons[11];          /* lego_unit.c, 004ead90 */
extern void* s_object_toolTipIconBlank;          /* lego_unit.c, 004eadbc */

/* LegoObject.c's own part-2/3 functions (legoobject.cpp, entity.hpp,
 * ai.hpp) whose port contract is the original's. */
void entity_iteration_callback_guard(void);              /* 0043b320 RemoveAll */
void ai_set_activity(struct EntitySlot* e, int id, int param); /* 004409e0 */
void entity_action_sequence_reset(struct EntitySlot* entity,
                                  int preserve_mode);    /* 00441940 */
int activity_register_bookkeeping(struct EntitySlot* e); /* 00447c40 */
/* LegoObject_GetBlockPos (004424f0): the port's entry also takes the level;
 * the GAME-013 port-tail adapter (legoobject.h) passes the current one. */
void legoobject_block_position_get(struct EntitySlot* entity, int* bx,
                                   int* by);
/* Stats.c (entity.hpp) and Weapons.c (weapons.cpp). */
uint32_t stats_table_word_at_144(int type, int row);     /* 00469ec0 */
float weapon_type_discharge_rate_get_target(int type);   /* 0046f140 */
/* Images.c: Image_SetupTrans (surface.hpp; the port takes the surface). */
void image_surface_blit_transformed(struct TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue); /* 0047dbf0 */

/* ---- group B ---- */
struct GameContext;
struct LegoBuildingModelE;
struct ScenePairedAnimationState;
struct RenderResourceBinding;
struct TerrainMap;
struct ElectricFenceObject;
struct InfoPoint2I;
/* legoGlobs stand-ins defined in lego_unit.c (GAME-021). */
extern uint32_t legoGlobs_flags1;                         /* 00556ecc */
extern struct LegoBuildingModelE* legoGlobs_buildingData; /* 005561e0 */
/* AITask.c (cmd.hpp, aitask.cpp). */
void argument_callback_dispatch(void* arg);                     /* 00402aa0 */
void cmd_create_task(struct EntitySlot* e, int32_t parameter);  /* 00402720 */
void task_callback_dispatch(void* payload);                     /* 00403a10 */
void entity_drain_command_chain(struct EntitySlot* e);          /* 00403a50 */
/* Bubble.c (ai.hpp, bubbles.cpp). */
void entity_pointer_registries_remove(struct EntitySlot* record); /* 004073a0 */
/* Creature.c / Building.c / Vehicle.c / Upgrade.c payload releases. */
void entity_payload_release_subslots(struct EntitySlot* e);     /* 00406ec0 */
void entity_payload_teardown(struct EntitySlot* e);             /* 00408940 */
uint32_t task_field84_get_and_field88_out(const void* task, uint32_t* out88); /* 00408360 */
void entity_payload_type1_release(
    struct ScenePairedAnimationState* state);                   /* 0046ce10 */
void render_resource_binding_release(
    struct RenderResourceBinding* binding);                     /* 0046c0f0 */
/* ElectricFence.c (terrain_cell_objects.h). */
void electric_fence_add(struct ElectricFenceObject* object);    /* 0040cf50 */
/* Encyclopedia.c (crt.hpp): takes the object as an int. */
void global_value_clear_if_equal(int value);                    /* 0040e910 */
/* Info.c (info_unit.h). */
void info_remove_object_references(void* object);               /* 00419830 */
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);                   /* 00419b80 */
/* Lego.c (entity.hpp; adapter at the end of lego.cpp). */
void object_pointer_registry_clear_match(int32_t pointer);      /* 0042dfc0 */
void* level_current_terrain_get_original(void);                 /* 00431570 */
/* LegoObject.c parts 2-3 (entity.hpp, legoobject.cpp, entity.cpp). */
void entity_linked_route_flag_clear(struct GameContext* ctx,
                                    struct EntitySlot* e);      /* 0043f100 */
void entity_linked_activity_event_dispatch(struct EntitySlot* entity,
                                           uint32_t events);    /* 0043f5b0 */
void entity_action_sequence_reset(struct EntitySlot* entity,
                                  int preserve_mode);           /* 00441940 */
void entity_world_position_set(struct EntitySlot* entity, float x, float y,
                               float heading, int update_heading); /* 004428a0 */
void entity_type_four_callback_dispatch(struct EntitySlot* source); /* 00447900 */
/* Map3D.c (terrain_map.h). */
int terrain_map_world_to_block(struct TerrainMap* map, float x, float y,
                               int* bx, int* by);               /* 0044f6d0 */
/* Message.c (cmd.hpp, message_unit.cpp). */
void selection_object_detach(void* entity);                     /* 004524b0 */
/* NERPs.c (crt.hpp, tutorial_script.cpp). */
void global_counter_004ff990_reset(void);                       /* 00454890 */
/* RotatedPairs (view.hpp, placement_footprint.cpp). */
void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation);      /* 00464110 */
/* SFX.c (sound.hpp, sfx.cpp). */
void sound_handle_stop_forward(int buffer_handle);              /* 00465190 */
/* Stats.c (entity.hpp, entity_stats.cpp). */
float entity_stats_float_ate8(const struct EntitySlot* e);      /* 00469d70 */
uint32_t entity_stats_word_get(const struct EntitySlot* e);     /* 00469e60 */
uint32_t entity_state_flags_get(const struct EntitySlot* e);    /* 00469e80 */
uint32_t entity_capability_flags_get(const struct EntitySlot* e); /* 00469ea0 */
/* G98 Container.c (container.h). */
void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object); /* 00472a40 */
/* G98 Utils.c (utils.h). */
char* string_duplicate(const char* text);                       /* 00477550 */
/* Original-ABI adapters at the end of legoobject.cpp (group B): the port
 * entries take a GameContext or hook table, or there is none. */
int legoobject_gb_weapon_target_clear_callback(void* obj, void* data); /* 00472060 */
int legoobject_gb_pool_find_with_local_predicate_adapter(void* obj); /* 0043f560 */
void legoobject_gb_auxiliary_payload_release_adapter(void* obj);   /* 0044be30 */
void* legoobject_gb_world_object_create_adapter(int id, void* position,
                                                unsigned int direction,
                                                void* points, uint32_t count,
                                                int notify);       /* 00409a90 */
void legoobject_gb_tutorial_counter_increment_adapter(int type);   /* 00454880 */

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
struct LevelState;
struct InfoPoint2I;
struct TerrainMap;
/* legoGlobs (Lego.c): currLevel is terrain_grid.cpp's port global; the
 * upgrade costs are stand-ins in lego_unit.c. */
extern struct LevelState* g_active_level_retail;    /* 005560d4 currLevel */
extern int legoGlobs_BuildingUpgradeCostOre;        /* 00556f98 */
extern int legoGlobs_BuildingUpgradeCostStuds;      /* 00556f9c */
/* Lego.c (lego_unit.c). 0041f8e0 is an empty function in this build; the
 * original's caller compares its (undefined) return value. */
void noop_callback(void);                           /* 0041f8e0 */
void global_object_field98_add(int amount);         /* 0041f8a0 */
int lego_upgrade_resource_check(int ore);           /* 0041f900 */
void lego_upgrade_resource_debit(int ore, int amount); /* 0041f9e0 */
/* 00431570 takes no argument; the port's entry takes two ignored cells
 * (adapter at the end of lego.cpp). */
void* level_current_terrain_get_original(void);     /* 00431570 */
/* Info.c (info_unit.c). */
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);       /* 00419b80 */
/* Map3D.c (terrain_map.c). */
int terrain_map_block_to_world(struct TerrainMap* map, unsigned int bx,
                               unsigned int by, float* xOut,
                               float* yOut);        /* 0044f640 */
/* Building.c (building_model.cpp): Building_SetUpgradeLevel; the port's
 * entry takes the object for the building model. */
void entity_payload_state_set(struct EntitySlot* e, int state); /* 004089a0 */
/* Stats.c (entity_stats.cpp). */
uint32_t entity_state_flags_get(const struct EntitySlot* e); /* 00469e80 */
/* 00469c10: the port's entry is declared only in a frozen header with C++
 * linkage; C entry at the end of entity_stats.cpp. */
int legoobject_gd_building_index_set_adapter(struct EntitySlot* entity,
                                             uint32_t state); /* 00469c10 */
uint32_t building_upgrade_admission_check(int type, int row); /* 00469f40 */
uint32_t entity_mapped_indexed_record_word_at_fc_get(int group, int index,
                                                     int slot); /* 0046a0c0 */
/* Vehicle.c (vehicle_model.cpp). */
uint32_t record_flag_bit_test(const void* record, uint32_t bit_index,
                              int reject_masked);   /* 0046cf40 */
/* Container.c (container_unit.cpp). */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]); /* 004754c0 */
/* Effects (rock_fall_styles.cpp). */
int lava_effect_select_and_place(int slot, const float* position,
                                 const float* rotation); /* 0040cce0 */
/* HelpWindow.c (helpwindow.cpp, namespace lego). */
void building_upgrade_state_changed_dispatch(int type, int row, int column,
                                             int state); /* 00418830 */
/* LegoObject.c parts 2-3 (legoobject.cpp, legoobject.h). */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y); /* 00442260 */
void entity_world_position_set(struct EntitySlot* entity, float x, float y,
                               float heading, int update_heading); /* 004428a0 */
/* 004424f0 LegoObject_GetBlockPos: adapter in the GAME-013 port tail
 * (legoobject.h). */
void legoobject_block_position_get(struct EntitySlot* entity, int* bx,
                                   int* by);
void* entity_type_component_get(const void* entity_record); /* 0044a200 */


/* ---- group E ---- */
struct LevelState;
struct TerrainMap;
struct TerrainMapVector3;
struct InfoPoint2I;
/* legoGlobs stand-ins defined in lego_unit.c (GAME-021). */
extern void* legoGlobs_contCrystal;                 /* 00556374 */
extern void* legoGlobs_contOresTable;               /* 0055637c */
extern char** legoGlobs_vehicleName;                /* 005561e8 */
extern char** legoGlobs_miniFigureName;             /* 005561ec */
extern char** legoGlobs_rockMonsterName;            /* 005561f0 */
extern char** legoGlobs_buildingName;               /* 005561f4 */
/* legoGlobs' object-name tables, defined in entity.cpp (C linkage from the
 * pre-declarations before entity_for_legoobject.hpp). */
extern const char* s_object_name_special[9];        /* 0055618c */
extern const char* s_object_the_name_special[9];    /* 005561b0 */
extern const char** s_object_name_kind1;            /* 00556254 */
extern const char** s_object_name_kind2;            /* 00556258 */
extern const char** s_object_name_kind3;            /* 0055625c */
extern const char** s_object_name_kind4;            /* 00556260 */
extern const char** s_object_name_kind10;           /* 00556264 */
extern const char** s_object_the_name_kind1;        /* 00556268 */
extern const char** s_object_the_name_kind2;        /* 0055626c */
extern const char** s_object_the_name_kind3;        /* 00556270 */
extern const char** s_object_the_name_kind4;        /* 00556274 */
extern const char** s_object_the_name_kind10;       /* 00556278 */
/* GAME-021 Lego.c (lego.cpp, level.hpp, entity_api.h). */
int entity_type_pair_lookup(const char* name, int* type_out, int* row_out,
                            int* zero_out);                     /* 0042e8b0 */
int terrain_cell_byte_state_classify_current(const int xy[2],
    uint32_t* out_a, uint32_t* out_b, uint32_t* out_c,
    uint32_t* out_d);                                           /* 0042c5b0 */
struct LevelState* level_current_owner_get(void);               /* 00431560 */
/* GAME-019 Info.c (info_unit.h). */
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);                   /* 00419b80 */
/* GAME-023 parts 2-3 (legoobject.cpp, entity.hpp). */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y);       /* 00442260 */
/* Map3D (terrain_map.h). */
float terrain_context_scale_get(const void* context);           /* 00450890 */
int terrain_map_block_vertices_get(struct TerrainMap* map, unsigned int bx,
                                   unsigned int by,
                                   struct TerrainMapVector3* positions); /* 004500d0 */
/* Message.c (message.h). */
void cmd_produce(int op, const void* a1, int a2, const int* pair); /* 00452060 */
/* Stats.c (entity.hpp). */
uint32_t entity_stats_word_get(const struct EntitySlot* e);    /* 00469e60 */
uint32_t entity_state_flags_get(const struct EntitySlot* e);   /* 00469e80 */
/* Text.c (frontend.hpp). */
void advisor_text_animation_update(int slot, int force, int selected); /* 0046ac60 */
/* Maths.c (maths.h). */
float random_float_range(float low, float high);                /* 004798b0 */

/* ---- group F ---- */

struct BlockRec;
struct TerrainMap;

/* Lego_Object passed to a port entry that takes EntitySlot. */
#define LEGOOBJ_SLOT(o) ((struct EntitySlot*)(o))

/* legoGlobs.toolName (005561fc, ai.cpp's s_tool_type_names) and, right
 * after it in the original's Lego_Globs, langToolName (00556228). */
#define LEGO_TOOL_TYPE_COUNT 11
extern const char* s_tool_type_names[8];                     /* ai.cpp, 005561fc */
#define legoGlobs_toolName (s_tool_type_names)
#define legoGlobs_langToolName ((char**)(s_tool_type_names + LEGO_TOOL_TYPE_COUNT))

/* G98 Config.c (config.h). */
const char* string_path_join(const char* root, ...);                         /* 00478f50 */
char* block_get_str_copy(struct BlockRec* root, const char* path);           /* 00479050 */
/* GAME-021 (lego_unit.c). */
void global_object_field98_clear_current(void);                              /* 0041f8c0 */
/* Lego_GetMap (00431570) takes no argument; the port's entry (level.hpp)
 * takes two it ignores. A cdecl call without them is the original's. */
void* level_current_terrain_get(int cell_x, int cell_y);                     /* 00431570 */
#define Lego_GetMap() ((struct TerrainMap*)((void* (*)(void))level_current_terrain_get)())
uint32_t terrain_cell_flag_8_bounded_current(uint32_t cx, uint32_t cy);      /* 00432b50 */
int terrain_cell_route_and_occupancy_check_current(
    const struct EntitySlot* entity, int from_x, int from_y,
    int to_x, int to_y, int strict);                                         /* 00431da0 */
/* GAME-026 Map3D (terrain_map.h). */
int terrain_map_world_to_block(struct TerrainMap* map, float x, float y,
                               int* bx, int* by);                            /* 0044f6d0 */
/* GAME-043 Stats.c (entity.hpp, entity_from_entity_stats.hpp). */
uint32_t stats_instance_word_at_ec(const void* config_block);                /* 00469bb0 */
uint32_t entity_stats_word_get(const struct EntitySlot* e);                  /* 00469e60 */
uint32_t entity_capability_flags_get(const struct EntitySlot* e);            /* 00469ea0 */
int entity_building_index_set(struct EntitySlot* entity, uint32_t state);    /* 00469c10 */
/* GAME-047 Vehicle.c (record.hpp): Vehicle_GetUpgradeLevel. */
uint32_t entity_type_mask_get(const void* record);                           /* 0046cf80 */
/* Vehicle_SetUpgradeLevel (0046cf90). The port's entry takes a record list
 * and a channel state instead; GAME-021's thin adapter in vehicle_model.cpp. */
void scene_record_indexed_entries_rebuild_original(void* vehicleModel, int upgradeLevel);
/* GAME-045 Text.c (frontend.hpp). */
void advisor_text_animation_update(int slot, int force, int selected);      /* 0046ac60 */
/* LegoObject.c parts 2/3, still port models in legoobject.cpp. Where the
 * port's parameter types differ (float arrays for the route's block
 * coordinates, a float count), the unit passes the original's values
 * through casts. */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x, float* out_y);      /* 00442260 */
int entity_route_setup_and_marker_associate(struct EntitySlot* entity,
                                            unsigned int count,
                                            const float* x_values,
                                            const float* y_values,
                                            const float* final_marker_position); /* 00441700 */
typedef int (*TerrainPathStopHook)(void* level, const uint32_t xy[2],
                                   void* user);
int terrain_neighbor_path_check_current(void* actor, uint32_t start_x,
    uint32_t start_y, uint32_t goal_x, uint32_t goal_y, int** out_x,
    int** out_y, float* out_count, TerrainPathStopHook stop, void* user);     /* 004410f0 */
int entity_flags_mask107c33de_or_1c00203_any_test(const struct EntitySlot* e); /* 0043baf0 */
/* 004424f0 LegoObject_GetBlockPos: the GAME-013 port-tail adapter. */
void legoobject_block_position_get(struct EntitySlot* entity, int* bx, int* by);

/* ---- group G ---- */

struct AnimationObject;
struct SceneResourceAnimationRecord;
struct LevelState;

/* legoGlobs.currLevel (005560d4): terrain_grid.cpp's g_active_level_retail. */
extern struct LevelState* g_active_level_retail;
#define LegoObjG_currLevel ((LegoObjG_Level*)g_active_level_retail)
/* objectGlobs.liveObjs (004eabcc) and liveCount (004ead5c). */
#define LegoObjG_liveObjs ((Lego_Object**)s_idle100)
#define LegoObjG_liveCount (*(unsigned int*)&s_idle100_n)

/* GAME-006/GAME-004/GAME-047 model handles. */
void* scene_object_primary_handle_get_or_create(void* record);              /* 004087b0 */
void* renderer_cached_handle_50_get_or_create(struct AnimationObject* object); /* 00407010 */
void* scene_resource_cached_slot_b8_get(struct SceneResourceAnimationRecord* record); /* 0046dac0 */
void* entity_type_component_get(const void* entity_record);                 /* 0044a200 */
/* GAME-021 Lego.c (lego_unit.c). 0041f8e0 is Lego_GetCrystalCount(flag) in
 * the original; lego_unit.c defines it as noop_callback(void), so the unit
 * calls it with the original's argument and result through a cast. */
void noop_callback(void);                                                    /* 0041f8e0 */
#define Lego_GetCrystalCount(onlyUsable) (((int (*)(int))noop_callback)(onlyUsable))
int lego_upgrade_resource_check(int ore);                                   /* 0041f900 */
void global_object_field8c_increment(void);                                 /* 0041f870 */
void lego_level_resource_balance_add_and_sound(int ore);                    /* 0041f9a0 */
/* legoGlobs.cont* (00556374-0055638c): lego_unit.c stand-ins. */
extern void* legoGlobs_contCrystal;            /* 00556374 */
extern void* legoGlobs_contDynamite;           /* 00556378 */
extern void* legoGlobs_contOresTable;          /* 0055637c, indexed by ore type */
extern void* legoGlobs_contOohScary;           /* 00556384 */
extern void* legoGlobs_contBarrier;            /* 00556388 */
extern void* legoGlobs_contElectricFence;      /* 0055638c */
void global_object_field8c_subtract(int amount);                            /* 0041f880 */
void lego_upgrade_resource_debit(int ore, int amount);                      /* 0041f9e0 */
/* G98 Container.c (view.hpp, container.h). */
void* scene_object_indexed_secondary_handle_get_or_create(void* record, int index); /* 00408810 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);                         /* 004754c0 */
uint32_t scene_action_name_set(void* scene_object, const char* action_name); /* 00473370 */
/* GAME-027 Message.c (cmd.hpp). */
void cmd_produce(int op, const void* a1, int a2, const int* pair);          /* 00452060 */
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);                 /* 00475430 */
/* GAME-002 AITask.c (cmd.hpp, entity.hpp; defined in aitask.cpp). */
void entity_command_unlink_by_kind(struct EntitySlot* e, int type,
                                   const void* extra, int unlink);           /* 00403d10 */
void entity_activity_record_remove_by_type(void* entity, int type,
                                           void* action_context);           /* 00403c00 */
/* GAME-021 Lego.c (level.hpp; port models in lego.cpp). */
int terrain_cell_state_equals_six_current(const int cell_xy[2]);            /* 00432f00 */
uint32_t terrain_cell_occupied_or_flag_current(int cx, int cy, int mode);   /* 00432aa0 */
/* LegoObject.c part 2 (entity.hpp). */
void entity_type5_finalize_if_allowed(struct EntitySlot* entity);           /* 00441ea0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]);        /* 00475500 */
/* LegoObject.c part 2 (ai.hpp), port models in legoobject.cpp. */
void ai_set_activity(struct EntitySlot* e, int id, int param);              /* 004409e0 */
int activity_register_bookkeeping(struct EntitySlot* e);                    /* 00447c40 */
/* 00447b30: the port's entry is in a namespace of a frozen header; thin
 * adapter at the end of legoobject.cpp. */
float legoobject_activity_tick_adapter(struct EntitySlot* entity, float delta);
/* G98 Maths.c (maths.h). */
short rng_thunk_draw15(void);                                               /* 004798a0 */
float random_float_range(float low, float high);                            /* 004798b0 */
/* GAME-002 (level.hpp; defined in aitask.cpp). */
void terrain_cell_record_counts_prune(void* entity);                        /* 00403910 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H */
