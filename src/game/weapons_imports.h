// SPDX-License-Identifier: LGPL-3.0-or-later
/* Weapons.c (GAME-050) imports: C declarations of the callees and globals the
 * unit uses from other modules (ADR 0005 step 6; finding 582). Included only
 * by weapons.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, with the port's
 * parameter types; weapons.c casts its original-layout records to them. */
#ifndef LEGORRR_GAME_WEAPONS_IMPORTS_H
#define LEGORRR_GAME_WEAPONS_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/maths.h"
#include "gods98/mesh_unit.h"
#include "gods98/utils.h"
#include "gods98/view.h"
#include "bezier.h"
#include "lego_unit.h"
#include "legoobject_unit.h"
#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct LevelState;
struct EngineTaskCallbackRecord;
struct MeshRecordPool;

/* GAME-021 Lego.c (terrain_grid.cpp, engine.cpp, lego.cpp, level.hpp). */
extern struct LevelState* g_active_level_retail;   /* 005560d4 currLevel */
extern const char* s_engine_config_root;            /* 005560c4 gameName */
extern float s_scroll_scale;                        /* 0055615c gameSpeed */
extern uint32_t s_panel_highlight_flags;            /* 00556ed0 flags2 */
struct LevelState* level_current_owner_get(void);   /* 00431560 Lego_GetLevel */
/* 00431570 Lego_GetMap takes no arguments. lego_unit.c defines it with the
 * port's two ignored cell arguments, so the C matching build calls it
 * through an unprototyped function type, which pushes none, as the original
 * does; the native C++ compile goes through lego.cpp's adapter (as
 * aitask_imports.h). */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define WEAPONS_LEGO_GET_MAP() \
    ((struct TerrainMap*)level_current_terrain_get_original())
#else
#define WEAPONS_LEGO_GET_MAP() \
    ((struct TerrainMap*)((void* (*)())level_current_terrain_get)())
#endif
#define WEAPONS_LEGO_GET_LEVEL() ((Lego_Level*)level_current_owner_get())
#define WEAPONS_MAP_WIDTH() (WEAPONS_LEGO_GET_MAP()->gridWidth)
#define WEAPONS_MAP_HEIGHT() (WEAPONS_LEGO_GET_MAP()->gridHeight)
#define legoGlobs_currLevel_blocks \
    LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
/* 0041f8e0 is Lego_GetCrystalCount(flag) in the original; lego_unit.c
 * defines it as noop_callback(void), so the unit calls it with the
 * original's argument and result through a cast (as legoobject_unit.c). */
#define Lego_GetCrystalCount(onlyUsable) (((int (*)(int))noop_callback)(onlyUsable))
int terrain_cell_walkable_flag_current(uint32_t cx, uint32_t cy); /* 00432bd0 */
int terrain_cell_state_equals_one_current(const int cell_xy[2]);  /* 00432ec0 */
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);    /* 00433090 */
int terrain_cell_drill_work_add_current(int x, int y, float divisor,
                                        float elapsed);           /* 00434570 */

/* GAME-023 LegoObject.c (legoobject_port.cpp, entity.hpp, ai.hpp; lego.cpp's
 * original-ABI adapters). */
/* In the matching build these name legoobject_unit.c's functions at the
 * same addresses (GAME-023 part 3, issue #615); one declaration each. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define entity_type_component_get legoobject_type_component_get
#define entity_directional_activity_start legoobject_directional_activity_start
#define entity_one_shot_attached_effect_start legoobject_one_shot_attached_effect_start
#define entity_pair_scalar_fields_set legoobject_pair_scalar_fields_set
#endif
void* entity_type_component_get(const void* entity_record);       /* 0044a200 */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y);         /* 00442260 */
int entity_type3_flag04000000_test(const struct EntitySlot* e);   /* 004436f0 */
void entity_directional_activity_start(struct EntitySlot* entity,
                                       const float direction[2],
                                       int mode);                 /* 00447950 */
int entity_one_shot_attached_effect_start(struct EntitySlot* entity,
                                          float duration);        /* 0044c030 */
int ai_activity_probe(const struct EntitySlot* e);                /* 0043c440 */
int entity_task_at_grid_coordinate_find(uint32_t x, uint32_t y,
                                        struct EntitySlot** out_entity); /* 0043b360 */
/* 0043f6a0 and 0044c150 take the original's arguments only through
 * lego.cpp's adapters natively: the port's entries take three arguments,
 * and an integer pair. The matching build calls the annotated functions
 * with the original's arguments; the extra one is ignored (cdecl). */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define entity_damage_apply_original entity_damage_apply
#define entity_pair_scalar_fields_set_original entity_pair_scalar_fields_set
#endif
void entity_damage_apply_original(struct EntitySlot* entity, float damage,
                                  int emitEffects, float elapsed); /* 0043f6a0 */
int entity_pair_scalar_fields_set_original(struct EntitySlot* entity,
                                           const float* pair,
                                           float scalar);         /* 0044c150 */

/* GAME-043 Stats.c (entity_stats.cpp, entity.hpp). */
float entity_stats_float_at6c(const struct EntitySlot* e);        /* 00469d40 */
float entity_stats_float_at74(const struct EntitySlot* e);        /* 00469d50 */
void* entity_stats_field_at_a0_corpus(const struct EntitySlot* e); /* 00469da0 */
uint32_t entity_stats_word_get(const struct EntitySlot* e);       /* 00469e60 */
uint32_t entity_state_flags_get(const struct EntitySlot* e);      /* 00469e80 */
uint32_t building_upgrade_admission_check(int type, int row);     /* 00469f40 */
float entity_speed_component_get(const struct EntitySlot* e);     /* 0046a0a0 */
float entity_collision_radius_get(const struct EntitySlot* e);    /* 0046a270 */
float entity_stats_float_at114(const struct EntitySlot* e);       /* 0046a290 */
float entity_stats_float_at118(const struct EntitySlot* e);       /* 0046a2b0 */
float entity_stats_float_at11c(const struct EntitySlot* e);       /* 0046a2d0 */
float entity_stats_float_at110(const struct EntitySlot* e);       /* 0046a2f0 */
float entity_stats_float_at10c(const struct EntitySlot* e);       /* 0046a310 */

/* Other game units. */
void point_project_onto_line2d(const float* line_start, const float* line_end,
                               const float* point, float* out);   /* 00408b60 */
void point2d_project_onto_direction(const float* origin, const float* direction,
                                    const float* point, float* out); /* 00408bf0 */
void scene_light_create_at_position(const float position[3]);     /* 0040be10 */
int lava_effect_select_and_place(int slot, const float* position,
                                 const float* rotation);          /* 0040cce0 */
uint32_t renderer_word_a_get(void);                               /* 00410b30 */
uint32_t renderer_word_b_get(void);                               /* 00410b40 */
uint32_t renderer_word_c_get(void);                               /* 00410b50 */
void global_counter_at_0055532c_increment(void);                  /* 00462890 */
int sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                   int loop, int positional,
                                   const float* xyz);             /* 00465050 */
/* 00465160 returns on the x87 stack either way; the original declares it
 * float, the port double (sound.hpp). */
#if defined(_MSC_VER) && _MSC_VER < 1200
float sound_object_duration_get(int alias);                       /* 00465160 */
#else
double sound_object_duration_get(int alias);                      /* 00465160 */
#endif

/* gods98 Container.c and Mesh.c (view.hpp, surface.hpp, record.hpp). The
 * container's master frame is at +0x04. */
#define WEAPONS_CONTAINER_FRAME(cont) (*(void**)((char*)(cont) + 0x4))
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);              /* 004754c0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]); /* 00475500 */
void scene_object_coordinate_transform_in_place(void* object,
                                                void* coordinates,
                                                void* transform); /* 00475070 */
struct EngineTaskCallbackRecord* engine_task_callback_record_allocate_and_initialize(
    void* owner, void* callback, uint32_t mode, void* context,
    uint32_t flag_value);                                         /* 00480870 */
int mesh_record_allocate_and_fill(struct MeshRecordPool* pool,
                                  uint32_t vertex_count,
                                  uint32_t index_rows,
                                  uint32_t index_columns,
                                  const uint16_t* source_indices); /* 00482220 */
void mesh_segment_vertex_fill(void* record, int block, int first_vertex,
                              int count, const float* positions,
                              const uint32_t triple[3],
                              const uint32_t* pairs);             /* 00482620 */
uint32_t render_object_color_vector_set(void* record, int block, float red,
                                        float green, float blue,
                                        int slot);                /* 00483270 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_WEAPONS_IMPORTS_H */
