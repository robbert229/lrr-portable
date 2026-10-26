// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — intrusive pointer stacks (see ptrstack.hpp). */

#include "ptrstack.hpp"
#include "heap.hpp"

#include <string.h>

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
// FUNCTION: LEGORR 0x0040fe40
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
// FUNCTION: LEGORR 0x0040fe60
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
// FUNCTION: LEGORR 0x0040fe90
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
// FUNCTION: LEGORR 0x0040f850
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

/* --- the record-creation leaves (2026-10-25) --------------------------------
 * SceneNodeRecordCreate (FUN_0040f6c0): allocates the 0xa0-byte scene-node
 * record through the engine heap, zeroes the counter slots, pins slot 8 to
 * 1.0, copies the param_2 triple into +0/+4/+8 (zeros when null) and the
 * param_1 triple into +12/+16/+20, duplicates param_3..param_6 into the
 * paired dword slots, stores param_7 at dword 0x26, and answers the record.
 * Deviations: the whole block rides zero-init (the corpus leaves the tail
 * dwords as heap garbage), and the null allocation answers NULL (the corpus
 * heap would assert). All record fields are 32-bit values, so the block
 * keeps the corpus layout exactly.
 *
 * CallbackStateRecordCreate (FUN_0040f7d0): allocates the 0x28-byte state
 * record, creates the scene-node record over (param_1, param_2, param_3,
 * param_4, param_5, 0, param_6) into slot +0, and records which of the two
 * triples was non-null at dwords 3/4. Pinned quirk: the corpus stores the
 * node result through the record even when that create's own allocation
 * fails (slot +0 lands NULL) while the flag dwords still land. Deviation:
 * the corpus writes dwords 1/2 explicitly zero — LP64 widens the +0 slot to
 * a native pointer, whose high half covers dword 1, and the port zeroes
 * the block.
 *
 * CallbackRecordAllocateInitialize (FUN_004628e0): allocates the 0x28-byte
 * record, zeroes ten dwords, allocates a link slot holding a zeroed
 * 0x20-byte buffer, stores the slot at dword 8, and fills dwords 0..5 with
 * param_2..param_6 — with param_3 riding twice, at dwords 0 and 2 (pinned).
 * Pinned quirk: the corpus writes the caller's out slot through *outside*
 * the allocation-success branch, so the slot lands NULL when the record
 * allocation fails. Deviations: the caller slot write is guarded, and the
 * slot (corpus 4 bytes) and its buffer pointer widen to native pointers. */
// FUNCTION: LEGORR 0x0040f6c0
void* scene_node_record_create(const uint32_t* triple1,
                               const uint32_t* triple2, uint32_t param_3,
                               uint32_t param_4, uint32_t param_5,
                               uint32_t param_6, uint32_t param_7)
{
    void* record = engine_heap_alloc(0xa0);
    if (record == NULL) {
        return NULL; /* deviation: the corpus heap would assert */
    }
    uint32_t* d = (uint32_t*)record;
    memset(record, 0, 0xa0); /* deviation: zero the whole block */
    d[8] = 0x3f800000u;      /* pinned: slot 8 is 1.0 */
    for (int i = 0; i < 3; ++i) {
        d[i] = triple2 != NULL ? triple2[i] : 0u;
        d[3 + i] = triple1 != NULL ? triple1[i] : 0u;
    }
    d[0x0e] = param_6;
    d[0x12] = param_6;
    d[0x0f] = param_3;
    d[0x13] = param_3;
    d[0x10] = param_4;
    d[0x14] = param_4;
    d[0x11] = param_5;
    d[0x15] = param_5;
    d[0x26] = param_7;
    return record;
}

// FUNCTION: LEGORR 0x0040f7d0
void* callback_state_record_create(const uint32_t* triple1,
                                   const uint32_t* triple2, uint32_t param_3,
                                   uint32_t param_4, uint32_t param_5,
                                   uint32_t param_6)
{
    void* record = engine_heap_alloc(0x28);
    if (record == NULL) {
        return NULL; /* deviation: the corpus heap would assert */
    }
    memset(record, 0, 0x28);
    void* inner = scene_node_record_create(triple1, triple2, param_3,
                                           param_4, param_5, 0u, param_6);
    memcpy(record, &inner, sizeof inner);
    const uint32_t has_triple1 = triple1 != NULL ? 1u : 0u;
    const uint32_t has_triple2 = triple2 != NULL ? 1u : 0u;
    __builtin_memcpy((char*)record + 12, &has_triple1, 4); /* corpus +0xc */
    __builtin_memcpy((char*)record + 16, &has_triple2, 4); /* corpus +0x10 */
    return record;
}

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
