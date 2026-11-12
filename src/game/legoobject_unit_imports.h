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

/* ---- group C ---- */

/* ---- group D ---- */

/* ---- group E ---- */

/* ---- group F ---- */

/* ---- group G ---- */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_UNIT_IMPORTS_H */
