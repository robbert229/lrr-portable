// SPDX-License-Identifier: LGPL-3.0-or-later
// Selection manager + pointer-to-command wiring (finding 06).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"
#include "game/select.hpp"

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

    /* Bad-type add rejected; type codes above 4 share the last table. */
    CHECK(entity_create(&ctx, 6, "m6", 0, 0, 0, 0) >= 0, "minion type 6 placed");
    CHECK(entity_create(&ctx, 10, "nope", 0, 0, 0, 0) < 0, "type 10 rejected");

    if (s_failures != 0) {
        fprintf(stderr, "select_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("select_test: OK\n");
    return 0;
}
