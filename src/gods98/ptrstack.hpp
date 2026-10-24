// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — intrusive pointer stacks (corpus FUN_0040fe40 /
 * FUN_0040fe60 / FUN_0040fe90; findings 366/393). The corpus's
 * callback-state lists and scene-node lists are singly-linked stacks:
 * items chain through a link word at item+0x9c, and the stack record
 * keeps its head and count at +0x4/+0x8 inside a larger record. These
 * three leaves are the whole stack machine — push, pop, and the
 * callback walk — so they ship standalone with the layout documented
 * below (same treatment as the heap seam and the LWO cursor reader).
 *
 * Layout deviation (documented): the corpus is a 32-bit program, so its
 * head slot holds a 4-byte pointer; the port stores the native pointer
 * and reproduces the item+0x9c link with byte-safe access (the corpus's
 * dword slot is not pointer-aligned on 64-bit hosts) so any record
 * shape can be chained. The corpus dereferences its stack
 * pointer unguarded (NULL pop/foreach crashes) and calls its callback
 * unconditionally; the port answers the empty case instead and skips a
 * NULL callback.
 */
#ifndef LEGORRR_GODS98_PTRSTACK_HPP
#define LEGORRR_GODS98_PTRSTACK_HPP

#include <stdint.h>
#include <stddef.h>

/* Corpus offsets: stack head +0x4 / count +0x8 (inside the containing
 * record), item link field +0x9c. */
#define PTRSTACK_LINK_OFFSET 0x9c

typedef struct PtrStack {
    void*    head;  /* corpus +0x4 */
    uint32_t count; /* corpus +0x8 */
} PtrStack;

/* The corpus callback: (container, item, context). */
typedef void (*PtrStackCallback)(PtrStack* stack, void* item, void* ctx);

/* FUN_0040fe40: link `item` to the current head at item+0x9c, make it
 * the head, and count++. NULL stack or item is the corpus's skip case. */
void ptrstack_push(PtrStack* stack, void* item);

/* FUN_0040fe60: answer the head when the count is nonzero, advance the
 * head through item+0x9c, and decrement the count; the corpus answers 0
 * on the empty stack (and never touches a freed item's link then). */
void* ptrstack_pop(PtrStack* stack);

/* FUN_0040fe90: walk the chain calling cb(container, item, ctx). The
 * corpus reads the *next* pointer after the callback returns — the
 * port reproduces that step order, so a callback that re-links the
 * current item steers the walk. */
void ptrstack_for_each(PtrStack* stack, PtrStackCallback cb, void* ctx);

/* CallbackStateListDispose (FUN_0040f850): drains the stack, releasing each
 * entry through the resource-destroy wrapper, then frees the head-state
 * word at stack-record +0x0 and the stack allocation itself (finding 393).
 * Deviation: the corpus record keeps the head dword beside (+0x4) the
 * count (+0x8) next to the state word (+0x0); the port lays the record
 * out host-widened — state word, then the PtrStack window — and copies
 * the window before draining. */
void callback_state_list_dispose(void* stack_allocation);
                                                /* FUN_0040f850 */

#endif /* LEGORRR_GODS98_PTRSTACK_HPP */
