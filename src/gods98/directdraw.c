// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-010 (text 0047c170-0047d410; .data strings
 * 004aae64-004aae7c; the common directDrawGlobs at 0076ac80; findings 505
 * and 596): gods98's DirectDraw.c. It enumerates the DirectDraw drivers,
 * their Direct3D devices and display modes, sets up the display (the
 * DirectDraw object, the front and back buffers and their clippers), flips
 * and clears the buffers, saves a surface as a bitmap, and converts
 * palettised surfaces. The binary attests none of the names; the functions
 * use descriptive DirectDraw_* names in snake case.
 *
 * The unit calls miniwin's DirectDraw and Direct3D interfaces (ADR 0007). */
#include "directdraw.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <miniwin/windows.h>

#include "crt/vc5names.h"
#include "files.h"
#include "images.h"
#include "directdraw_imports.h"
#include "gods98/main_globs.h" // [divergence:widescreen] appWidth, outside the 4:3 frame
#include "draw.h"                // [divergence:widescreen] Draw.c's clip window, for the 4:3 frame

#include <miniwin/d3d.h>
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>

/* The BMP file and info headers, packed (0x36 bytes). */
#pragma pack(1)
typedef struct DirectDrawBmpHeader {
    char bfType[2];
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} DirectDrawBmpHeader;
#pragma pack()

/* Main.c's device (lpDevice()). */
#define lpDevice() ((LPDIRECT3DRMDEVICE3)s_main_device)

static BOOL WINAPI directdraw_enum_driver_callback(GUID* lpGUID,
    LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext);
static HRESULT WINAPI directdraw_enum_device_callback(GUID* lpGuid,
    LPSTR lpDeviceDescription, LPSTR lpDeviceName,
    D3DDEVICEDESC* lpHWDesc, D3DDEVICEDESC* lpHELDesc,
    LPVOID lpContext);
static HRESULT WINAPI directdraw_enum_mode_callback(
    DDSURFACEDESC2* lpDDSurfaceDesc, LPVOID lpContext);

/* IID_IDirectDraw4 (0049f328) and IID_IDirect3D3 (0049f648) come from
 * miniwin's headers (the original links them from dxguid.lib). */

// GLOBAL: LEGORR 0x0076ac80
DirectDraw_Globs directDrawGlobs;

// FUNCTION: LEGORR 0x0047c170
void directdraw_initialise(void* hWnd)
{
    directDrawGlobs.driverCount = 0;
    directDrawGlobs.driverList = NULL;
    directDrawGlobs.deviceCount = 0;
    directDrawGlobs.deviceList = NULL;
    directDrawGlobs.modeCount = 0;
    directDrawGlobs.modeList = NULL;
    directDrawGlobs.hWnd = hWnd;
    directDrawGlobs.lpDirectDraw = NULL;
    directDrawGlobs.zSurf = NULL;
    directDrawGlobs.bSurf = NULL;
    directDrawGlobs.fSurf = NULL;
    directDrawGlobs.lpFrontClipper = NULL;
}

// FUNCTION: LEGORR 0x0047c1c0
int directdraw_enum_drivers(DirectDraw_Driver* list, unsigned int* count)
{
    directDrawGlobs.driverList = list;
    DirectDrawEnumerateA(directdraw_enum_driver_callback, NULL);
    *count = directDrawGlobs.driverCount;
    return TRUE;
}

// FUNCTION: LEGORR 0x0047c1f0
static BOOL WINAPI directdraw_enum_driver_callback(GUID* lpGUID,
    LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext)
{
    directDrawGlobs.driverList[directDrawGlobs.driverCount].flags =
        DIRECTDRAW_FLAG_VALID;
    if (lpGUID) directDrawGlobs.driverList[directDrawGlobs.driverCount].guid = *lpGUID;
    else directDrawGlobs.driverList[directDrawGlobs.driverCount].flags |=
        DIRECTDRAW_FLAG_DRIVER_PRIMARY;
    if (!lpGUID) directDrawGlobs.driverList[directDrawGlobs.driverCount].flags |=
        DIRECTDRAW_FLAG_DRIVER_WINDOWOK;
    snprintf(directDrawGlobs.driverList[directDrawGlobs.driverCount].desc, sizeof(directDrawGlobs.driverList[directDrawGlobs.driverCount].desc),
            "%s (%s)", lpDriverDescription, lpDriverName);
    directDrawGlobs.driverCount++;
    return TRUE;
}

// FUNCTION: LEGORR 0x0047c2e0
int directdraw_enum_devices(const DirectDraw_Driver* driver,
                            DirectDraw_Device* list, unsigned int* count)
{
    IDirectDraw* lpDD;
    IDirectDraw4* lpDD4;
    int res = FALSE;

    directDrawGlobs.deviceCount = 0;
    if (DirectDrawCreate((DirectDraw_Guid*)&driver->guid, &lpDD, NULL) == DD_OK) {
        if (lpDD->QueryInterface(IID_IDirectDraw4,
                                         (void**)&lpDD4) == DD_OK) {
            IDirect3D3* lpD3D;
            if (lpDD4->QueryInterface(IID_IDirect3D3,
                                              (void**)&lpD3D) == DD_OK) {
                directDrawGlobs.deviceList = list;
                lpD3D->EnumDevices(directdraw_enum_device_callback,
                                           NULL);
                res = TRUE;
                lpD3D->Release();
            }
            lpDD4->Release();
        }
        lpDD->Release();
    }
    *count = directDrawGlobs.deviceCount;
    return res;
}

// FUNCTION: LEGORR 0x0047c380
static HRESULT WINAPI directdraw_enum_device_callback(GUID* lpGuid,
    LPSTR lpDeviceDescription, LPSTR lpDeviceName,
    D3DDEVICEDESC* lpHWDesc, D3DDEVICEDESC* lpHELDesc,
    LPVOID lpContext)
{
    DirectDraw_Device* dev = &directDrawGlobs.deviceList[directDrawGlobs.deviceCount];
    D3DDEVICEDESC* desc;

    dev->flags = DIRECTDRAW_FLAG_VALID;
    if (lpHWDesc->dcmColorModel != D3DCOLORMODEL::NONE) {
        dev->flags = DIRECTDRAW_FLAG_VALID | DIRECTDRAW_FLAG_DEVICE_HARDWARE;
        desc = lpHWDesc;
    } else desc = lpHELDesc;

    if ((desc->dwFlags & D3DDD_COLORMODEL) == D3DDD_COLORMODEL && desc->dcmColorModel == D3DCOLOR_RGB) {
        dev->flags |= DIRECTDRAW_FLAG_DEVICE_COLOUR;
    }
    if ((desc->dwFlags & D3DDD_DEVICERENDERBITDEPTH) == D3DDD_DEVICERENDERBITDEPTH) {
        if ((desc->dwDeviceRenderBitDepth & DDBD_8) == DDBD_8) dev->flags |= DIRECTDRAW_FLAG_DEVICE_DEPTH8;
        if ((desc->dwDeviceRenderBitDepth & DDBD_16) == DDBD_16) dev->flags |= DIRECTDRAW_FLAG_DEVICE_DEPTH16;
        if ((desc->dwDeviceRenderBitDepth & DDBD_24) == DDBD_24) dev->flags |= DIRECTDRAW_FLAG_DEVICE_DEPTH24;
        if ((desc->dwDeviceRenderBitDepth & DDBD_32) == DDBD_32) dev->flags |= DIRECTDRAW_FLAG_DEVICE_DEPTH32;
    }
    if ((desc->dwFlags & D3DDD_DEVCAPS) == D3DDD_DEVCAPS) {
        if (desc->dwDevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) dev->flags |= DIRECTDRAW_FLAG_DEVICE_VIDEOTEXTURE;
        else if (desc->dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) dev->flags |= DIRECTDRAW_FLAG_DEVICE_SYSTEMTEXTURE;
    }

    dev->guid = *lpGuid;
    snprintf(dev->desc, sizeof(dev->desc), "%s (%s)", lpDeviceName, lpDeviceDescription);
    directDrawGlobs.deviceCount++;
    return 1; /* D3DENUMRET_OK */
}

// FUNCTION: LEGORR 0x0047c4b0
int directdraw_enum_modes(const DirectDraw_Driver* driver, int fullScreen,
                          DirectDraw_Mode* list, unsigned int* count)
{
    IDirectDraw* lpDD;
    int res = FALSE;
    DirectDraw_Guid* guid;

    directDrawGlobs.modeCount = 0;
    if (driver && (driver->flags & DIRECTDRAW_FLAG_VALID)) {
        if (driver->flags & DIRECTDRAW_FLAG_DRIVER_PRIMARY) guid = NULL;
        else guid = (DirectDraw_Guid*)&driver->guid;
        if (DirectDrawCreate(guid, &lpDD, NULL) == DD_OK) {
            IDirectDraw4* lpDD4;
            if (lpDD->QueryInterface(IID_IDirectDraw4,
                                             (void**)&lpDD4) == DD_OK) {
                directDrawGlobs.modeList = list;
                lpDD4->EnumDisplayModes(0, NULL, &fullScreen,
                                                directdraw_enum_mode_callback);
                res = TRUE;
                lpDD4->Release();
            }
            lpDD->Release();
        }
    }
    *count = directDrawGlobs.modeCount;
    return res;
}

// FUNCTION: LEGORR 0x0047c550
static HRESULT WINAPI directdraw_enum_mode_callback(
    DDSURFACEDESC2* lpDDSurfaceDesc, LPVOID lpContext)
{
    DirectDraw_Mode* mode = &directDrawGlobs.modeList[directDrawGlobs.modeCount];

    mode->flags = DIRECTDRAW_FLAG_VALID;
    mode->width = lpDDSurfaceDesc->dwWidth;
    mode->height = lpDDSurfaceDesc->dwHeight;
    mode->bitDepth = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

    // STRING: LEGORR 0x004aae6c
    if (*(int*)lpContext) snprintf(mode->desc, sizeof(mode->desc), "%ix%i (%i bit)", lpDDSurfaceDesc->dwWidth, lpDDSurfaceDesc->dwHeight, lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount);
    // STRING: LEGORR 0x004aae64
    else snprintf(mode->desc, sizeof(mode->desc), "%ix%i", lpDDSurfaceDesc->dwWidth, lpDDSurfaceDesc->dwHeight);

    if (!*(int*)lpContext && directDrawGlobs.modeCount &&
        mode->bitDepth != (unsigned int)window_runtime_display_caps_query()) {
        mode->flags &= ~DIRECTDRAW_FLAG_VALID;
        return 1; /* DDENUMRET_OK */
    }
    directDrawGlobs.modeCount++;
    return 1; /* DDENUMRET_OK */
}

// FUNCTION: LEGORR 0x0047c610
int directdraw_setup(int fullScreen, const DirectDraw_Driver* driver,
                     const DirectDraw_Device* device,
                     const DirectDraw_Mode* mode, unsigned int xPos,
                     unsigned int yPos, unsigned int width,
                     unsigned int height)
{
    DDSURFACEDESC2 ddsd;
    IDirectDraw* lpDD;
    DirectDraw_Guid* guid = NULL;
    unsigned int bpp = 16;
    HRESULT r;

    if (driver && !(driver->flags & DIRECTDRAW_FLAG_VALID)) driver = NULL;
    if (device && !(device->flags & DIRECTDRAW_FLAG_VALID)) device = NULL;
    if (mode && !(mode->flags & DIRECTDRAW_FLAG_VALID)) mode = NULL;
    if (mode) {
        width = mode->width;
        height = mode->height;
        bpp = mode->bitDepth;
    }

    directDrawGlobs.width = width;
    directDrawGlobs.height = height;
    directDrawGlobs.fullScreen = fullScreen;
    if (driver) guid = (driver->flags & DIRECTDRAW_FLAG_DRIVER_PRIMARY) ? NULL : (DirectDraw_Guid*)&driver->guid;

    renderer_window_mode_apply(fullScreen, xPos, yPos, width, height);

    if (DirectDrawCreate(guid, &lpDD, NULL) == DD_OK) {
        if (lpDD->QueryInterface(IID_IDirectDraw4, (void**)&directDrawGlobs.lpDirectDraw) == DD_OK) {
            if (directDrawGlobs.lpDirectDraw->SetCooperativeLevel((HWND)directDrawGlobs.hWnd, fullScreen ? DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE : DDSCL_NORMAL) == DD_OK) {
                if (fullScreen) r = directDrawGlobs.lpDirectDraw->SetDisplayMode(width, height, bpp, 0, 0);
                else r = DD_OK;
                if (r == DD_OK) {
                    memset(&ddsd, 0, sizeof(ddsd));
                    ddsd.dwSize = sizeof(ddsd);
                    ddsd.dwFlags = DDSD_CAPS;
                    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
                    if (fullScreen) {
                        ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
                        ddsd.dwBackBufferCount = 1;
                        ddsd.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
                    }
                    if (directDrawGlobs.lpDirectDraw->CreateSurface(&ddsd, &directDrawGlobs.fSurf, NULL) == DD_OK) {
                        if (!fullScreen) {
                            ddsd.ddsCaps.dwCaps &= ~DDSCAPS_PRIMARYSURFACE;
                            ddsd.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
                            ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
                            ddsd.dwWidth = width;
                            ddsd.dwHeight = height;
                            r = directDrawGlobs.lpDirectDraw->CreateSurface(&ddsd, &directDrawGlobs.bSurf, NULL);
                        } else {
                            DDSCAPS2 ddscaps;
                            memset(&ddscaps, 0, sizeof(ddscaps));
                            ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
                            r = directDrawGlobs.fSurf->GetAttachedSurface(&ddscaps, &directDrawGlobs.bSurf);
                        }
                        if (r == DD_OK) {
                            if (directdraw_create_clipper(fullScreen, width, height) &&
                                direct3drm_device_initialize((DirectDraw_Device*)device, lpDD, directDrawGlobs.bSurf, fullScreen)) {
                                lpDD->Release();
                                if (fullScreen) ShowCursor(FALSE);
                                return TRUE;
                            }
                            /* The attempt may have made one or both clippers
                             * before it failed. Init.c can set the display up
                             * again on the next device, which would overwrite
                             * the pointers and leak them. */
                            if (directDrawGlobs.lpFrontClipper) {
                                directDrawGlobs.lpFrontClipper->Release();
                                directDrawGlobs.lpFrontClipper = NULL;
                            }
                            if (directDrawGlobs.lpBackClipper) {
                                directDrawGlobs.lpBackClipper->Release();
                                directDrawGlobs.lpBackClipper = NULL;
                            }
                            directDrawGlobs.bSurf->Release();
                            directDrawGlobs.bSurf = NULL;
                        }
                        directDrawGlobs.fSurf->Release();
                        directDrawGlobs.fSurf = NULL;
                    }
                }
            }
            directDrawGlobs.lpDirectDraw->Release();
            directDrawGlobs.lpDirectDraw = NULL;
        }
        lpDD->Release();
    }
    return FALSE;
}

/* [divergence:widescreen] PORT-ONLY: the 4:3 frame. A mode wider than 4:3
 * (Init.c, tweaks:widescreen) is drawn at its full width in a level. The
 * screens around the levels (the front end, the rewards, the loading and
 * shutdown screens, the movies) are 640x480 pictures. While one is up, the
 * game draws into a frame of the mode's height and 4:3 width, an offscreen
 * surface that stands in for the back buffer (directDrawGlobs.bSurf), and
 * the flip shows it centred, with black at the sides. The mouse reads in
 * the frame's coordinates (Input.c). The display's size (Main.c's appWidth
 * and appHeight) stays the mode's, which the level's interface is laid out
 * for when the game starts; a 640x480 screen that measures itself by the
 * display's width reads directdraw_frame_width instead.
 *
 * The 3D device draws on the back buffer, not the frame. A screen that
 * renders 3D over its picture (the front end's rock wipe) has the frame put
 * on the back buffer first (directdraw_frame_flush, from Viewport_Render);
 * what it draws after that goes on top, keyed. */
static unsigned int s_frameWidth = 0;            /* the width Init.c widened the mode from; 0: not widened */
static IDirectDrawSurface4* s_frameSurf = NULL;  /* the frame */
static IDirectDrawSurface4* s_screenSurf = NULL; /* the back buffer, while framed */
static int s_frameFlushed = FALSE;               /* the frame is on the back buffer this frame */
#define DIRECTDRAW_FRAME_KEY 0x00ff00ffu         /* what the frame holds after a flush */

void directdraw_set_frame_width(unsigned int width)
{
    s_frameWidth = width;
}

/* The frame's width: the width the mode was widened from, or the mode's
 * width when it was not widened. */
static unsigned int directdraw_frame_width_for_mode(void)
{
    return s_frameWidth != 0 && s_frameWidth < directDrawGlobs.width ? s_frameWidth : directDrawGlobs.width;
}

int directdraw_set_framed(int framed)
{
    DDSURFACEDESC2 ddsd;
    unsigned int width = directdraw_frame_width_for_mode();
    int wasFramed = s_screenSurf != NULL;

    if (framed && s_screenSurf == NULL && width < directDrawGlobs.width && directDrawGlobs.bSurf != NULL) {
        if (s_frameSurf == NULL) {
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
            ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
            ddsd.dwWidth = width;
            ddsd.dwHeight = directDrawGlobs.height;
            if (directDrawGlobs.lpDirectDraw->CreateSurface(&ddsd, &s_frameSurf, NULL) != DD_OK) {
                s_frameSurf = NULL;
                return wasFramed;
            }
        }
        s_screenSurf = directDrawGlobs.bSurf;
        directDrawGlobs.bSurf = s_frameSurf;
        s_frameFlushed = FALSE;
        directdraw_clear(NULL, 0);
        viewport_subrectangle_set(NULL); /* Draw.c's clip window: the frame */
    } else if (!framed && s_screenSurf != NULL) {
        directDrawGlobs.bSurf = s_screenSurf;
        s_screenSurf = NULL;
        directdraw_clear(NULL, 0);
        viewport_subrectangle_set(NULL); /* Draw.c's clip window: the screen */
    }
    return wasFramed;
}

int directdraw_is_framed(void)
{
    return s_screenSurf != NULL;
}

int directdraw_frame_width(void)
{
    if (s_screenSurf == NULL) return s_display_w;
    return (int)directdraw_frame_width_for_mode();
}

int directdraw_frame_x(void)
{
    if (s_screenSurf == NULL) return 0;
    return (int)(directDrawGlobs.width - directdraw_frame_width_for_mode()) / 2;
}

int directdraw_widescreen_margin(void)
{
    return (int)(directDrawGlobs.width - directdraw_frame_width_for_mode());
}

static void directdraw_frame_fill(IDirectDrawSurface4* surf, unsigned int colour)
{
    DDBLTFX ddbltfx;

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = directdraw_get_colour(surf, colour);
    surf->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
}

/* Draws the frame on the back buffer, centred: over black, or keyed over
 * what is there. */
static void directdraw_frame_draw(int keyed)
{
    DDCOLORKEY key;
    RECT dest;
    int x = directdraw_frame_x();

    if (!keyed) directdraw_frame_fill(s_screenSurf, 0);
    dest.left = x;
    dest.top = 0;
    dest.right = x + (LONG)directdraw_frame_width_for_mode();
    dest.bottom = (LONG)directDrawGlobs.height;
    if (keyed) {
        key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = directdraw_get_colour(s_frameSurf, DIRECTDRAW_FRAME_KEY);
        s_frameSurf->SetColorKey(DDCKEY_SRCBLT, &key);
        s_screenSurf->Blt(&dest, s_frameSurf, NULL, DDBLT_KEYSRC | DDBLT_WAIT, NULL);
        s_frameSurf->SetColorKey(DDCKEY_SRCBLT, NULL);
    } else {
        s_screenSurf->Blt(&dest, s_frameSurf, NULL, DDBLT_WAIT, NULL);
    }
}

void directdraw_frame_flush(void)
{
    if (s_screenSurf == NULL || s_frameFlushed) return;
    directdraw_frame_draw(FALSE);
    directdraw_frame_fill(s_frameSurf, DIRECTDRAW_FRAME_KEY);
    s_frameFlushed = TRUE;
}

/* Puts the frame on the back buffer before a flip. */
static void directdraw_frame_present(void)
{
    if (s_screenSurf == NULL) return;
    directdraw_frame_draw(s_frameFlushed);
    if (s_frameFlushed) {
        /* The next frame starts black, as one that was never flushed. */
        directdraw_frame_fill(s_frameSurf, 0);
        s_frameFlushed = FALSE;
    }
    directDrawGlobs.bSurf = s_screenSurf;
}

// FUNCTION: LEGORR 0x0047c8d0
void directdraw_flip(void)
{
    directdraw_frame_present(); // [divergence:widescreen]
    if (directDrawGlobs.fullScreen) {
        directDrawGlobs.fSurf->Flip(NULL, DDFLIP_WAIT);
    } else {
        directdraw_blit_buffers();
    }
    if (s_screenSurf != NULL) directDrawGlobs.bSurf = s_frameSurf; // [divergence:widescreen]
}

// FUNCTION: LEGORR 0x0047c8f0
int directdraw_save_bmp(IDirectDrawSurface4* surface, const char* fname)
{
    unsigned int bBits = 0;
    DirectDrawBmpHeader header = { 0 };
    DDSURFACEDESC2 desc;
    File* ofp;
    unsigned int lineSize, size, bytesPerPixel;
    unsigned int rBits = 0, gBits = 0;
    unsigned int x, y, i, offset, out;
    unsigned char* buffer;
    unsigned int value;
    int success = FALSE;
    if ((ofp = file_open(fname, "wb")) != NULL) {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        if (surface->Lock(NULL, &desc, DDLOCK_WAIT, NULL) == DD_OK) {
            /* The row offsets below are y * pitch with an unsigned y: a negative
             * (bottom-up) pitch wrapped to the right row on Win32 but lands ~4 GB
             * away on 64-bit. miniwin's pitch is always positive; stop if not. */
            if (desc.lPitch < 0) {
                fprintf(stderr, "DirectDraw_SaveBMP: negative surface pitch %d\n", (int)desc.lPitch);
                abort();
            }
            lineSize = desc.dwWidth * 3;
            lineSize = (lineSize / 4) * 4 + ((lineSize % 4) ? 4 : 0);
            size = lineSize * desc.dwHeight;
            if ((buffer = (unsigned char*)malloc(size)) != NULL) {
                memset(buffer, 0, size);
                bytesPerPixel = desc.ddpfPixelFormat.dwRGBBitCount / 8;
                for (i = 0; i < 32; i++) if ((desc.ddpfPixelFormat.dwRBitMask >> i) & 1) rBits++;
                for (i = 0; i < 32; i++) if ((desc.ddpfPixelFormat.dwGBitMask >> i) & 1) gBits++;
                for (i = 0; i < 32; i++) if ((desc.ddpfPixelFormat.dwBBitMask >> i) & 1) bBits++;
                for (y = 0; y < desc.dwHeight; y++) {
                    for (x = 0, offset = 0, out = 0; x < desc.dwWidth; x++) {
                        out += 3;
                        value = *(unsigned int*)((unsigned char*)desc.lpSurface + y * desc.lPitch + offset);
                        buffer[(desc.dwHeight - y - 1) * lineSize + out - 1] = (unsigned char)((value >> (gBits + bBits)) << (8 - rBits));
                        buffer[(desc.dwHeight - y - 1) * lineSize + out - 2] = (unsigned char)((value >> bBits) << (8 - gBits));
                        buffer[(desc.dwHeight - y - 1) * lineSize + out - 3] = (unsigned char)(value << (8 - bBits));
                        offset += bytesPerPixel;
                    }
                }
                success = TRUE;
            }
            surface->Unlock(NULL);

            header.bfType[0] = 'B';
            header.bfType[1] = 'M';
            header.bfSize = size + sizeof(header);
            header.bfOffBits = sizeof(header);
            header.biCompression = 0;
            header.biSize = 40;
            header.biWidth = desc.dwWidth;
            header.biHeight = desc.dwHeight;
            header.biPlanes = 1;
            header.biBitCount = 24;
            header.biSizeImage = 0;
            header.biXPelsPerMeter = 1;
            header.biYPelsPerMeter = 1;
            header.biClrUsed = 0;
            header.biClrImportant = 0;
            if (success) {
                file_write(&header, sizeof(header), 1, ofp);
                file_write(buffer, size, 1, ofp);
            }
            free(buffer);
        }
        file_close(ofp);
    }
    return success;
}

// FUNCTION: LEGORR 0x0047cc20
void directdraw_return_front_buffer(void)
{
    RECT centre; // [divergence:widescreen]

    if (directDrawGlobs.fullScreen) {
        /* [divergence:widescreen] Into the 4:3 frame, the screen's centre
         * at 4:3, where the frame was shown. */
        centre.left = directdraw_frame_x();
        centre.top = 0;
        centre.right = centre.left + (LONG)directdraw_frame_width_for_mode();
        centre.bottom = (LONG)directDrawGlobs.height;
        directDrawGlobs.bSurf->Blt(NULL, directDrawGlobs.fSurf, s_screenSurf != NULL ? &centre : NULL, DDBLT_WAIT, NULL);
    }
}

// FUNCTION: LEGORR 0x0047cc50
void directdraw_blit_buffers(void)
{
    POINT pt;
    RECT dest;
    RECT src;

    dest.right = directDrawGlobs.width;
    pt.y = 0;
    pt.x = 0;
    dest.left = 0;
    dest.top = 0;
    dest.bottom = directDrawGlobs.height;
    ClientToScreen((HWND)directDrawGlobs.hWnd, &pt);
    OffsetRect(&dest, pt.x, pt.y);
    src.left = 0;
    src.top = 0;
    src.right = directDrawGlobs.width;
    src.bottom = directDrawGlobs.height;
    directDrawGlobs.fSurf->Blt(&dest, directDrawGlobs.bSurf, &src, DDBLT_WAIT, NULL);
}

// FUNCTION: LEGORR 0x0047ccf0
void directdraw_shutdown(void)
{
    /* [divergence:widescreen] The back buffer again, and the frame freed. */
    directdraw_set_framed(FALSE);
    if (s_frameSurf) {
        s_frameSurf->Release();
        s_frameSurf = NULL;
    }
    if (directDrawGlobs.fSurf) directDrawGlobs.fSurf->Release();
    if (directDrawGlobs.lpFrontClipper) directDrawGlobs.lpFrontClipper->Release();
    if (directDrawGlobs.lpBackClipper) directDrawGlobs.lpBackClipper->Release();
    if (directDrawGlobs.lpDirectDraw) {
        if (directDrawGlobs.fullScreen) directDrawGlobs.lpDirectDraw->RestoreDisplayMode();
        if (directDrawGlobs.lpDirectDraw) directDrawGlobs.lpDirectDraw->Release();
    }
}

/* Main.c's mainGlobs fields under the port's names (finding 679). */
#include "gods98/main_globs.h"

// FUNCTION: LEGORR 0x0047cd50
void directdraw_adjust_texture_usage(unsigned int* textureUsage)
{
    DDPIXELFORMAT pf;

    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    if (lpDevice()->FindPreferredTextureFormat(DDBD_8, D3DRMFPTF_PALETTIZED, &pf) != DD_OK) {
        memset(&pf, 0, sizeof(pf));
        pf.dwSize = sizeof(pf);
        if (lpDevice()->FindPreferredTextureFormat(DDBD_16, 0, &pf) == DD_OK) {
            *textureUsage *= (pf.dwRGBBitCount / 8);
        }
    }
}

// FUNCTION: LEGORR 0x0047cdd0
int directdraw_get_avail_texture_mem(unsigned int* total, unsigned int* avail)
{
    DDSCAPS2 ddsc;
    DWORD totalMem = 0;
    DWORD availMem = 0;
    HRESULT result;

    memset(&ddsc, 0, sizeof(ddsc));
    ddsc.dwCaps = DDSCAPS_TEXTURE;
    result = directDrawGlobs.lpDirectDraw->GetAvailableVidMem(&ddsc, &totalMem, &availMem);
    *total = totalMem;
    *avail = availMem;
    return result == DD_OK;
}

// FUNCTION: LEGORR 0x0047ce20
void directdraw_clear(Area2F* window, unsigned int colour)
{
    DDBLTFX ddbltfx;
    RECT rect;

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = directdraw_get_colour(directDrawGlobs.bSurf, colour);

    if (window) {
        rect.left = (LONG)window->x;
        rect.top = (LONG)window->y;
        rect.right = (LONG)(window->x + window->width);
        rect.bottom = (LONG)(window->y + window->height);
        directDrawGlobs.bSurf->Blt(&rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    } else {
        directDrawGlobs.bSurf->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    }
}

// FUNCTION: LEGORR 0x0047cee0
int directdraw_create_clipper(int fullScreen, unsigned int width, unsigned int height)
{
    HRGN handle;
    DWORD size;
    RGNDATA* region;

    if (directDrawGlobs.lpDirectDraw->CreateClipper(0, &directDrawGlobs.lpBackClipper, NULL) == DD_OK) {
        handle = CreateRectRgn(0, 0, width, height);
        size = GetRegionData(handle, 0, NULL);
        region = (RGNDATA*)malloc(size);
        GetRegionData(handle, size, region);
        /* The clip list holds its own copy of the rectangle. */
        DeleteObject(handle);
        if (directDrawGlobs.lpBackClipper->SetClipList(region, 0) == DD_OK) {
            free(region);
            if (directDrawGlobs.bSurf->SetClipper(directDrawGlobs.lpBackClipper) == DD_OK) {
                if (!fullScreen) {
                    if (directDrawGlobs.lpDirectDraw->CreateClipper(0, &directDrawGlobs.lpFrontClipper, NULL) == DD_OK) {
                        if (directDrawGlobs.lpFrontClipper->SetHWnd(0, (HWND)directDrawGlobs.hWnd) == DD_OK) {
                            if (directDrawGlobs.fSurf->SetClipper(directDrawGlobs.lpFrontClipper) == DD_OK) {
                                return TRUE;
                            }
                        }
                    }
                } else return TRUE;
            }
            directDrawGlobs.lpBackClipper->Release();
            directDrawGlobs.lpBackClipper = NULL;
        }
        /* The original frees the region data again after a failure past
         * SetClipList. */
        free(region);
    }
    return FALSE;
}

// FUNCTION: LEGORR 0x0047d000
void directdraw_blt_8_to_16(IDirectDrawSurface4* target, IDirectDrawSurface4* source, void* palette)
{
    DDSURFACEDESC2 descSource, descTarget;
    unsigned int x, colour, val, y;
    int i, shift, k, rBits, gBits, bBits;
    unsigned char r, g, b, index;

    memset(&descSource, 0, sizeof(descSource));
    memset(&descTarget, 0, sizeof(descTarget));
    descSource.dwSize = sizeof(descSource);
    descTarget.dwSize = sizeof(descTarget);
    if (source->Lock(NULL, &descSource, DDLOCK_WAIT, NULL) == DD_OK) {
        if (descSource.ddpfPixelFormat.dwRGBBitCount == 8 &&
            target->Lock(NULL, &descTarget, DDLOCK_WAIT, NULL) == DD_OK) {
            if (descTarget.ddpfPixelFormat.dwRGBBitCount == 16 &&
                descSource.dwWidth == descTarget.dwWidth &&
                descTarget.dwHeight == descSource.dwHeight) {
                for (i = 0, rBits = 0; i < 32; i++) if ((descTarget.ddpfPixelFormat.dwRBitMask >> i) & 1) rBits++;
                for (i = 0, gBits = 0; i < 32; i++) if ((descTarget.ddpfPixelFormat.dwGBitMask >> i) & 1) gBits++;
                for (i = 0, bBits = 0; i < 32; i++) if ((descTarget.ddpfPixelFormat.dwBBitMask >> i) & 1) bBits++;
                for (y = 0; y < descSource.dwHeight; y++) {
                    for (x = 0; x < descSource.dwWidth; x += 4) {
                        val = *(unsigned int*)((unsigned char*)descSource.lpSurface + y * descSource.lPitch + x);
                        for (shift = 24, k = 3; shift >= 0; shift -= 8, k--) {
                            index = (unsigned char)(val >> shift);
                            r = ((PALETTEENTRY*)palette)[index].peRed >> (8 - rBits);
                            g = ((PALETTEENTRY*)palette)[index].peGreen >> (8 - gBits);
                            b = ((PALETTEENTRY*)palette)[index].peBlue >> (8 - bBits);
                            if (k & 1) {
                                colour = (r << (gBits + bBits) | g << bBits | b) << 16;
                            } else {
                                colour |= r << (gBits + bBits) | g << bBits | b;
                                *(unsigned int*)((unsigned char*)descTarget.lpSurface + y * descTarget.lPitch + (x + ((k >> 1) & 1) * 2) * 2) = colour;
                            }
                        }
                    }
                }
            }
            target->Unlock(NULL);
        }
        source->Unlock(NULL);
    }
}

// FUNCTION: LEGORR 0x0047d2d0
unsigned int directdraw_get_colour(IDirectDrawSurface4* surf, unsigned int colour)
{
    PALETTEENTRY entries[256];
    DDPIXELFORMAT pf;
    IDirectDrawPalette* pal;
    unsigned int r = (colour >> 16) & 0xff, g = (colour >> 8) & 0xff, b = colour & 0xff;
    unsigned int rBits, gBits, bBits, i;

    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    surf->GetPixelFormat(&pf);
    if ((pf.dwFlags & DDPF_RGB) == DDPF_RGB) {
        rBits = directdraw_count_mask_bits(pf.dwRBitMask);
        gBits = directdraw_count_mask_bits(pf.dwGBitMask);
        bBits = directdraw_count_mask_bits(pf.dwBBitMask);
        return ((r >> (8 - rBits)) << (gBits + bBits)) | ((g >> (8 - gBits)) << bBits) | (b >> (8 - bBits));
    }
    surf->GetPalette(&pal);
    pal->GetEntries(0, 0, 256, entries);
    for (i = 0; i < 256; i++) {
        if (entries[i].peRed == r && entries[i].peGreen == g && entries[i].peBlue == b) return i;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047d3f0
unsigned int directdraw_count_mask_bits(unsigned int mask)
{
    unsigned int count = 0;
    while (mask) {
        mask &= mask - 1;
        count++;
    }
    return count;
}

