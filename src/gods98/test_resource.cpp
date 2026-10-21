// SPDX-License-Identifier: LGPL-3.0-or-later
/* resource_test — validation for src/gods98/resource.cpp.
 *
 * Always runs a synthetic WWAD round trip (build -> mount -> case-insensitive
 * lookup -> payload fidelity). With `--real <dir>` it additionally mounts the
 * shipped `LegoRR0.wad`/`LegoRR1.wad` (from that dir) and cross-checks known
 * entries from finding 01/21 against the shipped corpus (counts, the
 * L1-shadows-L0 name, .flh/.map/.bmp/.lwo payload tags).
 *
 * Wire-up: meson test 'resource' (suite 'resource'); pass the shipped wads
 * via the `test_wads_dir` meson option.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* rmdir */

#include "resource.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* ------------------------------------------------------------------ *
 *  Synthetic WWAD builder (finding 01 byte layout).
 * ------------------------------------------------------------------ */

typedef struct TmpEntry {
    const char* name;
    const char* payload;
    size_t      payload_len;
} TmpEntry;

static int ares_build_wad(const char* path, TmpEntry* in, int n)
{
    int ok = 0;
    size_t names_bytes = 0;
    size_t unc_bytes = 0;
    uint32_t toc_off = 0;
    uint32_t data_off = 0;
    unsigned char hdr[8];
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        return 0;
    }
    for (int i = 0; i < n; ++i) {
        names_bytes += strlen(in[i].name) + 1;
    }
    unc_bytes = (size_t)n; /* n empty NUL-terminated UNC paths */
    toc_off = 8 + (uint32_t)names_bytes + (uint32_t)unc_bytes;

    memcpy(hdr, "WWAD", 4);
    hdr[4] = (unsigned char)(n & 0xff);
    hdr[5] = (unsigned char)((n >> 8) & 0xff);
    hdr[6] = (unsigned char)((n >> 16) & 0xff);
    hdr[7] = (unsigned char)((n >> 24) & 0xff);
    if (fwrite(hdr, 1, 8, fp) != 8) {
        goto out;
    }
    for (int i = 0; i < n; ++i) {
        if (fwrite(in[i].name, 1, strlen(in[i].name) + 1, fp)
            != strlen(in[i].name) + 1) {
            goto out;
        }
    }
    for (int i = 0; i < n; ++i) { /* empty UNC paths */
        if (fputc(0, fp) == EOF) {
            goto out;
        }
    }
    data_off = toc_off + (uint32_t)n * 16;
    for (int i = 0; i < n; ++i) {
        uint32_t len = (uint32_t)in[i].payload_len;
        uint32_t row[4] = { 1, len, len, data_off };
        if (fwrite(row, 4, 4, fp) != 4) {
            goto out;
        }
        data_off += len;
    }
    for (int i = 0; i < n; ++i) {
        if (fwrite(in[i].payload, 1, in[i].payload_len, fp)
            != in[i].payload_len) {
            goto out;
        }
    }
    ok = 1;
out:
    fclose(fp);
    return ok;
}

static void test_synthetic(void)
{
    resource_unmount_all();
    TmpEntry entries[] = {
        { "Buildings\\A.BMP", "BM\x1a\x04\x00", 5 },
        { "Levels\\GameLevels\\Level01\\01.txt", "hello world", 11 },
        { "Data\\sound1.wav", "RIFF" "\x24" "\x00" "bad", 9 },
    };
    const char* tmpl = "/tmp/res_test_XXXXXX";
    char* dir = (char*)malloc(strlen(tmpl) + 1);
    strcpy(dir, tmpl);
    if (mkdtemp(dir) == NULL) {
        printf("SKIP synthetic: mkdtemp failed\n");
        free(dir);
        return;
    }
    char wad_path[512];
    snprintf(wad_path, sizeof(wad_path), "%s/LegoRR0.wad", dir);

    if (!ares_build_wad(wad_path, entries, 3)) {
        printf("FAIL: synthetic wad build\n");
        ++s_failures;
        return;
    }

    CHECK(resource_mount(dir, "LegoRR") == 1, "synthetic: mount == 1");
    CHECK(resource_wad_entry_count(0) == 3, "synthetic: entry_count == 3");
    CHECK(resource_wad_lookup(0, "Buildings\\A.BMP") == 0,
          "synthetic: exact lookup");
    CHECK(resource_wad_lookup(0, "buildings\\a.bmp") == 0,
          "synthetic: case-insensitive lookup");
    CHECK(resource_wad_lookup(0, "Data\\SOUND1.WAV") == 2,
          "synthetic: case-insensitive lookup 2");
    CHECK(resource_wad_lookup(0, "no.such.file") == -1,
          "synthetic: missing lookup == -1");
    CHECK(resource_wad_entry_stored(0, 1) == 11,
          "synthetic: TOC stored size");

    Resource* r = resource_open("levels\\gamelevels\\level01\\01.txt",
                                RESOURCE_AUTO);
    CHECK(r != NULL, "synthetic: open (mixed case)");
    if (r != NULL) {
        CHECK(resource_read(r) == 11 && memcmp(r->data, "hello world", 11) == 0,
              "synthetic: payload round-trip");
        resource_free(r);
    }
    Resource* miss = resource_open("no.such.file", RESOURCE_AUTO);
    CHECK(miss == NULL, "synthetic: open missing == NULL");

    remove(wad_path);
    rmdir(dir);
    free(dir);
}

/* ------------------------------------------------------------------ *
 *  Real-wad validation (--real <dir>).
 * ------------------------------------------------------------------ */

static void check_tag(const char* name, const char* expect,
                      size_t expect_len, const char* what)
{
    Resource* r = resource_open(name, RESOURCE_WAD_ONLY);
    if (r == NULL) {
        printf("FAIL: %s: open '%s' == NULL\n", what, name);
        ++s_failures;
        return;
    }
    int32_t size = resource_read(r);
    CHECK(size > 0, what);
    if (size > 0 && expect_len <= (size_t)size) {
        CHECK(memcmp(r->data, expect, expect_len) == 0, what);
    }
    resource_free(r);
}

static void test_real(const char* dir)
{
    resource_unmount_all();
    int n = resource_mount(dir, "LegoRR");
    CHECK(n == 2, "real: mounted two wads");
    if (n < 2) {
        return;
    }
    CHECK(resource_wad_entry_count(0) == 5631, "real: L0 entry count == 5631");
    CHECK(resource_wad_entry_count(1) == 214,  "real: L1 entry count == 214");

    /* L1 shadows L0 for names present in both (scan order 9 -> 0). */
    Resource* r = resource_open("levels\\gamelevels\\level01\\01.txt",
                                RESOURCE_AUTO);
    CHECK(r != NULL, "real: shadow open (mixed case)");
    if (r != NULL) {
        CHECK(r->branch == RESOURCE_WAD_BRANCH && r->wad_idx == 1,
              "real: shadow resolves to L1");
        CHECK(resource_read(r) == 582, "real: shadow payload size == 582");
        resource_free(r);
    }

    /* Payload tags from the shipped corpus. */
    check_tag("Buildings\\Barracks\\A000_ladder.BMP", "BM", 2,
              "real: BMP tag");
    check_tag("Buildings\\BIGTeleport\\TP_Base_New.lwo", "FORM", 4,
              "real: LWO tag");
    check_tag("Levels\\GameLevels\\Level01\\Cror_01.map", "MAP ", 4,
              "real: MAP tag");
    check_tag("Interface\\Pointers\\CDRILL.FLH", NULL, 0,
              "real: .flh entry opens (finding 21)");

    r = resource_open("Lego.cfg", RESOURCE_WAD_ONLY);
    CHECK(r != NULL && r->wad_idx == 1, "real: Lego.cfg in L1");
    if (r != NULL) {
        CHECK(resource_read(r) == 237575, "real: Lego.cfg size == 237575");
        resource_free(r);
    }

    Resource* miss = resource_open("no.such.file", RESOURCE_WAD_ONLY);
    CHECK(miss == NULL, "real: missing == NULL");
}

int main(int argc, char** argv)
{
    test_synthetic();

    const char* real_dir = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--real") == 0 && i + 1 < argc) {
            real_dir = argv[i + 1];
        }
    }
    if (real_dir != NULL) {
        test_real(real_dir);
    }

    if (s_failures == 0) {
        printf("resource_test: PASS%s\n", real_dir != NULL ? " (real wads)" : "");
        return 0;
    }
    printf("resource_test: %d failure(s)\n", s_failures);
    return 1;
}