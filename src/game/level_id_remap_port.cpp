// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-035 PTL.c's port tail (finding 652): the port's models of the
 * unit's functions, on util.cpp's table. Unannotated: the original-shape
 * unit is ptl.c (ADR 0005), which the C callers reach by aliases in their
 * imports headers. Natively these models are the live ones. Relocated by
 * tools/relocate_unit.py (ADR 0006); body hashes:
 * docs/coverage/relocations/GAME-035.csv. */
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

namespace unported_ledger_stubs {
/* The port's model of PTL_Initialise (0045d7e0): lego_port.cpp parses the
 * PTLFile itself and installs the pairs. */
// PORT-ONLY: the native table installer; ptl.c has the original-shape 0x0045d7e0.
void ledger_stub_0045d7e0_LevelIdentifierRemapTableLoad(
    const uint32_t* identifier_pairs, uint32_t pair_count)
{
    level_identifier_remap_table_install(identifier_pairs, pair_count);
}
} // namespace unported_ledger_stubs

/* The port's model of PTL_TranslateEvent (0045d870), on util.cpp's table. */
// PORT-ONLY: the native model; ptl.c has the original-shape 0x0045d870.
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

/* The port's model of RadarMap_SetZoom (0045d8a0): it takes the float's
 * bits. */
// PORT-ONLY: the native model; ptl.c has the original-shape 0x0045d8a0.
void object_field14_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x14, &value, sizeof(value));
}

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
