// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail (finding 591): the native port's WAD reader, which #496 relocated
 * into gods98/wad.cpp. Wad.c (G98-030) is now gods98/wad.c in original shape;
 * native code keeps calling these, under the port's names, over the port's
 * mounted archives (resource.cpp's s_wads) and its Resource records. They
 * carry no annotations: the unit owns the addresses. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resource.hpp"
#include "heap.hpp"
#include "record.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "scene_pool.hpp"
#include <stddef.h>
#include <stdint.h>
#include "port_loop_ports.hpp"
#include "../gods98/block.hpp"
#include "util.hpp"
#include "../gods98/engine.hpp"
#include "resource_for_wad.hpp"

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static Resource* ares_open_wad(int wad_idx, int entry_idx, const char* name);

/* WwadArchiveIndexLoad (FUN_0048b4e0): open and index one archive into the
 * next archive record. The port guards the corpus's fixed record array. */
int wwad_archive_index_load(const char* path)
{
    ResourceWad* wad;
    FILE* fp;
    if (path == NULL || s_wad_count >= RESOURCE_MAX_WADS) {
        return -1;
    }
    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }
    wad = &s_wads[s_wad_count];
    memset(wad, 0, sizeof(*wad));
    wad->fp = fp;
    if (!ares_parse(fp, wad)) {
        free(wad->names);
        free(wad->entries);
        memset(wad, 0, sizeof(*wad));
        fclose(fp);
        return -1;
    }
    snprintf(wad->path, sizeof(wad->path), "%s", path);
    ++s_wad_count;
    return s_wad_count - 1;
}

/* WadEntryNameLookup: case-insensitive scan of one mounted archive's names;
 * return its entry index or -1. Bounds checks are port safeguards. */
int resource_wad_lookup(int wad_idx, const char* name)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count || name == NULL) {
        return -1;
    }
    ResourceWad* w = &s_wads[wad_idx];
    for (uint32_t i = 0; i < w->entry_count; ++i) {
        const char* cand = w->names + w->entries[i].name_off;
        if (strcasecmp_lite(cand, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/* Partial WadPathOpen (FUN_0048bf70): scan mounted archives from newest to
 * oldest, resolve the entry, and return its resource handle. The original
 * opens the entry into a fixed stream slot immediately; this port reads its
 * payload lazily through resource_read. */
Resource* ares_open_wad_path(const char* name, int mode)
{
    for (int w = s_wad_count - 1; w >= 0; --w) {
        int e = resource_wad_lookup(w, name);
        if (e >= 0) {
            Resource* r = ares_open_wad(w, e, name);
            if (r != NULL) {
                r->mode = mode;
                return r;
            }
            /* WadPathOpen continues to the next older mounted archive when
             * opening a matching entry fails (for example, slot allocation
             * failure). Keep that fallback if this handle allocation fails. */
        }
    }
    return NULL;
}

void fixed_record_slot_resource_release(int slot)
{
    /* The corpus scales the index shl 4 and strides blindly (the tests only
     * release in-range slots) and clears ONLY the flag — the stale pointer
     * is unobservable because the get path checks the flag first. */
    if (s_fixed_slots[slot].in_use != 0) {
        engine_heap_free(fixed_record_slot_pointer_get(slot));
        s_fixed_slots[slot].in_use = 0;
    }
}

/* The mapped slot getter follows the retail unchecked 16-byte table walk;
 * the public helper keeps the portable bounds check. */
void* fixed_record_slot_resource_get_target(int slot)
{
    return s_fixed_slots[slot].in_use != 0
        ? fixed_record_slot_pointer_get(slot) : NULL;
}

/* WadMemoryStreamBufferGet (FUN_0048c030): the corpus wrapper routes the
 * slot through the fixed-record slot reader (FUN_0048bff0) and drops the
 * result — the observable half is the slot reader's resolution. */
void wad_memory_stream_buffer_get(int slot)
{
    (void)fixed_record_slot_resource_get_target(slot);
}
/* Open an entry inside mounted wad `wad_idx` (no payload yet). */
static Resource* ares_open_wad(int wad_idx, int entry_idx, const char* name)
{
    Resource* r = (Resource*)calloc(1, sizeof(Resource));
    if (r == NULL) {
        return NULL;
    }
    r->name = ares_strdup(name);
    if (r->name == NULL) {
        free(r);
        return NULL;
    }
    r->branch    = RESOURCE_WAD_BRANCH;
    r->wad_idx   = wad_idx;
    r->entry_idx = entry_idx;
    r->size      = -1;
    return r;
}

#if defined(_MSC_VER) && _MSC_VER < 1200
/* The matching build only. There files.h gives files.c the port's file-layer
 * names, so the port's file opens read Wad.c's tables (gods98/wad.c), while
 * resource_open still reads the port's s_wads. frameloop.cpp mounts one of
 * them; these mount the same archives into the other, as the four Wad.c
 * adapters did before Wad.c's conversion (finding 591). Native builds and
 * the reference build keep a single table and do not compile these. */
#include "wad.h"

/* After File_Initialise (resource_mount_startup_target), which mounts
 * <prefix>0.wad to <prefix>9.wad into Wad.c's tables: the same names into
 * the port's. */
void wad_port_mount_startup_archives(const char* prefix)
{
    char path[RESOURCE_PATH_MAX];
    int i;
    if (prefix == NULL || s_wad_count != 0) return;
    for (i = 0; i < RESOURCE_MAX_WADS; ++i) {
        snprintf(path, sizeof(path), "%s%i.wad", prefix, i);
        (void)wwad_archive_index_load(path);
    }
}

/* After resource_mount, which mounts into the port's tables: each archive
 * into Wad.c's, in the same order. */
void wad_port_mount_unit_archives(void)
{
    int i;
    for (i = 0; i < s_wad_count; ++i) {
        (void)wad_load(s_wads[i].path);
    }
}
#endif
