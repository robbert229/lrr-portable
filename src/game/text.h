// SPDX-License-Identifier: LGPL-3.0-or-later
/* Text.c (original translation unit GAME-045, finding 505; finding 607):
 * the in-game message panel's typed messages (Text_Walk to
 * Text_UnitUpgraded), with their images and sounds, and the NERPs message
 * line, by the unit's own names.
 *
 * C-compatible header (ADR 0005). text.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * In the matching build text_link_names.h, which text.c includes first,
 * gives nine of these functions the port's names (message.hpp,
 * frontend.hpp, lego_unit_imports.h), so every caller reaches them.
 * Natively nothing calls these names: the port tail text_port.cpp serves
 * the port's names. */
#ifndef LEGORRR_GAME_TEXT_H
#define LEGORRR_GAME_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif

struct Area2F;
struct TextWindow;

void text_load(struct TextWindow* textOnlyWindow,
               struct TextWindow* textImageWindow, unsigned int imageX,
               unsigned int imageY, float pauseTime);           /* 0046a7f0 */
void text_initialise(const char* panelImageFile, unsigned int panelX,
                     unsigned int panelY, unsigned int panelOpenY,
                     const struct Area2F* messageArea,
                     const struct Area2F* imageArea,
                     float panelTimer);                         /* 0046a950 */
void text_clear(void);                                          /* 0046a9e0 */
void text_restart(void);                                        /* 0046aa90 */
int text_get_type_from_name(const char* name, int* type);       /* 0046aad0 */
void text_set_message(int textType, const char* message);       /* 0046ab10 */
void text_set_message_with_image(int textType, const char* message,
                                 const char* imageFile,
                                 const char* soundName);        /* 0046abb0 */
void text_set_nerps_message(char* message, unsigned int counter); /* 0046ac20 */
void text_display_message(int textType, int changeTiming,
                          int setFlag4);                        /* 0046ac60 */
void text_update(float elapsed);                                /* 0046ad00 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TEXT_H */
