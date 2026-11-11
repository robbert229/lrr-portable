// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/crt.hpp"
#include "heap.hpp"
#include "string.hpp"

#if defined(_MSC_VER)
/* VC5's output.obj formatter is the same __output routine used by the
 * reference's bounded-string stream wrapper at 0x48e1e0. */
extern "C" int __cdecl _output(FILE* stream, const char* format,
                                va_list args);
extern "C" int __cdecl _flsbuf(int value, FILE* stream);
#endif

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

