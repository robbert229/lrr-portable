// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — block-text config DB (`Lego*` files, finding 12).
 *
 * Reconstruction of the corpus's BlockOpen/BlockFindPath/BlockGetValue
 * family (FUN_00478e60 / FUN_004792e0 / FUN_004790b0 / FUN_00478ff0 /
 * FUN_00478f50). One generic scanner + a path-lookup API serve every text
 * data file the engine reads: Lego.cfg (level/reward config — findings
 * 07/09/22), entity animation files (.ae), level scripts (.ptl), object
 * lists (.ol), nav files (.npl/.nrn).
 *
 * Format (the `Lego*` magic token is data, never checked):
 *   name { ... }   brace-nested, whitespace-separated tokens;
 *   `;` starts a line comment (cleared until \n).
 * A `name {…}` record's "value" is the next same-depth token; a `{` opens
 * children. Lookup is path-based: each whitespace-separated segment of
 * "A B C" matches a name at the corresponding nesting level (finding 12 §2).
 *
 * The shipped Lego.cfg wraps everything in one top-level `Lego {…}`, so
 * paths are written *relative to that wrapper* ("Level01 blocksize",
 * "VehicleTypes", "Levels\Level01 Reward Enable"…). config_find handles the
 * wrapper: a path starting with a group that is a direct child of the file's
 * single top-level record resolves by walking past the wrapper.
 *
 * Discipline (ADR 0003/0004): plain C-shaped C++, no OS headers, no STL.
 */
#ifndef LEGORRR_CONFIG_HPP
#define LEGORRR_CONFIG_HPP

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle types. Nodes are owned by their Db; pointers stay valid
 * until config_close(db). */
typedef struct ConfigDb   ConfigDb;
typedef struct ConfigNode ConfigNode;

/* Parse `resource_name` (resolved through the resource layer: loose file
 * or mounted wad). Returns NULL when the resource can't be opened. */
ConfigDb* config_open(const char* resource_name);

/* Release the Db and every node it owns. */
void config_close(ConfigDb* db);

/* Number of records at the "query root" (the wrapper-past level — see the
 * header note). */
int config_roots(const ConfigDb* db);
const ConfigNode* config_root(const ConfigDb* db, int i);

/* --- node inspection ------------------------------------------------ */
const char* config_node_name(const ConfigNode* n);
const char* config_node_value(const ConfigNode* n); /* NULL if none */
int         config_node_child_count(const ConfigNode* n);
const ConfigNode* config_node_child(const ConfigNode* n, int i);

/* --- path lookup ----------------------------------------------------- */
/* Find the record reached by the whitespace-separated path "A B C",
 * resolving past the single top-level wrapper (NULL when not found). */
const ConfigNode* config_find(ConfigDb* db, const char* path);

/* Path lookup that returns the node's *value*, or NULL. */
const char* config_value(ConfigDb* db, const char* path);

/* Typed getters mirroring the corpus's FUN_0048db70 (atoi) / FUN_0048d980
 * (atof) applied to the value, with the corpus's default-string behaviour:
 * a missing node falls back to `dflt`. TRUE/FALSE map to 1/0 as the game's
 * typed getter does. */
long  config_int(ConfigDb* db, const char* path, long dflt);
float config_float(ConfigDb* db, const char* path, float dflt);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_CONFIG_HPP */
