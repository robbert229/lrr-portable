// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/crt.hpp"
#include "string.hpp"

#if defined(_MSC_VER)
/* VC5's output.obj formatter is the same __output routine used by the
 * reference's bounded-string stream wrapper at 0x48e1e0. */
extern "C" int __cdecl _output(FILE* stream, const char* format,
                                va_list args);
extern "C" int __cdecl _flsbuf(int value, FILE* stream);
#endif

// FUNCTION: LEGORR 0x00477550
char* string_duplicate(const char* text)
{
    /* The reference operation is the CRT strdup idiom on the engine heap.
     * VC5's _strdup uses that same malloc/free heap; portable builds use the
     * CRT shim's engine-heap fallback. */
    return crt_string_duplicate(text);
}

/* FormattedTextBufferBuild (FUN_0048e1e0): VC5 builds use output.obj's
 * __output with a string-backed FILE; other builds use bounded vsnprintf. */
// FUNCTION: LEGORR 0x0048E1E0
uint32_t formatted_text_buffer_build(char* out, size_t capacity,
                                     const char* format, va_list args)
{
    if (out == NULL || capacity == 0) return 0;
    out[0] = '\0';
    if (format == NULL) return 0;
#if defined(_MSC_VER)
    FILE stream;
    int result;
    /* Match the target's temporary string stream: __output only needs these
     * four fields when _IOSTRG is set. */
    stream._ptr = out;
    stream._cnt = (int)((capacity - 1u) > 0x7fffffffu
                            ? 0x7fffffffu : capacity - 1u);
    stream._base = out;
    stream._flag = 0x42; /* _IOWRT | _IOSTRG */
    result = _output(&stream, format, args);
    if (stream._cnt >= 0) {
        *stream._ptr = '\0';
    }
    else {
        _flsbuf(0, &stream);
    }
    return (uint32_t)result;
#else
    (void)vsnprintf(out, capacity, format, args);
    return (uint32_t)strlen(out);
#endif
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
     * re-read from its stack slot every pass, the length hoisted into ebp).
     * On a miss the corpus increments the index before the hay pointer
     * (0x477655 inc edi; 0x477656 inc esi), then tests cmp/jb at 0x477657. */
    const char* hay = haystack;
    const unsigned hlen = (unsigned)strlen(hay);
    const unsigned nlen = (unsigned)strlen(needle);
    if (hlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (nlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    for (unsigned i = 0; i < hlen; ++i, ++hay) {
        if (strnicmp_lite(hay, needle, (int)nlen) == 0) {
            return hay;
        }
    }
    return NULL;
}

/* Portable fallback for VC5 aw_str.obj's private _strncnt helper at
 * 0x00493ec0. Its Reccmp library annotation lives with the CRT aliases in
 * src/crt/crt.cpp; keep this implementation only for non-MSVC builds. */
#if !defined(_MSC_VER)
int string_length_bounded(const char* text, int limit)
{
    const char* p = text;
    int remaining = limit;
    while (remaining != 0) {
        --remaining;
        if (*p == '\0') {
            break;
        }
        ++p;
    }
    if (*p != '\0') {
        return limit;
    }
    return (int)(p - text);
}
#endif

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
        strncat(s_path, "::", 2);
        used += 2;
        len = strlen(part);
        if (len > (size_t)(room - used - 1)) {
            len = (size_t)(room - used - 1);
        }
        strncat(s_path, part, len);
        used += (int)len;
    }
    va_end(ap);
    return s_path;
}

/* StringCaseSubstringFind (2026-10-25) — FUN_0048c040. Case-insensitive
 * substring walk over the corpus's __strnicmp (the same bounded CRT
 * compare as CaseInsensitiveSubstringFind): answers the match pointer and
 * reports the byte index and the needle length through the optional
 * outputs. The corpus gates on a non-empty haystack and a non-empty
 * needle. The fold compare uses the CRT's bounded compare. */

#if !defined(_MSC_VER) || !defined(_M_IX86)
int string_folded_compare(const char* a, const char* b, size_t n)
{
    return strnicmp_lite(a, b, (int)n);
}
#endif

// FUNCTION: LEGORR 0x0048c040
char* string_case_substring_find(char* haystack, const char* needle,
                                 uint32_t* out_index, size_t* out_length)
{
    if (haystack == NULL || needle == NULL) {
        return NULL; /* pinned deviation: test_string.cpp:145-147 */
    }
    /* Corpus walk: two intrinsic strlens into registers, three entry gates,
     * then a walking haystack pointer beside an inc counter with an unsigned
     * bound and test/jbe gate (the needle is re-read from its stack slot
     * every pass). */
    char* hay = haystack;
    unsigned int hlen = (unsigned int)strlen(haystack);
    unsigned int nlen = (unsigned int)strlen(needle);
    if (hlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (nlen == 0u) {
        return NULL; /* pinned: the corpus's strlen-shaped gate */
    }
    if (hlen <= 0u) {
        return NULL; /* the corpus's unsigned test/jbe bound before the walk */
    }
    for (unsigned index = 0; index < hlen; ++index) {
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
