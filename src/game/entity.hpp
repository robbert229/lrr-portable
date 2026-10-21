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

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENTITY_HPP */
