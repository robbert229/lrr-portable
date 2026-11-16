// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 Memory.h's Mem_Free: an inline over the CRT's free. Through the
 * inline, every argument of a run of frees loads into eax, as in the
 * original; direct free() calls rotate eax/ecx/edx. Needs <stdlib.h>.
 * lego_maths.h keeps Lego.c's own copy (lego_e_mem_free). */
#ifndef LEGORRR_GODS98_MEM_FREE_H
#define LEGORRR_GODS98_MEM_FREE_H

static inline void Mem_Free(void* addr)
{
    free(addr);
}

#endif /* LEGORRR_GODS98_MEM_FREE_H */
