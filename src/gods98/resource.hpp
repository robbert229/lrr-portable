// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — resource loader scaffold (WAD mount + dispatch).
 *
 * Reconstruction of the file/resource layer the corpus calls FUN_0047f6e0 /
 * FUN_0047f130 / FUN_0048bda0 / FUN_0048bf70 / FUN_0048be40 (finding 01).
 * Handles both branches documented there:
 *
 *   WAD branch — mount `<dir>/<prefix>%i.wad` (i = 0..9), the prefix being
 *       the exe basename minus ".EXE" (the find-20 correction of finding 02):
 *       "LegoRR" -> LegoRR0.wad / LegoRR1.wad. Parse the WWAD header + TOC,
 *       and open entries by case-insensitive name. Later mounts (higher i)
 *       shadow earlier ones when resolving unqualified names.
 *   STD branch — a plain loose file opened from the working directory (the
 *       corpus's `Data\...` path; the drive-letter CD fallback is out of
 *       scope until a mount story exists).
 *
 * Entry payload expansion (finding 01): recorded for entries whose flag word
 * has bit 1 set — a small header (4-byte magic, type byte at [3], byte-swapped
 * length at [4], payload from +0x12). Shipped wads never set the bit
 * (flags == 1 everywhere, stored == original), so types 1/2 (FUN_0049c8e0 /
 * FUN_0049ca60) remain dormant and are reported as not implemented.
 *
 * Discipline (ADR 0003/0004): plain C, stdio only (portable across the
 * native/winapi backends), no DirectX/WinAPI headers below the seam.
 */
#ifndef LEGORRR_RESOURCE_HPP
#define LEGORRR_RESOURCE_HPP

#include <cstdint>
#include <cstddef>

/* --- branch selectors (corpus dispatcher: 0 = WAD, 1 = STD) -------------- */
#define RESOURCE_WAD_BRANCH 0
#define RESOURCE_STD_BRANCH 1

/* open modes */
#define RESOURCE_AUTO     0 /* STD file if present, else WAD lookup */
#define RESOURCE_WAD_ONLY 1 /* archive member only */
#define RESOURCE_STD_ONLY 2 /* loose file only */

/* An opened resource. `data` is populated by resource_read() and is owned by
 * the handle (freed by resource_free). `mode` is the open() mode requested. */
typedef struct Resource {
    uint8_t* data;
    int32_t  size;
    int32_t  branch;   /* RESOURCE_WAD_BRANCH or RESOURCE_STD_BRANCH */
    int32_t  wad_idx;  /* mounted wad index, or -1 */
    int32_t  entry_idx;/* TOC entry index, or -1 */
    int32_t  mode;     /* as passed to resource_open */
    char*    name;     /* copy of the requested name */
} Resource;

/* Mount every `<dir>/<prefix>%i.wad` for i = 0..9 that exists and parses.
 * Returns the number of wads mounted (0 => none; error message -> stderr).
 * Only the first successful parse is required to be a valid WWAD. */
int resource_mount(const char* dir, const char* prefix);

/* Number of wads currently mounted. */
int resource_mounted_count(void);

/* Close and free every mounted wad (state reset, used between test runs). */
void resource_unmount_all(void);

/* Case-insensitive lookup of `name` inside one mounted wad (linear scan over
 * the archive-relative path table, ASCII fold only). Returns the TOC entry
 * index or -1. */
int resource_wad_lookup(int wad_idx, const char* name);

/* TOC introspection used by tests/validators. */
uint32_t resource_wad_entry_count(int wad_idx);
uint32_t resource_wad_entry_stored(int wad_idx, int entry_idx);

/* Open a resource by name. mode selects the branch policy (see defines).
 * Returns a handle or NULL on failure. */
Resource* resource_open(const char* name, int mode);

/* Read the payload into handle->data (lazy; no-op if already read or NULL).
 * Returns the payload size, or -1 on failure (dormant codec / read error). */
int32_t resource_read(Resource* r);

/* Release the handle and its payload. */
void resource_free(Resource* r);

#endif /* LEGORRR_RESOURCE_HPP */