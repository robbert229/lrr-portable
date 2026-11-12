// SPDX-License-Identifier: LGPL-3.0-or-later
/* What Interface.c (GAME-020, game/interface_unit.c) calls in units that are
 * not converted yet, with C linkage (ADR 0005 step 6).
 *
 * - Port entries whose contract equals the original's are declared here as
 *   the port spells them; their C++ declarations carry extern "C" in place,
 *   and src/game/test_c_boundaries.cpp includes both.
 * - Entries the port does not have, or has with another contract (an extra
 *   GameContext or LevelState argument, x and y for a block pointer, a
 *   declaration only in a frozen per-unit header), are thin unannotated
 *   adapters with the original signature, at the end of the port tail
 *   game/interface_port.cpp.
 * - Lego.c's legoGlobs fields and Input.c's key maps have no C-visible port
 *   owner. Unannotated stand-ins in the port tail take their place until
 *   those units are converted (#544 converts Lego.c).
 *
 * This header is separate from lego.h, cmd.h, entity.h, message.h and
 * frontend.h because other C units include those: every declaration a
 * header adds shifts their symbol numbering (finding 538, rule B). Only the
 * unit and the boundary test include it. */
#ifndef LEGORRR_GAME_INTERFACE_CALLEES_H
#define LEGORRR_GAME_INTERFACE_CALLEES_H

#include <stdint.h>

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct LevelState;
struct Point2I;
struct TerrainMap;
struct TextureSurface;

/* The fields of LegoObject (LegoObject.c, GAME-023) that the unit reads, at
 * their Win32 offsets. The port's EntitySlot has another layout; the unit
 * passes these records to port entries as EntitySlot pointers. */
typedef struct LegoObject {
    int type;                               /* +0x000 */
    int id;                                 /* +0x004 */
    char* customName;                       /* +0x008 */
    unsigned char pad_00c[0x308 - 0x00c];
    struct LegoObject* interactObject;      /* +0x308 */
    unsigned char pad_30c[0x324 - 0x30c];
    unsigned int carriedCount;              /* +0x324 */
    unsigned int carryCount;                /* +0x328 */
    unsigned char pad_32c[0x330 - 0x32c];
    unsigned int objLevel;                  /* +0x330 */
    unsigned char pad_334[0x340 - 0x334];
    float health;                           /* +0x340 */
    float energy;                           /* +0x344 */
    unsigned char pad_348[0x36c - 0x348];
    struct LegoObject* driveObject;         /* +0x36c */
    unsigned char pad_370[0x3d4 - 0x370];
    float field_3d4;                        /* +0x3d4 */
    int field_3d8;                          /* +0x3d8 */
    unsigned char pad_3dc[0x3e0 - 0x3dc];
    unsigned int flags1;                    /* +0x3e0 */
    unsigned int flags2;                    /* +0x3e4 */
    unsigned int flags3;                    /* +0x3e8 */
    unsigned int flags4;                    /* +0x3ec */
    unsigned int abilityFlags;              /* +0x3f0 */
} LegoObject;


/* Lego_Block (0x48 bytes) and the Lego_Level fields the unit reads. */
typedef struct LegoBlock {
    unsigned char pad_00[2];
    unsigned char terrain;                  /* +0x02 */
    unsigned char pad_03[5];
    unsigned int flags1;                    /* +0x08 */
    unsigned char pad_0c[0x48 - 0x0c];
} LegoBlock;

typedef struct LegoLevel {
    unsigned char pad_00[0x14];
    unsigned int width;                     /* +0x14 */
    unsigned char pad_18[0xb8 - 0x18];
    LegoBlock* blocks;                      /* +0xb8 */
} LegoLevel;

typedef struct LegoCamera {
    unsigned char pad_00[0x14];
    int mode;                               /* +0x14 */
} LegoCamera;

/* One entry of a Dependencies.c requirement list (0x10 bytes). */
typedef struct InterfaceDependency {
    int objType;
    int objIndex;
    int level;
    int showLevel;
} InterfaceDependency;

/* The object search the 0041f1xx-0041f3xx helpers hand to the object
 * pool (0x18 bytes). */
typedef struct InterfaceObjectSearch {
    int objType;                            /* +0x00 */
    int objIndex;                           /* +0x04 */
    unsigned int statsMask;                 /* +0x08 */
    unsigned int stateMask;                 /* +0x0c */
    unsigned int abilityMask;               /* +0x10 */
    unsigned int flags;                     /* +0x14: 1 type, 2 ability */
} InterfaceObjectSearch;

#define LEGOOBJ(o) ((struct EntitySlot*)(o))
#define INTERFACE_OBJECT_CALLBACK(f) ((int (*)(struct EntitySlot*, void*))(f))

/* ---- legoGlobs (Lego.c) stand-ins and key maps (Input.c) ---------------- */

/* DAT_005560d4 legoGlobs.currLevel. terrain_grid.cpp owns it. */
extern struct LevelState* g_active_level_retail;
#define lego_current_level ((LegoLevel*)g_active_level_retail)
#define LEGO_LEVEL_BLOCK(level, x, y) \
    (&((LegoLevel*)(level))->blocks[((LegoLevel*)(level))->width * (y) + (x)])
#define LEGO_BLOCK(x, y) LEGO_LEVEL_BLOCK(g_active_level_retail, x, y)

/* DAT_005560ec legoGlobs.cameraFP and DAT_00556160 legoGlobs.objectFP. */
extern LegoCamera* lego_camera_fp;
extern LegoObject* lego_object_fp;
/* DAT_005561e8/DAT_005561f4 the vehicle and building type names, and
 * DAT_0055627c/DAT_00556288 their counts. */
extern char** lego_vehicle_names;
extern char** lego_building_names;
extern unsigned int lego_vehicle_type_count;
extern unsigned int lego_building_type_count;
#define LEGO_VEHICLE_TYPE_COUNT lego_vehicle_type_count
#define LEGO_BUILDING_TYPE_COUNT lego_building_type_count
#define LEGO_VEHICLE_NAME(i) (lego_vehicle_names[i])
#define LEGO_BUILDING_NAME(i) (lego_building_names[i])
/* DAT_0055644c..DAT_00556454 the primary teleporters. */
extern LegoObject* lego_teleport_primary_small;
extern LegoObject* lego_teleport_primary_big;
extern LegoObject* lego_teleport_primary_laser;
/* DAT_00556474 (types) and DAT_005564c4 (indices): the queued object
 * requests, DAT_00556514 their count. */
typedef struct LegoObjectRequests {
    int type[20];
    int index[20];
} LegoObjectRequests;
extern LegoObjectRequests lego_object_requests;
extern unsigned int lego_object_request_count;
/* DAT_00556ebc, DAT_00556f98/DAT_00556f9c, DAT_00556fac: the upgrade-cost
 * tool tip, its ore and stud costs, and the custom-name separator. */
extern const char* lego_upgrade_cost_text;
extern unsigned int lego_upgrade_cost_ore;
extern unsigned int lego_upgrade_cost_studs;
extern const char* lego_custom_name_separator;
/* DAT_00556ecc legoGlobs.flags1 and DAT_00556ed4 legoGlobs.flags2. */
extern unsigned int lego_flags1;
extern unsigned int s_global_low_word_c;
/* DAT_00556f58: the upgrade level names. */
extern char* lego_upgrade_level_names[16];
/* DAT_004de814: LegoObject.c's tool-tip sounds, [type * 15 + index]. */
extern int object_tooltip_sfx[];
/* DAT_004eadc0: the ore icon of the cost tool tip. */
extern void* lego_tooltip_ore_icon;
/* DAT_0076ab80/DAT_0076aa00: Input.c's key map and the previous frame's. */
extern unsigned char input_key_map[256];
extern unsigned char input_prev_key_map[256];

/* ---- port entries with the original contract ---------------------------- */

/* AITask.c (GAME-002): game/cmd.hpp */
void cmd_mark_matching(int type, int payload, const int coords[2]);
void task_type23_create_for_category_mask(struct EntitySlot* e, int selector,
                                          int issue_records);
void entity_kind_command_issue(struct EntitySlot* e);
void entity_conditional_task_schedule(struct EntitySlot* e);
void object_command_task_create(struct EntitySlot* entity, int parameter);
void entity_threshold_progress_event_queue(void* payload, int flag);
int object_event_type2_queue(void* payload);
/* ToolTip.c (GAME-046): game/cmd.hpp */
void command_sample_stats_accumulate(int kind, const void* sample);
/* Lego.c (GAME-021): game/cmd.hpp, game/level.hpp, game/frontend.hpp,
 * gods98/engine.hpp */
int selected_entities_any_predicate_match(int cx, int cy, int require_stat_bit);
int any_object_can_occupy_terrain_cell(int cx, int cy);
void terrain_cell_work_flag_clear_current(const int coord[2],
                                          uint32_t selector);
int terrain_cell_route_and_occupancy_check_current(
    const struct EntitySlot* entity, int from_x, int from_y, int to_x,
    int to_y, int strict);
int terrain_cell_walkability_query_current(const struct EntitySlot* entity,
                                           uint32_t x, uint32_t y,
                                           int require_stat_bit,
                                           int gated_mode);
uint32_t terrain_cell_secondary_flag_8_current(int cx, int cy);
uint32_t terrain_cell_occupied_or_flag_current(int cx, int cy, int mode);
int terrain_cell_walkable_flag_current(uint32_t cx, uint32_t cy);
uint32_t terrain_cell_flag_20_bounded_current(uint32_t cx, uint32_t cy);
uint32_t terrain_cell_flag_reserved_current(const int cell_xy[2]);
uint32_t terrain_cell_mask_c0_current(int cx, int cy);
uint32_t terrain_cell_flag_8000_current(const int cell_xy[2]);
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);
int interface_spawn_request_enqueue(int type, uint32_t parameter);
void global_state_low_byte_clear_and_refresh(void);
/* AITask.c (GAME-002), Construction (GAME-007), Dependencies.c (GAME-010):
 * game/level.hpp, gods98/simtick.hpp */
void terrain_cell_refresh_dispatch_current(const int cell_xy[2], int mode);
int terrain_cell_predicate_check_current(const uint32_t pair_key[2]);
void simtick_selection_group_clear(const uint32_t key_pair[2]);
int ensure_cell_entries_loaded(uint32_t kind, int row, int slot);
/* FUN_00409300 Construction_PathCreate-style; FUN_0040af40, the object-pool
 * callback that tests one dependency (the port spells its arguments as two
 * records). */
void* tutorial_path_descriptor_create(const uint32_t* coord_pair);
int terrain_cell_resource_entry_matches(const uint32_t* cell_record,
                                        const uint32_t* resource_entry);
/* Encyclopedia (GAME-014): crt/crt.hpp */
void global_flag_bit0_clear(void);
/* LegoObject.c (GAME-023): game/entity.hpp, game/ai.hpp */
int entity_pool_find_by_callback_filtered_current(
    int (*callback)(struct EntitySlot*, void*), void* payload);
int building_upgrade_resource_admission_check(struct EntitySlot* building,
                                              uint32_t* out_secondary_cost);
void building_upgrade_commit(struct EntitySlot* building);
int entity_action_kind_prerequisite_check(struct EntitySlot* entity,
                                          int action);
struct EntitySlot* entity_route_marker_candidate_find(
    struct EntitySlot* source, int include_secondary);
const char* localized_object_name_get(uint32_t kind, int index);
uint32_t entity_stats_flag10_test(const struct EntitySlot* e);
uint32_t entity_stats_flag20_test(const struct EntitySlot* e);
uint32_t entity_stats_flag40_test(const struct EntitySlot* e);
int entity_vehicle_unload_cell_find_if_eligible(const struct EntitySlot* entity,
                                                int out_xy[2]);
int entity_record_flag20_query(const struct EntitySlot* entity,
                               int out_xy[2]);
struct EntitySlot* entity_action_kind_entity_create_and_command_issue(
    struct EntitySlot* source, int kind, int selector, int argument);
int entity_shared_eligibility_check(const struct EntitySlot* e, int relaxed);
int ai_activity_scan(const struct EntitySlot* e, int activity);
void entity_activity23_state_transition(struct EntitySlot* e);
/* Message.c (GAME-027): game/select.hpp */
int selection_active_has_any_get(void);
struct EntitySlot* selection_active_entity_get(void);
struct EntitySlot* const* selection_active_records_get(void);
uint32_t selection_active_count_get(void);
uint32_t selection_contains_entity_active_target(struct EntitySlot* entity,
                                                 uint32_t* index_out);
void selection_clear_active(void);
#define selection_active_entity_get_obj() \
    ((LegoObject*)selection_active_entity_get())
#define selection_active_records_get_obj() \
    ((LegoObject**)selection_active_records_get())
/* Stats.c (GAME-043): game/entity.hpp */
uint32_t entity_indexed_record_word_at_b4_get(int group, int index, int slot);
uint32_t entity_stats_word_get(const struct EntitySlot* e);
uint32_t entity_state_flags_get(const struct EntitySlot* e);
uint32_t entity_capability_flags_get(const struct EntitySlot* e);
uint32_t stats_table_word_at_148(int type, int row);
uint32_t stats_table_word_at_14c(int type, int row);
uint32_t building_upgrade_admission_check(int type, int row);
uint32_t stats_instance_word_at_ec(const void* config_block);
/* LegoObject.c (GAME-023): game/entity.hpp */
int entity_action_target_cell_is_valid(struct EntitySlot* entity);
/* SFX.c (GAME-041): gods98/sound.hpp */
int sound_submit_with_global_cooldown(int alias, int* out_handle);

/* ---- adapters (port tail), original signatures --------------------------- */

/* AITask_DoDig-style block tasks (FUN_00402510, FUN_00402d50, FUN_00403030,
 * FUN_00402fb0): the port's entries take x and y. */
void lego_ai_create_dig(const struct Point2I* blockPos, int arg2, int arg3);
void lego_ai_create_dynamite(const struct Point2I* blockPos, int arg2);
void lego_ai_create_reinforce(const struct Point2I* blockPos);
void lego_ai_create_dam(const struct Point2I* blockPos);
/* FUN_004031e0: the port's entry takes a GameContext. */
int aitask_list_contains(int code);
/* FUN_00409600: a ledger stub in a namespace. */
void lego_path_remove(const struct Point2I* blockPos);
/* FUN_0040af90 Dependencies_GetRequirements: declared only in frozen
 * per-unit headers. */
void dependencies_get_requirements(int objType, int objIndex, int level,
                                   InterfaceDependency** list,
                                   unsigned int* count);
/* FUN_0040d240: the port's entry takes the level and a sidecar query. */
int lego_block_has_lava_neighbour(int x, int y);
/* FUN_0041f8e0 and FUN_0041f900: Lego_GetResourceCount-style queries; the
 * port's entries have other contracts. */
int lego_get_resource_count(int type);
unsigned int lego_get_resource(int type);
/* FUN_00425d50: set the tracked object; no port entry. */
void lego_set_tracked_object(LegoObject* obj);
/* FUN_00429880: declared only in a frozen per-unit header. */
float lego_get_min_health(void);
/* FUN_0042e8b0: declared only in a frozen per-unit header. */
int lego_object_type_for_name(const char* name, int* objType, int* objIndex,
                              int* level);
/* FUN_0042f6f0: the port's entry has another parameter list. */
void lego_level_block_update(LegoLevel* level, int x, int y, int arg);
/* FUN_00431570 Lego_GetMap: the port's entry takes a block position. */
struct TerrainMap* lego_get_map(void);
/* FUN_00432a20: the port's entry takes the level too. */
int lego_object_can_enter_block(LegoObject* obj, int x, int y);
/* FUN_00438580: the port's entry takes a GameContext. */
void lego_object_set_primary_teleporter(LegoObject* obj, int on);
/* FUN_0043a7c0: the port's entry takes three arguments. */
int lego_object_can_eject(LegoObject* obj, int kind, int arg3, int arg4);
/* FUN_0043c200: the port's entry takes a GameContext. */
int lego_object_pair_requirement_met(int objType, int objIndex,
                                     int otherType, int otherIndex);
/* FUN_0043a5f0, FUN_0043fc20, FUN_004401b0, FUN_004424f0, FUN_004489a0:
 * LegoObject.c entries the port has with other contracts, or in a
 * namespace. */
void* lego_object_get_container(LegoObject* obj);
void lego_object_drop_carried(LegoObject* obj);
void lego_object_go_to_block(LegoObject* obj, int dig);
void lego_object_get_block(LegoObject* obj, int* x, int* y);
/* FUN_00438e50 and FUN_00439e70: ledger stubs in a namespace. */
LegoObject* lego_object_find_trainer(LegoObject* obj, const Point2F* center,
                                     unsigned int mask);
void lego_object_detach_all(LegoObject* obj, int resetMode);
void entity_kind6_tick_down_marker_spawn_obj(LegoObject* obj);
/* FUN_004754c0 Container_GetPosition: the port's entry is in view.cpp's
 * backend region. */
void container_get_position(void* container, void* reference,
                            Point2F* position);
/* FUN_0046b730 and FUN_0046b7a0: declared only in frozen per-unit
 * headers. */
void tooltip_set_persistent(int index, int set);
void tooltip_reset_timer(int index);

/* ---- the port's implementations of the shared entries ------------------
 * Native builds only: the unit's functions that keep the port's names
 * delegate to these (finding 564). The matching build compiles the original
 * bodies. */
#if !(defined(_MSC_VER) && _MSC_VER == 1100)
void command_context_post_to_hud_port(int kind, const uint32_t coord[2]);
uint32_t sim_status_banner_word_get_port(void);
void simulation_global_word_set_port(uint32_t value);
void advisor_callback_schedule_gate_port(void);
void* input_binding_default_value_get_port(int kind, int index, int mode);
void position_tracker_box_draw_port(const float bounds[4]);
void selection_mode7_cancel_at_point_port(const uint32_t coordinate[2]);
void cancel_action_for_matching_coordinates_port(const uint32_t coordinate[2]);
void selection_mode5_cancel_at_point_port(const uint32_t coordinate[2]);
void selection_clear_request_and_hud_reset_port(void);
int cell_action_permission_get_port(int column, int row);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_INTERFACE_CALLEES_H */
