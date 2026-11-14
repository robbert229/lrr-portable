// SPDX-License-Identifier: LGPL-3.0-or-later
/* objectGlobs (LegoObject.c, GAME-023): the original's one
 * LegoObject_Globs struct, at 004de790, 0xc63c bytes (004de790-004eadcc;
 * LegoObject_Initialise zeroes exactly that much). legoobject_unit.c
 * defines it (issue #775, finding 662). 004eadcc (the drill-frame
 * counter) and 004eadd0 (the released-flock list) follow it and stay
 * separate globals.
 *
 * Every field sits at its original offset. The names the units and the
 * port used for the fields while they were separate globals are macros
 * over the fields, so their code is unchanged (as lego_globs.h does for
 * legoGlobs, finding 653):
 *
 * - fields only C units read (legoobject_unit.c's and lego_unit.c's old
 *   stand-ins) are the struct's in every build;
 * - fields a C++ port file reads as well are the struct's in the matching
 *   build only. Natively legoobject_unit.c (legoobject_port.cpp for the
 *   two name tables) keeps them as separate globals, with their initial
 *   values, and every file reads them by those names, as before.
 *
 * The macros make any later declaration of the same names invalid, so
 * include this header after every header that declares them (C++ port
 * files: after their frozen headers). */
#ifndef LEGORRR_GAME_LEGOOBJECT_GLOBS_H
#define LEGORRR_GAME_LEGOOBJECT_GLOBS_H

/* The members have only basic and void* types, and the header includes
 * nothing (not even <stdint.h>, which a unit may not include yet). The
 * struct and objectGlobs then cost an including unit two symbols, and the
 * directive one (finding 538, finding 653). The macros give the pointer
 * fields their types. */
#ifdef __cplusplus
extern "C" {
#endif

struct LegoObject_Globs {
    void* listSet[32];                       /* +0x0000 004de790 s_pool_chunks; the object pool's chunks */
    void* freeList;                          /* +0x0080 004de810 s_pool_record_head */
    int field_84[300];                       /* +0x0084 004de814 s_terrain_sfx_grid; per-surface tool tip sounds */
    const char* activityName[79];            /* +0x0534 004decc4 s_activity; legoobject_port.cpp */
    float* field_670;                        /* +0x0670 004dee00 s_terrain_neighbor_path_cost; path-search cost map */
    unsigned int* field_674;                     /* +0x0674 004dee04 s_terrain_path_depth; path-search step map */
    unsigned int field_678;                      /* +0x0678 004dee08 s_terrain_path_depth_count */
    float field_67c;                         /* +0x067c 004dee0c objectGlobs_f_4dee0c; 25-second cycle */
    unsigned int listCount;                      /* +0x0680 004dee10 s_pool_chunk_count */
    unsigned int flags;                          /* +0x0684 004dee14 s_entity_state_flags */
    unsigned int toolNullIndex[11];              /* +0x0688 004dee18 s_activity_resource_slots */
    unsigned int field_6b4[0x12c0];              /* +0x06b4 004dee44 s_grid_count_second; [type][15][16] */
    unsigned int field_51b4[0x12c0];             /* +0x51b4 004e3944 s_grid_count_first; [type][15][16] */
    unsigned int field_9cb4;                     /* +0x9cb4 004e8444 objectGlobs_f_4e8444; recorded event masks */
    void* field_9cb8;                        /* +0x9cb8 004e8448 s_lego_f_4e8448; the first mini-figure loaded */
    int field_9cbc[20][2];                   /* +0x9cbc 004e844c s_predug_coordinates */
    int field_9d5c[10][2];                   /* +0x9d5c 004e84ec s_type11_cell_coordinates */
    unsigned int field_9dac;                 /* +0x9dac 004e853c s_predug_coordinate_count */
    unsigned int field_9db0;                 /* +0x9db0 004e8540 s_type11_cell_coordinate_count */
    unsigned int recordObjs[200][11];            /* +0x9db4 004e8544 objectGlobs_recordObjs (LegoObjGB_DeferredRecord, 0x2c each) */
    unsigned int recordObjsCount;            /* +0xc014 004ea7a4 objectGlobs_recordObjsCount */
    float field_c018;                        /* +0xc018 004ea7a8 objectGlobs_queuedAnimationFraction */
    unsigned int field_c01c[6];                  /* +0xc01c 004ea7ac s_render_config_snapshot */
    void* field_c034[256];                   /* +0xc034 004ea7c4 objectGlobs_f_4ea7c4 (Lego_Object*); visited units */
    int field_c434;                          /* +0xc434 004eabc4 s_object_f_4eabc4; their count */
    int field_c438;                          /* +0xc438 004eabc8 s_object_f_4eabc8; the buildings among them */
#ifdef __cplusplus
    struct EntitySlot* liveObjs[100];        /* +0xc43c 004eabcc s_idle100 */
#else
    void* liveObjs[100];                     /* +0xc43c 004eabcc s_idle100 */
#endif
    int liveCount;                           /* +0xc5cc 004ead5c s_idle100_n */
    const char* abilityName[6];              /* +0xc5d0 004ead60 s_ability_type_names; legoobject_port.cpp */
    void* ToolTipIcon_Abilities[6];          /* +0xc5e8 004ead78 s_object_toolTipIconAbilities */
    void* ToolTipIcon_Tools[11];             /* +0xc600 004ead90 s_object_toolTipIcons */
    void* ToolTipIcon_Blank;                 /* +0xc62c 004eadbc s_object_toolTipIconBlank */
    void* ToolTipIcon_Ore;                   /* +0xc630 004eadc0 objectGlobs_toolTipIconOre (Image*) */
    unsigned int field_c634;                     /* +0xc634 004eadc4 s_global_animation_state */
    float field_c638;                        /* +0xc638 004eadc8 objectGlobs_f_4eadc8; 125-second timer */
};

extern struct LegoObject_Globs objectGlobs;

/* The fields only C units read, under the units' names (every build). */
#define objectGlobs_f_4dee0c objectGlobs.field_67c
#define objectGlobs_f_4e8444 objectGlobs.field_9cb4
#define s_lego_f_4e8448 objectGlobs.field_9cb8
#define objectGlobs_recordObjs ((LegoObjGB_DeferredRecord*)objectGlobs.recordObjs)
#define objectGlobs_recordObjsCount objectGlobs.recordObjsCount
#define objectGlobs_queuedAnimationFraction objectGlobs.field_c018
#define objectGlobs_f_4ea7c4 ((Lego_Object**)objectGlobs.field_c034)
#define s_object_f_4eabc4 objectGlobs.field_c434
#define s_object_f_4eabc8 objectGlobs.field_c438
#define s_object_toolTipIconAbilities objectGlobs.ToolTipIcon_Abilities
#define s_object_toolTipIcons objectGlobs.ToolTipIcon_Tools
#define s_object_toolTipIconBlank objectGlobs.ToolTipIcon_Blank
#define objectGlobs_toolTipIconOre (*(struct Image**)&objectGlobs.ToolTipIcon_Ore)
#define objectGlobs_f_4eadc8 objectGlobs.field_c638

/* The fields a C++ port file reads too, under the port's names (matching
 * build only). */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define s_pool_chunks objectGlobs.listSet
#define s_pool_record_head objectGlobs.freeList
#define s_terrain_sfx_grid objectGlobs.field_84
#define s_activity objectGlobs.activityName
#define s_terrain_neighbor_path_cost objectGlobs.field_670
#define s_terrain_path_depth objectGlobs.field_674
#define s_terrain_path_depth_count objectGlobs.field_678
#define s_pool_chunk_count objectGlobs.listCount
#define s_entity_state_flags objectGlobs.flags
#define s_activity_resource_slots objectGlobs.toolNullIndex
#define s_grid_count_second objectGlobs.field_6b4
#define s_task_counter_nonzero objectGlobs.field_6b4[0xe10]
#define s_grid_count_first objectGlobs.field_51b4
#define s_task_counter_zero objectGlobs.field_51b4[0xe10]
#define s_predug_coordinates objectGlobs.field_9cbc
#define s_type11_cell_coordinates objectGlobs.field_9d5c
#define s_predug_coordinate_count objectGlobs.field_9dac
#define s_type11_cell_coordinate_count objectGlobs.field_9db0
#define s_render_config_snapshot objectGlobs.field_c01c
#define s_idle100 objectGlobs.liveObjs
#define s_idle100_n objectGlobs.liveCount
#define s_ability_type_names objectGlobs.abilityName
#define s_global_animation_state objectGlobs.field_c634
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_GLOBS_H */
