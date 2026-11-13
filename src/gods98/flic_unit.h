// SPDX-License-Identifier: LGPL-3.0-or-later
/* Flic.c (original translation unit G98-016, finding 632): gods98's FLIC
 * (.flh) player. Flic_Setup opens a file into a FLICSTRUCT and a
 * system-memory DirectDraw surface; Flic_Animate decodes the next frame into
 * the surface and blits it to the back buffer.
 *
 * C-compatible header (ADR 0005). flic_unit.c is compiled as C in the
 * matching build. The native port has no DirectDraw, so the unit compiles to
 * nothing there and the port's player (flic.hpp, flic.cpp, flic_port.cpp)
 * serves every caller. Shared C++ headers do not include this file: every
 * include and declaration a header adds shifts MSVC's symbol numbering
 * (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * The names are the unit's own, after gods98's Flic_* convention. In the
 * matching build seven of them take the port's names (flic_link_names.h). */
#ifndef LEGORRR_GODS98_FLIC_UNIT_H
#define LEGORRR_GODS98_FLIC_UNIT_H

#ifdef __cplusplus
extern "C" {
#endif

struct Area2F;
struct Flic_SurfaceIface;
struct File;

/* FLICSTRUCT userflags (+0x000). */
#define FLICMEMORY 0x00000001   /* the whole file is read into memory */
#define FLICLOOPINGON 0x00000002

/* FLICSTRUCT fsDisplayMode (+0x6d4). */
#define FLICMODE_BYTEPERPIXEL 0
#define FLICMODE_HICOLOR 4

/* The file header, as read from the start of the file (0x88 bytes). */
typedef struct Flic_Header {
    int size;                       /* +0x00 the file's length */
    unsigned short magic;           /* +0x04 */
    unsigned short frames;          /* +0x06 */
    unsigned short width;           /* +0x08 */
    unsigned short height;          /* +0x0a */
    unsigned short depth;           /* +0x0c */
    unsigned short flags;           /* +0x0e */
    int speed;                      /* +0x10 */
    unsigned char reserved[0x74];   /* +0x14 */
} Flic_Header;

/* An open flic (0x6e8 bytes). */
typedef struct FLICSTRUCT {
    unsigned int userflags;         /* +0x000 FLICMEMORY, FLICLOOPINGON */
    int fsXc;                       /* +0x004 */
    int fsYc;                       /* +0x008 */
    int fsXsize;                    /* +0x00c */
    int fsYsize;                    /* +0x010 */
    char* rambufferhandle;          /* +0x014 the file, when FLICMEMORY */
    char* destportalbuffer;         /* +0x018 */
    char filename[256];             /* +0x01c */
    unsigned char fsPalette256[256 * 3]; /* +0x11c */
    unsigned short fsPalette64k[256]; /* +0x41c */
    int framerate;                  /* +0x61c */
    int lastticks;                  /* +0x620 */
    int currentframe;               /* +0x624 */
    int overallframe;               /* +0x628 */
    int mode;                       /* +0x62c */
    int ringframe;                  /* +0x630 the loop position */
    int pointerposition;            /* +0x634 the file position */
    unsigned int fsPitch;           /* +0x638 */
    struct Flic_SurfaceIface* fsSurface; /* +0x63c */
    Flic_Header fsHeader;           /* +0x640 */
    struct File* filehandle;        /* +0x6c8 Files.c's File */
    void* fsSPtr;                   /* +0x6cc the locked surface's pixels */
    void* fsSource;                 /* +0x6d0 the current chunk */
    int fsDisplayMode;              /* +0x6d4 FLICMODE_* */
    int fsBitPlanes;                /* +0x6d8 */
    int fsLoadBufferSize;           /* +0x6dc */
    void* fsLoadBuffer;             /* +0x6e0 */
    int is15bit;                    /* +0x6e4 */
} FLICSTRUCT, *LPFLICSTRUCT;

int Flic_Setup(const char* filename, LPFLICSTRUCT* fsp,
               unsigned int flags);                            /* 00483c80 */
int Flic_Close(LPFLICSTRUCT fsp);                               /* 00483f00 */
int Flic_LoadHeader(const char* filename, LPFLICSTRUCT* fsp);   /* 00483f60 */
int Flic_Animate(LPFLICSTRUCT fsp, const struct Area2F* destArea,
                 int advance, int trans);                      /* 00484070 */
int Flic_Memory(LPFLICSTRUCT fsp);                              /* 004841d0 */
int Flic_Load(LPFLICSTRUCT fsp);                                /* 00484260 */
int Flic_FindChunk(LPFLICSTRUCT fsp);                           /* 00484320 */
int Flic_FrameChunk(LPFLICSTRUCT fsp);                          /* 004844b0 */
int Flic_DoChunk(LPFLICSTRUCT fsp);                             /* 00484610 */
int Flic_LoadPointers(LPFLICSTRUCT fsp);                        /* 00484720 */
int Flic_LoadPalette64(LPFLICSTRUCT fsp);                       /* 004847d0 */
int Flic_Copy(LPFLICSTRUCT fsp);                                /* 00484880 */
int Flic_BrunDepack(LPFLICSTRUCT fsp);                          /* 004848d0 */
int Flic_BrunDepack16(LPFLICSTRUCT fsp);                        /* 004849d0 */
int Flic_Brun(LPFLICSTRUCT fsp);                                /* 00484b20 */
int Flic_Unsupported(LPFLICSTRUCT fsp);                         /* 00484b90 */
void Flic_Palette256To64k(LPFLICSTRUCT fsp);                    /* 00484ba0 */
int Flic_Colour256(LPFLICSTRUCT fsp);                           /* 00484c00 */
void Flic_Nothing(LPFLICSTRUCT fsp);                            /* 00484c90 */
void Flic_DeltaWord(LPFLICSTRUCT fsp);                          /* 00484ca0 */
void Flic_DeltaWord16(LPFLICSTRUCT fsp);                        /* 00484e50 */
int Flic_Delta(LPFLICSTRUCT fsp);                               /* 00485030 */
unsigned short Flic_GetPaletteColour(unsigned char index,
                                     LPFLICSTRUCT fsp);         /* 004850c0 */
int Flic_GetHeight(LPFLICSTRUCT fsp);                           /* 004850e0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FLIC_UNIT_H */
