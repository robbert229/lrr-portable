// SPDX-License-Identifier: LGPL-3.0-or-later
/* Images.c, original translation unit G98-011 (finding 632), rebuilt in
 * original shape (ADR 0005): gods98's 2D images, text 0047d410-0047e460,
 * .rdata 0049e998 (255.0f), strings 004aae7c-004aaeb4, .bss
 * 00533908-00533994.
 *
 * An Image is a DirectDraw surface with its size, its pens and a colour-key
 * flag. Images come from a pool of blocks of 1, 2, 4, ... records
 * (imageGlobs.listSet) with a free list linked through the last word
 * (Image_AddList, Image_Create, Image_Remove). Image_LoadBMPScaled parses a
 * bitmap (BMP.c) into a system-memory surface, converting 8-bit and 24-bit
 * pixels to the display's 16-bit format and flipping the rows.
 *
 * The native port has no DirectDraw: the unit compiles to nothing there,
 * and the port's software surfaces and adapters serve the callers. In the
 * matching build images_link_names.h gives two functions the port's names,
 * and the C units' headers point their other calls at the unit. */
#include "images_link_names.h"

#if !defined(LEGORRR_BACKEND_MINIWIN)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"

#if defined(_MSC_VER) && _MSC_VER == 1100
#include <windows.h>
#endif

#include "images_unit.h"
#include "images_imports.h"

#if defined(_WIN32)
#define IMAGE_STDCALL __stdcall
#else
#define IMAGE_STDCALL
#endif

#define IMAGE_GLOB_FLAG_INITIALISED 0x00000001

/* BMP.c's parsed bitmap (BMP_Parse, 00489c30; 0x3c bytes). */
typedef struct Image_PaletteEntry {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char reserved;
} Image_PaletteEntry;

struct IndexedBitmapPaletteData {
    int width;                      /* +0x00 */
    int height;                     /* +0x04 */
    int aspectx;                    /* +0x08 */
    int aspecty;                    /* +0x0c */
    int bpp;                        /* +0x10 */
    int rgb;                        /* +0x14 0 for 8-bit (paletted) */
    int bytes_per_line;             /* +0x18 */
    unsigned char* buffer1;         /* +0x1c */
    unsigned char* buffer2;         /* +0x20 */
    int red_mask;                   /* +0x24 */
    int green_mask;                 /* +0x28 */
    int blue_mask;                  /* +0x2c */
    int alpha_mask;                 /* +0x30 */
    int palette_size;               /* +0x34 */
    Image_PaletteEntry* palette;    /* +0x38 */
};
typedef struct IndexedBitmapPaletteData BMP_Image;

/* DirectDraw's RECT, DDCOLORKEY, DDPIXELFORMAT and DDSURFACEDESC2. */
typedef struct Image_Rect {
    int left;
    int top;
    int right;
    int bottom;
} Image_Rect;

typedef struct Image_ColourKey {
    unsigned int dwColorSpaceLowValue;
    unsigned int dwColorSpaceHighValue;
} Image_ColourKey;

typedef struct Image_PixelFormat {
    unsigned int dwSize;            /* +0x00 */
    unsigned int dwFlags;           /* +0x04 */
    unsigned int dwFourCC;          /* +0x08 */
    unsigned int dwRGBBitCount;     /* +0x0c */
    unsigned int dwRBitMask;        /* +0x10 */
    unsigned int dwGBitMask;        /* +0x14 */
    unsigned int dwBBitMask;        /* +0x18 */
    unsigned int dwRGBAlphaBitMask; /* +0x1c */
} Image_PixelFormat;

struct Image_SurfaceDesc {
    unsigned int dwSize;            /* +0x00 */
    unsigned int dwFlags;           /* +0x04 */
    unsigned int dwHeight;          /* +0x08 */
    unsigned int dwWidth;           /* +0x0c */
    int lPitch;                     /* +0x10 */
    unsigned int dwBackBufferCount; /* +0x14 */
    unsigned int dwRefreshRate;     /* +0x18 */
    unsigned int dwAlphaBitDepth;   /* +0x1c */
    unsigned int dwReserved;        /* +0x20 */
    void* lpSurface;                /* +0x24 */
    Image_ColourKey ddckCKDestOverlay; /* +0x28 */
    Image_ColourKey ddckCKDestBlt;  /* +0x30 */
    Image_ColourKey ddckCKSrcOverlay; /* +0x38 */
    Image_ColourKey ddckCKSrcBlt;   /* +0x40 */
    Image_PixelFormat ddpfPixelFormat; /* +0x48 */
    unsigned int dwCaps;            /* +0x68 ddsCaps */
    unsigned int dwCaps2;           /* +0x6c */
    unsigned int dwCaps3;           /* +0x70 */
    unsigned int dwCaps4;           /* +0x74 */
    unsigned int dwTextureStage;    /* +0x78 */
};
typedef struct Image_SurfaceDesc Image_SurfaceDesc;

#define DDSD_CAPS 0x00000001
#define DDSD_HEIGHT 0x00000002
#define DDSD_WIDTH 0x00000004
#define DDSCAPS_OFFSCREENPLAIN 0x00000040
#define DDSCAPS_SYSTEMMEMORY 0x00000800
#define DDCKEY_SRCBLT 0x00000008
#define DDLOCK_WAIT 0x00000001
#define DDLOCK_READONLY 0x00000010
#define DDLOCK_WRITEONLY 0x00000020
#define DDBLT_KEYSRC 0x00008000
#define DDBLT_WAIT 0x01000000

/* IDirectDraw4 and IDirectDrawSurface4, the methods the unit calls. */
typedef struct Image_DDrawIface Image_DDrawIface;
typedef struct Image_SurfaceIface Image_SurfaceIface;

typedef struct Image_DDrawVtbl {
    void* QueryInterface;
    void* AddRef;
    void* Release;
    void* Compact;
    void* CreateClipper;
    void* CreatePalette;
    long (IMAGE_STDCALL* CreateSurface)(Image_DDrawIface*, Image_SurfaceDesc*,
                                        Image_SurfaceIface**, void*);
} Image_DDrawVtbl;

struct Image_DDrawIface {
    const Image_DDrawVtbl* lpVtbl;
};

typedef struct Image_SurfaceVtbl {
    void* QueryInterface;
    void* AddRef;
    unsigned long (IMAGE_STDCALL* Release)(Image_SurfaceIface*);
    void* AddAttachedSurface;
    void* AddOverlayDirtyRect;
    long (IMAGE_STDCALL* Blt)(Image_SurfaceIface*, Image_Rect*,
                              Image_SurfaceIface*, Image_Rect*, unsigned int,
                              void*);
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
    long (IMAGE_STDCALL* GetPixelFormat)(Image_SurfaceIface*,
                                         Image_PixelFormat*);
    void* GetSurfaceDesc;
    void* Initialize;
    void* IsLost;
    long (IMAGE_STDCALL* Lock)(Image_SurfaceIface*, Image_Rect*,
                               Image_SurfaceDesc*, unsigned int, void*);
    void* ReleaseDC;
    void* Restore;
    void* SetClipper;
    long (IMAGE_STDCALL* SetColorKey)(Image_SurfaceIface*, unsigned int,
                                      Image_ColourKey*);
    void* SetOverlayPosition;
    void* SetPalette;
    long (IMAGE_STDCALL* Unlock)(Image_SurfaceIface*, Image_Rect*);
} Image_SurfaceVtbl;

struct Image_SurfaceIface {
    const Image_SurfaceVtbl* lpVtbl;
};

#define lpDD() ((Image_DDrawIface*)directDrawGlobs.lpDirectDraw)
#define DirectDraw_bSurf() ((Image_SurfaceIface*)directDrawGlobs.bSurf)
#define Image_Surface(image) ((Image_SurfaceIface*)(image)->surface)

/* The unit's state (0x8c bytes). */
typedef struct Image_Globs {
    Image* listSet[IMAGE_MAXLISTS]; /* +0x00 00533908 */
    Image* freeList;                /* +0x80 00533988 */
    unsigned int listCount;         /* +0x84 0053398c */
    unsigned int flags;             /* +0x88 00533990 */
} Image_Globs;

// GLOBAL: LEGORR 0x00533908
static Image_Globs imageGlobs;

/* The unit's functions have C linkage in the reference build's C++ compile
 * too. */
#ifdef __cplusplus
extern "C" {
#endif

// FUNCTION: LEGORR 0x0047d410
void Image_Initialise(void)
{
    unsigned int loop;

    for (loop = 0; loop < IMAGE_MAXLISTS; loop++) {
        imageGlobs.listSet[loop] = NULL;
    }

    imageGlobs.flags = IMAGE_GLOB_FLAG_INITIALISED;
}

// FUNCTION: LEGORR 0x0047d430
void Image_Shutdown(void)
{
    unsigned int loop;

    Image_RemoveAll();

    for (loop = 0; loop < IMAGE_MAXLISTS; loop++) {
        if (imageGlobs.listSet[loop]) free(imageGlobs.listSet[loop]);
    }

    imageGlobs.freeList = NULL;
    imageGlobs.flags = 0;
}

// FUNCTION: LEGORR 0x0047d470
void Image_Remove(Image* dead)
{
    Image_Surface(dead)->lpVtbl->Release(Image_Surface(dead));

    dead->nextFree = imageGlobs.freeList;
    imageGlobs.freeList = dead;
}

// FUNCTION: LEGORR 0x0047d490
int Image_CopyToDataToSurface(Image_SurfaceIface* surface, BMP_Image* image)
{
    Image_SurfaceDesc desc = { sizeof(Image_SurfaceDesc) };

    if (surface->lpVtbl->Lock(surface, NULL, &desc,
                              DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL) == 0) {
        switch (image->bpp) {
        case 8:
            Image_8BitSourceCopy(&desc, image);
            break;
        case 16:
            break;
        case 24:
            Image_24BitSourceCopy(&desc, image);
            break;
        }
        surface->lpVtbl->Unlock(surface, NULL);
        return 1;
    }

    return 0;
}

// FUNCTION: LEGORR 0x0047d520
int Image_8BitSourceCopy(Image_SurfaceDesc* desc, BMP_Image* image)
{
    unsigned int bpp = desc->ddpfPixelFormat.dwRGBBitCount;

    if (bpp == 8) {
        return 0;
    } else if (bpp == 16) {
        unsigned int gm = desc->ddpfPixelFormat.dwGBitMask;
        unsigned short* to = (unsigned short*)desc->lpSurface;
        int lineLen = image->bytes_per_line;
        Image_PaletteEntry* pal = image->palette;
        unsigned int pitch = desc->lPitch;
        unsigned char* from = image->buffer1;
        unsigned int bm = desc->ddpfPixelFormat.dwBBitMask;
        unsigned int rm = desc->ddpfPixelFormat.dwRBitMask;
        int rbits = Image_CountMaskBits(rm);
        int gbits = Image_CountMaskBits(gm);
        int bbits = Image_CountMaskBits(bm);
        int rshift = Image_CountMaskBitShift(rm);
        int gshift = Image_CountMaskBitShift(gm);
        int bshift = Image_CountMaskBitShift(bm);
        int x;
        int y;

        for (y = 0; y < image->height; y++) {
            for (x = 0; x < image->width; x++) {
                unsigned int index = *from++;
                *to++ = (unsigned short)(((pal[index].blue >> (8 - bbits)) << bshift) |
                                         ((pal[index].red >> (8 - rbits)) << rshift) |
                                         ((pal[index].green >> (8 - gbits)) << gshift));
            }
            from += lineLen - image->width;
            to += (pitch / 2) - image->width;
        }

        Image_FlipSurface(desc);
        return 1;
    } else if (bpp == 24) {
        return 0;
    } else if (bpp == 32) {
        return 0;
    }

    return 1;
}

// FUNCTION: LEGORR 0x0047d700
int Image_CountMaskBits(unsigned int mask)
{
    int count = 0;
    unsigned int loop = 0;

    do {
        if (mask & (1 << loop)) count++;
        loop++;
    } while (loop < 32);

    return count;
}

// FUNCTION: LEGORR 0x0047d720
int Image_CountMaskBitShift(unsigned int mask)
{
    unsigned int loop;

    for (loop = 0; loop < 32; loop++) {
        if (mask & (1 << loop)) return loop;
    }

    return -1;
}

// FUNCTION: LEGORR 0x0047d740
void Image_FlipSurface(Image_SurfaceDesc* desc)
{
    unsigned int pitch = desc->lPitch / 2;
    unsigned short* fromLine = (unsigned short*)desc->lpSurface;
    unsigned short* toLine = fromLine + (desc->dwHeight - 1) * pitch;
    unsigned short* buffer = (unsigned short*)malloc(pitch * sizeof(unsigned short));
    unsigned int y;

    for (y = 0; y < desc->dwHeight / 2; y++) {
        memcpy(buffer, fromLine, pitch * sizeof(unsigned short));
        memcpy(fromLine, toLine, pitch * sizeof(unsigned short));
        memcpy(toLine, buffer, pitch * sizeof(unsigned short));
        fromLine += pitch;
        toLine -= pitch;
    }

    free(buffer);
}

// FUNCTION: LEGORR 0x0047d800
int Image_24BitSourceCopy(Image_SurfaceDesc* desc, BMP_Image* image)
{
    unsigned int bpp = desc->ddpfPixelFormat.dwRGBBitCount;

    if (bpp == 8) {
        return 0;
    } else if (bpp == 16) {
        unsigned int gm = desc->ddpfPixelFormat.dwGBitMask;
        unsigned int bm = desc->ddpfPixelFormat.dwBBitMask;
        unsigned short* to = (unsigned short*)desc->lpSurface;
        int lineLen = image->bytes_per_line;
        unsigned char* from = image->buffer1;
        unsigned int pitch = desc->lPitch;
        unsigned int rm = desc->ddpfPixelFormat.dwRBitMask;
        int rbits = Image_CountMaskBits(rm);
        int gbits = Image_CountMaskBits(gm);
        int bbits = Image_CountMaskBits(bm);
        int rshift = Image_CountMaskBitShift(rm);
        int gshift = Image_CountMaskBitShift(gm);
        int bshift = Image_CountMaskBitShift(bm);
        int x;
        int y;

        for (y = 0; y < image->height; y++) {
            for (x = 0; x < image->width; x++) {
                unsigned char b = *from++;
                unsigned char g = *from++;
                unsigned char r = *from++;
                *to++ = (unsigned short)(((b >> (8 - bbits)) << bshift) |
                                         ((g >> (8 - gbits)) << gshift) |
                                         ((r >> (8 - rbits)) << rshift));
            }
            from += lineLen - (image->width * 3);
            to += (pitch / 2) - image->width;
        }

        Image_FlipSurface(desc);
        return 1;
    } else if (bpp == 24) {
        return 0;
    } else if (bpp == 32) {
        return 0;
    }

    return 1;
}

// FUNCTION: LEGORR 0x0047d9d0
Image* Image_LoadBMPScaled(const char* fileName, unsigned int width,
                           unsigned int height)
{
    BMP_Image image = { 0 };
    Image_SurfaceDesc ddsd = { 0 };
    Image_SurfaceIface* surface = NULL;
    Image* newImage;
    void* buffer;
    unsigned int size = 0;
    unsigned int penZero;
    unsigned int pen255;
    char name[1024];

    penZero = pen255 = Image_RGB2CR(0, 0, 0);

    if ((buffer = file_load_binary(fileName, &size)) != NULL) {
        BMP_Parse(buffer, size, &image);

        if (!image.rgb) {
            penZero = Image_RGB2CR(image.palette[0].red, image.palette[0].green,
                                   image.palette[0].blue);
            if (!image.rgb) {
                pen255 = Image_RGB2CR(image.palette[255].red,
                                      image.palette[255].green,
                                      image.palette[255].blue);
            }
        }

        // STRING: LEGORR 0x004aae7c
        sprintf(name, "Data\\%s", fileName);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        ddsd.dwWidth = image.width;
        ddsd.dwHeight = image.height;

        if (lpDD()->lpVtbl->CreateSurface(lpDD(), &ddsd, &surface, NULL) == 0) {
            if (Image_CopyToDataToSurface(surface, &image)) {
                if ((newImage = Image_Create(surface, image.width, image.height,
                                             penZero, pen255)) != NULL) {
                    BMP_Cleanup(&image);
                    free(buffer);
                    return newImage;
                }
            }
        }

        if (surface) surface->lpVtbl->Release(surface);
        BMP_Cleanup(&image);
        free(buffer);
    }

    return NULL;
}

// FUNCTION: LEGORR 0x0047db90
unsigned int Image_RGB2CR(unsigned char r, unsigned char g, unsigned char b)
{
    union {
        unsigned char c[4];
        unsigned int v;
    } colour;

    colour.c[0] = r;
    colour.c[1] = g;
    colour.c[2] = b;
    colour.c[3] = 0;

    return colour.v;
}

// FUNCTION: LEGORR 0x0047dbc0
void Image_SetPenZeroTrans(Image* image)
{
    Image_ColourKey colourKey;

    colourKey.dwColorSpaceLowValue = image->penZero;
    colourKey.dwColorSpaceHighValue = image->penZero;
    Image_Surface(image)->lpVtbl->SetColorKey(Image_Surface(image),
                                              DDCKEY_SRCBLT, &colourKey);
    image->flags |= IMAGE_FLAG_TRANS;
}

// FUNCTION: LEGORR 0x0047dbf0
void Image_SetupTrans(Image* image, float lowr, float lowg, float lowb,
                      float highr, float highg, float highb)
{
    Image_ColourKey colourKey;
    unsigned int low;
    unsigned int high;

    low = Image_RGB2CR((unsigned char)(lowr * 255.0f),
                       (unsigned char)(lowg * 255.0f),
                       (unsigned char)(lowb * 255.0f));
    high = Image_RGB2CR((unsigned char)(highr * 255.0f),
                        (unsigned char)(highg * 255.0f),
                        (unsigned char)(highb * 255.0f));
    colourKey.dwColorSpaceLowValue = Image_DDColorMatch(Image_Surface(image), low);
    colourKey.dwColorSpaceHighValue = Image_DDColorMatch(Image_Surface(image), high);
    Image_Surface(image)->lpVtbl->SetColorKey(Image_Surface(image),
                                              DDCKEY_SRCBLT, &colourKey);
    image->flags |= IMAGE_FLAG_TRANS;
}

// FUNCTION: LEGORR 0x0047dcb0
void Image_DisplayScaled(Image* image, const Area2F* src,
                         const Point2F* destPos, const Point2F* destSize)
{
    Image_Rect r_src;
    Image_Rect r_dst;
    Image_SurfaceIface* bSurf = DirectDraw_bSurf();

    if (src) {
        r_src.left = (int)floor(src->x);
        r_src.top = (int)floor(src->y);
        r_src.right = (int)floor(src->x + src->width);
        r_src.bottom = (int)floor(src->y + src->height);
    }

    if (destPos) {
        r_dst.left = (int)floor(destPos->x);
        r_dst.top = (int)floor(destPos->y);
        if (destSize) {
            r_dst.right = (int)floor(destPos->x + destSize->x);
            r_dst.bottom = (int)floor(destPos->y + destSize->y);
        } else if (src) {
            r_dst.right = (int)floor(destPos->x + src->width);
            r_dst.bottom = (int)floor(destPos->y + src->height);
        } else {
            r_dst.right = (int)floor(destPos->x + (float)image->width);
            r_dst.bottom = (int)floor(destPos->y + (float)image->height);
        }
    }

    bSurf->lpVtbl->Blt(bSurf, destPos ? &r_dst : NULL, Image_Surface(image),
                       src ? &r_src : NULL,
                       (image->flags & IMAGE_FLAG_TRANS) ? DDBLT_KEYSRC | DDBLT_WAIT : DDBLT_WAIT,
                       NULL);
}

// FUNCTION: LEGORR 0x0047de60
void* Image_LockSurface(Image* image, unsigned int* pitch, unsigned int* bpp)
{
    Image_SurfaceDesc desc;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    if (Image_Surface(image)->lpVtbl->Lock(Image_Surface(image), NULL, &desc,
                                           DDLOCK_WAIT, NULL) == 0) {
        *pitch = desc.lPitch;
        *bpp = desc.ddpfPixelFormat.dwRGBBitCount;
        return desc.lpSurface;
    }

    return NULL;
}

// FUNCTION: LEGORR 0x0047ded0
void Image_UnlockSurface(Image* image)
{
    Image_Surface(image)->lpVtbl->Unlock(Image_Surface(image), NULL);
}

// FUNCTION: LEGORR 0x0047def0
unsigned int Image_GetPen255(Image* image)
{
    Image_PixelFormat pf;
    unsigned int pen = 0;

    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);

    if (Image_Surface(image)->lpVtbl->GetPixelFormat(Image_Surface(image), &pf) == 0) {
        ((unsigned char*)&pen)[0] = ((unsigned char*)&image->pen255)[3];
        ((unsigned char*)&pen)[1] = ((unsigned char*)&image->pen255)[2];
        ((unsigned char*)&pen)[2] = ((unsigned char*)&image->pen255)[1];
        ((unsigned char*)&pen)[3] = ((unsigned char*)&image->pen255)[0];
        return pen;
    }

    /* The original falls off the end here: eax holds GetPixelFormat's
     * error code. */
}

// FUNCTION: LEGORR 0x0047df50
unsigned int Image_GetPixelMask(Image* image)
{
    Image_PixelFormat pf;

    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);

    if (Image_Surface(image)->lpVtbl->GetPixelFormat(Image_Surface(image), &pf) == 0) {
        return 0xffffffff << (32 - pf.dwRGBBitCount);
    }

    return 0;
}

// FUNCTION: LEGORR 0x0047dfa0
int Image_GetPixel(Image* image, unsigned int x, unsigned int y,
                   unsigned int* colour)
{
    Image_SurfaceDesc desc;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    if (x < image->width && y < image->height) {
        if (Image_Surface(image)->lpVtbl->Lock(Image_Surface(image), NULL, &desc,
                                               DDLOCK_WAIT | DDLOCK_READONLY,
                                               NULL) == 0) {
            *colour = *(unsigned int*)((unsigned char*)desc.lpSurface +
                                       (y * desc.lPitch) +
                                       (x * (desc.ddpfPixelFormat.dwRGBBitCount >> 3))) >>
                      (32 - desc.ddpfPixelFormat.dwRGBBitCount);
            Image_Surface(image)->lpVtbl->Unlock(Image_Surface(image), NULL);
            return 1;
        }
    }

    return 0;
}

// FUNCTION: LEGORR 0x0047e050
Image* Image_Create(Image_SurfaceIface* surface, unsigned int width,
                    unsigned int height, unsigned int penZero,
                    unsigned int pen255)
{
    Image* newImage;
    Image* next;

    if (imageGlobs.freeList == NULL) Image_AddList();

    newImage = imageGlobs.freeList;
    next = newImage->nextFree;
    newImage->nextFree = newImage;
    imageGlobs.freeList = next;

    newImage->flags = 0;
    newImage->width = width;
    newImage->height = height;
    newImage->surface = surface;
    newImage->penZeroRGB = penZero;
    newImage->penZero = Image_DDColorMatch(surface, penZero);
    newImage->pen255 = Image_DDColorMatch(surface, pen255);

    return newImage;
}

// FUNCTION: LEGORR 0x0047e0c0
void Image_AddList(void)
{
    Image* list;
    unsigned int loop;
    unsigned int count;

    count = 0x00000001 << imageGlobs.listCount;

    if ((list = imageGlobs.listSet[imageGlobs.listCount] =
             (Image*)malloc(sizeof(Image) * count)) != NULL) {
        imageGlobs.listCount++;

        for (loop = 1; loop < count; loop++) {
            list[loop - 1].nextFree = &list[loop];
        }
        list[count - 1].nextFree = imageGlobs.freeList;
        imageGlobs.freeList = list;
    }
}

// FUNCTION: LEGORR 0x0047e130
void Image_RemoveAll(void)
{
    unsigned int list;
    unsigned int count;
    unsigned int loop;
    Image* tempImage;

    for (list = 0; list < imageGlobs.listCount; list++) {
        if (imageGlobs.listSet[list]) {
            count = 0x00000001 << list;
            for (loop = 0; loop < count; loop++) {
                if ((tempImage = &imageGlobs.listSet[list][loop]) != NULL) {
                    if (tempImage->nextFree == tempImage) {
                        Image_Remove(tempImage);
                    }
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x0047e190
unsigned int Image_DDColorMatch(Image_SurfaceIface* pdds, unsigned int rgb)
{
    unsigned char b;
    unsigned char r;
    unsigned int rm;
    Image_SurfaceDesc ddsd = { sizeof(Image_SurfaceDesc) };
    int gshift;
    int rbits;
    int bbits;
    unsigned char g;
    int gbits;
    unsigned int gm;
    int rshift;
    int bshift;
    unsigned int bm;
    unsigned int dw = 0;

    Image_CR2RGB(rgb, &r, &g, &b);

    if (pdds->lpVtbl->Lock(pdds, NULL, &ddsd, DDLOCK_WAIT, NULL) == 0) {
        rm = ddsd.ddpfPixelFormat.dwRBitMask;
        gm = ddsd.ddpfPixelFormat.dwGBitMask;
        bm = ddsd.ddpfPixelFormat.dwBBitMask;
        rbits = Image_CountMaskBits(rm);
        gbits = Image_CountMaskBits(gm);
        bbits = Image_CountMaskBits(bm);
        rshift = Image_CountMaskBitShift(rm);
        gshift = Image_CountMaskBitShift(gm);
        bshift = Image_CountMaskBitShift(bm);

        dw = ((r >> (8 - rbits)) << rshift) |
             ((g >> (8 - gbits)) << gshift) |
             ((b >> (8 - bbits)) << bshift);

        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32) {
            dw &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount) - 1;
        }

        pdds->lpVtbl->Unlock(pdds, NULL);
    }

    return dw;
}

// FUNCTION: LEGORR 0x0047e2d0
void Image_CR2RGB(unsigned int rgb, unsigned char* r, unsigned char* g,
                  unsigned char* b)
{
    if (r) *r = ((unsigned char*)&rgb)[0];
    if (g) *g = ((unsigned char*)&rgb)[1];
    if (b) *b = ((unsigned char*)&rgb)[2];
}

// FUNCTION: LEGORR 0x0047e300
void Image_GetScreenshot(Image* image, unsigned int xsize, unsigned int ysize)
{
    Image_SurfaceDesc desc;
    Image_SurfaceIface* surf;
    Image_Rect rect;
    long r;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    desc.dwWidth = xsize;
    desc.dwHeight = ysize;

    if ((r = lpDD()->lpVtbl->CreateSurface(lpDD(), &desc, &surf, NULL)) != 0) {
        directx_result_diagnostic_dispatch(r, 3,
            // STRING: LEGORR 0x004aae84
            "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Images.c", 1140);
        return;
    }

    rect.left = 0;
    rect.top = 0;
    rect.right = xsize;
    rect.bottom = ysize;
    surf->lpVtbl->Blt(surf, &rect, DirectDraw_bSurf(), NULL, 0, NULL);

    Image_InitFromSurface(image, surf, xsize, ysize, 0, 0);
}

// FUNCTION: LEGORR 0x0047e3e0
void Image_InitFromSurface(Image* newImage, Image_SurfaceIface* surface,
                           unsigned int width, unsigned int height,
                           unsigned int penZero, unsigned int pen255)
{
    if (imageGlobs.freeList == NULL) Image_AddList();

    newImage->width = width;
    newImage->nextFree = newImage;
    newImage->flags = 0;
    newImage->height = height;
    newImage->surface = surface;
    newImage->penZeroRGB = penZero;
    newImage->penZero = Image_DDColorMatch(surface, penZero);
    newImage->pen255 = Image_DDColorMatch(surface, pen255);
}

// FUNCTION: LEGORR 0x0047e440
int Image_SaveBMP(Image* image, const char* fname)
{
    return directdraw_save_bmp((struct DirectDrawSurfaceIface*)image->surface, fname);
}

#ifdef __cplusplus
}
#endif

#endif /* !LEGORRR_BACKEND_MINIWIN */
