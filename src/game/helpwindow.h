// SPDX-License-Identifier: LGPL-3.0-or-later
/* HelpWindow.c (original translation unit GAME-018, finding 505), rebuilt
 * in original shape (finding 595).
 *
 * C-compatible header (ADR 0005). helpwindow.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the same functions.
 *
 * These are the unit's own names. In the matching build seven of them take
 * the port's names (helpwindow_link_names.h); natively the port tail
 * helpwindow_port.cpp serves the port names with the port's own model. */
#ifndef LEGORRR_GAME_HELPWINDOW_H
#define LEGORRR_GAME_HELPWINDOW_H

#include "gods98/config.h"
#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Font;

void help_window_set_font(struct Font* font);                  /* 00418190 */
void help_window_hide(void);                                   /* 004181a0 */
void help_window_initialise(BlockFile* config, const char* gameName); /* 004181b0 */
void help_window_load_infos(BlockFile* config, const char* gameName); /* 00418450 */
void help_window_load_buttons(BlockFile* config, const char* gameName); /* 004185f0 */
void help_window_show_text(int show);                          /* 00418800 */
void help_window_recall_dependencies(int objType, int objIndex, int objLevel,
                                     int noDisplay);           /* 00418830 */
void help_window_list_dependencies(int objType, int objIndex,
                                   int objLevel);              /* 00418920 */
void help_window_close(void);                                  /* 004189d0 */
void help_window_update(void);                                 /* 00418a00 */
int help_window_check_buttons(unsigned int mouseX, unsigned int mouseY,
                              int leftButton, int leftButtonLast,
                              int leftReleased, float elapsed); /* 00418da0 */
void help_window_button_tooltip(int button);                   /* 00418f80 */
int help_window_press_button(int button, int leftButton,
                             int leftButtonLast);              /* 00418fc0 */
void help_window_button_action(int button, float elapsed);     /* 00419030 */
void help_window_draw_buttons(void);                           /* 00419100 */
Image* help_window_get_button_image(int button);               /* 00419150 */
int help_window_is_showing(void);                              /* 004191a0 */
int help_window_is_enabled(void);                              /* 004191f0 */
void help_window_toggle(int toggle, int disable);              /* 00419200 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_HELPWINDOW_H */
