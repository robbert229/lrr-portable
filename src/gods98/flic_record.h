// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-016 Flic.c's records, shared with the units that read an open flic's
 * frame (FrontEnd.c, Reward.c). The +0x... comments are the Win32 offsets;
 * the pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GODS98_FLIC_RECORD_H
#define LEGORRR_GODS98_FLIC_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

struct Area2F;
struct IDirectDrawSurface4;
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

/* Flic.c reads the header from the file as it is: keep its layout fixed. */
#ifdef __cplusplus
static_assert(sizeof(Flic_Header) == 0x88, "Flic_Header is the file's header");
#endif

/* An open flic (0x6e8 bytes on Win32). */
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
    struct IDirectDrawSurface4* fsSurface; /* +0x63c */
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

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FLIC_RECORD_H */
