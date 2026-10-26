// SPDX-License-Identifier: LGPL-3.0-or-later
// Objectives / advisor (finding 22) — synthetic level-block fixture.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/level.hpp"
#include "game/objective.hpp"
#include "gods98/block.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static const char* FIX = "objective_fixture";

static LevelState* fresh_level(void)
{
    LevelState* s = (LevelState*)calloc(1, sizeof(LevelState));
    if (s != NULL) {
        s->ptr_name = (void*)"Level01";
    }
    return s;
}

int main(void)
{
    char rmcmd[64]; snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", FIX); system(rmcmd);
    mkdir(FIX, 0755);
    char path[512];
    snprintf(path, sizeof(path), "%s/Lego.cfg", FIX);
    FILE* fp = fopen(path, "w");
    if (fp != NULL) {
        fputs(
            "Lego {\n"
            "  Level01 {\n"
            "    DontShowObjectiveAdvisor 1\n"
            "    CrystalObjective 5\n"
            "    OreObjective 3\n"
            "    TimerObjective 2\n"
            "    BlockObjective 4,6\n"
            "    ConstructionObjective 1,2\n"
            "    Reward { Enable TRUE Modifier 2 Timer 30 }\n"
            "  }\n"
            "  Level02 { DontShowObjectiveAdvisor 1 }\n"
            "}\n",
            fp);
        fclose(fp);
    }
    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir %s failed\n", FIX);
        return 1;
    }

    BlockFile* cfg = block_open("Lego.cfg");
    CHECK(cfg != NULL, "open cfg");
    if (cfg == NULL) {
        return 1;
    }

    /* Level with all five goal types armed. */
    LevelState* s = fresh_level();
    CHECK(s != NULL, "level block");
    objective_load(cfg, "Level01", s);

    CHECK((s->objective & 0x10u) == 0, "advisor suppression off (key = 1)");
    CHECK((s->objective & 0x20u) != 0, "acheive-advisor defaulted on");
    CHECK((s->objective & 0x80u) != 0, "failed-advisor defaulted on");
    CHECK((s->objective & 0x100u) != 0, "crystal objective armed");
    CHECK((s->objective & 0x200u) != 0, "ore objective armed");
    CHECK((s->objective & 0x400u) != 0, "block objective armed");
    CHECK((s->objective & 0x800u) != 0, "timer objective armed");
    CHECK((s->objective & 0x1000u) != 0, "construction objective armed");

    CHECK(level_int(s, OBJ_OFF_CRYSTAL_TARGET) == 5, "crystal target");
    CHECK(level_int(s, OBJ_OFF_ORE_TARGET) == 3, "ore target");
    CHECK(level_float(s, OBJ_OFF_TIMER_TARGET) == 50.0f, "timer = 2s * 25");
    CHECK(level_int(s, OBJ_OFF_BLOCK_A) == 4, "block target a");
    CHECK(level_int(s, OBJ_OFF_BLOCK_B) == 6, "block target b");
    CHECK(level_int(s, OBJ_OFF_CONSTRUCT_A) == 1, "construct target a");
    CHECK(level_int(s, OBJ_OFF_CONSTRUCT_B) == 2, "construct target b");

    /* Checker: unmet crystals -> not met. */
    level_set_int(s, OBJ_OFF_CRYSTAL_LIVE, 2);
    int mark = 0;
    CHECK(objective_check(s, &mark, 0.0f) == 0, "unmet crystal count");
    /* Met crystals + ore but no timer expiry -> met. */
    level_set_int(s, OBJ_OFF_CRYSTAL_LIVE, 5);
    level_set_int(s, OBJ_OFF_ORE_LIVE, 3);
    CHECK(objective_check(s, &mark, 0.0f) == 1, "all counts met");
    CHECK(mark == 1, "success marker");

    /* Timer expiry flips the outcome: with the achievement-suppression bit
     * (0x20) set by default, expiry marks 0 — the HitTimeFail-style "missed
     * the time" reading (finding 22). */
    level_set_int(s, OBJ_OFF_TIMER_TARGET, 0);
    CHECK(objective_check(s, &mark, 0.0f) == 1, "timer expiry is met");
    CHECK(mark == 0, "timer expiry under 0x20 marks 0 (hit-time-fail)");

    /* Announcer bit flow. */
    objective_announce(s, 1);
    CHECK((s->objective & 2u) != 0, "achieved bit set");
    CHECK(level_int(s, OBJ_OFF_RESULT_INDEX) == 1, "result index 1");
    CHECK((s->objective & 8u) != 0, "announced bit set");
    /* Advisor stream text + the suppression decision (finding 22). */
    CHECK(strstr(objective_stream(s), "Acheived") != NULL,
          "announcer built the Objective_Acheived stream");
    CHECK(objective_advisor_shown(s) == 0,
          "advisor suppressed with the 0x20 default (key absent)");
    s->objective &= ~LVL_OBJECTIVE_NO_ACH_ADV;
    CHECK(objective_advisor_shown(s) == 1,
          "advisor shown once the achieved suppression is cleared");
    objective_tick(s, 0.0f);
    CHECK((s->objective & 8u) == 0, "announced bit cleared by tick");
    objective_announce(s, 2); /* ignored: result already resolved */
    CHECK((s->objective & 4u) == 0, "failed not re-set after achievement");

    level_destroy(s);

    /* Advisor-only level: key=1 shows generic hints (0x10 clear) and the
     * default achievement/failure suppression stays armed (0x20/0x80). */
    LevelState* s2 = fresh_level();
    objective_load(cfg, "Level02", s2);
    CHECK((s2->objective & 0x10u) == 0, "generic advisors shown when key = 1");
    CHECK((s2->objective & 0x20u) != 0, "achievement suppression defaulted on");
    level_destroy(s2);
    block_close(cfg);

    /* The raise-commit wrapper (2026-10-24). */
    {
        LevelState fresh;
        memset(&fresh, 0, sizeof(fresh));
        int r = objective_voice_alias_raise_commit(&fresh);
        CHECK(r == 1, "the commit answers 1");
        CHECK((fresh.objective & 2u) != 0
                  && level_int(&fresh, OBJ_OFF_RESULT_INDEX) == 1,
              "the announcer ran through the wrapper");
    }

    /* ObjectiveVoiceAliasRaiseGateBit27 (2026-10-24): the modeled gate bit
     * never lands set, so the raise announces the failed result (2) and
     * answers 1 — the corpus's unset-word reading. (The `s` block was
     * destroyed above, so the raise rides a fresh zeroed block — the
     * corpus's unset-word reading needs no config.) */
    LevelState* s4 = fresh_level();
    level_set_int(s4, OBJ_OFF_TIMER_TARGET, 0);
    int g = objective_voice_alias_raise_gate_bit27(s4);
    CHECK(g == 1, "the raise answers one");
    CHECK(strstr(objective_stream(s4), "Failed") != NULL
              || (s4->objective & 8u) != 0,
          "the raise announces the failed result");
    level_destroy(s4);

    /* Advisor action window band (2026-10-25): the row table's mode/pair
     * stores, the 0xc mode flag raise/clear pair, and the token table. */
    {
        uint32_t x = 0u, y = 0u;
        advisor_action_position_get(3, &x, &y);
        CHECK(x == 0u && y == 0u, "the quiet advisor row reads zero");
        advisor_action_position_and_mode_set(3, 0xc, 7u, 9u);
        CHECK(advisor_action_flags_peek(3) == 0x10000u,
              "mode 0xc raises the flag bit");
        advisor_action_position_and_mode_set(3, 5, 1u, 2u);
        CHECK(advisor_action_flags_peek(3) == 0u,
              "any other mode clears the flag bit");
        advisor_action_position_and_mode_set(3, 0xc, 7u, 9u);
        /* Faithful quirk: the set's pair lands in the write window, not
         * the read-out window — the get still reads zero. */
        advisor_action_position_get(3, &x, &y);
        CHECK(x == 0u && y == 0u, "the read-out window rides the set");
        advisor_action_position_get(99999, &x, &y); /* bounds guarded */
        /* The token table: empty model answers 0; installed, the read
         * shape is the corpus's bounds-guarded table walk. */
        CHECK(objective_advisor_token_lookup(1) == 0,
              "the empty token table answers zero");
        static const uint32_t tokens[3] = { 0xAAu, 0xBBu, 0xCCu };
        objective_advisor_tokens_install(tokens, 3);
        CHECK(objective_advisor_token_lookup(2) == (int)0xCCu
                  && objective_advisor_token_lookup(3) == 0,
              "the installed table reads through the guard");
        objective_advisor_tokens_install(NULL, 0);
    }

    if (s_failures != 0) {
        fprintf(stderr, "objective_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("objective_test: OK\n");
    return 0;
}
