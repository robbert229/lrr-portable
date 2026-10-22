// SPDX-License-Identifier: LGPL-3.0-or-later
// Save/load + checkpoints (finding 08) — .sav/.osf round trip.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/game.hpp"
#include "game/level.hpp"
#include "game/savegame.hpp"
#include "gods98/texture.hpp"   /* texture_decode: `.dat` round-trip probe */

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static const char* FIX = "save_fixture";

/* Read a whole file into a malloc'd buffer (NULL on failure). */
static uint8_t* read_all(const char* path, size_t* out_sz)
{
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t* buf = (uint8_t*)malloc(n > 0 ? (size_t)n : 1);
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }
    if (fread(buf, 1, (size_t)n, fp) != (size_t)n) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *out_sz = (size_t)n;
    return buf;
}

int main(void)
{
    char rmcmd[64]; snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", FIX); system(rmcmd);
    mkdir(FIX, 0755);
    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir %s failed\n", FIX);
        return 1;
    }
    /* Saves/ directory must exist (the corpus driver creates it; tests mkdir). */
    mkdir("Saves", 0755);

    GameContext ctx;
    game_context_init(&ctx);

    /* Fake current level + a couple of populated task records. */
    LevelState* lvl = (LevelState*)calloc(1, sizeof(LevelState));
    lvl->ptr_name = (void*)"Level01";
    ctx.level = lvl;
    task_set_name(&ctx.tasks[0], "Crystals");
    task_set_flags(&ctx.tasks[0], 1);
    task_set_importance(&ctx.tasks[0], 60.0f);
    task_set_quota(&ctx.tasks[0], 5);
    task_set_value(&ctx.tasks[0], 3);
    task_set_name(&ctx.tasks[3], "Timer");
    task_set_flags(&ctx.tasks[3], 3);
    task_set_importance(&ctx.tasks[3], 20.0f);
    task_set_quota(&ctx.tasks[3], 900);

    /* Checkpoint blob build (CheckpointBlobBuild). */
    checkpoint_blob_build(&ctx);
    CHECK(ctx.checkpoint_blob != NULL, "blob allocated");
    {
        int32_t ten = 0;
        __builtin_memcpy(&ten, ctx.checkpoint_blob + 0x1CE4, 4);
        CHECK(ten == GAME_TASK_COUNT, "blob task count == 10");
        /* Task table copy: first task's name bytes land at blob+0x1CEC+0x04. */
        CHECK(memcmp(ctx.checkpoint_blob + 0x1CEC + 0x04, "Crystals",
                     strlen("Crystals")) == 0, "blob task name copy");
        CHECK(memcmp(ctx.checkpoint_blob + 0x1CEC + 3 * 0x210 + 0x04,
                     "Timer", strlen("Timer")) == 0, "blob 4th task copy");
        /* Blob tail must be exactly 0x318c i.e. task block ends at blob end. */
        CHECK(0x1CEC + GAME_TASK_COUNT * GAME_TASK_STRIDE == SAVE_BLOB_SIZE,
              "task block fills blob");
    }
    /* Corpus-shape pin (finding 08 §"Task table"): the checkpoint-relevant
     * record fields must land at the corpus offsets inside the copied blob
     * records, and the records must round-trip verbatim. */
    {
        const uint8_t* r0 = ctx.checkpoint_blob + 0x1CEC; /* slot 0 "Crystals" */
        uint32_t flags = 0;
        int32_t quota = 0, value = 0;
        float imp = 0.0f;
        uint32_t self = 0;
        __builtin_memcpy(&flags, r0 + 0x00, 4);
        __builtin_memcpy(&imp,   r0 + 0x24, 4);
        __builtin_memcpy(&quota, r0 + 0x28, 4);
        __builtin_memcpy(&value, r0 + 0x2C, 4);
        __builtin_memcpy(&self, ctx.checkpoint_blob + 0x1CE8, 4);
        CHECK(flags == 1u, "blob record flags @+0x00");
        CHECK(imp == 60.0f, "blob record importance @+0x24");
        CHECK(quota == 5, "blob record quota @+0x28");
        CHECK(value == 3, "blob record value pass-through @+0x2C");
        CHECK(self == 0x1CEC, "blob self-ref -> record array at +0x1CEC");
        CHECK(memcmp(ctx.checkpoint_blob + 0x1CEC, ctx.tasks,
                     GAME_TASK_COUNT * GAME_TASK_STRIDE) == 0,
              "blob records copied verbatim");
    }
    /* Tutorial fix: Level03 forces task slot 4 (Caverns) quota to 1, i.e.
     * the dword at blob + 0x1CEC + 0x868 (record_base + 4*0x210 + 0x28). */
    lvl->ptr_name = (void*)"Levels::Level03";
    checkpoint_blob_build(&ctx);
    {
        int32_t caverns_quota = 0;
        __builtin_memcpy(&caverns_quota, ctx.checkpoint_blob + 0x1CEC + 0x868, 4);
        CHECK(caverns_quota == 1, "Level03 tutorial quota fix");
    }
    lvl->ptr_name = (void*)"Level01";

    /* Save record 2 with the blob, mark dirty, write slot 3.
     * The slot context must exist first (the corpus builds it from the
     * level tree; here the read-with-create path bootstraps it). */
    save_read_slot(&ctx, 3, 0);
    save_set_current_record(&ctx, 2);
    save_write_record(&ctx, 2, ctx.checkpoint_blob);
    save_mark_record(&ctx, 2);
    CHECK(save_write_slot(&ctx, 3) != 0, "write slot 3");

    {
        size_t n = 0;
        uint8_t* raw = read_all("Saves/3.sav", &n);
        CHECK(raw != NULL, "Saves/3.sav exists");
        if (raw != NULL) {
            CHECK(n == SAVE_HEADER_SIZE + (size_t)SAVE_DEFAULT_RECORDS * SAVE_RECORD_SIZE,
                  ".sav file size = header + 5 records");
            /* Header count at +0x18. */
            int32_t cnt = 0;
            __builtin_memcpy(&cnt, raw + 0x18, 4);
            CHECK(cnt == SAVE_DEFAULT_RECORDS, "header record count");
            free(raw);
        }
    }
    /* .osf sidecar. */
    {
        size_t n = 0;
        uint8_t* raw = read_all("Saves/3.osf", &n);
        CHECK(raw != NULL, "Saves/3.osf exists");
        if (raw != NULL) {
            CHECK(n == 8, ".osf size (no completed entries)");
            /* Corpus magic constant 0x534F5252 — LE bytes R,R,O,S. */
            CHECK(raw[0] == 'R' && raw[1] == 'R' && raw[2] == 'O' && raw[3] == 'S',
                  ".osf magic bytes RROS");
            free(raw);
        }
    }

    /* Completed-objective entries. */
    osf_add(&ctx, 7, 8, "LegoManRescued");
    osf_add(&ctx, 9, 10, "Crystal");
    CHECK(ctx.osf_count == 2, "two osf entries");
    CHECK(osf_write_path(&ctx, 3) != 0, "write .osf");
    {
        size_t n = 0;
        uint8_t* raw = read_all("Saves/3.osf", &n);
        CHECK(raw != NULL, "Saves/3.osf rewritten");
        if (raw != NULL) {
            CHECK(n == 8 + 2 * 20, ".osf size with 2 entries");
            int32_t cnt = 0;
            __builtin_memcpy(&cnt, raw + 4, 4);
            CHECK(cnt == 2, "osf entry count");
            int32_t id0 = 0, ref0 = 0;
            __builtin_memcpy(&id0, raw + 8, 4);
            __builtin_memcpy(&ref0, raw + 12, 4);
            CHECK(id0 == 7 && ref0 == 8, "first osf entry id/ref");
            /* Name capped at 11 chars + one zero pad byte (corpus strncpy 0xb). */
            CHECK(memcmp(raw + 16, "LegoManResc\0", 12) == 0, "first name slot");
            free(raw);
        }
    }

    /* Load-side `.osf` restore (OsfRead @ 0x459470, entered from LevelLoad
     * @ 0x429890) — strict write -> reload round trip: the completed
     * objectives written to the sidecar come back byte-identical into the
     * live table and its shadow, with nothing left dirty or stale. */
    {
        GameContext rctx;
        game_context_init(&rctx);
        /* Fresh context — a player booting into the loaded slot. */
        osf_add(&rctx, 7, 8, "LegoManRescued");
        osf_add(&rctx, 9, 10, "Crystal");
        CHECK(save_write_slot(&rctx, 4) != 0, "rtrip: write slot 4 (+sidecar)");
        /* Reload: wipe the in-memory table (the corpus clears it before the
         * read), then restore strictly from the file. */
        rctx.osf_count = 0;
        rctx.osf_capacity = 0;
        rctx.osf_shadow_count = 0;
        rctx.osf_dirty = 0;
        rctx.osf_shadow_dirty = 0;
        int rr = osf_read_path(&rctx, 4);
        CHECK(rr == 1, "rtrip: sidecar file opened");
        CHECK(rctx.osf_count == 2 && rctx.osf_capacity == 2,
            "rtrip: live count/capacity restored");
        CHECK(rctx.osf[0].id == 7 && rctx.osf[0].ref == 8
            && memcmp(rctx.osf[0].name, "LegoManResc\0", 12) == 0,
            "rtrip: entry 0 identical (id/ref/name)");
        CHECK(rctx.osf[1].id == 9 && rctx.osf[1].ref == 10
            && memcmp(rctx.osf[1].name, "Crystal\0\0\0\0\0", 12) == 0,
            "rtrip: entry 1 identical (id/ref/name)");
        CHECK(rctx.osf_shadow_count == rctx.osf_count
            && memcmp(rctx.osf_shadow, rctx.osf,
                      (size_t)rctx.osf_count * sizeof(GameOsfEntry)) == 0,
            "rtrip: shadow mirrors live (nothing shadowed/stale)");
        CHECK(rctx.osf_dirty == 0 && rctx.osf_shadow_dirty == 0,
            "rtrip: both dirty flags clear after restore");
        free(rctx.records);
        rctx.records = NULL;
    }

    /* Corrupt / missing sidecar handling (the corpus's degrade-to-empty
     * failure mode): bad magic, a size that contradicts the count, a short
     * file, and an absent file all leave the live table empty. */
    {
        GameContext cctx;
        game_context_init(&cctx);
        /* bad magic: 'XX' + count 1 */
        uint8_t bad[8 + 20];
        bad[0] = 'X'; bad[1] = 'X';
        __builtin_memcpy(bad + 4, "\x01\x00\x00\x00", 4);
        memset(bad + 8, 0, 20);
        {
            FILE* fp = fopen("Saves/6.osf", "wb");
            fwrite(bad, 1, sizeof(bad), fp);
            fclose(fp);
        }
        CHECK(osf_read_path(&cctx, 6) == 1, "corrupt: bad-magic file opens (uVar3)");
        CHECK(cctx.osf_count == 0 && cctx.osf_shadow_count == 0,
            "corrupt: bad magic -> table stays empty");
        /* size mismatch: right count but wrong file size (count 1, 40 bytes) */
        {
            FILE* fp = fopen("Saves/6.osf", "wb");
            uint8_t hdr[8];
            uint8_t oversized_payload[40] = { 0 };
            __builtin_memcpy(hdr, "\x52\x52\x4f\x53", 4);  /* 0x534f5252 LE */
            __builtin_memcpy(hdr + 4, "\x01\x00\x00\x00", 4);
            fwrite(hdr, 1, 8, fp);
            fwrite(oversized_payload, 1, sizeof(oversized_payload), fp);
            fclose(fp);
        }
        cctx.osf_count = 0;
        CHECK(osf_read_path(&cctx, 6) == 1, "corrupt: size-mismatch file opens");
        CHECK(cctx.osf_count == 0, "corrupt: size mismatch -> table stays empty");
        /* short file (< 8-byte header) */
        {
            FILE* fp = fopen("Saves/6.osf", "wb");
            fwrite("RO", 1, 2, fp);
            fclose(fp);
        }
        CHECK(osf_read_path(&cctx, 6) == 1, "corrupt: short file opens");
        CHECK(cctx.osf_count == 0, "corrupt: short file -> table stays empty");
        /* absent sidecar -> fopen fails -> returns 0, table empty */
        int ar = osf_read_path(&cctx, 0);
        CHECK(ar == 0 && cctx.osf_count == 0,
            "corrupt: absent sidecar -> 0, table empty");
    }

    /* Stale-shadow hole across a slot switch (the corpus's failure mode):
     * OsfClear (0x4592a0) is live-only and the rollback 0x459360 copies
     * the shadow back with no dirty gate, so the corpus can RESURRECT a
     * previous level's shadow after a failed or absent read. The port is
     * safe only because osf_read clears its own shadow before opening —
     * load a valid sidecar, then a failed/absent one, and nothing of the
     * first may survive. This locks that pre-clear: drop it and these
     * checks fail. */
    {
        GameContext wctx;
        game_context_init(&wctx);
        osf_add(&wctx, 21, 22, "SlotOneObj");
        CHECK(save_write_slot(&wctx, 1) != 0, "stale-shadow: write slot 1 sidecar");
        /* corrupt one (bad magic) for the failed-read leg */
        {
            uint8_t bad[8 + 20];
            bad[0] = 'X'; bad[1] = 'X';
            __builtin_memcpy(bad + 4, "\x01\x00\x00\x00", 4);
            memset(bad + 8, 0, 20);
            FILE* fp = fopen("Saves/5.osf", "wb");
            fwrite(bad, 1, sizeof(bad), fp);
            fclose(fp);
        }

        GameContext lctx;
        game_context_init(&lctx);
        CHECK(osf_read_path(&lctx, 1) == 1 && lctx.osf_shadow_count == 1,
            "stale-shadow: slot 1 installs live + shadow");
        (void)osf_read_path(&lctx, 2); /* absent file -> 0 */
        CHECK(lctx.osf_count == 0 && lctx.osf_shadow_count == 0,
            "stale-shadow: absent read clears the old shadow");
        CHECK(osf_read_path(&lctx, 1) == 1 && lctx.osf_shadow_count == 1,
            "stale-shadow: slot 1 installs live + shadow (again)");
        CHECK(osf_read_path(&lctx, 5) == 1
            && lctx.osf_count == 0 && lctx.osf_shadow_count == 0,
            "stale-shadow: corrupt read clears the old shadow (no resurrection)");
        free(lctx.records);
        lctx.records = NULL;
        free(wctx.records);
        wctx.records = NULL;
    }

    /* Read the slot back and verify record 2 (dirty flag + blob). */
    ctx.active_slot = -1; /* force re-read */
    int rc = save_read_slot(&ctx, 3, 0);
    CHECK(rc != 0, "read slot 3 back");
    if (rc) {
        CHECK(ctx.record_count == SAVE_DEFAULT_RECORDS, "records count round trip");
        uint32_t* flags = save_record_flags(&ctx, 2);
        CHECK(flags != NULL, "record 2 flags");
        if (flags != NULL) {
            CHECK((*flags & 1u) != 0, "record 2 dirty flag persisted");
            CHECK((*flags & 2u) != 0, "record 2 empty-mark persisted");
        }
        /* Blob payload equality. */
        int32_t ten = 0;
        __builtin_memcpy(&ten, ctx.records + 2 * SAVE_RECORD_SIZE + 4 + 0x1CE4, 4);
        CHECK(ten == GAME_TASK_COUNT, "record payload task count");
        CHECK(memcmp(ctx.records + 2 * SAVE_RECORD_SIZE + 4 + 0x1CEC + 0x04,
                     "Crystals", strlen("Crystals")) == 0, "record payload task name");
    }

    /* CheckpointStep: current level achieved (result index 1) on slot -1
     * bootstraps slot 5 and commits. */
    ctx.save_slot = -1;
    ctx.current_record = 4;
    level_set_int(ctx.level, LEVEL_OFF_OBJ_RESULT, 1);
    ctx.records = NULL; /* force bootstrap path */
    checkpoint_step(&ctx);
    {
        size_t n = 0;
        uint8_t* raw = read_all("Saves/5.sav", &n);
        CHECK(raw != NULL, "Saves/5.sav written by checkpoint_step");
        free(raw);
        uint32_t* f5 = save_record_flags(&ctx, 4);
        CHECK(f5 != NULL && (*f5 & 1u) != 0, "checkpoint record marked on achievement");
    }

    free(ctx.records);
    checkpoint_blob_free(&ctx);

    /* Per-level records: a checkpoint at a campaign position past the
     * bootstrap count must grow the array and round-trip through the
     * slot file (the record<->level mapping indexes records by level). */
    game_context_init(&ctx);
    save_read_slot(&ctx, 2, 0);  /* absent file: bootstraps a count-5 slot */
    uint8_t pat[SAVE_BLOB_SIZE];
    for (size_t i = 0; i < SAVE_BLOB_SIZE; i++) {
        pat[i] = (uint8_t)(i * 31u + 7u);
    }
    CHECK(save_write_record(&ctx, 12, pat) == 1, "growth: write past bootstrap succeeds");
    CHECK(ctx.record_count >= 13, "growth: record_count grows to fit");
    uint32_t* f12 = save_record_flags(&ctx, 12);
    CHECK(f12 != NULL
              && memcmp(f12 + 1, pat, SAVE_BLOB_SIZE) == 0,
          "growth: blob lands in record 12");
    save_mark_record(&ctx, 12);
    save_commit(&ctx);
    save_read_slot(&ctx, 2, 0);                /* reload from disk */
    f12 = save_record_flags(&ctx, 12);
    CHECK(f12 != NULL && (*f12 & 1u) != 0
              && memcmp(f12 + 1, pat, SAVE_BLOB_SIZE) == 0,
          "growth: record 12 round-trips marked");
    free(ctx.records);
    ctx.records = NULL;
    ctx.record_count = 0;

    /* Slot menu state (save_slot_state) + checkpoint restore
     * (checkpoint_blob_apply) — finding 08 §"Menu layer". */
    game_context_init(&ctx);
    ctx.level = lvl;
    task_set_name(&ctx.tasks[1], "Ore");
    task_set_flags(&ctx.tasks[1], 2);
    task_set_importance(&ctx.tasks[1], 10.0f);
    task_set_quota(&ctx.tasks[1], 4);
    checkpoint_blob_build(&ctx);

    int filled = -1, completed = -1, resume = -2;
    CHECK(save_slot_state(&ctx, 3, &filled, &completed, &resume) == 1,
        "slot state: slot 3 file exists");
    CHECK(filled >= 1 && completed >= 1 && resume == 2,
        "slot state: record 2 marked+filled is the resume");
    CHECK(save_slot_state(&ctx, 0, NULL, NULL, NULL) == 0,
        "slot state: absent slot -> 0");

    /* Restore: clobber a task, apply the blob, expect it back (the blob
     * copied tasks[1]="Ore" at blob+0x1CEC + 1*0x210). */
    task_set_name(&ctx.tasks[0], "Clobbered");
    task_set_flags(&ctx.tasks[0], 0);
    checkpoint_blob_apply(&ctx, ctx.checkpoint_blob);
    CHECK(strcmp(task_name(&ctx.tasks[1]), "Ore") == 0
        && task_flags(&ctx.tasks[1]) == 2 && task_quota(&ctx.tasks[1]) == 4,
        "blob apply restores the task table");

    /* Saves/5.sav exists from the checkpoint_step above — a real slot. */
    resume = -2;
    CHECK(save_slot_state(&ctx, 5, NULL, NULL, &resume) == 1,
        "slot state: slot 5 exists");
    CHECK(resume >= 0, "slot state: slot 5 has a resume record");

    free(ctx.records);
    checkpoint_blob_free(&ctx);
    if (ctx.level != NULL) {
        free(ctx.level);
    }

    /* --- .dat slot thumbnail (FUN_0047e440 / FUN_0047c8f0 BMP dump) ------
     * The corpus's save-confirm writes Saves\<slot>.dat; FUN_00413a60 reads
     * it back with the BMP loader. Prove the writer's exact header layout,
     * bottom-up rows, and that the port's own decoder round-trips it. */
    {
        GameContext tctx;
        game_context_init(&tctx);
        uint32_t fb[4 * 3];
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 4; ++x) {
                fb[y * 4 + x] = (255u << 24) | ((uint32_t)(y + 1) << 16)
                              | ((uint32_t)(x * 2 + 5) << 8)
                              | (uint32_t)(x + y * 7);
            }
        }
        CHECK(save_write_thumbnail(&tctx, 3, fb, 4 * 4, 4, 3) == 1,
            "thumb: write Saves/3.dat");
        size_t sz = 0;
        uint8_t* b = read_all("Saves/3.dat", &sz);
        CHECK(b != NULL && sz == 54 + 12 * 3, "thumb: file is 90 bytes");
        if (b != NULL) {
            uint32_t u32;
            int le = 1;
            (void)le;
            CHECK(b[0] == 'B' && b[1] == 'M', "thumb: BM magic");
            memcpy(&u32, b + 2, 4);
            CHECK(u32 == 54 + 12 * 3, "thumb: file size field");
            memcpy(&u32, b + 10, 4);
            CHECK(u32 == 54, "thumb: data offset 54");
            memcpy(&u32, b + 14, 4);
            CHECK(u32 == 40, "thumb: DIB header 40");
            int32_t w32, h32;
            memcpy(&w32, b + 18, 4);
            memcpy(&h32, b + 22, 4);
            CHECK(w32 == 4 && h32 == 3, "thumb: w/h, positive (bottom-up)");
            CHECK(b[26] == 1 && b[28] == 24, "thumb: planes 1, 24 bpp");
            memcpy(&u32, b + 30, 4);
            CHECK(u32 == 0, "thumb: BI_RGB");
            memcpy(&u32, b + 38, 4);
            CHECK(u32 == 1, "thumb: XPel 1");
            /* File row 0 = surface row h-1 (bottom). */
            int rows_ok = 1, pad = 0;
            for (int f = 0; f < 3 && rows_ok; ++f) {
                for (int x = 0; x < 4; ++x) {
                    uint32_t p = fb[(2 - f) * 4 + x];
                    uint8_t* q = b + 54 + (size_t)f * 12 + (size_t)x * 3;
                    if (q[0] != (uint8_t)(p) || q[1] != (uint8_t)(p >> 8)
                        || q[2] != (uint8_t)(p >> 16)) {
                        rows_ok = 0;
                        break;
                    }
                }
            }
            CHECK(rows_ok, "thumb: bottom-up BGR rows match");
            (void)pad;
            /* Round trip through the port's BMP loader (the .dat reader). */
            TextureSurface* t = texture_decode(b, (uint32_t)sz);
            CHECK(t != NULL && t->width == 4 && t->height == 3,
                "thumb: decodes as 24bpp surface");
            if (t != NULL) {
                int rt = 1;
                for (int i = 0; i < 12; ++i) {
                    if ((t->pixels[i] & 0x00ffffffu)
                        != (fb[i] & 0x00ffffffu)) {
                        rt = 0;
                        break;
                    }
                }
                CHECK(rt, "thumb: round-trips through texture_decode");
                texture_free(t);
            }
            free(b);
        }
    }

    if (s_failures != 0) {
        fprintf(stderr, "savegame_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("savegame_test: OK\n");
    return 0;
}
