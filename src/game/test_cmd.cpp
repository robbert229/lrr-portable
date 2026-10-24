// SPDX-License-Identifier: LGPL-3.0-or-later
// Entity command primitives + queue executor (finding 10).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"
#include "game/level.hpp"
#include "game/select.hpp"
#include "gods98/heap.hpp"
#include "gods98/media.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static EntitySlot* make_ent(GameContext* ctx, int type, const char* name)
{
    int idx = entity_create(ctx, type, name, 0, 0, 0, 0);
    CHECK(idx >= 0, "entity_create");
    return (EntitySlot*)entity_at(ctx, type, idx);
}

int main(void)
{
    cmd_init();

    /* Registry: the 31 task-type names in address order (finding 10). */
    CHECK(strcmp(cmd_task_name(0), "Goto") == 0, "task 0 Goto");
    CHECK(strcmp(cmd_task_name(4), "Gather") == 0, "task 4 Gather");
    CHECK(strcmp(cmd_task_name(12), "Clear") == 0, "task 12 Clear");
    CHECK(strcmp(cmd_task_name(30), "FindLoad") == 0, "task 30 FindLoad");
    CHECK(cmd_task_name(31) == NULL, "task table bound 31");

    /* Registry: the 27 priority names + behaviour defaults/overrides. */
    CHECK(strcmp(cmd_priority_name(0), "Crystal") == 0, "prio 0 Crystal");
    CHECK(strcmp(cmd_priority_name(26), "Congregate") == 0, "prio 26 Congregate");
    CHECK(cmd_kind_behaviour(0) == 0x37, "behaviour[0] 0x37");
    CHECK(cmd_kind_behaviour(7) == 0x31, "behaviour[7] default 0x31");
    CHECK(cmd_kind_behaviour(26) == 99, "behaviour[26] 99");

    /* Priority-name lookup. */
    int kind = -1;
    CHECK(cmd_kind_by_name("Construction", &kind) == 1 && kind == 4,
          "name -> Construction kind 4");
    CHECK(cmd_kind_by_name("NotAThing", &kind) == 0, "unknown name rejected");
    CHECK(cmd_kind_by_name("ore", &kind) == 1 && kind == 1,
          "case-insensitive lookup");

    /* Pool: alloc zeroed, free returns to the list, recycled records reuse. */
    void* a = cmd_alloc(0);
    void* b = cmd_alloc(2);
    void* c = cmd_alloc(3);
    CHECK(a != NULL && b != NULL && c != NULL, "pool allocates three records");
    CHECK(cmd_rec_type(a) == 0 && cmd_rec_type(b) == 2 && cmd_rec_type(c) == 3,
          "record types");
    CHECK(cmd_rec_state(a) == 0, "fresh record state 0");
    cmd_free(a, 0);
    void* again = cmd_alloc(9);
    CHECK(again != NULL, "realloc after free");
    CHECK(cmd_rec_type(again) == 9, "recycled record gets new type");
    cmd_free(b, 0);
    cmd_free(c, 0);
    cmd_free(again, 0);

    GameContext ctx;
    game_context_init(&ctx);
    EntitySlot* movee = make_ent(&ctx, 2, "mover");
    EntitySlot* tgt   = make_ent(&ctx, 2, "miner");
    EntitySlot* bldg  = make_ent(&ctx, 4, "wall");
    EntitySlot* minion = make_ent(&ctx, 6, "minion");
    EntitySlot* train = make_ent(&ctx, 8, "train");

    /* Move command chains onto the entity (entity +0x2f0 analog). */
    cmd_create_move(movee, 3.0f, -2.0f, 1);
    CHECK(movee->cmd_head != NULL, "move chains onto the entity");
    CHECK(cmd_rec_type(movee->cmd_head) == 0, "move command type 0");
    CHECK(cmd_rec_state(movee->cmd_head) & CMD_STATE_PENDING ? 0 : 1,
          "move tail-linked (not pending)");

    /* Target command. */
    cmd_create_target(tgt, (void*)bldg);
    CHECK(tgt->cmd_head != NULL && cmd_rec_type(tgt->cmd_head) == 2,
          "target command type 2");
    CHECK(cmd_rec_payload(tgt->cmd_head) == (const void*)bldg,
          "target payload stored");

    /* Gated commands need entity +0x3e0 & 0x400. */
    cmd_create_gated(bldg, NULL);
    CHECK(bldg->cmd_head == NULL, "gated skipped without the 0x400 flag");
    bldg->flags_3e0 |= 0x400u;
    cmd_create_gated(bldg, NULL);
    CHECK(bldg->cmd_head != NULL && cmd_rec_type(bldg->cmd_head) == 5,
          "gated command type 5 with flag");

    /* Tasks land on the global list via CmdLinkHead (kind from entity type). */
    cmd_create_task(tgt);            /* type 2 -> default kind 2 */
    CHECK(cmd_global_head() != NULL && cmd_rec_type(cmd_global_head()) == 3,
          "task command type 3");
    CHECK(cmd_rec_kind(cmd_global_head()) == 2, "type-2 task kind 2");
    CHECK(cmd_rec_behaviour(cmd_global_head()) == cmd_kind_behaviour(2),
          "task behaviour follows registry");

    cmd_create_task(minion);         /* type 6 plain -> kind 0 */
    CHECK(cmd_rec_kind(cmd_global_head()) == 0, "minion task kind 0");
    minion->flags_3e8 |= 0x80000000u;
    cmd_create_task(minion);         /* flagged minion -> kind 0x12 */
    CHECK(cmd_rec_kind(cmd_global_head()) == 0x12, "flagged minion kind 0x12");

    cmd_create_task(train);          /* type 8 -> kind 3 + extra 8 */
    CHECK(cmd_rec_kind(cmd_global_head()) == 3, "train task kind 3");

    /* Global tick drains the list (the AI receivers are stub boundaries). */
    cmd_tick_global();

    /* Queue executor: 5-dword records, ping-pong, opcode switch. */
    cmd_context_set(&ctx);
    EntitySlot* d = make_ent(&ctx, 1, "digger");
    cmd_enqueue(5, d, 1, 4, 5);       /* move opcode 5: p1/p2 = coords */
    cmd_enqueue(0x17, d, 0, 0, 0);    /* default task */
    cmd_enqueue(0x40, d, 0, 0, 0);   /* manager-stub opcode */
    cmd_queue_tick();
    CHECK(d->cmd_head != NULL && cmd_rec_type(d->cmd_head) == 0,
          "queue move dispatched onto entity");
    CHECK(cmd_global_head() != NULL && cmd_rec_type(cmd_global_head()) == 3,
          "queue task dispatched to the global list");
    CHECK(cmd_rec_kind(cmd_global_head()) == 2, "queue task kind for type 1");

    /* Chain purge returns records to the pool. */
    cmd_chain_purge(d);
    CHECK(d->cmd_head == NULL, "purge clears the chain head");
    void* reused = cmd_alloc(0);
    CHECK(reused != NULL, "pool still has records after purge");
    cmd_free(reused, 0);

    /* Records for different entities can be adjacent in the global list.
     * Dropping a type from one entity must not follow the global link into
     * another entity's chain and free its record. */
    cmd_init();
    GameContext separate;
    game_context_init(&separate);
    EntitySlot* first = make_ent(&separate, 1, "first");
    EntitySlot* second = make_ent(&separate, 1, "second");
    cmd_create_move(first, 1.0f, 0.0f, 0);
    cmd_create_target(second, (void*)first);
    void* second_head = second->cmd_head;
    CHECK(cmd_chain_drop(first, 2) == 0,
          "drop does not remove another entity's target record");
    CHECK(first->cmd_head != NULL && cmd_rec_next(first->cmd_head) == NULL,
          "first entity chain ends at its own record");
    CHECK(second->cmd_head == second_head &&
              cmd_rec_type(second->cmd_head) == 2,
          "second entity retains its target record");

    /* The queue-adjacent leaves (2026-10-24): the corpus-shaped cancel,
     * the active-context hand-out, and the pure value search. */
    {
        cmd_produce(11, (const void*)(uintptr_t)1, 0, 0, 0);
        cmd_produce(11, (const void*)(uintptr_t)2, 0, 0, 0);
        cmd_produce(11, (const void*)(uintptr_t)1, 0, 0, 0);
        CHECK(cmd_queue_active_count_peek() == 3, "the bank holds three");
        cmd_queue_cancel_match(1);
        /* Corpus shape: the [1,2,1] sweep removes the slot-0 match (the
         * last 1 swaps into the slot the cursor already passed) and keeps
         * walking, leaving the swapped-in match behind — count 2. */
        CHECK(cmd_queue_active_count_peek() == 2,
              "the sweep leaves the swapped-in match (faithful)");
        cmd_queue_cancel_match(99);
        CHECK(cmd_queue_active_count_peek() == 2, "the miss changes nothing");
        cmd_queue_cancel_match(2);
        CHECK(cmd_queue_active_count_peek() == 1, "the second match drains");
        cmd_queue_cancel_match(1);
        CHECK(cmd_queue_active_count_peek() == 0, "the bank empties");

        /* Corpus swap-skip on [1,2]: cancel(1) leaves count 1 with the
         * successor swapped into the cursor's slot. */
        cmd_produce(11, (const void*)(uintptr_t)1, 0, 0, 0);
        cmd_produce(11, (const void*)(uintptr_t)2, 0, 0, 0);
        cmd_queue_cancel_match(1);
        CHECK(cmd_queue_active_count_peek() == 1,
              "the swapped-in successor is not rechecked (faithful)");
        cmd_queue_cancel_match(2);
    }

    {
        int count_out = -1;
        CHECK(active_context_record_get(&count_out) == NULL
                  && count_out == 0,
              "nothing selected: zero count, NULL handle");
    }

    {
        const int table[] = { 7, 9, 3, 9 };
        unsigned idx = 99;
        CHECK(cmd_array_find_value_index(table, 4, 3, &idx) == 1
                  && idx == 2,
              "the value search finds the second 3-shaped hit");
        idx = 55;
        CHECK(cmd_array_find_value_index(table, 4, 8, &idx) == 0
                  && idx == 55,
              "the miss leaves the output alone");
        CHECK(cmd_array_find_value_index(NULL, 4, 3, &idx) == 0,
              "the NULL array skips (deviation)");
        CHECK(cmd_array_find_value_index(table, 4, 7, NULL) == 1,
              "the NULL output is safe");
    }

    /* The selection snapshot / dispatch leaves (2026-10-24). */
    {
        GameContext sctx;
        game_context_init(&sctx);
        cmd_context_set(&sctx);

        void* arr = (void*)1;
        unsigned cnt = 77;
        CHECK(selection_snapshot_create(&arr, &cnt) == 0 && arr == (void*)1
                  && cnt == 77,
              "the empty selection answers zero, writes nothing");

        EntitySlot* e1 = make_ent(&sctx, 1, "snap1");
        EntitySlot* e2 = make_ent(&sctx, 1, "snap2");
        selection_add(&sctx, e1);
        selection_add(&sctx, e2);
        arr = NULL;
        CHECK(selection_snapshot_create(&arr, &cnt) == 1 && cnt == 2
                  && arr != NULL
                  && ((EntitySlot**)arr)[0] == e1
                  && ((EntitySlot**)arr)[1] == e2,
              "the snapshot copies both pointers and the count");
        engine_heap_free(arr);

        /* Dispatch: nothing selected, no context — reject. */
        media_min_distance_update_mode(1, NULL, 0);
        selection_clear(&sctx);
        CHECK(selection_command_dispatch(0) == 0,
              "nothing to dispatch — zero");

        /* Selection hit: a type-2 entity with the flag bit. */
        EntitySlot* t2 = make_ent(&sctx, 2, "dispatch2");
        t2->flags_3e8 |= 0x10u;
        selection_add(&sctx, t2);
        CHECK(selection_command_dispatch(5) == 1
                  && media_min_distance_context() == (void*)t2
                  && media_min_distance_mode() == 0,
              "the scan dispatches mode 0 to the candidate");
        selection_clear(&sctx);
        media_min_distance_update_mode(1, NULL, 0); /* clear the context */

        /* Owner indirection: the candidate swaps through a flagged owner. */
        EntitySlot owner;
        memset(&owner, 0, sizeof(owner));
        owner.type = 2;
        owner.flags_3e8 = 0x10u;
        EntitySlot child;
        memset(&child, 0, sizeof(child));
        child.type = 2;
        child.owner_36c = &owner;
        selection_add(&sctx, &child);
        CHECK(selection_command_dispatch(0) == 1
                  && media_min_distance_context() == (void*)&owner,
              "the dispatch follows the owner indirection");
        selection_clear(&sctx);

        /* Context gate first: a flagged, type-2 context dispatches even
         * with an empty selection. */
        EntitySlot sel;
        memset(&sel, 0, sizeof(sel));
        sel.type = 2;
        sel.flags_3e8 = 0x10u;
        media_min_distance_update_mode(0, &sel, 0);
        CHECK(selection_command_dispatch(9) == 1
                  && media_min_distance_context() == (void*)&sel,
              "the context gate dispatches first");

        /* A flagless context with an empty selection rejects. */
        memset(&sel, 0, sizeof(sel));
        sel.type = 1;
        sel.flags_3e8 = 0u;
        media_min_distance_update_mode(0, &sel, 0);
        CHECK(selection_command_dispatch(0) == 0,
              "the unflagged context with nothing selected rejects");
        media_min_distance_update_mode(1, NULL, 0);
        cmd_context_set(&ctx);
    }

    /* The random-cell producer + entity remove (2026-10-24). */
    {
        static uint8_t grid[8 * 8 * 0x48]; /* LEVEL_VOXEL_STRIDE cells */
        GameContext lctx;
        game_context_init(&lctx);
        cmd_context_set(&lctx);

        /* (Re)build the hand-built level block: 8x8, no voxels yet. */
        static LevelState lvl;
        memset(&lvl, 0, sizeof(lvl));
        memset(grid, 0, sizeof(grid));
        lvl.word[0x14 / 4] = 8; /* LEVEL_OFF_WIDTH  */
        lvl.word[0x18 / 4] = 8; /* LEVEL_OFF_HEIGHT */
        lvl.ptr_voxels = grid;
        lctx.level = &lvl;

        /* All cells unflagged: no draw is ever accepted. */
        entity_random_valid_cell_command_create(NULL);
        CHECK(cmd_queue_active_count_peek() == 0,
              "no walkable cell — nothing queues");

        /* Flag every cell walkable: the first draw queues 0x24. */
        for (int i = 0; i < 8 * 8; ++i) {
            const uint32_t f = 8u;
            memcpy(grid + (size_t)i * 0x48 + 0x08, &f, 4);
        }
        entity_random_valid_cell_command_create(NULL);
        CHECK(cmd_queue_active_count_peek() == 1,
              "the accepted draw queues operation 0x24");
        cmd_queue_tick();

        /* No level at all: the guard skips (no modulo-zero). */
        lctx.level = NULL;
        entity_random_valid_cell_command_create(NULL);
        CHECK(cmd_queue_active_count_peek() == 0,
              "the absent level skips (deviation)");
    }

    {
        /* selection_entity_remove: found removes, missing answers zero. */
        GameContext rctx;
        game_context_init(&rctx);
        cmd_context_set(&rctx);
        EntitySlot* x1 = make_ent(&rctx, 1, "rem1");
        EntitySlot* x2 = make_ent(&rctx, 1, "rem2");
        selection_add(&rctx, x1);
        selection_add(&rctx, x2);
        x1->flags_3e8 |= 0x200000u;
        CHECK(selection_entity_remove(x1) == 1, "the found entity leaves");
        CHECK(selection_count(&rctx) == 1
                  && selection_contains_entity(&rctx, x2) == 1
                  && selection_contains_entity(&rctx, x1) == 0,
              "the last entry swaps into the vacated slot");
        CHECK((x1->flags_3e8 & 0x200000u) == 0u, "the +0x3e8 bit clears");
        CHECK(selection_entity_remove(x1) == 0, "the second miss answers 0");
        CHECK(selection_entity_remove(NULL) == 0, "NULL answers zero");
        cmd_context_set(&ctx);
    }

    /* The selection filter / state monitor (2026-10-24). */
    {
        GameContext fctx;
        game_context_init(&fctx);
        cmd_context_set(&fctx);

        EntitySlot bad;
        memset(&bad, 0, sizeof(bad));
        bad.type = 4;
        CHECK(selection_candidate_filter(&bad, 1, 0) == 0
                  && selection_count(&fctx) == 0,
              "the unstate-gated candidate is ineligible");

        EntitySlot cand;
        memset(&cand, 0, sizeof(cand));
        cand.type = 4;
        cand.flags_3e8 = 0x100u;
        CHECK(selection_candidate_filter(&cand, 1, 0) == 0
                  && selection_count(&fctx) == 1
                  && cand.flags_3e8 == (0x200000u | 0x100u),
              "the eligible candidate appends and gets the selected bit");
        CHECK(selection_candidate_filter(&cand, 1, 0) == 0
                  && selection_count(&fctx) == 1,
              "the already-selected candidate is rejected");
        CHECK(selection_candidate_filter(&cand, 0, 0) == 0
                  && selection_count(&fctx) == 1,
              "mode 0 takes the already-selected path (marker seam)");

        /* Marked (unselected) candidates reject under mode 0. */
        EntitySlot marked;
        memset(&marked, 0, sizeof(marked));
        marked.type = 4;
        marked.flags_3e8 = 0x100u;
        marked.flags_3ec = 2u;
        CHECK(selection_candidate_filter(&marked, 0, 0) == 0
                  && selection_count(&fctx) == 1,
              "the marked candidate rejects under mode 0");

        /* Visual half: flag bit 0x80 with zero stats returns 1. */
        EntitySlot vis;
        memset(&vis, 0, sizeof(vis));
        vis.type = 4;
        vis.flags_3e8 = 0x100u | 0x80u;
        CHECK(selection_candidate_filter(&vis, 1, 0) == 1
                  && selection_count(&fctx) == 2,
              "the visual half reports success (seams cited)");

        /* Attach indirection: the filter swaps to the +0x304 object. */
        EntitySlot child;
        memset(&child, 0, sizeof(child));
        child.attach_304 = &cand;
        CHECK(selection_candidate_filter(&child, 1, 0) == 0
                  && selection_count(&fctx) == 2,
              "the attached candidate (already selected) is rejected");

        /* The hundred-entry cap: nothing appends past it. */
        fctx.selection_count = 100;
        EntitySlot more;
        memset(&more, 0, sizeof(more));
        more.type = 4;
        more.flags_3e8 = 0x100u;
        CHECK(selection_candidate_filter(&more, 1, 0) == 0
                  && fctx.selection_count == 100,
              "the cap rejects (corpus bound)");
        (void)more;

        /* The state monitor: count passes over the head entry. */
        static uint8_t stats_img[0x200];
        memset(stats_img, 0, sizeof(stats_img));
        const uint32_t sw = 0x20000u;
        memcpy(stats_img + 0x144, &sw, 4);

        EntitySlot head;
        memset(&head, 0, sizeof(head));
        EntitySlot owner;
        memset(&owner, 0, sizeof(owner));
        EntitySlot grand;
        memset(&grand, 0, sizeof(grand));
        owner.flags_3e4 = 8u | 0x100u;
        owner.owner_36c = &grand;
        head.owner_36c = &owner;

        GameContext mctx;
        game_context_init(&mctx);
        cmd_context_set(&mctx);
        selection_add(&mctx, &head);
        CHECK(selected_entity_state_monitor() == 1, "the monitor answers count");
        CHECK((owner.flags_3e4 & 8u) != 0u && owner.owner_36c == &grand,
              "no stats bit — the owner keeps its word and bit 3");

        head.stats_record = stats_img;
        owner.owner_36c = &grand;
        owner.flags_3e4 |= 8u;
        CHECK(selected_entity_state_monitor() == 1
                  && (owner.flags_3e4 & 8u) == 0u
                  && owner.owner_36c == NULL,
              "the stats bit clears the owner's word and bit 3");

        selection_add(&mctx, &owner); /* second entry: two corpus-shaped passes */
        owner.owner_36c = &grand;
        owner.flags_3e4 |= 8u;
        CHECK(selected_entity_state_monitor() == 2
                  && (owner.flags_3e4 & 8u) == 0u
                  && owner.owner_36c == NULL,
              "the two-pass run clears the owner again");
        cmd_context_set(&ctx);
    }

    /* The entity record schedulers (2026-10-24). */
    {
        GameContext qctx;
        game_context_init(&qctx);
        cmd_context_set(&qctx);

        /* Deferred schedule: attach mirror clear + flag clear creates the
         * task through the ported fallback. */
        EntitySlot work;
        memset(&work, 0, sizeof(work));
        work.type = 2;
        entity_deferred_work_schedule(&work);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 3
                  && cmd_rec_payload(cmd_global_head()) == &work,
              "the fallback task is created (pool seam misses)");
        cmd_tick_global();

        /* The gates: attach set, or the flag bit — nothing scheduled. */
        work.attach_304 = (const void*)1;
        entity_deferred_work_schedule(&work);
        CHECK(cmd_global_head() == NULL, "the attach mirror blocks");
        work.attach_304 = NULL;
        work.flags_3ec = 0x100u;
        entity_deferred_work_schedule(&work);
        CHECK(cmd_global_head() == NULL, "the flag bit blocks");
        work.flags_3ec = 0u;

        /* The validated record queue: NULL inert, unvalidated skips,
         * validated links the type-0x1a record. */
        entity_command15_validated_record_queue(NULL);
        EntitySlot plain;
        memset(&plain, 0, sizeof(plain));
        plain.field_340 = 1.0f;
        entity_command15_validated_record_queue(&plain);
        CHECK(cmd_global_head() == NULL, "the unvalidated entity skips");
        EntitySlot good;
        memset(&good, 0, sizeof(good));
        good.field_340 = 1.0f;
        static uint8_t vimg[0x200];
        memset(vimg, 0, sizeof(vimg));
        const uint32_t vf = 0x40000u;
        const float vfl = 0.5f;
        memcpy(vimg + 0x148, &vf, 4);
        memcpy(vimg + 0xe8, &vfl, 4);
        good.stats_record = vimg;
        entity_command15_validated_record_queue(&good);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x1a
                  && cmd_rec_payload(cmd_global_head()) == &good
                  && cmd_rec_kind(cmd_global_head()) == 0xf,
              "the validated entity queues a type-0x1a record");
        const uint32_t st = cmd_rec_state(cmd_global_head());
        CHECK((st & 0x40040u) == 0x40040u && (st & 0x10u) == 0u,
              "the state bits raise 6/14/18 and clear bit 4");
        cmd_tick_global();
        const uint32_t off = 0u;
        memcpy(vimg + 0x148, &off, 4); /* the state-flag gate closes */
        entity_command15_validated_record_queue(&good);
        CHECK(cmd_global_head() == NULL,
              "the flag-off entity skips (validator gate)");
        cmd_context_set(&ctx);
    }

    /* The registry accessors / match marker (2026-10-24). */
    {
        GameContext xctx;
        game_context_init(&xctx);
        cmd_context_set(&xctx);

        /* Priority tables: zero-start round-trip and the guarded edges. */
        CHECK(priority_entry_state_get(10) == 0, "the state table starts zero");
        priority_entry_state_set(10, 7);
        CHECK(priority_entry_state_get(10) == 7, "the state write lands");
        priority_entry_state_set(60, 1);
        CHECK(priority_entry_state_get(60) == 0
                  && priority_entry_state_get(26) == 0,
              "the guarded edges stay untouched");
        priority_entry_state_set(10, 0);

        const int save_behaviour = cmd_kind_behaviour(3);
        priority_entry_order_value_set(3, 9);
        CHECK(cmd_kind_behaviour(3) == 9, "the order write feeds behaviour");
        priority_entry_order_value_set(999, 5);
        CHECK(cmd_kind_behaviour(999) == 0x31, "the guarded write skips");
        priority_entry_order_value_set(3, save_behaviour);

        /* Set kind: unmarked records bind, marked ones pin 99. */
        EntitySlot e1 = {};
        memset(&e1, 0, sizeof(e1));
        e1.type = 2;
        cmd_create_task(&e1);
        void* head = cmd_global_head();
        CHECK(head != NULL, "a task for the walk");
        cmd_set_kind(head, 2);
        CHECK(cmd_rec_kind(head) == 2
                  && cmd_rec_behaviour(head) == cmd_kind_behaviour(2),
              "the unmarked record binds the kind");
        cmd_rec_state_set(head, 0x30000u);
        cmd_set_kind(head, 1);
        CHECK(cmd_rec_kind(head) == 1 && cmd_rec_behaviour(head) == 99,
              "the marker bits pin behaviour 99");
        cmd_set_kind(NULL, 1); /* guarded */

        /* Mark matching: type + payload, or type + coordinate pair. */
        cmd_rec_state_set(head, 0u);
        cmd_mark_matching(3, 0x1234, NULL);
        CHECK((cmd_rec_state(head) & 0x10000u) == 0u,
              "the non-matching payload marks nothing");
        cmd_mark_matching(99, (int)(uintptr_t)&e1, NULL);
        CHECK((cmd_rec_state(head) & 0x10000u) == 0u,
              "the non-matching type marks nothing");
        cmd_mark_matching(3, (int)(uintptr_t)&e1, NULL);
        CHECK((cmd_rec_state(head) & 0x10000u) != 0u,
              "the payload match raises bit 16");
        cmd_rec_state_set(head, 0u);
        const int zero_pair[2] = { 0, 0 };
        cmd_mark_matching(3, 0, zero_pair);
        CHECK((cmd_rec_state(head) & 0x10000u) != 0u,
              "the coordinate pair match raises bit 16");
        cmd_tick_global();
        cmd_context_set(&ctx);
    }

    /* The registry flush / wrappers / batch issue (2026-10-24). */
    {
        GameContext rctx;
        game_context_init(&rctx);
        cmd_context_set(&rctx);
        cmd_init();
        CHECK(cmd_global_enabled_peek() == 1, "the word starts at bit 0");
        cmd_flush_all(1);
        CHECK((cmd_global_enabled_peek() & 2) == 0,
              "the disable mode clears bit 1");

        EntitySlot e;
        memset(&e, 0, sizeof(e));
        e.type = 2;
        cmd_create_task(&e);
        priority_entry_state_set(10, 5);
        CHECK(cmd_global_head() != NULL, "a task for the flush");
        cmd_flush_all(0);
        CHECK(cmd_global_head() == NULL
                  && (cmd_global_enabled_peek() & 2) != 0
                  && priority_entry_state_get(10) == 0,
              "the zero flush drops heads, raises bit 1, zeroes states");
        cmd_flush_all(1);
        CHECK(cmd_global_enabled_peek() == 1, "bit 1 cleared back to bit 0");

        engine_memory_allocate(64); /* corpus discards the allocation */
        engine_memory_free(NULL);

        cmd_init();
        entity_command14_batch_issue(1);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 4
                  && cmd_rec_kind(cmd_global_head()) == 0x14
                  && (cmd_rec_state(cmd_global_head()) & 0x40u) == 0u,
              "a nonzero count issues an unflagged record");
        cmd_init();
        entity_command14_batch_issue(0);
        CHECK(cmd_global_head() != NULL
                  && (cmd_rec_state(cmd_global_head()) & 0x40u) != 0u,
              "the zero count raises bit 6");

        cmd_init();
        level_no_gather_object_setup(1);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x1b
                  && cmd_rec_kind(cmd_global_head()) == 0
                  && (cmd_rec_state(cmd_global_head()) & 0x440u) == 0x440u
                  && (cmd_rec_state(cmd_global_head()) & 0x10u) == 0u,
              "the setup record head-lands with bits 6/7/10");
        cmd_init();
        level_no_gather_object_setup(0);
        CHECK(cmd_rec_type(cmd_global_head()) == 0x1b
                  && cmd_global_head() != NULL,
              "the batched setup still lands the head record");
        {
            int seen_head = cmd_rec_type(cmd_global_head());
            void* cur = cmd_global_next(cmd_global_head());
            int seen = 1;
            int batch_seen = 0, gather_seen = 0;
            while (cur != NULL) {
                if (cmd_rec_type(cur) == 4) {
                    batch_seen = 1;
                }
                if (cmd_rec_type(cur) == 0x18) {
                    gather_seen = 1;
                }
                cur = cmd_global_next(cur);
                ++seen;
            }
            CHECK(seen_head == 0x1b && batch_seen && gather_seen,
                  "the batch and gather records chain behind the head");
        }
        cmd_init();
        handle_array_release_and_reset();
        CHECK(cmd_global_enabled_peek() == 0, "the reset zeroes the word");
        cmd_init();
        cmd_context_set(&ctx);
    }

    /* The cancellation ref and conditional issuers (2026-10-24). */
    {
        GameContext cctx;
        game_context_init(&cctx);
        cmd_context_set(&cctx);

        /* Cancel-ref: the marking walk raises bit 5 on the referenced
         * type-0x13 record; the 0x13 side takes the callback seam. */
        cmd_init();
        EntitySlot host;
        memset(&host, 0, sizeof(host));
        host.type = 2;
        cmd_create_task(&host);
        void* ref_rec = cmd_global_head();
        cmd_rec_type_set(ref_rec, 0x13);
        void* owner = cmd_alloc(1);
        CHECK(owner != NULL, "the owner record allocates");
        cmd_rec_ref_set(owner, ref_rec);
        CHECK(cmd_rec_ref_get(owner) == ref_rec, "the ref slot round-trips");
        cmd_cancel_ref(owner);
        CHECK((cmd_rec_state(ref_rec) & 0x20u) != 0u,
              "the marking walk raises bit 5");
        cmd_rec_state_set(ref_rec, 0u);
        cmd_rec_ref_set(owner, NULL);
        cmd_cancel_ref(owner);
        CHECK((cmd_rec_state(ref_rec) & 0x20u) == 0u,
              "no reference marks nothing");
        cmd_rec_type_set(ref_rec, 0x13);
        cmd_rec_ref_set(ref_rec, ref_rec);
        cmd_cancel_ref(ref_rec);
        CHECK((cmd_rec_state(ref_rec) & 0x20u) == 0u,
              "the 0x13 side answers through the seam (no marking)");
        cmd_rec_ref_set(ref_rec, NULL);
        cmd_cancel_ref(NULL); /* guarded */

        /* CmdFree's side effect: not suppressed marks, suppressed does not. */
        cmd_init();
        cmd_create_task(&host);
        void* marked = cmd_global_head();
        cmd_rec_type_set(marked, 0x13);
        void* carrier = cmd_alloc(1);
        cmd_rec_ref_set(carrier, marked);
        cmd_free(carrier, 0);
        CHECK((cmd_rec_state(marked) & 0x20u) != 0u,
              "the unsuppressed free runs the cancel ref");
        cmd_rec_state_set(marked, 0u);
        carrier = cmd_alloc(1);
        cmd_rec_ref_set(carrier, marked);
        cmd_free(carrier, 1);
        CHECK((cmd_rec_state(marked) & 0x20u) == 0u,
              "the suppressed free skips the cancel ref");

        /* Conditional schedule: the capability bit plus the two gates. */
        cmd_init();
        EntitySlot cond;
        memset(&cond, 0, sizeof(cond));
        static uint8_t cimg[0x200];
        memset(cimg, 0, sizeof(cimg));
        uint32_t cap = 1u;
        memcpy(cimg + 0x14c, &cap, 4);
        cond.stats_record = cimg;
        entity_conditional_task_schedule(&cond);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x1e
                  && cmd_rec_payload(cmd_global_head()) == &cond
                  && cmd_rec_kind(cmd_global_head()) == 0x11
                  && (cond.flags_3ec & 0x10000u) != 0u
                  && (cmd_rec_state(cmd_global_head()) & 0x100010u) == 0x100010u
                  && (cmd_rec_state(cmd_global_head()) & 0x40u) == 0u,
              "the conditional task lands (latch, kind, bits)");
        cmd_tick_global();
        entity_conditional_task_schedule(&cond);
        CHECK(cmd_global_head() == NULL, "the latch blocks the second pass");
        cond.flags_3ec = 0u;
        cond.flags_3e0 = 0x400u;
        entity_conditional_task_schedule(&cond);
        CHECK(cmd_global_head() == NULL, "the gate blocks");
        cond.flags_3e0 = 0u;
        cap = 0u;
        memcpy(cimg + 0x14c, &cap, 4);
        entity_conditional_task_schedule(&cond);
        CHECK(cmd_global_head() == NULL, "no capability bit, no task");
        entity_conditional_task_schedule(NULL);

        /* Threshold event queue: kind 7 or 0x13 with bit 21. */
        cmd_init();
        entity_threshold_progress_event_queue(&host, 0);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 10
                  && cmd_rec_payload(cmd_global_head()) == &host
                  && cmd_rec_kind(cmd_global_head()) == 7
                  && (cmd_rec_state(cmd_global_head()) & 0x200000u) == 0u,
              "the clear flag links kind 7");
        cmd_tick_global();
        entity_threshold_progress_event_queue(&host, 1);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_kind(cmd_global_head()) == 0x13
                  && (cmd_rec_state(cmd_global_head()) & 0x200000u) != 0u,
              "the set flag links kind 0x13 and raises bit 21");
        cmd_tick_global();
        entity_threshold_progress_event_queue(NULL, 0);
        cmd_tick_global();

        /* Target queue: the record chains onto the entity, never the list. */
        cmd_init();
        EntitySlot tgt_host;
        memset(&tgt_host, 0, sizeof(tgt_host));
        tgt_host.type = 2;
        entity_target_command_queue(&tgt_host, &host);
        CHECK(cmd_global_head() == NULL, "the target record skips the list");
        CHECK(tgt_host.cmd_head != NULL
                  && cmd_rec_type(tgt_host.cmd_head) == 0x11
                  && cmd_rec_payload(tgt_host.cmd_head) == &host,
              "the empty chain takes the record at the head");
        cmd_create_move(&tgt_host, 1.0f, 2.0f, 0);
        void* first = tgt_host.cmd_head;
        entity_target_command_queue(&tgt_host, &cond);
        CHECK(tgt_host.cmd_head == first, "the head stays put");
        int depth = 0;
        void* last = first;
        while (cmd_rec_next(last) != NULL) {
            last = cmd_rec_next(last);
            ++depth;
        }
        CHECK(depth == 2
                  && cmd_rec_type(last) == 0x11
                  && cmd_rec_payload(last) == &cond,
              "the chain tail takes the record");
        cmd_init();
        cmd_context_set(&ctx);
    }

    /* The record-array half and the entity-kind issuer (2026-10-24). */
    {
        GameContext sctx;
        game_context_init(&sctx);
        cmd_context_set(&sctx);
        cmd_init();

        /* Record array: the empty selection guards everything out. */
        void* rec = cmd_alloc(0x12);
        CHECK(rec != NULL, "the record allocates");
        entity_record_array_apply_zero_state(rec, 1);
        CHECK((cmd_rec_state(rec) & 0x140u) == 0u
                  && cmd_rec_count_get(rec) == 0u,
              "the empty selection raises nothing");
        cmd_free(rec, 1);
        entity_record_array_apply_zero_state(NULL, 1);

        /* Two entries: bits 6/8, the snapshot, and the count. */
        static EntitySlot picked[2];
        memset(picked, 0, sizeof(picked));
        sctx.selection[0] = &picked[0];
        sctx.selection[1] = &picked[1];
        sctx.selection_count = 2;
        rec = cmd_alloc(0x12);
        entity_record_array_apply_zero_state(rec, 0);
        CHECK((cmd_rec_state(rec) & 0x140u) == 0x140u
                  && cmd_rec_count_get(rec) == 2u
                  && cmd_rec_state(rec) != 0u,
              "the guard raises bits 6/8 and takes the snapshot");
        cmd_free(rec, 1); /* the snapshot rides the owned buffer */

        /* Empty-count variant: the snapshot path answers zero. */
        sctx.selection_count = 0;
        rec = cmd_alloc(0x12);
        cmd_rec_state_set(rec, 0u);
        entity_record_array_apply_zero_state(rec, 1);
        CHECK((cmd_rec_state(rec) & 0x140u) == 0u, "the reset guard inert");
        cmd_free(rec, 1);
        sctx.selection[0] = NULL;
        sctx.selection[1] = NULL;

        /* Entity-kind issue: the latch, the +0x3c shapes, bit 20. */
        cmd_init();
        EntitySlot kinded;
        memset(&kinded, 0, sizeof(kinded));
        static uint8_t kimg[0x200];
        memset(kimg, 0, sizeof(kimg));
        uint32_t cap = 0x100u; /* the simple arm: +0x3c = 1 */
        uint32_t st = 0u;
        memcpy(kimg + 0x14c, &cap, 4);
        memcpy(kimg + 0x144, &st, 4);
        kinded.stats_record = kimg;
        entity_kind_command_issue(&kinded);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x12
                  && cmd_rec_payload(cmd_global_head()) == &kinded
                  && cmd_rec_kind(cmd_global_head()) == 0xc,
              "the kind command links at kind 0xc");
        CHECK((cmd_rec_state(cmd_global_head()) & 0x40u) == 0u,
              "bit 6 stays clear");
        CHECK(kinded.flags_3e4 == 0x10u, "the latch raises bit 4");
        cmd_tick_global();
        cmd_init();
        entity_kind_command_issue(&kinded);
        CHECK(cmd_global_head() == NULL, "the latch blocks the second pass");
        cmd_tick_global();
        entity_kind_command_issue(NULL); /* guarded */

        /* The +0x3c arms: stats pair shapes. */
        struct { const char* what; uint32_t cap; uint32_t stats; int want; }
            arms[] = {
                { "cap-only", 0x100u, 0u, 1 },
                { "2000000-only", 0u, 0x2000000u, 4 },
                { "both", 0u, 0x3000000u, 1 },
                { "1000000-only", 0u, 0x1000000u, 2 },
                { "neither", 0u, 0u, 0 },
            };
        for (unsigned a = 0; a < sizeof(arms) / sizeof(arms[0]); ++a) {
            cmd_init();
            EntitySlot probe;
            memset(&probe, 0, sizeof(probe));
            static uint8_t pimg[0x200];
            memset(pimg, 0, sizeof(pimg));
            const uint32_t pc = arms[a].cap;
            const uint32_t ps = arms[a].stats;
            memcpy(pimg + 0x14c, &pc, 4);
            memcpy(pimg + 0x144, &ps, 4);
            probe.stats_record = pimg;
            entity_kind_command_issue(&probe);
            void* head = cmd_global_head();
            char msg[96];
            snprintf(msg, sizeof(msg), "the %s arm lands the +0x3c word",
                     arms[a].what);
            int got = head != NULL ? cmd_rec_extra_get(head) : -1;
            CHECK(got == arms[a].want, msg);
            cmd_tick_global();
        }

        /* Bit 20: the capability bit plus the gate. */
        cmd_init();
        sctx.selection_count = 1;
        static EntitySlot one;
        memset(&one, 0, sizeof(one));
        sctx.selection[0] = &one;
        EntitySlot armed;
        memset(&armed, 0, sizeof(armed));
        static uint8_t aim[0x200];
        memset(aim, 0, sizeof(aim));
        const uint32_t acap = 0x100u | 0x40u;
        const uint32_t as0 = 0u;
        memcpy(aim + 0x14c, &acap, 4);
        memcpy(aim + 0x144, &as0, 4);
        armed.stats_record = aim;
        entity_kind_command_issue(&armed);
        CHECK((cmd_rec_state(cmd_global_head()) & 0x100000u) != 0u
                  && (cmd_rec_state(cmd_global_head()) & 0x140u) == 0x100u,
              "the armed record raises bit 20 and keeps bit 8");
        cmd_tick_global();
        cmd_init();
        armed.flags_3e4 = 0u;   /* unlatch for the gated run */
        armed.flags_3e0 = 0x400u; /* the gate blocks bit 20 */
        entity_kind_command_issue(&armed);
        CHECK((cmd_rec_state(cmd_global_head()) & 0x100000u) == 0u
                  && (cmd_rec_state(cmd_global_head()) & 0x140u) == 0x100u,
              "the gated record skips bit 20 (mode 1 half)");
        cmd_tick_global();
        sctx.selection_count = 0;
        sctx.selection[0] = NULL;

        /* The dispatch seams change no state. */
        cmd_init();
        coordinate_pair_callback_dispatch(&kinded);
        argument_callback_dispatch(&kinded);
        CHECK(cmd_global_head() == NULL, "the dispatch seams do nothing");
        cmd_context_set(&ctx);
    }

    /* The terrain cell-command issuers (2026-10-24). */
    {
        static uint8_t grid[8 * 8 * 0x48];
        GameContext lctx;
        game_context_init(&lctx);
        cmd_context_set(&lctx);
        static LevelState lvl;
        memset(&lvl, 0, sizeof(lvl));
        memset(grid, 0, sizeof(grid));
        lvl.word[0x14 / 4] = 8; /* LEVEL_OFF_WIDTH  */
        lvl.word[0x18 / 4] = 8; /* LEVEL_OFF_HEIGHT */
        lvl.ptr_voxels = grid;
        /* The terrain handle stays absent: a real level's loader creates
         * it (the fixture-dependent half), so the surface-type wiring is
         * covered by level_test's writer pair while the cell marks — the
         * observable terrain half here — are pinned below. */
        lvl.ptr_terrain = NULL;
        lctx.level = &lvl;

#define CELL(x, y) (grid + ((size_t)(y) * 8u + (size_t)(x)) * 0x48)
#define PUT32(x, y, off, v)                                                    \
    do {                                                                       \
        const uint32_t v_ = (uint32_t)(v);                                     \
        memcpy(CELL(x, y) + (off), &v_, 4);                                     \
    } while (0)
#define GET32(x, y, off) (*(const uint32_t*)(CELL(x, y) + (off)))

        /* Stage: (1,1) takes flag 4 plus the traversable bit; (2,2) state
         * 1; (3,3) state 6; (4,4) plain; (5,5) walkable; (6,6) flag 8. */
        PUT32(1, 1, 0x08, 0x14u);
        CELL(2, 2)[2] = 1;
        CELL(3, 3)[2] = 6;
        PUT32(5, 5, 0x08, 0x10u);
        PUT32(6, 6, 0x08, 8u);

        /* Type-8 create: every guard skips. */
        cmd_init();
        terrain_cell_command_create(2, 2, 1, 0);
        terrain_cell_command_create(3, 3, 1, 0);
        terrain_cell_command_create(4, 4, 1, 0);
        CHECK(cmd_global_head() == NULL, "the state-1/6 and plain cells skip");
        lctx.level = NULL;
        terrain_cell_command_create(1, 1, 1, 0);
        CHECK(cmd_global_head() == NULL, "the absent level skips (deviation)");
        lctx.level = &lvl;

        /* The set-flag arm: the terrain half stays out. */
        cmd_init();
        terrain_cell_command_create(1, 1, 1, 1);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 8,
              "the flagged cell issues the type-8 record");
        {
            int cx = -1, cy = -1;
            cmd_rec_cell_xy_get(cmd_global_head(), &cx, &cy);
            CHECK(cx == 1 && cy == 1, "the coordinates land as words");
            CHECK(cmd_rec_action_get(cmd_global_head()) == 0
                      && cmd_rec_target_get(cmd_global_head()) == -1,
                  "the create record leaves the action unset, target -1");
            CHECK(cmd_rec_kind(cmd_global_head()) == 3, "kind 3 links it");
            CHECK((cmd_rec_state(cmd_global_head()) & 0x20008u) == 0x20008u
                      && (cmd_rec_state(cmd_global_head()) & 0x80u) != 0u,
                  "the set flag raises bits 3 and 13 (and bit 7)");
        }
        CHECK((GET32(1, 1, 0x08) & 0x800000u) == 0u,
              "the set flag skips the terrain half");

        /* The clear-flag arm: bit 3 drops and the terrain half runs. */
        cmd_tick_global();
        cmd_init();
        terrain_cell_command_create(1, 1, 0, 0);
        CHECK((cmd_rec_state(cmd_global_head()) & 0x8u) == 0u
                  && (cmd_rec_state(cmd_global_head()) & 0x80u) != 0u
                  && (cmd_rec_state(cmd_global_head()) & 0x20000u) == 0u,
              "the clear flag raises bit 7 only");
        CHECK(cmd_rec_type(cmd_global_head()) == 8
                  && (GET32(1, 1, 0x08) & 0x800000u) != 0u,
              "the clear flag raises the cell's bit 0x800000");

        /* Kind-nine issue: the guards, then the full shape. */
        cmd_tick_global();
        cmd_init();
        CHECK(kind_nine_cell_command_issue(2, 2, 1) == 0,
              "the state-1 cell is refused");
        CHECK(kind_nine_cell_command_issue(6, 6, 1) == 0,
              "the unwalkable cell is refused");
        CHECK(kind_nine_cell_command_issue(5, 5, 1) == 1, "the cell issues");
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 9
                  && cmd_rec_action_get(cmd_global_head()) == 8
                  && cmd_rec_target_get(cmd_global_head()) == -1
                  && (cmd_rec_state(cmd_global_head()) & 0x400u) != 0u
                  && cmd_rec_kind(cmd_global_head()) == 3,
              "the type-9 record carries action 8 and bit 10");
        CHECK(level_cell_flag_reserved(&lvl, 5, 5) != 0u,
              "the issue marks the cell occupied");
        CHECK(kind_nine_cell_command_issue(5, 5, 0) == 0,
              "the occupied mark refuses the second issue");

        /* Type-0x14 issue: the bounded flag-8 read gates it. */
        cmd_tick_global();
        cmd_init();
        cell_command_type13_issue(4, 4);
        CHECK(cmd_global_head() == NULL, "the plain cell issues nothing");
        cell_command_type13_issue(6, 6);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x14
                  && cmd_rec_action_get(cmd_global_head()) == 0xd
                  && (cmd_rec_state(cmd_global_head()) & 0x400u) != 0u
                  && cmd_rec_kind(cmd_global_head()) == 3,
              "the flagged cell issues the type-0x14 record");

        /* Type-0xf issue: no guard, the secondary bit 3 mark. */
        cmd_tick_global();
        cmd_init();
        cell_command_type11_issue(3, 3);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0xf
                  && cmd_rec_action_get(cmd_global_head()) == 0xb
                  && cmd_rec_kind(cmd_global_head()) == 4
                  && level_cell_secondary_flag_8(&lvl, 3, 3) != 0u,
              "the type-0xf record links at kind 4 and marks the cell");
        cmd_tick_global();
        cmd_init();
        CHECK(cmd_rec_cell_xy_get(NULL, NULL, NULL) == 0,
              "the guarded reads answer zero");

        /* The missing terrain handle guards the surface half. */
        terrain_cell_command_create(1, 1, 0, 0);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 8,
              "the record lands even with the terrain handle absent");
        cmd_tick_global();
#undef CELL
#undef PUT32
#undef GET32
        lctx.level = NULL;
        cmd_init();
        cmd_context_set(&ctx);
    }

    /* The task-type-23 category-mask issuer (2026-10-24). */
    {
        GameContext kctx;
        game_context_init(&kctx);
        cmd_context_set(&kctx);
        cmd_init();

        EntitySlot worker;
        memset(&worker, 0, sizeof(worker));
        static uint8_t wimg[0x200];
        memset(wimg, 0, sizeof(wimg));
        uint32_t wflags = 0x84u; /* bits 4 and 8 (selectors 1 and 0x10) */
        memcpy(wimg + 0x148, &wflags, 4);
        worker.stats_record = wimg;

        /* The gate: an unset bit refuses, an out-of-map selector refuses. */
        task_type23_create_for_category_mask(&worker, 4, 0);
        CHECK(cmd_global_head() == NULL, "an unset state bit refuses");
        task_type23_create_for_category_mask(&worker, 3, 1);
        CHECK(cmd_global_head() == NULL, "selector 3 is out of the map");
        task_type23_create_for_category_mask(NULL, 1, 1);
        CHECK(cmd_global_head() == NULL, "the guarded entity answers nothing");

        /* Selector 1: type 0x17, category kind 0xc, bit 6 clear. */
        task_type23_create_for_category_mask(&worker, 1, 0);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x17
                  && cmd_rec_payload(cmd_global_head()) == &worker
                  && cmd_rec_kind(cmd_global_head()) == 0xc
                  && (cmd_rec_state(cmd_global_head()) & 0x40u) == 0u,
              "selector 1 links the task at kind 0xc");
        cmd_tick_global();
        cmd_init();

        /* The selector-to-kind map, staged bit by bit. */
        struct { int sel; int kind; uint32_t flag; } map[] = {
            { 1, 0xc, 4u }, { 2, 0xc, 8u }, { 4, 0xc, 0x10u },
            { 8, 3, 0x20u }, { 0x10, 7, 0x40u }, { 0x20, 4, 0x80u },
        };
        for (unsigned m = 0; m < sizeof(map) / sizeof(map[0]); ++m) {
            cmd_init();
            memcpy(wimg + 0x148, &map[m].flag, 4);
            task_type23_create_for_category_mask(&worker, map[m].sel, 0);
            CHECK(cmd_global_head() != NULL
                      && cmd_rec_type(cmd_global_head()) == 0x17
                      && cmd_rec_extra_get(cmd_global_head()) == map[m].sel
                      && cmd_rec_kind(cmd_global_head()) == map[m].kind,
                  "the selector lands its kind and the +0x3c selector");
            task_type23_create_for_category_mask(&worker, 0, 0);
            task_type23_create_for_category_mask(&worker, 0x21, 0);
            cmd_tick_global();
        }
        /* Out-of-map selectors never pass the gate. */
        cmd_init();
        const uint32_t all = 0xFCu;
        memcpy(wimg + 0x148, &all, 4);
        task_type23_create_for_category_mask(&worker, 0, 1);
        task_type23_create_for_category_mask(&worker, 3, 1);
        task_type23_create_for_category_mask(&worker, 0x100, 1);
        CHECK(cmd_global_head() == NULL,
              "out-of-map selectors issue nothing");
        cmd_tick_global();

        /* The record-array half rides the caller argument: bit 8 raised,
         * bit 6 still clear after the link. */
        cmd_init();
        static EntitySlot picked;
        memset(&picked, 0, sizeof(picked));
        kctx.selection[0] = &picked;
        kctx.selection_count = 1;
        const uint32_t h4 = 4u;
        memcpy(wimg + 0x148, &h4, 4);
        task_type23_create_for_category_mask(&worker, 1, 1);
        CHECK((cmd_rec_state(cmd_global_head()) & 0x140u) == 0x100u
                  && cmd_rec_extra_get(cmd_global_head()) == 1
                  && cmd_rec_count_get(cmd_global_head()) == 1u,
              "the set argument runs the record-array half (bit 8 survives)");
        cmd_tick_global();
        kctx.selection_count = 0;
        kctx.selection[0] = NULL;
        cmd_init();
        cmd_context_set(&ctx);
    }

    /* The task-kind filtered search (2026-10-24). */
    {
        GameContext pctx;
        game_context_init(&pctx);
        cmd_context_set(&pctx);
        cmd_init();

        /* Kind support: the stats gate, the unconditional arms, the misses. */
        EntitySlot hands;
        memset(&hands, 0, sizeof(hands));
        static uint8_t himg[0x200];
        memset(himg, 0, sizeof(himg));
        uint32_t w3 = 0u; /* bit 3 clear: everything misses */
        memcpy(himg + 0x144, &w3, 4);
        hands.stats_record = himg;
        CHECK(entity_action_kind_supported(&hands, 9, 0) == 0
                  && entity_action_kind_supported(&hands, 8, 0) == 0
                  && entity_action_kind_supported(&hands, 7, 1) == 0,
              "the clear stats word misses every arm");
        CHECK(entity_action_kind_supported(NULL, 9, 0) == 0,
              "the guarded entity answers nothing");
        w3 = 8u;
        memcpy(himg + 0x144, &w3, 4);
        CHECK(entity_action_kind_supported(&hands, 9, 0) != 0
                  && entity_action_kind_supported(&hands, 8, 0) != 0
                  && entity_action_kind_supported(&hands, 0xb, 0) != 0
                  && entity_action_kind_supported(&hands, 0xd, 0) != 0,
              "the gated arms answer unconditionally");
        CHECK(entity_action_kind_supported(&hands, 3, 0) == 0
                  && entity_action_kind_supported(&hands, 6, 0) == 0,
              "kind 3 and the no-op arm miss");
        /* Kind 7: the upgrade counters — no level, selector pick, misses. */
        CHECK(entity_action_kind_supported(&hands, 7, 0) == 0,
              "no level answers zero for kind 7");
        static LevelState uplvl;
        memset(&uplvl, 0, sizeof(uplvl));
        pctx.level = &uplvl;
        uplvl.word[0xA0 / 4] = 7;
        CHECK(entity_action_kind_supported(&hands, 7, 0) != 0
                  && entity_action_kind_supported(&hands, 7, 1) == 0,
              "selector zero reads counter A");
        uplvl.word[0xAC / 4] = 9;
        CHECK(entity_action_kind_supported(&hands, 7, 1) != 0
                  && entity_action_kind_supported(&hands, 7, 5) != 0,
              "the other selectors read counter B");
        uplvl.word[0xA0 / 4] = 0;
        uplvl.word[0xAC / 4] = 0;
        CHECK(entity_action_kind_supported(&hands, 7, 3) == 0,
              "the zeroed counters miss");
        pctx.level = NULL;

        /* The callback half follows the same gate and forwards the kind. */
        CHECK(task_entity_flags_and_cell_task_match(&hands, 9) != 0,
              "the callback takes the gated kind");
        const uint32_t cleared = 0u;
        memcpy(himg + 0x144, &cleared, 4);
        CHECK(task_entity_flags_and_cell_task_match(&hands, 9) == 0,
              "the callback's gate blocks");
        w3 = 8u;
        memcpy(himg + 0x144, &w3, 4);

        /* The search: 1/2/4 never search. */
        CHECK(task_kind_filtered_search(1) == 0
                  && task_kind_filtered_search(2) == 0
                  && task_kind_filtered_search(4) == 0,
              "the excluded selectors never search");

        /* The empty pool misses. */
        CHECK(task_kind_filtered_search(9) == 0, "the empty pool misses");

        /* One live entity with the gated word answers selector 9. */
        pctx.entity_count[0] = 1;
        pctx.entity[0][0] = hands;
        pctx.entity[0][0].type = 2;
        CHECK(task_kind_filtered_search(9) != 0, "the live slot answers 9");
        CHECK(task_kind_filtered_search(3) == 0, "kind 3 misses");
        CHECK(task_kind_filtered_search(6) == 0, "the no-op arm misses");
        const uint32_t off = 0u;
        memcpy(himg + 0x144, &off, 4);
        CHECK(task_kind_filtered_search(9) == 0, "the ungated slot misses");

        /* Dead slots skip; the second table reaches too. */
        w3 = 8u;
        memcpy(himg + 0x144, &w3, 4);
        pctx.entity[0][0].type = 0; /* dead, count still covers it */
        pctx.entity_count[1] = 2;
        pctx.entity[1][1] = hands;
        pctx.entity[1][1].type = 2;
        CHECK(task_kind_filtered_search(8) != 0, "the second table answers");

        /* Kind 7 through the search: the level counters forward. */
        pctx.entity[1][1] = hands;
        pctx.entity[1][1].type = 2; /* the copy dropped the live marker */
        memcpy(himg + 0x144, &w3, 4);
        pctx.level = &uplvl; /* counters still zero */
        CHECK(task_kind_filtered_search(7) == 0, "kind 7 misses on counters");
        uplvl.word[0xA0 / 4] = 1;
        CHECK(task_kind_filtered_search(7) != 0,
              "kind 7 answers through the counter");
        uplvl.word[0xA0 / 4] = 0;
        pctx.level = NULL;
        pctx.entity_count[0] = 0;
        pctx.entity_count[1] = 0;
        cmd_init();
        cmd_context_set(&ctx);
    }

    /* The task-record queue producers (2026-10-24). */
    {
        GameContext qctx;
        game_context_init(&qctx);
        cmd_context_set(&qctx);
        cmd_init();
        EntitySlot host;
        memset(&host, 0, sizeof(host));
        host.type = 2;
        static EntitySlot picked;
        memset(&picked, 0, sizeof(picked));
        qctx.selection[0] = &picked;
        qctx.selection_count = 1;

#define CHAIN_COUNT(out)                                                     \
    do {                                                                     \
        int n_ = 0;                                                          \
        for (void* cur_ = cmd_global_head(); cur_ != NULL;                   \
             cur_ = cmd_global_next(cur_)) {                                 \
            ++n_;                                                            \
        }                                                                    \
        out = n_;                                                            \
    } while (0)

        /* Coordinate-pair event queue. */
        const int pair[2] = { 11, 22 };
        coordinate_pair_event_queue(pair, &host);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0xc
                  && cmd_rec_kind(cmd_global_head()) == 8,
              "the pair event queue links command 8");
        {
            int cx = -1, cy = -1;
            cmd_rec_cell_xy_get(cmd_global_head(), &cx, &cy);
            CHECK(cx == 11 && cy == 22
                      && cmd_rec_event_payload_get(cmd_global_head())
                             == &host,
                  "the pair and the +0x54 payload land");
        }
        cmd_tick_global();
        cmd_init();

        /* Object event type-2 queue: selection half plus the flag shape. */
        CHECK(object_event_type2_queue(&host) != 0, "the queue answers one");
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x13
                  && cmd_rec_kind(cmd_global_head()) == 2
                  && (cmd_rec_state(cmd_global_head()) & 0x140u) == 0x100u
                  && (cmd_rec_state(cmd_global_head())
                          & CMD_STATE_PENDING) != 0u
                  && cmd_rec_count_get(cmd_global_head()) == 1u,
              "bit 8 rides the selection, bit 6 stays clear, bit 4 is up");
        cmd_tick_global();
        cmd_init();

        /* The tail-landing producers. */
        const int tp[2] = { 5, 6 };
        coordinate_pair_task_record_queue(tp);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0
                  && cmd_rec_kind(cmd_global_head()) == 0x1a
                  && (cmd_rec_state(cmd_global_head()) & 0x50u) == 0x40u,
              "the pair task record tails at 0x1a (bit 6, no bit 4)");
        {
            int cx = -1, cy = -1;
            cmd_rec_cell_xy_get(cmd_global_head(), &cx, &cy);
            CHECK(cx == 5 && cy == 6, "the pair lands as words");
        }
        cmd_tick_global();
        cmd_init();
        scalar_task_record_queue(&host);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x1d
                  && cmd_rec_kind(cmd_global_head()) == 0x19
                  && cmd_rec_payload(cmd_global_head()) == &host
                  && (cmd_rec_state(cmd_global_head()) & 0x40u) != 0u,
              "the scalar record tails at 0x19 with bit 6");
        cmd_tick_global();
        cmd_init();
        const int pp[2] = { 7, 8 };
        task_pair_record_enqueue(pp);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x19
                  && cmd_rec_kind(cmd_global_head()) == 0x18,
              "the pair record tails at command 0x18");
        {
            int cx = -1, cy = -1;
            cmd_rec_cell_xy_get(cmd_global_head(), &cx, &cy);
            CHECK(cx == 7 && cy == 8, "the enqueue pair lands");
        }
        cmd_tick_global();
        cmd_init();
        task_record_kind10_queue(&host);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 10
                  && cmd_rec_kind(cmd_global_head()) == 0x16
                  && cmd_rec_payload(cmd_global_head()) == &host
                  && (cmd_rec_state(cmd_global_head()) & 0x40u) != 0u,
              "the kind-10 record tails at 0x16 with bit 6");
        cmd_tick_global();
        cmd_init();

        /* The pair removal: first match only, interior and head shapes. */
        const int r1[2] = { 1, 1 }, r2[2] = { 2, 2 }, r3[2] = { 3, 3 };
        coordinate_pair_task_record_queue(r1);
        coordinate_pair_task_record_queue(r2);
        coordinate_pair_task_record_queue(r3);
        task_registry_remove_type0_entry_by_pair(r2);
        int cnt = 0;
        CHAIN_COUNT(cnt);
        CHECK(cnt == 2, "the interior removal dropped exactly the match");
        task_registry_remove_type0_entry_by_pair(r1);
        {
            int cx = -1, cy = -1;
            int cnt2 = 0;
            CHAIN_COUNT(cnt2);
            cmd_rec_cell_xy_get(cmd_global_head(), &cx, &cy);
            CHECK(cnt2 == 1 && cx == 3 && cy == 3,
                  "the head removal exposes the tail");
        }
        task_registry_remove_type0_entry_by_pair(r3);
        CHECK(cmd_global_head() == NULL, "the last record goes");
        task_registry_remove_type0_entry_by_pair(r3); /* empty chain */
        task_registry_remove_type0_entry_by_pair(NULL); /* guarded */
        cmd_tick_global();
        cmd_init();

        /* The kind-10 entity removal: every match. */
        task_record_kind10_queue(&host);
        task_record_kind10_queue(&picked);
        task_record_kind10_queue(&host);
        task_registry_remove_type10_entries_for_entity(&host);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_payload(cmd_global_head()) == &picked
                  && cmd_global_next(cmd_global_head()) == NULL,
              "only the other entity's entry survives");
        task_registry_remove_type10_entries_for_entity(&picked);
        CHECK(cmd_global_head() == NULL, "the last kind-10 entries go");
        cmd_tick_global();
        cmd_init();

        /* The timed pair. */
        void* timed = timed_task_record_initialize_and_link(&host, 1, -1, 3);
        CHECK(timed != NULL
                  && cmd_rec_type(timed) == 5
                  && cmd_rec_kind(timed) == 10
                  && cmd_rec_payload(timed) == &host
                  && cmd_rec_action_get(timed) == 1
                  && cmd_rec_target_get(timed) == -1
                  && cmd_rec_cell_count_get(timed) == 3
                  && (cmd_rec_state(timed) & 0x4d0u) == 0x4c0u,
              "the timed record links command 10 with bits 6/7/10");
        cmd_tick_global();
        cmd_init();
        task_record_kind11_timed_queue(&host);
        CHECK(cmd_global_head() != NULL
                  && cmd_rec_type(cmd_global_head()) == 0x11
                  && cmd_rec_kind(cmd_global_head()) == 0
                  && cmd_rec_payload(cmd_global_head()) == &host
                  && (cmd_rec_state(cmd_global_head()) & 0xd0u) == 0xc0u,
              "the kind-11 timed record links command 0 with bits 6/7");
        cmd_tick_global();
        qctx.selection_count = 0;
        qctx.selection[0] = NULL;
        cmd_init();
#undef CHAIN_COUNT
        cmd_context_set(&ctx);
    }

    if (s_failures != 0) {
        fprintf(stderr, "cmd_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("cmd_test: OK\n");
    return 0;
}
