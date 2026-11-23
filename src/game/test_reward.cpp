// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_reward — the save image of the GAME-037 unit's level record
 * (game/reward.c): reward_level_serialize writes a RewardLevel as its Win32
 * layout, the 0x318c bytes FrontEnd.c keeps in a save file's mission table,
 * and reward_level_parse reads one back. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/frontend_record.h"
#include "game/reward.h"
#include "lrr_test.h"

static uint32_t image_word(const unsigned char* image, unsigned int offset)
{
    return (uint32_t)image[offset] | ((uint32_t)image[offset + 1] << 8) |
           ((uint32_t)image[offset + 2] << 16) | ((uint32_t)image[offset + 3] << 24);
}

static float image_float(const unsigned char* image, unsigned int offset)
{
    uint32_t word = image_word(image, offset);
    float value;
    memcpy(&value, &word, sizeof(value));
    return value;
}

/* Every non-pointer field gets a value derived from its item and position;
 * every pointer a non-NULL address the image must not keep. */
static void fill_item(RewardLevelItem* item, unsigned int seed, void* address)
{
    item->flags = 0x10000 + seed;
    snprintf(item->name, sizeof(item->name), "item%u", seed);
    item->importance = 1.5f + (float)seed;
    item->quota = 0x20 + seed;
    item->unk_02c = 0x2c + seed;
    item->total = 10.0f * (float)seed;
    item->actual = 2.25f * (float)seed;
    item->percent = 0.5f + (float)seed;
    item->count = 0x3c + seed;
    item->startCount = 0x40 + seed;
    item->lostCount = 0x44 + seed;
    item->damage = 0.75f * (float)seed;
    item->unk_04c = 0x4c + seed;
    item->percentInt = 0x50 + seed;
    snprintf(item->text, sizeof(item->text), "text %u", seed);
    snprintf(item->percentText, sizeof(item->percentText), "%u%%", seed);
    item->textX = 0x154 + seed;
    item->textY = 0x158 + seed;
    item->imageX = 0x15c + seed;
    item->imageY = 0x160 + seed;
    item->boxImageX = 0x164 + seed;
    item->boxImageY = 0x168 + seed;
    snprintf(item->flicName, sizeof(item->flicName), "flic%u.flh", seed);
    item->flicWidth = 0x1ec + seed;
    item->flicHeight = 0x1f0 + seed;
    item->flicX = 0x1f4 + seed;
    item->flicY = 0x1f8 + seed;
    item->flic = (struct Flic*)address;
    item->image = (Image*)address;
    item->font = (struct TextureSurface*)address;
    item->soundName = (char*)address;
    item->boxImage = (Image*)address;
}

static int items_equal(const RewardLevelItem* a, const RewardLevelItem* b)
{
    return a->flags == b->flags && !memcmp(a->name, b->name, sizeof(a->name)) &&
           a->importance == b->importance && a->quota == b->quota &&
           a->unk_02c == b->unk_02c && a->total == b->total && a->actual == b->actual &&
           a->percent == b->percent && a->count == b->count &&
           a->startCount == b->startCount && a->lostCount == b->lostCount &&
           a->damage == b->damage && a->unk_04c == b->unk_04c &&
           a->percentInt == b->percentInt && !memcmp(a->text, b->text, sizeof(a->text)) &&
           !memcmp(a->percentText, b->percentText, sizeof(a->percentText)) &&
           a->textX == b->textX && a->textY == b->textY && a->imageX == b->imageX &&
           a->imageY == b->imageY && a->boxImageX == b->boxImageX &&
           a->boxImageY == b->boxImageY &&
           !memcmp(a->flicName, b->flicName, sizeof(a->flicName)) &&
           a->flicWidth == b->flicWidth && a->flicHeight == b->flicHeight &&
           a->flicX == b->flicX && a->flicY == b->flicY;
}

int main(void)
{
    static RewardLevel level;
    static RewardLevel parsed;
    static unsigned char image[REWARD_LEVEL_IMAGE_SIZE];
    static unsigned char again[REWARD_LEVEL_IMAGE_SIZE];
    static char address;
    unsigned int i;

    CHECK(sizeof(Front_RewardLevel) == REWARD_LEVEL_IMAGE_SIZE,
          "the save table's record holds one image");

    memset(&level, 0, sizeof(level));
    level.enabled = 1;
    level.saveHasCapture = 1;
    level.saveCapture.surface = &address;
    level.saveCapture.width = 0x10c;
    level.saveCapture.height = 0x110;
    level.saveCapture.penZero = 0x114;
    level.saveCapture.pen255 = 0x118;
    level.saveCapture.penZeroRGB = 0x11c;
    level.saveCapture.flags = 0x120;
    level.saveCapture.nextFree = &level.saveCapture;
    for (i = 0; i < 100; i++) level.buildingCounts.names[i] = &address;
    level.buildingCounts.count = 7;
    level.buildingCounts.gridCount = 9;
    level.modifier = 0x1c0;
    level.unk_1c4 = 0x1c4;
    level.blockCount = 0x1c8;
    level.buildingCount = 0x1cc;
    level.buildingGridCount = 0x1d0;
    for (i = 0; i < 100; i++)
        snprintf(level.buildingNames[i], sizeof(level.buildingNames[i]), "Building%u", i);
    fill_item(&level.walls, 99, &address);
    level.itemCount = REWARD_ITEM_COUNT;
    level.items = level.itemsArray;
    for (i = 0; i < REWARD_ITEM_COUNT; i++) fill_item(&level.itemsArray[i], i, &address);

    memset(image, 0xcd, sizeof(image));
    reward_level_serialize(&level, image);

    /* The Win32 offsets. */
    CHECK(image_word(image, 0x0000) == 1, "enabled at +0x0000");
    CHECK(image_word(image, 0x000c) == 0x10c, "saveCapture.width at +0x000c");
    CHECK(image_word(image, 0x0020) == 0x120, "saveCapture.flags at +0x0020");
    CHECK(image_word(image, 0x01b8) == 7, "buildingCounts.count at +0x01b8");
    CHECK(image_word(image, 0x01c0) == 0x1c0, "modifier at +0x01c0");
    CHECK(image_word(image, 0x01d0) == 0x1d0, "buildingGridCount at +0x01d0");
    CHECK(!strcmp((const char*)image + 0x01d4 + 3 * 64, "Building3"),
          "buildingNames[3] at +0x0294");
    CHECK(image_word(image, 0x1ad4) == 0x10000 + 99, "walls.flags at +0x1ad4");
    CHECK(image_word(image, 0x1ce4) == REWARD_ITEM_COUNT, "itemCount at +0x1ce4");
    CHECK(image_word(image, 0x1cec + 4 * 0x210 + 0x03c) == 0x3c + 4,
          "itemsArray[4].count at +0x1cec + 4 * 0x210 + 0x3c");
    CHECK(!strcmp((const char*)image + 0x1cec + 2 * 0x210 + 0x16c, "flic2.flh"),
          "itemsArray[2].flicName at +0x16c");
    CHECK(image_float(image, offsetof(Front_RewardLevel, score)) ==
              level.itemsArray[REWARD_ITEM_SCORE].percent,
          "the save table's score is itemsArray[9].percent");

    /* The pointer slots hold no data. */
    CHECK(image_word(image, 0x0008) == 0, "saveCapture.surface is 0");
    CHECK(image_word(image, 0x0024) == 0, "saveCapture.nextFree is 0");
    for (i = 0; i < 100; i++)
        CHECK(image_word(image, 0x0028 + i * 4) == 0, "buildingCounts.names[] is 0");
    CHECK(image_word(image, 0x1ce8) == 0, "items is 0");
    for (i = 0; i < REWARD_ITEM_COUNT; i++) {
        unsigned int base = 0x1cec + i * 0x210;
        unsigned int slot;
        for (slot = 0x1fc; slot < 0x210; slot += 4)
            CHECK(image_word(image, base + slot) == 0, "an item's pointer slot is 0");
    }

    /* Parse it back, and write it again. */
    memset(&parsed, 0xab, sizeof(parsed));
    reward_level_parse(image, &parsed);
    CHECK(parsed.enabled == 1 && parsed.saveHasCapture == 1, "the flags round-trip");
    CHECK(parsed.saveCapture.width == 0x10c && parsed.saveCapture.height == 0x110 &&
              parsed.saveCapture.penZero == 0x114 && parsed.saveCapture.pen255 == 0x118 &&
              parsed.saveCapture.penZeroRGB == 0x11c && parsed.saveCapture.flags == 0x120,
          "saveCapture's words round-trip");
    CHECK(parsed.saveCapture.surface == NULL && parsed.saveCapture.nextFree == NULL,
          "saveCapture's pointers read back NULL");
    CHECK(parsed.buildingCounts.count == 7 && parsed.buildingCounts.gridCount == 9,
          "the building counts round-trip");
    CHECK(parsed.buildingCounts.names[0] == NULL, "the building names read back NULL");
    CHECK(parsed.modifier == 0x1c0 && parsed.unk_1c4 == 0x1c4 &&
              parsed.blockCount == 0x1c8 && parsed.buildingCount == 0x1cc &&
              parsed.buildingGridCount == 0x1d0,
          "the level counts round-trip");
    CHECK(!memcmp(parsed.buildingNames, level.buildingNames, sizeof(level.buildingNames)),
          "buildingNames round-trips");
    CHECK(items_equal(&parsed.walls, &level.walls), "walls round-trips");
    CHECK(parsed.walls.flic == NULL && parsed.walls.image == NULL &&
              parsed.walls.font == NULL && parsed.walls.soundName == NULL &&
              parsed.walls.boxImage == NULL,
          "an item's pointers read back NULL");
    CHECK(parsed.itemCount == REWARD_ITEM_COUNT && parsed.items == NULL,
          "itemCount round-trips; items reads back NULL");
    for (i = 0; i < REWARD_ITEM_COUNT; i++)
        CHECK(items_equal(&parsed.itemsArray[i], &level.itemsArray[i]),
              "itemsArray round-trips");

    memset(again, 0x5a, sizeof(again));
    reward_level_serialize(&parsed, again);
    CHECK(!memcmp(image, again, sizeof(image)), "serialize(parse(image)) == image");

    return lrr_test_result("reward");
}
