// SPDX-License-Identifier: LGPL-3.0-or-later
// Fog on miniwin's software renderer (ADR 0008, step 5), set up the way
// Container.c does it: SetSceneFogColor, SetSceneFogMode,
// SetSceneFogMethod, SetSceneFogParams and SetSceneFogEnable on the root
// frame. Viewport::Render loads the scene fog into the device's fog states,
// and Mesh.c's D3DFVF_VERTEX draws come out fogged. Each case draws an
// emissive white fan facing the camera at a known depth and compares the
// centre pixel with Direct3D's fog: colour * f + fog colour * (1 - f), with
// f = (end - z) / (end - start), exp(-density z) or exp(-(density z)^2),
// per vertex or per pixel (table).
// Synthetic only: a dummy-driver SDL window, no game data.
#include <miniwin/d3d.h>
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include <functional>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures;

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) {                                                     \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,         \
                    __LINE__, #expr);                                      \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static const int kSize = 64;

struct Scene {
    IDirectDraw4* dd = nullptr;
    IDirect3D3* d3d = nullptr;
    IDirectDrawSurface4* front = nullptr;
    IDirectDrawSurface4* back = nullptr;
    IDirect3DRM3* d3drm = nullptr;
    IDirect3DRMDevice3* device = nullptr;
    IDirect3DDevice3* im = nullptr;
    IDirect3DRMFrame3* root = nullptr;
    IDirect3DRMFrame3* camera = nullptr;
    IDirect3DRMFrame3* object = nullptr;
    IDirect3DRMViewport2* viewport = nullptr;
    IDirect3DRMUserVisual* visual = nullptr;
};

static Scene g_scene;
static std::function<void(IDirect3DDevice3*)> g_draw;
static D3DMATERIALHANDLE g_white;
static float g_depth = 10.0f;

static int Callback(
    IDirect3DRMUserVisual*,
    void*,
    D3DRMUSERVISUALREASON reason,
    IDirect3DRMDevice*,
    IDirect3DRMViewport*
)
{
    if (reason == D3DRMUSERVISUAL_RENDER && g_draw) {
        g_draw(g_scene.im);
    }
    return 1;
}

// A fan around a centre vertex at (0, 0, g_depth) facing the camera; with
// an emissive white material it is white before fog.
static void DrawFan(IDirect3DDevice3* im)
{
    const float s = 0.05f * g_depth;
    const float z = g_depth;
    D3DVERTEX v[5] = {
        {0, 0, z, 0, 0, -1, 0, 0},
        {-s, s, z, 0, 0, -1, 0, 0},
        {s, s, z, 0, 0, -1, 0, 0},
        {s, -s, z, 0, 0, -1, 0, 0},
        {-s, -s, z, 0, 0, -1, 0, 0},
    };
    WORD indices[12] = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
    im->SetRenderState(D3DRENDERSTATE_CULLMODE, static_cast<DWORD>(D3DCULL_NONE));
    im->SetTexture(0, nullptr);
    im->SetLightState(D3DLIGHTSTATE_MATERIAL, g_white);
    CHECK(im->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, v, 5, indices, 12, 0) == DD_OK);
}

struct RGB {
    int r, g, b;
};

// The back buffer is RGB565; channels come back scaled to 0..255.
static RGB Pixel(int x, int y)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    RGB out = {-1, -1, -1};
    if (g_scene.back->Lock(nullptr, &desc, DDLOCK_READONLY | DDLOCK_WAIT, nullptr) != DD_OK) {
        return out;
    }
    const Uint16* row = reinterpret_cast<const Uint16*>(static_cast<const Uint8*>(desc.lpSurface) + y * desc.lPitch);
    Uint16 p = row[x];
    g_scene.back->Unlock(nullptr);
    out.r = ((p >> 11) & 0x1f) * 255 / 31;
    out.g = ((p >> 5) & 0x3f) * 255 / 63;
    out.b = (p & 0x1f) * 255 / 31;
    return out;
}

static bool Near(const RGB& c, float r, float g, float b, int tolerance = 10)
{
    auto byte = [](float v) { return static_cast<int>(lroundf(fminf(1.0f, fmaxf(0.0f, v)) * 255.0f)); };
    bool ok = abs(c.r - byte(r)) <= tolerance && abs(c.g - byte(g)) <= tolerance && abs(c.b - byte(b)) <= tolerance;
    if (!ok) {
        fprintf(stderr, "  got %d,%d,%d want %d,%d,%d\n", c.r, c.g, c.b, byte(r), byte(g), byte(b));
    }
    return ok;
}

// Render the fan at depth z and read the centre pixel.
static RGB Fogged(float z, const std::function<void(IDirect3DDevice3*)>& before = nullptr)
{
    g_depth = z;
    g_draw = [&](IDirect3DDevice3* im) {
        if (before) {
            before(im);
        }
        DrawFan(im);
    };
    static_cast<IDirect3DRMViewport*>(g_scene.viewport)->Clear();
    CHECK(g_scene.viewport->Render(g_scene.root) == DD_OK);
    return Pixel(kSize / 2, kSize / 2);
}

// White through blue fog: the expected pixel for fog factor f.
static bool IsFog(const RGB& c, float f)
{
    return Near(c, f, f, 1.0f);
}

static void SetFog(D3DRMFOGMODE mode, DWORD method, float start, float end, float density)
{
    IDirect3DRMFrame3* root = g_scene.root;
    CHECK(root->SetSceneFogColor(0xff0000ff) == DD_OK);
    CHECK(root->SetSceneFogMode(mode) == DD_OK);
    CHECK(root->SetSceneFogMethod(method) == DD_OK);
    CHECK(root->SetSceneFogParams(start, end, density) == DD_OK);
    CHECK(root->SetSceneFogEnable(TRUE) == DD_OK);
}

// ---- the cases ----------------------------------------------------------

// Off, the fan stays white; Render leaves the device's FOGENABLE off.
static void CheckNoFog()
{
    CHECK(g_scene.root->SetSceneFogEnable(FALSE) == DD_OK);
    CHECK(Near(Fogged(50), 1, 1, 1));
    DWORD on = 1;
    CHECK(g_scene.im->GetRenderState(D3DRENDERSTATE_FOGENABLE, &on) == DD_OK && on == FALSE);
}

static void CheckModes(DWORD method)
{
    // Linear from 10 to 90: a quarter, half and three quarters of the way.
    SetFog(D3DRMFOG_LINEAR, method, 10, 90, 0);
    CHECK(IsFog(Fogged(5), 1));
    CHECK(IsFog(Fogged(30), 0.75f));
    CHECK(IsFog(Fogged(50), 0.5f));
    CHECK(IsFog(Fogged(70), 0.25f));
    CHECK(IsFog(Fogged(95), 0));

    // The level's set-up: exponential squared, density 0.0032.
    SetFog(D3DRMFOG_EXPONENTIALSQUARED, method, 0, 560, 0.0032f);
    for (float z : {20.0f, 60.0f, 90.0f}) {
        float dz = 0.0032f * z;
        CHECK(IsFog(Fogged(z), expf(-dz * dz)));
    }

    SetFog(D3DRMFOG_EXPONENTIAL, method, 0, 100, 0.02f);
    for (float z : {10.0f, 35.0f, 80.0f}) {
        CHECK(IsFog(Fogged(z), expf(-0.02f * z)));
    }
}

// Render loads the scene's fog into the device: table fog into the render
// states, vertex fog into the light states.
static void CheckDeviceStates()
{
    IDirect3DDevice3* im = g_scene.im;
    DWORD value = 0;
    float f = 0;

    SetFog(D3DRMFOG_EXPONENTIALSQUARED, D3DRMFOGMETHOD_TABLE, 0, 560, 0.0032f);
    Fogged(50);
    CHECK(im->GetRenderState(D3DRENDERSTATE_FOGENABLE, &value) == DD_OK && value == TRUE);
    CHECK(im->GetRenderState(D3DRENDERSTATE_FOGCOLOR, &value) == DD_OK && value == 0xff0000ff);
    CHECK(im->GetRenderState(D3DRENDERSTATE_FOGTABLEMODE, &value) == DD_OK);
    CHECK(value == static_cast<DWORD>(D3DFOG_EXP2));
    CHECK(im->GetRenderState(D3DRENDERSTATE_FOGTABLEEND, &value) == DD_OK);
    memcpy(&f, &value, sizeof(f));
    CHECK(f == 560.0f);
    CHECK(im->GetRenderState(D3DRENDERSTATE_FOGTABLEDENSITY, &value) == DD_OK);
    memcpy(&f, &value, sizeof(f));
    CHECK(f == 0.0032f);

    SetFog(D3DRMFOG_LINEAR, D3DRMFOGMETHOD_VERTEX, 1, 2, 0);
    Fogged(50);
    CHECK(im->GetRenderState(D3DRENDERSTATE_FOGTABLEMODE, &value) == DD_OK);
    CHECK(value == static_cast<DWORD>(D3DFOG_NONE));
    CHECK(im->GetLightState(D3DLIGHTSTATE_FOGMODE, &value) == DD_OK && value == static_cast<DWORD>(D3DFOG_LINEAR));
    CHECK(im->GetLightState(D3DLIGHTSTATE_FOGEND, &value) == DD_OK);
    memcpy(&f, &value, sizeof(f));
    CHECK(f == 2.0f);
}

// Mesh.c turns FOGENABLE (render state 28) off for its alpha-blended
// groups: those draw unfogged.
static void CheckDrawWithoutFog()
{
    SetFog(D3DRMFOG_LINEAR, D3DRMFOGMETHOD_TABLE, 10, 90, 0);
    CHECK(IsFog(Fogged(50), 0.5f));
    CHECK(Near(Fogged(50, [](IDirect3DDevice3* im) { im->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE); }), 1, 1, 1));
    // The next Render turns it back on, as D3DRM does.
    CHECK(IsFog(Fogged(50), 0.5f));
}

// ---- set-up ---------------------------------------------------------------

static HRESULT FindSoftwareDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID ctx)
{
    if (SDL_strcmp(name, "Miniwin Emulation") == 0) {
        *static_cast<GUID*>(ctx) = *guid;
    }
    return DDENUMRET_OK;
}

static bool SetUp(SDL_Window* window)
{
    Scene& s = g_scene;
    IDirectDraw* dd1 = nullptr;
    if (DirectDrawCreate(nullptr, &dd1, nullptr) != DD_OK || !dd1) {
        return false;
    }
    dd1->QueryInterface(IID_IDirectDraw4, (void**) &s.dd);
    dd1->QueryInterface(IID_IDirect3D3, (void**) &s.d3d);
    if (!s.dd || !s.d3d) {
        return false;
    }
    s.dd->SetCooperativeLevel(reinterpret_cast<HWND>(window), DDSCL_NORMAL);

    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    if (s.dd->CreateSurface(&desc, &s.front, nullptr) != DD_OK || !s.front) {
        return false;
    }
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    desc.dwWidth = kSize;
    desc.dwHeight = kSize;
    if (s.dd->CreateSurface(&desc, &s.back, nullptr) != DD_OK || !s.back) {
        return false;
    }

    GUID software = {};
    IDirect3D2* d3d2 = nullptr;
    dd1->QueryInterface(IID_IDirect3D2, (void**) &d3d2);
    if (!d3d2) {
        return false;
    }
    d3d2->EnumDevices(FindSoftwareDevice, &software);
    d3d2->Release();

    IDirect3DRM* d3drm1 = nullptr;
    if (Direct3DRMCreate(&d3drm1) != DD_OK || !d3drm1) {
        return false;
    }
    d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &s.d3drm);
    d3drm1->Release();
    IDirectDrawSurface* back1 = nullptr;
    s.back->QueryInterface(IID_IDirectDrawSurface, (void**) &back1);
    if (!s.d3drm || !back1 || s.d3drm->CreateDeviceFromSurface(&software, dd1, back1, 0, &s.device) != DD_OK) {
        return false;
    }
    back1->Release();
    dd1->Release();

    IDirect3DDevice2* im2 = nullptr;
    s.device->GetDirect3DDevice2(&im2);
    if (!im2) {
        return false;
    }
    im2->QueryInterface(IID_IDirect3DDevice3, (void**) &s.im);
    im2->Release();
    if (!s.im) {
        return false;
    }

    s.d3drm->CreateFrame(nullptr, &s.root);
    s.d3drm->CreateFrame(s.root, &s.camera);
    s.d3drm->CreateFrame(s.root, &s.object);
    if (!s.root || !s.camera || !s.object) {
        return false;
    }

    if (s.d3drm->CreateViewport(s.device, s.camera, 0, 0, kSize, kSize, &s.viewport) != DD_OK || !s.viewport) {
        return false;
    }
    s.viewport->SetFront(1.0f);
    s.viewport->SetBack(200.0f);
    s.viewport->SetField(0.5f);

    if (s.d3drm->CreateUserVisual(Callback, nullptr, &s.visual) != DD_OK || !s.visual) {
        return false;
    }
    s.object->AddVisual(s.visual);
    return true;
}

static void TearDown()
{
    Scene& s = g_scene;
    IUnknown* objects[] = {s.visual, s.viewport, s.object, s.camera, s.root, s.im,
                           s.device, s.d3drm,    s.back,   s.front,  s.d3d,  s.dd};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
    s = Scene();
}

int main()
{
    SDL_SetLogPriority(SDL_LOG_CATEGORY_CUSTOM, SDL_LOG_PRIORITY_CRITICAL);
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("miniwin_fog", kSize, kSize, 0);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    if (!SetUp(window)) {
        fprintf(stderr, "set-up failed\n");
        return 1;
    }

    // Emissive white: no lights needed.
    IDirect3DMaterial3* material = nullptr;
    if (g_scene.d3d->CreateMaterial(&material, nullptr) == DD_OK && material) {
        D3DMATERIAL data = {};
        data.dwSize = sizeof(data);
        data.diffuse = {0, 0, 0, 1};
        data.emissive = {1, 1, 1, 1};
        material->SetMaterial(&data);
        material->GetHandle(g_scene.im, &g_white);
    }
    CHECK(g_white);

    CheckNoFog();
    CheckModes(D3DRMFOGMETHOD_TABLE);
    CheckModes(D3DRMFOGMETHOD_VERTEX);
    CheckDeviceStates();
    CheckDrawWithoutFog();

    g_draw = nullptr;
    if (material) {
        material->Release();
    }
    TearDown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin fog: all checks passed\n");
    return 0;
}
