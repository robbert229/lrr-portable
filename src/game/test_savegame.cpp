// SPDX-License-Identifier: LGPL-3.0-or-later
// Save/load + checkpoints (finding 08) — .sav/.osf round trip.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/frontend.hpp" /* sav_tree_load's visited-band tail */
#include "game/game.hpp"
#include "game/level.hpp"
#include "game/savegame.hpp"
#include "crt/crt.hpp"   /* engine_feature_word_install: refresh gate */
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

/* Tree-walk observer: counts visits (the callback rides the corpus's
 * plain indirect call). */
static int s_counting_visit(void* node, void* user)
{
    (void)node;
    (*(int*)user)++;
    return 0;
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

    /* CheckpointBlobTaskProgressRecalculate (2026-10-25): the fold band
     * over the blob's task windows. A private context keeps the blob
     * pristine for the checks below. */
    {
        GameContext fc;
        memset(&fc, 0, sizeof fc);
        static LevelState flvl;
        memset(&flvl, 0, sizeof flvl);
        fc.level = &flvl;
        CHECK(checkpoint_blob_task_progress_recalculate(NULL) == 0,
              "the null context answers 0");
        CHECK(checkpoint_blob_task_progress_recalculate(&fc) == 0,
              "the missing blob answers 0");
        checkpoint_blob_build(&fc);
        CHECK(fc.checkpoint_blob != NULL, "the fold band built the blob");
        char* blob = (char*)fc.checkpoint_blob;
        float f = 0.0f;
        int got = 0;
        float gotf = 0.0f;
        /* Fold shape: 100 / den * num, a zero denominator answers 100,
         * and the clamp lands over the store. */
        memset(blob, 0, SAVE_BLOB_SIZE);
        f = 4.0f;
        memcpy(blob + 0x1d1c, &f, sizeof f);
        f = 1.0f;
        memcpy(blob + 0x1d20, &f, sizeof f);
        f = 0.0f;
        memcpy(blob + 0x1f2c, &f, sizeof f);
        f = 5.0f;
        memcpy(blob + 0x1f30, &f, sizeof f);
        f = 1.0f;
        memcpy(blob + 0x213c, &f, sizeof f);
        f = 250.0f;
        memcpy(blob + 0x2140, &f, sizeof f);
        got = checkpoint_blob_task_progress_recalculate(&fc);
        memcpy(&gotf, blob + 0x1d24, sizeof gotf);
        CHECK(got == 1 && gotf == 25.0f, "the fold divides through the den");
        memcpy(&gotf, blob + 0x1f34, sizeof gotf);
        CHECK(gotf == 100.0f, "a zero denominator answers 100");
        memcpy(&gotf, blob + 0x2144, sizeof gotf);
        CHECK(gotf == 100.0f, "the clamp lands over the fold");
        memcpy(&gotf, blob + 0x2774, sizeof gotf);
        CHECK(gotf == 100.0f, "the quiet folds still answer 100");
        /* Count-up shape: rounds of +100 on the sum, the folded percent
         * riding the carry, then the final fold plus the converted
         * bonus word (+0x1c0, the corpus's Reward/Modifier). */
        memset(blob, 0, SAVE_BLOB_SIZE);
        f = 5.0f;
        memcpy(blob + 0x1d1c, &f, sizeof f);
        f = 1.0f;
        memcpy(blob + 0x1d20, &f, sizeof f);
        f = 2.0f;
        memcpy(blob + 0x1d10, &f, sizeof f);
        /* The bonus word is a raw dword the corpus converts to float. */
        uint32_t bonus = 17u;
        memcpy(blob + 0x1c0, &bonus, sizeof bonus);
        checkpoint_blob_task_progress_recalculate(&fc);
        memcpy(&gotf, blob + 0x2fac, sizeof gotf);
        CHECK(gotf == 200.0f, "the count-up adds 100 a round");
        memcpy(&gotf, blob + 0x2fb0, sizeof gotf);
        CHECK(gotf == 40.0f, "the folded percent rides the carry");
        memcpy(&gotf, blob + 0x2fb4, sizeof gotf);
        CHECK(gotf == 37.0f, "the final fold adds the converted bonus");
        /* Corpus shape: the 0x2d90 loop keeps the accumulators in
         * registers and writes them back only at the end; a negative
         * carry clamps to 0.0 through the final fold. */
        memset(blob, 0, SAVE_BLOB_SIZE);
        f = 2.0f;
        memcpy(blob + 0x2d90, &f, sizeof f);
        f = 150.0f;
        memcpy(blob + 0x2da4, &f, sizeof f);
        bonus = 17u; /* raw dword again — the corpus converts, not copies */
        memcpy(blob + 0x1c0, &bonus, sizeof bonus);
        checkpoint_blob_task_progress_recalculate(&fc);
        memcpy(&gotf, blob + 0x2fac, sizeof gotf);
        CHECK(gotf == 200.0f, "the register loop lands the sum once");
        memcpy(&gotf, blob + 0x2fb0, sizeof gotf);
        CHECK(gotf == 0.0f, "the negative carry clamps to zero");
        memcpy(&gotf, blob + 0x2fb4, sizeof gotf);
        CHECK(gotf == 17.0f, "the quiet final fold leaves the bonus alone");
        checkpoint_blob_free(&fc);
        CHECK(fc.checkpoint_blob == NULL, "the fold block freed its blob");
    }

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

    /* Existing active fallback slot: selection was cleared without releasing
     * the records, as on the training return path. Exercise the real writer. */
    (void)save_read_slot(&ctx, 5, 0); /* create the initial slot if absent */
    CHECK(save_read_slot(&ctx, 5, 0) != 0, "fallback fixture reads existing slot");
    save_slot_set(&ctx, -1);
    ctx.current_record = 0;
    level_set_int(ctx.level, LEVEL_OFF_OBJ_RESULT, 0);
    ctx.checkpoint_blob[64] = 0x5a;
    remove("Saves/5.osf");
    checkpoint_step(&ctx);
    CHECK(ctx.save_slot == 5 && save_slot_get_target() == 5,
          "fallback selects the active slot for commit");
    {
        size_t n = 0;
        uint8_t* raw = read_all("Saves/5.sav", &n);
        CHECK(raw != NULL && n >= 0xb0 + SAVE_RECORD_SIZE && raw[0xb0 + 4 + 64] == 0x5a,
              "fallback commits checkpoint payload to existing save");
        free(raw);
        raw = read_all("Saves/5.osf", &n);
        CHECK(raw != NULL && n == 8 && memcmp(raw, "RROS\0\0\0\0", 8) == 0,
              "fallback writes empty objective sidecar");
        free(raw);
    }
    save_slot_set(&ctx, 3);
    checkpoint_step(&ctx);
    CHECK(ctx.save_slot == 3 && save_slot_get_target() == 3,
          "checkpoint preserves an explicitly selected slot");

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
        ctx.level = NULL; /* the tail builds without a level (the blob
                           * builder NULL-guards its name lane; leaving the
                           * dangling reference reads freed memory when
                           * MALLOC_PERTURB_ poisons the chunk) */
    }

    /* Manual labels 1 and 5 select raw files 0 and 4, leaving the
     * checkpoint fallback file 5 independent. */
    {
        GameContext manual;
        uint8_t pattern[SAVE_BLOB_SIZE];
        size_t n0 = 0, n4 = 0, n5 = 0, n5_after = 0;
        uint8_t *r0, *r4, *r5_before, *r5_after;
        game_context_init(&manual);
        r5_before = read_all("Saves/5.sav", &n5);
        memset(pattern, 0x31, sizeof(pattern));
        (void)save_read_slot(&manual, frontend_slot_raw_index(1), 0);
        save_slot_set(&manual, frontend_slot_raw_index(1));
        CHECK(save_write_record(&manual, 0, pattern), "manual first writes record");
        save_commit(&manual);
        CHECK(manual.save_slot == 0, "manual first selects raw slot 0");
        memset(pattern, 0x74, sizeof(pattern));
        (void)save_read_slot(&manual, frontend_slot_raw_index(5), 0);
        save_slot_set(&manual, frontend_slot_raw_index(5));
        CHECK(save_write_record(&manual, 0, pattern), "manual fifth writes record");
        save_commit(&manual);
        CHECK(manual.save_slot == 4, "manual fifth selects raw slot 4");
        r0 = read_all("Saves/0.sav", &n0);
        r4 = read_all("Saves/4.sav", &n4);
        r5_after = read_all("Saves/5.sav", &n5_after);
        CHECK(r0 != NULL && n0 >= 0xb0 + 5 && r0[0xb0 + 4] == 0x31,
              "manual first wrote raw 0 file");
        CHECK(r4 != NULL && n4 >= 0xb0 + 5 && r4[0xb0 + 4] == 0x74,
              "manual fifth wrote raw 4 file");
        CHECK(r5_before != NULL && r5_after != NULL && n5_after == n5 &&
              memcmp(r5_before, r5_after, n5) == 0,
              "manual selection leaves fallback raw 5 unchanged");
        free(r0); free(r4); free(r5_before); free(r5_after);
        free(manual.records);
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

    /* SavRecordPtr (2026-10-24). */
    {
        GameContext g;
        memset(&g, 0, sizeof(g));
        g.save_slot = 2;
        g.active_slot = 2;
        static uint8_t blob[3][0x3190];
        g.record_count = 3;
        g.records = &blob[0][0];
        CHECK(sav_record_ptr(&g, 1) == &blob[1][4], "the record address lands");
        CHECK(sav_record_ptr(&g, 3) == NULL, "the count guard answers null");
        g.save_slot = -1;
        CHECK(sav_record_ptr(&g, 0) == NULL, "a closed slot answers null");
    }

    /* SelectedLevelStartObjectUpdate (2026-10-24): bound-level read, guarded. */
    {
        GameContext ctx;
        game_context_init(&ctx);
        selected_level_start_object_update(NULL); /* guarded */
        const LevelState* keep = ctx.level;
        selected_level_start_object_update(&ctx); /* bound: reads through */
        ctx.level = NULL;
        selected_level_start_object_update(&ctx); /* unbound: the gate returns */
        ctx.level = const_cast<LevelState*>(keep);
    }

    /* SavRefreshSlots (2026-10-25): the six-slot walk. Feature bit clear
     * bootstraps the missing files and leaves existing ones; feature bit
     * set writes the live context into every slot on the way. */
    {
        static uint8_t blob[6][SAVE_BLOB_SIZE];
        for (int s = 0; s < 6; ++s) {
            memset(blob[s], 0x10 + s, SAVE_BLOB_SIZE);
        }
        GameContext ctx;
        game_context_init(&ctx);
        for (int s = 0; s < SAVE_SLOTS; ++s) {
            CHECK(save_read_slot(&ctx, s, 0) != 0 || s == 0,
                  "the slot bootstrap reads");
            CHECK(save_write_record(&ctx, 0, blob[s]) != 0,
                  "the distinct record lands");
            CHECK(save_write_slot(&ctx, s) != 0, "the slot writes");
        }
        CHECK(save_read_slot(&ctx, SAVE_SLOTS - 1, 0) != 0,
              "the live context reloads the last slot");
        for (int s = 0; s < SAVE_SLOTS; ++s) {
            char path[32];
            snprintf(path, sizeof(path), "Saves/%d.sav", s);
            size_t n = 0;
            uint8_t* raw = read_all(path, &n);
            CHECK(raw != NULL && n >= 0xB0u + 4u + SAVE_BLOB_SIZE,
                  "the six slot files exist");
            if (raw != NULL) {
                CHECK(raw[0xB0u + 4u] == (uint8_t)(0x10 + s),
                      "the slot keeps its distinct record");
                free(raw);
            }
        }

        /* Feature clear: reads only — the files ride. */
        GameContext reader;
        game_context_init(&reader);
        engine_feature_word_install(0u);
        sav_refresh_slots(&reader);
        {
            size_t n = 0;
            uint8_t* raw = read_all("Saves/3.sav", &n);
            CHECK(raw != NULL && raw[0xB0u + 4u] == 0x13u,
                  "the clear-feature refresh leaves the files");
            free(raw);
        }
        CHECK(reader.save_slot == SAVE_SLOTS - 1,
              "the refresh walk ends on the last slot");

        /* Feature set: the live records propagate to every slot. */
        engine_feature_word_install(0x400u);
        sav_refresh_slots(&ctx);
        engine_feature_word_install(0u);
        {
            size_t n = 0;
            uint8_t* raw = read_all("Saves/1.sav", &n);
            CHECK(raw != NULL && raw[0xB0u + 4u] == 0x15u,
                  "the set-feature refresh rewrites from the live context");
            free(raw);
        }
    }

    if (s_failures != 0) {
        fprintf(stderr, "savegame_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("savegame_test: OK\n");
    /* SavGetSlot (2026-10-24): the accessor reads the port's slot global. */
    CHECK(save_slot_get(&ctx) == ctx.save_slot, "the slot getter reads the word");
    save_slot_set(&ctx, 3);
    CHECK(save_slot_get(&ctx) == 3, "the slot setter round-trips");

    /* The save-slot band (2026-10-24): selection clear + ctx accessor. */
    save_slot_set(&ctx, 4);
    save_slot_selection_clear(&ctx);
    CHECK(save_slot_get(&ctx) == -1, "the clear drops to the closed sentinel");
    save_slot_set(&ctx, 5);
    CHECK(sav_slot_ctx(NULL, 0) == NULL
              && sav_slot_ctx(&ctx, -1) == NULL
              && sav_slot_ctx(&ctx, SAVE_SLOTS) == NULL,
          "the ctx accessor bounds clean");
    CHECK(sav_slot_ctx(&ctx, 2) == &ctx, "the ctx block answers valid slots");

    /* The save-side DAT accessor band (2026-10-24). */
    ctx.osf_dirty = 1;
    CHECK(osf_read_dirty_flag_get(&ctx) == 1
              && osf_read_dirty_flag_get(NULL) == 0,
          "the dirty flag reads through ctx");
    ctx.osf_dirty = 0;
    CHECK(checkpoint_blob_get(&ctx) == ctx.checkpoint_blob
              && checkpoint_blob_get(NULL) == NULL,
          "the blob getter threads the ctx");

    /* SaveStateEventContextSet (2026-10-24). */
    save_state_event_context_set(9u);
    CHECK(save_state_event_context_peek() == 9u, "the store lands");
    save_state_event_context_set(0u);

    /* CheckpointBlobFree (2026-10-24): frees and clears the port model. */
    checkpoint_blob_build(&ctx);
    CHECK(ctx.checkpoint_blob != NULL, "the blob builds");
    checkpoint_blob_free(&ctx);
    CHECK(ctx.checkpoint_blob == NULL, "the free clears the port pointer");
    checkpoint_blob_free(&ctx); /* the double free is a no-op */

    /* SavBuildCtx (2026-10-24): the slot-open probe. */
    save_slot_selection_clear(&ctx);
    CHECK(sav_build_ctx(&ctx) == NULL, "a closed slot builds nothing");
    save_slot_set(&ctx, 5);
    CHECK(sav_build_ctx(&ctx) == &ctx, "an open slot builds the block");
    save_slot_selection_clear(&ctx);

    /* The generic list window band (2026-10-25). */
    {
        static void* entries[4];
        static void* values_a[4];
        static void* values_b[4];
        SavListWindow win;
        memset(entries, 0, sizeof entries);
        memset(values_a, 0, sizeof values_a);
        memset(values_b, 0, sizeof values_b);
        win.entries = entries;
        win.values_a = values_a;
        win.values_b = values_b;

        const uint16_t key_a[] = { 'A', 'B', 0 };
        const uint16_t key_b[] = { 'C', 0 };
        win.count = 0;
        CHECK(list_get_node(&win, key_a) == NULL, "the empty window answers null");
        CHECK(list_find_name(&win, "menu") == -1, "the empty find answers -1");
        list_add_name(&win, key_a, (void*)5); /* no entries yet: silent */
        win.entries[0] = (void*)key_a;
        win.entries[1] = (void*)key_b;
        win.count = 2;
        list_add_name(&win, key_a, (void*)5);
        list_add_name(&win, key_b, (void*)7);
        CHECK(values_b[0] == (void*)5 && values_b[1] == (void*)7,
              "the add rides value array B");
        CHECK(list_has_item(&win, key_b) == (void*)7, "the has answers B");
        CHECK(list_has_item(&win, entries[2]) == NULL
                  && list_get_node(&win, entries[3]) == NULL,
              "the wide miss answers null");
        list_set_node(&win, key_a, (void*)9);
        CHECK(list_get_node(&win, key_a) == (void*)9, "the set rides array A");
        CHECK(values_b[0] == (void*)5, "the set does not touch array B");
        CHECK(list_has_item(&win, NULL) == NULL
                  && list_find_name(&win, NULL) == -1
                  && list_find_name(NULL, "x") == -1,
              "the window and key guards hold");
        list_add_name(NULL, key_a, (void*)1); /* guarded no-ops */
        list_set_node(NULL, key_a, (void*)1);
        {
            /* Pinned: the low byte terminates the wide key — the node
             * key { 0x1200 } compares equal to the probe { 0x0000 };
             * the terminator's high byte is never inspected. */
            static const uint16_t key_quirk[] = { 0x1200 };
            static const uint16_t key_probe[] = { 0x0000 };
            win.entries[2] = (void*)key_quirk;
            win.count = 3;
            list_add_name(&win, key_probe, (void*)11);
            CHECK(values_b[2] == (void*)11,
                  "the terminator quirk lands the store");
            win.entries[2] = NULL;
            win.count = 2;
        }
    }

    /* SavTreeWalk (2026-10-25). */
    {
        static SavTreeNode root;
        static SavTreeNode kid1;
        static SavTreeNode kid2;
        static void* kids[2];
        int visits = 0;
        kids[0] = &kid1;
        kids[1] = &kid2;
        root.children = kids;
        root.count = 2;
        root.visited = 0;
        kid1.children = NULL;
        kid1.count = 0;
        kid1.visited = 0;
        kid2.children = NULL;
        kid2.count = 0;
        kid2.visited = 0;
        CHECK(sav_tree_walk(&root, s_counting_visit, &visits) == 0
                  && visits == 3,
              "the walk visits each node once");
        visits = 0;
        CHECK(sav_tree_walk(&root, s_counting_visit, &visits) == 0
                  && visits == 0,
              "the revisited walk visits nothing");
        CHECK(sav_tree_walk(NULL, s_counting_visit, &visits) == 0,
              "the node guard holds");
        kid2.visited = 0;
        CHECK(sav_tree_walk(&kid2, NULL, &visits) == 0 && visits == 0,
              "the callback guard holds (the corpus strides it)");
    }

    /* SavTreeWalk2 / SavTreeLoad (2026-10-25). */
    {
        static SavTreeNode r;
        static SavTreeNode k1;
        static SavTreeNode k2;
        static void* kids[2];
        static uint32_t blocks[3 * 8]; /* three 0x20 block images */
        static char winbuf[2 * 0x3190];
        static uint32_t rec1[8];
        static uint32_t rec2[8];
        void* blockp = blocks;
        SavTree2State st;
        memset(blocks, 0, sizeof blocks);
        memset(winbuf, 0, sizeof winbuf);
        kids[0] = &k1;
        kids[1] = &k2;
        engine_feature_word_install(0u);
        /* flag lane, flag = 1: need rides 0 everywhere — the root
         * SETS through its carry 1, the kids CLEAR through their
         * blocks' zero +0x1c probes, and the cursor ends per node. */
        r.window = 1u;
        r.children = kids;
        r.count = 2;
        k1.window = 0u;
        k1.children = NULL;
        k1.count = 0;
        k2.window = 1u;
        k2.children = NULL;
        k2.count = 0;
        r.visited = 0u;
        k1.visited = 0u;
        k2.visited = 0u;
        blocks[0] = 0x80u; /* root: set lane */
        blocks[8] = 0xBFu;  /* kid: clear lane */
        blocks[16] = 0xCFu; /* kid: clear lane */
        st.window_base = NULL;
        st.window = 0u;
        st.blocks = &blockp;
        st.index = 0u;
        st.flag = 1u;
        sav_tree_walk2(&r, &st, 1, NULL);
        CHECK(r.visited != 0u && k1.visited != 0u && k2.visited != 0u
                  && st.index == 3u,
              "the walk2 enters each node once, cursor included");
        CHECK(blocks[0] == 0x84u && blocks[8] == 0xBBu
                  && blocks[16] == 0xCBu,
              "the carry sets and the probe-zero kids clear bit 2");

        /* Re-entry visits nothing (the flag lane). */
        sav_tree_walk2(&r, &st, 1, NULL);
        CHECK(st.index == 3u && blocks[0] == 0x84u,
              "the entered tree walks nothing");

        /* flag = 1 + extended gate: the flag zeroes on entry, need
         * rides 1, and every block SETS. */
        engine_feature_word_install(0x20000u);
        r.visited = 0u;
        k1.visited = 0u;
        k2.visited = 0u;
        blocks[0] = 0xA1u;
        blocks[8] = 0xB2u;
        blocks[16] = 0xC8u;
        st.index = 0u;
        st.flag = 1u;
        sav_tree_walk2(&r, &st, 1, NULL);
        CHECK(blocks[0] == 0xA5u && blocks[8] == 0xB6u
                  && blocks[16] == 0xCCu,
              "the gate zeroes the flag and need rides 1");

        /* stride lane (gate off): need bit 0 from
         * `node.window * 0x3190 + window_base`; r (window 1) reads the
         * zeroed slot and CLEARS its block (carry 0), k1 (window 0)
         * reads the raised bit and SETS through its need. */
        engine_feature_word_install(0u);
        ((uint32_t*)winbuf)[0] = 1u;
        r.children = kids;
        r.window = 1u;
        r.count = 2;
        r.visited = 0u;
        k1.visited = 0u;
        k2.window = 0u;
        k2.visited = 0u;
        blocks[0] = 0x15u;
        blocks[8] = 0x10u;
        blocks[16] = 0x10u;
        st.window_base = winbuf;
        st.flag = 1u; /* ignored off the flag lane */
        st.index = 0u;
        sav_tree_walk2(&r, &st, 0, NULL);
        CHECK(blocks[0] == 0x11u && blocks[8] == 0x14u
                  && blocks[16] == 0x14u && st.index == 3u,
              "the stride read rides node.window * 0x3190 + base");

        /* probe lane: need 0, carry 0, block +0x1c NON-zero -> SET. */
        r.children = NULL;
        r.count = 0;
        r.window = 0u;
        r.visited = 0u;
        blocks[0] = 0x10u;
        blocks[7] = 5u;
        st.window_base = NULL;
        st.flag = 1u;
        st.index = 0u;
        sav_tree_walk2(&r, &st, 0, NULL);
        CHECK(blocks[0] == 0x14u, "the nonzero +0x1c probe forces the set");
        blocks[7] = 0u;

        /* guards (deviations): the null node rides, a null block slot
         * still advances the cursor, and a null child slot returns. */
        const uint32_t before = st.index;
        sav_tree_walk2(NULL, &st, 1, NULL);
        CHECK(st.index == before, "the null node guards out");
        st.blocks = NULL;
        r.visited = 0u;
        sav_tree_walk2(&r, &st, 1, NULL);
        CHECK(st.index == before + 1u, "the null block slot still advances");

        /* SavTreeLoad: stack-init the state, walk, clear the bands. */
        memset(rec1, 0, sizeof rec1);
        memset(rec2, 0, sizeof rec2);
        rec1[4] = 0xdeadu; /* corpus record + 0x10 flag word */
        rec2[4] = 0xbeefu;
        sav_visited_entry_set(0, 0, rec1, 7u);
        sav_visited_entry_set(1, 0, rec2, 8u);
        frontend_flow_word_set(2, -1);
        int* wc = active_object_list_word();
        *wc = 1;
        frontend_flow_word_set(2, 0);
        wc = active_object_list_word();
        *wc = 1;
        r.children = kids;
        r.count = 2;
        r.visited = 0u;
        k1.visited = 0u;
        k2.visited = 0u;
        blocks[0] = 0xA1u;
        blocks[8] = 0xB2u;
        blocks[16] = 0xC8u;
        sav_tree_load(&r, NULL, &blockp, 0u, 1u, NULL);
        CHECK(r.visited != 0u && k1.visited != 0u && k2.visited != 0u
                  && blocks[0] == 0xA5u && (blocks[8] & 4u) == 0u,
              "the load walks with the stack-init state");
        CHECK(rec1[4] == 0u && ((uint32_t*)rec1)[4] == 0u,
              "the load clears the visited band");
        void* p = NULL;
        uint32_t w = 0u;
        sav_visited_entry_peek(1, 0, &p, &w);
        CHECK(p == rec2 && w == 0u, "the in-game band clears alongside");
    }

    /* SaveMenuRefresh (2026-10-26). */
    {
        static SavTreeNode m0;
        static SavTreeNode m1;
        static void* mkids[1];
        static uint32_t mblk[2 * 8]; /* two contiguous 0x20 block images */
        static char mwinbuf[2 * 0x3190];
        static void* mchain;
        static uint32_t recA[8];
        static uint32_t recB[8];
        GameContext mctx;

        engine_feature_word_install(0u);
        memset(mblk, 0, sizeof mblk);
        memset(mwinbuf, 0, sizeof mwinbuf);
        ((uint32_t*)mwinbuf)[0] = 1u; /* window 0 strides a raised word */
        mkids[0] = &m1;
        m0.window = 1u;  /* strides the zeroed high slot */
        m0.children = mkids;
        m0.count = 1;
        m1.window = 0u;
        m1.children = NULL;
        m1.count = 0;
        mchain = mblk;
        game_context_init(&mctx);

        /* hooks default clear: the refresh guards through, sets the slot,
         * and still clears the installed visited bands. */
        memset(recA, 0, sizeof recA);
        memset(recB, 0, sizeof recB);
        recA[4] = 0xdeadu;
        recB[4] = 0xbeefu;
        sav_visited_entry_set(0, 0, recA, 9u);
        sav_visited_entry_set(1, 0, recB, 10u);
        frontend_flow_word_set(2, -1);
        int* wc = active_object_list_word();
        *wc = 1;
        frontend_flow_word_set(2, 0);
        wc = active_object_list_word();
        *wc = 1;
        save_menu_refresh(&mctx, 3);
        CHECK(mctx.save_slot == 3, "the refresh parks the slot word");
        CHECK(recA[4] == 0u && recB[4] == 0u,
              "both load tails clear the visited bands");
        CHECK(sav_menu_window_peek(2) == NULL
                  && sav_tree_root_peek(5) == NULL,
              "the hook bounds guard the way out");

        /* slot-open stride lane: flag 0, stride window installed; the
         * root's zeroed high slot clears nothing through its carry 1
         * and the kid's raised window-0 slot sets through its need. */
        sav_tree_root_install(0, &m0);
        sav_menu_window_install(0, &mchain);
        sav_slot_stride_window_install(mwinbuf);
        m0.visited = 0u;
        m1.visited = 0u;
        mblk[0] = 0xA1u;
        mblk[8] = 0xB2u;
        save_menu_refresh(&mctx, 3);
        CHECK(mblk[0] == 0xA5u && mblk[8] == 0xB6u,
              "the stride need lanes ride through the block cursor");
        CHECK(m0.visited != 0u && m1.visited != 0u,
              "the in-band walk entered");

        /* unopened-slot flag lane: flag 1, no stride window — the root
         * SETS through its carry 1, the kid CLEARs through the probe. */
        m0.visited = 0u;
        m1.visited = 0u;
        mblk[0] = 0xA0u;
        mblk[8] = 0xB6u;
        save_menu_refresh(&mctx, -1);
        CHECK(mctx.save_slot == -1, "the unopened slot parks at -1");
        CHECK(mblk[0] == 0xA4u && mblk[8] == 0xB2u,
              "the flag lane sets through carry and clears through probe");

        /* cleanup: uninstall every hook the refresh reads. */
        sav_tree_root_install(1, NULL);
        sav_tree_root_install(0, NULL);
        sav_menu_window_install(0, NULL);
        sav_menu_window_install(1, NULL);
        sav_slot_stride_window_install(NULL);
    }

    /* Registry indexed-visit lookups (2026-10-26). */
    {
        static SavTreeNode r1;
        static SavTreeNode k1;
        static SavTreeNode g1;
        static SavTreeNode l1;
        static void* rkids[2];
        static void* kkids[1];
        static void* n;
        static uint32_t w;

        rkids[0] = &k1;
        rkids[1] = &l1;
        kkids[0] = &g1;
        r1.window = 0xA5u;
        r1.children = rkids;
        r1.count = 2;
        k1.window = 0x11u;
        k1.children = kkids;
        k1.count = 1;
        g1.window = 0x22u;
        g1.children = NULL;
        g1.count = 0;
        l1.window = 0x33u;
        l1.children = NULL;
        l1.count = 0;
#define RESET_VISITS() ((void)(r1.visited = k1.visited = 0u), \
                        (void)(g1.visited = l1.visited = 0u))

        /* visit order R, K, G, L: the thunk records the want-th node. */
        RESET_VISITS();
        n = registry_value_lookup(&r1, 0u);
        CHECK(n == &r1, "index 0 hits the root");
        RESET_VISITS();
        n = registry_value_lookup(&r1, 2u);
        CHECK(n == &g1, "the counter walks into the first subtree");
        RESET_VISITS();
        n = registry_value_lookup(&r1, 3u);
        CHECK(n == &l1, "the counter reaches the second child");
        RESET_VISITS();
        n = registry_value_lookup(&r1, 4u);
        CHECK(n == NULL, "past the last visit answers null");

        /* the word reader: node head dword, or -1. */
        RESET_VISITS();
        w = registry_value_index_lookup_or_not_found(&r1, 1u);
        CHECK(w == 0x11u, "the found node's head word reads back");
        RESET_VISITS();
        w = registry_value_index_lookup_or_not_found(&r1, 9u);
        CHECK(w == 0xffffffffu, "the miss answers -1");

        /* an already-entered tree visits nothing new. */
        RESET_VISITS();
        r1.visited = 1u;
        n = registry_value_lookup(&r1, 0u);
        CHECK(n == NULL, "the entered tree skips the flagged root entirely");
#undef RESET_VISITS
    }

    return 0;
}
