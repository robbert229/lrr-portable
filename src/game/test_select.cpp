// SPDX-License-Identifier: LGPL-3.0-or-later
// Selection manager + pointer-to-command wiring (finding 06).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/frontend.hpp"
#include "game/game.hpp"
#include "game/select.hpp"
#include "gods98/engine.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static EntitySlot* place(GameContext* ctx, int type, const char* name,
                         float x, float y)
{
    int idx = entity_create(ctx, type, name, x, y, 0, 0);
    CHECK(idx >= 0, "entity_create");
    return (EntitySlot*)entity_at(ctx, type, idx);
}

int main(void)
{
    cmd_init();
    GameContext ctx;
    game_context_init(&ctx);
    cmd_context_set(&ctx);

    EntitySlot* a = place(&ctx, 1, "diggerA", 3, 4);
    EntitySlot* b = place(&ctx, 2, "minerB", 8, 8);
    EntitySlot* c = place(&ctx, 4, "wallC", 20, 20);

    /* Add / dedupe / cap. */
    CHECK(selection_add(&ctx, a) == 1, "add a");
    CHECK(selection_add(&ctx, a) == 0, "dedupe rejects a");
    CHECK(selection_add(&ctx, b) == 1, "add b");
    CHECK(selection_count(&ctx) == 2, "count 2");
    CHECK(selection_first(&ctx) == a, "selection_first is a");
    CHECK((a->flags & ENTITY_SELECTED) != 0, "a selected flag");

    /* Pick nearest within tolerance. */
    CHECK(selection_pick(&ctx, 3, 4, 2.0f) == a, "pick a at its cell");
    CHECK(selection_pick(&ctx, 8, 8, 1.0f) == b, "pick b at its cell");
    CHECK(selection_pick(&ctx, 50, 50, 5.0f) == NULL, "no entity near (50,50)");

    /* Pointer command: move selection to a cell -> queue -> dispatch. */
    int n = selection_cmd_move(&ctx, 5, 5);
    CHECK(n == 2, "two moves enqueued");
    cmd_queue_tick();
    CHECK(a->cmd_head != NULL && cmd_rec_type(a->cmd_head) == 0,
          "move dispatched on a");
    CHECK(b->cmd_head != NULL && cmd_rec_type(b->cmd_head) == 0,
          "move dispatched on b");
    CHECK(c->cmd_head == NULL, "unselected entity untouched");

    /* Op-8 selection-add through the queue. */
    selection_cmd_add(&ctx, c);
    cmd_queue_tick();
    CHECK((c->flags & ENTITY_SELECTED) != 0, "op-8 added c to selection");
    CHECK(selection_count(&ctx) == 3, "selection now 3");

    /* Clear releases the selected flags. */
    selection_clear(&ctx);
    CHECK(selection_count(&ctx) == 0, "cleared");
    CHECK((a->flags & ENTITY_SELECTED) == 0, "a flag cleared");
    CHECK(selection_first(&ctx) == NULL, "empty selection first is NULL");

    /* Record accessors (FUN_004525b0/5e0/5f0/600). */
    CHECK(selection_has_any(&ctx) == 0, "empty set answers no");
    CHECK(selection_record_array(&ctx) == ctx.selection,
          "the array getter hands back the selection array");
    CHECK(selection_add(&ctx, a) == 1 && selection_add(&ctx, b) == 1,
          "re-add a and b");
    CHECK(selection_has_any(&ctx) == 1, "nonzero count answers yes");
    CHECK(selection_count(&ctx) == 2, "record count getter reads 2");
    EntitySlot* const* arr = selection_record_array(&ctx);
    CHECK(arr != NULL && arr[0] == a && arr[1] == b,
          "the array reads in slot order");
    selection_count_reset(&ctx);
    CHECK(selection_count(&ctx) == 0 && (a->flags & ENTITY_SELECTED) != 0,
          "count-only reset leaves the selected flags");
    selection_add(&ctx, a);
    selection_add(&ctx, b);
    selection_clear(&ctx);

    /* Bad-type add rejected; special codes (5..19) share the last table. */
    CHECK(entity_create(&ctx, 6, "m6", 0, 0, 0, 0) >= 0, "special code 6 placed");
    CHECK(entity_create(&ctx, 20, "nope", 0, 0, 0, 0) < 0, "code 20 (out of range) rejected");

    /* The candidate leaves (2026-10-24): the materialized dedupe, the
     * already-selected-or-marked gate, and the eligibility predicate. */
    {
        GameContext empty;
        game_context_init(&empty);
        CHECK(selection_contains_entity(&empty, a) == 0, "empty: absent");
        CHECK(selection_contains_entity(NULL, a) == 0, "NULL ctx: absent");
        selection_clear(&ctx);
        selection_add(&ctx, b);
        CHECK(selection_contains_entity(&ctx, b) == 1, "the added entity");
        CHECK(selection_contains_entity(&ctx, a) == 0, "the other absent");

        CHECK(selection_candidate_already_selected_or_marked(&ctx, b) == 1,
              "selected counts as taken");
        CHECK(selection_candidate_already_selected_or_marked(&ctx, a) == 0,
              "the unselected/unmarked answers 0");
        CHECK(selection_candidate_already_selected_or_marked(&ctx, NULL) == 0,
              "NULL rejects (deviation)");

        EntitySlot marked;
        memset(&marked, 0, sizeof(marked));
        marked.flags_3ec = 2u;
        CHECK(selection_candidate_already_selected_or_marked(&ctx, &marked) == 1,
              "the +0x3ec mark satisfies it unselected");

        /* Eligibility: minimal pass shape, then each rejection. */
        EntitySlot e;
        memset(&e, 0, sizeof(e));
        e.type = 4;
        e.flags_3e8 = 0x100u;
        CHECK(selection_candidate_eligibility_check(&e) == 1,
              "the state-gated entity passes");
        e.flags_3e8 = 0u;
        CHECK(selection_candidate_eligibility_check(&e) == 0,
              "the state bit is required");
        e.flags_3e8 = 0x100u;
        e.flags_3e4 = 2u; /* corpus: low byte & 10 */
        CHECK(selection_candidate_eligibility_check(&e) == 0,
              "the +0x3e4 low-byte band rejects");
        e.flags_3e4 = 0u;
        e.flags_3e0 = 0x04000000u; /* bit 26: in the 0x24 band */
        CHECK(selection_candidate_eligibility_check(&e) == 0,
              "the +0x3e0 band rejects");
        e.flags_3e0 = 0u;
        e.type = 1;
        CHECK(selection_candidate_eligibility_check(&e) == 1,
              "type 1 passes clean");
        e.flags_3e0 = 0x40000u;
        CHECK(selection_candidate_eligibility_check(&e) == 0,
              "type 1's extra 0x40000 rejects (finding 468)");
        e.flags_3e0 = 0u;
        e.type = 2;
        CHECK(selection_candidate_eligibility_check(&e) == 1
                  && (selection_candidate_eligibility_check(&e) != 0),
              "the 0x40000 branch is type-1-only");
        CHECK(selection_candidate_eligibility_check(NULL) == 0,
              "NULL rejects (deviation)");
    }

    if (s_failures != 0) {
        fprintf(stderr, "select_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("select_test: OK\n");
    return 0;
}
