// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-035: PTL.c, the level's message translation table (a level's
 * PTLFile), rebuilt in original shape (ADR 0005, finding 652). Text
 * 0045d7e0-0045d8b0; its state is the common ptlGlobs (00555be0-00555d24).
 *
 * PTL_Initialise reads the game's block of the PTLFile that Lego.c's
 * Lego_LoadLevel names: each item maps one message type to another, both
 * looked up by name with Message_LookupPhysicalName. Message_Update then
 * passes each queued event through PTL_TranslateEvent before it dispatches
 * the queue, so a level can make one action post another (or none).
 *
 * 0045d8a0 stores a radar map's zoom (RadarMap_SetZoom: Lego.c passes the
 * level's RadarMap and legoGlobs' zoom). It reads none of RadarMap.c's
 * data, so finding 606 leaves it at the end of this object, like a header
 * inline.
 *
 * The port's models stay in level_id_remap_port.cpp, unannotated, on the
 * port's table in util.cpp: natively they are the live ones. The C callers
 * reach the functions here by aliases in lego_unit_imports.h and
 * message_unit_imports.h. */
#include "ptl.h"
#include "gods98/config.h"
#include "message_unit.h"
#include "radarmap.h"

#define PTL_MAXPROPERTIES 40

/* One translation: events of fromType become toType. */
typedef struct PTL_Property {
    unsigned int fromType;  /* +0x0 Message_Type */
    unsigned int toType;    /* +0x4 Message_Type */
} PTL_Property;

/* PTL.c's state, 0x144 bytes: a common symbol in the original. */
typedef struct PTL_Globs {
    PTL_Property table[PTL_MAXPROPERTIES];  /* +0x000 */
    unsigned int count;                     /* +0x140 */
} PTL_Globs;

// GLOBAL: LEGORR 0x00555be0
PTL_Globs ptlGlobs;

/* PTL_Initialise: load the translations of the game's block in `fname`. */
// FUNCTION: LEGORR 0x0045d7e0
int ptl_initialise(const char* fname, const char* gameName)
{
    BlockFile* config;
    BlockRec* arrayItem;
    PTL_Property* property;

    ptlGlobs.count = 0;
    if ((config = block_open(fname)) != NULL) {
        for (arrayItem = block_subgroup(config, gameName); arrayItem != NULL;
             arrayItem = block_next_equal_key(arrayItem)) {
            property = &ptlGlobs.table[ptlGlobs.count++];
            property->fromType = message_lookup_physical_name(arrayItem->key);
            property->toType = message_lookup_physical_name(arrayItem->value);
        }
        block_close(config);
        return 1;
    }
    return 0;
}

/* PTL_TranslateEvent: an event of a translated type takes the new type. */
// FUNCTION: LEGORR 0x0045d870
void ptl_translate_event(Message_Event* event)
{
    unsigned int i;
    PTL_Property* property;

    for (i = 0; i < ptlGlobs.count; i++) {
        property = &ptlGlobs.table[i];
        if (event->type == property->fromType) {
            event->type = property->toType;
            return;
        }
    }
}

/* RadarMap_SetZoom. */
// FUNCTION: LEGORR 0x0045d8a0
void radarmap_set_zoom(RadarMap* radarMap, float zoom)
{
    radarMap->zoom = zoom;
}
