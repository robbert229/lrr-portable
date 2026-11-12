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

/* ---- group D ---- */

/* ---- group E ---- */

/* ---- group F ---- */

/* ---- group G ---- */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H */
