// SPDX-License-Identifier: LGPL-3.0-or-later
/* VC5 LIBC spellings for converted C units (ADR 0005). Converted units call
 * the routines by their LIBC names, as the original did. MSVC builds link
 * LIBC.LIB; other builds map the names onto the port's CRT layer (crt.hpp,
 * crt_heap.cpp) here, so the units themselves need no compiler conditionals.
 *
 * The native port compiles converted units as C++, so the port routines
 * keep the linkage their own headers give them. Include this header after
 * the C library headers: it redefines malloc and free. */
#ifndef LEGORRR_CRT_VC5NAMES_H
#define LEGORRR_CRT_VC5NAMES_H

#include <stdlib.h>

#if !defined(_MSC_VER)
char* crt_signed_to_string(int value, char* out, int radix);
#define _itoa crt_signed_to_string

#ifdef __cplusplus
extern "C" {
#endif
/* The engine heap (FUN_0048dbd0 _malloc, FUN_0048db80 _free). */
void* engine_heap_alloc(long size);
void engine_heap_free(void* ptr);
/* __strcmpi (FUN_0048ff10) and __strnicmp (FUN_0049dab0). */
int strcasecmp_lite(const char* a, const char* b);
int strnicmp_lite(const char* a, const char* b, int count);
#ifdef __cplusplus
}
#endif

#define malloc(size) engine_heap_alloc((long)(size))
#define free(ptr) engine_heap_free(ptr)
#define _strcmpi strcasecmp_lite
#define _strnicmp(a, b, count) strnicmp_lite((a), (b), (int)(count))
#endif

#endif /* LEGORRR_CRT_VC5NAMES_H */
