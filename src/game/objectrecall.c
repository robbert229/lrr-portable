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
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "game/entity_stats_link_names.h"
#include "gods98/files.h"
#include "objectrecall.h"
#include "legoobject_unit.h"
#include "entity_stats.h"

/* ObjectRecall_Globs' entry: one minifigure, 0x14 bytes. */
typedef struct ObjectRecall {
    unsigned int abilityFlags;  /* +0x00 LegoObject +0x3f0 */
    unsigned int level;         /* +0x04 LegoObject +0x330 */
    char customName[12];        /* +0x08 */
} ObjectRecall;

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
ObjectRecall_Globs objectRecallGlobs;

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
        header[0] = 0x534f5252; /* 'RRSO' */
        header[1] = objectRecallGlobs.objRecallCount;
        file_write(header, 4, 2, file);
        file_write(objectRecallGlobs.objRecall, sizeof(ObjectRecall),
                   objectRecallGlobs.objRecallCount, file);
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
    int length;
    int result = 0;

    object_recall_reset();
    if ((file = file_open(fname, "rb")) != NULL) {
        length = file_length(file) - 8;
        file_read(header, 4, 2, file);
        if (header[0] == 0x534f5252 && length == (int)(header[1] * sizeof(ObjectRecall))) {
            objectRecallGlobs.objRecallCount = header[1];
            objectRecallGlobs.objRecallSize = header[1];
            objectRecallGlobs.objRecall = (ObjectRecall*)malloc(
                header[1] * sizeof(ObjectRecall));
            file_read(objectRecallGlobs.objRecall, sizeof(ObjectRecall),
                      objectRecallGlobs.objRecallCount, file);
            object_recall_snapshot();
            objectRecallGlobs.objRecallSaveLoaded = 0;
        }
        file_close(file);
        result = 1;
    }
    return result;
}
