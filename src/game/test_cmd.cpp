// SPDX-License-Identifier: LGPL-3.0-or-later
// Entity command primitives + queue executor (finding 10).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"

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

    if (s_failures != 0) {
        fprintf(stderr, "cmd_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("cmd_test: OK\n");
    return 0;
}
