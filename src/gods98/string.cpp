// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.hpp"
#include "string.hpp"

// FUNCTION: LEGORR 0x00477550
char* string_duplicate(const char* text)
{
    if (text == NULL) {
        return NULL; /* pinned deviation: the corpus scas's blindly */
    }
    /* Corpus: engine_heap_alloc(len+1) then the strcpy intrinsic (second
     * scas + dword/byte rep movs). engine_heap_alloc failure rides blind
     * exactly like the corpus (no guard). */
    char* copy = (char*)engine_heap_alloc((long)(strlen(text) + 1u));
    strcpy(copy, text);
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

/* FUN_0048da50: substring find. Keep the corpus's leading-byte/leading-pair
 * gates and compare the remaining needle two bytes at a time. Advancing the
 * candidate one byte after a failed pair retains strstr's overlapping-match
 * behavior. */
// FUNCTION: LEGORR 0x0048da50
const char* string_find_substring(const char* haystack, const char* needle)
{
    const unsigned char* text = (const unsigned char*)haystack;
    const unsigned char* pattern = (const unsigned char*)needle;
    const unsigned char first = pattern[0];
    if (first == 0u) {
        return haystack;
    }
    const unsigned char second = pattern[1];
    if (second == 0u) {
        return (const char*)strchr(haystack, first);
    }
    for (;;) {
        unsigned char current;
        while ((current = *text) != first) {
            if (current == 0u) {
                return NULL;
            }
            ++text;
        }
        const unsigned char* candidate = text;
        ++text;
        if (*text == second) {
            const unsigned char* scan = text + 1;
            const unsigned char* next = pattern + 2;
            for (;;) {
                const unsigned char first_tail = *next;
                if (first_tail == 0u) {
                    return (const char*)candidate;
                }
                if (*scan != first_tail) {
                    break;
                }
                ++scan;
                ++next;
                const unsigned char second_tail = *next;
                if (second_tail == 0u) {
                    return (const char*)candidate;
                }
                if (*scan != second_tail) {
                    break;
                }
                ++scan;
                ++next;
            }
        }
    }
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

/* __strcmpi (FUN_0048ff10): compare raw bytes first, then fold ASCII A-Z on
 * a mismatch and retry when the folded bytes agree. The corpus normalizes a
 * remaining mismatch to -1/+1 (not the byte difference); its initialized
 * locale-table branch calls the shared fold helper instead. This inline
 * range-check spelling covers the corpus's table-uninitialized branch. */
// FUNCTION: LEGORR 0x0048ff10
int strcasecmp_lite(const char* a, const char* b)
{
    const unsigned char* pa = (const unsigned char*)a;
    const unsigned char* pb = (const unsigned char*)b;
    for (;;) {
        unsigned char ca = *pa++;
        unsigned char cb = *pb++;
        if (ca == cb) {
            if (ca == 0u) return 0;
            continue;
        }
        if (ca >= (unsigned char)'A' && ca <= (unsigned char)'Z') {
            ca = (unsigned char)(ca + 0x20u);
        }
        if (cb >= (unsigned char)'A' && cb <= (unsigned char)'Z') {
            cb = (unsigned char)(cb + 0x20u);
        }
        if (ca != cb) return ca < cb ? -1 : 1;
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
    unsigned char ca;
    unsigned char cb;
    do {
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
    } while (count != 0);
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
    /* Corpus walk: keep the haystack pointer in its walking register before
     * the two intrinsic strlen scans, then use separate zero-length gates
     * and an unsigned bound beside an incrementing counter (the needle is
     * re-read from its stack slot every pass, the length hoisted into ebp). */
    const char* hay = haystack;
    const unsigned hlen = (unsigned)strlen(hay);
    const unsigned nlen = (unsigned)strlen(needle);
    if (hlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (nlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
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
