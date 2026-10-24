// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — entity table + `.ol` object list (findings 10/07/12).
 *
 * `EntityPointer` (corpus engine-region.c, 0x42ed70) keeps four typed
 * entity arrays: type 1 = vehicles (stride 0x1ec), type 2 = minifigures
 * (stride 0x74 — DAT_005561d8), type 3 (stride 0x74 — 005561dc),
 * type 4 = buildings/objects (stride 0x14c — 005561e0); the `.ol` object
 * list the LevelLoad `OListFile` key names (`M01.ol`) is a block-text file
 * whose top-level `name { … }` records describe placed objects.
 *
 * The corpus's `.ol` consumer (`FUN_0042e020`) and its type resolver
 * (`FUN_0042e8b0` = ObjectTypeByName) are decompiled and mirrored here: the
 * *grammar* is corpus-verified by `entity_test --real` against all 44
 * shipped object lists (root `Lego*`, one record per object named
 * `Object1..` / other names, fields `type <name>`, `xPos`, `yPos`,
 * `heading`, `health` (default 100), optional `driving <ref>`; `Model`
 * appears nowhere in the shipped data and is kept as the port's extension
 * key), and the *codes* are decompile-verified — 1..4 from the shipped
 * `Lego.cfg` `…Types` groups, 5..19 special kinds, TVCamera −1, 0 = not
 * registered.
 */
#ifndef LEGORRR_GAME_ENTITY_HPP
#define LEGORRR_GAME_ENTITY_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Entity type constants (ObjectTypeByName FUN_0042e8b0 — decompile-verified).
 * 1..4 are the four typed tables (the `.ol`-placeable kinds); 5..19 are the
 * special-object kinds the corpus stores in per-kind singletons, which the
 * port groups under the shared special table. -1 = TVCamera (follow-camera
 * branch, no entity); 0 = unregistered. */
#define ENTITY_TYPE_VEHICLE  1
#define ENTITY_TYPE_MINIFIG  2
#define ENTITY_TYPE_OBJ      3   /* creatures/general objects (RockMonsterTypes) */
#define ENTITY_TYPE_BUILDING 4
#define ENTITY_TYPE_SPECIAL  5   /* first code of the special-object range */

/* A placed-object record produced by the `.ol` loader. */
typedef struct ObjectPlacement {
    char  name[64];
    int   type;          /* corpus code (1..19, -1 camera)           */
    char  type_name[24]; /* .ol `type` name (e.g. "SmallDigger")     */
    float x, y, z;       /* xPos, yPos, 0                            */
    float rot;           /* alias of heading                         */
    float heading;       /* .ol `heading` (degrees)                  */
    int   health;        /* .ol `health` (default 100)               */
    char  driving[32];   /* .ol `driving` target object name, or ""  */
    char  model[128];    /* optional `Model` key (port extension)    */
} ObjectPlacement;

/* Allocate a slot of `type`; returns the slot index or -1 when the table is
 * full or type is out of 1..4. */
int entity_create(GameContext* ctx, int type, const char* name,
                  float x, float y, float z, float rot);

/* Corpus strides by type (EntityPointer, finding 10 §"Kind registry" note). */
int  entity_stride(int type);
int  entity_count(GameContext* ctx, int type);
const EntitySlot* entity_at(GameContext* ctx, int type, int index);

/* Load the level's `.ol` object list (`OListFile` key) from `cfg` into the
 * entity table. Returns the number of objects placed, or -1 when the
 * resource is missing/unparsable. */
int object_list_load(GameContext* ctx, BlockFile* cfg, const char* level_name);

/* Load a `.ol` object list by resource path directly (the grammar half of
 * object_list_load, for probes/fixtures that already know the path; the
 * level wrapper resolves `OListFile` then calls this). */
int object_list_load_file(GameContext* ctx, const char* olist);

/* Attach a model to a slot by resource path (`.ol Model` key). The first
 * load per path goes through `model_open`; later loads reuse a process-
 * lifetime cache (the demo never frees models — documented). Empty path
 * clears. */
void entity_set_model(EntitySlot* e, const char* path);

/* The parsed LWO Model* for a slot (NULL when none/load failed). */
const void* entity_model(const EntitySlot* e);

/* --- entity record accessors (finding 457: the corpus +0x334 record) -----
 * Corpus families FUN_00469cb0 … FUN_0046a310 read fixed fields from the
 * entity's selected stats record (*(entity + 0x334)). The port mirrors that
 * record as EntitySlot.stats_record; NULL resolves to a zero template,
 * matching the shipped static templates (the Stats configuration loader
 * that fills real records is unported — reads
 * therefore default to 0/0.0f, a documented TODO(corpus) deviation). The
 * corpus's float fields return long double (x87 float10); the port hands
 * back the plain float stored in the record. Offsets are record-relative;
 * names are offset-preserving where semantics are unproved. */

#define ENTITY_STATS_RECORD_SIZE 0x150 /* the corpus record stride */

float entity_stats_float_at20(const EntitySlot* e);   /* FUN_00469cb0 */
float entity_stats_float_at24_indexed(const EntitySlot* e, int index);
                                      /* FUN_00469cc0 (terrain-speed array) */
float entity_stats_float_atd4(const EntitySlot* e);   /* FUN_00469ce0 */
float entity_stats_float_at9c(const EntitySlot* e);   /* FUN_00469d00 */
float entity_stats_float_atd8(const EntitySlot* e);   /* FUN_00469d20 */
float entity_stats_float_at6c(const EntitySlot* e);   /* FUN_00469d40 */
float entity_stats_float_at74(const EntitySlot* e);   /* FUN_00469d50 */
float entity_stats_float_at78(const EntitySlot* e);   /* FUN_00469d60 */
float entity_stats_float_ate8(const EntitySlot* e);   /* FUN_00469d70 */
float entity_stats_float_at70(const EntitySlot* e);   /* FUN_00469d90 */
void* entity_stats_field_at_a0(const EntitySlot* e);  /* FUN_00469da0: the
                                         * record+0xa0 address itself */
float entity_stats_float_at7c(const EntitySlot* e);   /* FUN_00469db0 */
float entity_stats_float_at80(const EntitySlot* e);   /* FUN_00469dc0 */
float entity_energy_decay_rate_get(const EntitySlot* e); /* FUN_00469de0 (+84) */
float entity_stats_float_atdc(const EntitySlot* e);   /* FUN_00469e20 */
uint32_t entity_stats_word_atac(const EntitySlot* e); /* FUN_00469e40 */
uint32_t entity_stats_word_get(const EntitySlot* e);  /* FUN_00469e60 (+144) */
uint32_t entity_state_flags_get(const EntitySlot* e); /* FUN_00469e80 (+148) */
uint32_t entity_capability_flags_get(const EntitySlot* e); /* FUN_00469ea0 (+14c) */
float entity_stats_float_at88(const EntitySlot* e);   /* FUN_00469f20
                                         * (the "screen shake" reader) */
uint32_t entity_stats_word_at_c0_or_c4(const EntitySlot* e, int selector);
                                      /* FUN_00469f90: +c4 if selector
                                       * nonzero, else +c0 */
float entity_stats_float_at8c(const EntitySlot* e);   /* FUN_00469fe0 */
float entity_stats_float_at90(const EntitySlot* e);   /* FUN_0046a000 */
float entity_stats_float_at98(const EntitySlot* e);   /* FUN_0046a020 */
float entity_stats_float_at94(const EntitySlot* e);   /* FUN_0046a040 */
float entity_stats_float_atf4(const EntitySlot* e);   /* FUN_0046a080 */
float entity_speed_component_get(const EntitySlot* e); /* FUN_0046a0a0 (+f8) */
float entity_stats_float_at130(const EntitySlot* e);  /* FUN_0046a170 */
float entity_stats_float_at134(const EntitySlot* e);  /* FUN_0046a190 */
float entity_stats_float_at120(const EntitySlot* e);  /* FUN_0046a1b0 */
float entity_stats_float_at128(const EntitySlot* e);  /* FUN_0046a1d0 */
float entity_stats_float_at124(const EntitySlot* e);  /* FUN_0046a1f0 */
uint32_t entity_stats_word_at13c(const EntitySlot* e); /* FUN_0046a210 */
float entity_stats_float_at12c(const EntitySlot* e);  /* FUN_0046a230 */
float entity_stats_float_at138(const EntitySlot* e);  /* FUN_0046a250 */
float entity_collision_radius_get(const EntitySlot* e); /* FUN_0046a270 (+e4) */
float entity_stats_float_at114(const EntitySlot* e);  /* FUN_0046a290 */
float entity_stats_float_at118(const EntitySlot* e);  /* FUN_0046a2b0 */
float entity_stats_float_at11c(const EntitySlot* e);  /* FUN_0046a2d0 */
float entity_stats_float_at110(const EntitySlot* e);  /* FUN_0046a2f0 */
float entity_stats_float_at10c(const EntitySlot* e);  /* FUN_0046a310 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENTITY_HPP */
