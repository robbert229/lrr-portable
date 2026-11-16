// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 Memory.h's Mem_Free for Interface.c: an inline over the CRT's
 * free, as lego_maths.h's copy for Lego.c. Through the inline, every
 * argument of 0041a850's run of frees loads into eax, as in the original;
 * direct free() calls rotate eax/ecx/edx. Needs <stdlib.h>. Only
 * interface_unit.c includes this file. */
#ifndef LEGORRR_GAME_INTERFACE_MEM_H
#define LEGORRR_GAME_INTERFACE_MEM_H

static __inline void interface_mem_free(void* addr)
{
    free(addr);
}

#endif /* LEGORRR_GAME_INTERFACE_MEM_H */
