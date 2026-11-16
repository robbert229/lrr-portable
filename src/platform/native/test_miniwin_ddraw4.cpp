// SPDX-License-Identifier: LGPL-3.0-or-later
// The miniwin behaviour the units' boot to the main menu relies on
// (DirectDraw.c, Images.c, Init.c, Container.c): display modes on the
// emulated 16-bit display, IDirectDraw4/IDirectDrawSurface4 surfaces, Lock,
// fills and colour-keyed blits, the framebuffer and its flip, the software
// device described as HAL, and IDirect3DRMFrame3 transforms.
// Synthetic only: an offscreen SDL window, no game data.
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include <math.h>
#include <stdio.h>
#include <vector>

static int failures;

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) {                                                     \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,         \
                    __LINE__, #expr);                                      \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static bool Near(float a, float b)
{
    return fabsf(a - b) < 1e-4f;
}

static bool NearVector(const D3DVECTOR& v, float x, float y, float z)
{
    return Near(v.x, x) && Near(v.y, y) && Near(v.z, z);
}

// ---- display modes ----------------------------------------------------

struct Mode {
    DWORD width, height, bpp, rMask, gMask, bMask;
};

struct ModeList {
    std::vector<Mode> modes;
    int stopAfter = 0;
};

static HRESULT CollectMode(LPDDSURFACEDESC2 desc, LPVOID context)
{
    ModeList* list = static_cast<ModeList*>(context);
    list->modes.push_back({desc->dwWidth, desc->dwHeight, desc->ddpfPixelFormat.dwRGBBitCount,
                           desc->ddpfPixelFormat.dwRBitMask, desc->ddpfPixelFormat.dwGBitMask,
                           desc->ddpfPixelFormat.dwBBitMask});
    if (list->stopAfter && (int) list->modes.size() >= list->stopAfter) {
        return DDENUMRET_CANCEL;
    }
    return DDENUMRET_OK;
}

static void CheckDisplayModes(IDirectDraw4* dd)
{
    // This build emulates a 16-bit RGB565 desktop (MINIWIN_PIXELFORMAT).
    CHECK(GetDeviceCaps(nullptr, BITSPIXEL) == 16);
    CHECK(GetDeviceCaps(nullptr, PLANES) == 1);

    ModeList all;
    CHECK(dd->EnumDisplayModes(0, nullptr, &all, CollectMode) == DD_OK);
    bool has640 = false;
    for (size_t i = 0; i < all.modes.size(); i++) {
        const Mode& m = all.modes[i];
        CHECK(m.bpp == 16 && m.rMask == 0xF800 && m.gMask == 0x07E0 && m.bMask == 0x001F);
        if (m.width == 640 && m.height == 480) {
            has640 = true;
        }
        if (i > 0) {
            const Mode& prev = all.modes[i - 1];
            // Sorted and without duplicates.
            CHECK(prev.width < m.width || (prev.width == m.width && prev.height < m.height));
        }
    }
    CHECK(has640);

    ModeList filtered;
    DDSURFACEDESC2 filter = {};
    filter.dwSize = sizeof(filter);
    filter.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    filter.dwWidth = 800;
    filter.dwHeight = 600;
    CHECK(dd->EnumDisplayModes(0, &filter, &filtered, CollectMode) == DD_OK);
    CHECK(filtered.modes.size() == 1 && filtered.modes[0].width == 800);

    ModeList cancelled;
    cancelled.stopAfter = 2;
    CHECK(dd->EnumDisplayModes(0, nullptr, &cancelled, CollectMode) == DD_OK);
    CHECK(cancelled.modes.size() == 2);
}

// ---- surfaces -----------------------------------------------------------

static IDirectDrawSurface4* CreateOffscreen(IDirectDraw4* dd, DWORD width, DWORD height, DWORD bpp = 0)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    desc.dwWidth = width;
    desc.dwHeight = height;
    if (bpp == 8) {
        desc.dwFlags = desc.dwFlags | DDSD_PIXELFORMAT;
        desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
        desc.ddpfPixelFormat.dwRGBBitCount = 8;
    }
    IDirectDrawSurface4* surface = nullptr;
    if (dd->CreateSurface(&desc, &surface, nullptr) != DD_OK) {
        return nullptr;
    }
    return surface;
}

static void Fill(IDirectDrawSurface4* surface, DWORD colour, RECT* rect = nullptr)
{
    DDBLTFX fx = {};
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = colour;
    CHECK(surface->Blt(rect, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fx) == DD_OK);
}

static Uint16 Pixel16(IDirectDrawSurface4* surface, int x, int y, DDLockFlags flags = DDLOCK_WAIT)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    if (surface->Lock(nullptr, &desc, flags, nullptr) != DD_OK) {
        return 0xDEAD;
    }
    Uint16 value = *(Uint16*) ((Uint8*) desc.lpSurface + y * desc.lPitch + x * 2);
    surface->Unlock(nullptr);
    return value;
}

static void CheckSurfaces(IDirectDraw4* dd)
{
    IDirectDrawSurface4* image = CreateOffscreen(dd, 8, 4);
    CHECK(image);
    if (!image) {
        return;
    }

    // A surface without a pixel format gets the display's: RGB565.
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    CHECK(image->GetSurfaceDesc(&desc) == DD_OK);
    CHECK(desc.dwWidth == 8 && desc.dwHeight == 4 && desc.lPitch >= 16);
    CHECK(desc.ddpfPixelFormat.dwRGBBitCount == 16 && desc.ddpfPixelFormat.dwRBitMask == 0xF800 &&
          desc.ddpfPixelFormat.dwGBitMask == 0x07E0 && desc.ddpfPixelFormat.dwBBitMask == 0x001F);

    // A 16-bit fill colour is a pixel value, not A8R8G8B8.
    Fill(image, 0xF800);
    CHECK(Pixel16(image, 0, 0) == 0xF800 && Pixel16(image, 7, 3) == 0xF800);
    RECT corner = {6, 2, 8, 4};
    Fill(image, 0x001F, &corner);
    CHECK(Pixel16(image, 7, 3) == 0x001F && Pixel16(image, 5, 3) == 0xF800);

    // Lock with a rectangle points at the rectangle's first pixel.
    DDSURFACEDESC2 locked = {};
    locked.dwSize = sizeof(locked);
    RECT at = {3, 2, 4, 3};
    CHECK(image->Lock(&at, &locked, DDLOCK_WAIT, nullptr) == DD_OK);
    if (locked.lpSurface) {
        *(Uint16*) locked.lpSurface = 0x1234;
        image->Unlock(nullptr);
    }
    CHECK(Pixel16(image, 3, 2) == 0x1234);

    // Colour keys apply only when the blit asks for them.
    IDirectDrawSurface4* sprite = CreateOffscreen(dd, 2, 2);
    IDirectDrawSurface4* target = CreateOffscreen(dd, 2, 2);
    CHECK(sprite && target);
    if (sprite && target) {
        Fill(sprite, 0x001F);
        RECT one = {1, 1, 2, 2};
        Fill(sprite, 0x07E0, &one);
        DDCOLORKEY key = {0x001F, 0x001F};
        CHECK(sprite->SetColorKey(DDCKEY_SRCBLT, &key) == DD_OK);

        Fill(target, 0xF800);
        CHECK(target->Blt(nullptr, sprite, nullptr, DDBLT_KEYSRC | DDBLT_WAIT, nullptr) == DD_OK);
        CHECK(Pixel16(target, 0, 0) == 0xF800 && Pixel16(target, 1, 1) == 0x07E0);

        Fill(target, 0xF800);
        CHECK(target->Blt(nullptr, sprite, nullptr, DDBLT_WAIT, nullptr) == DD_OK);
        CHECK(Pixel16(target, 0, 0) == 0x001F && Pixel16(target, 1, 1) == 0x07E0);

        Fill(target, 0xF800);
        RECT src = {0, 0, 2, 2};
        CHECK(target->BltFast(0, 0, sprite, &src, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT) == DD_OK);
        CHECK(Pixel16(target, 0, 0) == 0xF800 && Pixel16(target, 1, 1) == 0x07E0);

        Fill(target, 0xF800);
        CHECK(target->BltFast(0, 0, sprite, nullptr, DDBLTFAST_WAIT) == DD_OK);
        CHECK(Pixel16(target, 0, 0) == 0x001F);
    }
    if (sprite) {
        sprite->Release();
    }
    if (target) {
        target->Release();
    }

    // An 8-bit palettised surface: index fills and a palette.
    IDirectDrawSurface4* indexed = CreateOffscreen(dd, 4, 4, 8);
    CHECK(indexed);
    if (indexed) {
        PALETTEENTRY entries[256] = {};
        entries[7].peRed = 0x40;
        IDirectDrawPalette* palette = nullptr;
        CHECK(dd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, &palette, nullptr) == DD_OK);
        CHECK(palette && indexed->SetPalette(palette) == DD_OK);
        Fill(indexed, 7);
        DDSURFACEDESC2 desc8 = {};
        desc8.dwSize = sizeof(desc8);
        CHECK(indexed->Lock(nullptr, &desc8, DDLOCK_WAIT, nullptr) == DD_OK);
        CHECK(desc8.ddpfPixelFormat.dwRGBBitCount == 8);
        CHECK(desc8.lpSurface && ((Uint8*) desc8.lpSurface)[0] == 7);
        indexed->Unlock(nullptr);
        IDirectDrawPalette* got = nullptr;
        CHECK(indexed->GetPalette(&got) == DD_OK && got == palette);
        if (got) {
            PALETTEENTRY read = {};
            got->GetEntries(0, 7, 1, &read);
            CHECK(read.peRed == 0x40);
            got->Release();
        }
        if (palette) {
            palette->Release();
        }
        indexed->Release();
    }

    image->Release();
}

// ---- the framebuffer --------------------------------------------------

static HRESULT FindHalSoftwareDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC hal, LPD3DDEVICEDESC, LPVOID ctx)
{
    if (SDL_strcmp(name, "Miniwin Emulation") == 0) {
        // MINIWIN_SOFTWARE_HAL: the software device describes itself as HAL.
        CHECK(hal->dcmColorModel == D3DCOLOR_RGB);
        CHECK((hal->dwDeviceRenderBitDepth & DDBD_16) == DDBD_16);
        *static_cast<GUID*>(ctx) = *guid;
    }
    return DDENUMRET_OK;
}

static void CheckFrameBuffer(IDirectDraw4* dd, IDirectDraw* dd1, SDL_Window* window)
{
    CHECK(dd->SetCooperativeLevel(reinterpret_cast<HWND>(window), DDSCL_NORMAL) == DD_OK);

    // DirectDraw.c's windowed set-up: a primary surface and an offscreen 3D
    // device surface, which in miniwin are the same framebuffer.
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    IDirectDrawSurface4* front = nullptr;
    CHECK(dd->CreateSurface(&desc, &front, nullptr) == DD_OK && front);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = 64;
    desc.dwHeight = 64;
    IDirectDrawSurface4* back = nullptr;
    CHECK(dd->CreateSurface(&desc, &back, nullptr) == DD_OK && back);
    CHECK(front && front == back);
    if (!front || !back) {
        return;
    }
    DDSCAPS2 caps = {};
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    IDirectDrawSurface4* attached = nullptr;
    CHECK(front->GetAttachedSurface(&caps, &attached) == DD_OK && attached == front);
    if (attached) {
        attached->Release();
    }

    // The framebuffer is in the display format.
    DDSURFACEDESC2 fbDesc = {};
    fbDesc.dwSize = sizeof(fbDesc);
    CHECK(back->GetSurfaceDesc(&fbDesc) == DD_OK);
    CHECK(fbDesc.ddpfPixelFormat.dwRGBBitCount == 16 && fbDesc.dwWidth == 64 && fbDesc.dwHeight == 64);

    // Blits to the framebuffer need its renderer: Main.c's device set-up.
    IDirect3D2* d3d2 = nullptr;
    GUID software = {};
    CHECK(dd1->QueryInterface(IID_IDirect3D2, (void**) &d3d2) == DD_OK && d3d2);
    if (d3d2) {
        d3d2->EnumDevices(FindHalSoftwareDevice, &software);
        d3d2->Release();
    }
    IDirect3DRM* d3drm1 = nullptr;
    IDirect3DRM3* d3drm = nullptr;
    CHECK(Direct3DRMCreate(&d3drm1) == DD_OK && d3drm1);
    if (!d3drm1) {
        return;
    }
    CHECK(d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &d3drm) == DD_OK && d3drm);
    d3drm1->Release();
    IDirectDrawSurface* back1 = nullptr;
    CHECK(back->QueryInterface(IID_IDirectDrawSurface, (void**) &back1) == DD_OK && back1);
    IDirect3DRMDevice3* device = nullptr;
    CHECK(d3drm && back1 && d3drm->CreateDeviceFromSurface(&software, dd1, back1, 0, &device) == DD_OK && device);
    if (back1) {
        back1->Release();
    }

    if (device) {
        IDirect3DDevice2* im2 = nullptr;
        CHECK(device->GetDirect3DDevice2(&im2) == DD_OK && im2);
        if (im2) {
            IDirect3DDevice3* im = nullptr;
            CHECK(im2->QueryInterface(IID_IDirect3DDevice3, (void**) &im) == DD_OK && im);
            IDirect3D3* d3d = nullptr;
            CHECK(im && im->GetDirect3D(&d3d) == DD_OK && d3d);
            if (d3d) {
                d3d->Release();
            }
            if (im) {
                im->Release();
            }
            im2->Release();
        }

        // A fill, then a keyed sprite, read back through Lock.
        Fill(back, 0x07E0);
        CHECK(Pixel16(back, 10, 10, DDLOCK_READONLY | DDLOCK_WAIT) == 0x07E0);
        IDirectDrawSurface4* sprite = CreateOffscreen(dd, 2, 2);
        if (sprite) {
            Fill(sprite, 0x001F);
            RECT one = {1, 1, 2, 2};
            Fill(sprite, 0xF800, &one);
            DDCOLORKEY key = {0x001F, 0x001F};
            CHECK(sprite->SetColorKey(DDCKEY_SRCBLT, &key) == DD_OK);
            RECT src = {0, 0, 2, 2};
            CHECK(back->BltFast(4, 4, sprite, &src, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT) == DD_OK);
            CHECK(Pixel16(back, 4, 4, DDLOCK_READONLY | DDLOCK_WAIT) == 0x07E0);
            CHECK(Pixel16(back, 5, 5, DDLOCK_READONLY | DDLOCK_WAIT) == 0xF800);

            // Reading the screen back into a surface (Images.c's
            // Image_GetScreenshot).
            IDirectDrawSurface4* shot = CreateOffscreen(dd, 64, 64);
            if (shot) {
                CHECK(shot->Blt(nullptr, back, nullptr, DDBLT_WAIT, nullptr) == DD_OK);
                CHECK(Pixel16(shot, 5, 5) == 0xF800 && Pixel16(shot, 20, 20) == 0x07E0);
                shot->Release();
            }
            sprite->Release();
        }

        // Presenting: the fullscreen flip and the windowed blit to the
        // primary surface.
        CHECK(front->Flip(nullptr, DDFLIP_WAIT) == DD_OK);
        RECT whole = {0, 0, 64, 64};
        CHECK(front->Blt(&whole, back, &whole, DDBLT_WAIT, nullptr) == DD_OK);
        device->Release();
    }
    if (d3drm) {
        d3drm->Release();
    }
    back->Release();
    front->Release();
}

// ---- frames -------------------------------------------------------------

static void CheckFrames()
{
    IDirect3DRM* d3drm1 = nullptr;
    IDirect3DRM3* d3drm = nullptr;
    CHECK(Direct3DRMCreate(&d3drm1) == DD_OK && d3drm1);
    if (!d3drm1) {
        return;
    }
    d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &d3drm);
    d3drm1->Release();
    if (!d3drm) {
        return;
    }
    IDirect3DRMFrame3 *scene = nullptr, *parent = nullptr, *child = nullptr;
    CHECK(d3drm->CreateFrame(nullptr, &scene) == DD_OK);
    CHECK(d3drm->CreateFrame(scene, &parent) == DD_OK);
    CHECK(d3drm->CreateFrame(parent, &child) == DD_OK);
    if (!scene || !parent || !child) {
        return;
    }

    IDirect3DRMFrame3* got = nullptr;
    CHECK(child->GetParent(&got) == DD_OK && got == parent);
    if (got) {
        got->Release();
    }
    CHECK(child->GetScene(&got) == DD_OK && got == scene);
    if (got) {
        got->Release();
    }

    // Positions relative to a reference frame.
    D3DVECTOR position;
    CHECK(parent->SetPosition(scene, 10, 0, 0) == DD_OK);
    CHECK(child->SetPosition(scene, 10, 5, 0) == DD_OK);
    CHECK(child->GetPosition(parent, &position) == DD_OK && NearVector(position, 0, 5, 0));
    CHECK(child->GetPosition(scene, &position) == DD_OK && NearVector(position, 10, 5, 0));
    CHECK(child->GetPosition(nullptr, &position) == DD_OK && NearVector(position, 10, 5, 0));

    // Rotation about y by 90 degrees takes +z to +x (D3DRM's convention).
    CHECK(parent->AddRotation(D3DRMCOMBINE_BEFORE, 0, 1, 0, (float) M_PI / 2) == DD_OK);
    D3DVECTOR in = {0, 0, 1}, out = {};
    CHECK(parent->Transform(&out, &in) == DD_OK && NearVector(out, 11, 0, 0));
    D3DVECTOR back = {};
    CHECK(parent->InverseTransform(&back, &out) == DD_OK && NearVector(back, 0, 0, 1));
    // The child follows its parent: local (0, 5, 0) stays (10, 5, 0).
    CHECK(child->GetPosition(nullptr, &position) == DD_OK && NearVector(position, 10, 5, 0));
    D3DVECTOR dir, up;
    CHECK(child->GetOrientation(nullptr, &dir, &up) == DD_OK);
    CHECK(NearVector(dir, 1, 0, 0) && NearVector(up, 0, 1, 0));

    // GetTransform relative to the parent is the local transform, which
    // Container.c's SafeAddChild carries across a reparent.
    D3DRMMATRIX4D local;
    CHECK(child->GetTransform(parent, local) == DD_OK);
    CHECK(Near(local[3][0], 0) && Near(local[3][1], 5) && Near(local[3][2], 0));
    CHECK(scene->AddChild(child) == DD_OK);
    CHECK(child->AddTransform(D3DRMCOMBINE_REPLACE, local) == DD_OK);
    CHECK(child->GetPosition(nullptr, &position) == DD_OK && NearVector(position, 0, 5, 0));

    // SetOrientation points z along dir; BEFORE and AFTER translations
    // apply in the frame's and the parent's space.
    CHECK(child->SetPosition(scene, 0, 0, 0) == DD_OK);
    CHECK(child->SetOrientation(scene, 1, 0, 0, 0, 1, 0) == DD_OK);
    CHECK(child->GetOrientation(scene, &dir, &up) == DD_OK && NearVector(dir, 1, 0, 0) && NearVector(up, 0, 1, 0));
    CHECK(child->AddTranslation(D3DRMCOMBINE_BEFORE, 0, 0, 2) == DD_OK);
    CHECK(child->GetPosition(scene, &position) == DD_OK && NearVector(position, 2, 0, 0));
    CHECK(child->AddTranslation(D3DRMCOMBINE_AFTER, 0, 0, 2) == DD_OK);
    CHECK(child->GetPosition(scene, &position) == DD_OK && NearVector(position, 2, 0, 2));
    CHECK(child->AddScale(D3DRMCOMBINE_BEFORE, 2, 2, 2) == DD_OK);
    in = {0, 0, 1};
    CHECK(child->Transform(&out, &in) == DD_OK && NearVector(out, 4, 0, 2));
    CHECK(child->SetOrientation(scene, 0, 0, 0, 0, 1, 0) == DDERR_INVALIDPARAMS);

    child->Release();
    parent->Release();
    scene->Release();
    d3drm->Release();
}

int main()
{
    SDL_SetLogPriority(SDL_LOG_CATEGORY_CUSTOM, SDL_LOG_PRIORITY_CRITICAL);
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("miniwin_ddraw4", 64, 64, 0);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    IDirectDraw* dd1 = nullptr;
    IDirectDraw4* dd = nullptr;
    CHECK(DirectDrawCreate(nullptr, &dd1, nullptr) == DD_OK && dd1);
    if (dd1) {
        CHECK(dd1->QueryInterface(IID_IDirectDraw4, (void**) &dd) == DD_OK && dd);
    }
    if (dd) {
        CheckDisplayModes(dd);
        CheckSurfaces(dd);
        CheckFrameBuffer(dd, dd1, window);
        dd->Release();
    }
    if (dd1) {
        dd1->Release();
    }
    CheckFrames();

    SDL_DestroyWindow(window);
    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin DirectDraw 4: all checks passed\n");
    return 0;
}
