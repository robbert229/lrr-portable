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
 * The unit draws through miniwin's DirectDraw (ADR 0007). */
#include "flic_link_names.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"

#include <miniwin/windows.h>

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

/* The stream's chunks start wherever the previous one ended, so their
 * fields and the pixel words inside them need not be aligned: read them
 * with memcpy (the stream is little-endian, like every host the port
 * builds for). */
static inline unsigned short flic_read_u16(const void* at)
{
    unsigned short value;
    memcpy(&value, at, sizeof(value));
    return value;
}

static inline int flic_read_i32(const void* at)
{
    int value;
    memcpy(&value, at, sizeof(value));
    return value;
}

/* A chunk header's size (+0x00), type (+0x04) and subchunk count (+0x06). */
#define FLIC_CHUNK_SIZE(chunk) flic_read_i32(chunk)
#define FLIC_CHUNK_TYPE(chunk) flic_read_u16((const char*)(chunk) + 4)
#define FLIC_CHUNK_COUNT(chunk) (((const char*)(chunk))[6])

#include <miniwin/ddraw.h>

#define lpDD() ((IDirectDraw4*)directDrawGlobs.lpDirectDraw)
#define DirectDraw_bSurf() ((IDirectDrawSurface4*)directDrawGlobs.bSurf)

/* The unit's functions have C linkage in the reference build's C++ compile
 * too. */
#ifdef __cplusplus
extern "C" {
#endif

// FUNCTION: LEGORR 0x00483c80
int Flic_Setup(const char* filename, LPFLICSTRUCT* fsp, unsigned int flags)
{
    IDirectDraw4* dd = lpDD();
    DDCOLORKEY colourKey;
    DDSURFACEDESC2 ddsd;
    char buff[128];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    unsigned int width;
    unsigned int height;

    /* PORT-ONLY: a failed open leaves *fsp NULL. The original returned 0 with
     * *fsp still the half-built, zeroed FLICSTRUCT, which its callers
     * (FrontEnd.c's overlays, Reward.c's score flics, Pointer.c's cursors)
     * then held and animated as if it were open: a missing .flh made the
     * next Flic_Animate call through the struct's null surface (#983). */
    *fsp = (LPFLICSTRUCT)malloc(sizeof(FLICSTRUCT));
    if (*fsp == NULL) return 0;
    memset(*fsp, 0, sizeof(FLICSTRUCT));
    (*fsp)->fsDisplayMode = FLICMODE_HICOLOR;
    _splitpath(filename, drive, dir, fname, ext);

    if (!Flic_LoadHeader(filename, fsp)) goto fail;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    width = (*fsp)->fsHeader.width;
    height = (*fsp)->fsHeader.height;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    (*fsp)->fsXsize = width;
    (*fsp)->fsYsize = height;
    dd->CreateSurface(&ddsd, &(*fsp)->fsSurface, NULL);
    if ((*fsp)->fsSurface == NULL) goto fail;

    colourKey.dwColorSpaceLowValue = 0;
    colourKey.dwColorSpaceHighValue = 0;
    (*fsp)->fsSurface->SetColorKey(DDCKEY_SRCBLT,
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
        if ((*fsp)->rambufferhandle == NULL || (*fsp)->filehandle == NULL) {
            if ((*fsp)->filehandle != NULL) file_close((*fsp)->filehandle);
            (*fsp)->filehandle = NULL;
            goto fail;
        }
        file_seek((*fsp)->filehandle, 128, SEEK_SET);
        file_read((*fsp)->rambufferhandle, (*fsp)->fsHeader.size, 1,
                  (*fsp)->filehandle);
        file_close((*fsp)->filehandle);
    } else {
        (*fsp)->pointerposition = 128;
        (*fsp)->filehandle = file_open(filename, "rb");
        if ((*fsp)->filehandle == NULL) {
            // STRING: LEGORR 0x004ab10c
            snprintf(buff, sizeof(buff), "Not Enough Memory > %s", filename);
            goto fail;
        }
        file_seek((*fsp)->filehandle, 128, SEEK_SET);
    }

    (*fsp)->fsLoadBuffer = malloc(20000);

    return 1;

fail:
    /* PORT-ONLY: release what was built so far, so that the caller's pointer
     * is NULL (safe to test, draw and close) rather than a struct with no
     * surface. */
    if ((*fsp)->fsSurface != NULL) (*fsp)->fsSurface->Release();
    free((*fsp)->rambufferhandle);
    free(*fsp);
    *fsp = NULL;
    return 0;
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
        snprintf(buff, sizeof(buff), "Flic File Not Found %s", filename);
        return 0;
    }

    file_seek(fp, 0, SEEK_SET);
    file_read(&(*fsp)->fsHeader, sizeof(Flic_Header), 1, fp);

    if ((*fsp)->fsHeader.size != file_length(fp)) {
        file_close(fp);
        // STRING: LEGORR 0x004ab13c
        snprintf(buff, sizeof(buff), "Flic File Invalid", filename);
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
    snprintf(buff, sizeof(buff), "Flic File Invalid %s", filename);
    return 0;
}

// FUNCTION: LEGORR 0x00484070
int Flic_Animate(LPFLICSTRUCT fsp, const struct Area2F* destArea, int advance,
                 int trans)
{
    IDirectDrawSurface4* bSurf = DirectDraw_bSurf();
    RECT dest;
    DDSURFACEDESC2 ddsd;
    DDBLTFX bltFX;
    int ok;

    dest.left = (int)destArea->x;
    dest.top = (int)destArea->y;
    dest.right = dest.left + (int)destArea->width;
    dest.bottom = dest.top + (int)destArea->height;

    ok = 0;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    fsp->fsSurface->Lock(NULL, &ddsd, DDLOCK_WAIT,
                                 NULL);
    fsp->fsSPtr = ddsd.lpSurface;
    fsp->fsPitch = ddsd.lPitch;
    fsp->is15bit = (ddsd.ddpfPixelFormat.dwGBitMask == 0x3e0);

    if (fsp->currentframe == 0) advance = 1;
    if (advance) {
        if ((fsp->userflags & FLICMEMORY) == FLICMEMORY) ok = Flic_Memory(fsp);
        else ok = Flic_Load(fsp);
    }

    fsp->fsSurface->Unlock(NULL);

    memset(&bltFX, 0, sizeof(bltFX));
    bltFX.dwSize = sizeof(bltFX);
    bltFX.dwFillColor = 0xff00;
    bSurf->Blt(&dest, fsp->fsSurface, NULL,
                       (trans ? DDBLT_KEYSRC : (DDBltFlags)0) | DDBLT_WAIT, &bltFX);

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
        /* Flic_Setup reads the file from past its 128-byte header, so the
         * buffer holds fsHeader.size - 128 bytes. Stop at any chunk whose
         * header or body would run past them rather than read the heap
         * beyond: a non-looping flic that is animated after its last frame
         * starts its walk there. */
        int loaded = fsp->fsHeader.size - 128;
        if (start < 0 || start > loaded - 8) return 0;
        size = flic_read_i32((char*)header + start);
        header = (Flic_ChunkHeader*)((char*)header + start);
        found = 0;
        while (!found) {
            if (fsp->pointerposition >= fsp->fsHeader.size) break;
            if (fsp->pointerposition > loaded - 8 ||
                FLIC_CHUNK_SIZE(header) < 8 ||
                FLIC_CHUNK_SIZE(header) > loaded - fsp->pointerposition) {
                break;
            }
            switch (FLIC_CHUNK_TYPE(header)) {
            case FLI_PREFIX_TYPE:
                Flic_LoadPalette64(fsp);
                fsp->pointerposition += FLIC_CHUNK_SIZE(header);
                break;
            case FLI_POINTERS_TYPE:
                Flic_LoadPointers(fsp);
                fsp->pointerposition += FLIC_CHUNK_SIZE(header);
                break;
            case FLI_FRAME_TYPE:
                Flic_FrameChunk(fsp);
                found = 1;
                break;
            default:
                fsp->pointerposition += FLIC_CHUNK_SIZE(header);
                break;
            }
            /* The original leaves the walk as soon as it has drawn a frame,
             * so the frame ends it at start + size. Stepping on and adding
             * the next chunk's size as well skipped every other frame and
             * walked past the end of the buffer before the loop reset. */
            if (found) break;
            header = (Flic_ChunkHeader*)((char*)header + FLIC_CHUNK_SIZE(header));
            if (fsp->pointerposition > loaded - 8) break;
            size += FLIC_CHUNK_SIZE(header);
        }
    } else {
        size = FLIC_CHUNK_SIZE(header);
        found = 0;
        while (!found) {
            if (fsp->pointerposition >= fsp->fsHeader.size) break;
            switch (FLIC_CHUNK_TYPE(header)) {
            case FLI_PREFIX_TYPE:
                Flic_LoadPalette64(fsp);
                fsp->pointerposition += FLIC_CHUNK_SIZE(header);
                file_seek(fsp->filehandle, fsp->pointerposition, SEEK_SET);
                header = (Flic_ChunkHeader*)fsp->fsLoadBuffer;
                file_read(header, 16, 1, fsp->filehandle);
                size += FLIC_CHUNK_SIZE(header);
                break;
            case FLI_POINTERS_TYPE:
                Flic_LoadPointers(fsp);
                fsp->pointerposition += FLIC_CHUNK_SIZE(header);
                file_seek(fsp->filehandle, fsp->pointerposition, SEEK_SET);
                header = (Flic_ChunkHeader*)fsp->fsLoadBuffer;
                file_read(header, 16, 1, fsp->filehandle);
                size += FLIC_CHUNK_SIZE(header);
                break;
            case FLI_FRAME_TYPE:
                Flic_FrameChunk(fsp);
                found = 1;
                break;
            default:
                fsp->pointerposition += FLIC_CHUNK_SIZE(header);
                file_seek(fsp->filehandle, fsp->pointerposition, SEEK_SET);
                header = (Flic_ChunkHeader*)fsp->fsLoadBuffer;
                file_read(header, 16, 1, fsp->filehandle);
                size += FLIC_CHUNK_SIZE(header);
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
        subchunks = FLIC_CHUNK_COUNT(header);
        chunk = (int*)((char*)header + 16);
        fsp->pointerposition += 16;
        while (subchunks > 0 && fsp->pointerposition < fsp->fsHeader.size) {
            fsp->fsSource = chunk;
            Flic_DoChunk(fsp);
            fsp->pointerposition += FLIC_CHUNK_SIZE(chunk);
            chunk = (int*)((char*)chunk + FLIC_CHUNK_SIZE(chunk));
            subchunks--;
        }
    } else {
        chunk = (int*)fsp->fsLoadBuffer;
        subchunks = FLIC_CHUNK_COUNT(chunk);
        size = FLIC_CHUNK_SIZE(chunk) - 16;
        if (size > fsp->fsLoadBufferSize) {
            if (chunk) free(chunk);
            fsp->fsLoadBufferSize = size;
            fsp->fsLoadBuffer = malloc(size);
            if (fsp->fsLoadBuffer == NULL) {
                // STRING: LEGORR 0x004ab168
                snprintf(buff, sizeof(buff), "Flic Not Enough Memory");
                return 0;
            }
            chunk = (int*)fsp->fsLoadBuffer;
        }
        file_read(chunk, size, 1, fsp->filehandle);
        fsp->pointerposition += 16;
        while (subchunks > 0 && fsp->pointerposition < fsp->fsHeader.size) {
            fsp->fsSource = chunk;
            Flic_DoChunk(fsp);
            fsp->pointerposition += FLIC_CHUNK_SIZE(chunk);
            chunk = (int*)((char*)chunk + FLIC_CHUNK_SIZE(chunk));
            subchunks--;
        }
    }

    return 1;
}

// FUNCTION: LEGORR 0x00484610
int Flic_DoChunk(LPFLICSTRUCT fsp)
{
    int retval = 0;

    switch ((short)FLIC_CHUNK_TYPE(fsp->fsSource)) {
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
    case FLI_CHUNK_FLI_COPY:
        Flic_Copy(fsp);
        retval = 1;
        break;
    case FLI_CHUNK_BYTE_RUN:
    case FLI_CHUNK_DTA_BRUN:
        Flic_Brun(fsp);
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
        size = FLIC_CHUNK_SIZE(chunk);
        if (size > fsp->fsLoadBufferSize) {
            if (chunk) free(chunk);
            fsp->fsLoadBufferSize = size;
            fsp->fsLoadBuffer = malloc(size);
            if (fsp->fsLoadBuffer == NULL) {
                snprintf(buff, sizeof(buff), "Flic Not Enough Memory");
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
        size = FLIC_CHUNK_SIZE(chunk);
        if (size > fsp->fsLoadBufferSize) {
            if (chunk) free(chunk);
            fsp->fsLoadBufferSize = size;
            fsp->fsLoadBuffer = malloc(size);
            if (fsp->fsLoadBuffer == NULL) {
                snprintf(buff, sizeof(buff), "Flic Not Enough Memory");
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
                colour = flic_read_u16(src);
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
                    colour = flic_read_u16(src);
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

    packets = flic_read_u16(src);
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

    lines = flic_read_u16(src);
    src += 2;
    while (lines) {
        dst = (unsigned short*)((unsigned char*)fsp->fsSPtr + line * fsp->fsPitch);
        word = flic_read_u16(src);
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
        packets = flic_read_u16(src);
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

    lines = flic_read_u16(src);
    src += 2;
    while (lines) {
        dst = (unsigned short*)((unsigned char*)fsp->fsSPtr + line * fsp->fsPitch);
        word = flic_read_u16(src);
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
        packets = flic_read_u16(src);
        src += 2;
        while (packets) {
            dst += *src++;
            count = *src++;
            if (count & 0x80) {
                colour = flic_read_u16(src);
                src += 2;
                colour = ((colour & 0xffe0) << 1) | (colour & 0x1f);
                count = 0x100 - count;
                while (count) {
                    *dst++ = fsp->is15bit ? ((colour >> 1) & 0x7ff0) | (colour & 0x1f) : colour;
                    count--;
                }
            } else {
                while (count) {
                    colour = flic_read_u16(src);
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

/* The original's Flic_GetWidth has no address of its own: the linker folded
 * it with AnimClone_IsLws (004897d0, animclone.c's anim_clone_is_lws), which
 * also returns the 32-bit field at +0x0c, and the callers called that. On a
 * 64-bit build the two records no longer share that offset (an AnimClone's
 * lws sits after three pointers), so the callers read the flic's width here,
 * not through the folded getter. */
int Flic_GetWidth(LPFLICSTRUCT fsp)
{
    return fsp->fsXsize;
}

// FUNCTION: LEGORR 0x004850e0
int Flic_GetHeight(LPFLICSTRUCT fsp)
{
    return fsp->fsYsize;
}

#ifdef __cplusplus
}
#endif

