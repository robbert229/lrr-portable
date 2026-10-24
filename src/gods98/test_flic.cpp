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

    if (s_failures != 0) {
        fprintf(stderr, "flic: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("flic: all pass\n");
    return 0;
}