// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-024: 2000-slot handle table (finding 514). */
#include <stdlib.h>
#include "g98_024.h"
#include "crt/vc5names.h"

#define HANDLE_COUNT 2000

typedef struct HandleSlot {
    unsigned char* ptr;
    unsigned int flags;
} HandleSlot;

// GLOBAL: LEGORR 0x00544a20
static HandleSlot handles[HANDLE_COUNT];

// FUNCTION: LEGORR 0x004893f0
void level_map_slots_reset(void)
{
    unsigned int i;
    for (i = 0; i < HANDLE_COUNT; i++) {
        handles[i].ptr = NULL;
        handles[i].flags = 0;
    }
}

// FUNCTION: LEGORR 0x00489410
int level_map_slot_allocate(size_t size)
{
    unsigned int i;
    for (i = 0; i < HANDLE_COUNT; i++) {
        if (!(handles[i].flags & 1)) {
            handles[i].ptr = (unsigned char*)malloc(size);
            handles[i].flags |= 1;
            return i;
        }
    }
    return -1;
}

// FUNCTION: LEGORR 0x00489460
void level_map_slot_release(unsigned int slot)
{
    if (slot < HANDLE_COUNT && (handles[slot].flags & 1)) {
        free(handles[slot].ptr);
        handles[slot].ptr = NULL;
        handles[slot].flags = 0;
    }
}

// FUNCTION: LEGORR 0x004894a0
unsigned char* level_map_slot_get(unsigned int slot)
{
    if (slot < HANDLE_COUNT && (handles[slot].flags & 1))
        return handles[slot].ptr;
    return NULL;
}
