// SPDX-License-Identifier: LGPL-3.0-only
#include <stdint.h>

#if defined(_MSC_VER) && defined(_M_IX86)
/* The VC5 compiler emits stdcall references with @16 suffixes for 64-bit
 * arithmetic, while LIBC.LIB defines undecorated cdecl names. These aliases
 * call the cdecl bridges and clean the original helper arguments. */
extern "C" uint64_t __cdecl crt_runtime_allmul_call(
    uint32_t, uint32_t, uint32_t, uint32_t);
extern "C" uint64_t __cdecl crt_runtime_alldiv_call(
    uint32_t, uint32_t, uint32_t, uint32_t);
extern "C" uint64_t __cdecl crt_runtime_aulldiv_call(
    uint32_t, uint32_t, uint32_t, uint32_t);

extern "C" __declspec(naked) uint64_t __stdcall _allmul(
    uint32_t, uint32_t, uint32_t, uint32_t)
{
    __asm {
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        call crt_runtime_allmul_call
        add esp, 10h
        ret 10h
    }
}

extern "C" __declspec(naked) uint64_t __stdcall _alldiv(
    uint32_t, uint32_t, uint32_t, uint32_t)
{
    __asm {
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        call crt_runtime_alldiv_call
        add esp, 10h
        ret 10h
    }
}

extern "C" __declspec(naked) uint64_t __stdcall _aulldiv(
    uint32_t, uint32_t, uint32_t, uint32_t)
{
    __asm {
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        push dword ptr [esp + 10h]
        call crt_runtime_aulldiv_call
        add esp, 10h
        ret 10h
    }
}
#endif
