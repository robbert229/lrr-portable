// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — entity table + `.ol` object list (findings 10/07).
 *
 * `EntityPointer` (corpus engine-region.c, 0x42ed70) keeps four typed
 * entity arrays: type 1 = vehicles (stride 0x1ec), type 2 = minifigures
 * (stride 0x74 — DAT_005561d8), type 3 (stride 0x74 — 005561dc),
 * type 4 = buildings/objects (stride 0x14c — 005561e0); the `.ol` object
 * list the LevelLoad `olistfile` key names (`M01.ol`) is a block-text file
 * whose top-level `name { … }` records describe placed objects.
 *
 * The corpus's `.ol` consumer (FUN_0042e020) is not decompiled, so this is
 * the *interface* layer: it parses the block structure — one object per
 * top-level record with a `Type` integer and optional `x`/`y`/`z`/`Rot`
 * children — and places them into the typed tables. The field mapping is
 * best-effort until a shipped `.ol` is cross-checked (deferred, finding 07
 * §"Still open": "`.ol`/`.nrn`/`.ptl` resources consumed where?").
 */
#ifndef LEGORRR_GAME_ENTITY_HPP
#define LEGORRR_GAME_ENTITY_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Entity type constants (corpus EntityPointer mapping). */
#define ENTITY_TYPE_VEHICLE  1
#define ENTITY_TYPE_MINIFIG  2
#define ENTITY_TYPE_OBJ      3
#define ENTITY_TYPE_BUILDING 4

/* A placed-object record produced by the `.ol` loader. */
typedef struct ObjectPlacement {
    char  name[64];
    int   type;
    float x, y, z;
    float rot;
} ObjectPlacement;

/* Allocate a slot of `type`; returns the slot index or -1 when the table is
 * full or type is out of 1..4. */
int entity_create(GameContext* ctx, int type, const char* name,
                  float x, float y, float z, float rot);

/* Corpus strides by type (EntityPointer, finding 10 §"Kind registry" note). */
int  entity_stride(int type);
int  entity_count(GameContext* ctx, int type);
const EntitySlot* entity_at(GameContext* ctx, int type, int index);

/* Load the level's `.ol` object list (`olistfile` key) from `cfg` into the
 * entity table. Returns the number of objects placed, or -1 when the
 * resource is missing/unparsable. */
int object_list_load(GameContext* ctx, BlockFile* cfg, const char* level_name);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENTITY_HPP */
