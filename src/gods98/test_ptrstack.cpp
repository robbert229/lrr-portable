// SPDX-License-Identifier: LGPL-3.0-only
// Intrusive pointer stacks (findings 366/393): push/pop/for-each over the
// item+0x9c link and the +0x4/+0x8 head/count pair.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/ptrstack.hpp"
#include "gods98/heap.hpp"

static int s_failures = 0;

static uint32_t dword_at(const void* block, long offset)
{
    uint32_t v;
    __builtin_memcpy(&v, (const char*)block + offset, sizeof v);
    return v;
}

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

/* Corpus-shaped item: opaque record whose link slot lives at +0x9c.
 * Everything past the link is the item's own extent (the corpus items
 * are large records). */
typedef struct StackItem {
    char storage[PTRSTACK_LINK_OFFSET + 16];
} StackItem;

static void item_link_set(StackItem* item, void* next)
{
    __builtin_memcpy((char*)item + PTRSTACK_LINK_OFFSET, &next, sizeof next);
}

static void* item_link_get(StackItem* item)
{
    void* next;
    __builtin_memcpy(&next, (char*)item + PTRSTACK_LINK_OFFSET, sizeof next);
    return next;
}

static void test_push_pop(void)
{
    PtrStack s;
    memset(&s, 0, sizeof(s));
    StackItem a = {{0}}, b = {{0}}, c = {{0}};

    CHECK(s.count == 0 && s.head == NULL, "zeroed stack is empty");

    /* Push overwrites the item's own link slot, as in the corpus. */
    item_link_set(&a, (void*)(uintptr_t)0xdeadbeef);
    ptrstack_push(&s, &a);
    CHECK(s.head == &a && s.count == 1, "push makes the item the head");
    CHECK(item_link_get(&a) == NULL,
          "pushed item links to the old (empty) head");

    ptrstack_push(&s, &b);
    ptrstack_push(&s, &c);
    CHECK(s.count == 3, "push counts");
    CHECK(item_link_get(&c) == &b, "chain order is LIFO through the link");

    CHECK(ptrstack_pop(&s) == &c, "pop answers most recent");
    CHECK(s.count == 2 && s.head == &b, "pop advances the head");
    ptrstack_pop(&s);
    ptrstack_pop(&s);
    CHECK(s.count == 0 && s.head == NULL, "the stack drains to empty");
    CHECK(ptrstack_pop(&s) == NULL, "popping the empty stack answers NULL");

    /* Corpus guard: NULL stack or NULL item changes nothing. */
    ptrstack_push(NULL, &a);
    ptrstack_push(&s, NULL);
    CHECK(s.count == 0 && s.head == NULL, "the NULL guards keep it empty");
    ptrstack_pop(NULL);
    ptrstack_for_each(NULL, NULL, NULL);
}

/* Visit order + the corpus's step-order quirk: the next pointer is read
 * AFTER the callback body, so re-linking inside the callback steers. */
typedef struct Visit {
    StackItem* got[4];
    int n;
    int steer;
} Visit;

static void visit(PtrStack* stack, void* item, void* ctx)
{
    (void)stack;
    Visit* v = (Visit*)ctx;
    v->got[v->n++] = (StackItem*)item;
    if (v->steer == 1 && v->n == 1) {
        /* Callback unlinks the successor: the walk resumes at the
         * item's (now edited) link. */
        item_link_set((StackItem*)item, NULL);
        v->steer = 2;
    }
}

static void test_for_each(void)
{
    PtrStack s;
    memset(&s, 0, sizeof(s));
    StackItem a = {{0}}, b = {{0}}, c = {{0}};

    ptrstack_for_each(&s, visit, NULL);

    ptrstack_push(&s, &a); /* chain built by hand: head → a → b → c */
    item_link_set(&a, &b);
    item_link_set(&b, &c);
    Visit v = {{NULL, NULL, NULL, NULL}, 0, 0};
    ptrstack_for_each(&s, visit, &v);
    CHECK(v.n == 3 && v.got[0] == &a && v.got[1] == &b && v.got[2] == &c,
          "walk visits the chain in head order");

    Visit steer = {{NULL, NULL, NULL, NULL}, 0, 1};
    ptrstack_for_each(&s, visit, &steer);
    CHECK(steer.n == 1 && steer.got[0] == &a,
          "next is read after the callback: unlinking stops the walk");
}

int main(void)
{
    test_push_pop();
    test_for_each();
    /* CallbackStateListDispose (2026-10-24): the corpus-record drain frees. */
    {
        struct StackRecord { void* state; PtrStack stack; };
        void* raw = engine_heap_alloc(sizeof(struct StackRecord));
        struct StackRecord* rec = (struct StackRecord*)raw;
        rec->state = NULL;
        rec->stack.head = NULL;
        rec->stack.count = 0u;
        void* a = engine_heap_alloc(PTRSTACK_LINK_OFFSET + 16);
        void* b = engine_heap_alloc(PTRSTACK_LINK_OFFSET + 16);
        ptrstack_push(&rec->stack, a);
        ptrstack_push(&rec->stack, b);
        CHECK(rec->stack.count == 2u && rec->stack.head == b,
              "the corpus-shaped record holds the stack window");
        callback_state_list_dispose(rec); /* drains, frees entries, frees block */
        callback_state_list_dispose(NULL); /* guarded */
    }

    /* The record-creation leaves (2026-10-25): the 0xa0 scene-node record,
     * the state record's inner pointer + flags, and the callback record's
     * slot/buffer ladder with the param_3 double-store. */
    {
        const uint32_t t1[3] = { 11u, 12u, 13u };
        const uint32_t t2[3] = { 21u, 22u, 23u };
        void* rec = scene_node_record_create(t1, t2, 0xA3u, 0xA4u, 0xA5u,
                                             0xA6u, 0xA7u);
        CHECK(rec != NULL, "the node record allocates");
        CHECK(dword_at(rec, 0x00) == 21u && dword_at(rec, 0x04) == 22u
                  && dword_at(rec, 0x08) == 23u,
              "the second triple lands first");
        CHECK(dword_at(rec, 0x0c) == 11u && dword_at(rec, 0x10) == 12u
                  && dword_at(rec, 0x14) == 13u,
              "the first triple rides +12/16/20");
        CHECK(dword_at(rec, 0x20) == 0x3f800000u, "slot 8 pins 1.0f");
        CHECK(dword_at(rec, 0x18) == 0u && dword_at(rec, 0x1c) == 0u
                  && dword_at(rec, 0x24) == 0u && dword_at(rec, 0x28) == 0u
                  && dword_at(rec, 0x2c) == 0u && dword_at(rec, 0x30) == 0u
                  && dword_at(rec, 0x34) == 0u,
              "the counter slots stay clear");
        CHECK(dword_at(rec, 0x38) == 0xA6u && dword_at(rec, 0x48) == 0xA6u,
              "param_6 duplicates into the +38/+48 pair");
        CHECK(dword_at(rec, 0x3c) == 0xA3u && dword_at(rec, 0x4c) == 0xA3u,
              "param_3 duplicates into the +3c/+4c pair");
        CHECK(dword_at(rec, 0x40) == 0xA4u && dword_at(rec, 0x50) == 0xA4u,
              "param_4 duplicates into the +40/+50 pair");
        CHECK(dword_at(rec, 0x44) == 0xA5u && dword_at(rec, 0x54) == 0xA5u,
              "param_5 duplicates into the +44/+54 pair");
        CHECK(dword_at(rec, 0x98) == 0xA7u, "param_7 lands dword 0x26");
        CHECK(dword_at(rec, 0x58) == 0u && dword_at(rec, 0x94) == 0u
                  && dword_at(rec, 0x9c) == 0u,
              "the zeroed block keeps the tail clean (dev)");
        engine_heap_free(rec);

        void* nulls = scene_node_record_create(NULL, NULL, 1u, 2u, 3u, 4u, 5u);
        CHECK(nulls != NULL, "the null triples still build the record");
        int zeroed = 1;
        for (int off = 0; off < 0x18; off += 4) {
            zeroed = zeroed && dword_at(nulls, off) == 0u;
        }
        CHECK(zeroed, "the null triples land zeros");
        CHECK(dword_at(nulls, 0x20) == 0x3f800000u
                  && dword_at(nulls, 0x3c) == 1u && dword_at(nulls, 0x98) == 5u,
              "the pins ride past the triples");
        engine_heap_free(nulls);
    }

    {
        const uint32_t t1[3] = { 31u, 32u, 33u };
        void* rec = callback_state_record_create(t1, NULL, 7u, 8u, 9u, 10u);
        CHECK(rec != NULL, "the state record allocates");
        void* inner = NULL;
        __builtin_memcpy(&inner, rec, sizeof inner);
        CHECK(inner != NULL, "the node record rides slot +0");
        CHECK(dword_at(inner, 0x20) == 0x3f800000u
                  && dword_at(inner, 0x0c) == 31u && dword_at(inner, 0x00) == 0u,
              "the inner record builds over the triples");
        CHECK(dword_at(rec, 0x0c) == 1u && dword_at(rec, 0x10) == 0u,
              "the flag dwords read the triples' non-nullness");
        CHECK(dword_at(rec, 0x24) == 0u, "the tail dword stays clear");
        engine_heap_free(inner);
        engine_heap_free(rec);

        void* no_triples = callback_state_record_create(NULL, NULL, 0u, 0u, 0u,
                                                        0u);
        CHECK(no_triples != NULL && dword_at(no_triples, 0x0c) == 0u
                  && dword_at(no_triples, 0x10) == 0u,
              "the all-null state record flags nothing");
        void* inner2 = NULL;
        __builtin_memcpy(&inner2, no_triples, sizeof inner2);
        CHECK(inner2 != NULL, "the null state record still builds its node");
        engine_heap_free(inner2);
        engine_heap_free(no_triples);
    }

    {
        void* out = (void*)(uintptr_t)0xdeadbeef;
        void* rec = callback_record_allocate_initialize(&out, 2u, 3u, 4u, 5u,
                                                        6u);
        CHECK(rec != NULL && out == rec,
              "the out slot rides even past the pins (pinned quirk)");
        CHECK(dword_at(rec, 0x00) == 3u && dword_at(rec, 0x08) == 3u,
              "param_3 rides dwords 0 and 2 (pinned)");
        CHECK(dword_at(rec, 0x04) == 2u && dword_at(rec, 0x0c) == 4u
                  && dword_at(rec, 0x10) == 5u && dword_at(rec, 0x14) == 6u,
              "the payload dwords land in order");
        CHECK(dword_at(rec, 0x18) == 0u && dword_at(rec, 0x1c) == 0u,
              "the zeroed dwords stay clear");
        void* slot = NULL;
        __builtin_memcpy(&slot, (char*)rec + 0x20, sizeof slot);
        CHECK(slot != NULL, "the link slot allocates");
        void* buffer = NULL;
        __builtin_memcpy(&buffer, slot, sizeof buffer);
        CHECK(buffer != NULL, "the slot front points at the buffer");
        int zeroed = 1;
        for (int off = 0; off < 0x20; off += 4) {
            zeroed = zeroed && dword_at(buffer, off) == 0u;
        }
        CHECK(zeroed, "the 0x20 buffer zeroes through");
        engine_heap_free(buffer);
        engine_heap_free(slot);
        engine_heap_free(rec);

        /* the guarded out slot (deviation: the corpus strides it) */
        void* rec2 = callback_record_allocate_initialize(NULL, 1u, 1u, 1u, 1u,
                                                         1u);
        CHECK(rec2 != NULL, "the guarded out slot still builds");
        void* slot2 = NULL;
        __builtin_memcpy(&slot2, (char*)rec2 + 0x20, sizeof slot2);
        void* buffer2 = NULL;
        __builtin_memcpy(&buffer2, slot2, sizeof buffer2);
        engine_heap_free(buffer2);
        engine_heap_free(slot2);
        engine_heap_free(rec2);
    }

    if (s_failures != 0) {
        fprintf(stderr, "ptrstack_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("ptrstack_test: PASS\n");
    return 0;
}
