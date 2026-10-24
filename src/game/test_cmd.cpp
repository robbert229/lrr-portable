// SPDX-License-Identifier: LGPL-3.0-or-later
// Entity command primitives + queue executor (finding 10).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/frontend.hpp"
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

    /* Pending entity-reference clear (2026-10-25): the global walk zeroes
     * every record holding the matching reference and rides past the
     * rest. */
    {
        void* sentinel = (void*)0xa5a5u;
        void* other = (void*)0x5a5au;
        int touched = 0;
        for (void* rec = cmd_global_head(); rec != NULL;
             rec = cmd_global_next(rec)) {
            cmd_rec_entity_ref_install(rec, sentinel);
            ++touched;
        }
        CHECK(touched > 0, "the global list carries the staged records");
        void* keeper = cmd_global_head();
        cmd_rec_entity_ref_install(keeper, other);
        pending_record_entity_reference_clear(sentinel);
        int hits = 0, kept = 0;
        for (void* rec = cmd_global_head(); rec != NULL;
             rec = cmd_global_next(rec)) {
            if (cmd_rec_entity_ref(rec) == other) {
                ++kept;
            } else if (cmd_rec_entity_ref(rec) == sentinel) {
                ++hits;
            }
        }
        CHECK(hits == 0, "the matching references are all cleared");
        CHECK(kept == 1 && cmd_rec_entity_ref(keeper) == other,
              "the non-matching reference rides past");
        pending_record_entity_reference_clear(NULL);
        CHECK(cmd_rec_entity_ref(keeper) == other,
              "the zero reference rides past too");
    }

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

    if (s_failures != 0) {
        fprintf(stderr, "cmd_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("cmd_test: OK\n");
    return 0;
}
