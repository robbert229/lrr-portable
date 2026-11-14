// SPDX-License-Identifier: LGPL-3.0-or-later
/* Flic.c, original translation unit G98-016 (finding 632), rebuilt in
 * original shape (ADR 0005): gods98's player for the game's FLIC (.flh)
 * animations, text 00483c80-004850f0, strings 004ab10c-004ab17f.
 *
 * Flic_Setup opens a file into a FLICSTRUCT and a system-memory DirectDraw
 * surface of the flic's size, reading the whole file when FLICMEMORY is set.
 * Flic_Animate locks the surface, decodes the next frame into it
 * (Flic_Memory or Flic_Load, then Flic_FindChunk, Flic_FrameChunk and
 * Flic_DoChunk's decoders) and blits it to the back buffer.
 *
 * The native port has no DirectDraw: the unit compiles to nothing there,
 * and the port's player (flic.hpp) serves the callers. In the matching
 * build flic_link_names.h gives seven functions the port's names. */
#include "flic_link_names.h"

#if !defined(LEGORRR_BACKEND_MINIWIN)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"

#if defined(_MSC_VER) && _MSC_VER == 1100
#include <windows.h>
#endif

#include "images.h"
#include "flic_unit.h"

/* <stdlib.h>'s _splitpath buffer sizes, for compilers without them. */
#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256
#endif
#include "flic_imports.h"

#if defined(_WIN32)
#define FLIC_STDCALL __stdcall
#else
#define FLIC_STDCALL
#endif

/* Chunk types. */
#define FLI_CHUNK_COLOUR256 4
#define FLI_CHUNK_DELTA_FLC 7
#define FLI_CHUNK_COLOUR64 11
#define FLI_CHUNK_DELTA_FLI 12
#define FLI_CHUNK_BLACK 13
#define FLI_CHUNK_BYTE_RUN 15
#define FLI_CHUNK_FLI_COPY 16
#define FLI_CHUNK_DTA_BRUN 25
#define FLI_CHUNK_DTA_LC 27
#define FLI_CHUNK_LABEL 0x5344
#define FLI_CHUNK_SOUND 0x5555
#define FLI_PREFIX_TYPE 0x000b
#define FLI_POINTERS_TYPE 0x4b4c
#define FLI_FRAME_TYPE 0xf1fa

/* Header magics. */
#define FLI_MAGIC_FLI 0x1234
#define FLI_MAGIC_FLC 0x9119
#define FLI_MAGIC_DTA_BRUN 0xaf11
#define FLI_MAGIC_DTA_LC 0xaf12
#define FLI_MAGIC_DTA16 0xaf43

/* A chunk's header, as the chunk walk reads it (16 bytes for a frame). */
typedef struct Flic_ChunkHeader {
    int size;                       /* +0x00 */
    unsigned short type;            /* +0x04 */
    char chunks;                    /* +0x06 a frame's subchunk count */
} Flic_ChunkHeader;

/* A subchunk's header, as Flic_DoChunk reads it. */
typedef struct Flic_SubChunk {
    int size;                       /* +0x00 */
    short type;                     /* +0x04 */
} Flic_SubChunk;

/* DirectDraw's RECT, DDCOLORKEY, DDSURFACEDESC2 and DDBLTFX. */
typedef struct Flic_Rect {
    int left;
    int top;
    int right;
    int bottom;
} Flic_Rect;

typedef struct Flic_ColourKey {
    unsigned int low;
    unsigned int high;
} Flic_ColourKey;

typedef struct Flic_SurfaceDesc {
    unsigned int dwSize;            /* +0x00 */
    unsigned int dwFlags;           /* +0x04 */
    unsigned int dwHeight;          /* +0x08 */
    unsigned int dwWidth;           /* +0x0c */
    unsigned int lPitch;            /* +0x10 */
    unsigned int dwBackBufferCount; /* +0x14 */
    unsigned int dwRefreshRate;     /* +0x18 */
    unsigned int dwAlphaBitDepth;   /* +0x1c */
    unsigned int dwReserved;        /* +0x20 */
    void* lpSurface;                /* +0x24 */
    Flic_ColourKey ddckCKDestOverlay; /* +0x28 */
    Flic_ColourKey ddckCKDestBlt;   /* +0x30 */
    Flic_ColourKey ddckCKSrcOverlay; /* +0x38 */
    Flic_ColourKey ddckCKSrcBlt;    /* +0x40 */
    unsigned int pfSize;            /* +0x48 ddpfPixelFormat */
    unsigned int pfFlags;           /* +0x4c */
    unsigned int pfFourCC;          /* +0x50 */
    unsigned int pfRGBBitCount;     /* +0x54 */
    unsigned int pfRBitMask;        /* +0x58 */
    unsigned int pfGBitMask;        /* +0x5c */
    unsigned int pfBBitMask;        /* +0x60 */
    unsigned int pfRGBAlphaBitMask; /* +0x64 */
    unsigned int dwCaps;            /* +0x68 ddsCaps */
    unsigned int dwCaps2;           /* +0x6c */
    unsigned int dwCaps3;           /* +0x70 */
    unsigned int dwCaps4;           /* +0x74 */
    unsigned int dwTextureStage;    /* +0x78 */
} Flic_SurfaceDesc;

typedef struct Flic_BltFx {
    unsigned int dwSize;            /* +0x00 */
    unsigned int reserved[19];      /* +0x04 */
    unsigned int dwFillColor;       /* +0x50 */
    Flic_ColourKey ddckDestColorkey; /* +0x54 */
    Flic_ColourKey ddckSrcColorkey; /* +0x5c */
} Flic_BltFx;

#define DDSD_CAPS 0x00000001
#define DDSD_HEIGHT 0x00000002
#define DDSD_WIDTH 0x00000004
#define DDSCAPS_OFFSCREENPLAIN 0x00000040
#define DDSCAPS_SYSTEMMEMORY 0x00000800
#define DDCKEY_SRCBLT 0x00000008
#define DDLOCK_WAIT 0x00000001
#define DDBLT_KEYSRC 0x00008000
#define DDBLT_WAIT 0x01000000

/* IDirectDraw4 and IDirectDrawSurface4, the methods the unit calls. */
typedef struct Flic_DDrawIface Flic_DDrawIface;
typedef struct Flic_SurfaceIface Flic_SurfaceIface;

typedef struct Flic_DDrawVtbl {
    void* QueryInterface;
    void* AddRef;
    void* Release;
    void* Compact;
    void* CreateClipper;
    void* CreatePalette;
    long (FLIC_STDCALL* CreateSurface)(Flic_DDrawIface*, Flic_SurfaceDesc*,
                                       Flic_SurfaceIface**, void*);
} Flic_DDrawVtbl;

struct Flic_DDrawIface {
    const Flic_DDrawVtbl* lpVtbl;
};

typedef struct Flic_SurfaceVtbl {
    void* QueryInterface;
    void* AddRef;
    void* Release;
    void* AddAttachedSurface;
    void* AddOverlayDirtyRect;
    long (FLIC_STDCALL* Blt)(Flic_SurfaceIface*, Flic_Rect*,
                             Flic_SurfaceIface*, Flic_Rect*, unsigned int,
                             Flic_BltFx*);
    void* BltBatch;
    void* BltFast;
    void* DeleteAttachedSurface;
    void* EnumAttachedSurfaces;
    void* EnumOverlayZOrders;
    void* Flip;
    void* GetAttachedSurface;
    void* GetBltStatus;
    void* GetCaps;
    void* GetClipper;
    void* GetColorKey;
    void* GetDC;
    void* GetFlipStatus;
    void* GetOverlayPosition;
    void* GetPalette;
    void* GetPixelFormat;
    void* GetSurfaceDesc;
    void* Initialize;
    void* IsLost;
    long (FLIC_STDCALL* Lock)(Flic_SurfaceIface*, Flic_Rect*,
                              Flic_SurfaceDesc*, unsigned int, void*);
    void* ReleaseDC;
    void* Restore;
    void* SetClipper;
    long (FLIC_STDCALL* SetColorKey)(Flic_SurfaceIface*, unsigned int,
                                     Flic_ColourKey*);
    void* SetOverlayPosition;
    void* SetPalette;
    long (FLIC_STDCALL* Unlock)(Flic_SurfaceIface*, Flic_Rect*);
} Flic_SurfaceVtbl;

struct Flic_SurfaceIface {
    const Flic_SurfaceVtbl* lpVtbl;
};

#define lpDD() ((Flic_DDrawIface*)directDrawGlobs.lpDirectDraw)
#define DirectDraw_bSurf() ((Flic_SurfaceIface*)directDrawGlobs.bSurf)

/* The unit's functions have C linkage in the reference build's C++ compile
 * too. */
#ifdef __cplusplus
extern "C" {
#endif

// FUNCTION: LEGORR 0x00483c80
int Flic_Setup(const char* filename, LPFLICSTRUCT* fsp, unsigned int flags)
{
    Flic_DDrawIface* dd = lpDD();
    Flic_ColourKey colourKey;
    Flic_SurfaceDesc ddsd;
    char buff[128];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    unsigned int width;
    unsigned int height;

    *fsp = (LPFLICSTRUCT)malloc(sizeof(FLICSTRUCT));
    memset(*fsp, 0, sizeof(FLICSTRUCT));
    (*fsp)->fsDisplayMode = FLICMODE_HICOLOR;
    _splitpath(filename, drive, dir, fname, ext);

    if (!Flic_LoadHeader(filename, fsp)) return 0;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    width = (*fsp)->fsHeader.width;
    height = (*fsp)->fsHeader.height;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    (*fsp)->fsXsize = width;
    (*fsp)->fsYsize = height;
    dd->lpVtbl->CreateSurface(dd, &ddsd, &(*fsp)->fsSurface, NULL);

    colourKey.low = 0;
    colourKey.high = 0;
    (*fsp)->fsSurface->lpVtbl->SetColorKey((*fsp)->fsSurface, DDCKEY_SRCBLT,
                                           &colourKey);

    (*fsp)->userflags = flags;
    strcpy((*fsp)->filename, filename);
    (*fsp)->fsXc = 0;
    (*fsp)->fsYc = 0;
    (*fsp)->currentframe = 0;
    (*fsp)->overallframe = 0;
    (*fsp)->framerate = 25 << 16;

    if (((*fsp)->userflags & FLICMEMORY) == FLICMEMORY) {
        (*fsp)->pointerposition = 0;
        (*fsp)->rambufferhandle = (char*)malloc((*fsp)->fsHeader.size);
        (*fsp)->filehandle = file_open(filename, "rb");
        file_seek((*fsp)->filehandle, 128, SEEK_SET);
        file_read((*fsp)->rambufferhandle, (*fsp)->fsHeader.size, 1,
                  (*fsp)->filehandle);
        file_close((*fsp)->filehandle);
    } else {
        (*fsp)->pointerposition = 128;
        (*fsp)->filehandle = file_open(filename, "rb");
        if ((*fsp)->filehandle == NULL) {
            // STRING: LEGORR 0x004ab10c
            sprintf(buff, "Not Enough Memory > %s", filename);
            return 0;
        }
        file_seek((*fsp)->filehandle, 128, SEEK_SET);
    }

    (*fsp)->fsLoadBuffer = malloc(20000);

    return 1;
}

// FUNCTION: LEGORR 0x00483f00
int Flic_Close(LPFLICSTRUCT fsp)
{
    if (fsp == NULL) return 0;

    if ((fsp->userflags & FLICMEMORY) == FLICMEMORY) {
        free(fsp->rambufferhandle);
    } else {
        file_close(fsp->filehandle);
    }

    if (fsp->fsLoadBuffer) {
        free(fsp->fsLoadBuffer);
        fsp->fsLoadBuffer = NULL;
    }

    return 1;
}

// FUNCTION: LEGORR 0x00483f60
int Flic_LoadHeader(const char* filename, LPFLICSTRUCT* fsp)
{
    File* fp;
    char buff[128];

    if ((fp = file_open(filename, "rb")) == NULL) {
        // STRING: LEGORR 0x004ab150
        sprintf(buff, "Flic File Not Found %s", filename);
        return 0;
    }

    file_seek(fp, 0, SEEK_SET);
    file_read(&(*fsp)->fsHeader, sizeof(Flic_Header), 1, fp);

    if ((*fsp)->fsHeader.size != file_length(fp)) {
        file_close(fp);
        // STRING: LEGORR 0x004ab13c
        sprintf(buff, "Flic File Invalid", filename);
        return 0;
    }

    switch ((*fsp)->fsHeader.magic) {
    case FLI_MAGIC_FLI:
        file_close(fp);
        return 1;
    case FLI_MAGIC_FLC:
        file_close(fp);
        return 1;
    case FLI_MAGIC_DTA_BRUN:
        file_close(fp);
        return 1;
    case FLI_MAGIC_DTA_LC:
        file_close(fp);
        return 1;
    case FLI_MAGIC_DTA16:
        file_close(fp);
        return 1;
    }

    file_close(fp);
    // STRING: LEGORR 0x004ab124
    sprintf(buff, "Flic File Invalid %s", filename);
    return 0;
}

// FUNCTION: LEGORR 0x00484070
int Flic_Animate(LPFLICSTRUCT fsp, const struct Area2F* destArea, int advance,
                 int trans)
{
    Flic_SurfaceIface* bSurf = DirectDraw_bSurf();
    Flic_Rect dest;
    Flic_SurfaceDesc ddsd;
    Flic_BltFx bltFX;
    int ok;

    dest.left = (int)destArea->x;
    dest.top = (int)destArea->y;
    dest.right = dest.left + (int)destArea->width;
    dest.bottom = dest.top + (int)destArea->height;

    ok = 0;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    fsp->fsSurface->lpVtbl->Lock(fsp->fsSurface, NULL, &ddsd, DDLOCK_WAIT,
                                 NULL);
    fsp->fsSPtr = ddsd.lpSurface;
    fsp->fsPitch = ddsd.lPitch;
    fsp->is15bit = (ddsd.pfGBitMask == 0x3e0);

    if (fsp->currentframe == 0) advance = 1;
    if (advance) {
        if ((fsp->userflags & FLICMEMORY) == FLICMEMORY) ok = Flic_Memory(fsp);
        else ok = Flic_Load(fsp);
    }

    fsp->fsSurface->lpVtbl->Unlock(fsp->fsSurface, NULL);

    memset(&bltFX, 0, sizeof(bltFX));
    bltFX.dwSize = sizeof(bltFX);
    bltFX.dwFillColor = 0xff00;
    bSurf->lpVtbl->Blt(bSurf, &dest, fsp->fsSurface, NULL,
                       (trans ? DDBLT_KEYSRC : 0) | DDBLT_WAIT, &bltFX);

    return !ok;
}

// FUNCTION: LEGORR 0x004841d0
int Flic_Memory(LPFLICSTRUCT fsp)
{
    int retval = 0;

    Flic_FindChunk(fsp);
    if (fsp->currentframe == 0) fsp->ringframe = fsp->pointerposition;
    fsp->currentframe++;
    fsp->overallframe++;

    if ((fsp->userflags & FLICLOOPINGON) == FLICLOOPINGON) {
        if (fsp->currentframe == fsp->fsHeader.frames + 1) {
            fsp->currentframe = 1;
            fsp->pointerposition = fsp->ringframe;
        }
    } else {
        if (fsp->currentframe == fsp->fsHeader.frames + 1) retval = 1;
    }

    return retval;
}

// FUNCTION: LEGORR 0x00484260
int Flic_Load(LPFLICSTRUCT fsp)
{
    int retval = 0;
    void* buffer = fsp->fsLoadBuffer;

    if (fsp->currentframe == 0) fsp->ringframe = fsp->pointerposition;
    file_read(buffer, 16, 1, fsp->filehandle);
    Flic_FindChunk(fsp);
    fsp->currentframe++;
    fsp->overallframe++;

    if ((fsp->userflags & FLICLOOPINGON) == FLICLOOPINGON) {
        if (fsp->currentframe > fsp->fsHeader.frames) {
            file_seek(fsp->filehandle, fsp->ringframe, SEEK_SET);
            fsp->pointerposition = fsp->ringframe;
            fsp->currentframe = 0;
        }
    } else {
        if (fsp->currentframe > fsp->fsHeader.frames) retval = 1;
    }

    return retval;
}

// FUNCTION: LEGORR 0x00484320
int Flic_FindChunk(LPFLICSTRUCT fsp)
{
    Flic_ChunkHeader* header;
    int size;
    int start;
    int found;

    if ((fsp->userflags & FLICMEMORY) == FLICMEMORY) {
        header = (Flic_ChunkHeader*)fsp->rambufferhandle;
    } else {
        header = (Flic_ChunkHeader*)fsp->fsLoadBuffer;
    }
    start = fsp->pointerposition;

    if ((fsp->userflags & FLICMEMORY) == FLICMEMORY) {
        size = *(int*)((char*)header + start);
        header = (Flic_ChunkHeader*)((char*)header + start);
        found = 0;
        while (!found) {
            if (fsp->pointerposition >= fsp->fsHeader.size) break;
            switch (header->type) {
            case FLI_PREFIX_TYPE:
                Flic_LoadPalette64(fsp);
                fsp->pointerposition += header->size;
                break;
            case FLI_POINTERS_TYPE:
                Flic_LoadPointers(fsp);
                fsp->pointerposition += header->size;
                break;
            case FLI_FRAME_TYPE:
                Flic_FrameChunk(fsp);
                found = 1;
                break;
            default:
                fsp->pointerposition += header->size;
                break;
            }
            header = (Flic_ChunkHeader*)((char*)header + header->size);
            size += header->size;
            if (found) {
                fsp->pointerposition = start + size;
                return 0;
            }
        }
    } else {
        size = header->size;
        found = 0;
        while (!found) {
            if (fsp->pointerposition >= fsp->fsHeader.size) break;
            switch (header->type) {
            case FLI_PREFIX_TYPE:
                Flic_LoadPalette64(fsp);
                fsp->pointerposition += header->size;
                file_seek(fsp->filehandle, fsp->pointerposition, SEEK_SET);
                header = (Flic_ChunkHeader*)fsp->fsLoadBuffer;
                file_read(header, 16, 1, fsp->filehandle);
                size += header->size;
                break;
            case FLI_POINTERS_TYPE:
                Flic_LoadPointers(fsp);
                fsp->pointerposition += header->size;
                file_seek(fsp->filehandle, fsp->pointerposition, SEEK_SET);
                header = (Flic_ChunkHeader*)fsp->fsLoadBuffer;
                file_read(header, 16, 1, fsp->filehandle);
                size += header->size;
                break;
            case FLI_FRAME_TYPE:
                Flic_FrameChunk(fsp);
                found = 1;
                break;
            default:
                fsp->pointerposition += header->size;
                file_seek(fsp->filehandle, fsp->pointerposition, SEEK_SET);
                header = (Flic_ChunkHeader*)fsp->fsLoadBuffer;
                file_read(header, 16, 1, fsp->filehandle);
                size += header->size;
                break;
            }
        }
    }

    fsp->pointerposition = start + size;
    return 0;
}

// FUNCTION: LEGORR 0x004844b0
int Flic_FrameChunk(LPFLICSTRUCT fsp)
{
    Flic_ChunkHeader* header;
    int* chunk;
    char subchunks;
    int size;
    char buff[128];

    if ((fsp->userflags & FLICMEMORY) == FLICMEMORY) {
        header = (Flic_ChunkHeader*)(fsp->rambufferhandle + fsp->pointerposition);
        subchunks = header->chunks;
        chunk = (int*)((char*)header + 16);
        fsp->pointerposition += 16;
        while (subchunks > 0 && fsp->pointerposition < fsp->fsHeader.size) {
            fsp->fsSource = chunk;
            Flic_DoChunk(fsp);
            fsp->pointerposition += *chunk;
            chunk = (int*)((char*)chunk + *chunk);
            subchunks--;
        }
    } else {
        chunk = (int*)fsp->fsLoadBuffer;
        subchunks = ((Flic_ChunkHeader*)chunk)->chunks;
        size = *chunk - 16;
        if (size > fsp->fsLoadBufferSize) {
            if (chunk) free(chunk);
            fsp->fsLoadBufferSize = size;
            fsp->fsLoadBuffer = malloc(size);
            if (fsp->fsLoadBuffer == NULL) {
                // STRING: LEGORR 0x004ab168
                sprintf(buff, "Flic Not Enough Memory");
                return 0;
            }
            chunk = (int*)fsp->fsLoadBuffer;
        }
        file_read(chunk, size, 1, fsp->filehandle);
        fsp->pointerposition += 16;
        while (subchunks > 0 && fsp->pointerposition < fsp->fsHeader.size) {
            fsp->fsSource = chunk;
            Flic_DoChunk(fsp);
            fsp->pointerposition += *chunk;
            chunk = (int*)((char*)chunk + *chunk);
            subchunks--;
        }
    }

    return 1;
}

// FUNCTION: LEGORR 0x00484610
int Flic_DoChunk(LPFLICSTRUCT fsp)
{
    int retval = 0;

    switch (((Flic_SubChunk*)fsp->fsSource)->type) {
    case FLI_CHUNK_COLOUR256:
        Flic_Colour256(fsp);
        retval = 1;
        break;
    case FLI_CHUNK_COLOUR64:
        Flic_Unsupported(fsp);
        retval = 1;
        break;
    case FLI_CHUNK_DELTA_FLI:
        Flic_Unsupported(fsp);
        retval = 1;
        break;
    case FLI_CHUNK_BLACK:
        Flic_Unsupported(fsp);
        retval = 1;
        break;
    case FLI_CHUNK_BYTE_RUN:
    case FLI_CHUNK_DTA_BRUN:
        Flic_Brun(fsp);
        retval = 1;
        break;
    case FLI_CHUNK_FLI_COPY:
        Flic_Copy(fsp);
        retval = 1;
        break;
    case FLI_CHUNK_DELTA_FLC:
    case FLI_CHUNK_DTA_LC:
        Flic_Delta(fsp);
        retval = 1;
        break;
    case FLI_CHUNK_LABEL:
        retval = 1;
        break;
    case FLI_CHUNK_SOUND:
        Flic_Sound(fsp);
        retval = 1;
        break;
    }

    return retval;
}

// FUNCTION: LEGORR 0x00484720
int Flic_LoadPointers(LPFLICSTRUCT fsp)
{
    int* chunk;
    int size;
    char buff[256];

    if ((fsp->userflags & FLICMEMORY) != FLICMEMORY) {
        chunk = (int*)fsp->fsLoadBuffer;
        size = *chunk;
        if (size > fsp->fsLoadBufferSize) {
            if (chunk) free(chunk);
            fsp->fsLoadBufferSize = size;
            fsp->fsLoadBuffer = malloc(size);
            if (fsp->fsLoadBuffer == NULL) {
                sprintf(buff, "Flic Not Enough Memory");
                return 0;
            }
            chunk = (int*)fsp->fsLoadBuffer;
        }
        file_seek(fsp->filehandle, fsp->pointerposition, SEEK_SET);
        file_read(chunk, size, 1, fsp->filehandle);
    }

    return 0;
}

// FUNCTION: LEGORR 0x004847d0
int Flic_LoadPalette64(LPFLICSTRUCT fsp)
{
    int* chunk;
    int size;
    char buff[128];

    if ((fsp->userflags & FLICMEMORY) != FLICMEMORY) {
        chunk = (int*)fsp->fsLoadBuffer;
        size = *chunk;
        if (size > fsp->fsLoadBufferSize) {
            if (chunk) free(chunk);
            fsp->fsLoadBufferSize = size;
            fsp->fsLoadBuffer = malloc(size);
            if (fsp->fsLoadBuffer == NULL) {
                sprintf(buff, "Flic Not Enough Memory");
                return 1;
            }
            chunk = (int*)fsp->fsLoadBuffer;
        }
        file_seek(fsp->filehandle, fsp->pointerposition, SEEK_SET);
        file_read(chunk, size, 1, fsp->filehandle);
    }

    return 0;
}

// FUNCTION: LEGORR 0x00484880
int Flic_Copy(LPFLICSTRUCT fsp)
{
    if (fsp->fsBitPlanes == 16) {
        Flic_Nothing(fsp);
        return 1;
    }
    if (fsp->fsDisplayMode == FLICMODE_BYTEPERPIXEL) Flic_Nothing(fsp);
    if (fsp->fsDisplayMode == FLICMODE_HICOLOR) Flic_Nothing(fsp);
    return 1;
}

// FUNCTION: LEGORR 0x004848d0
int Flic_BrunDepack(LPFLICSTRUCT fsp)
{
    unsigned char* src = (unsigned char*)fsp->fsSource + 7;
    unsigned short* dst;
    unsigned short width;
    unsigned short count;
    unsigned char colour;
    unsigned short line;
    unsigned short height;

    height = (unsigned short)fsp->fsYsize;
    for (line = 0; line < height; line++, src++) {
        width = (unsigned short)fsp->fsXsize;
        dst = (unsigned short*)fsp->fsSPtr + (fsp->fsPitch >> 1) * line;
        while (width > 0) {
            count = *src++;
            if (count < 0x80) {
                width -= count;
                colour = *src++;
                while (count) {
                    *dst++ = Flic_GetPaletteColour(colour, fsp);
                    count--;
                }
            } else {
                count = 0x100 - count;
                width -= count;
                while (count) {
                    colour = *src++;
                    *dst++ = Flic_GetPaletteColour(colour, fsp);
                    count--;
                }
            }
        }
    }

    return 1;
}

// FUNCTION: LEGORR 0x004849d0
int Flic_BrunDepack16(LPFLICSTRUCT fsp)
{
    unsigned char* src = (unsigned char*)fsp->fsSource + 7;
    unsigned short* dst;
    short width;
    unsigned short count;
    unsigned short colour;
    unsigned short line = 0;
    int height = (short)fsp->fsYsize;

    while (line < height) {
        width = (short)fsp->fsXsize;
        dst = (unsigned short*)fsp->fsSPtr + (fsp->fsPitch >> 1) * line;
        while (width > 0) {
            count = *src++;
            if (count < 0x80) {
                width -= count;
                colour = *(unsigned short*)src;
                src += 2;
                colour = ((colour & 0xffe0) << 1) | (colour & 0x1f);
                while (count) {
                    *dst++ = fsp->is15bit ? ((colour >> 1) & 0x7ff0) | (colour & 0x1f) : colour;
                    count--;
                }
            } else {
                count = 0x100 - count;
                width -= count;
                while (count) {
                    colour = *(unsigned short*)src;
                    src += 2;
                    colour = ((colour & 0xffe0) << 1) | (colour & 0x1f);
                    *dst++ = fsp->is15bit ? ((colour >> 1) & 0x7ff0) | (colour & 0x1f) : colour;
                    count--;
                }
            }
        }
        src++;
        line++;
    }

    return 1;
}

// FUNCTION: LEGORR 0x00484b20
int Flic_Brun(LPFLICSTRUCT fsp)
{
    if (fsp->fsHeader.depth == 8) {
        if (fsp->fsDisplayMode == FLICMODE_BYTEPERPIXEL) Flic_Unsupported(fsp);
        if (fsp->fsDisplayMode == FLICMODE_HICOLOR) {
            Flic_BrunDepack(fsp);
            return 1;
        }
    } else if (fsp->fsHeader.depth == 16) {
        if (fsp->fsHeader.magic == FLI_MAGIC_DTA16) {
            Flic_BrunDepack16(fsp);
            return 1;
        }
        Flic_Unsupported(fsp);
    }
    return 1;
}

// FUNCTION: LEGORR 0x00484b90
int Flic_Unsupported(LPFLICSTRUCT fsp)
{
    return 1;
}

// FUNCTION: LEGORR 0x00484ba0
void Flic_Palette256To64k(LPFLICSTRUCT fsp)
{
    unsigned char* src = fsp->fsPalette256;
    unsigned short* dst = fsp->fsPalette64k;
    int i;

    for (i = 0; i < 256; i++) {
        *dst++ = (((unsigned short)(src[0] & 0x3e) << 5 | src[1]) << 5) |
                 (unsigned short)(src[2] >> 1);
        src += 3;
    }
    fsp->fsPalette64k[0] = 0;
}

// FUNCTION: LEGORR 0x00484c00
int Flic_Colour256(LPFLICSTRUCT fsp)
{
    unsigned char* dst = fsp->fsPalette256;
    unsigned char* src = (unsigned char*)fsp->fsSource + 6;
    unsigned short packets;
    unsigned short skip;
    unsigned short count;
    unsigned char colour;

    packets = *(unsigned short*)src;
    src += 2;
    while (packets) {
        skip = *src++;
        dst += skip * 3;
        count = *src++;
        if (count == 0) {
            packets = 1;
            count = 256;
            dst = fsp->fsPalette256;
        }
        count *= 3;
        while (count) {
            colour = *src++;
            *dst++ = colour >> 2;
            count--;
        }
        packets--;
    }

    if (fsp->fsDisplayMode == FLICMODE_HICOLOR) Flic_Palette256To64k(fsp);

    return 1;
}

// FUNCTION: LEGORR 0x00484c90
void Flic_Nothing(LPFLICSTRUCT fsp)
{
}

// FUNCTION: LEGORR 0x00484ca0
void Flic_DeltaWord(LPFLICSTRUCT fsp)
{
    unsigned char* src = (unsigned char*)fsp->fsSource + 6;
    unsigned short* dst;
    unsigned short line = 0;
    unsigned short lines;
    unsigned short word;
    unsigned short type;
    unsigned short packets;
    unsigned short skip;
    unsigned short count;
    unsigned char lo;
    unsigned char hi;
    unsigned char last;
    unsigned short writeLast;

    lines = *(unsigned short*)src;
    src += 2;
    while (lines) {
        dst = (unsigned short*)((unsigned char*)fsp->fsSPtr + line * fsp->fsPitch);
        word = *(unsigned short*)src;
        type = (word & 0xc000) >> 12;
        if (type == 0xc) {
            src += 2;
            line -= word;
            continue;
        }
        if (type == 0x4) return;
        writeLast = 0;
        if (type == 0x8) {
            last = *src;
            src += 2;
            writeLast = 1;
        }
        packets = *(unsigned short*)src;
        src += 2;
        while (packets) {
            skip = *src++;
            count = *src++;
            dst += skip;
            if (count & 0x80) {
                count = 0x100 - count;
                lo = *src++;
                hi = *src++;
                while (count) {
                    *dst++ = Flic_GetPaletteColour(lo, fsp);
                    *dst++ = Flic_GetPaletteColour(hi, fsp);
                    count--;
                }
            } else {
                while (count) {
                    lo = *src++;
                    hi = *src++;
                    *dst++ = Flic_GetPaletteColour(lo, fsp);
                    *dst++ = Flic_GetPaletteColour(hi, fsp);
                    count--;
                }
            }
            packets--;
        }
        if (writeLast) *dst = Flic_GetPaletteColour(last, fsp);
        lines--;
        line++;
    }
}

// FUNCTION: LEGORR 0x00484e50
void Flic_DeltaWord16(LPFLICSTRUCT fsp)
{
    unsigned char* src = (unsigned char*)fsp->fsSource + 6;
    unsigned short* dst;
    unsigned short line = 0;
    unsigned short lines;
    unsigned short word;
    unsigned short type;
    unsigned short packets;
    unsigned short count;
    unsigned short colour;
    unsigned short last;
    unsigned short writeLast;

    lines = *(unsigned short*)src;
    src += 2;
    while (lines) {
        dst = (unsigned short*)((unsigned char*)fsp->fsSPtr + line * fsp->fsPitch);
        word = *(unsigned short*)src;
        type = (word & 0xc000) >> 12;
        if (type == 0xc) {
            src += 2;
            line -= word;
            continue;
        }
        if (type == 0x4) return;
        writeLast = 0;
        if (type == 0x8) {
            last = fsp->is15bit ? ((word >> 1) & 0x7ff0) | (word & 0x1f) : word;
            src += 2;
            writeLast = 1;
        }
        packets = *(unsigned short*)src;
        src += 2;
        while (packets) {
            dst += *src++;
            count = *src++;
            if (count & 0x80) {
                colour = *(unsigned short*)src;
                src += 2;
                colour = ((colour & 0xffe0) << 1) | (colour & 0x1f);
                count = 0x100 - count;
                while (count) {
                    *dst++ = fsp->is15bit ? ((colour >> 1) & 0x7ff0) | (colour & 0x1f) : colour;
                    count--;
                }
            } else {
                while (count) {
                    colour = *(unsigned short*)src;
                    src += 2;
                    colour = ((colour & 0xffe0) << 1) | (colour & 0x1f);
                    *dst++ = fsp->is15bit ? ((colour >> 1) & 0x7ff0) | (colour & 0x1f) : colour;
                    count--;
                }
            }
            packets--;
        }
        if (writeLast) *(unsigned char*)dst = (unsigned char)last;
        line++;
        lines--;
    }
}

// FUNCTION: LEGORR 0x00485030
int Flic_Delta(LPFLICSTRUCT fsp)
{
    if (fsp->fsHeader.depth == 8) {
        if (fsp->fsDisplayMode == FLICMODE_BYTEPERPIXEL) Flic_Nothing(fsp);
        if (fsp->fsDisplayMode == FLICMODE_HICOLOR) {
            Flic_DeltaWord(fsp);
            return 1;
        }
    } else if (fsp->fsHeader.depth == 16) {
        if (fsp->fsHeader.magic == FLI_MAGIC_FLI) {
            Flic_Nothing(fsp);
            return 1;
        }
        if (fsp->fsHeader.magic == FLI_MAGIC_DTA16) {
            Flic_DeltaWord16(fsp);
            return 1;
        }
        Flic_Nothing(fsp);
        return 1;
    }
    return 1;
}

// FUNCTION: LEGORR 0x004850c0
unsigned short Flic_GetPaletteColour(unsigned char index, LPFLICSTRUCT fsp)
{
    return fsp->fsPalette64k[index];
}

// FUNCTION: LEGORR 0x004850e0
int Flic_GetHeight(LPFLICSTRUCT fsp)
{
    return fsp->fsYsize;
}

#ifdef __cplusplus
}
#endif

#endif /* !LEGORRR_BACKEND_MINIWIN */
