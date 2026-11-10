// SPDX-License-Identifier: LGPL-3.0-or-later
/* VC5 LIBC spellings for converted C units (ADR 0005). Converted units call
 * the routines by their LIBC names, as the original did. MSVC builds link
 * LIBC.LIB; other builds map the names onto the port's CRT layer (crt.hpp)
 * here, so the units themselves need no compiler conditionals.
 *
 * The native port compiles converted units as C++, so the port routines
 * keep their C++ linkage. */
#ifndef LEGORRR_CRT_VC5NAMES_H
#define LEGORRR_CRT_VC5NAMES_H

#include <stdlib.h>

#if !defined(_MSC_VER)
char* crt_signed_to_string(int value, char* out, int radix);
#define _itoa crt_signed_to_string
#endif

#endif /* LEGORRR_CRT_VC5NAMES_H */
