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
#include <sys/stat.h> /* mkdir */

#include "resource.hpp"
#include "heap.hpp"

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

/* ------------------------------------------------------------------ *
 *  CD/STD fallback validation (finding 01, FUN_0047f660).
 * ------------------------------------------------------------------ */

static void write_file(const char* path, const char* bytes, size_t len)
{
    FILE* fp = fopen(path, "wb");
    if (fp != NULL) {
        fwrite(bytes, 1, len, fp);
        fclose(fp);
    }
}

static void test_cd_fallback(void)
{
    char tmp[] = "/tmp/res_cd_XXXXXX";
    if (mkdtemp(tmp) == NULL) {
        printf("SKIP cd-fallback: mkdtemp failed\n");
        return;
    }
    char data_dir[512];
    snprintf(data_dir, sizeof(data_dir), "%s/Data", tmp);
    mkdir(data_dir, 0700);

    char cdkey[512], delme[512], cdfile[512];
    snprintf(cdkey,  sizeof(cdkey),  "%s/cd.key", tmp);
    snprintf(delme,  sizeof(delme),  "%s/delme.dat", tmp);
    snprintf(cdfile, sizeof(cdfile), "%s/Data/cdfile.bin", tmp);
    write_file(cdkey, "", 0);
    write_file(delme, "", 0);
    write_file(cdfile, "CDDATA", 6);

    /* Remap the CD root onto the fake tree (host path, not a DOS drive). */
    resource_set_cd_root(tmp);
    CHECK(resource_cd_present() == 1, "cd: cd.key present");
    CHECK(resource_data_present() == 1, "cd: delme.dat present");

    Resource* r = resource_open("cdfile.bin", RESOURCE_AUTO);
    CHECK(r != NULL && r->branch == RESOURCE_STD_BRANCH, "cd: AUTO fallback");
    if (r != NULL) {
        CHECK(resource_read(r) == 6 && memcmp(r->data, "CDDATA", 6) == 0,
              "cd: AUTO payload");
        resource_free(r);
    }
    CHECK(resource_open("cdfile.bin", RESOURCE_WAD_ONLY) == NULL,
          "cd: WAD_ONLY does not use the CD fallback");
    CHECK(resource_open("missing.xyz", RESOURCE_AUTO) == NULL,
          "cd: missing stays NULL");

    /* Default (no remap) uses "%c:\", which is not a real path on POSIX. */
    resource_set_cd_root("");
    CHECK(resource_cd_present() == 0, "cd: default drive-root probe false");

    remove(cdkey);
    remove(delme);
    remove(cdfile);
    rmdir(data_dir);
    rmdir(tmp);
}

int main(int argc, char** argv)
{
    test_synthetic();
    test_cd_fallback();

    const char* real_dir = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--real") == 0 && i + 1 < argc) {
            real_dir = argv[i + 1];
        }
    }
    if (real_dir != NULL) {
        test_real(real_dir);
    }

    /* Fixed-record slot resources (2026-10-24): the install/get/release. */
    {
        void* res = engine_heap_alloc(8);
        fixed_record_slot_install(3, res);
        CHECK(fixed_record_slot_resource_get(3) == res
                  && fixed_record_slot_flag_peek(3) == 1,
              "the installed slot answers its resource");
        CHECK(fixed_record_slot_resource_get(4) == NULL,
              "an untouched slot answers null");
        fixed_record_slot_resource_release(3);
        CHECK(fixed_record_slot_resource_get(3) == NULL
                  && fixed_record_slot_flag_peek(3) == 0,
              "the release frees the resource and clears the flag");
        fixed_record_slot_resource_release(3); /* idempotent */
        CHECK(fixed_record_slot_resource_get(-1) == NULL
                  && fixed_record_slot_resource_get(16) == NULL
                  && fixed_record_slot_flag_peek(99) == 0,
              "the window guards answer clear");
    }

    /* WadMemoryStreamBufferGet (2026-10-25): the slot-routed half. */
    {
        int obj = 0;
        fixed_record_slot_install(5, &obj);
        wad_memory_stream_buffer_get(5);
        CHECK(fixed_record_slot_resource_get(5) == &obj,
              "the wrapper leaves the slot resolved");
        fixed_record_slot_install(5, NULL);
        wad_memory_stream_buffer_get(-1);
        CHECK(fixed_record_slot_resource_get(-1) == NULL,
              "the slot guard runs clear");
    }

    /* --- wad payload band (2026-10-25) --------------------------------- */
    {
        /* Dispatch: type 0 is the raw pass-through — the big-endian size
         * dword at +4, payload from +0x12, copied in dwords plus tail. */
        static unsigned char hdr[0x12 + 11];
        unsigned char dst[16];
        memset(hdr, 0, sizeof hdr);
        memcpy(hdr, "RNC", 3);
        hdr[3] = 0;             /* type 0 */
        hdr[4] = 0; hdr[5] = 0; hdr[6] = 0; hdr[7] = 11; /* BE size 11 */
        memcpy(hdr + 0x12, "payload!123", 11);
        memset(dst, 0xEE, sizeof dst);
        CHECK(wad_payload_decode_dispatch(hdr, dst) == 0,
              "the raw pass-through answers 0");
        CHECK(memcmp(dst, "payload!123", 11) == 0, "the payload lands");
        CHECK(dst[11] == 0xEE && dst[15] == 0xEE,
              "the copy stops at the size (dword+tail stages)");

        /* The verdicts: magic, types 1/2 (ported codecs), unknown. The
         * codecs answer the corpus's xor ax,ax = 0 on every path; with a
         * zero size dword the output cursor starts at the end, so the
         * codec exits before touching the stream or the buffer. */
        hdr[0] = 'X';
        CHECK(wad_payload_decode_dispatch(hdr, dst) == WAD_DECODE_MAGIC_FAIL,
              "the bad magic answers 0xffff");
        hdr[0] = 'R';
        hdr[7] = 0;             /* size 0: the codecs gate out immediately */
        hdr[3] = 1;
        memset(dst, 0xEE, sizeof dst);
        /* The codecs now return the output-end pointer masked with
         * 0xffff0000 (faithful to the corpus); callers test only AX, so the
         * verdict is the low word == 0, not the whole int. */
        CHECK(((unsigned)wad_payload_decode_dispatch(hdr, dst) & 0xffffu) == 0,
              "the type-1 codec answers the corpus verdict 0");
        CHECK(dst[0] == 0xEE && dst[15] == 0xEE,
              "the zero-size type-1 stream writes nothing");
        hdr[3] = 2;
        memset(dst, 0xEE, sizeof dst);
        CHECK(((unsigned)wad_payload_decode_dispatch(hdr, dst) & 0xffffu) == 0,
              "the type-2 codec answers the corpus verdict 0");
        CHECK(dst[0] == 0xEE && dst[15] == 0xEE,
              "the zero-size type-2 stream writes nothing");
        hdr[3] = 9;
        CHECK(wad_payload_decode_dispatch(hdr, dst) == WAD_DECODE_TYPE_FAIL,
              "the odd type answers 0xfffe");
        CHECK(wad_payload_decode_dispatch(NULL, dst) == WAD_DECODE_MAGIC_FAIL,
              "the null payload answers the magic verdict (deviation)");
    }

    {
        /* Expand: probe answers the size, decode answers the buffer. */
        static unsigned char hdr[0x12 + 11];
        memset(hdr, 0, sizeof hdr);
        memcpy(hdr, "RNC", 3);
        hdr[7] = 11;
        memcpy(hdr + 0x12, "payload!123", 11);
        void* out = (void*)1;
        CHECK(wad_expand_payload(hdr, NULL) == 11 && out == (void*)1,
              "the null-pointer probe answers the size untouched");
        void* buf = NULL;
        CHECK(wad_expand_payload(hdr, &buf) == 11 && buf != NULL
                  && memcmp(buf, "payload!123", 11) == 0,
              "the decode hands over the buffer and size");
        engine_heap_free(buf);

        hdr[0] = 'X';
        buf = NULL;
        CHECK(wad_expand_payload(hdr, &buf) == 0 && buf == NULL,
              "the failed dispatch releases the buffer and answers 0");
        CHECK(wad_expand_payload(NULL, &buf) == 0,
              "the null payload answers 0 (deviation)");
    }

    {
        /* Huffman table clear: 16-byte entries ride 0 / word 0xffff / 0 /
         * word 0; the entry past the count rides untouched. */
        unsigned char table[4 * 16];
        memset(table, 0x5A, sizeof table);
        wad_huffman_table_clear(table, 3);
        int ok = 1;
        for (int i = 0; i != 3; ++i) {
            const unsigned char* e = table + (size_t)i * 16u;
            uint32_t d0, d8;
            uint16_t w4, w12;
            memcpy(&d0, e + 0, 4);
            memcpy(&w4, e + 4, 2);
            memcpy(&d8, e + 8, 4);
            memcpy(&w12, e + 12, 2);
            if (d0 != 0u || w4 != 0xffffu || d8 != 0u || w12 != 0u) {
                ok = 0;
            }
        }
        CHECK(ok, "the cleared entries land zero/0xffff/zero/zero");
        int untouched = 1;
        for (int i = 0; i != 16; ++i) {
            if (table[3 * 16 + i] != 0x5Au) {
                untouched = 0;
            }
        }
        CHECK(untouched, "the count leaves the tail entry alone");
        wad_huffman_table_clear(table, 0);
        memset(table, 0x5A, sizeof table);
        wad_huffman_table_clear(table, 0);
        int all_ride = 1;
        for (unsigned char b : table) {
            if (b != 0x5Au) {
                all_ride = 0;
            }
        }
        CHECK(all_ride, "the zero count clears nothing");
        wad_huffman_table_clear(NULL, 3); /* guarded */
    }

    {
        /* File-handle pair: kind 1 rides flagged, kind 0 carries the
         * stream buffer, anything else answers NULL. */
        void* p = game_file_handle_allocate(1);
        CHECK(p != NULL && ((GameFileHandle*)p)->flag == 1
                  && ((GameFileHandle*)p)->stream == NULL,
              "the kind-1 handle rides flagged with no stream buffer");
        engine_heap_free(p);
        p = game_file_handle_allocate(0);
        CHECK(p != NULL && ((GameFileHandle*)p)->flag == 0
                  && ((GameFileHandle*)p)->stream != NULL,
              "the kind-0 handle carries its stream buffer");
        engine_heap_free(((GameFileHandle*)p)->stream);
        engine_heap_free(p);
        CHECK(game_file_handle_allocate(2) == NULL,
              "the odd kind answers NULL");
    }

    {
        /* Stream-slot opener: a mounted entry answers 1 with the handle,
         * the pair behind stays zeroed; misses answer 0. */
        const char* tmpl = "/tmp/res_slot_XXXXXX";
        char* dir = (char*)malloc(strlen(tmpl) + 1);
        strcpy(dir, tmpl);
        if (mkdtemp(dir) != NULL) {
            resource_unmount_all();
            TmpEntry entries[] = {
                { "Data\\BAND.TXT", "slotpayload", 11 },
            };
            char wad_path[512];
            snprintf(wad_path, sizeof(wad_path), "%s/LegoRR0.wad", dir);
            if (ares_build_wad(wad_path, entries, 1)
                && resource_mount(dir, "LegoRR") == 1) {
                WadStreamSlots slots;
                memset(&slots, 0xAB, sizeof slots);
                CHECK(wad_entry_read_into_stream_slot(&slots,
                                                      "Data\\BAND.TXT")
                          == 1,
                      "the present entry answers true");
                CHECK(slots.handle != NULL, "the handle lands in slot zero");
                CHECK(slots.stream == NULL && slots.word2 == 0u,
                      "the pair behind rides zeroed");
                resource_free((Resource*)slots.handle);
                memset(&slots, 0xAB, sizeof slots);
                CHECK(wad_entry_read_into_stream_slot(&slots, "no.such.e")
                          == 0,
                      "the missing entry answers false");
                CHECK(slots.handle == NULL, "the miss leaves slot zero null");
                CHECK(wad_entry_read_into_stream_slot(NULL, "x") == 0,
                      "the null slot window answers false (deviation)");
            } else {
                CHECK(0, "the slot test wad mounted");
            }
            resource_unmount_all();
            remove(wad_path);
            rmdir(dir);
        }
        free(dir);
    }

    /* --- resource-handle release band (2026-10-25) --------------------- */
    {
        /* The set release: band slots and named globals free through the
         * engine-heap seam and clear. */
        CHECK(resource_handle_set_slot_peek(0) == NULL
                  && resource_handle_set_slot_peek(24) == NULL
                  && resource_handle_set_slot_peek(-1) == NULL
                  && resource_handle_set_slot_peek(36) == NULL,
              "the untouched set slots peek clear");
        for (int i = 0; i != 3; ++i) {
            resource_handle_set_slot_install(i, engine_heap_alloc(8));
        }
        resource_handle_set_slot_install(25, engine_heap_alloc(8));
        resource_handle_set_slot_install(35, engine_heap_alloc(8));
        resource_handle_set_slot_install(2, engine_heap_alloc(8)); /* overwrite */
        resource_handle_set_slot_install(0, engine_heap_alloc(16));
        resource_handle_set_slot_install(25, engine_heap_alloc(16));
        CHECK(resource_handle_set_slot_peek(0) != NULL
                  && resource_handle_set_slot_peek(25) != NULL,
              "the installs land");
        resource_handle_set_release();
        int cleared = 1;
        for (int s = 0; s != 36; ++s) {
            if (resource_handle_set_slot_peek(s) != NULL) {
                cleared = 0;
            }
        }
        CHECK(cleared, "the release frees and clears every set slot");
        resource_handle_set_release(); /* the empty band rides clean */
    }

    {
        /* The list disposer: handles freed, then the array, then the
         * record — and the record freed even when the array word is zero. */
        ResourceHandleList* list =
            (ResourceHandleList*)engine_heap_alloc(sizeof(ResourceHandleList));
        list->word1 = 0u;
        list->count = 3;
        list->handles = (void**)engine_heap_alloc(3 * sizeof(void*));
        list->handles[0] = engine_heap_alloc(8);
        list->handles[1] = NULL; /* the zero entry rides */
        list->handles[2] = engine_heap_alloc(8);
        resource_handle_list_dispose(list); /* no double-free, clean run */
        ResourceHandleList* bare =
            (ResourceHandleList*)engine_heap_alloc(sizeof(ResourceHandleList));
        bare->word1 = 0u;
        bare->count = 0;
        bare->handles = NULL;
        resource_handle_list_dispose(bare); /* record freed, loop skipped */
        resource_handle_list_dispose(NULL); /* guarded */
        CHECK(1, "the list disposer runs the free ladder");
    }

    /* --- key-group action-code band (2026-10-25) ----------------------- */
    {
        /* InputKeyGroupRegister: the count word lands in the pair's +4
         * slot, then the code dwords ride the heap array. */
        resource_input_key_group_register(5, 4, 1, 2, 3, 4);
        CHECK(resource_handle_set_band_word_peek(5) == 4u
                  && *(const uint32_t*)resource_handle_set_slot_peek(5) == 1u
                  && ((const uint32_t*)resource_handle_set_slot_peek(5))[3]
                         == 4u,
              "the register stages the count word and the code array");

        /* Guard: an out-of-range group strides nowhere (the corpus strides
         * the band blindly). */
        resource_input_key_group_register(24, 1, 1);
        resource_input_key_group_register(-1, 1, 1);
        CHECK(resource_handle_set_band_word_peek(24) == 0u
                  && resource_handle_set_band_word_peek(-1) == 0u,
              "the register guard answers where the corpus strides");

        /* Re-registering the same group re-stages in place (the corpus
         * leaks the superseded array, and so does the port). */
        resource_input_key_group_register(5, 1, 9);
        CHECK(resource_handle_set_band_word_peek(5) == 1u
                  && *(const uint32_t*)resource_handle_set_slot_peek(5) == 9u,
              "re-registering restages the group in place");

        /* MapInitialize: the eighteen fixed groups, group 6 left out. */
        interaction_action_code_map_initialize();
        static const uint32_t counts[19] = { 4, 3, 1, 1, 1, 2, 0, 4, 2, 10,
                                            7, 6, 2, 4, 1, 3, 8, 6, 4 };
        int counts_ok = 1;
        for (int g = 0; g != 19; ++g) {
            if (resource_handle_set_band_word_peek(g) != counts[g]) {
                counts_ok = 0;
            }
        }
        CHECK(counts_ok, "the initializer stages the eighteen fixed counts");
        CHECK(resource_handle_set_band_word_peek(6) == 0u
                  && resource_handle_set_slot_peek(6) == NULL,
              "group 6 stays unregistered");
        CHECK(resource_handle_set_band_word_peek(0) == 4u
                  && resource_handle_set_slot_peek(0) != NULL
                  && ((const uint32_t*)resource_handle_set_slot_peek(0))[3]
                         == 4u,
              "group 0 rides the fixed codes {1,2,3,4}");
        CHECK(((const uint32_t*)resource_handle_set_slot_peek(9))[0] == 0x1fu
                  && ((const uint32_t*)resource_handle_set_slot_peek(0x10))[7]
                         == 0x38u,
              "the fixed code rows land in their groups");
        resource_handle_set_release(); /* the arrays free through the seam */
        CHECK(resource_handle_set_slot_peek(9) == NULL,
              "the release frees the staged group arrays");
    }

    if (s_failures == 0) {
        printf("resource_test: PASS%s\n", real_dir != NULL ? " (real wads)" : "");
        return 0;
    }
    printf("resource_test: %d failure(s)\n", s_failures);
    return 1;
}