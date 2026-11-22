// SPDX-License-Identifier: LGPL-3.0-or-later
/* The MSVC 5.0 LIBC names the units call (ADR 0005), on the host's C
 * library. The original linked LIBC.LIB statically; its old names (itoa,
 * stricmp, strnicmp, strupr, strlwr, through oldnames.lib) and the
 * underscore routines (_splitpath, _finite, _strcmpi) have no portable
 * equivalent, so this header supplies them. rand and srand keep the VC5
 * linear congruential generator, so the game's random sequences match the
 * original's for a given seed.
 *
 * Include this header after the C library headers: it redefines rand,
 * srand and vsprintf. */
#ifndef LEGORRR_CRT_VC5NAMES_H
#define LEGORRR_CRT_VC5NAMES_H

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

/* _rand and _srand: the VC5 LIBC generator (holdrand * 214013 + 2531011,
 * bits 16-30). */
inline uint32_t& crt_rand_state()
{
    static uint32_t holdrand = 1;
    return holdrand;
}
inline int crt_rand()
{
    uint32_t& s = crt_rand_state();
    s = s * 214013u + 2531011u;
    return (int)((s >> 16) & 0x7fff);
}
inline void crt_srand(unsigned int seed)
{
    crt_rand_state() = seed;
}
#define rand crt_rand
#define srand crt_srand

/* _itoa. */
inline char* crt_itoa(int value, char* out, int radix)
{
    char tmp[34];
    char* p = tmp;
    unsigned int v;
    char* o = out;
    if (radix == 10 && value < 0) {
        *o++ = '-';
        v = (unsigned int)(-(int64_t)value);
    } else {
        v = (unsigned int)value;
    }
    do {
        unsigned int d = v % (unsigned int)radix;
        *p++ = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned int)radix;
    } while (v != 0);
    while (p != tmp) *o++ = *--p;
    *o = '\0';
    return out;
}
#define itoa crt_itoa
#define _itoa crt_itoa

/* _strupr and _strlwr. */
inline char* crt_strupr(char* s)
{
    for (char* p = s; *p; p++) *p = (char)toupper((unsigned char)*p);
    return s;
}
inline char* crt_strlwr(char* s)
{
    for (char* p = s; *p; p++) *p = (char)tolower((unsigned char)*p);
    return s;
}
#define strupr crt_strupr
#define _strupr crt_strupr
#define strlwr crt_strlwr
#define _strlwr crt_strlwr

/* _strcmpi, _stricmp and _strnicmp. */
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define _strcmpi strcasecmp
#define strnicmp(a, b, count) strncasecmp((a), (b), (size_t)(count))
#define _strnicmp(a, b, count) strncasecmp((a), (b), (size_t)(count))

/* _finite. */
inline int crt_finite(double value)
{
    return std::isfinite(value) ? 1 : 0;
}
#define _finite crt_finite

/* _splitpath: drive, directory, file name and extension of a DOS path;
 * any output may be NULL. */
inline void crt_splitpath(const char* path, char* drive, char* dir,
                          char* fname, char* ext)
{
    const char* p = path;
    const char* lastSlash = NULL;
    const char* lastDot = NULL;
    const char* q;
    if (p[0] != '\0' && p[1] == ':') {
        if (drive) {
            drive[0] = p[0];
            drive[1] = ':';
            drive[2] = '\0';
        }
        p += 2;
    } else if (drive) {
        drive[0] = '\0';
    }
    for (q = p; *q; q++) {
        if (*q == '\\' || *q == '/') lastSlash = q + 1;
        else if (*q == '.') lastDot = q;
    }
    if (lastSlash) {
        if (dir) {
            memcpy(dir, p, (size_t)(lastSlash - p));
            dir[lastSlash - p] = '\0';
        }
        p = lastSlash;
    } else if (dir) {
        dir[0] = '\0';
    }
    if (lastDot && lastDot >= p) {
        if (fname) {
            memcpy(fname, p, (size_t)(lastDot - p));
            fname[lastDot - p] = '\0';
        }
        if (ext) strcpy(ext, lastDot);
    } else {
        if (fname) strcpy(fname, p);
        if (ext) ext[0] = '\0';
    }
}
#define _splitpath crt_splitpath

/* _vsprintf: the host's vsprintf with VC5 LIBC's handling of the two
 * formats where hosts differ. VC5 has no "b" conversion: it drops the "%"
 * and any flags, width, precision and size before the "b", prints the "b"
 * as text and takes no argument. glibc prints the argument in binary and
 * the UCRT aborts. A "%" whose specification runs into the end of the
 * format prints nothing in VC5; glibc fails the whole call. Info text
 * reaches TextWindow's vsprintf with a "%b" for each image token (#951).
 * A "*" width before a "b" takes an int in VC5 but nothing here. */
inline int crt_vsprintf(char* buffer, const char* format, va_list args)
{
    std::string fixed;
    const char* copied = format;
    const char* p = format;
    const char* q;
    bool changed = false;

    while (*p != '\0') {
        if (*p != '%') {
            p++;
            continue;
        }
        q = p + 1;
        while (*q == '-' || *q == '+' || *q == ' ' || *q == '#' || *q == '0') q++;
        while (*q == '*' || isdigit((unsigned char)*q)) q++;
        if (*q == '.') {
            q++;
            while (*q == '*' || isdigit((unsigned char)*q)) q++;
        }
        if (q[0] == 'I' && q[1] == '6' && q[2] == '4') q += 3;
        else while (*q == 'h' || *q == 'l' || *q == 'L') q++;
        if (*q == 'b' || *q == '\0') {
            fixed.append(copied, p);
            if (*q == 'b') {
                fixed += 'b';
                q++;
            }
            copied = q;
            changed = true;
            p = q;
        } else {
            p = q + 1;
        }
    }
    if (!changed) return vsprintf(buffer, format, args);
    fixed.append(copied);
    return vsprintf(buffer, fixed.c_str(), args);
}
#define vsprintf crt_vsprintf

#endif /* LEGORRR_CRT_VC5NAMES_H */
