// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what wad uses from resource.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_RESOURCE_FOR_WAD_HPP
#define LRR_GODS98_RESOURCE_FOR_WAD_HPP

#define RESOURCE_MAX_WADS 10

#define RESOURCE_PATH_MAX 4096

typedef struct WadEntry {
    uint32_t flags;
    uint32_t orig;    /* uncompressed length */
    uint32_t stored;  /* length on disk */
    uint32_t off;     /* absolute file offset of the payload */
    uint32_t name_off;/* byte offset of the name inside RESOURCE_wads[].names */
} WadEntry;

typedef struct ResourceWad {
    FILE*     fp;         /* binary handle, kept open for reads */
    char      path[RESOURCE_PATH_MAX];
    uint32_t  file_size;
    uint32_t  entry_count;
    char*     names;      /* copied name blob (archive-relative paths) */
    WadEntry* entries;
} ResourceWad;

/* --- the fixed-record slot resources (see resource.hpp) -------------------- */
#define FIXED_RECORD_SLOTS 16

/* The fixed-record slot band: sixteen 16-byte records {resource@+0x00,
 * in-use flag@+0x04} at DAT_005756a0 (all-zero image bytes; the corpus
 * scales the index shl 4 and strides blindly). The +0x08/+0x0c words
 * belong to the unported reader (FUN_0048c010) — pad only. */
typedef struct FixedRecordSlot {
    uint32_t resource;  /* +0x00: reference pointer word */
    uint32_t in_use;    /* +0x04 */
    uint8_t  pad[8];    /* +0x08..0x0f */
} FixedRecordSlot;

#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
#define fixed_record_slot_pointer_get(slot) s_fixed_slot_resources[(slot)]
#else
#define fixed_record_slot_pointer_get(slot) \
    ((void*)(uintptr_t)s_fixed_slots[(slot)].resource)
#endif

extern ResourceWad s_wads[RESOURCE_MAX_WADS];
extern int s_wad_count;
char* ares_strdup(const char* s);
int ares_parse(FILE* fp, ResourceWad* out);
extern FixedRecordSlot s_fixed_slots[FIXED_RECORD_SLOTS];
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
extern void* s_fixed_slot_resources[FIXED_RECORD_SLOTS];
#endif

#endif /* LRR_GODS98_RESOURCE_FOR_WAD_HPP */
