// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the GAME-023 functions (game/entity.cpp)
 * that original-shape units call. Shared C++ headers do not include it. */
#ifndef LEGORRR_GAME_ENTITY_H
#define LEGORRR_GAME_ENTITY_H

#include "gods98/config.h"
#include "gods98/model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MeshLOD_Load (FUN_0044a7d0) with the original's arguments: the meshes
 * of level `lod` for `gameName` from the `.ae` config. The port maps
 * 0x0044a7d0 to a ledger stub with another contract, so this is a port
 * adapter: it loads nothing and returns NULL (finding 549). */
ModelEntryRecord* mesh_lod_load(BlockFile* config, const char* gameName,
                                const char* filename, int lod,
                                unsigned int count);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ENTITY_H */
