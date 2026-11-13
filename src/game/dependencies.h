// SPDX-License-Identifier: LGPL-3.0-or-later
/* Dependencies.c (original translation unit GAME-010, finding 505; finding
 * 612): the object dependency table from Lego.cfg's Dependencies block, by
 * the unit's own names.
 *
 * C-compatible header (ADR 0005). dependencies.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it.
 *
 * In the matching build dependencies_link_names.h, which dependencies.c
 * includes first, gives these functions the port's names, so every caller
 * reaches them. Natively nothing calls these names: the port tail
 * dependencies_port.cpp and screen_effect_quads.cpp serve the port's
 * names. */
#ifndef LEGORRR_GAME_DEPENDENCIES_H
#define LEGORRR_GAME_DEPENDENCIES_H

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Lego_Object;
struct Dependencies_Requirement;
struct Dependencies_Dependents;

void dependencies_set_enabled(int on);                                   /* 0040ab10 */
void dependencies_reset_clear_all_level_flags_10c(void);                /* 0040ab30 */
void dependencies_initialise(struct BlockRec* config,
                             const char* gameName);                      /* 0040ab70 */
int dependencies_object_requirements_met(int objType, unsigned int objID,
                                         unsigned int objLevel);         /* 0040aea0 */
int dependencies_live_object_callback(struct Lego_Object* liveObj,
                                      void* data);                       /* 0040af40 */
void dependencies_object_get_requirements(int objType, unsigned int objID,
                                          unsigned int objLevel,
                                          struct Dependencies_Requirement** requirements,
                                          unsigned int* count);          /* 0040af90 */
void dependencies_object_find_dependents(int objType, unsigned int objID,
                                         unsigned int objLevel,
                                         struct Dependencies_Dependents* dependents); /* 0040b000 */
void dependencies_update(void);                                          /* 0040b1b0 */
unsigned int dependencies_object_is_unlocked(int objType, unsigned int objID,
                                             unsigned int objLevel);     /* 0040b250 */
void dependencies_object_add_level_flag_100(int objType, unsigned int objID,
                                            unsigned int objLevel);      /* 0040b2a0 */
unsigned int dependencies_object_get_level_flag_100(int objType,
                                                    unsigned int objID,
                                                    unsigned int objLevel); /* 0040b2e0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_DEPENDENCIES_H */
