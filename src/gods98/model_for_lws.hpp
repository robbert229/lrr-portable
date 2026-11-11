// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lws uses from model.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_MODEL_FOR_LWS_HPP
#define LRR_GODS98_MODEL_FOR_LWS_HPP

/* The reference keeps two adjacent 8-byte-stride path/record pair tables,
 * 0x1f40 bytes apart: 1000 entries each, scanned case-insensitively through
 * the shared pair-table helpers (FUN_00488730/FUN_00488790).  The primary
 * table (DAT_00540838, count DAT_005446bc) keys on the joined request path;
 * the secondary table (DAT_00542778, count DAT_005446c0) keys on the raw
 * path and is consulted under the fallback prefix (DAT_005446b8). */
#define MODEL_RESOURCE_CACHE_CAPACITY 1000u

typedef struct ModelResourcePair {
    char* key;
    void* value;
} ModelResourcePair;

/* Portable notification for a parsed-model cache hit. It is not the
 * FUN_00480900 source record: that routine expects a 13-dword resource
 * template with per-entry 0x70-byte renderer records, absent from Model. */
typedef void (*ModelResourceInstanceHook)(void* record, void* flags);

extern ModelResourcePair s_model_resource_primary[MODEL_RESOURCE_CACHE_CAPACITY];
extern ModelResourcePair s_model_resource_secondary[MODEL_RESOURCE_CACHE_CAPACITY];
extern const char* s_model_resource_fallback_prefix;
extern ModelResourceInstanceHook s_model_resource_instance_hook;

#endif /* LRR_GODS98_MODEL_FOR_LWS_HPP */
