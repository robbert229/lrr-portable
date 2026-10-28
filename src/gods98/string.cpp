// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string.hpp"

/* FUN_00477550: allocate strlen+1 through the heap seam and copy. The
 * corpus allocates via its engine-heap wrapper (TODO(corpus)); the port
 * mallocs (deviation in string.hpp). */
// FUNCTION: LEGORR 0x00477550
char* string_duplicate(const char* text)
{
    if (text == NULL) {
        return NULL;
    }
    const size_t n = strlen(text) + 1u;
    char* copy = (char*)malloc(n);
    if (copy == NULL) {
        return NULL; /* corpus: the heap seam would have asserted */
    }
    memcpy(copy, text, n);
    return copy;
}

/* FormattedTextBufferBuild (FUN_0048e1e0): bounded software equivalent of
 * the engine's va_list formatter. The returned count excludes the NUL. */
// FUNCTION: LEGORR 0x0048E1E0
uint32_t formatted_text_buffer_build(char* out, size_t capacity,
                                     const char* format, va_list args)
{
    if (out == NULL || capacity == 0) return 0;
    out[0] = '\0';
    if (format == NULL) return 0;
#if defined(_MSC_VER) && _MSC_VER < 1900
    const int result = _vsnprintf(out, capacity, format, args);
    if (result < 0) out[capacity - 1] = '\0';
#else
    (void)vsnprintf(out, capacity, format, args);
#endif
    return (uint32_t)strlen(out);
}

/* FUN_0048da50: substring find. The corpus's aligned-word/SWAR scan is a
 * byte-equivalent strstr, plus the same early-outs (empty needle -> the
 * haystack start). */
// FUNCTION: LEGORR 0x0048da50
const char* string_find_substring(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return NULL;
    }
    if (*needle == '\0') {
        return haystack;
    }
    return strstr(haystack, needle);
}

/* CaseInsensitiveSubstringFind (FUN_00477610) — see string.hpp. The
 * bounded fold compare models the corpus's __strnicmp: ASCII case-fold,
 * exactly `n` bytes, stopping early when the bytes agree on a NUL.
 * Deviation pinned: the corpus keeps striding a terminated haystack byte
 * against an unterminated needle tail; the port's compare stops. */
static int s_strnfoldcmp(const char* a, const char* b, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        int ca = (unsigned char)a[i];
        int cb = (unsigned char)b[i];
        if (ca >= 'A' && ca <= 'Z') {
            ca += 'a' - 'A';
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb += 'a' - 'A';
        }
        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
    }
    return 0;
}

/* __strcmpi (FUN_0048ff10): the CRT's case-folded strcmp — lower A-Z folds
 * upward via range checks, compares byte-wise, answers the byte difference
 * (0 on match). Used by the corpus's Level03 checkpoint hard-code
 * comparison and the string-registry walk. */
// FUNCTION: LEGORR 0x0048ff10
int strcasecmp_lite(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (ca >= 'a' && ca <= 'z') ca -= 0x20;
        if (cb >= 'a' && cb <= 'z') cb -= 0x20;
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
}

/* __strnicmp (FUN_0049dab0): the CRT's bounded case-insensitive compare —
 * ASCII-folded (A-Z += 0x20 via range checks, the corpus's table-uninit
 * fallback shape), stops at NUL or the count, answers 0/-1/+1 (the corpus's
 * sign-normalized result, not the character difference). Deviation: the
 * corpus lazy-builds a fold table at DAT_00549178; the port folds inline. */
// FUNCTION: LEGORR 0x0049dab0
int strnicmp_lite(const char* a, const char* b, int count)
{
    if (count == 0) {
        return 0;
    }
    const unsigned char* pa = (const unsigned char*)a;
    const unsigned char* pb = (const unsigned char*)b;
    unsigned char ca = 0;
    unsigned char cb = 0;
    for (;;) {
        ca = *pa;
        cb = *pb;
        if (ca == 0u || cb == 0u) {
            break; /* the corpus compares the raw bytes at a NUL break */
        }
        ++pa;
        ++pb;
        if (ca >= (unsigned char)'A' && ca <= (unsigned char)'Z') {
            ca = (unsigned char)(ca + 0x20u);
        }
        if (cb >= (unsigned char)'A' && cb <= (unsigned char)'Z') {
            cb = (unsigned char)(cb + 0x20u);
        }
        if (ca != cb) {
            break;
        }
        --count;
        if (count == 0) {
            return 0;
        }
    }
    if (ca == cb) {
        return 0;
    }
    return (ca < cb) ? -1 : 1;
}

// FUNCTION: LEGORR 0x00477610
const char* case_insensitive_substring_find(const char* haystack,
                                            const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return NULL; /* pinned deviation: the corpus dereferences both blindly */
    }
    /* Corpus walk: two intrinsic strlens into registers, two separate
     * zero-length gates, then a walking haystack pointer beside an inc
     * counter with an unsigned bound (the needle is re-read from its stack
     * slot every pass, the length hoisted once into ebp). */
    const unsigned hlen = (unsigned)strlen(haystack);
    const unsigned nlen = (unsigned)strlen(needle);
    if (hlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (nlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    const char* hay = haystack;
    for (unsigned i = 0; i < hlen; ++i) {
        if (strnicmp_lite(hay, needle, (int)nlen) == 0) {
            return hay;
        }
        ++hay;
    }
    return NULL;
}

/* FUN_00493ec0: length bounded by `limit` (the corpus walks byte-wise,
 * decrementing the bound first, and answers `limit` when no NUL turns up
 * inside it — byte-identical to scanning min(strlen, limit)). */
// FUNCTION: LEGORR 0x00493ec0
int string_length_bounded(const char* text, int limit)
{
    if (text == NULL || limit <= 0) {
        return 0;
    }
    int n = 0;
    while (n < limit && text[n] != '\0') {
        ++n;
    }
    return n;
}

/* FUN_00478f50: `root::comp1::comp2…` into the static builder. */
// FUNCTION: LEGORR 0x00478f50
const char* string_path_join(const char* root, ...)
{
    static char s_path[STRING_PATH_MAX]; /* the corpus static scratch */
    if (root == NULL) {
        root = "";
    }
    int room = (int)sizeof s_path;
    int used = 0;
    size_t len = strlen(root);
    if (len >= (size_t)room) {
        len = (size_t)room - 1u;
    }
    memcpy(s_path, root, len);
    used = (int)len;
    s_path[used] = '\0';

    va_list ap;
    va_start(ap, root);
    for (const char* part = va_arg(ap, const char*);
         part != NULL; part = va_arg(ap, const char*)) {
        if (room - used < 3) {
            break; /* out of room: stop joining (bounded deviation) */
        }
        s_path[used++] = ':';
        s_path[used++] = ':';
        len = strlen(part);
        if (len > (size_t)(room - used - 1)) {
            len = (size_t)(room - used - 1);
        }
        memcpy(s_path + used, part, len);
        used += (int)len;
        s_path[used] = '\0';
    }
    va_end(ap);
    return s_path;
}

/* StringCaseSubstringFind (2026-10-25) — FUN_0048c040. Case-insensitive
 * substring walk over the corpus's __strnicmp (the same bounded fold
 * compare as CaseInsensitiveSubstringFind): answers the match pointer and
 * reports the byte index and the needle length through the optional
 * outputs. The corpus gates on a non-empty haystack and a non-empty
 * needle. Deviations: null strings are guarded (the corpus dereferences
 * both), and the fold compare stops at NUL (the corpus strides). */

int string_folded_compare(const char* a, const char* b, size_t n)
{
    return s_strnfoldcmp(a, b, n);
}

// FUNCTION: LEGORR 0x0048c040
char* string_case_substring_find(char* haystack, const char* needle,
                                 uint32_t* out_index, size_t* out_length)
{
    if (haystack == NULL || needle == NULL) {
        return NULL; /* deviation: the corpus dereferences both blindly */
    }
    /* Corpus walk: two intrinsic strlens into registers, three entry gates,
     * then a walking haystack pointer beside an inc counter with an unsigned
     * bound (the needle is re-read from its stack slot every pass). */
    char* hay = haystack;
    long hlen = (long)strlen(haystack);
    long nlen = (long)strlen(needle);
    if (hlen == 0) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (nlen == 0) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (hlen <= 0) {
        return NULL; /* the corpus's unsigned test/jbe bound before the walk */
    }
    for (unsigned index = 0; index < (unsigned)hlen; ++index) {
        if (strnicmp_lite(hay, needle, (int)nlen) == 0) {
            if (out_index != NULL) {
                *out_index = (uint32_t)index;
            }
            if (out_length != NULL) {
                *out_length = (size_t)nlen;
            }
            return hay;
        }
        ++hay;
    }
    return NULL;
}
