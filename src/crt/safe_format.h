// SPDX-License-Identifier: LGPL-3.0-or-later
/* The units' snprintf and vsnprintf (crt/vc5names.h): a formatter safe for
 * format strings from game data. Units pass dialog, level, NERPs, tooltip,
 * menu and info text as formats (finding 693), so a mod controls them.
 *
 * - d i u o x X (lengths hh h l ll z j t) and f F e E g G a A reach the
 *   host's snprintf one conversion at a time, rebuilt from what was parsed.
 *   L is accepted and ignored: callers pass doubles.
 * - s, c, p, %, infinities, NaNs and field padding are done here; floats get
 *   two-digit exponents where msvcrt prints three. Other float digits,
 *   rounding and %a are the host's.
 * - %n takes its pointer and writes nothing.
 * - Any other letter, the port's %b image token among them, prints as the
 *   letter and takes no argument; a "*" before it takes its int.
 * - An unfinished conversion at the end of the format prints nothing and
 *   takes nothing.
 * - Widths cap at 1024 and precisions at 256.
 *
 * The output is truncated to size - 1 characters and terminated; the return
 * value is C99's (the full length, or -1 above INT_MAX). */
#ifndef LEGORRR_CRT_SAFE_FORMAT_H
#define LEGORRR_CRT_SAFE_FORMAT_H

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cmath>

#define CRT_FORMAT_MAX_WIDTH 1024
#define CRT_FORMAT_MAX_PRECISION 256

struct CrtFormatSink {
    char* buffer;
    size_t size;
    uint64_t count;
};

/* n characters of s, or n copies of fill when s is NULL. */
inline void crt_format_put(CrtFormatSink* out, const char* s, size_t n, char fill)
{
    uint64_t room = (out->count + 1 < out->size) ? out->size - 1 - out->count : 0;
    size_t stored = (uint64_t)n < room ? n : (size_t)room;
    if (stored && s) memcpy(out->buffer + out->count, s, stored);
    else if (stored) memset(out->buffer + out->count, fill, stored);
    out->count += n;
}

/* text in a field: left-aligned, zero-padded after a sign or "0x", or
 * right-aligned. */
inline void crt_format_field(CrtFormatSink* out, const char* text, size_t len, int width, bool left, bool zero)
{
    size_t pad = (size_t)width > len ? (size_t)width - len : 0;
    size_t lead = 0;

    if (left) {
        crt_format_put(out, text, len, 0);
        crt_format_put(out, NULL, pad, ' ');
        return;
    }
    if (zero) {
        if (len > 0 && (text[0] == '-' || text[0] == '+' || text[0] == ' ')) lead = 1;
        if (len >= lead + 2 && text[lead] == '0' && (text[lead + 1] | 0x20) == 'x') lead += 2;
    }
    crt_format_put(out, text, lead, 0);
    crt_format_put(out, NULL, pad, zero ? '0' : ' ');
    crt_format_put(out, text + lead, len - lead, 0);
}

/* One conversion by the host into text[cap]; the length stored. */
template <typename T>
inline size_t crt_format_host(char* text, size_t cap, const char* spec, int precision, T value)
{
    int len = precision >= 0 ? snprintf(text, cap, spec, precision, value) : snprintf(text, cap, spec, value);
    return len < 0 ? 0 : (size_t)len < cap ? (size_t)len : cap - 1;
}

/* A width or precision: digits saturated at limit, or a "*" int. */
inline int crt_format_number(const char** p, va_list* ap, int limit, bool* negative)
{
    int value = 0;
    *negative = false;
    if (**p == '*') {
        (*p)++;
        value = va_arg(*ap, int);
        if (value < 0) {
            *negative = true;
            value = value == INT_MIN ? INT_MAX : -value;
        }
    } else {
        for (; **p >= '0' && **p <= '9'; (*p)++) {
            if (value <= limit) value = value * 10 + (**p - '0');
        }
    }
    return value > limit ? limit : value;
}

inline int crt_vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
    CrtFormatSink out = {buffer, size, 0};
    va_list ap;
    const char* p = format;

    va_copy(ap, args);
    while (*p != '\0') {
        bool left = false, plus = false, space = false, alt = false, zero = false, negative;
        int width, precision = -1;
        char length = 0, conv, spec[16], text[1024];
        size_t len;
        const char* end;

        if (*p != '%') {
            crt_format_put(&out, p++, 1, 0);
            continue;
        }
        /* An unfinished conversion takes no argument. */
        for (end = ++p; *end != '\0' && strchr("-+ #0123456789.*hlzjtL", *end); end++) {
        }
        if (*end == '\0') break;
        for (;; p++) {
            if (*p == '-') left = true;
            else if (*p == '+') plus = true;
            else if (*p == ' ') space = true;
            else if (*p == '#') alt = true;
            else if (*p == '0') zero = true;
            else break;
        }
        width = crt_format_number(&p, &ap, CRT_FORMAT_MAX_WIDTH, &negative);
        left = left || negative;
        if (*p == '.') {
            p++;
            precision = crt_format_number(&p, &ap, CRT_FORMAT_MAX_PRECISION, &negative);
            if (negative) precision = -1;
        }
        /* length: 'H' for hh, 'L' for ll, else the letter; L itself is dropped. */
        if ((p[0] == 'h' || p[0] == 'l') && p[1] == p[0]) {
            length = (char)(p[0] - 'a' + 'A');
            p += 2;
        } else if (*p != '\0' && strchr("hlzjtL", *p)) {
            length = *p == 'L' ? 0 : *p;
            p++;
        }
        conv = *p++;

        switch (conv) {
        case '%':
            crt_format_put(&out, "%", 1, 0);
            break;
        case 'n':
            // [divergence:printf-n] a %n in game text writes nothing.
            (void)va_arg(ap, void*);
            break;
        case 'c':
            text[0] = (char)va_arg(ap, int);
            crt_format_field(&out, text, 1, width, left, false);
            break;
        case 's': {
            const char* s = va_arg(ap, const char*);
            size_t limit = precision < 0 ? SIZE_MAX : (size_t)precision;
            if (s == NULL) s = "(null)";
            for (len = 0; len < limit && s[len] != '\0'; len++) {
            }
            crt_format_field(&out, s, len, width, left, false);
            break;
        }
        case 'p':
            len = crt_format_host(text, sizeof(text), "0x%llx", -1, (unsigned long long)(uintptr_t)va_arg(ap, void*));
            crt_format_field(&out, text, len, width, left, false);
            break;
        case 'd':
        case 'i': {
            long long v = length == 'L' ? va_arg(ap, long long)
                        : length == 'l' ? va_arg(ap, long)
                        : length == 'z' ? (long long)(ptrdiff_t)va_arg(ap, size_t)
                        : length == 'j' ? (long long)va_arg(ap, intmax_t)
                        : length == 't' ? (long long)va_arg(ap, ptrdiff_t)
                                        : (long long)va_arg(ap, int);
            if (length == 'H') v = (signed char)v;
            else if (length == 'h') v = (short)v;
            snprintf(spec, sizeof(spec), "%%%s%s%slld", plus ? "+" : "", space ? " " : "", precision >= 0 ? ".*" : "");
            len = crt_format_host(text, sizeof(text), spec, precision, v);
            crt_format_field(&out, text, len, width, left, zero && precision < 0);
            break;
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            unsigned long long v = length == 'L' ? va_arg(ap, unsigned long long)
                                 : length == 'l' ? va_arg(ap, unsigned long)
                                 : length == 'z' ? (unsigned long long)va_arg(ap, size_t)
                                 : length == 'j' ? (unsigned long long)va_arg(ap, uintmax_t)
                                 : length == 't' ? (unsigned long long)(size_t)va_arg(ap, ptrdiff_t)
                                                 : (unsigned long long)va_arg(ap, unsigned int);
            if (length == 'H') v = (unsigned char)v;
            else if (length == 'h') v = (unsigned short)v;
            snprintf(spec, sizeof(spec), "%%%s%sll%c", alt && conv != 'u' ? "#" : "", precision >= 0 ? ".*" : "", conv);
            len = crt_format_host(text, sizeof(text), spec, precision, v);
            crt_format_field(&out, text, len, width, left, zero && precision < 0);
            break;
        }
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        case 'a':
        case 'A': {
            double v = va_arg(ap, double);
            char* e;
            if (!std::isfinite(v)) {
                /* C99's spelling; msvcrt and the UCRT differ. */
                bool upper = conv >= 'A' && conv <= 'Z';
                len = (size_t)snprintf(text, sizeof(text), "%s%s", std::signbit(v) ? "-" : plus ? "+" : space ? " " : "",
                                       std::isnan(v) ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf"));
                crt_format_field(&out, text, len, width, left, false);
                break;
            }
            snprintf(spec, sizeof(spec), "%%%s%s%s%s%c", plus ? "+" : "", space ? " " : "", alt ? "#" : "",
                     precision >= 0 ? ".*" : "", conv);
            len = crt_format_host(text, sizeof(text), spec, precision, v);
            /* Two exponent digits at least, as C99 says; msvcrt prints three. */
            e = (conv | 0x20) == 'a' ? NULL : strpbrk(text, "eE");
            while (e && (e[1] == '+' || e[1] == '-') && strlen(e + 2) > 2 && e[2] == '0') {
                memmove(e + 2, e + 3, strlen(e + 2));
                len--;
            }
            crt_format_field(&out, text, len, width, left, zero);
            break;
        }
        default:
            // [divergence:printf-unknown-letter] any other letter, the
            // port's %b image token among them, prints as the letter.
            crt_format_put(&out, &conv, 1, 0);
            break;
        }
    }
    va_end(ap);

    if (size > 0) buffer[out.count < size ? out.count : size - 1] = '\0';
    return out.count > (uint64_t)INT_MAX ? -1 : (int)out.count;
}

inline int crt_snprintf(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    int len;

    va_start(args, format);
    len = crt_vsnprintf(buffer, size, format, args);
    va_end(args);
    return len;
}

#endif /* LEGORRR_CRT_SAFE_FORMAT_H */
