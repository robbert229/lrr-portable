// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-030 Wad.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-030.csv. */
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
#include "game/startup_config_for_wad.hpp"

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static Resource* ares_open_wad(int wad_idx, int entry_idx, const char* name);

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0048b4a0
int buffered_char_stream_read_to_string(void* stream, char* output)
{
    char* cursor = output;
    int value;
    if (output == NULL || s_buffered_char_read_hook == NULL) return 0;
    value = s_buffered_char_read_hook(s_buffered_char_read_context, stream);
    while (value != 0) {
        *cursor++ = (char)value;
        value = s_buffered_char_read_hook(s_buffered_char_read_context,
                                         stream);
    }
    *cursor = '\0';
    return 1;
}
} // namespace unported_ledger_stubs

/* WwadArchiveIndexLoad (FUN_0048b4e0): open and index one archive into the
 * next archive record. The port guards the corpus's fixed record array. */
// FUNCTION: LEGORR 0x0048b4e0
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

// FUNCTION: LEGORR 0x0048bcf0
uint32_t wad_entry_length_get(int wad_idx, int entry_idx)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count || entry_idx < 0
        || (uint32_t)entry_idx >= s_wads[wad_idx].entry_count) {
        return 0;
    }
    return s_wads[wad_idx].entries[entry_idx].stored;
}
/* WadEntryNameLookup: case-insensitive scan of one mounted archive's names;
 * return its entry index or -1. Bounds checks are port safeguards. */
// FUNCTION: LEGORR 0x0048bda0
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
// FUNCTION: LEGORR 0x0048bf70
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

// FUNCTION: LEGORR 0x0048bfc0
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
// FUNCTION: LEGORR 0x0048bff0
void* fixed_record_slot_resource_get_target(int slot)
{
    return s_fixed_slots[slot].in_use != 0
        ? fixed_record_slot_pointer_get(slot) : NULL;
}

/* WadStreamLengthGet (FUN_0048c010) resolves the two WAD indices held by the
 * stream slot and performs the reference's indexed length lookup. */
// FUNCTION: LEGORR 0x0048c010
void wad_stream_length_get(int slot)
{
    Resource* resource =
        (Resource*)fixed_record_slot_resource_get(slot);
    (void)wad_entry_length_get(resource->wad_idx, resource->entry_idx);
}

/* WadMemoryStreamBufferGet (FUN_0048c030): the corpus wrapper routes the
 * slot through the fixed-record slot reader (FUN_0048bff0) and drops the
 * result — the observable half is the slot reader's resolution. */
// FUNCTION: LEGORR 0x0048c030
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
