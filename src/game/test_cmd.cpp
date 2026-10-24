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

    if (s_failures != 0) {
        fprintf(stderr, "cmd_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("cmd_test: OK\n");
    return 0;
}
