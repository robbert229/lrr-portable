// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — intrusive pointer stacks (see ptrstack.hpp). */

#include "ptrstack.hpp"
#include "heap.hpp"

#include <string.h>

// FUNCTION: LEGORR 0x004628e0
void* callback_record_allocate_initialize(void** out_record, uint32_t param_2,
                                          uint32_t param_3, uint32_t param_4,
                                          uint32_t param_5, uint32_t param_6)
{
    void* record = engine_heap_alloc(0x28);
    if (record != NULL) {
        memset(record, 0, 0x28);
        void** slot = (void**)engine_heap_alloc(sizeof(void*));
        __builtin_memcpy((char*)record + 0x20, &slot,
                         sizeof slot); /* corpus dword 8 */
        if (slot != NULL) {
            /* deviation: the corpus strides when this alloc fails */
            void* buffer = engine_heap_alloc(0x20);
            if (buffer != NULL) {
                memset(buffer, 0, 0x20);
            }
            *slot = buffer;
        }
        uint32_t* d = (uint32_t*)record;
        d[5] = param_6;
        d[1] = param_2;
        d[2] = param_3;
        d[3] = param_4;
        d[4] = param_5;
        d[0] = param_3; /* pinned: param_3 rides dwords 0 and 2 */
    }
    if (out_record != NULL) {
        /* pinned quirk: the corpus writes the slot even when the record
         * allocation failed (NULL lands in the caller's slot) */
        *out_record = record;
    }
    return record;
}
