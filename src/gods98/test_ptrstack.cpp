// SPDX-License-Identifier: LGPL-3.0-only
// Intrusive pointer stacks (findings 366/393): push/pop/for-each over the
// item+0x9c link and the +0x4/+0x8 head/count pair.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/ptrstack.hpp"
#include "gods98/heap.hpp"

static int s_failures = 0;

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

    if (s_failures != 0) {
        fprintf(stderr, "ptrstack_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("ptrstack_test: PASS\n");
    return 0;
}
