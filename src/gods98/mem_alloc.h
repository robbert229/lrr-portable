// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 Memory.h's Mem_Alloc and Mem_ReAlloc: inlines over the CRT's
 * malloc and realloc, as mem_free.h's Mem_Free is over free. Through the
 * inline the size (and pointer) reach the call as the original's code
 * passes them. Needs <stdlib.h>. */
#ifndef LEGORRR_GODS98_MEM_ALLOC_H
#define LEGORRR_GODS98_MEM_ALLOC_H

static inline void* Mem_Alloc(unsigned int size)
{
    return malloc(size);
}

static inline void* Mem_ReAlloc(void* addr, unsigned int size)
{
    return realloc(addr, size);
}

#endif /* LEGORRR_GODS98_MEM_ALLOC_H */
