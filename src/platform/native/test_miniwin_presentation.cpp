// SPDX-License-Identifier: LGPL-3.0-or-later
// miniwin's presentation of the virtual display in a window of another size
// (miniwin/miniwinpresentation.h):
// - the fit: the largest rectangle of the display's aspect, centred, with
//   bars at the sides or above and below;
// - the render target's size and the filtering rule (whole-number scales);
// - the mouse: messages, GetCursorPos and SetCursorPos in the virtual
//   display's coordinates, through the fit, however the window is resized;
// - a frame buffer lock draws only the pixels the client changed, so the
//   rest of a picture rendered at another resolution keeps its pixels.
// Synthetic only: SDL's dummy video driver and the software renderer.
#include "test_miniwin_device.h"

#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>
#include <miniwin/miniwinpresentation.h>
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
    return fabsf(a - b) < 1.0e-3f;
}

static bool FitIs(int windowW, int windowH, float scale, float offsetX, float offsetY)
{
    ViewportTransform fit = Miniwin_FitTransform(640, 480, windowW, windowH);
    bool ok = Near(fit.scale, scale) && Near(fit.offsetX, offsetX) && Near(fit.offsetY, offsetY);
    if (!ok) {
        fprintf(stderr, "fit of 640x480 in %dx%d: scale %g offset %g,%g\n", windowW, windowH, fit.scale,
                fit.offsetX, fit.offsetY);
    }
    return ok;
}

static void CheckFit()
{
    CHECK(FitIs(640, 480, 1.0f, 0.0f, 0.0f));
    CHECK(FitIs(1280, 960, 2.0f, 0.0f, 0.0f));
    CHECK(FitIs(1024, 768, 1.6f, 0.0f, 0.0f));
    // Wider than 4:3: pillarboxed.
    CHECK(FitIs(1280, 720, 1.5f, 160.0f, 0.0f));
    CHECK(FitIs(2560, 1080, 2.25f, 560.0f, 0.0f));
    // Taller than 4:3: letterboxed.
    CHECK(FitIs(1280, 1024, 2.0f, 0.0f, 32.0f));
    // Smaller than the display: scaled down.
    CHECK(FitIs(320, 240, 0.5f, 0.0f, 0.0f));

    CHECK(Miniwin_IsIntegerScale(1.0f));
    CHECK(Miniwin_IsIntegerScale(2.0f));
    CHECK(Miniwin_IsIntegerScale(3.0f));
    CHECK(!Miniwin_IsIntegerScale(1.5f));
    CHECK(!Miniwin_IsIntegerScale(1.6f));
    CHECK(!Miniwin_IsIntegerScale(2.25f));
    CHECK(!Miniwin_IsIntegerScale(0.5f));

    int w = 0, h = 0;
    Miniwin_RenderSize(MiniwinRenderResolution::Window, 640, 480, 1920, 1080, &w, &h);
    CHECK(w == 1920 && h == 1080);
    // At the virtual display's resolution: the window's aspect, with the
    // display at a scale of one.
    Miniwin_RenderSize(MiniwinRenderResolution::Virtual, 640, 480, 1920, 1080, &w, &h);
    CHECK(w == 853 && h == 480);
    Miniwin_RenderSize(MiniwinRenderResolution::Virtual, 640, 480, 1280, 1024, &w, &h);
    CHECK(w == 640 && h == 512);
    Miniwin_RenderSize(MiniwinRenderResolution::Virtual, 640, 480, 1280, 960, &w, &h);
    CHECK(w == 640 && h == 480);
    Miniwin_RenderSize(MiniwinRenderResolution::Virtual, 640, 480, 320, 240, &w, &h);
    CHECK(w == 320 && h == 240);
}

// ---- the window and its messages -------------------------------------------

struct Seen {
    UINT message;
    LPARAM lParam;
};

static std::vector<Seen> seen;

static LRESULT CALLBACK Procedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    seen.push_back({message, lParam});
    if (message == WM_CLOSE) {
        return 0;
    }
    return DefWindowProcA(hWnd, message, wParam, lParam);
}

static void Pump()
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

// The client point of the last `message`, as signed coordinates.
static bool LastPoint(UINT message, int* x, int* y)
{
    for (auto it = seen.rbegin(); it != seen.rend(); ++it) {
        if (it->message == message) {
            *x = (short) LOWORD(it->lParam);
            *y = (short) HIWORD(it->lParam);
            return true;
        }
    }
    return false;
}

static void PushMotion(SDL_WindowID id, float x, float y)
{
    SDL_Event event = {};
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.windowID = id;
    event.motion.x = x;
    event.motion.y = y;
    CHECK(SDL_PushEvent(&event));
}

static void PushButton(SDL_WindowID id, float x, float y)
{
    SDL_Event event = {};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = id;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = true;
    event.button.clicks = 1;
    event.button.x = x;
    event.button.y = y;
    CHECK(SDL_PushEvent(&event));
}

static bool MovedTo(SDL_WindowID id, float windowX, float windowY, int x, int y)
{
    seen.clear();
    PushMotion(id, windowX, windowY);
    Pump();
    int gotX = 0, gotY = 0;
    bool ok = LastPoint(WM_MOUSEMOVE, &gotX, &gotY) && gotX == x && gotY == y;
    if (!ok) {
        fprintf(stderr, "window point %g,%g: client %d,%d, expected %d,%d\n", windowX, windowY, gotX, gotY, x, y);
    }
    return ok;
}

// Whether GetCursorPos reaches (x, y). A video driver with a real pointer
// reports a warp as motion some time later: pump until it arrives.
static bool CursorAt(int x, int y, POINT* point)
{
    for (int tries = 0; tries < 50; tries++) {
        if (GetCursorPos(point) && point->x == x && point->y == y) {
            return true;
        }
        Pump();
        SDL_Delay(10);
    }
    fprintf(stderr, "GetCursorPos: %ld,%ld, expected %d,%d\n", (long) point->x, (long) point->y, x, y);
    return false;
}

// ---- the 3D device ----------------------------------------------------------

struct Display {
    IDirectDraw4* dd = nullptr;
    IDirectDrawSurface4* front = nullptr;
    IDirectDrawSurface4* back = nullptr;
    IDirect3DRM3* d3drm = nullptr;
    IDirect3DRMDevice3* device = nullptr;
};

static HRESULT FindTestDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID ctx)
{
    if (SDL_strcmp(name, MiniwinTestDevice()) == 0) {
        *static_cast<GUID*>(ctx) = *guid;
    }
    return DDENUMRET_OK;
}

// The display as DirectDraw.c sets it up in a window: the primary surface
// takes the window's size as the virtual display's, and the back buffer is
// the 3D device's surface.
static bool SetUp(HWND hWnd, Display& d)
{
    IDirectDraw* dd1 = nullptr;
    if (DirectDrawCreate(nullptr, &dd1, nullptr) != DD_OK || !dd1) {
        return false;
    }
    dd1->QueryInterface(IID_IDirectDraw4, (void**) &d.dd);
    if (!d.dd || d.dd->SetCooperativeLevel(hWnd, DDSCL_NORMAL) != DD_OK) {
        return false;
    }
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    if (d.dd->CreateSurface(&desc, &d.front, nullptr) != DD_OK || !d.front) {
        return false;
    }
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = 640;
    desc.dwHeight = 480;
    if (d.dd->CreateSurface(&desc, &d.back, nullptr) != DD_OK || !d.back) {
        return false;
    }

    GUID guid = {};
    IDirect3D2* d3d2 = nullptr;
    dd1->QueryInterface(IID_IDirect3D2, (void**) &d3d2);
    if (!d3d2) {
        return false;
    }
    d3d2->EnumDevices(FindTestDevice, &guid);
    d3d2->Release();
    if (MiniwinTestDeviceMissing(guid)) {
        return false;
    }
    IDirect3DRM* d3drm1 = nullptr;
    if (Direct3DRMCreate(&d3drm1) != DD_OK || !d3drm1) {
        return false;
    }
    d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &d.d3drm);
    d3drm1->Release();
    IDirectDrawSurface* back1 = nullptr;
    d.back->QueryInterface(IID_IDirectDrawSurface, (void**) &back1);
    if (!d.d3drm || !back1 || d.d3drm->CreateDeviceFromSurface(&guid, dd1, back1, 0, &d.device) != DD_OK) {
        g_miniwinTestDeviceMissing = !MiniwinTestDefaultDevice();
        return false;
    }
    back1->Release();
    dd1->Release();
    return true;
}

// A 640x480 image of noise: any resampling of it changes its pixels.
static IDirectDrawSurface4* MakeNoise(IDirectDraw4* dd)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = 640;
    desc.dwHeight = 480;
    IDirectDrawSurface4* surface = nullptr;
    if (dd->CreateSurface(&desc, &surface, nullptr) != DD_OK || !surface) {
        return nullptr;
    }
    DDSURFACEDESC2 lock = {};
    lock.dwSize = sizeof(lock);
    if (surface->Lock(nullptr, &lock, DDLOCK_WAIT, nullptr) != DD_OK) {
        return surface;
    }
    Uint32 seed = 12345;
    int bytes = lock.ddpfPixelFormat.dwRGBBitCount / 8;
    for (DWORD y = 0; y < lock.dwHeight; y++) {
        Uint8* row = static_cast<Uint8*>(lock.lpSurface) + y * lock.lPitch;
        for (DWORD x = 0; x < lock.dwWidth * bytes; x++) {
            seed = seed * 1103515245u + 12345u;
            row[x] = (Uint8) (seed >> 16);
        }
    }
    surface->Unlock(nullptr);
    return surface;
}

int main()
{
    SDL_SetLogPriority(SDL_LOG_CATEGORY_CUSTOM, SDL_LOG_PRIORITY_CRITICAL);
    CheckFit();

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, MiniwinTestVideoDriver());
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    WNDCLASSA wc = {};
    wc.lpfnWndProc = Procedure;
    wc.lpszClassName = "test_miniwin_presentation";
    CHECK(RegisterClassA(&wc) != 0);
    HWND hWnd =
        CreateWindowExA(0, wc.lpszClassName, "", WS_POPUP | WS_CAPTION | WS_THICKFRAME, 0, 0, 640, 480, NULL, NULL, NULL, NULL);
    if (!hWnd) {
        return MiniwinTestUnavailable("CreateWindowExA");
    }
    SDL_Window* window = reinterpret_cast<SDL_Window*>(hWnd);
    SDL_WindowID id = SDL_GetWindowID(window);
    ShowWindow(hWnd, SW_SHOW); // as Main.c does: a pointer can only be warped into a shown window
    SDL_SyncWindow(window);
    Pump();

    // Before the display is set up, client coordinates are the window's.
    CHECK(MovedTo(id, 123.0f, 45.0f, 123, 45));

    Display d;
    if (!SetUp(hWnd, d)) {
        if (g_miniwinTestDeviceMissing) {
            return kMiniwinTestSkip;
        }
        fprintf(stderr, "set-up failed\n");
        return 1;
    }

    // The window at the display's size: one to one.
    CHECK(MovedTo(id, 320.0f, 240.0f, 320, 240));

    // Resized wider than 4:3: the display is pillarboxed at 1.5 times.
    SDL_SetWindowSize(window, 1280, 720);
    SDL_SyncWindow(window);
    int pixelW = 0, pixelH = 0;
    SDL_GetWindowSizeInPixels(window, &pixelW, &pixelH);
    if (pixelW != 1280 || pixelH != 720) {
        fprintf(stderr, "the video driver cannot resize the window (%dx%d): resize checks skipped\n", pixelW, pixelH);
    }
    else {
        CHECK(MovedTo(id, 640.0f, 360.0f, 320, 240));
        CHECK(MovedTo(id, 160.0f, 0.0f, 0, 0));
        CHECK(MovedTo(id, 1119.0f, 719.0f, 639, 479));
        // Over a bar: outside the display, for the client to clamp.
        CHECK(MovedTo(id, 100.0f, 30.0f, -40, 20));
        CHECK(MovedTo(id, 1270.0f, 30.0f, 740, 20));
        seen.clear();
        PushButton(id, 400.0f, 300.0f);
        Pump();
        int x = 0, y = 0;
        CHECK(LastPoint(WM_LBUTTONDOWN, &x, &y) && x == 160 && y == 200);

        // SetCursorPos and GetCursorPos agree, through the fit.
        CHECK(SetCursorPos(320, 240));
        POINT point = {};
        CHECK(CursorAt(320, 240, &point));
        CHECK(SetCursorPos(0, 479));
        CHECK(CursorAt(0, 479, &point));
        float mouseX = 0.0f, mouseY = 0.0f;
        SDL_GetMouseState(&mouseX, &mouseY);
        CHECK(mouseX >= 160.0f && mouseX < 161.5f && mouseY >= 718.5f && mouseY < 720.0f);

        // Taller than 4:3: letterboxed at 2 times.
        SDL_SetWindowSize(window, 1280, 1024);
        SDL_SyncWindow(window);
        CHECK(MovedTo(id, 640.0f, 512.0f, 320, 240));
        CHECK(MovedTo(id, 0.0f, 32.0f, 0, 0));
    }

    // A frame buffer lock that changes one pixel draws that pixel, and
    // leaves every other pixel of the picture as the renderer drew it. The
    // window is half the display's size, so the software renderer draws at
    // 320x240 and a lock reads the picture back scaled up to 640x480: drawn
    // back whole, the picture would be resampled twice.
    SDL_SetWindowSize(window, 320, 240);
    SDL_SyncWindow(window);
    SDL_GetWindowSizeInPixels(window, &pixelW, &pixelH);
    if (pixelW == 320 && pixelH == 240) {
        IDirectDrawSurface4* noise = MakeNoise(d.dd);
        CHECK(noise != nullptr);
        CHECK(d.front->Flip(nullptr, DDFLIP_WAIT) == DD_OK); // applies the resize
        CHECK(d.back->BltFast(0, 0, noise, nullptr, DDBLTFAST_WAIT) == DD_OK);

        // Before: the picture read back at the display's size.
        DDSURFACEDESC2 lock = {};
        lock.dwSize = sizeof(lock);
        std::vector<Uint8> before;
        CHECK(d.back->Lock(nullptr, &lock, DDLOCK_READONLY | DDLOCK_WAIT, nullptr) == DD_OK);
        LONG pitch = lock.lPitch;
        int bytes = lock.ddpfPixelFormat.dwRGBBitCount / 8;
        before.assign(static_cast<Uint8*>(lock.lpSurface), static_cast<Uint8*>(lock.lpSurface) + pitch * 480);
        d.back->Unlock(nullptr);

        // A read-write lock that sets a 4x4 block of pixels, as Draw.c's
        // pixel writers do (a single pixel would cover less than one pixel
        // of the half-size picture).
        lock = {};
        lock.dwSize = sizeof(lock);
        CHECK(d.back->Lock(nullptr, &lock, DDLOCK_WAIT, nullptr) == DD_OK);
        for (int y = 200; y < 204; y++) {
            Uint8* pixel = static_cast<Uint8*>(lock.lpSurface) + y * lock.lPitch + 300 * bytes;
            SDL_memset(pixel, 0xFF, 4 * bytes);
        }
        d.back->Unlock(nullptr);

        // After: the same picture, but for the changed block's neighbourhood.
        std::vector<Uint8> after;
        lock = {};
        lock.dwSize = sizeof(lock);
        CHECK(d.back->Lock(nullptr, &lock, DDLOCK_READONLY | DDLOCK_WAIT, nullptr) == DD_OK);
        after.assign(static_cast<Uint8*>(lock.lpSurface), static_cast<Uint8*>(lock.lpSurface) + pitch * 480);
        d.back->Unlock(nullptr);

        int changedFar = 0;
        int changedNear = 0;
        for (int y = 0; y < 480; y++) {
            for (int x = 0; x < 640; x++) {
                bool same = SDL_memcmp(&before[y * pitch + x * bytes], &after[y * pitch + x * bytes], bytes) == 0;
                bool near = x >= 296 && x < 308 && y >= 196 && y < 208;
                if (!same) {
                    (near ? changedNear : changedFar)++;
                }
            }
        }
        CHECK(changedNear > 0);
        if (changedFar != 0) {
            fprintf(stderr, "%d pixels away from the changed block changed\n", changedFar);
        }
        CHECK(changedFar == 0);
        noise->Release();
    }
    else {
        fprintf(stderr, "the video driver cannot resize the window: lock check skipped\n");
    }

    d.device->Release();
    d.d3drm->Release();
    d.back->Release();
    d.front->Release();
    d.dd->Release();
    DestroyWindow(hWnd);
    SDL_Quit();

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("miniwin presentation: ok\n");
    return 0;
}
