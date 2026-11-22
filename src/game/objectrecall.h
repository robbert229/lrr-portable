// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-030 ObjectRecall.c (original translation unit, finding 505; rebuilt
 * in finding 652).
 *
 * C-compatible header (ADR 0005). objectrecall.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). The C callers declare the
 * functions they call in their imports headers (finding 648);
 * src/game/test_c_boundaries.cpp includes this header. */
#ifndef LEGORRR_GAME_OBJECTRECALL_H
#define LEGORRR_GAME_OBJECTRECALL_H

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

/* ObjectRecall_Globs' entry: one minifigure, 0x14 bytes. */
typedef struct ObjectRecall {
    unsigned int abilityFlags;  /* +0x00 LegoObject +0x3f0 */
    unsigned int level;         /* +0x04 LegoObject +0x330 */
    char customName[12];        /* +0x08 */
} ObjectRecall;

void object_recall_store_minifigure(struct Lego_Object* liveObj); /* 00459190 */
int object_recall_recall_minifigure(struct Lego_Object* liveObj); /* 00459240 */
void object_recall_reset(void);                                    /* 004592a0 */
void object_recall_snapshot(void);                                 /* 004592e0 */
int object_recall_restore(void);                                   /* 00459360 */
int object_recall_is_loaded(void);                                 /* 004593d0 */
int object_recall_save(const char* fname);                         /* 004593e0 */
int object_recall_load_rrosfile(const char* fname);                /* 00459470 */

/* The .osf file's image of an ObjectRecall: its Win32 layout, 0x14 bytes,
 * little-endian (abilityFlags at +0x00, level at +0x04, customName at
 * +0x08). The file holds the 'RRSO' magic and the count, then one image per
 * entry. */
#define OBJECT_RECALL_IMAGE_SIZE 0x14
void object_recall_serialize(const ObjectRecall* objRecall, unsigned char* image);
void object_recall_parse(const unsigned char* image, ObjectRecall* objRecall);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OBJECTRECALL_H */
