// SPDX-License-Identifier: LGPL-3.0-or-later
/* AITask.c (GAME-002) imports: C declarations of the callees and globals the
 * unit uses from other modules (ADR 0005 step 6; finding 570). Included only
 * by aitask.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration must name a definition with C linkage. */
#ifndef LEGORRR_GAME_AITASK_IMPORTS_H
#define LEGORRR_GAME_AITASK_IMPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;

/* Matching build only: three GAME-023 queries that legoobject_unit.c (#581)
 * now annotates under other names, and that legoobject_port_aliases.h does
 * not alias (their port entries are namespaced ledger stubs). Each macro
 * renames one declaration and its calls one for one (finding 538). */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define ledger_stub_00438a60_SpatialObjectQueryCenterMask01 legoobject_query_center_mask01
#define ledger_stub_00438ae0_SpatialObjectQueryCenterMask02 legoobject_query_center_mask02
#define ledger_stub_00438bf0_SpatialObjectQueryCenterMask80 legoobject_query_center_mask80
#endif

/* --- group A (00401cb0-00402d30) --- */

struct AITask;
struct LevelState;
struct TerrainMap;

/* GAME-021 Lego.c (level.hpp; lego.cpp). */
struct LevelState* level_current_owner_get(void);           /* 00431560 */
/* 00431570 Lego_GetMap takes no arguments. Its definition (lego_unit.c,
 * GAME-021 part 2) keeps the port's two ignored cell arguments, so the C
 * matching build calls it through an unprototyped function type, which
 * pushes none, as the original does; the native C++ compile goes through
 * lego.cpp's adapter. lego_unit.h declares level_current_terrain_get. */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define AITASK_LEGO_GET_MAP() \
    ((struct TerrainMap*)level_current_terrain_get_original())
#else
#define AITASK_LEGO_GET_MAP() \
    ((struct TerrainMap*)((void* (*)())level_current_terrain_get)())
#endif
int terrain_cell_walkable_flag_current(unsigned int cx,
                                       unsigned int cy);     /* 00432bd0 */
int terrain_cell_speed_class_current(const int cell_xy[2]);  /* 00432c50 */
void level_tile_mark_current_target(const int cell_xy[2], int set); /* 00432e00 */
int terrain_cell_state_equals_one_current(const int cell_xy[2]); /* 00432ec0 */
int terrain_cell_state_equals_six_current(const int cell_xy[2]); /* 00432f00 */
int terrain_cell_mask18_clear_current(unsigned int cx,
                                      unsigned int cy);      /* 00432f40 */
unsigned int level_cell_flag_4_current(unsigned int cx,
                                       unsigned int cy);     /* 00432f90 */
unsigned int level_cell_occupied_at_current(const int cell_xy[2]); /* 00433090 */
/* GAME-008 Dependencies.c (level.hpp). */
int ensure_cell_entries_loaded(unsigned int kind, int row, int slot); /* 0040aea0 */
/* GAME-022 LegoObject.c (entity.hpp). */
int entity_pool_find_by_callback_filtered_current(
    int (*callback)(struct EntitySlot*, void*), void* payload); /* 004377b0 */
int entity_flag40000_threshold_compare(const struct EntitySlot* e); /* 00437cc0 */
int entity_flags_contain_requested_single_bit(const struct EntitySlot* e,
                                              int selector);  /* 00438ed0 */
void entity_attached_object_list_detach(struct EntitySlot* entity,
                                        int reset_mode);      /* 00439e70 */
void entity_action_sequence_reset(struct EntitySlot* entity,
                                  int preserve_mode);         /* 00441940 */
/* Map3D (terrain_map.h). */
int terrain_map_block_highlight_set(struct TerrainMap* map, int bx, int by,
                                    int highlight);           /* 0044f4e0 */
void terrain_map_block_highlight_clear(struct TerrainMap* map, int bx,
                                       int by);               /* 0044f570 */
/* GAME-024 Message.c (cmd.hpp, select.hpp). */
int selection_snapshot_create(void** array_out, unsigned* count_out); /* 00452520 */
int selection_active_has_any_get(void);                       /* 004525b0 */
/* Stats.c (entity.hpp). */
unsigned int entity_stats_word_get(const struct EntitySlot* e);      /* 00469e60 */
unsigned int entity_capability_flags_get(const struct EntitySlot* e); /* 00469ea0 */


/* --- end group A --- */

/* --- group B (00402d50-00403700) --- */

/* GAME-021 Lego.c (level.hpp; lego.cpp). */
unsigned int terrain_cell_flag_8_bounded_current(unsigned int cx,
                                                 unsigned int cy); /* 00432b50 */
unsigned int terrain_cell_flag_20_bounded_current(unsigned int cx,
                                                  unsigned int cy); /* 00432d90 */
unsigned int terrain_cell_flag_reserved_current(const int cell_xy[2]); /* 00432dd0 */
void terrain_cell_secondary_flag_8_set_current(int cx, int cy, int set); /* 004324d0 */
/* terrain_grid.cpp (terrain_grid.hpp): legoGlobs.currLevel. */
extern struct LevelState* g_active_level_retail;            /* 005560d4 */
/* Bubbles.c (cmd_shared.hpp; bubbles.cpp). */
void idle_register(struct EntitySlot* e);                   /* 00407450 */
/* GAME-022 LegoObject.c (entity.hpp; legoobject_port.cpp). */
void* ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(
    const struct EntitySlot* entity);                       /* 00438bf0 */
struct EntitySlot* entity_lookup_at_world_position(
    const float world_position[2]);                         /* 00438df0 */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y);   /* 00442260 */
int ai_activity_scan(const struct EntitySlot* e, int activity); /* 0044b480 */
/* GAME-024 Message.c (select.hpp; message_unit.cpp). */
struct EntitySlot* const* selection_active_records_get(void); /* 004525e0 */
unsigned int selection_active_count_get(void);              /* 004525f0 */
/* Stats.c (entity.hpp; entity_stats.cpp). */
int entity_grouped_code_table_find_value_index(unsigned int value,
                                               int* out_group); /* 0046a120 */


/* --- end group B --- */

/* --- group F (00403750-00403f80) --- */

/* GAME-022 LegoObject.c (entity.hpp, port_loop_ports.hpp; legoobject_port.cpp). */
void* ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
    const int* cell_xy, unsigned int filter_a, unsigned int filter_b,
    unsigned int filter_c);                                          /* 00438a60 */
void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const struct EntitySlot* entity, const int* cell_xy,
    unsigned int filter_a, unsigned int filter_b);                   /* 00438ae0 */
struct EntitySlot* entity_action_kind_entity_create_and_command_issue(
    struct EntitySlot* source, int kind, int selector, int argument); /* 0043a650 */
void entity_linked_activity_event_dispatch(struct EntitySlot* entity,
                                           unsigned int events);     /* 0043f5b0 */
/* Stats.c (entity.hpp). */
unsigned int stats_instance_word_at_ec(const void* config_block);    /* 00469bb0 */
/* GAME-021 Lego.c (level.hpp; lego.cpp). */
int terrain_cell_state_update_current(const int* cell_xy);           /* 00432c90 */
void terrain_cell_flag_80000_clear_or_set_current(const int* cell_xy,
                                                  int set);          /* 00432710 */
void terrain_cell_flag_10000000_set_current(const int* cell_xy, int set); /* 00433120 */
/* Path/opgroup (level.hpp, cmd_shared.hpp; placement.cpp). */
int terrain_descriptor_coordinate_count_limit_check(
    unsigned int descriptor_key, int type, int row);                 /* 00408d70 */
void cmd_operation_group_append_object(unsigned int key,
                                       struct EntitySlot* entity);   /* 00408ce0 */
/* Message.c (cmd.hpp) and the advisor (frontend.hpp). */
void cmd_produce(int op, const void* a1, int a2, const int* pair);   /* 00452060 */
void advisor_text_animation_update(int slot, int force, int selected); /* 0046ac60 */
/* Maths.c (maths.h). */
float random_float_range(float low, float high);                     /* 004798b0 */
/* --- end group F --- */

/* --- group C (00404030-00404f50) --- */
struct TerrainMap;
struct TerrainMapVector3;
/* LegoObject.c (GAME-023; entity.hpp, ai.hpp: C linkage). */
int entity_gated_step_command_issue(struct EntitySlot* entity,
                                    struct EntitySlot* target);      /* 00439a20 */
int entity_deposit_activity_gate(struct EntitySlot* entity,
                                 struct EntitySlot* target);         /* 0043a300 */
int entity_flag400_and_health_threshold_check(
    const struct EntitySlot* entity);                                /* 004399c0 */
int entity_route_to_explicit_target(struct EntitySlot* actor,
                                    struct EntitySlot* target);      /* 0043fe70 */
int entity_route_marker_setup(struct EntitySlot* actor);             /* 0043fff0 */
int entity_face_target_action_start(struct EntitySlot* entity,
                                    struct EntitySlot* target,
                                    int mode);                       /* 00441ed0 */
int entity_random_activity_attempt(struct EntitySlot* e);            /* 00443670 */
int entity_set_activity_place(struct EntitySlot* e);                 /* 00448c50 */
/* Bubbles.c (entity.hpp). */
void entity_reset_auxiliary_state(struct EntitySlot* e);             /* 00407370 */
/* Lego.c: 004403d0's adapter at the end of lego.cpp (GAME-021). */
int level_debug_cell_retarget_adapter(struct EntitySlot* entity,
                                      struct EntitySlot* target);    /* 004403d0 */
/* NERPs (tutorial_script.cpp adapter, GAME-021). */
unsigned int system_state_flags_get_original(const int* parameters); /* 00456240 */
/* Map3D.c (terrain_map.h). */
int terrain_map_block_vertices_get(struct TerrainMap* map, unsigned int bx,
                                   unsigned int by,
                                   struct TerrainMapVector3* positions); /* 004500d0 */
/* Maths.c (maths.h). */
float random_float_range(float low, float high);                     /* 004798b0 */
/* LegoObject.c entries whose port definitions are namespaced ledger stubs
 * (or take other arguments): original-ABI adapters at the end of
 * aitask_port.cpp. */
int aitask_cell_step_command_issue_adapter(struct EntitySlot* object,
                                           int* blockPos, void* wPos,
                                           int actionMode,
                                           int markPending);         /* 0043a120 */
int aitask_route_setup_adapter(struct EntitySlot* object, unsigned int count,
                               int* routeX, int* routeY, float* wPos); /* 00441700 */
int aitask_adjacent_position_find_adapter(struct EntitySlot* object,
                                          int* blockPos, int* outPos,
                                          int mode);                 /* 00443710 */
int aitask_nearest_cell_route_commit_adapter(struct EntitySlot* object,
                                             unsigned int x, unsigned int y,
                                             int mode);              /* 00446e40 */
int aitask_type_five_spawn_adapter(struct EntitySlot* object);       /* 004470d0 */
int aitask_move_to_cell_adapter(struct EntitySlot* object, int* blockPos); /* 00448800 */
int aitask_gated_step_record_adapter(struct EntitySlot* object,
                                     int* blockPos);                 /* 00448880 */
int aitask_neighbor_cell_move_adapter(struct EntitySlot* object,
                                      int* blockPos);                /* 00448a60 */
int aitask_cardinal_step_link_adapter(struct EntitySlot* object,
                                      struct EntitySlot* target,
                                      int linkMode);                 /* 00448c90 */
int aitask_proximity_step_jitter_adapter(struct EntitySlot* object,
                                         struct EntitySlot* target,
                                         int marker);                /* 00448eb0 */
int aitask_kind6_approach_route_adapter(struct EntitySlot* object);  /* 00449010 */
int aitask_proximity_step_link_adapter(struct EntitySlot* object,
                                       struct EntitySlot* target, int mode,
                                       int setGlobalGate);           /* 004490a0 */
int aitask_walkable_cell_route_adapter(struct EntitySlot* object,
                                       int* blockPos);               /* 00449240 */
int aitask_cell_step_mark_adapter(struct EntitySlot* object,
                                  int* blockPos);                    /* 004492b0 */
int aitask_target_cell_route_associate_adapter(struct EntitySlot* object,
                                               struct EntitySlot* target); /* 00449430 */
int aitask_target_cell_route_marker14_adapter(struct EntitySlot* object,
                                              struct EntitySlot* target); /* 00449520 */
int aitask_coordinate_route_mark_adapter(struct EntitySlot* object,
                                         int* blockPos);             /* 00449610 */
int aitask_route_step_adapter(struct EntitySlot* object);            /* 00449700 */
int aitask_proximity_step_clearing_adapter(struct EntitySlot* object,
                                           struct EntitySlot* target); /* 00449ac0 */
int aitask_type11_nearest_resolve_adapter(struct EntitySlot* object,
                                          int* outPos);              /* 0044a3d0 */
int aitask_position_step_link_adapter(struct EntitySlot* object,
                                      void* selector);               /* 0044b500 */
/* --- end group C --- */

/* --- group D (00404fc0-00405950) --- */
struct TerrainMap;
/* LegoObject.c (GAME-023, entity.hpp, ai.hpp, legoobject.h). */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y);           /* 00442260 */
int entity_shared_eligibility_check(const struct EntitySlot* e,
                                    int relaxed);                    /* 004385b0 */
int ai_can_do(const struct EntitySlot* e);                           /* 0044c530 */
int scene_object_has_task_type_4_5_7(const struct EntitySlot* e);    /* 004493e0 */
int entity_accepts_target_for_task_check(const struct EntitySlot* entity,
                                         const struct EntitySlot* target,
                                         int apply_extra_gate);      /* 00439bd0 */
int entity_task_target_cell_eligibility_check(
    const struct EntitySlot* entity, const struct EntitySlot* target); /* 00439b80 */
int entity_record_flag20_query(const struct EntitySlot* entity,
                               int out_xy[2]);                       /* 00439e40 */
struct EntitySlot* entity_lookup_at_world_position(
    const float world_position[2]);                                  /* 00438df0 */
int entity_type3_flag04000000_test(const struct EntitySlot* e);      /* 004436f0 */
int grid_record_count_difference_sum(int type, int skip_flagged_rows); /* 00437100 */
int entity_capacity_or_state_predicate(const struct EntitySlot* entity); /* 0043c650 */
int ai_activity_scan(const struct EntitySlot* e, int activity);      /* 0044b480 */
/* LegoObject_GetBlockPos (004424f0): the port's entry takes the level as
 * well; adapter at the end of lego.cpp. */
void entity_position_to_terrain_cell_adapter(const struct EntitySlot* entity,
                                             int* cell_x, int* cell_y);
/* 0043a5f0 (LegoObject_GetActivityContainer-style) and 0044b4c0: the port's
 * entries have other contracts or no C declaration; adapters at the end of
 * aitask_port.cpp. */
void* aitask_object_container_get_adapter(struct EntitySlot* object);
int aitask_object_command_gate_adapter(struct EntitySlot* object, int type);
/* 00432550 (Lego.c) takes the block position; the port's entry
 * (always_accept_place_command_cell) takes no argument. Adapter at the end
 * of aitask_port.cpp. */
int aitask_place_command_cell_accept_adapter(int* blockPos);
/* Stats.c (entity.hpp, ai.hpp). */
unsigned int entity_stats_word_get(const struct EntitySlot* e);      /* 00469e60 */
unsigned int entity_state_flags_get(const struct EntitySlot* e);     /* 00469e80 */
unsigned int entity_capability_flags_get(const struct EntitySlot* e); /* 00469ea0 */
unsigned int stats_instance_word_at_ec(const void* config_block);    /* 00469bb0 */
float entity_stats_float_ate8(const struct EntitySlot* e);           /* 00469d70 */
float entity_stats_float_at98(const struct EntitySlot* e);           /* 0046a020 */
int ai_activity_cap(const struct EntitySlot* e);                     /* 0046a060 */
/* Weapons.c (entity.hpp). */
int see_through_walls_ray_check(void* first, void* second);          /* 00471960 */
int entity_centers_within_object_radius(const struct EntitySlot* first,
                                        const struct EntitySlot* second); /* 00472130 */
/* Lego.c (GAME-021: level.hpp, lego_unit.h, interface_callees.h). */
int terrain_cell_walkability_query_current(const struct EntitySlot* entity,
                                           unsigned int x, unsigned int y,
                                           int require_stat_bit,
                                           int gated_mode);          /* 00432770 */
float engine_float_field260_get_current_target(void);                /* 00429880 */
extern float legoGlobs_MinEnergyForEat;                              /* 00556458 */
extern unsigned int s_panel_highlight_flags;                         /* 00556ed0 */
/* Map3D.c (terrain_map.h). */
int terrain_map_block_to_world(struct TerrainMap* map, unsigned int bx,
                               unsigned int by, float* xOut,
                               float* yOut);                         /* 0044f640 */
int terrain_map_world_to_block(struct TerrainMap* map, float x, float y,
                               int* bx, int* by);                    /* 0044f6d0 */
/* Container.c (view.hpp) and the building model (view.hpp). */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);                 /* 004754c0 */
void* scene_object_secondary_handle_get_or_create(void* record);     /* 004087e0 */

/* --- end group D --- */

/* --- group E (00405c10-004065e0) --- */
/* Main.c's millisecond clock (frameloop.h), a tail call to WINMM's
 * timeGetTime. The original AITask.c calls it, not the import (#589). */
unsigned long main_time_get(void);                                   /* 00478000 */
/* Bubbles.c (cmd_shared.hpp; bubbles.cpp). */
void idle_register(struct EntitySlot* object);                       /* 00407450 */
/* DirectDraw globals (crt.hpp, crt.h; crt.cpp). */
extern unsigned int s_renderer_global_dword;                         /* 0076ac84 */
extern unsigned int s_render_surface_secondary;                      /* 0076ac8c */
/* Message.c (select.hpp, cmd.hpp, message.h). */
unsigned int selection_contains_entity_active_target(struct EntitySlot* entity,
                                                     unsigned int* index_out); /* 00452610 */
int cmd_array_find_value_index(const int* array, unsigned count, int value,
                               unsigned* index_out);                 /* 00452650 */
void cmd_produce(int op, const void* a1, int a2, const int* pair);   /* 00452060 */
/* Lego.c (level.hpp; lego.cpp). */
/* 00431c70: lego_first_free_neighbor_cell_pick (lego_unit.h). */
/* LegoObject.c (legoobject.h). */
void entity_surface_vector_normalize(void* surface, float out_xy[2]); /* 004422a0 */
/* 00440c30 (route search) and 00438e50 (nearest object by type mask): the
 * port's entries are namespaced ledger stubs in legoobject_port.cpp. Adapters at
 * the end of aitask_port.cpp. */
int aitask_grid_route_search_adapter(struct EntitySlot* object,
                                     unsigned int startX, unsigned int startY,
                                     unsigned int goalX, unsigned int goalY,
                                     int** outX, int** outY,
                                     unsigned int* outCount);
struct EntitySlot* aitask_object_find_trainer_adapter(struct EntitySlot* object,
                                                      const float* center,
                                                      unsigned int mask);
/* --- end group E --- */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AITASK_IMPORTS_H */
