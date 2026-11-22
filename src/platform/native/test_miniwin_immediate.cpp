// SPDX-License-Identifier: LGPL-3.0-or-later
// Immediate-mode drawing on miniwin's software renderer (ADR 0008, step 1),
// the way gods98's Mesh.c draws: a Direct3DRM user visual whose callback
// sets render states and a material on the IDirect3DDevice3 and calls
// DrawIndexedPrimitive with D3DFVF_VERTEX triangle lists. Each case renders
// into a 64x64 target and reads the pixels back through Lock: coverage, the
// z test, additive and SRCALPHA blending, back-face culling, the transforms
// D3DRM sets before the callback, and drawing after Render returns (the
// post-render list).
// Synthetic only: a dummy-driver SDL window, no game data.
#include <miniwin/d3d.h>
#include <miniwin/d3drm.h>
#include <miniwin/ddraw.h>
#include <miniwin/windows.h>

#include <SDL3/SDL.h>

#include "test_miniwin_device.h"

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

// The scene: a camera at the origin looking down +z, and the user visual
// on a frame of its own.
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

// What the user visual's callback does, and what it saw.
struct Visual {
    std::function<void(IDirect3DDevice3*)> draw;
    int canSee = 0;
    int render = 0;
    bool reportVisible = true;
    D3DMATRIX world = {};
    IDirect3DRMViewport* viewport = nullptr;
};

static Scene g_scene;

static int Callback(
    IDirect3DRMUserVisual*,
    void* arg,
    D3DRMUSERVISUALREASON reason,
    IDirect3DRMDevice*,
    IDirect3DRMViewport* viewport
)
{
    Visual* visual = static_cast<Visual*>(arg);
    if (reason == D3DRMUSERVISUAL_CANSEE) {
        ++visual->canSee;
        return visual->reportVisible ? 1 : 0;
    }
    ++visual->render;
    visual->viewport = viewport;
    g_scene.im->GetTransform(D3DTRANSFORMSTATE_WORLD, &visual->world);
    if (visual->draw) {
        visual->draw(g_scene.im);
    }
    return 1;
}

// ---- drawing helpers ------------------------------------------------------

static IDirect3DMaterial3* MakeMaterial(float r, float g, float b, float a, D3DMATERIALHANDLE* handle)
{
    IDirect3DMaterial3* material = nullptr;
    if (g_scene.d3d->CreateMaterial(&material, nullptr) != DD_OK || !material) {
        return nullptr;
    }
    D3DMATERIAL data = {};
    data.dwSize = sizeof(data);
    data.diffuse = {r, g, b, a};
    data.ambient = {r, g, b, a};
    material->SetMaterial(&data);
    material->GetHandle(g_scene.im, handle);
    return material;
}

// A square of half-size s at depth z, wound clockwise as seen from the camera unless ccw is set.
static void DrawQuad(IDirect3DDevice3* im, float s, float z, bool ccw = false)
{
    D3DVERTEX v[4] = {
        {-s, s, z, 0, 0, -1, 0, 0},
        {s, s, z, 0, 0, -1, 1, 0},
        {-s, -s, z, 0, 0, -1, 0, 1},
        {s, -s, z, 0, 0, -1, 1, 1},
    };
    WORD cw[6] = {0, 1, 2, 2, 1, 3};
    WORD counter[6] = {0, 2, 1, 2, 3, 1};
    HRESULT hr = im->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, v, 4, ccw ? counter : cw, 6, 0);
    CHECK(hr == DD_OK);
}

static void UseMaterial(IDirect3DDevice3* im, D3DMATERIALHANDLE handle)
{
    CHECK(im->SetLightState(D3DLIGHTSTATE_MATERIAL, handle) == DD_OK);
}

static void ResetStates(IDirect3DDevice3* im)
{
    im->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    im->SetRenderState(D3DRENDERSTATE_CULLMODE, static_cast<DWORD>(D3DCULL_CCW));
    im->SetRenderState(D3DRENDERSTATE_SHADEMODE, static_cast<DWORD>(D3DSHADE_GOURAUD));
}

// ---- reading back -------------------------------------------------------

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

static bool Near(const RGB& c, int r, int g, int b, int tolerance = 10)
{
    return abs(c.r - r) <= tolerance && abs(c.g - g) <= tolerance && abs(c.b - b) <= tolerance;
}

static int CountPixels(int r, int g, int b)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    if (g_scene.back->Lock(nullptr, &desc, DDLOCK_READONLY | DDLOCK_WAIT, nullptr) != DD_OK) {
        return -1;
    }
    int count = 0;
    for (int y = 0; y < kSize; ++y) {
        const Uint16* row =
            reinterpret_cast<const Uint16*>(static_cast<const Uint8*>(desc.lpSurface) + y * desc.lPitch);
        for (int x = 0; x < kSize; ++x) {
            Uint16 p = row[x];
            RGB c = {((p >> 11) & 0x1f) * 255 / 31, ((p >> 5) & 0x3f) * 255 / 63, (p & 0x1f) * 255 / 31};
            count += Near(c, r, g, b) ? 1 : 0;
        }
    }
    g_scene.back->Unlock(nullptr);
    return count;
}

// Clear to black and render the scene once.
static void RenderFrame()
{
    static_cast<IDirect3DRMViewport*>(g_scene.viewport)->Clear();
    CHECK(g_scene.viewport->Render(g_scene.root) == DD_OK);
}

// ---- the cases ----------------------------------------------------------

static void CheckCallbacks(Visual& visual)
{
    visual.draw = nullptr;
    visual.canSee = visual.render = 0;
    RenderFrame();
    CHECK(visual.canSee == 1 && visual.render == 1);
    // RENDER saw the object frame's world matrix (it sits at x = 1) and its
    // own viewport.
    CHECK(visual.world._11 == 1.0f && visual.world._41 == 1.0f && visual.world._42 == 0.0f);
    CHECK(visual.viewport == static_cast<IDirect3DRMViewport*>(g_scene.viewport));

    // A visual that reports it cannot be seen is not asked to render.
    visual.reportVisible = false;
    visual.canSee = visual.render = 0;
    RenderFrame();
    CHECK(visual.canSee == 1 && visual.render == 0);
    visual.reportVisible = true;

    // Under a rotated parent the world matrix is the frame's own transform
    // followed by its parent's (row vectors), the same matrix
    // Frame3::GetTransform reports: a quarter turn about z takes the
    // object's x = 1 to y = 1.
    IDirect3DRMFrame3* parent = nullptr;
    g_scene.d3drm->CreateFrame(g_scene.root, &parent);
    CHECK(parent != nullptr);
    if (parent) {
        parent->SetPosition(g_scene.root, 0.0f, 0.0f, 3.0f);
        parent->SetOrientation(g_scene.root, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f);
        parent->AddChild(g_scene.object);
        g_scene.object->SetPosition(parent, 1.0f, 0.0f, 0.0f);
        D3DRMMATRIX4D expected;
        CHECK(g_scene.object->GetTransform(g_scene.root, expected) == DD_OK);
        visual.canSee = visual.render = 0;
        RenderFrame();
        CHECK(visual.render == 1);
        const float* got = &visual.world._11;
        bool same = true;
        for (int i = 0; i < 16; ++i) {
            same = same && fabsf(got[i] - (&expected[0][0])[i]) < 1e-5f;
        }
        CHECK(same);
        CHECK(fabsf(visual.world._41) < 1e-5f && fabsf(visual.world._42 - 1.0f) < 1e-5f);
        CHECK(fabsf(visual.world._43 - 3.0f) < 1e-5f);
        g_scene.root->AddChild(g_scene.object);
        parent->Release();
    }

    // The drawing cases put the object frame at the origin.
    g_scene.object->SetOrientation(g_scene.root, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    g_scene.object->SetPosition(g_scene.root, 0.0f, 0.0f, 0.0f);
}

static void CheckCoverage(Visual& visual, D3DMATERIALHANDLE red)
{
    // At z = 10 the field of 0.5 shows x and y in [-5, 5], so a quad of
    // half-size 2.5 covers the middle 32x32 pixels.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        UseMaterial(im, red);
        D3DVERTEX v[4] = {
            {-2.5f, 2.5f, 10, 0, 0, -1, 0, 0},
            {2.5f, 2.5f, 10, 0, 0, -1, 1, 0},
            {-2.5f, -2.5f, 10, 0, 0, -1, 0, 1},
            {2.5f, -2.5f, 10, 0, 0, -1, 1, 1},
        };
        WORD indices[6] = {0, 1, 2, 2, 1, 3};
        CHECK(im->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_VERTEX, v, 4, indices, 6, 0) == DD_OK);
    };
    RenderFrame();
    CHECK(Near(Pixel(32, 32), 255, 0, 0));
    CHECK(Near(Pixel(2, 2), 0, 0, 0));
    CHECK(Near(Pixel(61, 61), 0, 0, 0));
    int covered = CountPixels(255, 0, 0);
    // 32x32, give or take a row and a column of edge pixels.
    CHECK(covered >= 31 * 31 && covered <= 33 * 33);
    if (covered < 31 * 31 || covered > 33 * 33) {
        fprintf(stderr, "coverage: %d red pixels\n", covered);
    }
}

static void CheckDepth(Visual& visual, D3DMATERIALHANDLE red, D3DMATERIALHANDLE green)
{
    // Near green drawn first, far red second: the z test keeps green.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        UseMaterial(im, green);
        DrawQuad(im, 1, 5);
        UseMaterial(im, red);
        DrawQuad(im, 6, 10);
    };
    RenderFrame();
    CHECK(Near(Pixel(32, 32), 0, 255, 0));
    CHECK(Near(Pixel(48, 32), 255, 0, 0)); // red outside green's square

    // With the z test off, the later far quad wins.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        UseMaterial(im, green);
        DrawQuad(im, 1, 5);
        im->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
        UseMaterial(im, red);
        DrawQuad(im, 6, 10);
    };
    RenderFrame();
    CHECK(Near(Pixel(32, 32), 255, 0, 0));

    // Without z writes, the near quad leaves no depth behind.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
        UseMaterial(im, green);
        DrawQuad(im, 1, 5);
        im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
        UseMaterial(im, red);
        DrawQuad(im, 6, 10);
    };
    RenderFrame();
    CHECK(Near(Pixel(32, 32), 255, 0, 0));
}

static void CheckBlending(Visual& visual, D3DMATERIALHANDLE blue, D3DMATERIALHANDLE red, D3DMATERIALHANDLE halfRed)
{
    // Additive (ONE, ONE), as Mesh.c's ALPHATRANS: blue + red.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        UseMaterial(im, blue);
        DrawQuad(im, 6, 10);
        im->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        im->SetRenderState(D3DRENDERSTATE_SRCBLEND, static_cast<DWORD>(D3DBLEND_ONE));
        im->SetRenderState(D3DRENDERSTATE_DESTBLEND, static_cast<DWORD>(D3DBLEND_ONE));
        im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
        UseMaterial(im, red);
        DrawQuad(im, 1, 5);
    };
    RenderFrame();
    CHECK(Near(Pixel(32, 32), 255, 0, 255));
    CHECK(Near(Pixel(56, 32), 0, 0, 255));

    // SRCALPHA, INVSRCALPHA (ALPHASA0) with half-transparent red over blue.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        UseMaterial(im, blue);
        DrawQuad(im, 6, 10);
        im->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        im->SetRenderState(D3DRENDERSTATE_SRCBLEND, static_cast<DWORD>(D3DBLEND_SRCALPHA));
        im->SetRenderState(D3DRENDERSTATE_DESTBLEND, static_cast<DWORD>(D3DBLEND_INVSRCALPHA));
        UseMaterial(im, halfRed);
        DrawQuad(im, 1, 5);
    };
    RenderFrame();
    RGB mixed = Pixel(32, 32);
    CHECK(Near(mixed, 128, 0, 127));
    if (!Near(mixed, 128, 0, 127)) {
        fprintf(stderr, "SRCALPHA, INVSRCALPHA: %d %d %d\n", mixed.r, mixed.g, mixed.b);
    }

    // SRCALPHA, ONE (ALPHASA1): half red added to blue.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        UseMaterial(im, blue);
        DrawQuad(im, 6, 10);
        im->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        im->SetRenderState(D3DRENDERSTATE_SRCBLEND, static_cast<DWORD>(D3DBLEND_SRCALPHA));
        im->SetRenderState(D3DRENDERSTATE_DESTBLEND, static_cast<DWORD>(D3DBLEND_ONE));
        UseMaterial(im, halfRed);
        DrawQuad(im, 1, 5);
    };
    RenderFrame();
    CHECK(Near(Pixel(32, 32), 128, 0, 255));

    // Additive triangles sharing an edge add once along it: the quad's
    // diagonal is no brighter than its interior.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        im->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        im->SetRenderState(D3DRENDERSTATE_SRCBLEND, static_cast<DWORD>(D3DBLEND_SRCALPHA));
        im->SetRenderState(D3DRENDERSTATE_DESTBLEND, static_cast<DWORD>(D3DBLEND_ONE));
        UseMaterial(im, halfRed);
        DrawQuad(im, 6, 10);
    };
    RenderFrame();
    CHECK(CountPixels(128, 0, 0) == kSize * kSize);
}

static void CheckCulling(Visual& visual, D3DMATERIALHANDLE red)
{
    struct Case {
        D3DCULL mode;
        bool ccw;
        bool drawn;
    };
    const Case cases[] = {
        {D3DCULL_CCW, false, true},
        {D3DCULL_CCW, true, false},
        {D3DCULL_CW, false, false},
        {D3DCULL_CW, true, true},
        {D3DCULL_NONE, false, true},
        {D3DCULL_NONE, true, true},
    };
    for (const Case& c : cases) {
        visual.draw = [&](IDirect3DDevice3* im) {
            ResetStates(im);
            im->SetRenderState(D3DRENDERSTATE_CULLMODE, static_cast<DWORD>(c.mode));
            UseMaterial(im, red);
            DrawQuad(im, 1, 5, c.ccw);
        };
        RenderFrame();
        bool drawn = Near(Pixel(32, 32), 255, 0, 0);
        CHECK(drawn == c.drawn);
        if (drawn != c.drawn) {
            fprintf(stderr, "cull mode %u, ccw %d: drawn %d\n", static_cast<unsigned>(c.mode), c.ccw, drawn);
        }
    }
}

static void CheckPostRender(Visual& visual, D3DMATERIALHANDLE red, D3DMATERIALHANDLE green)
{
    // Mesh.c draws its post-render list after Render returns, with the
    // transforms and z-buffer the frame left behind.
    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        UseMaterial(im, green);
        DrawQuad(im, 1, 5);
    };
    RenderFrame();
    ResetStates(g_scene.im);
    UseMaterial(g_scene.im, red);
    DrawQuad(g_scene.im, 6, 10); // behind green: only the ring around it shows
    CHECK(Near(Pixel(32, 32), 0, 255, 0));
    CHECK(Near(Pixel(50, 32), 255, 0, 0));
}

static void CheckRetainedAfterVisual(Visual& visual, D3DMATERIALHANDLE green)
{
    // A retained-mesh frame walked after the user visual's: its mesh draws
    // after the visual's immediate-mode draws in the same frame, through
    // the device's own retained-mode pipeline.
    IDirect3DRMFrame3* frame = nullptr;
    IDirect3DRMMesh* mesh = nullptr;
    CHECK(g_scene.d3drm->CreateFrame(g_scene.root, &frame) == DD_OK && frame);
    CHECK(g_scene.d3drm->CreateMesh(&mesh) == DD_OK && mesh);
    if (!frame || !mesh) {
        return;
    }
    unsigned int faces[6] = {0, 1, 2, 2, 1, 3};
    D3DRMGROUPINDEX group = 0;
    CHECK(mesh->AddGroup(4, 2, 3, faces, &group) == DD_OK);
    D3DRMVERTEX v[4] = {};
    const float pos[4][2] = {{-4, 4}, {4, 4}, {-4, -4}, {4, -4}};
    for (int i = 0; i < 4; ++i) {
        v[i].position = {pos[i][0], pos[i][1], 10};
        v[i].normal = {0, 0, -1};
    }
    CHECK(mesh->SetVertices(group, 0, 4, v) == DD_OK);
    mesh->SetGroupColorRGB(group, 1.0f, 0.0f, 0.0f);
    frame->AddVisual(mesh);

    visual.draw = [&](IDirect3DDevice3* im) {
        ResetStates(im);
        UseMaterial(im, green);
        DrawQuad(im, 1, 5);
    };
    RenderFrame();
    CHECK(Near(Pixel(32, 32), 0, 255, 0)); // the visual's near quad
    CHECK(Near(Pixel(50, 32), 255, 0, 0)); // the mesh around it
    CHECK(Near(Pixel(2, 2), 0, 0, 0));     // nothing beyond the mesh

    frame->DeleteVisual(mesh);
    g_scene.root->DeleteChild(frame);
    mesh->Release();
    frame->Release();
}

// ---- set-up -------------------------------------------------------------

static HRESULT FindTestDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID ctx)
{
    if (SDL_strcmp(name, MiniwinTestDevice()) == 0) {
        *static_cast<GUID*>(ctx) = *guid;
    }
    return DDENUMRET_OK;
}

static bool SetUp(SDL_Window* window, Visual* visual)
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

    // DirectDraw.c's windowed set-up: a primary surface, then the offscreen
    // 3D device surface, which in miniwin is the same framebuffer.
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

    GUID device = {};
    IDirect3D2* d3d2 = nullptr;
    dd1->QueryInterface(IID_IDirect3D2, (void**) &d3d2);
    if (!d3d2) {
        return false;
    }
    d3d2->EnumDevices(FindTestDevice, &device);
    d3d2->Release();
    if (MiniwinTestDeviceMissing(device)) {
        return false;
    }

    IDirect3DRM* d3drm1 = nullptr;
    if (Direct3DRMCreate(&d3drm1) != DD_OK || !d3drm1) {
        return false;
    }
    d3drm1->QueryInterface(IID_IDirect3DRM3, (void**) &s.d3drm);
    d3drm1->Release();
    IDirectDrawSurface* back1 = nullptr;
    s.back->QueryInterface(IID_IDirectDrawSurface, (void**) &back1);
    if (!s.d3drm || !back1 || s.d3drm->CreateDeviceFromSurface(&device, dd1, back1, 0, &s.device) != DD_OK) {
        g_miniwinTestDeviceMissing = !MiniwinTestDefaultDevice();
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
    s.object->SetPosition(s.root, 1.0f, 0.0f, 0.0f);

    // A white ambient light: lit colours are the material's diffuse colour.
    IDirect3DRMLight* light = nullptr;
    s.d3drm->CreateLightRGB(D3DRMLIGHT_AMBIENT, 1.0f, 1.0f, 1.0f, &light);
    if (!light) {
        return false;
    }
    s.root->AddLight(light);
    light->Release();

    if (s.d3drm->CreateViewport(s.device, s.camera, 0, 0, kSize, kSize, &s.viewport) != DD_OK || !s.viewport) {
        return false;
    }
    s.viewport->SetFront(1.0f);
    s.viewport->SetBack(100.0f);
    s.viewport->SetField(0.5f);

    if (s.d3drm->CreateUserVisual(Callback, visual, &s.visual) != DD_OK || !s.visual) {
        return false;
    }
    s.object->AddVisual(s.visual);
    return true;
}

static void TearDown()
{
    Scene& s = g_scene;
    IUnknown* objects[] = {s.visual, s.viewport, s.object, s.camera, s.root, s.im, s.device, s.d3drm, s.back, s.front, s.d3d, s.dd};
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
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, MiniwinTestVideoDriver());
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("miniwin_immediate", kSize, kSize, MiniwinTestWindowFlags());
    if (!window) {
        return MiniwinTestUnavailable("SDL_CreateWindow");
    }

    Visual visual;
    if (!SetUp(window, &visual)) {
        if (g_miniwinTestDeviceMissing) {
            return kMiniwinTestSkip;
        }
        fprintf(stderr, "set-up failed\n");
        return 1;
    }

    D3DMATERIALHANDLE red = 0, green = 0, blue = 0, halfRed = 0;
    IDirect3DMaterial3* materials[] = {
        MakeMaterial(1, 0, 0, 1, &red),
        MakeMaterial(0, 1, 0, 1, &green),
        MakeMaterial(0, 0, 1, 1, &blue),
        MakeMaterial(1, 0, 0, 0.5f, &halfRed),
    };
    CHECK(red && green && blue && halfRed);

    CheckCallbacks(visual);
    CheckCoverage(visual, red);
    CheckDepth(visual, red, green);
    CheckBlending(visual, blue, red, halfRed);
    CheckCulling(visual, red);
    CheckPostRender(visual, red, green);
    CheckRetainedAfterVisual(visual, green);

    for (IDirect3DMaterial3* material : materials) {
        if (material) {
            material->Release();
        }
    }
    TearDown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin immediate mode: all checks passed\n");
    return 0;
}
