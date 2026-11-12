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
    unsigned char pad_030[0x28c - 0x030];
    float field_28c;                  /* +0x28c */
    float field_290;                  /* +0x290 */
    float field_294;                  /* +0x294 */
    int field_298;                    /* +0x298 */
    int field_29c;                    /* +0x29c */
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
    void* field_2dc;                  /* +0x2dc */
    int field_2e0;                    /* +0x2e0 */
    int field_2e4;                    /* +0x2e4 */
    int activity;                     /* +0x2e8 activity index */
    int field_2ec;                    /* +0x2ec */
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
    void* field_354;                  /* +0x354 */
    unsigned char pad_358[0x360 - 0x358];
    float field_360;                  /* +0x360 */
    void* field_364;                  /* +0x364 */
    float field_368;                  /* +0x368 */
    struct Lego_Object* driveObject;  /* +0x36c */
    int carriedTools[5];              /* +0x370 */
    unsigned int numCarriedTools;     /* +0x384 */
    unsigned char pad_388[0x3b4 - 0x388];
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
int legoobject_remove(Lego_Object* obj);                        /* 00437540 */
int legoobject_run_through_lists_skip_upgrade_parts(Lego_ObjectCallback callback, void* data); /* 004377b0 */
int legoobject_run_through_lists(Lego_ObjectCallback callback, void* data, int skipUpgradeParts); /* 004377d0 */
void legoobject_rename(Lego_Object* obj, const char* name);     /* 00437880 */
void legoobject_grid_object_records_release(void);              /* 004378e0 */
void legoobject_deferred_olist_create_and_link(int* record);    /* 00437940 */
void legoobject_deferred_olist_queue_add(int a, int b, int c, void* d, int e, int f, int g, int h); /* 00437c20 */
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
int legoobject_shared_eligibility_check(Lego_Object* obj, int relaxed); /* 004385b0 */
Lego_Object* legoobject_create_in_world(void* model, int type, int id, int last, float x, float y, float heading); /* 00438610 */
Lego_Object* legoobject_at_grid_position_find(int* cell);       /* 00438670 */
int legoobject_grid_position_match_callback(Lego_Object* obj, void* data); /* 004386b0 */
int legoobject_available_resource_spend(int resource, int amount); /* 00438720 */
int legoobject_building_upgrade_resource_admission_check(Lego_Object* building, unsigned int* outCost); /* 00438770 */
void legoobject_building_upgrade_commit(Lego_Object* building); /* 004387f0 */
int legoobject_action_kind_prerequisite_check(Lego_Object* obj, int action); /* 004388b0 */
int legoobject_unknown_00438960(int a, int b);                  /* 00438960 */
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
int legoobject_dock_definition_matches_cell(int* cell);         /* 00438f60 */
int legoobject_route_marker_candidate_visit(Lego_Object* obj, void* data); /* 00438fb0 */
int legoobject_task_kind_filtered_search(int selector);         /* 00439200 */
int legoobject_task_flags_and_cell_task_match(Lego_Object* obj, void* task); /* 00439240 */
void legoobject_build_placement_command_batch_expand(int* cell); /* 00439280 */
void legoobject_build_placement_command_pair_create(int* cell, int id); /* 00439340 */
void legoobject_build_placement_command_setup(int* cell, int id, float* world, int update); /* 00439370 */
void legoobject_build_placement_alternate_command_setup(int* cell, int id, float* world, int update); /* 004394b0 */
const char* legoobject_display_name_lookup(Lego_Object* obj);  /* 004395e0 */
const char* legoobject_item_name_by_kind(unsigned int kind, int index); /* 004396c0 */
const char* legoobject_the_name_get(unsigned int kind, int index); /* 00439790 */
const char* legoobject_name_get(unsigned int kind, int index);  /* 00439860 */
void legoobject_copy_two_dword_pair(void* record, unsigned int* outA, unsigned int* outB); /* 00439930 */

/* ---- group F: 00439950-00439e40 ---- */
void legoobject_tool_name_strings_load(void* loader, void* config); /* 00439950 */
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
void legoobject_attached_object_list_detach(Lego_Object* obj, int resetMode); /* 00439e70 */
int legoobject_cell_step_command_issue_with_jitter(Lego_Object* obj, int* target, float* input, int mode, int markPending); /* 0043a120 */
int legoobject_deposit_activity_gate(Lego_Object* obj, Lego_Object* target); /* 0043a300 */
void legoobject_visual_resource_get_or_create(Lego_Object* obj); /* 0043a5f0 */
Lego_Object* legoobject_action_kind_create_and_command_issue(Lego_Object* source, int kind, int selector, int argument); /* 0043a650 */
int legoobject_action_kind_supported(Lego_Object* obj, int kind, int argument); /* 0043a7c0 */
void legoobject_action_record_consume_and_cleanup(Lego_Object* obj, Lego_Object* record); /* 0043a850 */
void legoobject_simulation_update_counter_add(int unused, int count); /* 0043a8f0 */
void legoobject_simulation_mode_counter_add(int mode, int count); /* 0043a910 */
void legoobject_tracked_pointer_remove(void* pointer);          /* 0043a930 */
void legoobject_lava_crust_activity_register(Lego_Object* obj); /* 0043a960 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_H */
