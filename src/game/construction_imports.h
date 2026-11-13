// SPDX-License-Identifier: LGPL-3.0-or-later
/* Construction.c (GAME-052) imports: C declarations of the callees and
 * globals the unit uses from other modules (ADR 0005 step 6; finding 603).
 * Included only by construction.c and test_c_boundaries.cpp, so the other
 * C units' symbol numbering does not change (finding 568, "Unit-private
 * imports header"). Each declaration names a definition with C linkage, with
 * the parameter types the other C units' headers give it. */
#ifndef LEGORRR_GAME_CONSTRUCTION_IMPORTS_H
#define LEGORRR_GAME_CONSTRUCTION_IMPORTS_H

#include "legoobject_unit.h"
#include "lego_unit.h"
#include "terrain_grid.h"
#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct InfoPoint2I;
struct LevelState;

/* GAME-002 AITask.c (aitask.h). */
void aitask_cmd_create_task(struct EntitySlot* object, float timeIn); /* 00402720 */
void aitask_bobbin_clear_delay(const int* blockPos);                  /* 004027a0 */
void aitask_terrain_cell_refresh_dispatch_current(const int* blockPos,
                                                  int mode);          /* 00402ae0 */
void aitask_cmd_cancel_by_coords(int zone, int matchCell, int field20,
                                 int field24);                        /* 00403700 */
void aitask_cmd_command_kind_record_issue(int field20, int field24, int field28,
                                          const int* blockPos, int zone,
                                          const float* randPair, int flag800); /* 00403750 */
/* GAME-013 ElectricFence (terrain_cell_objects.h). */
void electric_fence_block_surfaces_update(int bx, int by);            /* 0040cea0 */
/* GAME-019 Info.c (info_unit.h). */
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);                         /* 00419b80 */
/* GAME-020 Interface.c (interface_unit.h). */
void cancel_action_for_matching_coordinates(const uint32_t coordinate[2]); /* 0041cfb0 */
/* GAME-021 Lego.c (lego_unit.c; lego_port.cpp's models natively). */
uint32_t terrain_cell_occupied_or_flag_current(int cx, int cy, int mode); /* 00432aa0 */
uint32_t terrain_cell_secondary_bit0_current(const int cell_xy[2]);   /* 00433060 */
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);        /* 00433090 */
void terrain_cell_secondary_bit0_set_current(const int cell_xy[2]);   /* 004325d0 */
void terrain_cell_flag40000000_clear_or_set_current(const int cell_xy[2],
                                                    int set);         /* 00432560 */
void terrain_cell_mutation_progress(struct LevelState* level, uint32_t x,
                                    uint32_t y);                      /* 00433380 */
/* 00432550 Lego.c's block-is-path test takes the block and answers TRUE;
 * lego_unit.c defines it without parameters. The C matching build calls it
 * through this unprototyped declaration, which pushes the block as the
 * original does; the native C++ compile calls it without one. */
#ifdef __cplusplus
int always_accept_place_command_cell(void);
#define CONSTRUCTION_BLOCK_IS_PATH(pos) always_accept_place_command_cell()
#else
int always_accept_place_command_cell();
#define CONSTRUCTION_BLOCK_IS_PATH(pos) always_accept_place_command_cell(pos)
#endif
/* 00431570 Lego_GetMap takes no arguments. lego_unit.c defines it with the
 * port's two ignored cell arguments, so the C matching build calls it
 * through an unprototyped function type, which pushes none, as the original
 * does; the native C++ compile goes through lego_port.cpp's adapter (as
 * fallin_imports.h). */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define CONSTRUCTION_LEGO_GET_MAP() \
    ((struct TerrainMap*)level_current_terrain_get_original())
#else
#define CONSTRUCTION_LEGO_GET_MAP() \
    ((struct TerrainMap*)((void* (*)())level_current_terrain_get)())
#endif
/* GAME-023 LegoObject.c (legoobject_unit.c; the rest in legoobject_unit.h). */
void legoobject_dispatch_event5_and_set_flag(Lego_Object* obj);       /* 00447b00 */
/* GAME-027 Message.c (message.h). */
void cmd_produce(int op, const void* a1, int a2, const int* pair);    /* 00452060 */
/* GAME-043 Stats.c, by the port's names, which entity_stats.c's
 * definitions take in the matching build (entity_stats_link_names.h). */
uint32_t entity_indexed_record_word_at_b0_get(int group, int index,
                                              int slot);              /* 00469ac0 */
uint32_t entity_indexed_record_word_at_b4_get(int group, int index,
                                              int slot);              /* 00469af0 */
uint32_t stats_table_word_at_b8(int type, int row, int stride_index);  /* 00469b20 */
unsigned int entity_stats_word_get(const struct EntitySlot* e);       /* 00469e60 */
unsigned int entity_state_flags_get(const struct EntitySlot* e);      /* 00469e80 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_CONSTRUCTION_IMPORTS_H */
