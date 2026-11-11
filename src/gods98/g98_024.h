// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-024: the gods98 handle table (finding 515). */
#ifndef LEGORRR_GODS98_G98_024_H
#define LEGORRR_GODS98_G98_024_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void level_map_slots_reset(void);
int level_map_slot_allocate(size_t size);
void level_map_slot_release(unsigned int slot);
unsigned char* level_map_slot_get(unsigned int slot);
#ifdef __cplusplus
}
#endif
#endif
