// SPDX-License-Identifier: LGPL-3.0-only
/* Corpus text primitives (findings 397/12): the string helpers the engine
 * callers use throughout, from the CRT string region.
 *
 * Deviations (documented): the substring scanner's corpus
 * leading-byte/leading-pair gates and paired tail comparison are reproduced
 * in plain C — identical results. The config path builder
 * (FUN_00478f50) is in gods98/config.h. */
#ifndef LEGORRR_GODS98_STRING_HPP
#define LEGORRR_GODS98_STRING_HPP

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#if defined(_MSC_VER) && defined(_M_IX86)
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00477550: duplicate a NUL-terminated string (gods98/utils.c, which
 * also declares it in gods98/utils.h). `text` must not be NULL; callers
 * free the copy through the engine heap. */
char* string_duplicate(const char* text);

/* FormattedTextBufferBuild (FUN_0048e1e0): format into caller-owned storage
 * and return the number of bytes present before NUL. Capacity bounds the
 * stream-backed VC5 formatter and the portable vsnprintf implementation. */
uint32_t formatted_text_buffer_build(char* out, size_t capacity,
                                     const char* format, va_list args);

/* ObjectiveAdvisorTextCopy (FUN_00485300): format advisor text into the
 * record's inline buffer and update its mode/dirty fields. */
void objective_advisor_text_copy(void* record, int mode,
                                 const char* format, ...);

/* CRT entry shims are implemented in src/crt/crt.cpp and declared here for
 * the engine/game callers that use these string operations.
 * FUN_0048da50: first occurrence of `needle` in `haystack`, or NULL.
 * Empty needle yields the haystack start, exactly as the corpus's
 * early-outs do. The corpus's 1-byte fast path (aligned uint scan) and
 * word-at-a-time SWAR loop are byte-equivalent strstr behavior; the port
 * calls strstr. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define string_find_substring(haystack, needle) strstr((haystack), (needle))
#else
const char* string_find_substring(const char* haystack, const char* needle);
#endif

/* __strcmpi FUN_0048ff10: CRT case-insensitive strcmp. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define strcasecmp_lite(a, b) _strcmpi((a), (b))
#else
int strcasecmp_lite(const char* a, const char* b);
#endif
/* __strnicmp FUN_0049dab0: CRT bounded case-insensitive string compare. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define strnicmp_lite(a, b, count) _strnicmp((a), (b), (size_t)(count))
#else
int strnicmp_lite(const char* a, const char* b, int count);
#endif

/* CaseInsensitiveSubstringFind (FUN_00477610, gods98/utils.c): the first
 * position in `haystack` where a case-insensitive compare of the needle's
 * length matches, or NULL. An empty haystack or needle matches nothing;
 * neither may be NULL. */
const char* case_insensitive_substring_find(const char* haystack,
                                            const char* needle);
                                                /* FUN_00477610 */

/* StringCaseSubstringFind (FUN_0048c040): the corpus's other
 * case-insensitive substring walk — same __strnicmp shape (the bounded
 * ASCII fold compare, exposed here for the LWO bands), but the match also
 * reports its byte index and the needle length through the optional
 * outputs. Defined by gods98/lwo.c (G98-031); neither string may be
 * NULL. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define string_folded_compare(a, b, n) _strnicmp((a), (b), (n))
#else
int string_folded_compare(const char* a, const char* b, size_t n);
#endif
char* string_case_substring_find(char* haystack, const char* needle,
                                 uint32_t* out_index,
                                 size_t* out_length);   /* FUN_0048c040 */

/* FUN_00493ec0: length of `text` up to `limit` — index of the first NUL,
 * or `limit` when no NUL appears within the bound (the corpus returns the
 * bound itself when it runs out first; limit 0 reads nothing and answers
 * 0). Negative limits are outside the corpus's caller set; the port
 * answers limit-as-is (deviation). */
#if !defined(_MSC_VER)
int string_length_bounded(const char* text, int limit);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_STRING_HPP */
