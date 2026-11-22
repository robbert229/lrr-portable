// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_savegame — FrontEnd.c's campaign progress (findings 08, 66, 67, 71,
 * 105, 106, 408): the six `Saves\<n>.sav` slots, the per-level records in
 * them, and how a record's completed bit unlocks the level menu.
 *
 * The test calls the unit's save functions (frontend_unit.c, 00417a90-
 * 00417fc0) on a fixture data directory, and checks the port's save
 * directory (files_native_save_path, as isle-portable's savepath) and the
 * .osf reader's assert. If Files.c's start-up cannot find the fixture's
 * Data directory it calls exit(0), which the atexit guard reports as a
 * failure. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

#include "game/frontend_unit.h"
#include "game/objectrecall.h"
#include "game/reward.h"
#include "gods98/files.h"
#include "gods98/files_native.h"
#include "gods98/main_globs.h"

/* The unit's functions under the names they link as natively
 * (frontend_unit_link_names.h). */
extern "C" {
extern Front_Globs frontGlobs;
int save_read_slot_target(uint32_t saveIndex, void* outData, int readOnly); /* 00417a90 */
int front_save_write_save_file(unsigned int saveIndex, Front_SaveData* saveData); /* 00417bd0 */
void sav_refresh_slots_target(void);                        /* 00417df0 */
void* sav_build_ctx_target(void);                           /* 00417e70 */
void save_slot_set_target(int saveNumber);                  /* 00417ea0 */
void front_save_set_level_completed(unsigned int levelIndex); /* 00417eb0 */
int save_write_record_target(int levelIndex, const uint8_t* rewardLevel); /* 00417f40 */
void front_save_write_current_save_file(void);              /* 00417fc0 */
void front_levels_update_available(Front_LevelLink* startLink, Front_SaveReward* rewards,
                                   Front_LevelSet* levelSet, Front_SelectData* select,
                                   int setLocked);          /* 004173e0 */
int float_to_long_x87(unsigned int index, Front_SaveData* saveData); /* 00417430 */
}

static const char* FIX = "savegame_fixture";
static const unsigned int HEADER_READ = 0xb0;  /* the reader's header */
static const unsigned int HEADER_FRESH = 0xb4; /* the fresh-slot writer's header */
static const unsigned int RECORD = 0x3190;

static int s_failures = 0;
static int s_finished = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static void exit_guard(void)
{
    if (!s_finished) {
        printf("FAIL: the test exited early (Files.c start-up found no Data directory?)\n");
        fflush(stdout);
        _exit(1);
    }
}

/* ---- fixture files ------------------------------------------------------ */

static void slot_path(char* out, size_t size, int slot, const char* ext)
{
    snprintf(out, size, "Data/Saves/%d.%s", slot, ext);
}

static unsigned char* read_file(int slot, const char* ext, size_t* size)
{
    char path[64];
    FILE* fp;
    long n;
    unsigned char* buf;

    slot_path(path, sizeof(path), slot, ext);
    *size = 0;
    if ((fp = fopen(path, "rb")) == NULL) return NULL;
    fseek(fp, 0, SEEK_END);
    n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = (unsigned char*)malloc(n > 0 ? (size_t)n : 1);
    if (buf != NULL && fread(buf, 1, (size_t)n, fp) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(fp);
    if (buf != NULL) *size = (size_t)n;
    return buf;
}

static int file_exists_on_disk(int slot, const char* ext)
{
    char path[64];
    struct stat st;
    slot_path(path, sizeof(path), slot, ext);
    return stat(path, &st) == 0;
}

static void remove_slot(int slot)
{
    char path[64];
    slot_path(path, sizeof(path), slot, "sav");
    remove(path);
    slot_path(path, sizeof(path), slot, "osf");
    remove(path);
}

static uint32_t word_at(const unsigned char* bytes, size_t offset)
{
    return (uint32_t)bytes[offset] | ((uint32_t)bytes[offset + 1] << 8) |
           ((uint32_t)bytes[offset + 2] << 16) | ((uint32_t)bytes[offset + 3] << 24);
}

static void put_word(unsigned char* bytes, size_t offset, uint32_t value)
{
    bytes[offset] = (unsigned char)value;
    bytes[offset + 1] = (unsigned char)(value >> 8);
    bytes[offset + 2] = (unsigned char)(value >> 16);
    bytes[offset + 3] = (unsigned char)(value >> 24);
}

/* A slot file as the reader expects one: an 0xb0-byte header and `count`
 * records, each record's flag word `flags` and its first payload byte
 * `marker`. */
static void write_slot_file(int slot, uint32_t count, uint32_t flags, unsigned char marker)
{
    char path[64];
    unsigned char header[0xb0];
    unsigned char* record = (unsigned char*)calloc(1, RECORD);
    FILE* fp;
    uint32_t i;

    memset(header, 0, sizeof(header));
    put_word(header, 0x18, count);
    put_word(record, 0, flags);
    record[4] = marker;
    slot_path(path, sizeof(path), slot, "sav");
    if ((fp = fopen(path, "wb")) == NULL) {
        CHECK(0, "fixture: cannot write a slot file (no Data/Saves?)");
        free(record);
        return;
    }
    fwrite(header, 1, sizeof(header), fp);
    for (i = 0; i < count; i++) fwrite(record, 1, RECORD, fp);
    fclose(fp);
    free(record);
}

static int records_all_zero(const unsigned char* bytes, size_t from, size_t to)
{
    size_t i;
    for (i = from; i < to; i++) {
        if (bytes[i] != 0) return 0;
    }
    return 1;
}

/* ---- the level graphs --------------------------------------------------- */

/* The missions: M0 -> { M1, M3 }, M1 -> { M2 }; depth-first order M0, M1,
 * M2, M3, which is also each level's set index. The tutorials: T0 -> T1.
 * front_levels_reset_visited clears `visited` through the level sets, so
 * each set lists its links. */
enum { MISSIONS = 4, TUTORIALS = 2 };
static Front_LevelLink s_m[MISSIONS];
static Front_LevelLink s_t[TUTORIALS];
static Front_LevelLink* s_m0Links[2];
static Front_LevelLink* s_m1Links[1];
static Front_LevelLink* s_t0Links[1];
static Front_LevelLink* s_mList[MISSIONS];
static Front_LevelLink* s_tList[TUTORIALS];
static int s_mVisited[MISSIONS];
static int s_tVisited[TUTORIALS];

static void build_level_graphs(void)
{
    int i;
    for (i = 0; i < MISSIONS; i++) {
        memset(&s_m[i], 0, sizeof(s_m[i]));
        s_m[i].setIndex = i;
        s_mList[i] = &s_m[i];
    }
    s_m0Links[0] = &s_m[1];
    s_m0Links[1] = &s_m[3];
    s_m[0].linkLevels = s_m0Links;
    s_m[0].linkCount = 2;
    s_m1Links[0] = &s_m[2];
    s_m[1].linkLevels = s_m1Links;
    s_m[1].linkCount = 1;
    for (i = 0; i < TUTORIALS; i++) {
        memset(&s_t[i], 0, sizeof(s_t[i]));
        s_t[i].setIndex = i;
        s_tList[i] = &s_t[i];
    }
    s_t0Links[0] = &s_t[1];
    s_t[0].linkLevels = s_t0Links;
    s_t[0].linkCount = 1;

    frontGlobs.missionLevels.count = MISSIONS;
    frontGlobs.missionLevels.levels = s_mList;
    frontGlobs.missionLevels.visitedList = s_mVisited;
    frontGlobs.tutorialLevels.count = TUTORIALS;
    frontGlobs.tutorialLevels.levels = s_tList;
    frontGlobs.tutorialLevels.visitedList = s_tVisited;
    frontGlobs.startMissionLink = &s_m[0];
    frontGlobs.startTutorialLink = &s_t[0];
    /* mainMenuValues[5] != -1 is Front_IsTutorialSelected. */
    frontGlobs.mainMenuValues[5] = -1;
    frontGlobs.saveNumber = -1;
}

/* ---- the tests ---------------------------------------------------------- */

/* Slot numbers are unsigned and below six (00417a90, 00417bd0). */
static void test_slot_range(void)
{
    Front_SaveData data;
    memset(&data, 0, sizeof(data));
    CHECK(save_read_slot_target(6, &data, 0) == 0, "range: slot 6 does not read");
    CHECK(!file_exists_on_disk(6, "sav"), "range: reading slot 6 creates no file");
    CHECK(front_save_write_save_file(6, NULL) == 0, "range: slot 6 does not write");
    CHECK(save_read_slot_target((uint32_t)-1, &data, 0) == 0, "range: slot -1 is out of range");
}

/* A missing slot: the read-only read misses; the ordinary read misses too
 * but first writes a fresh slot, whose header is 4 bytes longer than the one
 * the reader reads (finding 105). */
static void test_fresh_slot(void)
{
    Front_SaveData data;
    unsigned char* bytes;
    size_t size;
    unsigned int i;

    remove_slot(0);
    memset(&data, 0, sizeof(data));
    CHECK(save_read_slot_target(0, &data, 1) == 0, "fresh: the read-only read misses");
    CHECK(!file_exists_on_disk(0, "sav"), "fresh: the read-only read creates no file");

    CHECK(save_read_slot_target(0, &data, 0) == 0, "fresh: the first ordinary read misses");
    bytes = read_file(0, "sav", &size);
    CHECK(bytes != NULL, "fresh: the miss writes Saves\\0.sav");
    if (bytes != NULL) {
        CHECK(size == HEADER_FRESH + MISSIONS * RECORD,
              "fresh: an 0xb4-byte header and one record per mission");
        CHECK(word_at(bytes, 0x04) == 5, "fresh: music volume 5 at +0x04");
        CHECK(word_at(bytes, 0x08) == 7, "fresh: SFX volume 7 at +0x08");
        CHECK(word_at(bytes, 0x0c) == 1, "fresh: game speed 1 at +0x0c");
        CHECK(word_at(bytes, 0x10) == 5, "fresh: brightness 5 at +0x10");
        CHECK(word_at(bytes, 0x18) == MISSIONS, "fresh: the mission count at +0x18");
        CHECK(records_all_zero(bytes, HEADER_FRESH, size), "fresh: every record is empty");
        free(bytes);
    }
    CHECK(!file_exists_on_disk(0, "osf"), "fresh: a fresh slot writes no .osf");

    /* The reader reads 0xb0 bytes, then the records from there: the fresh
     * file's 4 extra header bytes become the first record's flag word (zero),
     * and the read still fits the file. */
    memset(&data, 0, sizeof(data));
    CHECK(save_read_slot_target(0, &data, 0) == 1, "fresh: the second read hits");
    CHECK(data.missionsCount == MISSIONS && data.sfxVolume == 7 && data.musicVolume == 5 &&
              data.gameSpeed == 1 && data.brightness == 5,
          "fresh: the header reads back");
    CHECK(data.missionsTable != NULL, "fresh: the read allocates the records");
    if (data.missionsTable != NULL) {
        int clear = 1;
        for (i = 0; i < data.missionsCount; i++) {
            if (data.missionsTable[i].flags != 0) clear = 0;
        }
        CHECK(clear, "fresh: no level is completed");
        free(data.missionsTable);
    }
}

/* The current slot writes and reads back whole: the header's option words
 * and every record, flag word and payload, verbatim (findings 66, 67). */
static void test_round_trip(void)
{
    Front_SaveData* slot = &frontGlobs.saveData[1];
    Front_SaveData back;
    unsigned char* bytes;
    size_t size;
    unsigned int i;
    int same = 1;

    remove_slot(1);
    memset(slot, 0, sizeof(*slot));
    slot->musicVolume = 3;
    slot->sfxVolume = 9;
    slot->gameSpeed = 2;
    slot->brightness = 6;
    slot->missionsCount = MISSIONS;
    slot->missionsTable = (Front_SaveReward*)calloc(MISSIONS, sizeof(Front_SaveReward));
    for (i = 0; i < MISSIONS; i++) {
        memset(&slot->missionsTable[i].reward, 0x40 + i, sizeof(Front_RewardLevel));
    }
    slot->missionsTable[1].flags = 1;
    slot->missionsTable[2].flags = 3;
    save_slot_set_target(1);
    front_save_write_current_save_file();

    bytes = read_file(1, "sav", &size);
    CHECK(bytes != NULL && size == HEADER_READ + MISSIONS * RECORD,
          "round trip: an 0xb0-byte header and the records");
    if (bytes != NULL) {
        CHECK(word_at(bytes, 0x04) == 3 && word_at(bytes, 0x08) == 9 &&
                  word_at(bytes, 0x0c) == 2 && word_at(bytes, 0x10) == 6,
              "round trip: the option words at +0x04..+0x10");
        CHECK(word_at(bytes, 0x34) == 0, "round trip: no pointer on disk at +0x34");
        CHECK(word_at(bytes, HEADER_READ + 1 * RECORD) == 1 &&
                  word_at(bytes, HEADER_READ + 2 * RECORD) == 3,
              "round trip: the flag words lead the records");
        free(bytes);
    }
    bytes = read_file(1, "osf", &size);
    CHECK(bytes != NULL && size >= 8 && word_at(bytes, 0) == 0x534f5252,
          "round trip: the write also writes Saves\\1.osf ('RRSO')");
    free(bytes);

    memset(&back, 0, sizeof(back));
    CHECK(save_read_slot_target(1, &back, 0) == 1, "round trip: the slot reads back");
    CHECK(back.musicVolume == 3 && back.sfxVolume == 9 && back.gameSpeed == 2 &&
              back.brightness == 6 && back.missionsCount == MISSIONS,
          "round trip: the header matches");
    if (back.missionsTable != NULL) {
        for (i = 0; i < MISSIONS; i++) {
            if (back.missionsTable[i].flags != slot->missionsTable[i].flags ||
                memcmp(&back.missionsTable[i].reward, &slot->missionsTable[i].reward,
                       sizeof(Front_RewardLevel)) != 0) {
                same = 0;
            }
        }
        free(back.missionsTable);
    } else {
        same = 0;
    }
    CHECK(same, "round trip: every record matches");
    save_slot_set_target(-1);
}

/* A record-count mismatch (finding 106): the read-only read takes the file
 * as it is; the ordinary read rewrites the slot fresh, clears the caller's
 * record and misses, so the saved progress is gone. */
static void test_read_count_mismatch(void)
{
    Front_SaveData data;
    unsigned char* bytes;
    size_t size;

    write_slot_file(2, 7, 1, 0x5a);
    memset(&data, 0, sizeof(data));
    CHECK(save_read_slot_target(2, &data, 1) == 1, "read mismatch: the read-only read hits");
    CHECK(data.missionsCount == 7 && data.missionsTable != NULL &&
              data.missionsTable[6].flags == 1 &&
              ((unsigned char*)&data.missionsTable[6].reward)[0] == 0x5a,
          "read mismatch: the read-only read keeps the file's 7 records");
    free(data.missionsTable);

    memset(&data, 0x11, sizeof(data));
    CHECK(save_read_slot_target(2, &data, 0) == 0, "read mismatch: the ordinary read misses");
    {
        Front_SaveData zero;
        memset(&zero, 0, sizeof(zero));
        CHECK(memcmp(&data, &zero, sizeof(data)) == 0,
              "read mismatch: the caller's record is cleared");
    }
    bytes = read_file(2, "sav", &size);
    CHECK(bytes != NULL && size == HEADER_FRESH + MISSIONS * RECORD &&
              word_at(bytes, 0x18) == MISSIONS && records_all_zero(bytes, HEADER_FRESH, size),
          "read mismatch: the slot file is rewritten fresh, the old records lost");
    free(bytes);
}

/* A write whose record count no longer matches the mission graph writes a
 * fresh slot instead and still reports success (00417bd0). */
static void test_write_count_mismatch(void)
{
    Front_SaveData data;
    unsigned char* bytes;
    size_t size;

    remove_slot(3);
    memset(&data, 0, sizeof(data));
    data.sfxVolume = 2;
    data.missionsCount = 3;
    data.missionsTable = (Front_SaveReward*)calloc(3, sizeof(Front_SaveReward));
    data.missionsTable[0].flags = 1;
    data.missionsTable[1].flags = 1;
    CHECK(front_save_write_save_file(3, &data) == 1, "write mismatch: the write reports success");
    bytes = read_file(3, "sav", &size);
    CHECK(bytes != NULL && size == HEADER_FRESH + MISSIONS * RECORD &&
              word_at(bytes, 0x08) == 7 && records_all_zero(bytes, HEADER_FRESH, size),
          "write mismatch: the file is a fresh slot, not the data passed");
    CHECK(!file_exists_on_disk(3, "osf"), "write mismatch: no .osf is written");
    free(bytes);
    free(data.missionsTable);
}

/* Front_Save_SetLevelCompleted (00417eb0): flag 1 is "completed", flag 2
 * marks a tutorial-index record (below 8), which the mission select shows
 * without a score. */
static void test_set_level_completed(void)
{
    Front_SaveData* slot = &frontGlobs.saveData[4];
    const unsigned int count = 10;

    memset(slot, 0, sizeof(*slot));
    slot->missionsCount = count;
    /* One extra record: index == count passes the original's `<=` check. */
    slot->missionsTable = (Front_SaveReward*)calloc(count + 1, sizeof(Front_SaveReward));

    save_slot_set_target(-1);
    CHECK(sav_build_ctx_target() == NULL, "completed: no current slot, no save data");
    front_save_set_level_completed(2);
    CHECK(slot->missionsTable[2].flags == 0, "completed: no current slot marks nothing");

    save_slot_set_target(4);
    frontGlobs.mainMenuValues[5] = -1; /* a mission, not a tutorial */
    front_save_set_level_completed(2);
    CHECK(slot->missionsTable[2].flags == 3, "completed: a mission below index 8 sets 1 and 2");
    front_save_set_level_completed(9);
    CHECK(slot->missionsTable[9].flags == 1, "completed: a mission at index 8+ sets 1 only");
    front_save_set_level_completed(count);
    CHECK(slot->missionsTable[count].flags == 1,
          "completed: index == count is accepted (the original's `<=`)");

    frontGlobs.mainMenuValues[5] = 0; /* a tutorial is selected */
    front_save_set_level_completed(8);
    CHECK(slot->missionsTable[8].flags == 0, "completed: a tutorial at index 8+ marks nothing");
    front_save_set_level_completed(5);
    CHECK(slot->missionsTable[5].flags == 3, "completed: a tutorial below 8 sets 1 and 2");

    frontGlobs.mainMenuValues[5] = -1;
    save_slot_set_target(-1);
    free(slot->missionsTable);
    memset(slot, 0, sizeof(*slot));
}

/* Front_Save_SetRewardLevel (00417f40) stores the level's reward record
 * as its save image; the record's score (itemsArray[9].percent, image
 * +0x2fb4) is what the slot menu shows (00417430). The table never grows. */
static void test_set_reward_level(void)
{
    Front_SaveData* slot = &frontGlobs.saveData[4];
    RewardLevel* level = (RewardLevel*)calloc(1, sizeof(RewardLevel));
    const unsigned char* image;
    float stored;

    memset(slot, 0, sizeof(*slot));
    slot->missionsCount = MISSIONS;
    slot->missionsTable = (Front_SaveReward*)calloc(MISSIONS, sizeof(Front_SaveReward));
    level->enabled = 1;
    level->modifier = 17;
    level->itemCount = REWARD_ITEM_COUNT;
    level->itemsArray[9].percent = 87.0f;

    save_slot_set_target(-1);
    CHECK(save_write_record_target(1, (const uint8_t*)level) == 0,
          "reward: no current slot stores nothing");

    save_slot_set_target(4);
    CHECK(save_write_record_target(2, (const uint8_t*)level) == 1, "reward: the record stores");
    image = (const unsigned char*)&slot->missionsTable[2].reward;
    CHECK(word_at(image, 0x0000) == 1, "reward: Enable at image +0x0000");
    CHECK(word_at(image, 0x01c0) == 17, "reward: Modifier at image +0x01c0");
    CHECK(word_at(image, 0x1ce4) == REWARD_ITEM_COUNT, "reward: the item count at +0x1ce4");
    memcpy(&stored, image + 0x2fb4, sizeof(stored));
    CHECK(stored == 87.0f && slot->missionsTable[2].reward.score == 87.0f,
          "reward: the score lands at image +0x2fb4");
    CHECK(float_to_long_x87(2, slot) == 87, "reward: the slot menu's score reads it");
    CHECK(slot->missionsTable[2].flags == 0, "reward: storing a record does not complete it");
    CHECK(save_write_record_target(MISSIONS, (const uint8_t*)level) == 0,
          "reward: an index past the table stores nothing (no growth)");

    save_slot_set_target(-1);
    free(slot->missionsTable);
    memset(slot, 0, sizeof(*slot));
    free(level);
}

/* Front_Levels_UpdateAvailable (004173e0): loading progress sets each
 * menu entry's selectable flag (4) from the records' completed bits. A
 * level is open when it, or the level linking to it, is completed; the
 * first level always is; the record's payload plays no part (finding 71). */
static void test_unlocks(void)
{
    Front_SaveReward rewards[MISSIONS];
    Front_SelectItem items[MISSIONS];
    Front_SelectData select;
    int i;

#define OPEN(i) ((items[(i)].flags & 4) != 0)
#define RUN(setLocked)                                                         \
    do {                                                                       \
        for (i = 0; i < MISSIONS; i++) items[i].flags = 0x10;                  \
        front_levels_update_available(&s_m[0], rewards, &frontGlobs.missionLevels, \
                                      &select, (setLocked));                   \
    } while (0)

    memset(rewards, 0, sizeof(rewards));
    memset(items, 0, sizeof(items));
    memset(&select, 0, sizeof(select));
    select.selItemList = items;
    select.selItemCount = MISSIONS;
    mainGlobs.flags = 0;
    mainGlobs.programmerLevel = 0;

    RUN(1);
    CHECK(OPEN(0) && !OPEN(1) && !OPEN(2) && !OPEN(3), "unlock: a new game opens only M0");
    CHECK((items[1].flags & 0x10) != 0, "unlock: the other flag bits stay");

    /* Payload with no completed bit: still locked. */
    memset(&rewards[0].reward, 0xff, sizeof(rewards[0].reward));
    rewards[0].flags = 2;
    RUN(1);
    CHECK(!OPEN(1) && !OPEN(3), "unlock: a record without flag 1 unlocks nothing");

    rewards[0].flags = 1;
    RUN(1);
    CHECK(OPEN(0) && OPEN(1) && !OPEN(2) && OPEN(3), "unlock: completing M0 opens M1 and M3");
    RUN(1);
    CHECK(OPEN(1) && !OPEN(2) && OPEN(3), "unlock: the walk resets its visits; a rerun agrees");

    rewards[0].flags = 0;
    rewards[1].flags = 1;
    RUN(1);
    CHECK(OPEN(0) && OPEN(1) && OPEN(2) && !OPEN(3),
          "unlock: a completed level opens itself and its links, whatever links to it");

    memset(rewards, 0, sizeof(rewards));
    items[3].frontEndOpen = 1;
    RUN(1);
    CHECK(OPEN(3) && !OPEN(1), "unlock: FrontEndOpen opens a level regardless");
    items[3].frontEndOpen = 0;

    /* No records (no slot): setLocked decides. */
    for (i = 0; i < MISSIONS; i++) items[i].flags = 0;
    front_levels_update_available(&s_m[0], NULL, &frontGlobs.missionLevels, &select, 0);
    CHECK(OPEN(0) && OPEN(1) && OPEN(2) && OPEN(3), "unlock: no records and unlocked opens all");
    for (i = 0; i < MISSIONS; i++) items[i].flags = 0;
    front_levels_update_available(&s_m[0], NULL, &frontGlobs.missionLevels, &select, 1);
    CHECK(OPEN(0) && !OPEN(1) && !OPEN(2) && !OPEN(3),
          "unlock: no records and locked opens only M0");

    /* mainGlobs flag 0x20000 (and programmer level 2+) opens everything. */
    mainGlobs.flags = 0x20000;
    RUN(1);
    CHECK(OPEN(0) && OPEN(1) && OPEN(2) && OPEN(3), "unlock: flag 0x20000 opens every level");
    mainGlobs.flags = 0;
    mainGlobs.programmerLevel = 2;
    RUN(1);
    CHECK(OPEN(0) && OPEN(1) && OPEN(2) && OPEN(3), "unlock: programmer level 2 opens every level");
    mainGlobs.programmerLevel = 0;
#undef RUN
#undef OPEN
}

/* Front_Save_ReadSaveFiles (00417df0): the six-slot scan at start-up. It
 * reads every valid slot into frontGlobs.saveData; mainGlobs flag 0x400
 * rewrites every slot fresh first. */
static void test_read_save_files(void)
{
    int i;
    unsigned char* bytes;
    size_t size;

    for (i = 0; i < 6; i++) remove_slot(i);
    write_slot_file(1, MISSIONS, 1, 0x21);
    write_slot_file(4, 9, 1, 0x24); /* a stale record count */

    mainGlobs.flags = 0;
    sav_refresh_slots_target();
    CHECK(frontGlobs.saveData[1].missionsCount == MISSIONS &&
              frontGlobs.saveData[1].missionsTable != NULL &&
              frontGlobs.saveData[1].missionsTable[3].flags == 1,
          "scan: a valid slot loads with its progress");
    CHECK(frontGlobs.saveData[4].missionsCount == MISSIONS &&
              frontGlobs.saveData[4].missionsTable != NULL &&
              frontGlobs.saveData[4].missionsTable[0].flags == 0,
          "scan: a stale slot is reset and reread empty");
    for (i = 0; i < 6; i++) {
        char what[64];
        snprintf(what, sizeof(what), "scan: slot %d exists after the scan", i);
        CHECK(file_exists_on_disk(i, "sav"), what);
    }

    mainGlobs.flags = 0x400;
    sav_refresh_slots_target();
    mainGlobs.flags = 0;
    bytes = read_file(1, "sav", &size);
    CHECK(bytes != NULL && size == HEADER_FRESH + MISSIONS * RECORD &&
              records_all_zero(bytes, HEADER_FRESH, size),
          "scan: flag 0x400 rewrites a valid slot fresh");
    free(bytes);
    CHECK(frontGlobs.saveData[1].missionsTable != NULL &&
              frontGlobs.saveData[1].missionsTable[3].flags == 0,
          "scan: flag 0x400 rereads the slot empty");
}

static int is_directory(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static long file_size_at(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 ? (long)st.st_size : -1;
}

/* file_make_dir builds "Data\\Saves"; the substitution maps it, so the
 * directory is Saves inside Data, not one named with a backslash. */
static void test_make_dir(void)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "rm -rf Data/Saves");
    if (system(cmd) != 0) return;
    files_native_set_save_path(NULL);
    file_make_dir("Saves");
    CHECK(is_directory("Data/Saves"), "mkdir: Data\\Saves is Data/Saves");
    CHECK(access("Data\\Saves", F_OK) != 0, "mkdir: no directory named with a backslash");
}

/* The save directory (files_native_save_path, isle-portable's savepath):
 * every name under the original's "Saves" opens there instead; other data
 * names stay in the data directory. */
static void test_save_directory(void)
{
    char cwd[1024];
    std::string root, saves;
    Front_SaveData* slot = &frontGlobs.saveData[1];
    File* file;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        CHECK(0, "savedir: getcwd");
        return;
    }
    root = std::string(cwd) + "/profile";
    saves = root + "/saves";
    files_native_set_save_path((saves + "/").c_str()); /* a trailing '/' is trimmed */
    CHECK(files_native_save_path() != NULL && saves == files_native_save_path(),
          "savedir: the configured path, without its trailing separator");

    remove_slot(0);
    {
        Front_SaveData fresh;
        memset(&fresh, 0, sizeof(fresh));
        CHECK(save_read_slot_target(0, &fresh, 0) == 0, "savedir: a missing slot misses");
    }
    CHECK(is_directory(saves.c_str()), "savedir: the fresh slot makes the directory, parents too");
    CHECK(file_size_at(saves + "/0.sav") == (long)(HEADER_FRESH + MISSIONS * RECORD),
          "savedir: the fresh slot is written there");
    CHECK(!file_exists_on_disk(0, "sav"), "savedir: nothing is written in Data/Saves");
    CHECK(file_exists("Saves\\0.sav"), "savedir: File_Exists looks there");

    memset(slot, 0, sizeof(*slot));
    slot->missionsCount = MISSIONS;
    slot->missionsTable = (Front_SaveReward*)calloc(MISSIONS, sizeof(Front_SaveReward));
    slot->missionsTable[2].flags = 1;
    save_slot_set_target(1);
    front_save_write_current_save_file();
    save_slot_set_target(-1);
    CHECK(file_size_at(saves + "/1.sav") == (long)(HEADER_READ + MISSIONS * RECORD),
          "savedir: the current slot is written there");
    CHECK(file_size_at(saves + "/1.osf") >= 8, "savedir: its .osf too");
    {
        Front_SaveData back;
        memset(&back, 0, sizeof(back));
        CHECK(save_read_slot_target(1, &back, 0) == 1 && back.missionsTable != NULL &&
                  back.missionsTable[2].flags == 1,
              "savedir: the slot reads back from there");
        free(back.missionsTable);
    }
    free(slot->missionsTable);
    memset(slot, 0, sizeof(*slot));

    /* Menu::SaveImage::Path's thumbnails ("Saves\\%d.dat") go there too. */
    if ((file = file_open("Saves\\2.dat", "wb")) != NULL) {
        file_write("BM", 1, 2, file);
        file_close(file);
    }
    CHECK(file_size_at(saves + "/2.dat") == 2, "savedir: a thumbnail opens there");
    file = file_open("SAVES\\2.dat", "rb");
    CHECK(file != NULL, "savedir: \"Saves\" matches without case");
    if (file != NULL) file_close(file);

    /* Other names stay in the data directory. */
    if ((file = file_open("Savesx.txt", "wb")) != NULL) file_close(file);
    CHECK(access("Data/Savesx.txt", F_OK) == 0, "savedir: a name that only starts with Saves stays");

    files_native_set_save_path("");
    CHECK(files_native_save_path() == NULL, "savedir: an empty path restores Data\\Saves");
}

/* ObjectRecall_LoadRROSFile (00459470) asserts the sidecar's 'RRSO' magic,
 * as isle-portable asserts its save version; a release build loads nothing
 * from it instead. Each load runs in a child so the abort is observable. */
static int load_osf_in_child(const unsigned char* bytes, size_t size)
{
    FILE* fp = fopen("Data/Saves/assert.osf", "wb");
    pid_t pid;
    int status = 0;

    if (fp == NULL) {
        CHECK(0, "fixture: cannot write the .osf (no Data/Saves?)");
        return -1;
    }
    fwrite(bytes, 1, size, fp);
    fclose(fp);
    fflush(stdout);
    if ((pid = fork()) == 0) {
        object_recall_load_rrosfile("Saves\\assert.osf");
        _exit(0);
    }
    waitpid(pid, &status, 0);
    return status;
}

static void test_osf_assert(void)
{
    unsigned char good[8] = { 'R', 'R', 'O', 'S', 0, 0, 0, 0 };
    unsigned char bad[8] = { 'X', 'X', 'X', 'X', 0, 0, 0, 0 };
    int status;

    files_native_set_save_path(NULL);
    mkdir("Data/Saves", 0755);
    status = load_osf_in_child(good, sizeof(good));
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0, "osf: a valid sidecar loads");
    status = load_osf_in_child(bad, sizeof(bad));
#ifndef NDEBUG
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
          "osf: a debug build asserts on a wrong magic");
#else
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "osf: a release build loads nothing from a wrong magic");
#endif
    remove("Data/Saves/assert.osf");
}

/* ---- the .osf file's records (ObjectRecall.c) ---------------------------- */

static void write_disk_file(const char* path, const unsigned char* bytes, size_t size)
{
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        CHECK(0, "fixture: cannot write an .osf (no Data/Saves?)");
        return;
    }
    fwrite(bytes, 1, size, fp);
    fclose(fp);
}

static unsigned char* read_disk_file(const char* path, size_t* size)
{
    FILE* fp;
    long n;
    unsigned char* buf;

    *size = 0;
    if ((fp = fopen(path, "rb")) == NULL) return NULL;
    fseek(fp, 0, SEEK_END);
    n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = (unsigned char*)malloc(n > 0 ? (size_t)n : 1);
    if (buf != NULL && fread(buf, 1, (size_t)n, fp) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(fp);
    if (buf != NULL) *size = (size_t)n;
    return buf;
}

/* A hand-built .osf, as the original writes it: the magic 0x534f5252
 * ('RRSO', the bytes "RROS" on disk), a count of two,
 * then two 0x14-byte entries (abilityFlags, level, customName[12]),
 * little-endian. */
static const unsigned char OSF_TWO[8 + 2 * 0x14] = {
    'R', 'R', 'O', 'S', 0x02, 0x00, 0x00, 0x00,
    /* entry 0: flags 0x00000123, level 3, "Axle" */
    0x23, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    'A', 'x', 'l', 'e', 0, 0, 0, 0, 0, 0, 0, 0,
    /* entry 1: flags 0x80402010, level 0x01020304, an 11-character name */
    0x10, 0x20, 0x40, 0x80, 0x04, 0x03, 0x02, 0x01,
    'S', 'p', 'a', 'r', 'k', 'y', 'B', 'o', 'l', 't', 's', 0,
};

/* object_recall_serialize writes an entry at its Win32 offsets, and
 * object_recall_parse reads it back. */
static void test_osf_record_image(void)
{
    ObjectRecall entry, back;
    unsigned char image[OBJECT_RECALL_IMAGE_SIZE];

    CHECK(OBJECT_RECALL_IMAGE_SIZE == 0x14, "osf image: an entry is 0x14 bytes");
    object_recall_parse(OSF_TWO + 8 + 0x14, &entry);
    CHECK(entry.abilityFlags == 0x80402010u, "osf image: abilityFlags at +0x00");
    CHECK(entry.level == 0x01020304u, "osf image: level at +0x04");
    CHECK(memcmp(entry.customName, "SparkyBolts", 12) == 0, "osf image: customName at +0x08");
    memset(image, 0xcc, sizeof(image));
    object_recall_serialize(&entry, image);
    CHECK(memcmp(image, OSF_TWO + 8 + 0x14, sizeof(image)) == 0,
          "osf image: serialize writes the parsed entry's bytes back");
    memset(&back, 0xcc, sizeof(back));
    object_recall_parse(image, &back);
    CHECK(back.abilityFlags == entry.abilityFlags && back.level == entry.level &&
              memcmp(back.customName, entry.customName, sizeof(back.customName)) == 0,
          "osf image: parse reads the serialized entry back");
}

/* ObjectRecall_LoadRROSFile (00459470) then ObjectRecall_Save (004593e0)
 * write the same bytes they read. */
static void test_osf_round_trip(void)
{
    unsigned char* bytes;
    size_t size;

    files_native_set_save_path(NULL);
    mkdir("Data/Saves", 0755);
    write_disk_file("Data/Saves/in.osf", OSF_TWO, sizeof(OSF_TWO));
    CHECK(object_recall_load_rrosfile("Saves\\in.osf") == 1, "osf round trip: the load succeeds");
    CHECK(object_recall_save("Saves\\out.osf") == 1, "osf round trip: the save succeeds");
    bytes = read_disk_file("Data/Saves/out.osf", &size);
    CHECK(bytes != NULL && size == sizeof(OSF_TWO) &&
              memcmp(bytes, OSF_TWO, sizeof(OSF_TWO)) == 0,
          "osf round trip: the saved file is byte-identical to the loaded one");
    free(bytes);

    /* A count that does not match the file's length loads nothing, and the
     * next save writes an empty table. */
    write_disk_file("Data/Saves/in.osf", OSF_TWO, sizeof(OSF_TWO) - 0x14);
    CHECK(object_recall_load_rrosfile("Saves\\in.osf") == 1,
          "osf mismatch: the load still opens the file");
    CHECK(object_recall_save("Saves\\out.osf") == 1, "osf mismatch: the save succeeds");
    bytes = read_disk_file("Data/Saves/out.osf", &size);
    {
        static const unsigned char empty[8] = { 'R', 'R', 'O', 'S', 0, 0, 0, 0 };
        CHECK(bytes != NULL && size == sizeof(empty) && memcmp(bytes, empty, sizeof(empty)) == 0,
              "osf mismatch: a mismatched file loads no entries");
    }
    free(bytes);
    object_recall_reset();
    remove("Data/Saves/in.osf");
    remove("Data/Saves/out.osf");
}

int main(void)
{
    char cmd[128];

    atexit(exit_guard);
    snprintf(cmd, sizeof(cmd), "rm -rf %s", FIX);
    if (system(cmd) != 0) return 1;
    mkdir(FIX, 0755);
    if (chdir(FIX) != 0) {
        printf("FAIL: chdir %s\n", FIX);
        s_finished = 1;
        return 1;
    }
    mkdir("Data", 0755);
    file_initialise("savegame_test", 0, "");
    /* The saves stay in Data\\Saves unless a test configures a directory. */
    files_native_set_save_path(NULL);
    test_make_dir();
    build_level_graphs();

    test_slot_range();
    test_fresh_slot();
    test_round_trip();
    test_read_count_mismatch();
    test_write_count_mismatch();
    test_set_level_completed();
    test_set_reward_level();
    test_unlocks();
    test_read_save_files();
    test_save_directory();
    test_osf_assert();
    test_osf_record_image();
    test_osf_round_trip();

    s_finished = 1;
    if (s_failures != 0) {
        printf("savegame_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("savegame_test: OK\n");
    return 0;
}
