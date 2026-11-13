// SPDX-License-Identifier: LGPL-3.0-or-later
/* Pointer.c (original translation unit GAME-033, finding 505; finding 612):
 * the mouse pointer's 56 types, their bitmaps and flics, and the current
 * pointer, by the unit's own names.
 *
 * C-compatible header (ADR 0005). pointer.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * In the matching build pointer_link_names.h, which pointer.c includes
 * first, gives these functions the port's names (ui_list.hpp, frontend.hpp,
 * media.hpp, crt.hpp, lego_unit_imports.h), so every caller reaches them.
 * Natively nothing calls these names: the port tail pointer_port.cpp serves
 * the port's names. */
#ifndef LEGORRR_GAME_POINTER_H
#define LEGORRR_GAME_POINTER_H

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Image;

void pointer_initialise(void);                                   /* 0045c830 */
void pointer_load_pointers(struct BlockRec* config);             /* 0045ca70 */
int pointer_get_type(const char* name, int* pointerType);        /* 0045cbd0 */
struct Image* pointer_get_image(int pointerType);                /* 0045cc10 */
void pointer_set_current_if_timer_finished(int pointerType);     /* 0045cc20 */
void pointer_set_current(int pointerType, float timer);          /* 0045cc40 */
int pointer_get_current_type(void);                              /* 0045cc60 */
void pointer_draw_pointer(unsigned int mouseX,
                          unsigned int mouseY);                  /* 0045cc70 */
void pointer_update(float elapsed);                              /* 0045cd90 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_POINTER_H */
