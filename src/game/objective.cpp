// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/block.hpp"
#include "gods98/media.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"

/* Config key names (finding 22). */
static const char* K_DONT_ADV      = "DontShowObjectiveAdvisor";
static const char* K_DONT_ACH_ADV  = "DontShowObjectiveAcheiveAdvisor";
static const char* K_DONT_FAIL_ADV  = "DontShowObjectiveFailedAdvisor";
static const char* K_ACH_AVI       = "ObjectiveAcheivedAVI";
static const char* K_CRYSTAL_OBJ   = "CrystalObjective";
static const char* K_ORE_OBJ       = "OreObjective";
static const char* K_TIMER_OBJ     = "TimerObjective";
static const char* K_CONSTRUCT_OBJ = "ConstructionObjective";
static const char* K_BLOCK_OBJ     = "BlockObjective";

static int obj_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (ca >= 'a' && ca <= 'z') ca -= 0x20;
        if (cb >= 'a' && cb <= 'z') cb -= 0x20;
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
}

static const char* obj_cfg_str(BlockFile* cfg, const char* level, const char* key,
                               char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL) {
        return NULL;
    }
    char path[512];
    if (util_level_cfg_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return NULL;
    }
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return NULL;
    }
    snprintf(out, out_sz, "%s", v);
    return out;
}

/* Corpus FUN_004790d0 typed getter used by the advisor-suppression keys:
 * boolean vocabulary (TRUE/YES/ON → 1, FALSE/NO/OFF → 0), else strtol. */
static int obj_typed_int(BlockFile* cfg, const char* level, const char* key, int dflt)
{
    char path[512];
    if (util_level_cfg_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return dflt;
    }
    if (obj_strcasecmp(v, "TRUE") == 0 || obj_strcasecmp(v, "YES") == 0
        || obj_strcasecmp(v, "ON") == 0) {
        return 1;
    }
    if (obj_strcasecmp(v, "FALSE") == 0 || obj_strcasecmp(v, "NO") == 0
        || obj_strcasecmp(v, "OFF") == 0) {
        return 0;
    }
    char* end = NULL;
    long r = strtol(v, &end, 10);
    return (end == v) ? 2 : (int)r;
}

/* Split "a b" / "a,b" value into two longs (block/construction targets). */
static void obj_split_two(const char* s, long* a, long* b)
{
    *a = 0;
    *b = 0;
    if (s == NULL) {
        return;
    }
    *a = util_atoi(s);
    const char* p = s;
    while (*p != '\0' && *p != ' ' && *p != ',' && *p != '|') {
        ++p;
    }
    if (*p != '\0') {
        *b = util_atoi(p + 1);
    }
}

/* Corpus FUN_00458580 — crystal/ore objectives. */
static void obj_set_resources(LevelState* s, long crystal, long ore)
{
    if (crystal != 0) {
        s->objective |= LVL_OBJECTIVE_CRYSTALS;
        level_set_int(s, OBJ_OFF_CRYSTAL_TARGET, (int32_t)crystal);
    }
    if (ore != 0) {
        s->objective |= LVL_OBJECTIVE_ORE;
        level_set_int(s, OBJ_OFF_ORE_TARGET, (int32_t)ore);
    }
}

/* Corpus FUN_004585c0 — block objective (two targets). */
static void obj_set_block(LevelState* s, long a, long b)
{
    s->objective |= LVL_OBJECTIVE_BLOCK;
    level_set_int(s, OBJ_OFF_BLOCK_A, (int32_t)a);
    level_set_int(s, OBJ_OFF_BLOCK_B, (int32_t)b);
}

/* Corpus FUN_004585f0 — timer objective (ticks = seconds*25). */
static void obj_set_timer(LevelState* s, float ticks, int hit_time_fail)
{
    s->objective |= LVL_OBJECTIVE_TIMER;
    level_set_float(s, OBJ_OFF_TIMER_TARGET, ticks);
    if (hit_time_fail) {
        s->objective |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
}

/* Corpus FUN_00458620 — construction objective. */
static void obj_set_construction(LevelState* s, long a, long b)
{
    s->objective |= LVL_OBJECTIVE_CONSTRUCTION;
    level_set_int(s, OBJ_OFF_CONSTRUCT_A, (int32_t)a);
    level_set_int(s, OBJ_OFF_CONSTRUCT_B, (int32_t)b);
}

void objective_load(BlockFile* cfg, const char* level_name, LevelState* s)
{
    if (cfg == NULL || s == NULL) {
        return;
    }
    /* Zero the 0x54-byte objective slot block (+0xbc..+0x110). */
    for (int o = OBJ_SLOT_HANDLE; o < OBJ_SLOT_HANDLE + 0x54; o += 4) {
        level_set_int(s, o, 0);
    }
    s->objective = 0;
    s->objective_pending = 1;

    /* Advisor-suppression keys: bit set when the key is NOT 1. */
    if (obj_typed_int(cfg, level_name, K_DONT_ADV, 0) != 1) {
        s->objective |= LVL_OBJECTIVE_NO_ADVISOR;
    }
    if (obj_typed_int(cfg, level_name, K_DONT_ACH_ADV, 0) != 1) {
        s->objective |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
    if (obj_typed_int(cfg, level_name, K_DONT_FAIL_ADV, 0) != 1) {
        s->objective |= LVL_OBJECTIVE_NO_FAIL_ADV;
    }

    /* Achieved-AVI slot: the corpus loads an AVI by name and stores dims at
     * level+0xe4/+0xe8, or sets +0xec=1 when the file has no dimensions.
     * Surface/movie handles are the render lane — the dims and the marker
     * are kept for tests. */
    {
        char value[256];
        if (obj_cfg_str(cfg, level_name, K_ACH_AVI, value, sizeof(value)) != NULL) {
            char fields[UTIL_QUERY_MAX_FIELDS][256];
            int st = util_query_fields(value, fields, sizeof(fields[0]));
            if (st == 3) {
                level_set_float(s, OBJ_OFF_AVI_DIM, (float)util_atoi(fields[1]));
                level_set_float(s, OBJ_OFF_AVI_DIM + 4, (float)util_atoi(fields[2]));
            } else {
                level_set_int(s, OBJ_OFF_AVI_SECOND, 1);
            }
        }
    }

    /* Resource objectives (finding 22 table). */
    {
        char value[256];
        long crystal = 0, ore = 0;
        if (obj_cfg_str(cfg, level_name, K_CRYSTAL_OBJ, value, sizeof(value)) != NULL) {
            crystal = util_atoi(value);
        }
        if (obj_cfg_str(cfg, level_name, K_ORE_OBJ, value, sizeof(value)) != NULL) {
            ore = util_atoi(value);
        }
        obj_set_resources(s, crystal, ore);
    }
    {
        char value[64];
        if (obj_cfg_str(cfg, level_name, K_TIMER_OBJ, value, sizeof(value)) != NULL) {
            float ticks = (float)util_atoi(value) * 25.0f;
            int htf = obj_strcasecmp(value, "HitTimeFailObjective") == 0;
            obj_set_timer(s, ticks, htf);
        }
    }
    {
        char value[64];
        if (obj_cfg_str(cfg, level_name, K_CONSTRUCT_OBJ, value, sizeof(value)) != NULL) {
            long a = 0, b = 0;
            obj_split_two(value, &a, &b);
            obj_set_construction(s, a, b);
        }
    }
    {
        char value[64];
        if (obj_cfg_str(cfg, level_name, K_BLOCK_OBJ, value, sizeof(value)) != NULL) {
            long a = 0, b = 0;
            obj_split_two(value, &a, &b);
            obj_set_block(s, a, b);
        }
    }
    /* ObjectiveText feeds the message registry (corpus FUN_004574e0); not
     * stored on the level block — HUD milestone. */
    (void)K_DONT_ADV;

    if (s->objective == 0) {
        s->objective = LVL_OBJECTIVE_NO_ADVISOR;
    }
}

int objective_check(LevelState* s, int* mark, float dt)
{
    if (s == NULL) {
        return 0;
    }
    if ((s->objective & LVL_OBJECTIVE_NO_ADVISOR) != 0) {
        return 0;
    }
    int m = 1;
    if ((s->objective & LVL_OBJECTIVE_TIMER) != 0) {
        float timer = level_float(s, OBJ_OFF_TIMER_TARGET) - dt;
        level_set_float(s, OBJ_OFF_TIMER_TARGET, timer);
        if (timer <= 0.0f) {
            if ((s->objective & LVL_OBJECTIVE_NO_ACH_ADV) != 0) {
                m = 0; /* HitTimeFail: timer expiry marks success */
            }
            *mark = m;
            return 1;
        }
    }
    if ((s->objective & LVL_OBJECTIVE_CRYSTALS) != 0
        && (uint32_t)level_int(s, OBJ_OFF_CRYSTAL_LIVE)
           < (uint32_t)level_int(s, OBJ_OFF_CRYSTAL_TARGET)) {
        return 0;
    }
    if ((s->objective & LVL_OBJECTIVE_ORE) != 0
        && (uint32_t)level_int(s, OBJ_OFF_ORE_LIVE)
           < (uint32_t)level_int(s, OBJ_OFF_ORE_TARGET)) {
        return 0;
    }
    *mark = m;
    return 1;
}

void objective_announce(LevelState* s, int who)
{
    if (s == NULL) {
        return;
    }
    /* A result is already shown (bits 1/2/4) — the corpus returns early. */
    if ((s->objective & 7u) != 0) {
        return;
    }
    int result = 0;
    const char* outcome = NULL;
    switch (who) {
    case 0: /* pending */
        s->objective |= LVL_OBJECTIVE_STILL_PENDING;
        outcome = "Pending";
        result = 0;
        break;
    case 1: /* achieved */
        s->objective_result = 1;
        s->objective |= LVL_OBJECTIVE_ACHIEVED;
        outcome = "Acheived";
        result = 1;
        break;
    case 2: /* failed */
        s->objective_result = 0;
        s->objective |= LVL_OBJECTIVE_FAILED;
        outcome = "Failed";
        result = 2;
        break;
    case 3: /* failed-with-crystals */
        s->objective_result = 0;
        s->objective |= LVL_OBJECTIVE_FAILED | LVL_OBJECTIVE_CRYSTALS;
        outcome = "FailedCrystals";
        result = 2;
        break;
    default:
        return;
    }
    level_set_int(s, OBJ_OFF_RESULT_INDEX, result);
    /* Advisor stream text (corpus `Stream Objective_%s_%s_%s` ->
     * DAT_004ffe38 with the outcome, a slot string, and the level name). */
    {
        const char* lvl = s->ptr_name != NULL ? (const char*)s->ptr_name : "";
        snprintf(s->obj_stream, sizeof(s->obj_stream),
                 "Stream Objective_%s_%d_%s", outcome, result, lvl);
    }
    s->objective |= LVL_OBJECTIVE_ANNOUNCED; /* hint shown; cleared per-frame */
}

const char* objective_stream(const LevelState* s)
{
    return s != NULL ? s->obj_stream : "";
}

int objective_advisor_shown(const LevelState* s)
{
    if (s == NULL || s->obj_stream[0] == '\0') {
        return 0;
    }
    /* The corpus honours the outcome's suppression bit (0x10 generic /
     * 0x20 achieved / 0x80 failed) before showing the hint. */
    uint32_t suppress = LVL_OBJECTIVE_NO_ADVISOR;
    if (s->objective & LVL_OBJECTIVE_ACHIEVED) {
        suppress |= LVL_OBJECTIVE_NO_ACH_ADV;
    }
    if (s->objective & LVL_OBJECTIVE_FAILED) {
        suppress |= LVL_OBJECTIVE_NO_FAIL_ADV;
    }
    return (s->objective & suppress) == 0 ? 1 : 0;
}

int objective_tick(LevelState* s, float dt)
{
    if (s == NULL) {
        return 1;
    }
    /* Consume an announced hint: the state machine clears bit 8 once the
     * advisor overlay has been shown (the show call is a stub). */
    if ((s->objective & LVL_OBJECTIVE_ANNOUNCED) != 0) {
        s->objective &= ~LVL_OBJECTIVE_ANNOUNCED;
    }
    if ((s->objective & (LVL_OBJECTIVE_ACHIEVED | LVL_OBJECTIVE_FAILED)) != 0) {
        return 0; /* resolved */
    }
    if ((s->objective & LVL_OBJECTIVE_STILL_PENDING) != 0) {
        return 0; /* pending result drains out-of-level */
    }
    /* Idle: run the checker. */
    int mark = 0;
    if (objective_check(s, &mark, dt)) {
        objective_announce(s, mark != 0 ? 1 : 2);
        return 0;
    }
    return 1;
}

int objective_voice_alias_raise_commit(LevelState* s)
{
    objective_announce(s, 1);
    return 1;
}

/* ObjectiveVoiceAliasRaiseGateBit27 (FUN_00454b80) — see objective.hpp. */
int objective_voice_alias_raise_gate_bit27(LevelState* s)
{
    if ((media_cd_flags_get() & 0x8000000u) == 0u) {
        objective_announce(s, 2);
    }
    return 1;
}

/* --- the advisor action window band (see objective.hpp) ------------------- */

#define ADV_ACTION_ROWS 32 /* deviation: the corpus strides the rows blindly */

/* Row layout (dwords, stride 9): mode +0x00, pair +0x04/+0x08,
 * read-out pair +0x0c/+0x10, flag word +0x14. */
static uint32_t s_adv_actions[ADV_ACTION_ROWS][9];

void advisor_action_position_and_mode_set(int row, int mode,
                                          uint32_t dword_a, uint32_t dword_b)
{
    if (row < 0 || row >= ADV_ACTION_ROWS) {
        return; /* deviation: blind stride */
    }
    uint32_t* rec = s_adv_actions[row];
    rec[0] = (uint32_t)mode;   /* DAT base + 0x00 */
    rec[1] = dword_a;          /* DAT base + 0x04 */
    rec[2] = dword_b;          /* DAT base + 0x08 */
    if ((uint32_t)mode == 0xcu) {
        rec[5] |= 0x10000u;    /* DAT base + 0x14 */
    } else {
        rec[5] &= 0xfffeffffu;
    }
}

void advisor_action_position_get(int row, uint32_t* dword_c, uint32_t* dword_d)
{
    if (row < 0 || row >= ADV_ACTION_ROWS) {
        return; /* deviation: blind stride */
    }
    if (dword_c != NULL) {
        *dword_c = s_adv_actions[row][3]; /* DAT base + 0x0c */
    }
    if (dword_d != NULL) {
        *dword_d = s_adv_actions[row][4]; /* DAT base + 0x10 */
    }
}

uint32_t advisor_action_flags_peek(int row)
{
    return (row >= 0 && row < ADV_ACTION_ROWS) ? s_adv_actions[row][5] : 0u;
}

/* ObjectiveAdvisorTokenLookup (FUN_00453200). The corpus's table pair
 * (count word + table pointer) has no ported writers, so the model
 * starts empty (documented deviation) and an install hook lets the
 * faithful read shape be exercised. */
static const uint32_t* s_adv_tokens;
static uint32_t        s_adv_token_count;

void objective_advisor_tokens_install(const uint32_t* table, uint32_t count)
{
    s_adv_tokens = table;
    s_adv_token_count = count;
}

int objective_advisor_token_lookup(uint32_t index)
{
    if (index < s_adv_token_count && s_adv_tokens != NULL) {
        return (int)s_adv_tokens[index];
    }
    return 0;
}
