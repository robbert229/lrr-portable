// SPDX-License-Identifier: LGPL-3.0-or-later
// Lights on miniwin's software renderer (ADR 0008, step 4), as the level
// sets them up: Container_MakeLight's D3DRM lights with SetColor, SetRange,
// SetUmbra, SetPenumbra and SetEnableFrame, lighting Mesh.c's
// D3DFVF_VERTEX draws with a D3DMATERIAL. Each case lights a small fan
// facing the camera at the centre of a 64x64 target and compares the centre
// pixel with Direct3D's lighting formulas: emissive + ambient light times
// material ambient + per light, attenuation * spot factor * N.L * light
// colour * material diffuse, and nothing beyond the range. D3DRM hands a
// spot to Direct3D as theta = umbra, phi = penumbra and falloff 0 (read
// back from Microsoft's D3DRM.DLL under Wine). Direct3D's theta and phi are
// full apex angles, and falloff 0 makes the spot factor 1 anywhere inside
// phi, so a D3DRM spot lights fully out to half its penumbra from the axis
// and not at all beyond, whatever its umbra.
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
    IDirect3DRMFrame3* lightFrame = nullptr;
    IDirect3DRMViewport2* viewport = nullptr;
    IDirect3DRMUserVisual* visual = nullptr;
    IDirect3DRMLight* spot = nullptr;
    IDirect3DRMLight* ambient = nullptr;
};

static Scene g_scene;
static std::function<void(IDirect3DDevice3*)> g_draw;

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

static IDirect3DMaterial3* MakeMaterial(const D3DMATERIAL& values, D3DMATERIALHANDLE* handle)
{
    IDirect3DMaterial3* material = nullptr;
    if (g_scene.d3d->CreateMaterial(&material, nullptr) != DD_OK || !material) {
        return nullptr;
    }
    D3DMATERIAL data = values;
    data.dwSize = sizeof(data);
    material->SetMaterial(&data);
    material->GetHandle(g_scene.im, handle);
    return material;
}

// A fan of four triangles around a centre vertex at (0, 0, kDepth), facing
// the camera: the centre pixel is the centre vertex's lit colour.
static const float kDepth = 10.0f;

static void DrawFan(IDirect3DDevice3* im, D3DMATERIALHANDLE material)
{
    const float s = 0.5f;
    D3DVERTEX v[5] = {
        {0, 0, kDepth, 0, 0, -1, 0, 0},
        {-s, s, kDepth, 0, 0, -1, 0, 0},
        {s, s, kDepth, 0, 0, -1, 0, 0},
        {s, -s, kDepth, 0, 0, -1, 0, 0},
        {-s, -s, kDepth, 0, 0, -1, 0, 0},
    };
    WORD indices[12] = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
    im->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    im->SetRenderState(D3DRENDERSTATE_CULLMODE, static_cast<DWORD>(D3DCULL_NONE));
    im->SetTexture(0, nullptr);
    im->SetLightState(D3DLIGHTSTATE_MATERIAL, material);
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

// Render the fan with material and read the centre pixel.
static RGB Lit(D3DMATERIALHANDLE material)
{
    g_draw = [&](IDirect3DDevice3* im) { DrawFan(im, material); };
    static_cast<IDirect3DRMViewport*>(g_scene.viewport)->Clear();
    CHECK(g_scene.viewport->Render(g_scene.root) == DD_OK);
    return Pixel(kSize / 2, kSize / 2);
}

// The spot light at (0, 0, z) shining along (sin a, 0, cos a).
static void AimSpot(float z, float a)
{
    g_scene.lightFrame->SetPosition(g_scene.root, 0, 0, z);
    g_scene.lightFrame->SetOrientation(g_scene.root, sinf(a), 0, cosf(a), 0, 1, 0);
}

// ---- the cases ----------------------------------------------------------

// The spot is at full strength out to half its penumbra angle and lights
// nothing beyond, and its umbra changes nothing.
static void CheckSpotCone(D3DMATERIALHANDLE white)
{
    CHECK(g_scene.spot->SetRange(100.0f) == DD_OK);
    for (float umbra : {0.0f, 0.2f, 0.4f}) {
        CHECK(g_scene.spot->SetUmbra(umbra) == DD_OK);
        CHECK(g_scene.spot->SetPenumbra(0.4f) == DD_OK);
        for (float a : {0.0f, 0.1f, 0.18f}) {
            AimSpot(2.0f, a);
            CHECK(Near(Lit(white), 1, 1, 1));
        }
        for (float a : {0.22f, 0.3f, 0.5f}) {
            AimSpot(2.0f, a);
            CHECK(Near(Lit(white), 0, 0, 0));
        }
    }
    CHECK(g_scene.spot->SetUmbra(0.2f) == DD_OK);
}

// The light colour, the range and the attenuation.
static void CheckColourRangeAttenuation(D3DMATERIALHANDLE white, D3DMATERIALHANDLE orange)
{
    AimSpot(2.0f, 0.0f);
    CHECK(g_scene.spot->SetColor(0xff804000) == DD_OK);
    CHECK(Near(Lit(white), 128 / 255.0f, 64 / 255.0f, 0));
    // The light's colour times the material's diffuse colour.
    CHECK(Near(Lit(orange), 128 / 255.0f, 32 / 255.0f, 0));
    CHECK(g_scene.spot->SetColorRGB(1, 1, 1) == DD_OK);

    // The fan is 8 from the light.
    CHECK(g_scene.spot->SetRange(7.0f) == DD_OK);
    CHECK(Near(Lit(white), 0, 0, 0));
    CHECK(g_scene.spot->SetRange(9.0f) == DD_OK);
    CHECK(Near(Lit(white), 1, 1, 1));

    // 1 / (1 + 8/8) and 1 / (1 + 64/64).
    CHECK(g_scene.spot->SetLinearAttenuation(1 / 8.0f) == DD_OK);
    CHECK(Near(Lit(white), 0.5f, 0.5f, 0.5f));
    CHECK(g_scene.spot->SetLinearAttenuation(0) == DD_OK);
    CHECK(g_scene.spot->SetQuadraticAttenuation(1 / 64.0f) == DD_OK);
    CHECK(Near(Lit(white), 0.5f, 0.5f, 0.5f));
    CHECK(g_scene.spot->SetQuadraticAttenuation(0) == DD_OK);
    CHECK(g_scene.spot->SetRange(100.0f) == DD_OK);
}

// Ambient light lights the material's ambient colour, not its diffuse one,
// and emissive adds to everything; specular adds after, when enabled.
static void CheckMaterialTerms()
{
    AimSpot(2.0f, 1.5f); // the spot points away
    g_scene.ambient->SetColorRGB(0.5f, 0.5f, 0.5f);

    D3DMATERIAL values = {};
    values.diffuse = {1, 1, 1, 1};
    values.ambient = {0, 1, 0, 1};
    values.emissive = {0.25f, 0, 0, 1};
    D3DMATERIALHANDLE handle = 0;
    IDirect3DMaterial3* material = MakeMaterial(values, &handle);
    CHECK(Near(Lit(handle), 0.25f, 0.5f, 0));

    // A material with no ambient colour, as Mesh.c makes them, ignores the
    // ambient light.
    D3DMATERIAL meshDefault = {};
    meshDefault.diffuse = {1, 1, 1, 1};
    D3DMATERIALHANDLE plain = 0;
    IDirect3DMaterial3* plainMaterial = MakeMaterial(meshDefault, &plain);
    CHECK(Near(Lit(plain), 0, 0, 0));
    g_scene.ambient->SetColorRGB(0, 0, 0);

    // Specular: the light sits by the camera, so the half vector is the
    // normal and N.H is 1.
    D3DMATERIAL shiny = {};
    shiny.specular = {0, 0, 1, 1};
    shiny.diffuse = {0, 0, 0, 1};
    shiny.power = 4.0f;
    D3DMATERIALHANDLE shinyHandle = 0;
    IDirect3DMaterial3* shinyMaterial = MakeMaterial(shiny, &shinyHandle);
    AimSpot(0.0f, 0.0f);
    CHECK(Near(Lit(shinyHandle), 0, 0, 0));
    g_scene.im->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, TRUE);
    CHECK(Near(Lit(shinyHandle), 0, 0, 1));
    g_scene.im->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);

    IUnknown* objects[] = {material, plainMaterial, shinyMaterial};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
}

// A light with an enable frame lights only that frame and its descendants.
static void CheckEnableFrame(D3DMATERIALHANDLE white)
{
    Scene& s = g_scene;
    AimSpot(2.0f, 0.0f);
    IDirect3DRMFrame3* other = nullptr;
    IDirect3DRMFrame3* child = nullptr;
    s.d3drm->CreateFrame(s.root, &other);
    s.d3drm->CreateFrame(s.object, &child);
    IDirect3DRMFrame* objectFrame = nullptr;
    IDirect3DRMFrame* otherFrame = nullptr;
    s.object->QueryInterface(IID_IDirect3DRMFrame, (void**) &objectFrame);
    other->QueryInterface(IID_IDirect3DRMFrame, (void**) &otherFrame);

    CHECK(s.spot->SetEnableFrame(objectFrame) == DD_OK);
    CHECK(Near(Lit(white), 1, 1, 1));

    s.object->DeleteVisual(static_cast<IUnknown*>(s.visual));
    child->AddVisual(static_cast<IUnknown*>(s.visual));
    CHECK(Near(Lit(white), 1, 1, 1));

    CHECK(s.spot->SetEnableFrame(otherFrame) == DD_OK);
    CHECK(Near(Lit(white), 0, 0, 0));

    CHECK(s.spot->SetEnableFrame(nullptr) == DD_OK);
    CHECK(Near(Lit(white), 1, 1, 1));

    child->DeleteVisual(static_cast<IUnknown*>(s.visual));
    s.object->AddVisual(static_cast<IUnknown*>(s.visual));
    IUnknown* objects[] = {objectFrame, otherFrame, child, other};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
}

// ---- set-up ---------------------------------------------------------------

// Retained mode: a positional light's attenuation, range and cone scale its
// specular highlight as they scale its diffuse light. A black mesh with a
// shiny material shows only the highlight, at full strength from the spot
// straight ahead, and at half with a constant attenuation of 2.
static void CheckRetainedSpecular()
{
    Scene& s = g_scene;
    AimSpot(2.0f, 0.0f);
    g_draw = nullptr;
    IDirect3DRMFrame3* frame = nullptr;
    IDirect3DRMMesh* mesh = nullptr;
    IDirect3DRMMaterial2* material = nullptr;
    CHECK(s.d3drm->CreateFrame(s.root, &frame) == DD_OK && frame);
    CHECK(s.d3drm->CreateMesh(&mesh) == DD_OK && mesh);
    CHECK(s.d3drm->CreateMaterial(10.0f, &material) == DD_OK && material);
    if (!frame || !mesh || !material) {
        return;
    }
    unsigned int faces[6] = {0, 1, 2, 2, 1, 3};
    D3DRMGROUPINDEX group = 0;
    CHECK(mesh->AddGroup(4, 2, 3, faces, &group) == DD_OK);
    D3DRMVERTEX v[4] = {};
    const float pos[4][2] = {{-0.5f, 0.5f}, {0.5f, 0.5f}, {-0.5f, -0.5f}, {0.5f, -0.5f}};
    for (int i = 0; i < 4; ++i) {
        v[i].position = {pos[i][0], pos[i][1], 0};
        v[i].normal = {0, 0, -1};
    }
    CHECK(mesh->SetVertices(group, 0, 4, v) == DD_OK);
    // The highlight's half vector is taken at the frame's origin.
    frame->SetPosition(s.root, 0, 0, kDepth);
    mesh->SetGroupColorRGB(group, 0, 0, 0);
    IDirect3DRMMaterial* material1 = nullptr;
    material->QueryInterface(IID_IDirect3DRMMaterial, (void**) &material1);
    CHECK(material1 && mesh->SetGroupMaterial(group, material1) == DD_OK);
    frame->AddVisual(mesh);

    auto centre = [&]() {
        static_cast<IDirect3DRMViewport*>(s.viewport)->Clear();
        CHECK(s.viewport->Render(s.root) == DD_OK);
        return Pixel(kSize / 2, kSize / 2);
    };
    CHECK(Near(centre(), 1, 1, 1));
    CHECK(s.spot->SetConstantAttenuation(2.0f) == DD_OK);
    CHECK(Near(centre(), 0.5f, 0.5f, 0.5f));
    CHECK(s.spot->SetConstantAttenuation(1.0f) == DD_OK);

    frame->DeleteVisual(mesh);
    s.root->DeleteChild(frame);
    IUnknown* objects[] = {material1, material, mesh, frame};
    for (IUnknown* object : objects) {
        if (object) {
            object->Release();
        }
    }
}

static HRESULT FindTestDevice(LPGUID guid, LPSTR name, LPSTR, LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID ctx)
{
    if (SDL_strcmp(name, MiniwinTestDevice()) == 0) {
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
    s.d3drm->CreateFrame(s.root, &s.lightFrame);
    if (!s.root || !s.camera || !s.object || !s.lightFrame) {
        return false;
    }

    // The level's lights: an ambient light (off until a case needs it) and
    // a white spot light.
    s.d3drm->CreateLightRGB(D3DRMLIGHT_AMBIENT, 0, 0, 0, &s.ambient);
    s.d3drm->CreateLightRGB(D3DRMLIGHT_SPOT, 1, 1, 1, &s.spot);
    if (!s.ambient || !s.spot) {
        return false;
    }
    s.root->AddLight(s.ambient);
    s.lightFrame->AddLight(s.spot);

    if (s.d3drm->CreateViewport(s.device, s.camera, 0, 0, kSize, kSize, &s.viewport) != DD_OK || !s.viewport) {
        return false;
    }
    s.viewport->SetFront(1.0f);
    s.viewport->SetBack(100.0f);
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
    IUnknown* objects[] = {s.spot,   s.ambient, s.visual, s.viewport, s.lightFrame, s.object, s.camera,
                           s.root,   s.im,      s.device, s.d3drm,    s.back,       s.front,  s.d3d,
                           s.dd};
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
    SDL_Window* window = SDL_CreateWindow("miniwin_lights", kSize, kSize, 0);
    if (!window) {
        return MiniwinTestUnavailable("SDL_CreateWindow");
    }
    if (!SetUp(window)) {
        if (g_miniwinTestDeviceMissing) {
            return kMiniwinTestSkip;
        }
        fprintf(stderr, "set-up failed\n");
        return 1;
    }

    D3DMATERIAL whiteValues = {};
    whiteValues.diffuse = {1, 1, 1, 1};
    D3DMATERIAL orangeValues = {};
    orangeValues.diffuse = {1, 0.5f, 0, 1};
    D3DMATERIALHANDLE white = 0, orange = 0;
    IDirect3DMaterial3* materials[] = {MakeMaterial(whiteValues, &white), MakeMaterial(orangeValues, &orange)};
    CHECK(white && orange);

    CheckSpotCone(white);
    CheckColourRangeAttenuation(white, orange);
    CheckMaterialTerms();
    CheckEnableFrame(white);
    if (MiniwinTestRetainedAdditions("CheckRetainedSpecular")) {
        CheckRetainedSpecular();
    }

    g_draw = nullptr;
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
    printf("miniwin lights: all checks passed\n");
    return 0;
}
