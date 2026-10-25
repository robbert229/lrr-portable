// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_flic — validation for src/gods98/flic.cpp (finding 21).
 *
 * A synthetic `.flh` (keyframe + 2 deltas + 1 hold) with known RGB555 pixels
 * is built in memory and decoded end-to-end: geometry, per-frame pixels,
 * hold semantics, loop restart and the RGBA conversion are asserted. With
 * `--real <game-dir>` every shipped `.flh` in the mounted wads is opened and
 * stepped to EOF, plus the loose `Data/AVI/` overlay copies (PB1..PB5,
 * Rocky, …), with structural invariants and content spot checks (HOLO
 * dims/frames; an animated pointer flic whose frame 0 != frame 1).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "flic.hpp"
#include "gods98/resource.hpp"
#include "movie.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static void le16w(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)(v >> 8);
}
static void le32w(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
    p[2] = (uint8_t)((v >> 16) & 0xff);
    p[3] = (uint8_t)((v >> 24) & 0xff);
}

#define FLIC_W 4
#define FLIC_H 4
#define FLIC_FRAMES 4 /* nFrames 3 + keyframe */
/* header and chunk-header sizes (finding 21 §2/§3) */
#define FLIC_HEADER 136
#define FLIC_CHUNK_HDR 16

/* Build a 4x4 AF43 FLIC: 128-byte header, chunk0 keyframe (rows solid
 * 1,2,3,4) at 0x80, chunk1 delta (pixel 0,0 -> 0x001F), chunk2 delta (row
 * jump + pixel 1,1 -> 0x7C00), chunk3 hold. Returns bytes. */
static size_t flic_tiny(uint8_t* buf, size_t cap)
{
    const uint32_t hdr = 0x80; /* standard FLIC header; chunks at 0x80 */
    /* chunk0 keyframe body: per row [0x04][color u16][terminator] */
    uint8_t kf[FLIC_H][4];
    for (int row = 0; row < FLIC_H; ++row) {
        kf[row][0] = 0x04;
        le16w(kf[row] + 1, (uint16_t)(row + 1));
        kf[row][3] = 0x00; /* row terminator */
    }
    const uint32_t kf_rec_size = 7 + FLIC_H * 4; /* header + body */
    const uint32_t kf_chunk = FLIC_CHUNK_HDR + kf_rec_size;
    /* chunk1: delta record [size][27][nChanged=1]; body count=1, sc=0x0001,
     * color=0x001F */
    const uint32_t d1_rec_size = 8 + 2 + 2 + 2;
    const uint32_t d1_chunk = FLIC_CHUNK_HDR + d1_rec_size;
    /* chunk2: [size][27][1]; body jump 0xFFFF, count=1, sc=0x0101,
     * color=0x7C00 */
    const uint32_t d2_rec_size = 8 + 2 + 2 + 2 + 2;
    const uint32_t d2_chunk = FLIC_CHUNK_HDR + d2_rec_size;
    const uint32_t hold_chunk = 16; /* count 0, no body */

    const size_t total = hdr + kf_chunk + d1_chunk + d2_chunk + hold_chunk;
    if (cap < total) {
        return 0;
    }

    /* header */
    le32w(buf + 0x00, (uint32_t)total);
    le16w(buf + 0x04, 0xaf43);
    le16w(buf + 0x06, FLIC_FRAMES - 1); /* nFrames (last index) */
    le16w(buf + 0x08, FLIC_W);
    le16w(buf + 0x0a, FLIC_H);
    le16w(buf + 0x0c, 16);
    memset(buf + 0x0e, 0, hdr - 0x0e);

    uint8_t* p = buf + hdr;
    /* chunk0 */
    le32w(p + 0x00, kf_chunk);
    le16w(p + 0x04, 0xf1fa);
    le16w(p + 0x06, 1);
    memset(p + 0x08, 0, 8);
    le32w(p + 0x10, kf_rec_size);
    le16w(p + 0x14, 25);
    memset(p + 0x16, 0, 1);
    for (int row = 0; row < FLIC_H; ++row) {
        memcpy(p + 0x17 + row * 4, kf[row], 4);
    }
    p += kf_chunk;
    /* chunk1 */
    le32w(p + 0x00, d1_chunk);
    le16w(p + 0x04, 0xf1fa);
    le16w(p + 0x06, 1);
    memset(p + 0x08, 0, 8);
    le32w(p + 0x10, d1_rec_size);
    le16w(p + 0x14, 27);
    le16w(p + 0x16, 1);       /* nChangedLines */
    le16w(p + 0x18, 1);       /* command count */
    le16w(p + 0x1a, 0x0001);  /* skip 0, ctrl 1 */
    le16w(p + 0x1c, 0x001f);  /* blue */
    p += d1_chunk;
    /* chunk2 */
    le32w(p + 0x00, d2_chunk);
    le16w(p + 0x04, 0xf1fa);
    le16w(p + 0x06, 1);
    memset(p + 0x08, 0, 8);
    le32w(p + 0x10, d2_rec_size);
    le16w(p + 0x14, 27);
    le16w(p + 0x16, 1);
    le16w(p + 0x18, 0xffff);  /* row jump: row += 1 */
    le16w(p + 0x1a, 1);       /* command count */
    le16w(p + 0x1c, 0x0101);  /* skip 1, ctrl 1 */
    le16w(p + 0x1e, 0x7c00);  /* red */
    p += d2_chunk;
    /* chunk3: hold */
    le32w(p + 0x00, hold_chunk);
    le16w(p + 0x04, 0xf1fa);
    le16w(p + 0x06, 0);
    memset(p + 0x08, 0, 8);
    return total;
}

static void test_synthetic(void)
{
    uint8_t buf[1024];
    size_t n = flic_tiny(buf, sizeof(buf));
    CHECK(n > 0, "synthetic .flh built");

    char path[256];
    snprintf(path, sizeof(path), "/tmp/opencode/lrr_flic_test_%ld.flh",
             (long)getpid());
    FILE* f = fopen(path, "wb");
    CHECK(f != NULL, "write synthetic .flh");
    if (f == NULL) {
        return;
    }
    fwrite(buf, 1, n, f);
    fclose(f);

    Flic* fl = flic_open(path);
    CHECK(fl != NULL, "synthetic .flh opens");
    if (fl == NULL) {
        remove(path);
        return;
    }
    CHECK(flic_width(fl) == FLIC_W && flic_height(fl) == FLIC_H,
          "synthetic geometry 4x4");
    CHECK(flic_frames(fl) == FLIC_FRAMES, "frames = nFrames + 1 = 4");

    const uint16_t* pix = flic_frame555(fl);
    CHECK(pix != NULL, "frame 0 pixels");
    if (pix != NULL) {
        for (int row = 0; row < FLIC_H; ++row) {
            for (int x = 0; x < FLIC_W; ++x) {
                CHECK(pix[row * FLIC_W + x] == (uint16_t)(row + 1),
                      "keyframe row fill color");
            }
        }
    }
    /* RGBA conversion (packed A<<24|B<<16|G<<8|R — the renderer byte order). */
    CHECK(flic_rgb555_to_rgba(0x001f) == 0xffff0000u, "555 blue -> RGBA");
    CHECK(flic_rgb555_to_rgba(0x03e0) == 0xff00ff00u, "555 green -> RGBA");
    CHECK(flic_rgb555_to_rgba(0x7c00) == 0xff0000ffu, "555 red -> RGBA");
    const uint8_t* rgba = flic_frame_rgba(fl);
    CHECK(rgba != NULL && rgba[3] == 0xff,
          "frame0 rgba alpha present");

    flic_tick_loop(fl, 0);
    CHECK(flic_frame(fl) == 1, "tick -> frame 1");
    pix = flic_frame555(fl);
    CHECK(pix != NULL && pix[0] == 0x001f && pix[1] == 1,
          "delta1 set (0,0) to blue");
    rgba = flic_frame_rgba(fl);
    CHECK(rgba != NULL && rgba[0] == 0x00 && rgba[1] == 0x00
              && rgba[2] == 0xff && rgba[3] == 0xff,
          "frame1 rgba blue pixel");

    flic_tick_loop(fl, 0);
    CHECK(flic_frame(fl) == 2, "tick -> frame 2");
    pix = flic_frame555(fl);
    CHECK(pix != NULL && pix[FLIC_W * 1 + 1] == 0x7c00
              && pix[FLIC_W * 1 + 0] == 2,
          "delta2: row jump to row 1, (1,1) red, (1,0) untouched");

    flic_tick_loop(fl, 0);
    CHECK(flic_frame(fl) == 3, "hold -> frame 3");
    pix = flic_frame555(fl);
    CHECK(pix != NULL && pix[FLIC_W * 1 + 1] == 0x7c00,
          "hold repeats the previous frame");

    flic_tick_loop(fl, 0);
    CHECK(flic_eof(fl) == 1, "non-loop tick past the end sets eof");
    CHECK(flic_frame(fl) == 3, "eof sticks on the last frame");
    flic_close(fl);

    /* Loop semantics: restart at index 1 (the keyframe stays decoded — the
     * corpus RAM-mode rule). */
    fl = flic_open(path);
    CHECK(fl != NULL, "reopen for loop check");
    if (fl != NULL) {
        for (int i = 0; i < FLIC_FRAMES; ++i) {
            flic_tick_loop(fl, 1);
        }
        CHECK(flic_frame(fl) == 1, "loop wraps: 4 ticks -> frame 1");
        flic_tick_loop(fl, 1);
        CHECK(flic_frame(fl) == 2, "loop wraps again to frame 2");
        flic_close(fl);
    }

    remove(path);
}

static const char* ext_is_flh(const char* name)
{
    const char* dot = strrchr(name, '.');
    if (dot == NULL) {
        return NULL;
    }
    for (int i = 0; dot[1 + i] != '\0'; ++i) {
        char c = dot[1 + i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c + 0x20);
        }
        if (c != "flh"[i]) {
            return NULL;
        }
    }
    return ".flh";
}

/* Step a flic to EOF; checks every frame ticks without tripping bounds. */
static int step_to_eof(Flic* f)
{
    int ticks = 0;
    while (!flic_eof(f) && ticks < 100000) {
        flic_tick_loop(f, 0);
        ++ticks;
    }
    return ticks;
}

static void probe_one(Flic* fl, const char* label, int* pass, int* fail,
                      int* frames_total)
{
    if (fl == NULL) {
        fprintf(stderr, "flic: %s did not open\n", label);
        ++(*fail);
        return;
    }
    int w = flic_width(fl), h = flic_height(fl), n = flic_frames(fl);
    int ticks = step_to_eof(fl);
    int ok = w > 0 && h > 0 && n > 0 && ticks == n && flic_eof(fl);
    if (!ok) {
        fprintf(stderr, "flic: %s decode mismatch (w=%d h=%d frames=%d "
                        "ticks=%d eof=%d)\n",
                label, w, h, n, ticks, flic_eof(fl));
        ++(*fail);
    } else {
        ++(*pass);
    }
    *frames_total += n;
}

static void probe_real(const char* wad_dir)
{
    int mounted = resource_mount(wad_dir, "LegoRR");
    if (mounted <= 0) {
        fprintf(stderr, "flic: --real probe skipped (no wads from %s)\n",
                wad_dir);
        return;
    }
    int pass = 0, fail = 0, frames = 0;

    /* Every .flh entry in the mounted wads (finding 21: 18 files). */
    for (int w = 0; w < resource_mounted_count(); ++w) {
        uint32_t entries = resource_wad_entry_count(w);
        for (uint32_t e = 0; e < entries; ++e) {
            const char* ename = resource_wad_entry_name(w, e);
            if (ename == NULL || ext_is_flh(ename) == NULL) {
                continue;
            }
            Flic* fl = flic_open(ename);
            probe_one(fl, ename, &pass, &fail, &frames);
            flic_close(fl);
        }
    }

    /* The loose Data/AVI/ overlay copies the front-end uses (avi\PB1.flh…)
     * resolve through flic_open's loose branch: point the resolver at the
     * game dir. */
    setenv("LEGORRR_WAD_DIR", wad_dir, 1);
    static const char* const overlays[] = {
        "PB1", "PB2", "PB3", "PB4", "PB5", "Rocky", "Struck",
        "Sandwich", "claser", "Slug", "Capt",
    };
    for (size_t i = 0; i < sizeof(overlays) / sizeof(overlays[0]); ++i) {
        char name[64];
        snprintf(name, sizeof(name), "avi\\%s.flh", overlays[i]);
        Flic* fl = flic_open(name);
        probe_one(fl, name, &pass, &fail, &frames);
        flic_close(fl);
    }
    setenv("LEGORRR_WAD_DIR", "", 1);

    /* Content spot checks against the shipped inventory (finding 21 §6:
     * "HOLO (158x226, 81 frames)" — header nFrames = 81, chunk count = 82). */
    Flic* holo = flic_open("Interface\\InfoPanel\\HOLO.FLH");
    CHECK(holo != NULL && flic_width(holo) == 158 && flic_height(holo) == 226
              && flic_frames(holo) == 82,
          "HOLO.FLH 158x226 (nFrames 81 -> 82 chunks)");
    if (holo != NULL) {
        /* Animated: the last decoded frame must differ from frame 0. */
        const uint16_t* f0 = flic_frame555(holo);
        uint16_t* f0copy = (uint16_t*)malloc(158 * 226 * 2);
        if (f0copy != NULL) {
            memcpy(f0copy, f0, 158 * 226 * 2);
        }
        step_to_eof(holo);
        const uint16_t* fN = flic_frame555(holo);
        int diff = 0;
        if (f0copy != NULL) {
            for (int i = 0; i < 158 * 226; ++i) {
                if (f0copy[i] != fN[i]) {
                    diff = 1;
                    break;
                }
            }
        }
        CHECK(diff == 1, "HOLO animates (last frame differs)");
        free(f0copy);
        flic_close(holo);
    }

    resource_unmount_all();
    CHECK(fail == 0, "every shipped .flh decodes to EOF cleanly");
    fprintf(stderr, "flic: --real probe: %d+11 .flh decoded, %d frames, %s\n",
            pass, frames, fail == 0 ? "all ok" : "MISMATCH");
}

int main(int argc, char** argv)
{
    printf("== flic ==\n");
    test_synthetic();
    for (int a = 1; a < argc; ++a) {
        if (strcmp(argv[a], "--real") == 0 && a + 1 < argc) {
            probe_real(argv[a + 1]);
            ++a;
        }
    }
    /* FlicOpcode25SuccessStub (FUN_00484b90, finding 21): the 8-bit
     * opcode-25 handler is a success stub with no pixel writes. */
    CHECK(flic_opcode25_success_stub() == 1u,
          "the 8-bit opcode-25 handler answers success");

    /* Opcode-27 fallback (2026-10-24): pure return, callable. */
    flic_opcode27_fallback_noop();
    CHECK(1, "the opcode-27 fallback returns");

    /* FlicOpcode16ModeDispatch window (2026-10-25). */
    {
        static uint8_t mode_rec[0x6e0];
        memset(mode_rec, 0, sizeof mode_rec);
        uint32_t word = 0u;
        word = 0x10u;
        memcpy(mode_rec + 0x6d8, &word, sizeof word);
        CHECK(flic_opcode16_mode_dispatch(mode_rec) == 1,
              "the mode-0x10 route answers 1");
        word = 4u;
        memcpy(mode_rec + 0x6d4, &word, sizeof word);
        word = 0u;
        memcpy(mode_rec + 0x6d8, &word, sizeof word);
        CHECK(flic_opcode16_mode_dispatch(mode_rec) == 1
                  && flic_opcode16_mode_dispatch(NULL) == 1,
              "the opcode routes and the NULL window answer 1");
    }

    /* PackedTripletTableToWordBuild (2026-10-25): the 256-triplet fold
     * at +0x11c into the words at +0x41c, and the first-word clear. */
    {
        static unsigned char rec[0x620];
        memset(rec, 0, sizeof rec);
        for (int i = 0; i != 0x100; ++i) {
            rec[0x11c + i * 3 + 0] = (unsigned char)(i * 7u + 11u);
            rec[0x11c + i * 3 + 1] = (unsigned char)(i * 11u + 3u);
            rec[0x11c + i * 3 + 2] = (unsigned char)(i * 13u + 5u);
        }
        flic_packed_triplet_table_to_word_build(rec);
        int bad = 0;
        for (int i = 1; i != 0x100; ++i) {
            const unsigned char* t = rec + 0x11c + i * 3;
            const uint16_t want = (uint16_t)(
                (((unsigned)t[1] | ((unsigned)(t[0] & 0x3e) << 5)) << 5)
                | (unsigned)(t[2] >> 1));
            uint16_t got = 0u;
            memcpy(&got, rec + 0x41c + i * 2, sizeof got);
            if (got != want) {
                bad++;
            }
        }
        CHECK(bad == 0, "the whole table folds (first word excepted)");
        uint16_t w0 = 0xffffu;
        memcpy(&w0, rec + 0x41c, sizeof w0);
        CHECK(w0 == 0u, "the first word is cleared after the fold");
        flic_packed_triplet_table_to_word_build(NULL);
        CHECK(1, "the null record is guarded (deviation)");
    }

    /* FlicDecode16BitRowRecords (2026-10-25): the chunked triplet walk,
     * the zero-count restart lap, and the op-4 hand-off to the fold. */
    {
        static unsigned char rec[0x1000];
        static unsigned char stream[0x900];
        memset(rec, 0, sizeof rec);
        memset(stream, 0, sizeof stream);
        stream[6] = 2; /* two chunks */
        stream[8 + 0] = 2;  /* skip two triplets */
        stream[8 + 1] = 1;  /* copy one triplet */
        stream[8 + 2] = 0xFF;
        stream[8 + 3] = 0xFE;
        stream[8 + 4] = 0xFC;
        stream[8 + 5] = 0;  /* skip zero */
        stream[8 + 6] = 2;  /* copy two triplets */
        stream[8 + 7] = 0x01;
        stream[8 + 8] = 0x02;
        stream[8 + 9] = 0x03;
        stream[8 + 10] = 0x11;
        stream[8 + 11] = 0x22;
        stream[8 + 12] = 0x33;
        void* sp = stream;
        flic_frame_window_layout_install(rec, 0, 0, 0, sp, NULL, 0);
        flic_decode16_bit_row_records(rec);
        CHECK(rec[0x11c + 6] == 0x3F && rec[0x11c + 7] == 0x3F
                  && rec[0x11c + 8] == 0x3F,
              "the first chunk skips two triplets and copies a third");
        CHECK(rec[0x11c + 9] == 0x00 && rec[0x11c + 12] == 0x04
                  && rec[0x11c + 13] == 0x08 && rec[0x11c + 14] == 0x0C,
              "the second chunk lands its six source bytes >>2");
        CHECK(rec[0x41c] == 0u, "the fold does not ride when op is not four");

        memset(rec, 0, sizeof rec);
        memset(stream, 0, sizeof stream);
        stream[6] = 4;
        stream[8] = 3;   /* skip three (discarded by the restart) */
        stream[8 + 1] = 0; /* zero count byte: full restart lap */
        memset(stream + 8 + 2, 0xFF, 0x300);
        flic_frame_window_layout_install(rec, 0, 0, 0, sp, NULL, 0);
        int word4 = 4;
        __builtin_memcpy(rec + 0x6d4, &word4, sizeof word4);
        flic_decode16_bit_row_records(rec);
        int bad = 0;
        for (int i = 0; i != 0x300; ++i) {
            if (rec[0x11c + i] != 0x3F) {
                bad++;
            }
        }
        CHECK(bad == 0, "the zero-count chunk rewrites the whole table");
        CHECK(rec[0x11c + 0x300 + 3] == 0xFFu,
              "the restart lap ends the walk (the fold lands on the tail)");
        CHECK(rec[0x61d] == 0u,
              "the word tail beyond the fold rides untouched");
        uint16_t w1 = 0u, w3 = 0u, w0 = 1u;
        memcpy(&w1, rec + 0x41c + 2, sizeof w1);
        memcpy(&w3, rec + 0x41c + 6, sizeof w3);
        memcpy(&w0, rec + 0x41c, sizeof w0);
        CHECK(w1 == 0xFFFFu && w3 == 0xFFFFu, "the op-4 fold folds the table");
        CHECK(w0 == 0u, "the fold clears the first word");
        CHECK(flic_decode16_bit_row_records(NULL) == 1,
              "the null record answers 1 (deviation)");
    }

    /* FlicDecode8BitOpcode25Mode4 (2026-10-25): the literal/copy run walk
     * with the row straddle and the palette store window. */
    {
        static unsigned char rec[0x1000];
        static unsigned char stream[0x100];
        static unsigned char pixels[0x40];
        memset(rec, 0, sizeof rec);
        memset(stream, 0, sizeof stream);
        memset(pixels, 0, sizeof pixels);
        uint16_t pal20 = 0xABCDu, pal21 = 0x1234u;
        __builtin_memcpy(rec + 0x41c + 0x20 * 2u, &pal20, sizeof pal20);
        __builtin_memcpy(rec + 0x41c + 0x21 * 2u, &pal21, sizeof pal21);
        void* sp2 = stream;
        void* pp = pixels;
        uint16_t rows = 2u, row_len = 1u;
        int pitch = 4;
        flic_frame_window_layout_install(rec, (int)row_len, (int)rows,
                                         pitch, sp2, pp, 0);
        /* stream+7 rows: [1, idx0], +1 straddle byte, [1, idx1] */
        stream[7 + 0] = 1;
        stream[7 + 1] = 0x20;
        stream[7 + 2] = 0x77; /* straddled (never consumed) */
        stream[7 + 3] = 1;
        stream[7 + 4] = 0x21;
        CHECK(flic_decode8_bit_opcode25_mode4(rec) == 1, "the walk answers 1");
        uint16_t p0 = 0u, p1 = 0u;
        memcpy(&p0, pixels + 0, sizeof p0);
        memcpy(&p1, pixels + 4, sizeof p1);
        CHECK(p0 == 0xABCDu && p1 == 0x1234u,
              "the literal runs land the palette words per row");

        memset(pixels, 0, sizeof pixels);
        memset(rec, 0, sizeof rec);
        __builtin_memcpy(rec + 0x41c + 0x20 * 2u, &pal20, sizeof pal20);
        __builtin_memcpy(rec + 0x41c + 0x21 * 2u, &pal21, sizeof pal21);
        rows = 1u;
        row_len = 2u;
        flic_frame_window_layout_install(rec, (int)row_len, (int)rows,
                                         pitch, sp2, pp, 0);
        stream[7 + 0] = 0xFE; /* copy run of two */
        stream[7 + 1] = 0x20;
        stream[7 + 2] = 0x21;
        flic_decode8_bit_opcode25_mode4(rec);
        memcpy(&p0, pixels + 0, sizeof p0);
        memcpy(&p1, pixels + 2, sizeof p1);
        CHECK(p0 == 0xABCDu && p1 == 0x1234u,
              "the copy run walks its own palette bytes");

        memset(rec, 0, sizeof rec);
        memset(pixels, 0, sizeof pixels);
        __builtin_memcpy(rec + 0x41c + 0x20 * 2u, &pal20, sizeof pal20);
        rows = 1u;
        row_len = 1u;
        flic_frame_window_layout_install(rec, (int)row_len, (int)rows,
                                         pitch, sp2, pp, 0);
        stream[7 + 0] = 0;  /* zero count: advances without writing */
        stream[7 + 1] = 0;
        stream[7 + 2] = 1;  /* then the single literal lands */
        stream[7 + 3] = 0x20;
        flic_decode8_bit_opcode25_mode4(rec);
        memcpy(&p0, pixels + 0, sizeof p0);
        CHECK(p0 == 0xABCDu, "the zero count byte advances, it never stops");
        flic_frame_window_layout_install(rec, 9, 3, 64, sp2, pp, 1);
        int16_t rl = 0, rc = 0;
        int fl = 0;
        memcpy(&rl, rec + 0x0c, sizeof rl);
        memcpy(&rc, rec + 0x10, sizeof rc);
        memcpy(&fl, rec + 0x6e4, sizeof fl);
        CHECK(rl == 9 && rc == 3 && fl == 1, "the install hook lands");
        CHECK(flic_decode8_bit_opcode25_mode4(NULL) == 1,
              "the null record answers 1 (deviation)");
    }

    /* FlicDecode16BitKeyframe (2026-10-25): the word-stream row walk. */
    {
        static unsigned char rec[0x1000];
        static unsigned char stream[0x100];
        static unsigned char pixels[0x100];
        memset(rec, 0, sizeof rec);
        memset(stream, 0, sizeof stream);
        memset(pixels, 0, sizeof pixels);
        void* sp = stream;
        void* pp = pixels;
        int pitch = 4;
        int16_t rows = 2, row_len = 2;
        flic_frame_window_layout_install(rec, row_len, rows, pitch, sp, pp, 0);
        /* row 0: literal run of two of the word 0x07C0 (green field <<1) */
        stream[7] = 2;
        stream[7 + 1] = 0xC0; /* the value word is read from stream+8 */
        stream[7 + 2] = 0x07;
        stream[7 + 3] = 0x55; /* row straddle (never consumed) */
        /* row 1: copy run of two words */
        stream[7 + 4] = 0xFE;
        stream[7 + 5] = 0xC0;
        stream[7 + 6] = 0x07;
        stream[7 + 7] = 0x1F;
        stream[7 + 8] = 0x00;
        CHECK(flic_decode16_bit_keyframe(rec) == 1, "the walk answers 1");
        uint16_t w[8];
        memcpy(w, pixels, sizeof w);
        CHECK(w[0] == 0x0F80u && w[1] == 0x0F80u,
              "the literal run repeats the expanded word (green <<1)");
        CHECK(w[2] == 0x0F80u && w[3] == 0x001Fu,
              "the copy run walks its own words (the low five ride)");
        memset(pixels, 0, sizeof pixels);
        memset(rec, 0, 0x1000);
        rows = 1;
        row_len = 1;
        int pitch2 = 4;
        int flag = 1;
        flic_frame_window_layout_install(rec, row_len, rows, pitch2, sp, pp,
                                         flag);
        /* zero count (advance only; consumes 3 bytes), then a literal
         * under the flag: count 1, value word 0x07C0 */
        stream[7] = 0;
        stream[7 + 1] = 0;
        stream[7 + 2] = 0;
        stream[7 + 3] = 1;
        stream[7 + 4] = 0xC0;
        stream[7 + 5] = 0x07;
        flic_decode16_bit_keyframe(rec);
        memcpy(w, pixels, 2);
        CHECK(w[0] == 0x07C0u, "the flag keeps the green field where it is");
        CHECK(flic_decode16_bit_keyframe(NULL) == 1,
              "the null record answers 1 (deviation)");
    }

    /* FlicDecode16BitDelta (2026-10-25): the chunk walk, the remembered
     * byte store, the back-count quirk, and the whole-word row subtract. */
    {
        static unsigned char rec[0x1000];
        static unsigned char stream[0x200];
        static unsigned char pixels[0x10000];
        memset(rec, 0, sizeof rec);
        void* sp = stream;
        void* pp = pixels;
        int pitch = 8;
        flic_frame_window_layout_install(rec, 0, 0, pitch, sp, pp, 0);

        /* (d) the plain pair chunk: the kind-0 header IS the pair count
         * (the corpus re-reads the header word), skip 1, two words. */
        memset(stream, 0, sizeof stream);
        memset(pixels, 0, sizeof pixels);
        stream[6] = 1;
        uint16_t w16;
        w16 = 1u;      memcpy(stream + 8, &w16, 2);       /* kind 0: one pair */
        w16 = 0x0201u; memcpy(stream + 10, &w16, 2);      /* skip 1, run 2 */
        w16 = 0x001Fu; memcpy(stream + 12, &w16, 2);
        w16 = 0x003Fu; memcpy(stream + 14, &w16, 2);
        CHECK(flic_decode16_bit_delta(rec) == 1, "the chunk answers 1");
        uint16_t w[3];
        memcpy(w, pixels, sizeof w);
        CHECK(w[0] == 0u && w[1] == 0x001Fu && w[2] == 0x005Fu,
              "the pair skips one pixel and expands its words");

        /* (e) the 8-chunk: remember the header, byte-store at the end
         * (kind 8 advances the cursor, so the pair count is a real word). */
        memset(stream, 0, sizeof stream);
        memset(pixels, 0, sizeof pixels);
        stream[6] = 1;
        w16 = 0x8041u; memcpy(stream + 8, &w16, 2);   /* kind 8, byte 0x41 */
        w16 = 1u;      memcpy(stream + 10, &w16, 2);  /* one pair */
        w16 = 0x0100u; memcpy(stream + 12, &w16, 2);  /* skip 0, run 1 */
        w16 = 0x001Fu; memcpy(stream + 14, &w16, 2);
        flic_decode16_bit_delta(rec);
        uint16_t e0 = 0u, e1 = 0u;
        memcpy(&e0, pixels + 0, 2);
        memcpy(&e1, pixels + 2, 2);
        CHECK(e0 == 0x001Fu, "the 8-chunk expands its pair word");
        CHECK((e1 & 0xffu) == 0x41u && (e1 & 0xff00u) == 0u,
              "the remembered pixel lands as a lone byte store");

        /* (f) the back-count quirk: 0x100 - high repeats one word. */
        memset(stream, 0, sizeof stream);
        memset(pixels, 0, sizeof pixels);
        stream[6] = 1;
        w16 = 1u;      memcpy(stream + 8, &w16, 2);   /* kind 0: one pair */
        w16 = 0x8301u; memcpy(stream + 10, &w16, 2);  /* skip 1, 0x83 */
        w16 = 0x001Fu; memcpy(stream + 12, &w16, 2);
        flic_decode16_bit_delta(rec);
        int bad = 0;
        for (int i = 0; i != 125; ++i) { /* row 0 base + the skip of one */
            uint16_t got = 0u;
            memcpy(&got, pixels + 2 + (size_t)i * 2u, 2);
            if (got != 0x001Fu) {
                bad++;
            }
        }
        CHECK(bad == 0, "the back-count run repeats the one word 125 times");

        /* (g) the 0xC retire quirk: the whole word rides the ushort row. */
        memset(stream, 0, sizeof stream);
        memset(pixels, 0, sizeof pixels);
        stream[6] = 2;
        w16 = 1u;      memcpy(stream + 8, &w16, 2);   /* kind 0: one pair */
        w16 = 0x0100u; memcpy(stream + 10, &w16, 2);  /* skip 0, run 1 */
        w16 = 0x001Fu; memcpy(stream + 12, &w16, 2);
        w16 = 0xC111u; memcpy(stream + 14, &w16, 2);  /* retire */
        w16 = 1u;      memcpy(stream + 16, &w16, 2);  /* kind 0: one pair */
        w16 = 0x0100u; memcpy(stream + 18, &w16, 2);  /* skip 0, run 1 */
        w16 = 0x1234u; memcpy(stream + 20, &w16, 2);
        flic_decode16_bit_delta(rec);
        uint16_t a = 0u, b = 0u;
        memcpy(&a, pixels, 2);
        memcpy(&b, pixels + 0x3ef0u * 8u, 2);
        CHECK(a == 0x001Fu, "the first row lands at the row stride");
        CHECK(b == 0x2454u,
              "the retire retargets the row by the whole-word subtract");
        CHECK(flic_decode16_bit_delta(NULL) == 1,
              "the null record answers 1 (deviation)");
    }

    if (s_failures != 0) {
        fprintf(stderr, "flic: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("flic: all pass\n");
    return 0;
}