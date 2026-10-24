// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_block — validation for src/gods98/block.cpp (finding 12).
 *
 * Synthetic grammar cases always run: nesting, ';' comments, pending-flush
 * records, multi-root files, first-token '*' wildcard, case folding, and the
 * typed getters with the corpus bool vocabulary {TRUE,YES,ON}/
 * {FALSE,NO,OFF}/else→2.
 *
 * With `--real <dir>` the shipped wads are mounted and EVERY block-text
 * entry (56 .ae, 44 .ol, 37 .ptl, 1 Lego.cfg = 138) is parsed, with a
 * record-count + FNV digest cross-checked against the ground-truth table
 * extracted from the shipped bytes via tools/rrwad.py, plus spot checks on
 * Barracks.ae / IceMonster.ae / Lego.cfg values.
 *
 * Wire-up: meson test 'block' (suite 'block'); real checks via the
 * `test_wads_dir` meson option.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "block.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static char* s_parse(const char* text)
{
    return (char*)text; /* block_parse never mutates the caller buffer */
}

static BlockFile* parse(const char* text)
{
    return block_parse(s_parse(text), strlen(text));
}

/* ASCII-only fold to compare extensions without locale surprises */
static int s_lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 0x20 : c; }
static int s_ext_is(const char* name, const char* ext)
{
    const char* dot = strrchr(name, '.');
    if (dot == NULL || strlen(dot + 1) != strlen(ext)) {
        return 0;
    }
    for (int i = 0; dot[1 + i]; i++) {
        if (s_lower(dot[1 + i]) != ext[i]) {
            return 0;
        }
    }
    return 1;
}

static const char* val(BlockFile* bf, const char* path)
{
    return block_get_strptr(bf, path);
}

static int val_is(BlockFile* bf, const char* path, const char* want)
{
    const char* got = val(bf, path);
    return got != NULL && strcmp(got, want) == 0;
}

/* ------------------------------------------------------------------ *
 *  Synthetic grammar cases
 * ------------------------------------------------------------------ */

static void test_basic(void)
{
    BlockFile* bf = parse(
        "Lego* {\n"
        "\tScale\t1.0\n"
        "\tActivities {\n"
        "\t\tA1 Stand\n"
        ";\tA2 Hidden\n"
        "\t\tA3\tCreate\n"
        "\t}\n"
        "\tPowerOff {\n"
        "\t\tFILE Bar\n"
        "\t}\n"
        "}\n");
    CHECK(bf != NULL, "basic: parse");
    CHECK(bf && block_record_count(bf) == 7, "basic: 7 records (comment wins)");
    CHECK(bf && val_is(bf, "Lego*", "{"), "basic: container value is '{'");
    CHECK(val_is(bf, "Lego* Scale", "1.0"), "basic: name value");
    CHECK(val_is(bf, "lego* scale", "1.0"), "basic: case-insensitive");
    CHECK(val_is(bf, "LegoRR Scale", "1.0"), "basic: exe-base '*' prefix match");
    CHECK(block_find(bf, "Lego* Activities A2") == NULL, "basic: ';' comments out");
    CHECK(val_is(bf, "Lego* Activities A3", "Create"), "basic: nested value");
    CHECK(block_subgroup(bf, "Lego* Activities") != NULL, "basic: subgroup ok");
    CHECK(block_subgroup(bf, "Lego* Scale") == NULL, "basic: subgroup needs children");
    CHECK(val_is(bf, "Lego* PowerOff FILE", "Bar"), "basic: two levels");
    CHECK(block_find(bf, "Lego* Missing") == NULL, "basic: miss -> NULL");

    /* children of Activities: A1 then A3, names/values readable */
    BlockRec* acts = block_subgroup(bf, "Lego* Activities");
    BlockRec* first = block_first_child(acts);
    CHECK(first && strcmp(block_name(first), "A1") == 0, "basic: first child A1");
    CHECK(first && block_next_sibling(first)
        && strcmp(block_name(block_next_sibling(first)), "A3") == 0,
        "basic: sibling A3");
    block_close(bf);
}

static void test_wildcard(void)
{
    BlockFile* bf = parse("Root* {\n\tDeep { x 1\n\t\tk* { y 2 }\n\t}\n}\n");
    CHECK(bf != NULL, "wildcard: parse");
    CHECK(val_is(bf, "RootRR Deep x", "1"), "wildcard: first-token '*' prefix");
    CHECK(val_is(bf, "rootrr DEEP x", "1"), "wildcard: fold + prefix");
    CHECK(val_is(bf, "RootRR Deep k* y", "2"), "wildcard: deeper exact '*' name");
    CHECK(block_find(bf, "RootRR Deep kZ y") == NULL, "wildcard: not gated deeper");
    CHECK(block_find(bf, "Zoop Deep x") == NULL, "wildcard: prefix must match root");
    block_close(bf);
}

static void test_comments_braces(void)
{
    BlockFile* bf = parse(
        "A 1 ; trailing comment\n"
        "B 2\n"
        "C { ; hidden inside\n"
        "}\n"
        "E 5\n"
        "R { a 1 } } extra 9\n"
        "R2 { b");
    CHECK(bf != NULL, "braces: parse");
    CHECK(val_is(bf, "A", "1"), "braces: inline comment stripped");
    CHECK(val_is(bf, "B", "2"), "braces: sibling after comment");
    CHECK(val_is(bf, "E", "5"), "braces: pop then sibling at root");
    CHECK(val_is(bf, "R a", "1"), "braces: nested pair");
    CHECK(val_is(bf, "extra", "9"), "braces: extra '}' cannot eat siblings");
    BlockRec* b = block_find(bf, "R2 b");
    CHECK(b != NULL && block_value(b) == NULL, "braces: EOF flush = valueless record");
    block_close(bf);
}

static void test_colon_sep(void)
{
    /* The shipped Lego.cfg shape: root `Lego*` with sibling `Main`/`Menu`/
     * `Levels` groups; the exe queries with the `::` group separator
     * (DAT_004aab18 = "::", finding 23) — "Lego* Main Menu::MainMenuFull
     * MenuCount". Both ' ' and "::" must tokenize. */
    BlockFile* bf = parse(
        "Lego* {\n"
        "\tMain {\n"
        "\t\tStartLevel\tLevels::Tran01\n"
        "\t}\n"
        "\tMenu {\n"
        "\t\tMainMenuFull {\n"
        "\t\t\tMenuCount 5\n"
        "\t\t\tMenu1 {\n"
        "\t\t\t\tItemCount 1\n"
        "\t\t\t\tItem1 Next:0:0:Start_Game:Menu2\n"
        "\t\t\t}\n"
        "\t\t}\n"
        "\t}\n"
        "\tLevels {\n"
        "\t\tTran01 {\n"
        "\t\t\tSurfaceMap x.map\n"
        "\t\t}\n"
        "\t}\n"
        "}\n");
    CHECK(bf != NULL, "colon: parse");
    CHECK(val_is(bf, "Lego* Menu::MainMenuFull MenuCount", "5"),
        "colon: :: menu path");
    CHECK(val_is(bf, "Lego* Menu MainMenuFull MenuCount", "5"),
        "colon: space form parity");
    CHECK(val_is(bf, "Lego* Menu::MainMenuFull Menu1 Item1",
                 "Next:0:0:Start_Game:Menu2"), "colon: :: nested item");
    /* the shipped level *address* (StartLevel value) resolves the group: */
    CHECK(val_is(bf, "Lego* Main StartLevel", "Levels::Tran01"),
        "colon: StartLevel value kept intact (a value, not a path)");
    CHECK(val_is(bf, "Lego* Levels::Tran01 SurfaceMap", "x.map"),
        "colon: Levels::X level address resolves the record");
    CHECK(block_subgroup(bf, "Lego* Menu::MainMenuFull Menu1") != NULL,
        "colon: :: subgroup");
    CHECK(block_find(bf, "Lego* Menu::MainMenuFull MenuNope") == NULL,
        "colon: miss -> NULL");
    block_close(bf);
}

static void test_multiroot(void)
{
    BlockFile* bf = parse("A { x { y 1 } }\nB { x { y 2 } }\n");
    CHECK(val_is(bf, "A x y", "1"), "multiroot: tree A");
    CHECK(val_is(bf, "B x y", "2"), "multiroot: tree B");
    CHECK(val_is(bf, "A x", "{"), "multiroot: intermediate is container");
    block_close(bf);
}

/* SortedEntryNextEqualKey (FUN_00479020): the config loaders' sibling
 * iteration idiom — first match then repeatedly "next at the same
 * depth" (findings 230/403). */
static void test_equal_key(void)
{
    BlockFile* bf = parse(
        "S {\n"
        "  c1 { g 1 }\n"
        "  c2 2\n"
        "  c3 { g 3 }\n"
        "}\n");
    BlockRec* c1 = block_find(bf, "S c1");
    BlockRec* c2 = block_find(bf, "S c2");
    BlockRec* c3 = block_find(bf, "S c3");
    BlockRec* g1 = block_find(bf, "S c1 g");
    CHECK(c1 != NULL && c2 != NULL && c3 != NULL && g1 != NULL,
          "equal-key: fixture records");
    CHECK(block_next_equal_key(c1) == c2, "skips c1's subtree in document order");
    CHECK(block_next_equal_key(c2) == c3, "simple child continues the walk");
    CHECK(block_next_equal_key(c3) == NULL, "last sibling ends the walk");
    CHECK(block_next_equal_key(g1) == NULL, "subtree record has no sibling");
    CHECK(block_next_equal_key(NULL) == NULL, "NULL start answers NULL");
    /* The loaders' loop idiom. */
    int seen = 0;
    for (BlockRec* e = block_find(bf, "S c1"); e != NULL;
         e = block_next_equal_key(e)) {
        ++seen;
    }
    CHECK(seen == 3, "iteration walks all three siblings");
    block_close(bf);
}

static void test_getters(void)
{
    BlockFile* bf = parse(
        "G {\n"
        " B1 TRUE\n B2 yes\n B3 ON\n B4 FALSE\n B5 NO\n B6 off\n B7 maybe\n"
        " I1 -5\n I2 42\n F1 1.5\n S1 0,-1:0,-1\n"
        "}\n");
    CHECK(bf != NULL, "getters: parse");
    CHECK(block_get_bool(bf, "G B1") == 1, "bool TRUE");
    CHECK(block_get_bool(bf, "G B2") == 1, "bool yes");
    CHECK(block_get_bool(bf, "G B3") == 1, "bool ON");
    CHECK(block_get_bool(bf, "G B4") == 0, "bool FALSE");
    CHECK(block_get_bool(bf, "G B5") == 0, "bool NO");
    CHECK(block_get_bool(bf, "G B6") == 0, "bool off");
    CHECK(block_get_bool(bf, "G B7") == 2, "bool junk -> 2");
    CHECK(block_get_bool(bf, "G missing") == 2, "bool missing -> 2");
    CHECK(block_get_int(bf, "G I1", 99) == -5, "int negative");
    CHECK(block_get_int(bf, "G I2", 99) == 42, "int positive");
    CHECK(block_get_int(bf, "G nope", 7) == 7, "int default");
    CHECK(block_get_float(bf, "G F1", 9.f) == 1.5f, "float");
    CHECK(block_get_float(bf, "G nope", 9.f) == 9.f, "float default");
    char buf[4];
    CHECK(block_get_str(bf, "G S1", buf, sizeof(buf))
        && strcmp(buf, "0,-") == 0, "str truncates safely");
    CHECK(strcmp(block_get_strptr(bf, "G S1"), "0,-1:0,-1") == 0, "strptr full value");
    CHECK(block_get_strptr(bf, "G nope") == NULL, "strptr missing -> NULL");

    /* empty corpus: parse-only, zero records, no crash */
    BlockFile* zero = block_parse((char*)"", 0);
    CHECK(zero != NULL && block_record_count(zero) == 0, "empty buffer parses clean");
    CHECK(block_find(zero, "anything") == NULL, "empty: find -> NULL");
    block_close(zero);
    block_close(bf);
}

/* ------------------------------------------------------------------ *
 *  Real-wad validation (--real <dir>).
 * ------------------------------------------------------------------ */

#define REAL_ENTRIES 138   /* 56 .ae + 44 .ol + 37 .ptl + 1 Lego.cfg */
#define REAL_RECORDS 12876 /* corpus record count, extracted via tools/rrwad.py */
#define REAL_FNV     0x1a0f0c04u /* FNV-1a over every folded record, same order */

static uint32_t s_fnv(uint32_t h, const char* s)
{
    for (; *s; s++) {
        h = (h ^ (uint32_t)(unsigned char)s[0]) * 16777619u;
    }
    return h;
}

/* fold one record into the digest (name, 0xff separator, value-or-empty) */
static void s_fold(BlockRec* r, uint32_t* h)
{
    const char* nm = block_name(r);
    if (nm != NULL) {
        *h = s_fnv(*h, nm);
    }
    *h = s_fnv(*h, "\xff");
    if (block_value(r) != NULL) {
        *h = s_fnv(*h, block_value(r));
    }
}

static void test_real(const char* dir)
{
    int mounted = resource_mount(dir, "LegoRR");
    CHECK(mounted >= 2, "real: both wads mounted");

    static char seen[200][256];
    int nseen = 0;
    unsigned parsed = 0, ae = 0, ol = 0, ptl = 0, cfg = 0;
    uint32_t h = 2166136261u;
    unsigned total_rec = 0;

    for (int i = 0; i < resource_mounted_count(); i++) {
        uint32_t n = resource_wad_entry_count(i);
        for (uint32_t j = 0; j < n; j++) {
            const char* nm = resource_wad_entry_name(i, j);
            if (nm == NULL) {
                continue;
            }
            int is_ae = s_ext_is(nm, "ae"), is_ol = s_ext_is(nm, "ol");
            int is_ptl = s_ext_is(nm, "ptl"), is_cfg = s_ext_is(nm, "cfg");
            if (!(is_ae || is_ol || is_ptl || is_cfg)) {
                continue;
            }
            int dup = 0;
            for (int k = 0; k < nseen; k++) {
                if (strcasecmp(seen[k], nm) == 0) {
                    dup = 1;
                    break;
                }
            }
            if (dup) {
                continue;
            }
            snprintf(seen[nseen < 200 ? nseen : 199], sizeof(seen[0]), "%s", nm);
            nseen++;

            Resource* res = resource_open(nm, RESOURCE_WAD_ONLY);
            if (res == NULL || resource_read(res) <= 0) {
                printf("FAIL: real: open %s\n", nm);
                ++s_failures;
                if (res != NULL) {
                    resource_free(res);
                }
                continue;
            }
            BlockFile* bf = block_parse((char*)res->data, (size_t)res->size);
            resource_free(res);
            if (bf == NULL) {
                printf("FAIL: real: parse %s\n", nm);
                ++s_failures;
                continue;
            }
            parsed++;
            if (is_ae) { ae++; } else if (is_ol) { ol++; }
            else if (is_ptl) { ptl++; } else { cfg++; }

            total_rec += (unsigned)block_record_count(bf);
            for (size_t k = 0; k < block_record_count(bf); k++) {
                s_fold(block_record_at(bf, k), &h);
            }
            block_close(bf);
        }
    }

    CHECK(parsed == REAL_ENTRIES, "real: 138 block-text entries parsed");
    CHECK(ae == 56, "real: 56 .ae");
    CHECK(ol == 44, "real: 44 .ol");
    CHECK(ptl == 37, "real: 37 .ptl");
    CHECK(cfg == 1, "real: 1 Lego.cfg");
    CHECK(total_rec == REAL_RECORDS && h == REAL_FNV,
        "real: corpus record digest matches extracted ground truth");
    printf("real: %u records, digest %08x\n", total_rec, h);
}

static void test_real_spots(const char* dir)
{
    (void)dir; /* opens go through the mounted resource layer */
    BlockFile* bf = block_open("Buildings\\Barracks\\Barracks.ae");
    CHECK(bf != NULL, "spot: Barracks.ae opens");
    if (bf != NULL) {
        CHECK(val_is(bf, "Lego* Scale", "1.0"), "spot: Barracks Scale");
        CHECK(val_is(bf, "LegoRR Scale", "1.0"), "spot: Barracks exe-root query");
        CHECK(val_is(bf, "Lego* Activities Activity_Teleport", "Create"),
            "spot: Barracks Activity_Teleport");
        CHECK(val_is(bf, "LegoRR Activities Activity_Teleport", "Create"),
            "spot: same record via '*' root");
        CHECK(val_is(bf, "Lego* PowerOff FILE", "Barracks_Idle"),
            "spot: Barracks PowerOff FILE");
        block_close(bf);
    }

    bf = block_open("Creatures\\IceMonster\\IceMonster.ae");
    CHECK(bf != NULL, "spot: IceMonster.ae opens");
    if (bf != NULL) {
        CHECK(val_is(bf, "Lego* Scale", "1.0"), "spot: Ice Scale");
        CHECK(block_get_bool(bf, "Lego* CameraFlipDir") == 1, "spot: Ice CameraFlipDir");
        CHECK(block_find(bf, "LegoLow*") != NULL, "spot: Ice LegoLow* sibling root");
        CHECK(val_is(bf, "Lego* HIGHPOLY Arml", "HArml"), "spot: Ice HIGHPOLY map");
        block_close(bf);
    }

    bf = block_open("Lego.cfg");
    CHECK(bf != NULL, "spot: Lego.cfg opens");
    if (bf != NULL) {
        CHECK(block_get_int(bf, "Lego* Main TextureUsage", 0) == 3145728,
            "spot: cfg TextureUsage");
        CHECK(val_is(bf, "Lego* Main CreditsTextFile", "Credits.txt"),
            "spot: cfg CreditsTextFile");
        block_close(bf);
    }
}

int main(int argc, char** argv)
{
    test_basic();
    test_wildcard();
    test_colon_sep();
    test_comments_braces();
    test_multiroot();
    test_equal_key();
    test_getters();

    if (argc >= 3 && strcmp(argv[1], "--real") == 0) {
        test_real(argv[2]);
        test_real_spots(argv[2]);
    }

    if (s_failures == 0) {
        printf("block: all tests passed\n");
        return 0;
    }
    printf("block: %d FAILURES\n", s_failures);
    return 1;
}
