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
 * The port defines these in legoobject_port.cpp, each annotated at the field's
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
extern float* s_terrain_neighbor_path_cost;      /* legoobject_port.cpp, 004dee00 */
extern void* s_object_toolTipIconAbilities[6];   /* lego_unit.c, 004ead78 */
extern void* s_object_toolTipIcons[11];          /* lego_unit.c, 004ead90 */
extern void* s_object_toolTipIconBlank;          /* lego_unit.c, 004eadbc */

/* LegoObject.c part 3 (legoobject_port.cpp, ai.hpp). */
int legoobject_activity_register_bookkeeping(Lego_Object* obj); /* 00447c40 */
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
/* Lego.c: 0042dfc0 is lego_unit.c's (lego_unit.h, C linkage); 00431570 is
 * an adapter at the end of lego.cpp. */
void lego_object_pointer_registry_clear_match(void* object);    /* 0042dfc0 */
void* level_current_terrain_get_original(void);                 /* 00431570 */
/* LegoObject.c part 3 (entity.hpp, legoobject_port.cpp). */
void legoobject_type_four_callback_dispatch(Lego_Object* obj); /* 00447900 */
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
/* Original-ABI adapters at the end of legoobject_port.cpp (group B): the port
 * entries take a GameContext or hook table, or there is none. */
int legoobject_gb_weapon_target_clear_callback(void* obj, void* data); /* 00472060 */
void legoobject_auxiliary_payload_release(Lego_Object* obj);       /* 0044be30 */
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
/* LegoObject.c parts 2 and 3 (legoobject_port.cpp; entity.hpp, ai.hpp). The
 * port's 0043f560 and 0044b370 take a GameContext and 0044b680 is a
 * namespaced ledger stub with another contract: adapters at the end of
 * legoobject_port.cpp. 00449f50: GAME-021's adapter in lego.cpp. */
int legoobject_activity_register_bookkeeping(Lego_Object* obj); /* 00447c40 */
void legoobject_type_operation_dispatch(Lego_Object* obj,
                                        int hide);        /* 00449f50 */
void legoobject_activity_grant(Lego_Object* obj, int activity); /* 0044b370 */
void legoobject_scene_object_tree_attach(Lego_Object* obj); /* 0044b680 */

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
void* legoobject_type_component_get(Lego_Object* obj); /* 0044a200 */


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
#if defined(_MSC_VER) && defined(_M_IX86)
/* The matching build calls Lego.c's original-shape entries in lego_unit.c
 * (GAME-021 part 2, finding 571); their port models are unannotated. One
 * declaration either way (finding 538, rule B). */
int lego_entity_type_pair_lookup(const char* objName, int* outType,
                                 unsigned int* outID,
                                 void** outModel);              /* 0042e8b0 */
#define entity_type_pair_lookup(name, type_out, row_out, zero_out) \
    lego_entity_type_pair_lookup((name), (type_out), \
                                 (unsigned int*)(row_out), (void**)(zero_out))
int lego_terrain_cell_byte_state_classify_current(const int* blockPos,
    unsigned int* crystalsLv0, unsigned int* crystalsLv1,
    unsigned int* oreLv0, unsigned int* oreLv1);                /* 0042c5b0 */
#define terrain_cell_byte_state_classify_current(xy, a, b, c, d) \
    lego_terrain_cell_byte_state_classify_current((xy), (unsigned int*)(a), \
        (unsigned int*)(b), (unsigned int*)(c), (unsigned int*)(d))
#else
int entity_type_pair_lookup(const char* name, int* type_out, int* row_out,
                            int* zero_out);                     /* 0042e8b0 */
int terrain_cell_byte_state_classify_current(const int xy[2],
    uint32_t* out_a, uint32_t* out_b, uint32_t* out_c,
    uint32_t* out_d);                                           /* 0042c5b0 */
#endif
struct LevelState* level_current_owner_get(void);               /* 00431560 */
/* GAME-019 Info.c (info_unit.h). */
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);                   /* 00419b80 */
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
#if defined(_MSC_VER) && defined(_M_IX86)
/* Matching build: lego_unit.c's original-shape entry (finding 571). */
int lego_terrain_cell_route_and_occupancy_check(struct EntitySlot* liveObj,
    int bx, int by, int toX, int toY, int ignoreOwnBlock);                   /* 00431da0 */
#define terrain_cell_route_and_occupancy_check_current(e, fx, fy, tx, ty, s) \
    lego_terrain_cell_route_and_occupancy_check((struct EntitySlot*)(e), \
        (fx), (fy), (tx), (ty), (s))
#else
int terrain_cell_route_and_occupancy_check_current(
    const struct EntitySlot* entity, int from_x, int from_y,
    int to_x, int to_y, int strict);                                         /* 00431da0 */
#endif
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
void* legoobject_type_component_get(Lego_Object* obj);                 /* 0044a200 */
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
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]);        /* 00475500 */
/* LegoObject.c part 3 (ai.hpp). */
int legoobject_activity_register_bookkeeping(Lego_Object* obj); /* 00447c40 */
/* 00447b30: part 3's LegoObject_MoveAnimation. */
float legoobject_activity_tick(Lego_Object* obj, float elapsed);
/* G98 Maths.c (maths.h). */
short rng_thunk_draw15(void);                                               /* 004798a0 */
float random_float_range(float low, float high);                            /* 004798b0 */
/* GAME-002 (level.hpp; defined in aitask.cpp). */
void terrain_cell_record_counts_prune(void* entity);                        /* 00403910 */


/* ---- group H (part 2, issue #588) ---- */
struct AnimationObject;
struct ScenePairedAnimationState;
struct ViewProjectionAdapter;
struct Lego_Level;
struct LegoCreatureModelE;
struct TerrainMap;
/* legoGlobs stand-ins defined in lego_unit.c (GAME-021). */
extern void* legoGlobs_viewMain;                                            /* 005560dc */
extern struct LegoCreatureModelE* legoGlobs_rockMonsterData;                /* 005561dc */
/* GAME-002 AITask.c (aitask.c). */
void aitask_ai_idle_chain_record_attach(struct EntitySlot* object);        /* 00403f30 */
/* Creature.c (animation_object.c). */
void renderer_object_update_from_record_field64(struct AnimationObject* object,
                                                int swap);                  /* 00406c00 */
void renderer_object_update_from_record_field68(struct AnimationObject* object,
                                                int swap);                  /* 00406c30 */
void renderer_object_update_from_record_field6c(struct AnimationObject* object,
                                                int swap,
                                                unsigned int camera);       /* 00406c60 */
/* Effects (rock_fall_styles.cpp, resource.hpp). */
void scene_marker_create_at_position_or_object(void* object,
                                               const float position[3]);  /* 0040beb0 */
/* GAME-021 Lego.c (lego_unit.c). */
void lego_periodic_callback_dispatch(int on);                              /* 00425b40 */
uint32_t active_engine_field_b0_get_current(void);                         /* 00429850 */
void lego_cell_init_pass(struct Lego_Level* level, unsigned int bx,
                         unsigned int by, int reserved);                   /* 0042f6f0 */
void terrain_cell_work_flag_clear_current(const int coord[2],
                                          uint32_t selector);              /* 00430320 */
void lego_terrain_cell_update_queue_flush(void);                           /* 004321a0 */
void lego_terrain_cell_previous_state_track(void);                         /* 00432200 */
/* GAME-021 Lego.c (lego.cpp, level.hpp). */
int terrain_cell_walkable_flag_current(uint32_t cx, uint32_t cy);          /* 00432bd0 */
uint32_t terrain_cell_flag_20_bounded_current(uint32_t cx, uint32_t cy);   /* 00432d90 */
uint32_t terrain_cell_flag_reserved_current(const int cell_xy[2]);         /* 00432dd0 */
void level_tile_mark_current_target(const int cell_xy[2], int set);        /* 00432e00 */
uint32_t terrain_cell_mask_c0_current(int cx, int cy);                     /* 00432e60 */
int terrain_cell_state_equals_one_current(const int cell_xy[2]);           /* 00432ec0 */
uint32_t terrain_cell_flag_8000_current(const int cell_xy[2]);             /* 00433030 */
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);             /* 00433090 */
int terrain_cell_mask_180000_current(const int cell_xy[2]);                /* 004330e0 */
/* Original-ABI adapters at the end of legoobject_port.cpp (group H): the port's
 * 00435230 takes the cell as two ints, its 0044a4f0 is in a namespace, and
 * the port has no 0044bf00. */
void legoobject_gh_random_variant_cell_scatter_adapter(const int* blockPos,
                                                       int radius,
                                                       int mode);           /* 00435230 */
int legoobject_nearest_cell_coordinate_world_resolve(Lego_Object* obj,
                                                     int* outBlockPos); /* 0044a4f0 */
void legoobject_timed_callback_list_update(float* elapsed);              /* 0044bf00 */
/* Map3D.c (terrain_map.c). */
float terrain_map_world_z_get(struct TerrainMap* map, float x, float y);   /* 0044f940 */
/* NERPs.c (tutorial_script.cpp, message.hpp): the original takes the
 * NERPs argument list; the port's entry takes none. */
uint32_t system_state_flags_get(void);                                     /* 00456240 */
#define LegoObjH_NERPsTutorialFlags(args) (((uint32_t (*)(int))system_state_flags_get)(args))
/* Reward.c (reward.c). */
void global_counter_at_00555324_increment(void);                           /* 00462870 */
/* SFX.c (sfx.cpp, sound.hpp). */
int sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                   int loop, int positional,
                                   const float* xyz);                      /* 00465050 */
/* GAME-043 Stats.c (entity.hpp, entity_stats.cpp). */
float entity_stats_float_at74(const struct EntitySlot* e);                 /* 00469d50 */
float entity_stats_float_at78(const struct EntitySlot* e);                 /* 00469d60 */
float entity_stats_float_at94(const struct EntitySlot* e);                 /* 0046a040 */
/* Vehicle.c (vehicle_model.cpp, resource.hpp). */
void paired_resource_state_set(struct ScenePairedAnimationState* state,
                               int enabled);                               /* 0046ced0 */
void scene_entity_animation_tracks_set_enabled(
    struct ScenePairedAnimationState* state, int enabled);                 /* 0046cf20 */
/* G98 Viewports.c (viewports.cpp, view.hpp). */
void camera_screen_coordinate_project(
    const struct ViewProjectionAdapter* adapter, float* output,
    const float* input);                                                    /* 00477290 */
void render_interface_transform_point(void* object, void* output,
                                      const void* input);                   /* 004772b0 */
/* G98 Maths.c (maths.c). */
int sphere_ray_intersection_discriminant_positive(const float* center,
                                                  float radius,
                                                  const float* camera,
                                                  const float* line);       /* 00479c10 */

/* ---- group I (part 2, issue #588) ---- */
struct EntitySlot;
/* GAME-002 AITask.c (aitask.c). */
void aitask_entity_deferred_work_schedule(struct EntitySlot* object);      /* 004026c0 */
/* In the matching build part 1's entity_command_unlink_by_kind (above)
 * names this function (aitask_port_aliases.h) with a const void* target;
 * aitask.h declares a struct EntitySlot*. Each build sees one type. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LEGOOBJ_I_UNLINK_TARGET const void*
#else
#define LEGOOBJ_I_UNLINK_TARGET struct EntitySlot*
#endif
void aitask_entity_command_unlink_by_kind(struct EntitySlot* object, int type,
                                          LEGOOBJ_I_UNLINK_TARGET target,
                                          int unlink);                      /* 00403d10 */
/* Bubble.c (ai.hpp, bubbles.cpp). */
void type4_object_registry_insert(struct EntitySlot* record);              /* 00407410 */
/* Building.c placement (placement.cpp): both port entries are in a
 * namespace; adapters at the end of legoobject_port.cpp. */
void legoobject_gi_placement_cell_search_expand_adapter(const int cell_xy[2]); /* 004093b0 */
int legoobject_gi_nearby_placement_candidate_find_adapter(const int start_xy[2],
                                                          int resource_amount); /* 00409450 */
/* GAME-019 Info.c (info_unit.c). */
struct InfoPoint2I;
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);                               /* 00419b80 */
/* GAME-021 Lego.c (lego_unit.c, lego.cpp). */
float engine_float_field210_get_by_index_current(int index);               /* 00429860 */
float engine_float_field260_get_current_target(void);                      /* 00429880 */
uint32_t global_state_field84_get_current_target(void);                    /* 0042b500 */
void terrain_cell_work_flag_circle_set_current(const int center_xy[2],
                                               int radius);                 /* 00435010 */
/* LegoObject.c part 3 (ai.hpp, legoobject_port.cpp). */
int legoobject_activity_scan(Lego_Object* obj, int activity);            /* 0044b480 */
/* Map3D.c (terrain_map.c). */
float terrain_context_scale_get(const void* context);                      /* 00450890 */
/* MapShared draw (view.hpp, map_view.cpp). */
void textured_scene_object_draw_at_position(const void* view,
                                            const float position_xy[2],
                                            float z, float size);           /* 0045db00 */
/* GAME-043 Stats.c (entity.hpp, entity_stats.cpp). */
uint32_t stats_instance_word_at_bc(const void* config_block);              /* 00469b50 */
uint32_t stats_instance_word_at_e0(const void* config_block);              /* 00469b80 */
float stats_table_float_at_dc(int type, int row);                          /* 00469e00 */
float entity_stats_float_atdc(const struct EntitySlot* e);                 /* 00469e20 */
uint32_t entity_stats_word_atac(const struct EntitySlot* e);               /* 00469e40 */
uint32_t entity_stats_word_get(const struct EntitySlot* e);                /* 00469e60 */
uint32_t entity_state_flags_get(const struct EntitySlot* e);               /* 00469e80 */
/* 0043bc40's callees. */
struct LegoObject;
struct AnimationObject;
struct ElectricFenceObject;
extern void* legoGlobs_contMiniTeleportUp;                                 /* 00556398 */
void aitask_task_registry_remove_type10_entries_for_entity(
    struct EntitySlot* object);                                             /* 00403560 */
unsigned int activity_runner_update(struct AnimationObject* object,
                                    const char* activityName, float time); /* 00406e40 */
void entity_destroy_tracked_pointer_cleanup(void* entity);                  /* 00409ff0 */
void electric_fence_remove(struct ElectricFenceObject* object);            /* 0040d0a0 */
void interface_driven_object_menu_check(struct LegoObject* obj);           /* 0041cf20 */
void* legoobject_type_component_get(Lego_Object* obj);                /* 0044a200 */
void legoobject_kind_one_random_spawn_trigger(Lego_Object* obj);           /* 0044ade0 */
int legoobject_activity_register_bookkeeping(Lego_Object* obj); /* 00447c40 */
void selection_object_detach(void* entity);                                /* 004524b0 */
int selection_entity_remove_active(struct EntitySlot* entity);             /* 00452c50 */
float entity_stats_float_at6c(const struct EntitySlot* e);                 /* 00469d40 */
uint32_t entity_capability_flags_get(const struct EntitySlot* e);          /* 00469ea0 */
void scene_record_indexed_entries_rebuild_original(void* vehicleModel,
                                                   int upgradeLevel);       /* 0046cf90 */
uint32_t scene_action_name_set(void* scene_object, const char* action_name); /* 00473370 */
void* entity_visual_resource_load(void* scene_object);                     /* 00473690 */
void scene_render_lock_toggle(void* scene_object, int enabled);            /* 00473b40 */
float ddiscene_frame_update_and_callbacks(void* scene_object, float time); /* 00475140 */
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);                /* 00475430 */
int ddiscene_object_pose_method_dispatch(void* object, void* alternate_frame,
                                         float parameter_3, float parameter_4,
                                         float parameter_5, float parameter_6,
                                         float parameter_7,
                                         float parameter_8);                /* 00475470 */
void scene_frame_scale_set(void* object, uint32_t mode, float x, float y,
                           float z);                                        /* 004755b0 */
/* 0043eea0's callees. */
extern void* legoGlobs_contProcessedOre;                                   /* 00556380 */
void scene_object_scalar_adjust_by_direction(void* record, int direction); /* 004082e0 */
void* scene_object_primary_handle_get_or_create(void* record);             /* 004087b0 */
void lego_level_resource_balance_add_and_sound(int ore);                   /* 0041f9a0 */
void cmd_produce(int op, const void* a1, int a2, const int* pair);         /* 00452060 */
uint32_t stats_instance_word_at_ec(const void* config_block);              /* 00469bb0 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);                        /* 004754c0 */
/* 0043f130-0043f6a0's callees. */
extern unsigned int legoGlobs_f_556514;   /* lego_unit.c, 00556514 objTeleportQueue count */
void aitask_entity_threshold_progress_event_queue(struct EntitySlot* object,
                                                  int urgent);              /* 00402f30 */
void flagged_object_timer_registry_insert(struct EntitySlot* record);      /* 00407540 */
/* 00435aa0 (Lego.c part 3): the port's entry also takes the engine
 * context; GAME-021's adapter at the end of lego.cpp. */
void lego_panel_selection_highlight_toggle_adapter(int on);                /* 00435aa0 */
void global_float_accumulator_00555330_add(float value);                   /* 004628a0 */
void global_float_accumulate_00555120(float value);                        /* 004628c0 */
float entity_stats_float_at80(const struct EntitySlot* e);                 /* 00469dc0 */
uint32_t scene_object_owner_get(const void* scene_object);                 /* 00473db0 */
/* 0043f190's callees. */
struct TerrainMap;
void global_object_field8c_subtract(int amount);                           /* 0041f880 */
int lego_entity_at(int objType, unsigned int objID, void** outModel);      /* 0042ed70 */
int terrain_map_world_to_block(struct TerrainMap* map, float x, float y,
                               int* bx, int* by);                           /* 0044f6d0 */
int terrain_map_block_to_world(struct TerrainMap* map, unsigned int bx,
                               unsigned int by, float* xOut,
                               float* yOut);                                /* 0044f640 */
int entity_state_override_pop(struct EntitySlot* entity);                  /* 00459240 */
uint32_t entity_indexed_record_word_at_b4_get(int group, int index,
                                              int slot);                    /* 00469af0 */
uint32_t stats_table_word_at_148(int type, int row);                       /* 00469ee0 */
void six_resource_slots_set_state(const void* record, uint32_t flag);      /* 0046cfc0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]);       /* 00475500 */
/* 0043f7d0's callees. */
extern uint32_t s_panel_highlight_flags;  /* engine.cpp, 00556ed0 legoGlobs.flags2 */
extern float legoGlobs_MinEnergyForEat;   /* lego_unit.c, 00556458 */
void aitask_entity_target_command_queue(struct EntitySlot* object,
                                        struct EntitySlot* target);         /* 00402f70 */
int aitask_entity_task_chain_contains_kind(struct EntitySlot* object,
                                           int type);                       /* 004041b0 */
void quad_mesh_render_record_create(void* entity, uint32_t count);         /* 0040a4b0 */
int electric_fence_object_strike(struct ElectricFenceObject* object);      /* 0040de40 */
void level_state_field9c_add(int amount);                                  /* 0041f920 */
int entity_cell_type_speed_multiplier_resolve_current(
    const struct EntitySlot* entity, int x, int y, float scale,
    float* out_scale);                                                      /* 00433180 */
float entity_energy_decay_rate_get(const struct EntitySlot* e);            /* 00469de0 */
/* Original-ABI adapters at the end of legoobject_port.cpp (group I): 00459190
 * takes the object (the port's osf_add a context and its fields), and the
 * port's 00409d50 is in a namespace. */
void legoobject_gi_osf_add_adapter(void* obj);                             /* 00459190 */
void legoobject_gi_cell_path_effects_apply_adapter(void* obj);             /* 00409d50 */
/* Weapons.c (entity.hpp, weapons.cpp). */
int object_pair_ray_hit_check(void* first, void* second);                  /* 004718d0 */
int entity_centers_within_configured_radius(const struct EntitySlot* first,
                                            const struct EntitySlot* second); /* 00472080 */

/* ---- group J (part 2, issue #588) ---- */

/* The callees of LegoObject_Callback_Update (0043c810) outside GAME-023
 * part 2. Where the port's entry takes another contract the unit calls it
 * through a cast (only the matching build runs the unit) or through a
 * PORT-ONLY adapter in legoobject_port.cpp. */
struct AnimationObject;
struct LegoObject;
struct Lego_Level;
struct LegoPoint2I2A;
struct EngineContext;
struct GameContext;
void ai_idle_chain_record_attach(struct EntitySlot* entity);          /* 00403f30 */
void aitask_task_pair_record_enqueue(const int* blockPos);            /* 004034e0 */
void animation_object_time_update(struct AnimationObject* object,
                                  float time);                        /* 00406d10 */
float animation_object_time_get(struct AnimationObject* object);      /* 00406da0 */
void* renderer_cached_handle_40_get_or_create(void* object);          /* 00406fb0 */
int renderer_cached_handle_4c_is_not_one(struct AnimationObject* object); /* 00407040 */
void type2_timed_registry_insert(struct EntitySlot* record);          /* 004074b0 */
void set_type2_object_record_value(struct EntitySlot* record,
                                   uint32_t value);                   /* 00407510 */
void* terrain_descriptor_find_exact_current(const uint32_t* pair_key,
                                            const uint32_t* single_key); /* 004090c0 */
void simtick_selection_group_release(const uint32_t key_pair[2]);    /* 00409110 */
void interface_object_menu_check(struct LegoObject* obj);             /* 0041cea0 */
void global_object_field94_increment(void);                           /* 0041f980 */
void global_object_fielda8_increment(void);                           /* 0041f990 */
void* lego_mode_dependent_field24_get(void);                          /* 00423080 */
void lego_air_supply_amount_update(struct EntitySlot* liveObj,
                                   float elapsed);                    /* 00424560 */
int lego_emerge_map_trigger_spawn(struct Lego_Level* level,
                                  const struct LegoPoint2I2A* blockPos,
                                  struct LegoPoint2I2A* outPos);      /* 0042c330 */
int lego_terrain_cell_mutate(struct Lego_Level* level, unsigned int bx,
                             unsigned int by, int isHiddenCavern);    /* 00430530 */
int lego_terrain_cell_initialize(struct Lego_Level* level, unsigned int bx,
                                 unsigned int by);                    /* 00430ee0 */
void lego_terrain_cell_flag20_clear_and_refresh(const int* blockPos); /* 004310f0 */
int lego_terrain_cell_material_get(int bx, int by);                   /* 004319b0 */
int terrain_cell_variant_flag_propagate_current(const int cell_xy[2]); /* 00432600 */
int terrain_cell_walkability_query_current(const struct EntitySlot* e,
                                           uint32_t cx, uint32_t cy,
                                           int require_stat_bit,
                                           int gated_mode);           /* 00432770 */
int terrain_cell_walkable_flag_current(uint32_t cx, uint32_t cy);     /* 00432bd0 */
int terrain_cell_speed_class_current(const int cell_xy[2]);           /* 00432c50 */
uint32_t terrain_cell_flag_20_bounded_current(uint32_t cx, uint32_t cy); /* 00432d90 */
void level_tile_mark_current_target(const int cell_xy[2], int set);   /* 00432e00 */
void current_level_terrain_grid_pick(struct EngineContext* ctx, float x, float y,
                                     int* gx, int* gy);               /* 00433be0 */
int entity_neighbor_blocked_cell_step_probe(struct EntitySlot* entity,
                                            float cadence, int mode); /* 00433c10 */
int terrain_cell_drill_work_add_current(int x, int y, float divisor,
                                        float elapsed);               /* 00434570 */
void panel_selection_highlight_toggle(struct EngineContext* ctx, int on);             /* 00435aa0 */
int legoobject_progress_advance_by_25(Lego_Object* obj);              /* 00447730 */
void legoobject_clear_linked_object_state(Lego_Object* obj);          /* 00447780 */
int legoobject_activity_terrain_eligibility_transition(Lego_Object* obj, float elapsed,
                                                      int* out);      /* 00449980 */
/* Deviation (scoring only): the third parameter is unnamed so the
 * declaration keeps the two names the port's prototype had (finding 538:
 * parameter names count). */
int legoobject_work_object_position_get(Lego_Object* obj, float* outX,
                                        float*);                      /* 0044a2a0 */
int legoobject_ai_can_do(Lego_Object* obj);                           /* 0044c530 */
float terrain_map_world_z_get(struct TerrainMap* map, float x, float y); /* 0044f940 */
int sound_alias_play(int alias, int loop);                            /* 00464fa0 */
int sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                   int loop, int positional,
                                   const float* xyz);                 /* 00465050 */
float entity_stats_float_at24_indexed(const struct EntitySlot* e, int index); /* 00469cc0 */
uint32_t entity_stats_word_at_c0_or_c4(const struct EntitySlot* e,
                                       int selector);                 /* 00469f90 */
uint32_t entity_stats_word_atc8(const struct EntitySlot* e);          /* 00469fc0 */
float entity_stats_float_atf4(const struct EntitySlot* e);            /* 0046a080 */
float entity_speed_component_get(const struct EntitySlot* e);         /* 0046a0a0 */
uint32_t stats_table_word_at_100(int type, int row, int stride_index); /* 0046a0f0 */
void scene_object_attached_resource_release(void* record);           /* 0046d1a0 */
int object_pair_ray_hit_check(void* first, void* second);             /* 004718d0 */
int entity_flag100_and_shared_eligibility_check(const struct EntitySlot* e); /* 00471ce0 */
int entity_velocity_slot_dispatch(void* entity, void* argument);      /* 00471fc0 */
int entity_centers_within_configured_radius(const struct EntitySlot* first,
                                            const struct EntitySlot* second); /* 00472080 */
float ddiscene_advance_by_time_target(void* scene_object, float delta); /* 00475120 */
float ddiscene_frame_update_and_callbacks(void* scene_object, float time); /* 00475140 */
void ddiscene_apply_act_frame_timing(void* scene_object);            /* 00475300 */
int ddiscene_object_pose_method_dispatch(void* object, void* alternate_frame,
                                         float parameter_3, float parameter_4,
                                         float parameter_5, float parameter_6,
                                         float parameter_7, float parameter_8); /* 00475470 */
void render_object_rotate_axis(void* object, uint32_t mode, float x,
                               float y, float z, float angle);        /* 00475580 */
void scene_object_camera_teleport_or_offset(void* object, uint32_t mode,
                                            float x, float y, float z); /* 004755e0 */
float* d3drm_vector_random(float* r);                                 /* 00479500 */
void flic_opcode27_fallback_noop(void);                               /* 00484c90 */
extern uint32_t s_panel_highlight_flags;  /* engine.cpp, 00556ed0 */
extern int legoGlobs_ReinforceHits;       /* lego_unit.c, 00556f08 */
/* PORT-ONLY adapters in legoobject_port.cpp (group J). */
void legoobject_linked_proximity_response(Lego_Object* obj); /* 004474f0 */
int legoobject_progress_negative_delta_handle(Lego_Object* obj); /* 004475c0 */
int legoobject_route_step_command_issue(Lego_Object* obj); /* 00449700 */
void legoobject_motion_to_scene_node_sync(Lego_Object* obj,
                                          float elapsed);             /* 0044bc30 */
int legoobject_monster_spawn_behavior_tick(Lego_Object* obj); /* 0044bfd0 */
void legoobject_heading_step_move(Lego_Object* obj,
                                  float elapsed);                     /* 0044c1b0 */
void legoobject_attached_entity_wake(Lego_Object* obj); /* 0044c110 */

/* ---- group K (part 2, issue #588) ---- */

struct SceneResourceAnimationRecord;

/* LegoObject_GetCarryNull?: legoobject_visual_resource_get_or_create
 * (0043a5f0) returns the object's carry null or, failing that, its
 * activity container; part 1 declares it void (its result is the last
 * call's), so group K reads the result through a cast. */
#define legoobject_k_carry_null_get(obj) \
    (((void* (*)(Lego_Object*))legoobject_visual_resource_get_or_create)(obj))

/* Maths_Vector3DModulus (Maths.h inline; the unit includes <math.h>). */
#define legoobject_vector3d_modulus(v) \
    ((float)sqrt((v)[0] * (v)[0] + (v)[1] * (v)[1] + (v)[2] * (v)[2]))

/* objectGlobs' path-search step map (004dee04; legoobject_port.cpp). */
extern uint32_t* s_terrain_path_depth;

/* GAME-002 AITask.c (aitask.c). */
void aitask_object_attached_record_allocate(struct EntitySlot* object); /* 004033c0 */
void aitask_ai_idle_chain_record_attach(struct EntitySlot* object);   /* 00403f30 */
/* GAME-004 Creature.c (view.hpp, animation_object.c): Creature_GetCarryNull. */
void* renderer_cached_handle_48_get_or_create(void* object);          /* 00406fe0 */
/* GAME-006 Building.c (view.hpp, building_model.cpp). */
void* scene_object_indexed_handle_get_or_create(void* record, int index); /* 00408770 */
void* scene_object_secondary_handle_get_or_create(void* record);     /* 004087e0 */
/* LegoObject.c part 3 (entity.hpp, legoobject_port.cpp). */
int legoobject_nearest_connected_cell_route_select(Lego_Object* obj, unsigned int bx,
    unsigned int by, int diagonals, unsigned int* outIndex, int strict); /* 004469c0 */
/* SFX.c (sound.hpp, sfx.cpp). */
int sound_alias_lookup(const char* name, int* out_index);            /* 00464c70 */
int sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                   int loop, int positional,
                                   const float* xyz);                /* 00465050 */
double sound_object_duration_get(int alias);                         /* 00465160 */
/* GAME-043 Stats.c (entity.hpp, entity_stats.cpp). */
uint32_t stats_instance_word_at_f0(const void* config_block);        /* 00469be0 */
/* GAME-047 Vehicle.c (resource.hpp, vehicle_model.cpp): Vehicle_GetCarryNull. */
void* attached_entity_slot_object_get(struct SceneResourceAnimationRecord* record,
                                      int index);                    /* 0046db20 */
/* G98 Container.c (scene_resources.hpp, container_unit.cpp): Container_Texture_SetSurface?. */
void animated_texture_frame_set(void* texture, uint32_t frame_index,
                                const void* frame_state);            /* 00474da0 */

/* ---- group L (part 2, issue #588) ---- */

struct AnimationObject;
struct SceneResourceAnimationRecord;
struct TerrainMap;
struct TerrainMapPoint2;

/* objectGlobs' path-search cost map count (004dee08; legoobject_port.cpp). The
 * map itself, 004dee00, is s_terrain_neighbor_path_cost (group A). */
extern uint32_t s_terrain_path_depth_count;
/* legoGlobs.flags2 (00556ed0): engine.cpp's flag word. */
extern uint32_t s_panel_highlight_flags;

/* GAME-002 AITask.c (aitask.c; no port alias). */
void aitask_entity_task_state_update_after_cleanup(struct EntitySlot* object,
                                                   int remove);      /* 00403af0 */
void aitask_entity_task_queue_flush(struct EntitySlot* object);      /* 004040a0 */
/* GAME-004 Creature.c (animation_object.c): Creature_SetOrientation. */
void animation_object_transform_set(struct AnimationObject* object, float x,
                                    float y);                        /* 00406dc0 */
/* GAME-006 Building.c (building_model.cpp): Building_SetOrientation. */
void scene_object_pair_set_render_parameters(void* pair, float x, float y); /* 004086c0 */
/* Effects (rock_fall_styles.cpp). */
void scene_marker_create_at_object_position(void* object);          /* 0040bde0 */
/* GAME-021 Lego.c (level.hpp; lego.cpp). */
void level_tile_mark_current_target(const int cell_xy[2], int set);  /* 00432e00 */
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);       /* 00433090 */
/* GAME-026 Map3D.c (terrain_map.h). */
int terrain_map_world_to_block_centre(struct TerrainMap* map, float x, float y,
                                      int* bx, int* by, float* centre); /* 0044f700 */
int terrain_map_world_to_block_offset(struct TerrainMap* map,
                                      const struct TerrainMapPoint2* pos,
                                      struct TerrainMapPoint2* block,
                                      struct TerrainMapPoint2* offset); /* 0044f870 */
float terrain_map_world_z_get(struct TerrainMap* map, float x, float y); /* 0044f940 */
int cell_neighbor_direction_code(int x1, int y1, int x2, int y2);    /* 004508a0 */
/* GAME-043 Stats.c (entity.hpp). */
float entity_stats_float_at74(const struct EntitySlot* e);          /* 00469d50 */
/* GAME-047 Vehicle.c (resource.hpp, vehicle_model.cpp): Vehicle_SetOrientation. */
void scene_resource_draw_at_origin(struct SceneResourceAnimationRecord* record,
                                   float x, float y, float z);       /* 0046d350 */
/* Weapons.c (weapon.hpp, weapons.cpp, effects.hpp). */
uint32_t object_type_name_index_find(const char* name);             /* 0046f0d0 */
float weapon_type_recharge_time_get_target(int type);                /* 0046f110 */
int entity_spawn_record_slot_init(void* owner, const float position[3],
                                  const float direction[3],
                                  uint32_t auxiliary, uint32_t mode); /* 0046faf0 */
int object_flight_curve_record_init(void* owner, const float start[3],
                                    const float end[3], uint32_t auxiliary,
                                    uint32_t mode);                  /* 0046fc70 */
/* G98 Container.c (view.hpp): Container_SetOrientation. */
int ddiscene_object_pose_method_dispatch(void* object, void* alternate_frame,
                                         float parameter_3, float parameter_4,
                                         float parameter_5, float parameter_6,
                                         float parameter_7, float parameter_8); /* 00475470 */
/* G98 Maths.c (maths.h). */
float* d3drm_vector_rotate(float* r, float* v, float* axis, float theta); /* 00479510 */

/* ---- group M (part 2, issue #588) ---- */
/* GAME-002 AITask.c (aitask.c). */
void aitask_entity_pending_task_flag_clear(struct EntitySlot* object);      /* 00403b40 */
/* 0040c5b0 (resource.hpp, rock_fall_styles.cpp): the original returns the
 * container it creates for the block; the unit calls the port's entry
 * through a cast. */
int scene_resource_slot_register_name(uint32_t handle_key,
                                      uint32_t secondary_key);               /* 0040c5b0 */
/* GAME-021 Lego.c (level.hpp, lego.cpp). */
uint32_t terrain_cell_flag_800_bounded_current(uint32_t cx, uint32_t cy);    /* 00432b00 */
/* GAME-021 Lego.c (lego_unit.h; level.hpp, lego.cpp). */
uint32_t lego_runtime_flag40000000_get(void);                                /* 0041f970 */
int terrain_cell_type_is_8_or_10_or_11_current(int cx, int cy);              /* 00432b90 */
uint32_t terrain_cell_flag_2000_current(int cx, int cy);                     /* 00432fd0 */
/* GAME-026 Map3D.c (terrain_map.h). */
int terrain_map_world_edge_point_get(struct TerrainMap* map, float x, float y,
                                     int diagonal, float distance,
                                     float* xOut, float* yOut);              /* 0044fb90 */
/* G98 Container.c (container_unit.cpp): Container_AddRotation and
 * Container_AddTranslation. */
void render_object_rotate_axis(void* object, uint32_t mode, float x,
                               float y, float z, float angle);               /* 00475580 */
void scene_object_camera_teleport_or_offset(void* object, uint32_t mode,
                                            float x, float y, float z);      /* 004755e0 */
/* LegoObject.c part 3 (entity.hpp, legoobject_port.cpp): the original returns
 * the object hit and takes the position; the unit calls the port's entry
 * through a cast. */
Lego_Object* legoobject_flag_gated_callback_dispatch(Lego_Object* obj, float* pos,
                                                   float minRadius, int skipFlag8); /* 00445d70 */
/* 00446610: the port's entry is in a namespace; PORT-ONLY adapter at the
 * end of legoobject_port.cpp. */
void legoobject_movement_tick_scale_and_delta_get(Lego_Object* obj, float elapsed,
                                                  float* outTransSpeed,
                                                  float* outElapsed);        /* 00446610 */
/* GAME-047 Vehicle.c (record.hpp, vehicle_model.cpp): Vehicle_GetActivityContainer. */
uint32_t scene_object_resource_get(const void* scene_object);                /* 0046d9e0 */
/* GAME-004 Creature.c (record.hpp, animation_object.h): Creature_GetActivityContainer. */
void* animation_object_handle_get(const void* object);                     /* 00406e30 */
struct AnimationObject;
struct TerrainMap;
/* The world-Z callbacks (00442540, 00442560) as the setters' parameter
 * types: Creature.c's (void* data) and Building.c's probe (void). */
typedef float (*LegoObjM_HeightFn)(float x, float y, void* data);
typedef void (*LegoObjM_ProbeFn)(float x, float y, void* arg);
/* GAME-004 Creature.c (animation_object.h): Creature_SetPosition. */
void render_node_position_from_callback(struct AnimationObject* object, float x,
                                        float y, LegoObjM_HeightFn heightFn,
                                        void* data);                       /* 00406df0 */
/* GAME-006 Building.c (record.hpp, building_model.cpp): Building_SetPosition. */
void scene_object_pair_set_callback_scalar(void* pair, float x, float y,
                                           LegoObjM_ProbeFn probe, void* arg); /* 00408710 */
/* GAME-048 Vehicle.c (vehicle_model.h): Vehicle_SetPosition. */
struct VehicleModel;
void vehicle_set_position(struct VehicleModel* vehicle, float xPos, float yPos,
    float (*zCallback)(float xPos, float yPos, struct TerrainMap* map),
    struct TerrainMap* surfMap);                                             /* 0046d380 */
struct SceneResourceAnimationRecord;
/* GAME-047 Vehicle.c (resource.hpp, vehicle_model.cpp): the 0xc0 null. */
void* scene_resource_cached_slot_c0_get(struct SceneResourceAnimationRecord* record); /* 0046daf0 */
/* GAME-021 Lego.c (level.hpp, lego.cpp). */
int terrain_cell_mask_180000_current(const int cell_xy[2]);                /* 004330e0 */

/* ---- group N (part 2, issue #588) ---- */

struct BezierCurve;
/* 00403d10 and 00403f30 (aitask.c) are group I's and group K's
 * declarations above. */
/* GAME-003 (bezier.c, bezier.h): BezierCurve_Interpolate. */
unsigned int path_arc_length_interpolate(const struct BezierCurve* curve,
                                         float distance, Point2F* r);       /* 004068c0 */
/* Bubble.c (bubbles.cpp, cmd_shared.hpp). */
void idle_register(struct EntitySlot* e);                                  /* 00407450 */
/* GAME-021 Lego.c (lego_unit.c, lego_unit.h). */
extern float legoGlobs_MiniFigureRunAway;  /* lego_unit.c, 00556518 */
int lego_entity_matches_current_selection_or_owner(const void* object);    /* 004295c0 */
/* LegoObject.c part 3 (entity.hpp, legoobject_port.cpp). 00445d70 returns the
 * object it collides with; the port's entry returns it as an int. */
Lego_Object* legoobject_flag_gated_callback_dispatch(Lego_Object* obj, float* pos,
                                                   float minRadius, int skipFlag8); /* 00445d70 */
/* GAME-043 Stats.c (entity.hpp, entity_stats.cpp). */
float entity_stats_float_at8c(const struct EntitySlot* e);                 /* 00469fe0 */
float entity_stats_float_at90(const struct EntitySlot* e);                 /* 0046a000 */
/* PORT-ONLY adapters in legoobject_port.cpp (group N): the port's 00446610 and
 * 004477d0 are namespaced ledger stubs. */
void legoobject_movement_tick_scale_and_delta_get(Lego_Object* obj, float elapsed,
                                                  float* outTransSpeed,
                                                  float* outElapsed);       /* 00446610 */
void legoobject_path_step_lookahead_write(Lego_Object* obj); /* 004477d0 */

/* ---- group O (part 2, issue #588) ---- */

/* GAME-003 BezierCurve.c (bezier.h, C linkage): the route curve. */
#include "bezier.h"
/* legoGlobs.DynamiteWakeRadius? (00556464; lego_unit.c). */
extern float legoGlobs_DynamiteDamageRadius;
/* GAME-002 AITask.c (aitask.h). */
void aitask_entity_task_batch_append(struct EntitySlot** objects,
                                     unsigned int count,
                                     struct EntitySlot* target);    /* 00403650 */
/* GAME-021 Lego.c (level.hpp, lego.cpp): Level_Block_* queries. */
uint32_t terrain_cell_flag_2000_current(int cx, int cy);           /* 00432fd0 */
int terrain_cell_type_is_8_or_10_or_11_current(int cx, int cy);    /* 00432b90 */
uint32_t terrain_cell_flag_80_current(int cx, int cy);             /* 00433000 */
/* 004332c0 returns the block's byte 3; the port declares a uint32_t
 * (level.hpp). The unit reads the byte through a cast, as the original
 * uses only al. */
uint32_t terrain_cell_record_byte3_current(int x, int y);          /* 004332c0 */
#define legoobject_o_block_byte3(x, y) \
    (((uint8_t (*)(int, int))terrain_cell_record_byte3_current)(x, y))
/* Map3D.c (terrain_map.h). */
int terrain_map_world_edge_point_get(struct TerrainMap* map, float x, float y,
                                     int diagonal, float distance,
                                     float* xOut, float* yOut);    /* 0044fb90 */
/* GAME-043 Stats.c (entity.hpp). */
float entity_stats_float_at9c(const struct EntitySlot* e);         /* 00469d00 */
float entity_stats_float_at70(const struct EntitySlot* e);         /* 00469d90 */
/* Weapons.c (weapon.hpp): Weapon_GetDamageForObject?. */
float entity_indexed_float_lookup_with_minus_one_fallback(
    int weapon_type, const void* entity_record);                   /* 0046f1d0 */
/* G98 Container.c (view.hpp): Container_GetAnimationTime and
 * Container_GetAnimationFrames. */
float ddiscene_object_length_get(void* scene_object);              /* 00475390 */
uint32_t ddiscene_object_frame_position_get(void* scene_object);   /* 004753f0 */
/* LIBC __finite (0048e870; vc5names.h maps it for other compilers). */
int _finite(double x);
/* PORT-ONLY adapter in legoobject_port.cpp (group O): 0044b250 has C++
 * linkage. */
void legoobject_pairing_activity_raise_and_mutual_link(Lego_Object* first,
                                                       Lego_Object* second); /* 0044b250 */

/* Maths.h's inline Vector3D and Vector2D helpers, as group O's functions
 * expand them (static __inline; /Ob1 expands them in place). */
static __inline float legoobject_o_vector3_dot(LegoObjJ_Vector3F* a, LegoObjJ_Vector3F* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static __inline float legoobject_o_vector3_modulus(LegoObjJ_Vector3F* v)
{
    return (float)sqrt(legoobject_o_vector3_dot(v, v));
}

static __inline LegoObjJ_Vector3F* legoobject_o_vector3_scale(LegoObjJ_Vector3F* r,
                                                             LegoObjJ_Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline LegoObjJ_Vector3F* legoobject_o_vector3_normalise(LegoObjJ_Vector3F* r)
{
    return legoobject_o_vector3_scale(r, r, 1.0f / legoobject_o_vector3_modulus(r));
}

static __inline LegoObjJ_Vector3F* legoobject_o_vector3_cross(LegoObjJ_Vector3F* r,
                                                             LegoObjJ_Vector3F* a,
                                                             LegoObjJ_Vector3F* b)
{
    float x;
    float y;
    float z;

    x = a->y * b->z - a->z * b->y;
    y = a->z * b->x - a->x * b->z;
    z = a->x * b->y - a->y * b->x;
    r->x = x;
    r->y = y;
    r->z = z;
    return r;
}

static __inline float legoobject_o_vector2_modulus(Point2F* v)
{
    return (float)sqrt(v->x * v->x + v->y * v->y);
}

static __inline Point2F* legoobject_o_vector2_scale(Point2F* r, Point2F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    return r;
}

static __inline Point2F* legoobject_o_vector2_add(Point2F* r, Point2F* a, Point2F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    return r;
}

static __inline Point2F* legoobject_o_vector2_subtract(Point2F* r, Point2F* a, Point2F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    return r;
}

static __inline Point2F* legoobject_o_vector2_copy(Point2F* r, Point2F* a)
{
    r->x = a->x;
    r->y = a->y;
    return r;
}

static __inline Point2F* legoobject_o_vector2_normalise(Point2F* r)
{
    return legoobject_o_vector2_scale(r, r, 1.0f / legoobject_o_vector2_modulus(r));
}

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H */
