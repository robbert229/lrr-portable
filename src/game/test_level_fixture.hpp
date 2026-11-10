// SPDX-License-Identifier: LGPL-3.0-or-later
// Shared helpers for native tests that drive level_load() on a synthetic
// fixture. level_load is now a faithful port of LevelLoad: it requires a PTL
// remap file, a TextureSet group (with an indexed-texture loader installed
// for the roof texture), a non-empty object list, and an active GameContext.
// Test-only header; never included by product code.
#ifndef LRR_TEST_LEVEL_FIXTURE_HPP
#define LRR_TEST_LEVEL_FIXTURE_HPP

#include <stdio.h>

#include "game/entity.hpp"
#include "game/game.hpp"
#include "gods98/resource.hpp"

/* Level-block config lines a fixture level needs in addition to its maps. */
#define TLF_LEVEL_KEYS                                                         \
    "    TextureSet TlfSet\n"                                                  \
    "    PTLFile Tlf.ptl\n"                                                    \
    "    OListFile Tlf.ol\n"

/* Root-group config lines (siblings of the level block). */
#define TLF_ROOT_GROUPS                                                        \
    "  TlfSet {\n"                                                             \
    "    SurfTextWidth 1\n"                                                    \
    "    SurfTextHeight 1\n"                                                   \
    "    MeshBaseName tlfmesh\n"                                               \
    "    TextureBaseName tlftex\n"                                             \
    "    RoofTexture tlfroof\n"                                                \
    "  }\n"

static inline void tlf_write_file(const char* dir, const char* name,
                                  const char* body)
{
    char path[512];
    FILE* fp;
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    fp = fopen(path, "w");
    if (fp != NULL) {
        fputs(body, fp);
        fclose(fp);
    }
}

/* Writes the PTL remap file and the smallest object list OListEntityRecordParse
 * accepts (a lone TVCamera counts as placed without creating an entity). */
static inline void tlf_write_aux_files(const char* dir)
{
    tlf_write_file(dir, "Tlf.ptl", "Lego {\n}\n");
    tlf_write_file(dir, "Tlf.ol",
        "LegoOl {\n"
        "  Cam {\n"
        "    type TVCamera\n"
        "    xPos 2\n"
        "    yPos 2\n"
        "    heading 0\n"
        "  }\n"
        "}\n");
}

static inline void* tlf_indexed_texture_load(const char* name, void* user)
{
    (void)name;
    return user; /* any non-NULL token: the roof texture "loaded" */
}
static inline void tlf_indexed_surface_release(void* handle, void* user)
{
    (void)handle;
    (void)user;
}

/* Installs the indexed-texture loader TextureSetConfigLoad needs for the roof
 * texture. */
static inline void tlf_install_texture_hooks(void)
{
    static int token;
    indexed_surface_resource_hooks_set(NULL, tlf_indexed_texture_load,
                                       tlf_indexed_surface_release, NULL,
                                       &token);
}

/* Initialises a GameContext and makes it the active one LevelLoad parses its
 * object list into. NOTE: game_context_init() itself installs its context as
 * the process-wide entity-pool/current-level owner, so a later block-local
 * context steals it; re-bind with entity_pool_callback_context_set() once it
 * goes out of scope. */
static inline void tlf_bind_context(GameContext* ctx)
{
    game_context_init(ctx);
    entity_pool_callback_context_set(ctx);
    tlf_install_texture_hooks();
}

#endif
