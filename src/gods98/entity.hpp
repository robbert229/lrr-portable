// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — entity visual assembly (the `.ae` config table).
 *
 * Reconstruction of the ACT branch (kind 4) of the entity-visual factory the
 * corpus calls FUN_00472cd0, plus the kind dispatch it front-ends
 * (FUN_00475930). The `.ae` table is the per-entity state→animation
 * mapping the game loads for every building/creature/vehicle at spawn time
 * (finding 12 §4, this module's rows verified over all 56 shipped `.ae`
 * files).
 *
 * Corpus map:
 *   FUN_00475930  entity_kind_lookup — split on ':', NOTEXTURE flag, match
 *                 against the 9-slot kind table initialized by FUN_00472710:
 *                 {NULL, MESH, FRAME, ANIM, ACT, -, -, LWS, LWO} (indices
 *                 0..8; 5/6 are zero and never match).
 *   FUN_00472cd0  ACT branch only: opens dot-path `<name>\<basename>.ae`
 *                 (the resource layer resolves it), BlockOpen, reads root
 *                 "SCALE", then iterates `Activities` children in document
 *                 order; each activity contributes one EntityActivity when
 *                 its group has a FILE (animation/scene base name). A
 *                 '!'-prefixed key is dropped when feature bit 0x400000
 *                 (the `-reduceanimation` cfg switch) is set, else processed
 *                 with the '!' stripped.
 *
 * Scene assembly (FUN_00475d10, FUN_00473370, FUN_004765c0 events + scale
 * application via the record vtable) is a documented seam: this module only
 * builds the table, mirroring how model.cpp ends at the parsed Model. The
 * exe drives each row's scene from <entity-path>\<FILE>.<ext> with the
 * state key, transcoef, trigger, sample, lwsfile and looping values recorded
 * here.
 *
 * Feature word: DAT_00505f84 (engine.hpp's EngineContext::feature). Startup
 * zeroes it; the cfg loader FUN_00477bf0 ORs 0x400000 for `reduceanimation`
 * (string "-reduceanimation" at 0x4aad04) — the only writer of that bit.
 */
#ifndef LEGORRR_ENTITY_HPP
#define LEGORRR_ENTITY_HPP

#include <stddef.h>
#include <stdint.h>

#include "block.hpp"

/* --- kind vocabulary (FUN_00475930 / FUN_00472710 table) ----------------- */
#define ENTITY_KIND_NULL  0
#define ENTITY_KIND_MESH  1
#define ENTITY_KIND_FRAME 2
#define ENTITY_KIND_ANIM  3
#define ENTITY_KIND_ACT   4
#define ENTITY_KIND_LWS   7
#define ENTITY_KIND_LWO   8
#define ENTITY_KIND_UNKNOWN (-1)

/* Feature bit that drops '!'-prefixed activities (DAT_00505f84 & 0x400000,
 * set by the `-reduceanimation` standard parameter). */
#define ENTITY_FEATURE_REDUCE_ANIM 0x400000u

/* Kind dispatch (corpus FUN_00475930). `kind` is a "LWO","LWS","ACT",...
 * word, optionally followed by ":NOTEXTURE" in the caller context; the
 * suffix sets `*notexture` (1 when present) and is stripped before the
 * table match. Returns the kind index (0..8) or ENTITY_KIND_UNKNOWN.
 * Matching is ASCII case-insensitive; a NULL/empty kind returns UNKNOWN. */
int entity_kind_lookup(const char* kind, int* notexture);

/* One activity of an assembly (document order, FILE-bearing rows only). */
typedef struct EntityActivity {
    char  state[64];   /* activity key ("Activity_Stand"), '!' stripped */
    char  name[64];    /* value token: target animation group ("Stand") */
    char  file[260];   /* FILE value (scene path base name, e.g. "Stand2") */
    char  sample[260];  /* SAMPLE value ("" when absent) */
    float transcoef;    /* TRANSCOEF float (default 0.0) */
    int   trigger;      /* TRIGGER int (default 0) */
    int   lwsfile;      /* LWSFILE bool: 1 true, 0 false, 2 unparsed/missing */
    int   looping;      /* LOOPING bool (same encoding) */
    int   reduce_anim;  /* key was '!'-prefixed (kept under default feature) */
} EntityActivity;

/* Parsed `.ae` table. `scale` is the root SCALE float (0.0 when absent —
 * the exe only applies a non-zero scale to its scenes). Activities are in
 * document order; rows whose group had no FILE are not present (the exe
 * makes no scene for them). */
typedef struct EntityAssembly {
    float        scale;
    size_t       count;
    EntityActivity* acts;  /* owned array, document order */
} EntityAssembly;

/* Open `<name>\<basename-of-name>.ae` through the resource layer (finding 01
 * resolution) and parse it. `name` is the entity's config-relative path
 * (e.g. "Buildings\\Barracks"); `root` is the block root token the paths are
 * queried under ("LegoRR"; corpus files root "Lego*" which `name*`-matches);
 * `feature` is the engine feature word (DAT_00505f84) used by the '!' gate.
 * Returns NULL on open/read/parse failure. */
EntityAssembly* entity_assembly_open(const char* name, const char* root,
                                     uint32_t feature);

/* Parse an opened block as an `.ae` assembly (same rules as above).
 * Returns NULL when the `Activities` group is missing or empty. */
EntityAssembly* entity_assembly_parse(BlockFile* bf, const char* root,
                                     uint32_t feature);

void entity_assembly_free(EntityAssembly* ea);

#endif /* LEGORRR_ENTITY_HPP */
