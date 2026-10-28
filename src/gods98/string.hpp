// SPDX-License-Identifier: LGPL-3.0-only
/* Corpus text primitives (findings 397/12): the string helpers the engine
 * callers use throughout, from the CRT string region.
 *
 * Deviations (documented): the corpus allocates the duplicate through its
 * engine-heap wrapper (TODO(corpus) seam); the port mallocs. The substring
 * scanner's aligned word-at-a-time SWAR scan is a pure speed optimization —
 * the port uses plain byte semantics, identical results. The config path
 * builder writes into a fixed static buffer and stops appending when full,
 * where the corpus copies through the static limit. */
#ifndef LEGORRR_GODS98_STRING_HPP
#define LEGORRR_GODS98_STRING_HPP

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00477550: NUL copy through the engine heap. The corpus copies
 * strlen+1 bytes via EngineHeapAlloc (TODO(corpus) wrapper — malloc here);
 * NULL on allocation failure (corpus would have asserted in the heap
 * seam). Caller frees. */
char* string_duplicate(const char* text);

/* FormattedTextBufferBuild (FUN_0048e1e0): format into caller-owned storage
 * and return the number of bytes present before NUL. Capacity bounds the
 * fixed scratch buffers used by the software port. */
uint32_t formatted_text_buffer_build(char* out, size_t capacity,
                                     const char* format, va_list args);

/* FUN_0048da50: first occurrence of `needle` in `haystack`, or NULL.
 * Empty needle yields the haystack start, exactly as the corpus's
 * early-outs do. The corpus's 1-byte fast path (aligned uint scan) and
 * word-at-a-time SWAR loop are byte-equivalent strstr behavior; the port
 * calls strstr. */
const char* string_find_substring(const char* haystack, const char* needle);

/* __strnicmp (FUN_0049dab0): the CRT's bounded case-insensitive compare —
 * ASCII fold, stops at NUL or the count, answers 0/-1/+1 (the corpus's
 * sign-normalized result, not the character difference). */
/* __strcmpi FUN_0048ff10: case-folded strcmp, byte difference (0 on match) */
int strcasecmp_lite(const char* a, const char* b);
/* __strnicmp FUN_0049dab0: bounded case-insensitive compare, 0/-1/+1 */
int strnicmp_lite(const char* a, const char* b, int count);

/* CaseInsensitiveSubstringFind (FUN_00477610): the folded sibling of the
 * plain find — every start position through the corpus's scan bound (the
 * haystack start to the last byte; the corpus do-while skips the empty
 * tail position) gets a bounded fold compare of the needle length.
 * Corpus gate pinned: an empty haystack or an empty needle answers null
 * (the corpus's strlen-shaped tests). Deviations: the port guards the
 * two pointers, and its fold compare stops where the corpus strides a
 * terminated haystack byte against an unterminated needle tail. */
const char* case_insensitive_substring_find(const char* haystack,
                                            const char* needle);
                                                /* FUN_00477610 */

/* StringCaseSubstringFind (FUN_0048c040): the corpus's other
 * case-insensitive substring walk — same __strnicmp shape (the bounded
 * ASCII fold compare, exposed here for the LWO bands), but the match also
 * reports its byte index and the needle length through the optional
 * outputs. Same gates/deviations as CaseInsensitiveSubstringFind. */
int string_folded_compare(const char* a, const char* b, size_t n);
char* string_case_substring_find(char* haystack, const char* needle,
                                 uint32_t* out_index,
                                 size_t* out_length);   /* FUN_0048c040 */

/* FUN_00493ec0: length of `text` up to `limit` — index of the first NUL,
 * or `limit` when no NUL appears within the bound (the corpus returns the
 * bound itself when it runs out first; limit 0 reads nothing and answers
 * 0). Negative limits are outside the corpus's caller set; the port
 * answers limit-as-is (deviation). */
int string_length_bounded(const char* text, int limit);

#define STRING_PATH_MAX 256 /* static builder buffer (corpus DAT_00506098) */

/* FUN_00478f50: joins the `::`-delimited config path root plus
 * NULL-terminated trailing components into the static builder buffer and
 * returns it (the same buffer every call — single-threaded use, as the
 * corpus's static scratch). This only BUILDS the path: the record lookup
 * itself is block_find (the corpus's other half). Appends stop at the
 * buffer bound where the corpus runs past the static. */
const char* string_path_join(const char* root, ...);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_STRING_HPP */
