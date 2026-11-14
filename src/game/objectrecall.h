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

void object_recall_store_minifigure(struct Lego_Object* liveObj); /* 00459190 */
int object_recall_recall_minifigure(struct Lego_Object* liveObj); /* 00459240 */
void object_recall_reset(void);                                    /* 004592a0 */
void object_recall_snapshot(void);                                 /* 004592e0 */
int object_recall_restore(void);                                   /* 00459360 */
int object_recall_is_loaded(void);                                 /* 004593d0 */
int object_recall_save(const char* fname);                         /* 004593e0 */
int object_recall_load_rrosfile(const char* fname);                /* 00459470 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OBJECTRECALL_H */
