// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 DirectDraw.c (original translation unit G98-010, finding 596):
 * driver, device and display-mode enumeration, the display setup, the
 * front and back buffers and their clippers, and a few surface helpers.
 *
 * C-compatible boundary header (ADR 0005). directdraw.c is compiled as C in
 * the matching build and as C++ in the MinGW reference build. The native
 * port has no DirectDraw: the unit compiles to nothing there, and
 * directdraw_port.cpp gives the entry points that converted units call
 * their native bodies. Shared C++ headers do not include this file: every
 * include and declaration a header adds shifts MSVC's symbol numbering
 * (finding 506). src/game/test_c_boundaries.cpp includes it. */
#ifndef LEGORRR_GODS98_DIRECTDRAW_H
#define LEGORRR_GODS98_DIRECTDRAW_H

#ifdef __cplusplus
extern "C" {
#endif

/* The COM interfaces the unit calls (directdraw.c lays out their method
 * tables). */
struct DirectDrawIface;
struct DirectDrawSurfaceIface;
struct DirectDrawClipperIface;
struct Area2F;

/* GUID, laid out as the SDK's. */
typedef struct DirectDraw_Guid {
    unsigned int data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} DirectDraw_Guid;

/* A DirectDraw driver, a Direct3D device and a display mode, as the
 * enumeration callbacks fill them (0x114, 0x114 and 0x110 bytes). */
typedef struct DirectDraw_Driver {
    DirectDraw_Guid guid;           /* +0x000 */
    char desc[256];                 /* +0x010 */
    unsigned int flags;            /* +0x110 DIRECTDRAW_FLAG_* */
} DirectDraw_Driver;

typedef struct DirectDraw_Device {
    DirectDraw_Guid guid;           /* +0x000 */
    char desc[256];                 /* +0x010 */
    unsigned int flags;            /* +0x110 DIRECTDRAW_FLAG_DEVICE_* */
} DirectDraw_Device;

typedef struct DirectDraw_Mode {
    unsigned int width;            /* +0x000 */
    unsigned int height;           /* +0x004 */
    unsigned int bitDepth;         /* +0x008 */
    char desc[256];                 /* +0x00c */
    unsigned int flags;            /* +0x10c */
} DirectDraw_Mode;

#define DIRECTDRAW_FLAG_VALID                   0x00000001
#define DIRECTDRAW_FLAG_DRIVER_PRIMARY          0x00000010
#define DIRECTDRAW_FLAG_DRIVER_WINDOWOK         0x00000020
#define DIRECTDRAW_FLAG_DEVICE_DEPTH8           0x00000010
#define DIRECTDRAW_FLAG_DEVICE_DEPTH16          0x00000020
#define DIRECTDRAW_FLAG_DEVICE_DEPTH24          0x00000040
#define DIRECTDRAW_FLAG_DEVICE_DEPTH32          0x00000080
#define DIRECTDRAW_FLAG_DEVICE_COLOUR           0x00001000
#define DIRECTDRAW_FLAG_DEVICE_HARDWARE         0x00002000
#define DIRECTDRAW_FLAG_DEVICE_VIDEOTEXTURE     0x00004000
#define DIRECTDRAW_FLAG_DEVICE_SYSTEMTEXTURE    0x00008000

/* The unit's state: a common symbol at 0076ac80 (0x48 bytes). Other units
 * read the DirectDraw object (+0x04) and the back buffer (+0x0c). */
typedef struct DirectDraw_Globs {
    void* hWnd;                                     /* +0x00 0076ac80 */
    struct DirectDrawIface* lpDirectDraw;           /* +0x04 0076ac84 */
    struct DirectDrawSurfaceIface* fSurf;           /* +0x08 0076ac88 */
    struct DirectDrawSurfaceIface* bSurf;           /* +0x0c 0076ac8c */
    struct DirectDrawSurfaceIface* zSurf;           /* +0x10 0076ac90 */
    struct DirectDrawClipperIface* lpFrontClipper;  /* +0x14 0076ac94 */
    struct DirectDrawClipperIface* lpBackClipper;   /* +0x18 0076ac98 */
    DirectDraw_Driver* driverList;                  /* +0x1c 0076ac9c */
    unsigned int reserved_20;                      /* +0x20 never referenced */
    DirectDraw_Device* deviceList;                  /* +0x24 0076aca4 */
    unsigned int reserved_28;                      /* +0x28 never referenced */
    DirectDraw_Mode* modeList;                      /* +0x2c 0076acac */
    unsigned int driverCount;                      /* +0x30 0076acb0 */
    unsigned int deviceCount;                      /* +0x34 0076acb4 */
    unsigned int modeCount;                        /* +0x38 0076acb8 */
    int fullScreen;                                 /* +0x3c 0076acbc */
    unsigned int width;                            /* +0x40 0076acc0 */
    unsigned int height;                           /* +0x44 0076acc4 */
} DirectDraw_Globs;

#if !defined(LEGORRR_BACKEND_MINIWIN)
extern DirectDraw_Globs directDrawGlobs;
#endif

void directdraw_initialise(void* hWnd);                                 /* 0047c170 */
int directdraw_enum_drivers(DirectDraw_Driver* list,
                            unsigned int* count);                      /* 0047c1c0 */
int directdraw_enum_devices(const DirectDraw_Driver* driver,
                            DirectDraw_Device* list,
                            unsigned int* count);                      /* 0047c2e0 */
int directdraw_enum_modes(const DirectDraw_Driver* driver, int fullScreen,
                          DirectDraw_Mode* list,
                          unsigned int* count);                        /* 0047c4b0 */
int directdraw_setup(int fullScreen, const DirectDraw_Driver* driver,
                     const DirectDraw_Device* device,
                     const DirectDraw_Mode* mode, unsigned int xPos,
                     unsigned int yPos, unsigned int width,
                     unsigned int height);                             /* 0047c610 */
void directdraw_flip(void);                                             /* 0047c8d0 */
int directdraw_save_bmp(struct DirectDrawSurfaceIface* surface,
                        const char* fname);                             /* 0047c8f0 */
void directdraw_return_front_buffer(void);                              /* 0047cc20 */
void directdraw_blit_buffers(void);                                     /* 0047cc50 */
void directdraw_shutdown(void);                                         /* 0047ccf0 */
void directdraw_adjust_texture_usage(unsigned int* textureUsage);      /* 0047cd50 */
int directdraw_get_avail_texture_mem(unsigned int* total,
                                     unsigned int* avail);             /* 0047cdd0 */
void directdraw_clear(struct Area2F* window, unsigned int colour);     /* 0047ce20 */
int directdraw_create_clipper(int fullScreen, unsigned int width,
                              unsigned int height);                    /* 0047cee0 */
void directdraw_blt_8_to_16(struct DirectDrawSurfaceIface* target,
                            struct DirectDrawSurfaceIface* source,
                            void* palette);                             /* 0047d000 */
unsigned int directdraw_get_colour(struct DirectDrawSurfaceIface* surf,
                                    unsigned int colour);              /* 0047d2d0 */
unsigned int directdraw_count_mask_bits(unsigned int mask);           /* 0047d3f0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_DIRECTDRAW_H */
