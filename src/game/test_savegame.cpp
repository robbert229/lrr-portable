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
    /* Tutorial fix: Level03 pushes blob+0x868 = 1. */
    lvl->ptr_name = (void*)"Levels\\Level03";
    checkpoint_blob_build(&ctx);
    CHECK(ctx.checkpoint_blob[0x868] == 1, "Level03 tutorial fix");
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

    if (s_failures != 0) {
        fprintf(stderr, "savegame_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("savegame_test: OK\n");
    return 0;
}
