// SPDX-License-Identifier: LGPL-3.0-or-later
// Selection manager + pointer-to-command wiring (finding 06).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/level.hpp"
#include "game/frontend.hpp"
#include "game/game.hpp"
#include "game/select.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

#define LEVEL_DIM 64

/* A LEVEL_DIM x LEVEL_DIM all-zero surface map. */
static void write_surface_map(const char* path)
{
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        return;
    }
    const char magic[4] = { 'M', 'A', 'P', ' ' };
    const uint32_t len = LEVEL_DIM * LEVEL_DIM * 2, w = LEVEL_DIM, h = LEVEL_DIM;
    static const uint16_t cells[LEVEL_DIM * LEVEL_DIM] = { 0 };
    fwrite(magic, 1, 4, fp);
    fwrite(&len, 1, 4, fp);
    fwrite(&w, 1, 4, fp);
    fwrite(&h, 1, 4, fp);
    fwrite(cells, 1, sizeof(cells), fp);
    fclose(fp);
}

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

    /* The faithful pick / move / HUD paths resolve entity positions and
     * cells through the context's level terrain, so give the context a
     * scratch level (a real terrain over a written surface map; file opens
     * resolve under the CD root, the fixture directory). */
    system("rm -rf select_fixture");
    mkdir("select_fixture", 0755);
    write_surface_map("select_fixture/L01_surface.map");
    if (chdir("select_fixture") != 0) {
        fprintf(stderr, "select_test: chdir failed\n");
        return 1;
    }
    resource_set_cd_root(".");
    resource_set_cd_data_dir(".");
    void* const level_terrain = terrain_create("L01_surface.map", 1.0f);
    CHECK(level_terrain != NULL, "scratch terrain");
    static LevelState level;
    static uint8_t level_grid[LEVEL_DIM * LEVEL_DIM * LEVEL_VOXEL_STRIDE];
    memset(&level, 0, sizeof(level));
    level_set_int(&level, LEVEL_OFF_WIDTH, LEVEL_DIM);
    level_set_int(&level, LEVEL_OFF_HEIGHT, LEVEL_DIM);
    level.ptr_voxels = level_grid;
    level.ptr_terrain = level_terrain;
    ctx.level = &level;

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
    /* Picks are in grid cells: the entities' world positions map through the
     * level's terrain, so read each one's grid position back first. */
    float ga[2] = { 0, 0 }, gb[2] = { 0, 0 };
    CHECK(entity_position_grid_coordinates_get(a, ctx.level, ga) != 0
              && entity_position_grid_coordinates_get(b, ctx.level, gb) != 0,
          "grid coordinates resolve");
    CHECK(selection_pick(&ctx, ga[0], ga[1], 2.0f) == a, "pick a at its cell");
    CHECK(selection_pick(&ctx, gb[0], gb[1], 1.0f) == b, "pick b at its cell");
    CHECK(selection_pick(&ctx, 62, 62, 1.0f) == NULL, "no entity near (62,62)");

    /* Pointer command: move selection to a cell -> queue -> dispatch. */
    int n = selection_cmd_move(&ctx, 5, 5);
    CHECK(n == 2, "two moves enqueued");
    cmd_queue_tick();
    CHECK(a->cmd_head != NULL && cmd_rec_type(a->cmd_head) == 0,
          "move dispatched on a");
    CHECK(b->cmd_head != NULL && cmd_rec_type(b->cmd_head) == 0,
          "move dispatched on b");
    CHECK(c->cmd_head == NULL, "unselected entity untouched");

    /* Selection-add uses operation 1 and the actual eligibility owner. */
    c->flags_3e8 |= 0x100u;
    c->field_340 = 100.0f;
    selection_cmd_add(&ctx, c);
    cmd_queue_tick();
    CHECK((c->flags & ENTITY_SELECTED) != 0, "op-1 added c to selection");
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
    /* The count-only reset leaves the +0x3e8 selected bits (and the active
     * records are gone), so no later clear can release them and the faithful
     * add refuses to re-select: release them by hand. */
    a->flags_3e8 &= ~ENTITY_SELECTED;
    b->flags_3e8 &= ~ENTITY_SELECTED;
    a->flags &= ~ENTITY_SELECTED;
    b->flags &= ~ENTITY_SELECTED;
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
        /* game_context_init installs `empty` as the process-wide command and
         * entity-pool owner; take them back before it leaves scope. */
        cmd_context_set(&ctx);
        entity_pool_callback_context_set(&ctx);
        selection_clear(&ctx);
        selection_add(&ctx, b);
        CHECK(selection_contains_entity(&ctx, b) == 1, "the added entity");
        CHECK(selection_contains_entity(&ctx, a) == 0, "the other absent");

        CHECK(selection_candidate_already_selected_or_marked(&ctx, b) == 1,
              "selected counts as taken");
        /* selection_clear leaves the +0x3ec "recently selected" mark (bit 2)
         * on every entity it released; drop a's to model an unmarked one. */
        a->flags_3ec &= ~2u;
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

    /* CurrentLevelStateFlagsContainMask (2026-10-24). */
    {
        selection_clear(&ctx);
        CHECK(current_level_state_flags_contain_mask(&ctx, 1u) == 0,
              "no selection answers the mask false");
        CHECK(current_level_state_flags_contain_mask(NULL, 1u) == 0,
              "the null context answers false");
        selection_add(&ctx, a); /* re-open the cleared selection */
        a->flags_3f0 = 0x11u;
        CHECK(current_level_state_flags_contain_mask(&ctx, 1u) != 0
                  && current_level_state_flags_contain_mask(&ctx, 2u) == 0,
              "the mask ANDs the first entry's word");
        a->flags_3f0 = 0u; /* zero-init mirror: back to the shipped state */
        selection_clear(&ctx);
    }

    /* SelectionClearMarkerAndResetHud (2026-10-25): gate + raise pair. */
    {
        EngineContext eng = {};
        static uint8_t rec[0x150];
        memset(rec, 0, sizeof rec);
        a->flags_3ec = 0u;
        a->owner_36c = NULL;
        a->stats_record = rec;

        /* Unmarked: the whole raise path is skipped, inert. */
        selection_clear_marker_and_reset_hud(&eng, a);
        CHECK((eng.state & 0x10000000u) == 0u && a->flags_3ec == 0u,
              "the unmarked entity raises nothing");

        /* Faithful gate (FUN_0044c4a0): a marked entity needs the state-flags
         * word (+0x148) bit 0x20000 AND (the stats word (+0x144) bit 0x20000
         * clear OR an owner). Marked, state bit set, stats bit clear: both
         * raises fire. */
        a->flags_3ec = 2u;
        rec[0x14a] = 0x02u; /* state-flags word @+148 bit 0x20000 */
        selection_clear_marker_and_reset_hud(&eng, a);
        CHECK((eng.state & 0x10000000u) != 0u && (a->flags_3ec & 3u) == 3u,
              "the marked state-flagged entity raises both words");

        /* No state-flags bit: nothing raises even when marked. */
        eng.state = 0u;
        a->flags_3ec = 2u;
        rec[0x14a] = 0u;
        selection_clear_marker_and_reset_hud(&eng, a);
        CHECK((eng.state & 0x10000000u) == 0u && a->flags_3ec == 2u,
              "the marked entity without the state bit raises nothing");

        /* State bit + stats bit set + null owner: no raise. */
        rec[0x14a] = 0x02u;
        rec[0x146] = 0x02u; /* stats word @+144 bit 0x20000 */
        selection_clear_marker_and_reset_hud(&eng, a);
        CHECK((eng.state & 0x10000000u) == 0u && a->flags_3ec == 2u,
              "the set stats bit with no owner skips the raise");

        /* Same, owner set: the owner half of the fold raises. */
        static const int owner = 1;
        a->owner_36c = &owner;
        selection_clear_marker_and_reset_hud(&eng, a);
        CHECK((eng.state & 0x10000000u) != 0u && (a->flags_3ec & 1u) != 0u,
              "the owner present raises through the fold");
        a->owner_36c = NULL;
        a->flags_3ec = 0u;
        a->stats_record = NULL;
        memset(rec, 0, sizeof rec);

        selection_clear_marker_and_reset_hud(&eng, NULL);
        selection_clear_marker_and_reset_hud(NULL, a);
        CHECK(1, "the null handle and null entity stay inert");
    }

    /* SelectionStateTick (2026-10-25): the class ladder, the HUD context
     * post, and the compaction pass. Stats ride the zero template
     * (NULL record) unless pinned. */
    {
        static uint32_t bad_rec[0x150 / 4];   /* stats +0x144: bit 0x2000000 */
        static uint32_t flag_rec[0x150 / 4];  /* stats +0x144: bit 0x20000 */
        static uint32_t both_rec[0x150 / 4];  /* stats +0x144: both bits */
        bad_rec[0x144 / 4] = 0x2000000u;
        flag_rec[0x144 / 4] = 0x20000u;
        both_rec[0x144 / 4] = 0x02020000u; /* bit 17 | bit 25 */
        static const int sentinel = 1;

        EntitySlot* clean1 = place(&ctx, 1, "cleanTick1", 30, 5);
        EntitySlot* bad25 = place(&ctx, 1, "badTick1", 31, 5);
        EntitySlot* ownedFlag = place(&ctx, 1, "ownedTick1", 32, 5);
        EntitySlot* flaggedBad = place(&ctx, 1, "flagTick1", 36, 5);
        EntitySlot* noOwnerFlag = place(&ctx, 1, "freeTick1", 33, 5);
        EntitySlot* gen3 = place(&ctx, 3, "genTick3", 34, 5);
        EntitySlot* miner11 = place(&ctx, 11, "minerTick11", 35, 5);
        bad25->stats_record = bad_rec;
        ownedFlag->stats_record = flag_rec;
        ownedFlag->owner_36c = (void*)&sentinel;
        flaggedBad->stats_record = both_rec;
        flaggedBad->owner_36c = (void*)&sentinel;
        noOwnerFlag->stats_record = flag_rec;
        miner11->flags_3e4 = 0x2000000u;
        /* (selection_add raises +0x3e8 itself; pre-setting it makes the
         * faithful add refuse the entry.) */

        /* Empty selection: nothing classifies, context 0, nothing to remove. */
        selection_state_tick(&ctx);
        CHECK(selection_count(&ctx) == 0 && map_store_kind_peek() == 0u,
              "the empty selection posts 0");

        /* Kind-2 class: the unowned kind 2 wins the ladder, compaction
         * revisits the swapped-in slot, and only kind 2 survives. */
        selection_clear(&ctx);
        selection_add(&ctx, clean1);
        selection_add(&ctx, b); /* the file's type-2 entity, unowned */
        clean1->flags_3e8 |= ENTITY_SELECTED;
        selection_state_tick(&ctx);
        CHECK(map_store_kind_peek() == 9u, "kind 2 posts context 9");
        CHECK(selection_count(&ctx) == 1 && ctx.selection[0] == b,
              "only the kind-2 entry survives the compaction");
        CHECK((clean1->flags_3e8 & ENTITY_SELECTED) == 0,
              "the dropped entry loses the selected bit");

        /* Kind-1 flagged class: the owner-set bit-17+bit-25 rider rides the
         * bad class alongside the bit-25 kind 1 (both survive). */
        selection_clear(&ctx);
        selection_add(&ctx, flaggedBad);
        selection_add(&ctx, bad25);
        selection_state_tick(&ctx);
        CHECK(map_store_kind_peek() == 10u, "the flagged kind 1 posts 10");
        CHECK(selection_count(&ctx) == 2,
              "both kind-1 flagged shapes survive the flagged class");

        /* Kind-1 clean class: the owner-set bit-17 entry is a clean-class
         * rider, and the unowned bit-17 entry drops out of it. */
        selection_clear(&ctx);
        selection_add(&ctx, clean1);
        selection_add(&ctx, ownedFlag);
        selection_state_tick(&ctx);
        CHECK(map_store_kind_peek() == 0xbu && selection_count(&ctx) == 2,
              "the clean class keeps both its shapes");
        selection_add(&ctx, noOwnerFlag);
        selection_state_tick(&ctx);
        CHECK(map_store_kind_peek() == 0xbu && selection_count(&ctx) == 2
                  && ctx.selection[1] != noOwnerFlag,
              "the unowned bit-17 entry drops from the clean class");

        /* Flag class: the unowned bit-17 entry owns context 0xc. */
        selection_clear(&ctx);
        selection_add(&ctx, noOwnerFlag);
        selection_state_tick(&ctx);
        CHECK(map_store_kind_peek() == 0xcu && selection_count(&ctx) == 1
                  && ctx.selection[0] == noOwnerFlag,
              "the flag class keeps the unowned bit-17 entry");

        /* Kind 4 and kind 11 classes. */
        selection_clear(&ctx);
        selection_add(&ctx, c); /* the file's type-4 entity */
        selection_state_tick(&ctx);
        CHECK(map_store_kind_peek() == 0xdu && selection_count(&ctx) == 1
                  && ctx.selection[0] == c,
              "kind 4 owns context 0xd");
        selection_clear(&ctx);
        selection_add(&ctx, miner11);
        selection_state_tick(&ctx);
        CHECK(map_store_kind_peek() == 0xeu && selection_count(&ctx) == 1
                  && ctx.selection[0] == miner11,
              "kind 11 with the +0x3e4 bit owns context 0xe");

        /* Class-none: an entry that classifies nothing posts 0 and every
         * entry drops (the quirk pinned). */
        selection_clear(&ctx);
        CHECK(selection_add(&ctx, gen3) == 1 &&
                  (gen3->flags_3e8 & ENTITY_SELECTED) != 0,
              "the class-none entry is selected before the tick");
        selection_state_tick(&ctx);
        CHECK(selection_count(&ctx) == 0
                  && (gen3->flags_3e8 & ENTITY_SELECTED) == 0,
              "the class-none shape drops every entry");

        selection_state_tick(NULL); /* guarded */
    }

    if (s_failures != 0) {
        fprintf(stderr, "select_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("select_test: OK\n");
    return 0;
}
