// SPDX-License-Identifier: LGPL-3.0-or-later
/* Keys.c (original translation unit G98-018, finding 513).
 *
 * C-compatible header (ADR 0005). keys.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp checks it against
 * the C++ declarations of the same functions. */
#ifndef LEGORRR_GODS98_KEYS_H
#define LEGORRR_GODS98_KEYS_H

#ifdef __cplusplus
extern "C" {
#endif

/* One name per DirectInput scan code (DIK_*). */
#define KEY_NAMES_COUNT 256

/* KEY_NAMES_COUNT + 1: keys.c explains the spare slot. */
extern const char* key_names[KEY_NAMES_COUNT + 1];

/* FUN_00485a20 (Keys_Initialise). */
void keyboard_key_names_init(void);

/* FUN_00485e30 (Key_Find): 1 and the scan code in *out_index when a name
 * matches, ignoring case; 0 otherwise. */
int named_option_index_find_case_insensitive(const char* name,
                                             unsigned char* out_index);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_KEYS_H */
