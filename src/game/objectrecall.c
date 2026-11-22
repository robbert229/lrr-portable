// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-030: ObjectRecall.c, the minifigures' recall table that carries
 * their ability flags, levels and custom names from one level to the next
 * (the .osf file), rebuilt in original shape (ADR 0005, finding 652). Text
 * 00459190-00459530, .bss 004ffe50-004ffe68.
 *
 * A live table collects the minifigures that leave a level
 * (ObjectRecall_StoreMiniFigure) and hands them back to the minifigures of
 * the next one (ObjectRecall_RecallMiniFigure). A shadow table keeps the
 * copy the save file and a level restart go back to.
 *
 * The port's models stay in object_state_file_port.cpp, unannotated: they
 * run on the game context natively. The C callers reach the functions here
 * by aliases in their imports headers (finding 648). The unit has no link
 * names: nothing calls it by a port name. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "game/entity_stats_link_names.h"
#include "gods98/files.h"
#include "objectrecall.h"
#include "legoobject_unit.h"
#include "entity_stats.h"

/* ObjectRecall.c's .bss (004ffe50-004ffe68), 0x18 bytes. */
typedef struct ObjectRecall_Globs {
    ObjectRecall* objRecall;          /* +0x00 the live table */
    unsigned int objRecallCount;      /* +0x04 */
    unsigned int objRecallSize;       /* +0x08 capacity, in entries */
    ObjectRecall* objRecallSave;      /* +0x0c the shadow table */
    unsigned int objRecallSaveCount;  /* +0x10 */
    int objRecallSaveLoaded;          /* +0x14 */
} ObjectRecall_Globs;

// GLOBAL: LEGORR 0x004ffe50
ObjectRecall_Globs objectRecallGlobs = { 0 };

/* The .osf file's little-endian words. */
static void object_recall_image_put_word(unsigned char* image, unsigned int offset,
                                         uint32_t value)
{
    image[offset] = (unsigned char)value;
    image[offset + 1] = (unsigned char)(value >> 8);
    image[offset + 2] = (unsigned char)(value >> 16);
    image[offset + 3] = (unsigned char)(value >> 24);
}

static uint32_t object_recall_image_word(const unsigned char* image, unsigned int offset)
{
    return (uint32_t)image[offset] | ((uint32_t)image[offset + 1] << 8) |
           ((uint32_t)image[offset + 2] << 16) | ((uint32_t)image[offset + 3] << 24);
}

/* ObjectRecall_StoreMiniFigure: append a minifigure with ability flags, a
 * level or a custom name to the live table, growing it by ten entries. */
// FUNCTION: LEGORR 0x00459190
void object_recall_store_minifigure(Lego_Object* liveObj)
{
    ObjectRecall* recallObj;

    if (liveObj->type == 2 &&
        (liveObj->abilityFlags || liveObj->objLevel || liveObj->customName)) {
        if (objectRecallGlobs.objRecallCount == objectRecallGlobs.objRecallSize) {
            objectRecallGlobs.objRecallSize += 10;
            objectRecallGlobs.objRecall = (ObjectRecall*)realloc(
                objectRecallGlobs.objRecall,
                objectRecallGlobs.objRecallSize * sizeof(ObjectRecall));
        }
        recallObj = &objectRecallGlobs.objRecall[objectRecallGlobs.objRecallCount++];
        recallObj->abilityFlags = liveObj->abilityFlags;
        recallObj->level = liveObj->objLevel;
        memset(recallObj->customName, 0, sizeof(recallObj->customName));
        if (liveObj->customName)
            strncpy(recallObj->customName, liveObj->customName, 11);
    }
}

/* ObjectRecall_RecallMiniFigure: a minifigure takes the last live entry's
 * ability flags, level and custom name. */
// FUNCTION: LEGORR 0x00459240
int object_recall_recall_minifigure(Lego_Object* liveObj)
{
    ObjectRecall* recallObj;

    if (liveObj->type == 2 && objectRecallGlobs.objRecallCount) {
        recallObj = &objectRecallGlobs.objRecall[--objectRecallGlobs.objRecallCount];
        liveObj->abilityFlags = recallObj->abilityFlags;
        stats_object_set_object_level((StatsObject*)liveObj, recallObj->level);
        legoobject_rename(liveObj, recallObj->customName);
        return 1;
    }
    return 0;
}

/* Free the live table. The original's name is not known. */
// FUNCTION: LEGORR 0x004592a0
void object_recall_reset(void)
{
    objectRecallGlobs.objRecallCount = 0;
    objectRecallGlobs.objRecallSize = 0;
    if (objectRecallGlobs.objRecall)
        free(objectRecallGlobs.objRecall);
    objectRecallGlobs.objRecall = NULL;
}

/* Copy the live table to the shadow table and mark the shadow loaded. */
// FUNCTION: LEGORR 0x004592e0
void object_recall_snapshot(void)
{
    if (objectRecallGlobs.objRecallCount && objectRecallGlobs.objRecall) {
        if (objectRecallGlobs.objRecallSave)
            free(objectRecallGlobs.objRecallSave);
        objectRecallGlobs.objRecallSave = (ObjectRecall*)malloc(
            objectRecallGlobs.objRecallCount * sizeof(ObjectRecall));
        memcpy(objectRecallGlobs.objRecallSave, objectRecallGlobs.objRecall,
               objectRecallGlobs.objRecallCount * sizeof(ObjectRecall));
        objectRecallGlobs.objRecallSaveCount = objectRecallGlobs.objRecallCount;
        objectRecallGlobs.objRecallSaveLoaded = 1;
    }
}

/* Reset the live table and rebuild it from the shadow table. */
// FUNCTION: LEGORR 0x00459360
int object_recall_restore(void)
{
    object_recall_reset();
    if (objectRecallGlobs.objRecallSave && objectRecallGlobs.objRecallSaveCount) {
        objectRecallGlobs.objRecall = (ObjectRecall*)malloc(
            objectRecallGlobs.objRecallSaveCount * sizeof(ObjectRecall));
        memcpy(objectRecallGlobs.objRecall, objectRecallGlobs.objRecallSave,
               objectRecallGlobs.objRecallSaveCount * sizeof(ObjectRecall));
        objectRecallGlobs.objRecallCount = objectRecallGlobs.objRecallSaveCount;
        objectRecallGlobs.objRecallSize = objectRecallGlobs.objRecallSaveCount;
        return 1;
    }
    return 0;
}

/* ObjectRecall_IsLoaded. */
// FUNCTION: LEGORR 0x004593d0
int object_recall_is_loaded(void)
{
    return objectRecallGlobs.objRecallSaveLoaded;
}

/* ObjectRecall_Save: write the live table to `fname` ('RRSO', the count,
 * then the entries), copy it to the shadow table and clear the shadow's
 * loaded flag. */
// FUNCTION: LEGORR 0x004593e0
int object_recall_save(const char* fname)
{
    File* file;
    unsigned int header[2];

    if ((file = file_open(fname, "wb")) != NULL) {
        unsigned char headerImage[8];
        unsigned char image[OBJECT_RECALL_IMAGE_SIZE];
        unsigned int i;

        header[0] = 0x534f5252; /* 'RRSO' */
        header[1] = objectRecallGlobs.objRecallCount;
        object_recall_image_put_word(headerImage, 0, header[0]);
        object_recall_image_put_word(headerImage, 4, header[1]);
        file_write(headerImage, 4, 2, file);
        for (i = 0; i < objectRecallGlobs.objRecallCount; i++) {
            object_recall_serialize(&objectRecallGlobs.objRecall[i], image);
            file_write(image, OBJECT_RECALL_IMAGE_SIZE, 1, file);
        }
        file_close(file);
        object_recall_snapshot();
        objectRecallGlobs.objRecallSaveLoaded = 0;
        return 1;
    }
    return 0;
}

/* ObjectRecall_LoadRROSFile: reset the live table, read the 'RRSO' header
 * and, when the file's length matches the count, the entries, then copy
 * them to the shadow table and clear its loaded flag. */
// FUNCTION: LEGORR 0x00459470
int object_recall_load_rrosfile(const char* fname)
{
    File* file;
    unsigned int header[2];
    unsigned char headerImage[8];
    unsigned char* image;
    unsigned int i;
    int length;
    int result = 0;

    object_recall_reset();
    if ((file = file_open(fname, "rb")) != NULL) {
        length = file_length(file) - 8;
        memset(headerImage, 0, sizeof(headerImage));
        file_read(headerImage, 4, 2, file);
        header[0] = object_recall_image_word(headerImage, 0);
        header[1] = object_recall_image_word(headerImage, 4);
        /* As isle-portable asserts its save version (LegoGameState::Load):
         * a debug build stops at a sidecar from another program; a release
         * build falls through to the original's check, which loads nothing. */
        assert(header[0] == 0x534f5252);
        /* The length check is made in 64 bits: the original's 32-bit
         * product wraps, so a huge count could pass it. */
        if (header[0] == 0x534f5252 && length >= 0 &&
            (uint64_t)length == (uint64_t)header[1] * OBJECT_RECALL_IMAGE_SIZE) {
            objectRecallGlobs.objRecallCount = header[1];
            objectRecallGlobs.objRecallSize = header[1];
            objectRecallGlobs.objRecall = (ObjectRecall*)malloc(
                header[1] * sizeof(ObjectRecall));
            image = (unsigned char*)malloc(header[1] * OBJECT_RECALL_IMAGE_SIZE);
            if (header[1] &&
                (objectRecallGlobs.objRecall == NULL || image == NULL ||
                 file_read(image, OBJECT_RECALL_IMAGE_SIZE, (int)header[1], file) !=
                     (int)header[1])) {
                /* Out of memory or a short read: load nothing, as from a
                 * mismatched file. */
                object_recall_reset();
            } else {
                for (i = 0; i < objectRecallGlobs.objRecallCount; i++)
                    object_recall_parse(image + i * OBJECT_RECALL_IMAGE_SIZE,
                                        &objectRecallGlobs.objRecall[i]);
                object_recall_snapshot();
                objectRecallGlobs.objRecallSaveLoaded = 0;
            }
            free(image);
        }
        file_close(file);
        result = 1;
    }
    return result;
}

/* ---- The .osf file's image of an ObjectRecall (objectrecall.h) -------- */

static_assert(sizeof(((ObjectRecall*)0)->abilityFlags) == 4 &&
                  sizeof(((ObjectRecall*)0)->level) == 4,
              "the ObjectRecall image's words are 32-bit");
static_assert(0x08 + sizeof(((ObjectRecall*)0)->customName) == OBJECT_RECALL_IMAGE_SIZE,
              "the ObjectRecall image ends with the custom name");

void object_recall_serialize(const ObjectRecall* objRecall, unsigned char* image)
{
    object_recall_image_put_word(image, 0x00, objRecall->abilityFlags);
    object_recall_image_put_word(image, 0x04, objRecall->level);
    memcpy(image + 0x08, objRecall->customName, sizeof(objRecall->customName));
}

void object_recall_parse(const unsigned char* image, ObjectRecall* objRecall)
{
    objRecall->abilityFlags = object_recall_image_word(image, 0x00);
    objRecall->level = object_recall_image_word(image, 0x04);
    memcpy(objRecall->customName, image + 0x08, sizeof(objRecall->customName));
}
