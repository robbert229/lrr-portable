// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-035 GAME-035: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-035.csv. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <locale.h>
#include "crt/crt.hpp"
#include "gods98/resource.hpp"
#include "gods98/heap.hpp"
#include "gods98/view.hpp"
#include "effects.hpp"
#include "port_loop_ports.hpp"
#include "util.hpp"
#include "util_for_level_id_remap.hpp"
#include "record.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"
#include <limits.h>
#include "record_for_level_id_remap.hpp"

#if defined(_MSC_VER) && _MSC_VER < 1200
#include "gods98/config.h"
#include "message_unit.h"

/* LevelIdentifierRemapTableLoad (FUN_0045d7e0) in the original's shape:
 * each item of the game's block in `fileName` maps one physical object name
 * to another, both looked up with Message_LookupPhysicalName. Lego.c's
 * Lego_LoadLevel calls it through lego_unit_imports.h's link name (#562,
 * finding 642). The original's table and count are 0x555be0 and 0x555d20;
 * the port keeps them in util.cpp. */
// FUNCTION: LEGORR 0x0045d7e0
extern "C" int level_remap_table_load(const char* fileName,
                                      const char* gameName)
{
    BlockFile* config;
    BlockRec* item;
    uint32_t* pair;

    s_level_remap_count = 0;
    if ((config = block_open(fileName)) != NULL) {
        for (item = block_subgroup(config, gameName); item != NULL;
             item = block_next_equal_key(item)) {
            pair = s_level_remap_pairs[s_level_remap_count++];
            pair[0] = message_lookup_physical_name(item->key);
            pair[1] = message_lookup_physical_name(item->value);
        }
        block_close(config);
        return 1;
    }
    return 0;
}
#else
namespace unported_ledger_stubs {
// LevelIdentifierRemapTableLoad
// PORT-ONLY: the native table installer; the matching build has the original-shape 0x0045d7e0 above.
void ledger_stub_0045d7e0_LevelIdentifierRemapTableLoad(
    const uint32_t* identifier_pairs, uint32_t pair_count)
{
    level_identifier_remap_table_install(identifier_pairs, pair_count);
}
} // namespace unported_ledger_stubs
#endif

// FUNCTION: LEGORR 0x0045d870
void command_entity_reference_remap(uint32_t* entity_reference)
{
    unsigned int i;
    if (entity_reference == NULL) return;
    for (i = 0; i < s_level_remap_count; ++i) {
        if (*entity_reference == s_level_remap_pairs[i][0]) {
            *entity_reference = s_level_remap_pairs[i][1];
            return;
        }
    }
}

// FUNCTION: LEGORR 0x0045d8a0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void object_field14_set(void* record, uint32_t value)
{
    __asm {
        fld dword ptr [esp + 8]
        mov eax, dword ptr [esp + 4]
        fstp dword ptr [eax + 14h]
        ret
    }
}
#else
void object_field14_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x14, &value, sizeof(value));
}
#endif

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original RadarMap_SetZoom (FUN_0045d8a0) takes the zoom as a float; the
 * port's object_field14_set takes its bits. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (RadarMap_SetZoom 0045d8a0 -> object_field14_set)
extern "C" void radar_map_zoom_set_adapter(void* radarMap, float zoom)
{
    uint32_t bits;
    memcpy(&bits, &zoom, sizeof(bits));
    object_field14_set(radarMap, bits);
}
