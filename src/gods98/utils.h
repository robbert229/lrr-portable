// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 string and token helpers (original translation unit G98-004,
 * finding 505): tokenise, duplicate, search, hash and read a boolean.
 *
 * C-compatible header (ADR 0005). utils.c is compiled as C in the matching
 * build and as C++ in the native port. The C++ code keeps using the
 * declarations in gods98/sound.hpp, gods98/string.hpp, gods98/block.hpp,
 * game/frontend.hpp and game/util.hpp, which carry extern "C";
 * src/game/test_c_boundaries.cpp includes them with this header and fails to
 * compile if they disagree. Shared C++ headers do not include this file:
 * every include and declaration a header adds shifts MSVC's symbol
 * numbering, and with it the code generated for unrelated functions
 * (finding 506). */
#ifndef LEGORRR_GODS98_UTILS_H
#define LEGORRR_GODS98_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00477440: split `text` in place at each occurrence of the string
 * `separators`, writing a pointer to each part to `out`. Returns the part
 * count, 0 for an empty string. */
int sound_variant_split(char* text, char** out, const char* separators);

/* FUN_004774b0: split `text` in place at each run of whitespace. A leading
 * run gives an empty first part, and a trailing run an empty last part. */
int whitespace_tokenize(char* text, char** fields);

/* FUN_00477550: a heap copy of `text`. The caller frees it. */
char* string_duplicate(const char* text);

/* FUN_00477590: format into 256 bytes, turn `\n` into a newline and `_`
 * into a space, and return a heap copy of the result. */
char* formatted_text_escape_decode_and_emit(const char* format, ...);

/* FUN_00477610: the first case-insensitive occurrence of `needle` in
 * `haystack`, or NULL. An empty string matches nothing. */
const char* case_insensitive_substring_find(const char* haystack,
                                            const char* needle);

/* FUN_00477670: polynomial hash of `text`, optionally skipping whitespace
 * and folding to upper case. */
int game_string_hash_compute(const unsigned char* text,
                             int skip_whitespace, int uppercase);

/* FUN_00477710: 1 for YES, TRUE or ON, 0 for NO, FALSE or OFF (without
 * case), and 2 for anything else. */
int block_get_bool(const char* value);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_UTILS_H */
