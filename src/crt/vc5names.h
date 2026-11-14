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
/* The original's callers use the old names of the late LIBC members
 * (strnicmp, itoa, strlwr, strupr), which oldnames.lib resolves; that is why
 * those members follow the import thunks (issue #447, finding 643). */
#define itoa crt_signed_to_string
/* _rand (FUN_0048e170): the port's VC5 LCG. */
int level_random_draw15(void);
#define rand level_random_draw15

#ifdef __cplusplus
extern "C" {
#endif
/* The engine heap (FUN_0048dbd0 _malloc, FUN_0048db80 _free). */
void* engine_heap_alloc(long size);
void engine_heap_free(void* ptr);
/* __strcmpi (FUN_0048ff10) and __strnicmp (FUN_0049dab0). */
int strcasecmp_lite(const char* a, const char* b);
int strnicmp_lite(const char* a, const char* b, int count);
/* __finite (FUN_0048e870). */
int double_is_finite(double value);
/* __splitpath (FUN_00490090): the port's model, in gods98/lwo_port.cpp. */
void crt_path_split(const char* path, char* drive, char* dir, char* fname,
                    char* ext);
#ifdef __cplusplus
}
#endif

#define malloc(size) engine_heap_alloc((long)(size))
#define free(ptr) engine_heap_free(ptr)
#ifdef __cplusplus
/* _realloc (FUN_0048dc80): the port's model in crt.cpp, over the same heap
 * as engine_heap_alloc. Other compilers build the units as C++. */
namespace unported_ledger_stubs {
void* ledger_stub_0048dc80_GrowHeapArray(void* allocation,
                                         unsigned int requested_size);
}
#define realloc(ptr, size) \
    unported_ledger_stubs::ledger_stub_0048dc80_GrowHeapArray( \
        (ptr), (unsigned int)(size))
/* __strlwr (FUN_0049dc10): the port's model in crt.cpp (crt.hpp). */
char* crt_string_to_lower(char* text);
#define _strlwr crt_string_to_lower
#define strlwr crt_string_to_lower
#endif
#define _strcmpi strcasecmp_lite
/* The original's callers spell __strcmpi by its old name too, except
 * Files.c's direct calls, so LIBC pulls stricmp.obj among Files.c's
 * references (finding 667). */
#define stricmp strcasecmp_lite
#define _strnicmp(a, b, count) strnicmp_lite((a), (b), (int)(count))
#define strnicmp(a, b, count) strnicmp_lite((a), (b), (int)(count))
#define _finite double_is_finite
#define _splitpath crt_path_split
#endif

#endif /* LEGORRR_CRT_VC5NAMES_H */
