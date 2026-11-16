// SPDX-License-Identifier: LGPL-3.0-or-later
/* Native port only: a DirectDraw surface whose every method fails with
 * DDERR_SURFACELOST. draw_port.cpp's default back buffer is one, and tests
 * derive from it and override the methods draw.c calls (GetSurfaceDesc,
 * Lock and Unlock). */
#ifndef LEGORRR_GODS98_DRAW_PORT_HPP
#define LEGORRR_GODS98_DRAW_PORT_HPP

#include <miniwin/ddraw.h>

struct DrawPortLostSurface : public IDirectDrawSurface4 {
    // IDirectDrawSurface
    HRESULT AddAttachedSurface(LPDIRECTDRAWSURFACE) override { return DDERR_SURFACELOST; }
    HRESULT Blt(LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DDBltFlags, LPDDBLTFX) override
    {
        return DDERR_SURFACELOST;
    }
    HRESULT BltFast(DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DDBltFastFlags) override
    {
        return DDERR_SURFACELOST;
    }
    HRESULT Flip(LPDIRECTDRAWSURFACE, DDFlipFlags) override { return DDERR_SURFACELOST; }
    HRESULT GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE*) override { return DDERR_SURFACELOST; }
    HRESULT GetDC(HDC*) override { return DDERR_SURFACELOST; }
    HRESULT GetPalette(LPDIRECTDRAWPALETTE*) override { return DDERR_SURFACELOST; }
    HRESULT GetPixelFormat(LPDDPIXELFORMAT) override { return DDERR_SURFACELOST; }
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC) override { return DDERR_SURFACELOST; }
    HRESULT IsLost() override { return DDERR_SURFACELOST; }
    HRESULT Lock(LPRECT, LPDDSURFACEDESC, DDLockFlags, HANDLE) override { return DDERR_SURFACELOST; }
    HRESULT ReleaseDC(HDC) override { return DDERR_SURFACELOST; }
    HRESULT Restore() override { return DDERR_SURFACELOST; }
    HRESULT SetClipper(LPDIRECTDRAWCLIPPER) override { return DDERR_SURFACELOST; }
    HRESULT SetColorKey(DDColorKeyFlags, LPDDCOLORKEY) override { return DDERR_SURFACELOST; }
    HRESULT SetPalette(LPDIRECTDRAWPALETTE) override { return DDERR_SURFACELOST; }
    HRESULT Unlock(LPVOID) override { return DDERR_SURFACELOST; }
    // IDirectDrawSurface4
    HRESULT Blt(LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DDBltFlags, LPDDBLTFX) override
    {
        return DDERR_SURFACELOST;
    }
    HRESULT Flip(LPDIRECTDRAWSURFACE4, DDFlipFlags) override { return DDERR_SURFACELOST; }
    HRESULT GetAttachedSurface(LPDDSCAPS2, LPDIRECTDRAWSURFACE4*) override { return DDERR_SURFACELOST; }
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC2) override { return DDERR_SURFACELOST; }
    HRESULT Lock(LPRECT, LPDDSURFACEDESC2, DDLockFlags, HANDLE) override { return DDERR_SURFACELOST; }
    HRESULT Unlock(LPRECT) override { return DDERR_SURFACELOST; }
};

#endif /* LEGORRR_GODS98_DRAW_PORT_HPP */
