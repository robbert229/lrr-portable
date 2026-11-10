// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_entity — validation for src/gods98/entity.cpp (finding 12 §4).
 *
 * Synthetic cases always run: the kind dispatch table (FUN_00475930) with
 * the ':'-split NOTEXTURE flag, and entity_assembly over hand-built `.ae`
 * block text — ACTIVITIES document order, '!' gating under both feature
 * modes, FILE-missing activity skipped, TRANSCOEF/TRIGGER/LWSFILE/LOOPING/
 * SAMPLE defaults, root SCALE, group-name case folding, `Lego*` prefix root.
 *
 * With `--real <dir>` the shipped wads mount and EVERY .ae entry (56) is
 * assembled; totals and an aggregate FNV digest cross-check the same view
 * extracted from the shipped bytes with tools/rrae.py (feature modes 0 and
 * reduce-animation 0x400000).
 *
 * Wire-up: meson test 'entity_ae' (suite 'entity_ae'); real checks via the
 * `test_wads_dir` meson option.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "resource.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* --- synthetic: kind dispatch --------------------------------------------- */

static void test_kind(void)
{
    int nt = -1;
    CHECK(entity_kind_lookup("NULL", &nt) == ENTITY_KIND_NULL && nt == 0,
          "kind: NULL -> 0");
    CHECK(entity_kind_lookup("MESH", &nt) == ENTITY_KIND_MESH && nt == 0,
          "kind: MESH -> 1");
    CHECK(entity_kind_lookup("FRAME", &nt) == ENTITY_KIND_FRAME && nt == 0,
          "kind: FRAME -> 2");
    CHECK(entity_kind_lookup("ANIM", &nt) == ENTITY_KIND_ANIM && nt == 0,
          "kind: ANIM -> 3");
    CHECK(entity_kind_lookup("ACT", &nt) == ENTITY_KIND_ACT && nt == 0,
          "kind: ACT -> 4");
    CHECK(entity_kind_lookup("LWS", &nt) == ENTITY_KIND_LWS && nt == 0,
          "kind: LWS -> 7");
    CHECK(entity_kind_lookup("LWO", &nt) == ENTITY_KIND_LWO && nt == 0,
          "kind: LWO -> 8");
    /* case-insensitive */
    CHECK(entity_kind_lookup("lwo", &nt) == ENTITY_KIND_LWO && nt == 0,
          "kind: lowercase fold");
    /* ':' NOTEXTURE suffix ("LWO:NOTEXTURE" at 0x4a1dbc, s_NOTEXTURE 0x4aab78) */
    CHECK(entity_kind_lookup("LWO:NOTEXTURE", &nt) == ENTITY_KIND_LWO && nt == 1,
          "kind: LWO:NOTEXTURE sets flag");
    CHECK(entity_kind_lookup("LWO:nOTeXtUrE", &nt) == ENTITY_KIND_LWO && nt == 1,
          "kind: NOTEXTURE case-insensitive");
    CHECK(entity_kind_lookup("LWO:OTHER", &nt) == ENTITY_KIND_LWO && nt == 0,
          "kind: other suffix clears flag");
    CHECK(entity_kind_lookup("cfg", &nt) == ENTITY_KIND_UNKNOWN,
          "kind: unknown -> -1");
    CHECK(entity_kind_lookup(NULL, &nt) == ENTITY_KIND_UNKNOWN,
          "kind: NULL -> -1");
    CHECK(entity_kind_lookup("", &nt) == ENTITY_KIND_UNKNOWN,
          "kind: empty -> -1");
    /* slots 5/6 are zero in the corpus table and never match */
    CHECK(entity_kind_lookup("TEXTURE", &nt) == ENTITY_KIND_UNKNOWN,
          "kind: no phantom slot match");
}

/* --- synthetic: .ae assembly ---------------------------------------------- */

static EntityAssembly* parse_text(const char* text)
{
    BlockFile* bf = block_parse((char*)text, (size_t)strlen(text));
    if (bf == NULL) {
        return NULL;
    }
    EntityAssembly* ea = entity_assembly_parse(bf, "LegoRR", 0);
    block_close(bf);
    return ea;
}

static void test_assembly_basic(void)
{
    /* corpus layout of a small two-state creature */
    const char* text =
        "Lego* {\n"
        "\tScale\t\t1.0\n"
        "\tActivities {\n"
        "\t\tActivity_Stand\tStand\n"
        "\t\tActivity_Route\tWalk\n"
        "\t\t!Activity_TurnLeft\tTurnLeft\n"   /* gated by default feature */
        "\t}\n"
        "\tStand {\n"
        "\t\tFILE\t\tStand2\n"
        "\t\tTRANSCOEF\t0.0\n"
        "\t\tLWSFILE\t\tTRUE\n"
        "\t}\n"
        "\tWalk {\n"
        "\t\tFILE\t\twalk4\n"
        "\t\tLOOPING\t\tTRUE\n"
        "\t}\n"
        "\tTurnLeft {\n"          /* ! key strips '!' -> state present */
        "\t\tFILE\t\tturn90left\n"
        "\t}\n"
        "}\n";
    EntityAssembly* ea = parse_text(text);
    CHECK(ea != NULL, "basic: assembly parsed");
    if (ea == NULL) {
        return;
    }
    CHECK(ea->scale == 1.0f && ea->count == 3, "basic: scale + 3 activities");
    if (ea->count == 3) {
        CHECK(strcmp(ea->acts[0].state, "Activity_Stand") == 0
                  && strcmp(ea->acts[0].name, "Stand") == 0
                  && strcmp(ea->acts[0].file, "Stand2") == 0,
              "basic: stand row fields");
        CHECK(ea->acts[0].transcoef == 0.0f
                  && ea->acts[0].trigger == 0
                  && ea->acts[0].lwsfile == 1
                  && ea->acts[0].looping == 2,
              "basic: stand defaults + LWSFILE TRUE");
        CHECK(strcmp(ea->acts[1].name, "Walk") == 0
                  && ea->acts[1].looping == 1
                  && ea->acts[1].lwsfile == 2,
              "basic: walk LOOPING 1, LWSFILE missing");
        CHECK(strcmp(ea->acts[2].state, "Activity_TurnLeft") == 0
                  && strcmp(ea->acts[2].name, "TurnLeft") == 0
                  && ea->acts[2].reduce_anim == 1,
              "basic: '!' key included, stripped, marked");
    }
    entity_assembly_free(ea);
}

static void test_assembly_reduce_anim(void)
{
    const char* text =
        "Lego* {\n"
        "\tActivities {\n"
        "\t\tActivity_Stand\tStand\n"
        "\t\t!Activity_Turn\tTurn\n"
        "\t}\n"
        "\tStand { FILE Stand2 }\n"
        "\tTurn { FILE turn90left }\n"
        "}\n";
    BlockFile* bf = block_parse((char*)text, (size_t)strlen(text));
    CHECK(bf != NULL, "reduce: block parsed");
    if (bf == NULL) {
        return;
    }
    EntityAssembly* a0 = entity_assembly_parse(bf, "LegoRR", 0);
    CHECK(a0 != NULL && a0->count == 2, "reduce: feature=0 keeps '!' row");
    EntityAssembly* ax = entity_assembly_parse(bf, "LegoRR",
                                               ENTITY_FEATURE_REDUCE_ANIM);
    CHECK(ax != NULL && ax->count == 1
              && strcmp(ax->acts[0].name, "Stand") == 0,
          "reduce: 0x400000 drops the '!' activity");
    entity_assembly_free(a0);
    entity_assembly_free(ax);
    block_close(bf);
}

static void test_assembly_missing_file(void)
{
    const char* text =
        "Lego* {\n"
        "\tActivities {\n"
        "\t\tActivity_Stand\tStand\n"
        "\t\tActivity_NoFile\tNoFile\n"
        "\t}\n"
        "\tStand { FILE Stand2 }\n"
        "}\n";  /* group NoFile: no FILE -> row omitted */
    EntityAssembly* ea = parse_text(text);
    CHECK(ea != NULL && ea->count == 1
              && strcmp(ea->acts[0].name, "Stand") == 0,
          "missingfile: activity without FILE is dropped");
    if (ea != NULL) {
        entity_assembly_free(ea);
    }
}

static void test_assembly_defaults(void)
{
    const char* text =
        "LegoLow* {\n"   /* wildcard root: query token must not match here */
        "\tActivities { Activity_X X }\n"
        "\tX { FILE x }\n"
        "}\n"
        "Lego* {\n"
        "\tActivities { Activity_Y Y }\n"
        "\tY { FILE y }\n"
        "}\n";
    /* "LegoRR" matches "Lego*" only ("LegoLow*" prefix mismatch) */
    BlockFile* bf = block_parse((char*)text, (size_t)strlen(text));
    CHECK(bf != NULL, "roots: block parsed");
    if (bf == NULL) {
        return;
    }
    EntityAssembly* ea = entity_assembly_parse(bf, "LegoRR", 0);
    CHECK(ea != NULL && ea->count == 1
              && strcmp(ea->acts[0].name, "Y") == 0,
          "roots: LegoRR selects the Lego* subtree");
    if (ea != NULL) {
        entity_assembly_free(ea);
    }
    block_close(bf);
}

/* --- real-corpus validation --------------------------------------------------
 * Every .ae entry is opened by the corpus path build (name = parent dir of
 * the entry) and assembled; totals + FNV digest are cross-checked against
 * the same view extracted from the shipped bytes by the ae_truth.py model,
 * with duplicate groups resolved last-match as the original does.
 * Layout mirrors test_model.cpp (mount, dedup by folded key, sort, digest). */

#define REAL_AE        56

/* feature 0 (shipped default) */
#define REAL_ACTIVITIES 439
#define REAL_WITH_FILE  439  /* every activity carries a FILE in the corpus */
#define REAL_SAMPLE     4
#define REAL_LWS_TRUE   439
#define REAL_LOOP_NZ    369
#define REAL_TRIGGER_SUM 283
#define REAL_TRANSCOEF_NZ 36
#define REAL_SCALE_NZ   51
/* LargeDigger.ae has two `Drill` groups under `Lego*` ("digDRILL" and
 * "digdrill"). Through the '*' root the original Config_FindItem keeps the
 * LAST match (finding 508), so both digests fold "digdrill"; the earlier
 * tree parser kept the first (0xe4e3630c / 0xfc217039). File lookups ignore
 * case, so the loaded scene is the same. */
#define REAL_DIGEST     0x80631cacu

/* reduce-animation feature (0x400000): '!' rows dropped */
#define REAL_RED_ACTIVITIES 354
#define REAL_RED_DIGEST 0x2a2df559u

typedef struct { char key[256]; char raw[256]; } Entry;

static Entry* s_ent;
static int s_nent, s_capent;

static void ent_add(const char* key, const char* raw)
{
    for (int i = 0; i < s_nent; i++) {
        if (strcmp(s_ent[i].key, key) == 0) {
            snprintf(s_ent[i].raw, sizeof s_ent[i].raw, "%s", raw);
            return; /* higher mount wins */
        }
    }
    if (s_nent == s_capent) {
        int ncap = s_capent ? s_capent * 2 : 256;
        Entry* ne = (Entry*)realloc(s_ent, (size_t)ncap * sizeof *ne);
        if (!ne) {
            return;
        }
        s_ent = ne;
        s_capent = ncap;
    }
    snprintf(s_ent[s_nent].key, sizeof s_ent[s_nent].key, "%s", key);
    snprintf(s_ent[s_nent].raw, sizeof s_ent[s_nent].raw, "%s", raw);
    s_nent++;
}

static int ent_cmp(const void* a, const void* b)
{
    return strcmp(((const Entry*)a)->key, ((const Entry*)b)->key);
}

static void fold(char* d, const char* s)
{
    int i = 0;
    for (; s[i] && i < 255; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z')
            c += 0x20;
        if (c == '\\')
            c = '/';
        d[i] = c;
    }
    d[i] = 0;
}

static uint32_t s_fnv(uint32_t h, const char* s)
{
    for (; *s; s++) {
        h = (h ^ (uint32_t)(unsigned char)*s) * 16777619u;
    }
    return h;
}

/* int-folded then 0xff separator, mirroring ae_truth.py fold_row() */
static void fold_int(uint32_t* h, long v)
{
    char b[32];
    snprintf(b, sizeof b, "%ld", v);
    *h = s_fnv(*h, b);
    *h = s_fnv(*h, "\xff");
}

static void fold_str(uint32_t* h, const char* s)
{
    *h = s_fnv(*h, s);
    *h = s_fnv(*h, "\xff");
}

/* qtrans: quantize a float to 1/10000 ints (round half away from zero) */
static int qtrans(float v)
{
    return (int)(v * 10000.0f + (v >= 0.0f ? 0.5f : -0.5f));
}

/* Fold one file's contribution the same way ae_truth.py does: scale first
 * (no 0xff separator), then, when the assembly exists, one row per activity
 * with each field terminated by 0xff. A NULL assembly still folds "0". */
static void fold_file(uint32_t* h, const EntityAssembly* ea)
{
    long qs = 0;
    if (ea != NULL) {
        int q = qtrans(ea->scale);
        qs = q ? q : 0;
    }
    char sb[32];
    snprintf(sb, sizeof sb, "%ld", qs);
    *h = s_fnv(*h, sb);
    if (ea == NULL) {
        return;
    }
    for (size_t k = 0; k < ea->count; k++) {
        const EntityActivity* a = &ea->acts[k];
        fold_str(h, a->state);
        fold_str(h, a->name);
        fold_str(h, a->file);
        fold_str(h, a->sample);
        fold_int(h, a->lwsfile);
        fold_int(h, a->looping);
        fold_int(h, a->trigger);
        fold_int(h, qtrans(a->transcoef));
    }
}

static void test_real(const char* dir)
{
    int mounted = resource_mount(dir, "LegoRR");
    CHECK(mounted >= 2, "real: both wads mounted");
    if (mounted < 2) {
        return;
    }

    for (int w = 0; w < resource_mounted_count(); w++) {
        uint32_t n = resource_wad_entry_count(w);
        for (uint32_t j = 0; j < n; j++) {
            const char* nm = resource_wad_entry_name(w, j);
            if (!nm) {
                continue;
            }
            char key[256];
            fold(key, nm);
            ent_add(key, nm);
        }
    }
    qsort(s_ent, (size_t)s_nent, sizeof s_ent[0], ent_cmp);

    uint32_t h = 2166136261u;
    uint32_t hr = 2166136261u;
    unsigned acts = 0, acts_r = 0, withfile = 0, sample = 0, lws = 0, loop = 0,
             tc = 0, scale_nz = 0;
    long trig = 0;

    unsigned files = 0;
    for (int i = 0; i < s_nent; i++) {
        size_t kl = strlen(s_ent[i].key);
        if (kl < 3 || strcmp(s_ent[i].key + kl - 3, ".ae") != 0) {
            continue;
        }
        files++;

        /* Open by the archive's own name (test_block's approach) and parse
         * the block — validates the assembly table over every shipped file.
         * The `<name>\<basename>.ae` path build is exercised synthetically
         * and via a spot check below. */
        BlockFile* bf = block_open(s_ent[i].raw);
        EntityAssembly* ea = NULL;
        if (bf != NULL) {
            ea = entity_assembly_parse(bf, "LegoRR", 0);
            block_close(bf);
        }
        if (ea == NULL) {
            printf("FAIL: real: no assembly %s\n", s_ent[i].key);
            ++s_failures;
        } else {
            acts += (unsigned)ea->count;
            for (size_t k = 0; k < ea->count; k++) {
                const EntityActivity* a = &ea->acts[k];
                if (a->file[0]) {
                    withfile++;
                }
                if (a->sample[0]) {
                    sample++;
                }
                if (a->lwsfile == 1) {
                    lws++;
                }
                if (a->looping != 0) {
                    loop++;
                }
                trig += a->trigger;
                if (a->transcoef != 0.0f) {
                    tc++;
                }
            }
            if (ea->scale != 0.0f) {
                scale_nz++;
            }
        }
        fold_file(&h, ea);
        entity_assembly_free(ea);

        /* reduce-animation digest (drops '!' rows) */
        BlockFile* br = block_open(s_ent[i].raw);
        EntityAssembly* er = NULL;
        if (br != NULL) {
            er = entity_assembly_parse(br, "LegoRR", ENTITY_FEATURE_REDUCE_ANIM);
            block_close(br);
        }
        if (er != NULL) {
            acts_r += (unsigned)er->count;
        }
        fold_file(&hr, er);
        entity_assembly_free(er);
    }

    /* spot: the corpus path build — "Creatures\Spider" resolves to
     * "Creatures\Spider\Spider.ae" through the resource layer. */
    {
        EntityAssembly* sp = entity_assembly_open("Creatures\\Spider", "LegoRR", 0);
        CHECK(sp != NULL && sp->count == 2
                  && strcmp(sp->acts[0].name, "Stand") == 0,
              "real: entity_assembly_open resolves <name>\\<base>.ae");
        entity_assembly_free(sp);
    }

    CHECK(files == REAL_AE, "real: 56 .ae entries assembled");
    CHECK(acts == REAL_ACTIVITIES, "real: activity total (feature=0)");
    CHECK(withfile == REAL_WITH_FILE, "real: all rows carry a FILE");
    CHECK(sample == REAL_SAMPLE, "real: SAMPLE field count");
    CHECK(lws == REAL_LWS_TRUE, "real: LWSFILE TRUE count");
    CHECK(loop == REAL_LOOP_NZ, "real: LOOPING nonzero count");
    CHECK(trig == REAL_TRIGGER_SUM, "real: TRIGGER value sum");
    CHECK(tc == REAL_TRANSCOEF_NZ, "real: TRANSCOEF nonzero count");
    CHECK(scale_nz == REAL_SCALE_NZ, "real: SCALE nonzero count");
    CHECK(h == REAL_DIGEST, "real: digest matches ground truth (feature=0)");
    CHECK(acts_r == REAL_RED_ACTIVITIES, "real: reduce-animation activity total");
    CHECK(hr == REAL_RED_DIGEST, "real: reduce-animation digest matches");
    printf("real: %u assemblies, digest %08x / reduce %08x\n", files, h, hr);
}

int main(int argc, char** argv)
{
    test_kind();
    test_assembly_basic();
    test_assembly_reduce_anim();
    test_assembly_missing_file();
    test_assembly_defaults();

    if (argc >= 3 && strcmp(argv[1], "--real") == 0) {
        test_real(argv[2]);
    }

    if (s_failures == 0)
        printf("entity: all checks passed\n");
    return s_failures ? 1 : 0;
}
