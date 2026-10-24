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

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00477550: NUL copy through the engine heap. The corpus copies
 * strlen+1 bytes via EngineHeapAlloc (TODO(corpus) wrapper — malloc here);
 * NULL on allocation failure (corpus would have asserted in the heap
 * seam). Caller frees. */
char* string_duplicate(const char* text);

/* FUN_0048da50: first occurrence of `needle` in `haystack`, or NULL.
 * Empty needle yields the haystack start, exactly as the corpus's
 * early-outs do. The corpus's 1-byte fast path (aligned uint scan) and
 * word-at-a-time SWAR loop are byte-equivalent strstr behavior; the port
 * calls strstr. */
const char* string_find_substring(const char* haystack, const char* needle);

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
