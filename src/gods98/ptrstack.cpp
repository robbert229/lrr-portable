// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — intrusive pointer stacks (see ptrstack.hpp). */

#include "ptrstack.hpp"
#include "heap.hpp"

/* Item link slot: corpus item+0x9c. The corpus reads/writes a 32-bit
 * dword there; the port moves native pointers with memcpy so access is
 * byte-safe even though +0x9c is not pointer-aligned on 64-bit hosts
 * (documented deviation in the header). */
static void* ptrstack_link_get(const void* item)
{
    void* next;
    __builtin_memcpy(&next, (const char*)item + PTRSTACK_LINK_OFFSET,
                     sizeof(next));
    return next;
}

static void ptrstack_link_set(void* item, void* next)
{
    __builtin_memcpy((char*)item + PTRSTACK_LINK_OFFSET, &next,
                     sizeof(next));
}

/* PointerStackPush (FUN_0040fe40): link item → current head, make item
 * the head, count++. Corpus guard: both pointers nonzero. */
void ptrstack_push(PtrStack* stack, void* item)
{
    if (stack == NULL || item == NULL) {
        return;
    }
    ptrstack_link_set(item, stack->head);
    stack->head = item;
    stack->count = stack->count + 1;
}

/* PointerStackPop (FUN_0040fe60): nonempty stack answers the head and
 * advances it through the item link, decrementing the count. */
void* ptrstack_pop(PtrStack* stack)
{
    if (stack == NULL || stack->count == 0) {
        return NULL;
    }
    void* item = stack->head;
    stack->head = ptrstack_link_get(item);
    stack->count = stack->count - 1;
    return item;
}

/* PointerStackForEach (FUN_0040fe90): callback walk. The corpus's for-
 * loop fetches item+0x9c AFTER the callback body runs — reproduced. */
void ptrstack_for_each(PtrStack* stack, PtrStackCallback cb, void* ctx)
{
    if (stack == NULL || cb == NULL) {
        return;
    }
    for (void* item = stack->head; item != NULL;) {
        cb(stack, item, ctx);
        item = ptrstack_link_get(item);
    }
}

/* CallbackStateListDispose (FUN_0040f850) — see ptrstack.hpp. */
void callback_state_list_dispose(void* stack_allocation)
{
    if (stack_allocation == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    char* block = (char*)stack_allocation;
    PtrStack stack;
    __builtin_memcpy(&stack, block + sizeof(void*), sizeof stack);
    void* item = ptrstack_pop(&stack);
    while (item != NULL) {
        optional_resource_destroy(item);
        item = ptrstack_pop(&stack);
    }
    void* head_state = NULL;
    __builtin_memcpy(&head_state, block, sizeof head_state);
    engine_heap_free(head_state);
    engine_heap_free(block);
}
