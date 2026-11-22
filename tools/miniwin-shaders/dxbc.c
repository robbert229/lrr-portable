/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Compiles HLSL to DXBC with vkd3d-utils' D3DCompile, for the Direct3D 11
 * variant of miniwin's SDL GPU shaders. gencshadersource.py's `dxbc` step
 * calls D3DCompile through Python's ctypes, which cannot call vkd3d-utils'
 * ms_abi functions on x86-64 Linux.
 *
 *   dxbc <vs_5_0|ps_5_0> <in.hlsl> <out.dxbc> */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__x86_64__) && !defined(_WIN32)
#define WINAPI __attribute__((ms_abi))
#else
#define WINAPI
#endif

typedef struct Blob Blob;
typedef struct {
    long(WINAPI* QueryInterface)(Blob*, const void*, void**);
    unsigned long(WINAPI* AddRef)(Blob*);
    unsigned long(WINAPI* Release)(Blob*);
    void*(WINAPI* GetBufferPointer)(Blob*);
    size_t(WINAPI* GetBufferSize)(Blob*);
} BlobVtbl;
struct Blob {
    const BlobVtbl* vtbl;
};

long WINAPI D3DCompile(const void* source, size_t size, const char* name, const void* defines, void* include,
                       const char* entry, const char* target, unsigned flags1, unsigned flags2, Blob** code,
                       Blob** errors);

int main(int argc, char** argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: dxbc <vs_5_0|ps_5_0> <in.hlsl> <out.dxbc>\n");
        return 2;
    }
    FILE* in = fopen(argv[2], "rb");
    if (!in) {
        perror(argv[2]);
        return 1;
    }
    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, 0, SEEK_SET);
    char* source = malloc(size + 1);
    if (!source || fread(source, 1, size, in) != (size_t) size) {
        fclose(in);
        return 1;
    }
    source[size] = '\0';
    fclose(in);

    Blob* code = NULL;
    Blob* errors = NULL;
    long hr = D3DCompile(source, size, argv[2], NULL, NULL, "main", argv[1], 0, 0, &code, &errors);
    if (errors) {
        fwrite(errors->vtbl->GetBufferPointer(errors), 1, errors->vtbl->GetBufferSize(errors), stderr);
        errors->vtbl->Release(errors);
    }
    if (hr < 0 || !code) {
        fprintf(stderr, "D3DCompile failed: 0x%08lx\n", (unsigned long) hr & 0xffffffffUL);
        return 1;
    }
    FILE* out = fopen(argv[3], "wb");
    if (!out) {
        perror(argv[3]);
        return 1;
    }
    fwrite(code->vtbl->GetBufferPointer(code), 1, code->vtbl->GetBufferSize(code), out);
    fclose(out);
    code->vtbl->Release(code);
    free(source);
    return 0;
}
