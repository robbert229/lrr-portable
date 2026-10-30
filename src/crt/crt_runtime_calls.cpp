// SPDX-License-Identifier: LGPL-3.0-only
#include <stdint.h>

#if defined(_MSC_VER) && defined(_M_IX86)
/* VC5 LIBC.LIB exports these compiler helpers with undecorated cdecl symbol
 * spellings. Keep these calls separate from the stdcall aliases in
 * crt_runtime_aliases.cpp so the source identifiers can have each ABI. */
extern "C" uint64_t __cdecl _allmul(uint32_t, uint32_t, uint32_t, uint32_t);
extern "C" uint64_t __cdecl _alldiv(uint32_t, uint32_t, uint32_t, uint32_t);
extern "C" uint64_t __cdecl _aulldiv(uint32_t, uint32_t, uint32_t, uint32_t);

extern "C" uint64_t __cdecl crt_runtime_allmul_call(
    uint32_t lo_a, uint32_t hi_a, uint32_t lo_b, uint32_t hi_b)
{
    return _allmul(lo_a, hi_a, lo_b, hi_b);
}

extern "C" uint64_t __cdecl crt_runtime_alldiv_call(
    uint32_t lo_a, uint32_t hi_a, uint32_t lo_b, uint32_t hi_b)
{
    return _alldiv(lo_a, hi_a, lo_b, hi_b);
}

extern "C" uint64_t __cdecl crt_runtime_aulldiv_call(
    uint32_t lo_a, uint32_t hi_a, uint32_t lo_b, uint32_t hi_b)
{
    return _aulldiv(lo_a, hi_a, lo_b, hi_b);
}
#endif
