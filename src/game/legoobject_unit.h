// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 LegoObject.c (original translation unit, finding 505), part 1 of
 * 3 (issue #575): the original-shaped records the unit reads and the unit's
 * functions.
 *
 * C-compatible header (ADR 0005). legoobject_unit.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 538). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the shared functions.
 *
 * The offsets are the original's Win32 layout. The port's EntitySlot
 * (game.hpp) is another layout; the unit's functions that read Lego_Object
 * run only in the matching build. */
#ifndef LEGORRR_GAME_LEGOOBJECT_UNIT_H
#define LEGORRR_GAME_LEGOOBJECT_UNIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reads a field of an original record by its Win32 offset, for records and
 * fields the unit has no struct member for yet. */
#define LEGOOBJ_FIELD(ptr, type, offset) (*(type*)((char*)(ptr) + (offset)))

/* LegoObject (0x40c bytes): the original's object record. The pool
 * (legoobject_pool_chunk_allocate) hands them out from chunks in strides of
 * 0x40c; +0x408 links free records. Named by role where the port or the
 * pseudocode knows it, otherwise by offset. Lego_Object, not LegoObject:
 * interface_callees.h (GAME-020) defines a partial LegoObject view, and the
 * boundary test includes both headers. */
typedef struct Lego_Object {
    int type;                         /* +0x000 LegoObject_Type */
    int id;                           /* +0x004 object index within the type */
    char* customName;                 /* +0x008 */
    void* vehicle;                    /* +0x00c model, by type */
    void* miniFigure;                 /* +0x010 */
    void* rockMonster;                /* +0x014 */
    void* building;                   /* +0x018 */
    void* other;                      /* +0x01c */
    void* upgradePart;                /* +0x020 */
    void* routeBlocks;                /* +0x024 route buffer */
    unsigned int routeBlocksTotal;    /* +0x028 */
    unsigned int routeBlocksCurrent;  /* +0x02c */
    unsigned int routeCurveCount;     /* +0x030 BezierCurve routeCurve: count */
    float routeCurvePoints[50][2];    /* +0x034 routeCurve.points */
    float routeCurveDistances[50];    /* +0x1c4 routeCurve.distances */
    float field_28c;                  /* +0x28c */
    float field_290;                  /* +0x290 */
    float field_294;                  /* +0x294 */
    float field_298;                  /* +0x298 last route direction x */
    float field_29c;                  /* +0x29c last route direction y */
    float field_2a0;                  /* +0x2a0 */
    float field_2a4;                  /* +0x2a4 */
    float field_2a8;                  /* +0x2a8 */
    float faceDirection[3];           /* +0x2ac */
    float faceDirectionLength;        /* +0x2b8 */
    int field_2bc;                    /* +0x2bc */
    int field_2c0;                    /* +0x2c0 */
    float field_2c4;                  /* +0x2c4 */
    float field_2c8;                  /* +0x2c8 */
    float field_2cc;                  /* +0x2cc */
    unsigned int field_2d0;           /* +0x2d0 */
    float animTime;                   /* +0x2d4 */
    unsigned int animRepeat;          /* +0x2d8 */
    void* field_2dc;                  /* +0x2dc sink effect container */
    int field_2e0;                    /* +0x2e0 */
    void* activityCont;               /* +0x2e4 container the activity runs */
    const char* activityName1;        /* +0x2e8 activityName entries */
    const char* activityName2;        /* +0x2ec */
    void* aiTask;                     /* +0x2f0 command chain head */
    float targetBlockPos[2];          /* +0x2f4 */
    struct Lego_Object* routeToObject;  /* +0x2fc */
    struct Lego_Object* interactObject; /* +0x300 reciprocal of +0x2fc */
    struct Lego_Object* carryingThisObject; /* +0x304 */
    struct Lego_Object* carriedObjects[6];  /* +0x308 */
    unsigned int carryingIndex;       /* +0x320 */
    unsigned int numCarriedObjects;   /* +0x324 */
    unsigned int carryNullFrames;     /* +0x328 */
    int field_32c;                    /* +0x32c */
    unsigned int objLevel;            /* +0x330 */
    void* stats;                      /* +0x334 ObjectStats */
    float aiTimer;                    /* +0x338 */
    float field_33c;                  /* +0x33c */
    float health;                     /* +0x340 */
    float energy;                     /* +0x344 */
    void* field_348;                  /* +0x348 */
    int field_34c;                    /* +0x34c */
    int drillSoundHandle;             /* +0x350 */
    int soundHandle;                  /* +0x354 */
    unsigned char pad_358[0x360 - 0x358];
    float field_360;                  /* +0x360 */
    void* field_364;                  /* +0x364 */
    float field_368;                  /* +0x368 */
    struct Lego_Object* driveObject;  /* +0x36c */
    int carriedTools[5];              /* +0x370 */
    unsigned int numCarriedTools;     /* +0x384 */
    unsigned char pad_388[0x39c - 0x388];
    float weaponDirection[3];         /* +0x39c fire direction */
    float weaponTarget[3];            /* +0x3a8 fire target position */
    float field_3b4;                  /* +0x3b4 */
    float field_3b8;                  /* +0x3b8 */
    float field_3bc;                  /* +0x3bc */
    void* field_3c0;                  /* +0x3c0 */
    unsigned char pad_3c4[0x3cc - 0x3c4];
    struct Lego_Object* field_3cc;    /* +0x3cc */
    float field_3d0;                  /* +0x3d0 */
    float field_3d4;                  /* +0x3d4 */
    float field_3d8;                  /* +0x3d8 */
    float field_3dc;                  /* +0x3dc */
    unsigned int flags1;              /* +0x3e0 */
    unsigned int flags2;              /* +0x3e4 */
    unsigned int flags3;              /* +0x3e8 */
    unsigned int flags4;              /* +0x3ec */
    unsigned int abilityFlags;        /* +0x3f0 */
    int field_3f4;                    /* +0x3f4 */
    int field_3f8;                    /* +0x3f8 */
    float field_3fc;                  /* +0x3fc */
    float field_400;                  /* +0x400 */
    int field_404;                    /* +0x404 */
    struct Lego_Object* nextFree;     /* +0x408 */
} Lego_Object;

/* LegoObject_RunThroughLists callback. */
typedef int (*Lego_ObjectCallback)(Lego_Object* obj, void* data);

/* ------------------------------------------------------------------ */
/* The unit's functions, in original order. Each group section is      */
/* edited only by the group that owns it.                              */
/* ------------------------------------------------------------------ */

/* ---- group A: 00436c20-00437510 ---- */
void legoobject_initialise(void);                               /* 00436c20 */
void legoobject_shutdown(void);                                 /* 00437050 */
void legoobject_render_config_copy_out(unsigned int* out);      /* 004370b0 */
void legoobject_render_config_copy_in_or_clear(unsigned int* in); /* 004370d0 */
int legoobject_grid_record_count_difference_sum(int type, int skip); /* 00437100 */
void legoobject_tool_tip_icons_load(void* config);              /* 00437150 */
void legoobject_spawn_coordinate_pool_clear(void);              /* 004372a0 */
void legoobject_process_queued_grid_animations(void* anchor, int mode, float delta); /* 00437300 */
void legoobject_activity_toggle(Lego_Object* obj, int on);      /* 004373d0 */
void legoobject_grid_record_count_tables_clear(void);           /* 00437440 */
int legoobject_grid_record_count_difference_get(int row, int column, int slot, int subtract); /* 00437460 */
unsigned int legoobject_grid_record_count_get(int row, int column, int slot); /* 004374a0 */
void legoobject_task_counter_increment_by_branch(int nonzero);  /* 004374d0 */
void legoobject_callback_dispatch_unless_type_five(Lego_Object* obj); /* 004374f0 */
int legoobject_linked_route_clear_callback(Lego_Object* obj, void* data); /* 00437510 */

/* ---- group B: 00437540-00437cc0 ---- */
/* objectGlobs.recordObjs (004e8544): one deferred object of the level's
 * object list, 0x2c bytes, created when its block is uncovered. */
typedef struct LegoObjGB_DeferredRecord {
    int blockPos[2];                  /* +0x00 */
    float worldPos[2];                /* +0x08 */
    float heading;                    /* +0x10 */
    void* model;                      /* +0x14 */
    int type;                         /* +0x18 */
    int id;                           /* +0x1c */
    float health;                     /* +0x20 */
    char* name;                       /* +0x24 */
    char* linkName;                   /* +0x28 driving/route target's name */
} LegoObjGB_DeferredRecord;
int legoobject_remove(Lego_Object* obj);                        /* 00437540 */
int legoobject_run_through_lists_skip_upgrade_parts(Lego_ObjectCallback callback, void* data); /* 004377b0 */
int legoobject_run_through_lists(Lego_ObjectCallback callback, void* data, int skipUpgradeParts); /* 004377d0 */
void legoobject_rename(Lego_Object* obj, const char* name);     /* 00437880 */
void legoobject_grid_object_records_release(void);              /* 004378e0 */
void legoobject_deferred_olist_create_and_link(int* blockPos);  /* 00437940 */
void legoobject_deferred_olist_queue_add(void* model, int type, int id, float* worldPos, float heading, float health, const char* name, const char* linkName); /* 00437c20 */
int legoobject_flag40000_threshold_compare(Lego_Object* obj);   /* 00437cc0 */

/* ---- group C: 00437d00-00438580 ---- */
Lego_Object* legoobject_create(void* model, int type, int id);  /* 00437d00 */
Lego_Object* legoobject_pool_allocate(void);                    /* 004382c0 */
void legoobject_pool_chunk_allocate(void);                      /* 00438310 */
unsigned int legoobject_global_animation_state_get(void);       /* 00438390 */
void legoobject_global_animation_state_set(unsigned int state); /* 004383a0 */
void legoobject_rover_work_queue_update(Lego_Object* obj, int mode); /* 004383b0 */
void legoobject_kind_record_update_dispatch(Lego_Object* obj);  /* 00438460 */
void legoobject_flag80000000_set_or_clear(Lego_Object* obj, int on); /* 00438580 */

/* ---- group D: 004385b0-00438ed0 ---- */
/* The search record the LegoObject_RunThroughLists callback 00438fb0
 * scans with (nine dwords): the nearest match within bestDistance whose
 * kind is selected by mask, and the mask's extra arguments. */
typedef struct LegoObjGD_SearchData {
    Lego_Object* result;              /* +0x00 */
    float x;                          /* +0x04 search centre */
    float y;                          /* +0x08 */
    float bestDistance;               /* +0x0c starts at 10000.0f */
    unsigned int mask;                /* +0x10 */
    int arg14;                        /* +0x14 */
    int arg18;                        /* +0x18 */
    int arg1c;                        /* +0x1c */
    int arg20;                        /* +0x20 */
} LegoObjGD_SearchData;

/* The record of the block-position callback 004386b0. */
typedef struct LegoObjGD_BlockSearch {
    Lego_Object* result;              /* +0x00 */
    int bx;                           /* +0x04 */
    int by;                           /* +0x08 */
} LegoObjGD_BlockSearch;

int legoobject_shared_eligibility_check(Lego_Object* obj, int relaxed); /* 004385b0 */
Lego_Object* legoobject_create_in_world(void* model, int type, int id, int last, float x, float y, float heading); /* 00438610 */
Lego_Object* legoobject_at_grid_position_find(int* cell);       /* 00438670 */
int legoobject_grid_position_match_callback(Lego_Object* obj, void* data); /* 004386b0 */
int legoobject_available_resource_spend(Lego_Object* obj, int amount); /* 00438720 */
int legoobject_building_upgrade_resource_admission_check(Lego_Object* building, unsigned int* outCost); /* 00438770 */
void legoobject_building_upgrade_commit(Lego_Object* building); /* 004387f0 */
int legoobject_action_kind_prerequisite_check(Lego_Object* obj, int action); /* 004388b0 */
Lego_Object* legoobject_unknown_00438960(Lego_Object* obj, int alt); /* 00438960 */
Lego_Object* legoobject_route_marker_candidate_find(Lego_Object* source, int secondary); /* 004389e0 */
Lego_Object* legoobject_query_center_mask01(int* cell, int a, int b, int c); /* 00438a60 */
Lego_Object* legoobject_query_center_mask02(Lego_Object* obj, int* cell, int a, int b); /* 00438ae0 */
Lego_Object* legoobject_query_center_mask_from_argument(Lego_Object* obj, int mask); /* 00438b80 */
Lego_Object* legoobject_query_center_mask80(Lego_Object* obj); /* 00438bf0 */
Lego_Object* legoobject_candidate_search(Lego_Object* obj);     /* 00438c60 */
Lego_Object* legoobject_query_center_mask20(float* center);     /* 00438cd0 */
Lego_Object* legoobject_query_center_mask10(float* center);     /* 00438d30 */
Lego_Object* legoobject_query_center_mask200(float* center);    /* 00438d90 */
Lego_Object* legoobject_lookup_at_world_position(float* position); /* 00438df0 */
Lego_Object* legoobject_query_center_mask100(Lego_Object* obj, float* center, int extra); /* 00438e50 */
int legoobject_flags_contain_requested_single_bit(Lego_Object* obj, int selector); /* 00438ed0 */

/* ---- group E: 00438f60-00439930 ---- */
/* The search record the spatial queries (004389e0-00438e50) pass to
 * legoobject_route_marker_candidate_visit. */
typedef struct LegoObjGE_RouteMarkerSearch {
    Lego_Object* result;     /* +0x00 nearest match */
    float x;                 /* +0x04 query position */
    float y;                 /* +0x08 */
    float bestDistance;      /* +0x0c */
    unsigned int flags;      /* +0x10 search mask */
    int actionKind;          /* +0x14 legoobject_action_kind_supported */
    int actionArgument;      /* +0x18 */
    int actionExtra;         /* +0x1c */
    int selector;            /* +0x20 legoobject_flags_contain_requested_single_bit */
} LegoObjGE_RouteMarkerSearch;
/* Lego_Level and Lego_Block (0x48), the fields group E reads. */
typedef struct LegoObjGE_Block {
    unsigned char pad_00[0x2c];
    int field_2c;            /* +0x2c build-placement counter */
    int field_30;            /* +0x30 alternate build-placement counter */
    unsigned char pad_34[0x48 - 0x34];
} LegoObjGE_Block;
/* Point2F. */
typedef struct LegoObjGE_Point2F {
    float x;
    float y;
} LegoObjGE_Point2F;
int legoobject_dock_definition_matches_cell(Lego_Object* obj);  /* 00438f60 */
int legoobject_route_marker_candidate_visit(Lego_Object* obj, void* data); /* 00438fb0 */
int legoobject_task_kind_filtered_search(int selector);         /* 00439200 */
int legoobject_task_flags_and_cell_task_match(Lego_Object* obj, void* task); /* 00439240 */
void legoobject_build_placement_command_batch_expand(int* cell); /* 00439280 */
void legoobject_build_placement_command_pair_create(int* cell, int id); /* 00439340 */
void legoobject_build_placement_command_setup(int* cell, int id, LegoObjGE_Point2F* world, int update); /* 00439370 */
void legoobject_build_placement_alternate_command_setup(int* cell, int id, LegoObjGE_Point2F* world, int update); /* 004394b0 */
const char* legoobject_display_name_lookup(Lego_Object* obj);  /* 004395e0 */
const char* legoobject_item_name_by_kind(unsigned int kind, int index); /* 004396c0 */
const char* legoobject_the_name_get(unsigned int kind, int index); /* 00439790 */
const char* legoobject_name_get(unsigned int kind, int index);  /* 00439860 */
void legoobject_copy_two_dword_pair(void* record, unsigned int* outA, unsigned int* outB); /* 00439930 */

/* ---- group F: 00439950-00439e40 ---- */
typedef struct LegoObjF_Point2I { int x; int y; } LegoObjF_Point2I;
void legoobject_tool_name_strings_load(void* config, const char* gameName); /* 00439950 */
void legoobject_activity_scene_state_update(void);              /* 00439990 */
int legoobject_flag400_and_health_threshold_check(Lego_Object* obj); /* 004399c0 */
int legoobject_gated_step_command_issue(Lego_Object* obj, Lego_Object* target); /* 00439a20 */
int legoobject_task_target_cell_eligibility_check(Lego_Object* obj, Lego_Object* target); /* 00439b80 */
int legoobject_accepts_target_for_task_check(Lego_Object* obj, Lego_Object* target, int extra); /* 00439bd0 */
void legoobject_indexed_flag_set_and_refresh(Lego_Object* obj); /* 00439c80 */
void legoobject_animation_state_set(Lego_Object* obj, unsigned int state); /* 00439cd0 */
unsigned int legoobject_stats_flag10_test(Lego_Object* obj);    /* 00439cf0 */
unsigned int legoobject_stats_flag20_test(Lego_Object* obj);    /* 00439d10 */
unsigned int legoobject_stats_flag40_test(Lego_Object* obj);    /* 00439d30 */
int legoobject_vehicle_unload_adjacent_cell_find(Lego_Object* obj, int* outCell); /* 00439d50 */
int legoobject_vehicle_unload_cell_find_if_eligible(Lego_Object* obj, int* outCell); /* 00439e10 */
int legoobject_record_flag20_query(Lego_Object* obj, int* outCell); /* 00439e40 */

/* ---- group G: 00439e70-0043a960 ---- */
/* Lego_Block (0x48) and the Lego_Level fields group G reads. */
typedef struct LegoObjG_Block {
    unsigned char pad_00[2];
    unsigned char terrain;            /* +0x02 */
    unsigned char pad_03[0x48 - 0x03];
} LegoObjG_Block;
/* RoutingBlock (0x14 bytes): one entry of Lego_Object.routeBlocks. */
typedef struct LegoObjG_RouteBlock {
    int blockPos[2];                  /* +0x00 */
    float blockOffset[2];             /* +0x08 */
    unsigned char flags;              /* +0x10 */
    unsigned char actionFlags;        /* +0x11 */
    unsigned char pad_12[0x14 - 0x12];
} LegoObjG_RouteBlock;
typedef struct LegoObjG_Level {
    unsigned char pad_00[0x14];
    unsigned int width;               /* +0x14 */
    unsigned char pad_18[0xb8 - 0x18];
    LegoObjG_Block* blocks;           /* +0xb8 */
} LegoObjG_Level;
void legoobject_attached_object_list_detach(Lego_Object* obj, int resetMode); /* 00439e70 */
int legoobject_cell_step_command_issue_with_jitter(Lego_Object* obj, int* target, float* input, int mode, int markPending); /* 0043a120 */
int legoobject_deposit_activity_gate(Lego_Object* obj, Lego_Object* target); /* 0043a300 */
void legoobject_visual_resource_get_or_create(Lego_Object* obj); /* 0043a5f0 */
Lego_Object* legoobject_action_kind_create_and_command_issue(Lego_Object* source, int kind, int argument, int last); /* 0043a650 */
int legoobject_action_kind_supported(Lego_Object* obj, int kind, int argument, int last); /* 0043a7c0 */
void legoobject_action_record_consume_and_cleanup(Lego_Object* obj, Lego_Object* record); /* 0043a850 */
void legoobject_simulation_update_counter_add(int unused, unsigned int count); /* 0043a8f0 */
void legoobject_simulation_mode_counter_add(int unused, unsigned int count); /* 0043a910 */
void legoobject_tracked_pointer_remove(void* pointer);          /* 0043a930 */
void legoobject_lava_crust_activity_register(Lego_Object* obj); /* 0043a960 */

/* ---- group H: 0043a9e0-0043bbc0 (part 2, issue #588) ---- */
/* The record LegoObject_RunThroughLists hands 0043b3b0: the screen point
 * to pick at and where the hit goes. */
typedef struct LegoObjH_PickSearch {
    Lego_Object** result;             /* +0x00 */
    float x;                          /* +0x04 screen position */
    float y;                          /* +0x08 */
} LegoObjH_PickSearch;
/* The record of 0043b770: the viewport and the screen rectangle. */
typedef struct LegoObjH_RectSearch {
    void* viewport;                   /* +0x00 */
    float left;                       /* +0x04 */
    float top;                        /* +0x08 */
    float right;                      /* +0x0c */
    float bottom;                     /* +0x10 */
} LegoObjH_RectSearch;
/* The record of 0043b8d0: the listener, its position and the two ranges
 * that switch the objects' meshes. */
typedef struct LegoObjH_LodSearch {
    Lego_Object* obj;                 /* +0x00 the listener (skipped) */
    int usePosition;                  /* +0x04 */
    float mediumDist;                 /* +0x08 */
    float highDist;                   /* +0x0c */
    float position[3];                /* +0x10 */
} LegoObjH_LodSearch;
/* Map3D's block grid size (its first two words). */
typedef struct LegoObjH_Map {
    int blockWidth;                   /* +0x00 */
    int blockHeight;                  /* +0x04 */
} LegoObjH_Map;
/* legoGlobs.rockMonsterData: the creature models (0x74 bytes each). */
typedef struct LegoObjH_CreatureModel {
    unsigned char bytes[0x74];
} LegoObjH_CreatureModel;
/* legoGlobs.buildingData: the building models (0x14c bytes each). */
typedef struct LegoObjH_BuildingModel {
    unsigned char bytes[0x14c];
} LegoObjH_BuildingModel;
void legoobject_queue_for_rockfall_processing_if_eligible(Lego_Object* obj); /* 0043a9e0 */
void legoobject_pair_interaction_gate(Lego_Object* obj, Lego_Object* other); /* 0043a9f0 */
void legoobject_proximity_activity_raise(Lego_Object* obj); /* 0043aab0 */
void legoobject_terrain_cell_variant_update_queue(Lego_Object* obj); /* 0043abf0 */
int legoobject_terrain_cell_variant_visit_callback(Lego_Object* obj, void* data); /* 0043ac90 merged callback */
Lego_Object* legoobject_predug_spawn_coordinate_select_and_create(Lego_Object* originObj, int id); /* 0043ad50 */
Lego_Object* legoobject_emergence_random_spawn_attempt(void); /* 0043aea0 */
Lego_Object* legoobject_creature_spawn_from_emergence(void* model, int type, int id, int bx, int by); /* 0043af30 */
void legoobject_update_all(float elapsed); /* 0043b270 */
void legoobject_remove_all(void); /* 0043b320 */
int legoobject_remove_all_callback(Lego_Object* obj, void* data); /* 0043b350 merged callback */
int legoobject_pick_at_screen_position(unsigned int x, unsigned int y, Lego_Object** outObj); /* 0043b360 */
int legoobject_pick_at_screen_position_callback(Lego_Object* obj, void* data); /* 0043b3b0 merged callback */
void legoobject_select_in_screen_rect(void* viewport, float* corner1, float* corner2); /* 0043b6c0 */
int legoobject_select_in_screen_rect_callback(Lego_Object* obj, void* data); /* 0043b770 merged callback */
void legoobject_first_person_meshes_swap(Lego_Object* obj, unsigned int camera, int swap); /* 0043b820 */
void legoobject_lod_meshes_update(Lego_Object* obj, void* cont, float mediumDist, float highDist, int usePosition); /* 0043b850 */
int legoobject_lod_meshes_update_callback(Lego_Object* obj, void* data); /* 0043b8d0 merged callback */
int legoobject_flags_mask107c33de_or_1c00203_any_test(Lego_Object* obj); /* 0043baf0 */
void legoobject_building_deposit_state_monitor(Lego_Object* obj); /* 0043bb20 */
void legoobject_detach_carried_object_and_reset_state(Lego_Object* obj); /* 0043bbc0 */

/* ---- group I1: 0043bc40-0043c6b0 (part 2, issue #588) ---- */
/* AITask, the fields group I reads. */
typedef struct LegoObjI_AITask {
    int taskType;                     /* +0x00 AITask_Type */
    unsigned char pad_04[0x10 - 0x04];
    Lego_Object* targetObject;        /* +0x10 */
} LegoObjI_AITask;
void legoobject_objective_sidecar_append_gate(Lego_Object* obj); /* 0043bc40 */
int legoobject_state_pair_predicate(int type, int id, int type2, int id2); /* 0043c200 */
int legoobject_nested_float_add_unless_blocked(Lego_Object* obj, void* data); /* 0043c280 */
void legoobject_periodic_timer25_advance(float elapsed, int renderTrack); /* 0043c2b0 */
int legoobject_periodic_timer25_callback(Lego_Object* obj, void* data); /* 0043c2f0 merged callback; verify */
int legoobject_type26_command_completion_gate(Lego_Object* obj); /* 0043c3e0 */
int legoobject_can_do_activity_classify(Lego_Object* obj); /* 0043c440 */
int legoobject_task_type_and_condition_check(void* unused, Lego_Object* target, int condition); /* 0043c490 */
void legoobject_finish_state_1a(Lego_Object* obj); /* 0043c4c0 */
void legoobject_mark_and_finish_state_1a(Lego_Object* obj); /* 0043c530 */
void legoobject_kind_four_post_create_resolve(Lego_Object* obj); /* 0043c570 */
int legoobject_capacity_or_state_predicate(Lego_Object* obj); /* 0043c650 */
void legoobject_activity_counters_advance(Lego_Object* obj, float elapsed); /* 0043c6b0 */

/* ---- group J: 0043c810-0043c810 (part 2, issue #588) ---- */
/* Lego_Block (0x48) and the Lego_Level fields group J reads. */
typedef struct LegoObjJ_Block {
    unsigned char pad_00[2];
    unsigned char terrain;            /* +0x02 */
    unsigned char pad_03[0x0c - 0x03];
    unsigned int flags1;              /* +0x0c */
    unsigned char pad_10[0x48 - 0x10];
} LegoObjJ_Block;
typedef struct LegoObjJ_Level {
    unsigned char pad_00[0x14];
    unsigned int width;               /* +0x14 */
    unsigned char pad_18[0xb8 - 0x18];
    LegoObjJ_Block* blocks;           /* +0xb8 */
} LegoObjJ_Level;
/* Vector3F. */
typedef struct LegoObjJ_Vector3F {
    float x;
    float y;
    float z;
} LegoObjJ_Vector3F;
/* Point2I. */
typedef struct LegoObjJ_Point2I {
    int x;
    int y;
} LegoObjJ_Point2I;
/* AITask (0x68), the fields 0043c810 reads. */
typedef struct LegoObjJ_AITask {
    unsigned char pad_00[0x10];
    Lego_Object* object;              /* +0x10 */
    unsigned char pad_14[0x38 - 0x14];
    int activity;                     /* +0x38 */
} LegoObjJ_AITask;
/* LegoObject_Callback_Update: the per-object update LegoObject_UpdateAll
 * (0043b270) runs through LegoObject_RunThroughLists; data points at the
 * elapsed time. */
int legoobject_update_callback(Lego_Object* obj, void* data); /* 0043c810 */

/* Maths_Vector3DScale and Maths_Vector3DAdd, the inline helpers of
 * Gods98's Maths.h: inlined, they leave no code of their own. */
static __inline LegoObjJ_Vector3F* legoobject_vector3_scale(LegoObjJ_Vector3F* r,
                                                           LegoObjJ_Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline LegoObjJ_Vector3F* legoobject_vector3_add(LegoObjJ_Vector3F* r,
                                                         LegoObjJ_Vector3F* a,
                                                         LegoObjJ_Vector3F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

/* ---- group I2: 0043eea0-0043f7d0 (part 2, issue #588) ---- */
/* legoGlobs.objTeleportQueue: the queued object requests (types, ids). */
typedef struct LegoObjI_Requests {
    int type[20];                     /* +0x00 */
    int id[20];                       /* +0x50 */
} LegoObjI_Requests;
void legoobject_simulation_queue_flush(Lego_Object* obj); /* 0043eea0 */
void legoobject_linked_route_flag_clear(Lego_Object* obj, Lego_Object* target); /* 0043f100 */
void legoobject_trigger_frame_callback(void* cont, void* data); /* 0043f130 merged callback */
int legoobject_spawn_request_queue_add(Lego_Object* obj, int type, int id); /* 0043f150 */
void legoobject_spawn_request_queue_process(Lego_Object* obj); /* 0043f190 */
void legoobject_pool_find_with_local_predicate(Lego_Object* obj); /* 0043f560 */
int legoobject_pool_find_local_predicate_callback(Lego_Object* obj, void* data); /* 0043f580 merged callback */
void legoobject_linked_activity_event_dispatch(Lego_Object* obj, unsigned int events); /* 0043f5b0 */
void legoobject_damage_apply(Lego_Object* obj, float damage, int reactToHit, float elapsed); /* 0043f6a0 */
void legoobject_energy_low_target_route_command_tick(Lego_Object* obj, float elapsed); /* 0043f7d0 */

/* ---- group K: 0043fb40-00440c70 (part 2, issue #588) ---- */
/* The search record of the two radius queries (004407b0, 004408c0) and
 * their LegoObject_RunThroughLists callbacks (00440800, 00440920). */
typedef struct LegoObjK_SearchData {
    Lego_Object* obj;                 /* +0x00 the object at the centre, skipped */
    float x;                          /* +0x04 centre */
    float y;                          /* +0x08 */
    float radius;                     /* +0x0c */
    float damage;                     /* +0x10 00440800: damage at the centre */
    float eventRadius;                /* +0x14 00440800: rock monster event radius */
} LegoObjK_SearchData;
/* Maths_Vector3DSubtract, Maths.h's inline function on Vector3F (x, y,
 * z); the matching build compiles with /Ob1, so it is expanded in place, as
 * the original's was. Maths_Vector3DModulus is legoobject_unit_imports.h's
 * legoobject_vector3d_modulus (it needs <math.h>). */
static __inline float* legoobject_vector3d_subtract(float* r, float* a, float* b)
{
    r[0] = a[0] - b[0];
    r[1] = a[1] - b[1];
    r[2] = a[2] - b[2];
    return r;
}
int legoobject_damage_hurt_sound_submit(Lego_Object* obj, float elapsed, float damage); /* 0043fb40 */
int legoobject_record_pair_pose_copy_and_activity_raise(Lego_Object* obj); /* 0043fc20 */
int legoobject_action_target_cell_is_valid(Lego_Object* obj); /* 0043fdc0 */
int legoobject_route_to_explicit_target(Lego_Object* obj, Lego_Object* target); /* 0043fe70 */
int legoobject_route_marker_setup(Lego_Object* obj); /* 0043fff0 */
void legoobject_update_and_unregister_at_tile(Lego_Object* routeTo, int* blockPos); /* 00440130 */
int legoobject_terrain_cell_to_world_position(Lego_Object* obj, int noUnloadCell); /* 004401b0 */
int legoobject_level_debug_cell_retarget_dispatch(Lego_Object* obj, Lego_Object* target); /* 004403d0 */
void legoobject_spatial_callback_query(Lego_Object* obj, float radius, float damage, float eventRadius); /* 004407b0 */
int legoobject_spatial_callback_query_visit(Lego_Object* obj, void* data); /* 00440800 */
void legoobject_spatial_callback_query_at_optional_coordinate(Lego_Object* obj, float* position, float radius); /* 004408c0 */
int legoobject_spatial_callback_query_at_optional_coordinate_visit(Lego_Object* obj, void* data); /* 00440920 */
void legoobject_set_activity(Lego_Object* obj, int activity, int restart); /* 004409e0 */
void legoobject_attached_direction_vector_update(Lego_Object* obj); /* 00440a10 */
void legoobject_object_list_field_assign_all(Lego_Object* obj, int value); /* 00440bf0 */
int legoobject_grid_route_search_adapter(Lego_Object* obj, unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY, unsigned int** outXs, unsigned int** outYs, unsigned int* outCount); /* 00440c30 */
int legoobject_terrain_grid_path_search(Lego_Object* obj, unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY, unsigned int** outXs, unsigned int** outYs, unsigned int* outCount, int (*callback)(Lego_Object* obj, int* blockPos, void* data), void* data); /* 00440c70 */

/* ---- group L: 004410f0-00442560 (part 2, issue #588) ---- */
struct TerrainMap;
/* Vector3F, for the original's whole-vector copies and Maths.h's inline
 * vector functions. */
typedef struct LegoObjL_Vector3F {
    float x;
    float y;
    float z;
} LegoObjL_Vector3F;
/* Maths_Vector3DScale, one of Maths.h's inline functions (the matching
 * build compiles with /Ob1, so it is expanded in place, as the original's
 * was). */
static __inline LegoObjL_Vector3F* legoobject_vector3d_scale(LegoObjL_Vector3F* r,
    LegoObjL_Vector3F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}
/* The stop callback of legoobject_terrain_neighbor_path_check: called with
 * each newly reached block; nonzero ends the search there. */
typedef int (*LegoObjL_RouteCallback)(Lego_Object* obj, int* blockPos, void* data);
int legoobject_terrain_neighbor_path_check(Lego_Object* obj, unsigned int startX, unsigned int startY, unsigned int endX, unsigned int endY, unsigned int** outXs, unsigned int** outYs, unsigned int* outCount, LegoObjL_RouteCallback callback, void* data); /* 004410f0 */
int legoobject_route_setup_and_marker_associate(Lego_Object* obj, unsigned int count, unsigned int* bxList, unsigned int* byList, float* wPos); /* 00441700 */
void legoobject_action_sequence_reset(Lego_Object* obj, int completed); /* 00441940 */
void legoobject_state_reset(Lego_Object* obj, int restart, int detach); /* 00441b30 */
void legoobject_type5_finalize_if_allowed(Lego_Object* obj); /* 00441ea0 */
int legoobject_face_target_action_start(Lego_Object* obj, Lego_Object* target, int kind); /* 00441ed0 */
void legoobject_kind_effect_slot_init_dispatch(Lego_Object* obj, int kind); /* 004420d0 */
void legoobject_action_route_state_initialize(Lego_Object* obj); /* 00442210 */
void legoobject_transform_position_get(Lego_Object* obj, float* outX, float* outY); /* 00442260 */
void legoobject_surface_vector_normalize(Lego_Object* obj, float* outDir); /* 004422a0 */
void legoobject_velocity_vector_dispatch(Lego_Object* obj, float heading, float* dir); /* 00442300 */
float legoobject_vector_direction_angle_radians_get(Lego_Object* obj); /* 00442480 */
void legoobject_position_to_terrain_cell(Lego_Object* obj, int* outX, int* outY); /* 004424f0 */
float legoobject_world_z_callback(float x, float y, struct TerrainMap* map); /* 00442540 */
float legoobject_world_z_callback_lake(float x, float y, struct TerrainMap* map); /* 00442560 */

/* ---- group M: 004425f0-00443840 (part 2, issue #588) ---- */
/* Maths.h's inline vector functions the group's functions expand (the
 * matching build compiles with /Ob1, as the original was); sqrt for them.
 * legoobject_unit.c includes <math.h> first, so this include adds nothing
 * to the unit. */
#include <math.h>
/* Maths_Vector3DAdd. */
static __inline LegoObjL_Vector3F* legoobject_vector3d_add(LegoObjL_Vector3F* r,
    LegoObjL_Vector3F* a, LegoObjL_Vector3F* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}
/* Point2F and Maths_Vector2DSubtract, _Modulus, _Scale, _Normalise and
 * _DotProduct. */
typedef struct LegoObjM_Point2F {
    float x;
    float y;
} LegoObjM_Point2F;
static __inline LegoObjM_Point2F* legoobject_vector2d_subtract(LegoObjM_Point2F* r,
    LegoObjM_Point2F* a, LegoObjM_Point2F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    return r;
}
static __inline float legoobject_vector2d_modulus(LegoObjM_Point2F* v)
{
    return (float)sqrt(v->x * v->x + v->y * v->y);
}
static __inline LegoObjM_Point2F* legoobject_vector2d_scale(LegoObjM_Point2F* r,
    LegoObjM_Point2F* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    return r;
}
static __inline LegoObjM_Point2F* legoobject_vector2d_normalise(LegoObjM_Point2F* r, LegoObjM_Point2F* a)
{
    float inv;
    inv = 1.0f / legoobject_vector2d_modulus(a);
    return legoobject_vector2d_scale(r, a, inv);
}
static __inline float legoobject_vector2d_dot(LegoObjM_Point2F* a, LegoObjM_Point2F* b)
{
    return a->x * b->x + a->y * b->y;
}
/* Maths_Vector3DSubtract and Maths_Vector3DModulus (Maths.h inlines). */
static __inline LegoObjL_Vector3F* legoobject_m_vector3d_subtract(LegoObjL_Vector3F* r,
    LegoObjL_Vector3F* a, LegoObjL_Vector3F* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}
static __inline float legoobject_m_vector3d_modulus(LegoObjL_Vector3F* v)
{
    float m;
    m = (float)sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    return m;
}
void legoobject_subobject_pose_delta_normalize_and_dispatch(Lego_Object* obj, float param2, float param3); /* 004425f0 */
void legoobject_world_position_set(Lego_Object* obj, float x, float y, float theta, int assignHeading); /* 004428a0 */
int legoobject_movement_accumulator_apply(Lego_Object* obj, float elapsed, float* outTransSpeed); /* 00442b10 */
void legoobject_pose_orientation_dispatch(Lego_Object* obj, float elapsed); /* 00442f80 */
void legoobject_scene_object_pose_position_sync(Lego_Object* obj); /* 00443510 */
int legoobject_random_activity_attempt(Lego_Object* obj); /* 00443670 */
int legoobject_type3_flag04000000_test(Lego_Object* obj); /* 004436f0 */
int legoobject_adjacent_position_candidate_find(Lego_Object* obj, int* blockPos, int* outPos); /* 00443710 */
void legoobject_dispatch_event31_for_pending_flag(Lego_Object* obj); /* 004437f0 */
int legoobject_route_cell_candidate_accept(Lego_Object* obj, int* blockPos, void* data); /* 00443840 */

/* ---- group N: 004438b0-00444620 (part 2, issue #588) ---- */
/* Maths_Vector2DSubtract and Maths_Vector2DModulus, Maths.h's inline
 * functions on Point2F (x, y); the matching build compiles with /Ob1, so
 * they are expanded in place, as the original's were. The modulus needs
 * <math.h>, which the unit includes. */
static __inline float* legoobject_n_vector2d_subtract(float* r, float* a, float* b)
{
    r[0] = a[0] - b[0];
    r[1] = a[1] - b[1];
    return r;
}
#define legoobject_n_vector2d_modulus(v) \
    ((float)sqrt((v)[0] * (v)[0] + (v)[1] * (v)[1]))
/* r = origin + ray * dist on Point2F, an inline vector function of the
 * same kind (00444460 expands it in place). */
static __inline float* legoobject_vector2d_ray_end_point(float* r, float* origin,
                                                         float* ray, float dist)
{
    r[0] = origin[0] + ray[0] * dist;
    r[1] = origin[1] + ray[1] * dist;
    return r;
}
void legoobject_path_step_advance_and_arrival_check(Lego_Object* obj, float elapsed); /* 004438b0 */
void legoobject_linked_flag_and_command_update(Lego_Object* obj); /* 004440a0 */
void legoobject_activity_timeout_accumulate_and_raise(Lego_Object* obj, float elapsed); /* 004440f0 */
int legoobject_random_cell_step_route_issue(Lego_Object* obj); /* 00444260 */
void legoobject_flagged_vector_step_command_issue(Lego_Object* obj, float* dir); /* 00444460 */
void legoobject_enter_event21_state_once(Lego_Object* obj); /* 00444620 */

/* ---- group O: 00444680-00445830 (part 2, issue #588) ---- */
/* The record field_32c points at for a rock monster with stats flag
 * 0x200000 (a bat's home position and its attack flag). */
typedef struct LegoObjO_Home {
    unsigned char pad_00[0x18];
    float x;                          /* +0x18 */
    float pad_1c;                     /* +0x1c */
    float y;                          /* +0x20 */
    unsigned int flags;               /* +0x24 */
} LegoObjO_Home;
int legoobject_step_movement_vector_resolve(Lego_Object* obj, int nextBlock, int turn, int reverse); /* 00444680 */
void legoobject_turn_to_vector_activity_issue(Lego_Object* obj, float* dir); /* 00444fb0 */
void legoobject_path_step_vector_normalize(Lego_Object* obj); /* 004451e0 */
int legoobject_path_step_vector_callback(Lego_Object* obj, void* data); /* 00445340 merged callback */
int legoobject_move_vector_surface_probe(Lego_Object* obj); /* 004455a0 */
void legoobject_behavior_callback_tick(Lego_Object* obj, float elapsed); /* 004456e0 */
int legoobject_behavior_callback_a(Lego_Object* obj, void* data); /* 00445770 merged callback */
int legoobject_behavior_callback_b(Lego_Object* obj, void* data); /* 00445830 merged callback */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_H */
