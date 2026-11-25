// SPDX-License-Identifier: LGPL-3.0-or-later
/* Independently generated scene hierarchy, geometry and keyframes. Exercise
 * existing format parsing, shared mesh lookup and animation lifecycle. */
#include <cmath>
#include <string>
#include <cstdlib>
#include "platform/native/test_fixture.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/lws.h"
#include "gods98/mesh_link_names.h"
#include "gods98/mesh_api.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"

// Mesh.c's private returned texture record. The test inspects only the
// underlying surface identity and releases the references it obtained.
struct Mesh_Texture {
    LPDIRECTDRAWSURFACE4 surface;
    LPDIRECT3DTEXTURE2 texture;
    int colourKey;
};

namespace {
using namespace synthetic_fixture;
bool enabled = true;
unsigned signals = 0, lookups = 0;
int find_signal(const char* name, int* id)
{
    ++lookups;
    CHECK_STREQ(name, "FixtureSignal");
    *id = 13;
    return 1;
}
int play_signal(void* frame, int id, int loop, int onFrame, const float* position)
{
    CHECK(frame != nullptr, "signal source scene frame");
    CHECK_EQ(id, 13);
    CHECK_EQ(loop, 0);
    CHECK_EQ(onFrame, 1);
    CHECK_EQ(position, nullptr);
    ++signals;
    return 1;
}
int sound_enabled() { return enabled ? 1 : 0; }

const char StaticMotion[] =
    "ObjectMotion (unnamed)\n  9\n  1\n"
    "  0 0 0 0 0 0 1 1 1\n  0 0 0 0 0\n"
    "EndBehavior 1\n";
const char MovingMotion[] =
    "ObjectMotion (unnamed)\n  9\n  2\n"
    "  0 0 0 0 0 0 1 1 1\n  0 0 0 0 0\n"
    "  10 0 0 90 0 0 2 1 2\n  10 0 0 0 0\n"
    "EndBehavior 1\n";

bool files()
{
    if (!CHECK(fixture_enter("lws_resource_graph") &&
               fixture_mkdir("Data") && fixture_mkdir("Data/Scene") &&
               fixture_mkdir("Data/Shared"), "isolated generic asset directory")) return false;
    if (!CHECK(write_triangle("Data/Scene/local.lwo") &&
               write_triangle("Data/Shared/primitive.lwo") &&
               write_bmp("Data/Scene/local.bmp", 8, 8) &&
               write_bmp("Data/Shared/shared.bmp", 4, 8),
               "generated triangular geometry and grayscale textures"))
        return false;
    std::string scene = "LWSC\n1\n\nFirstFrame 0\nLastFrame 12\nFramesPerSecond 25\n"
                        "AddNullObject Root\n";
    scene += StaticMotion;
    scene += "ShadowOptions 7\nLoadObject Unused\\primitive.lwo\n";
    scene += MovingMotion;
    scene += "ParentObject 1\nObjDissolve (envelope)\n  1\n  2\n"
             "  0.25\n  0 0 0 0 0\n  0.75\n  10 0 0 0 0\nEndBehavior 1\n"
             "ShadowOptions 7\nLoadObject Unused\\primitive.lwo\n";
    scene += StaticMotion;
    scene += "ParentObject 1\nShadowOptions 7\nLoadObject Unused\\local.lwo\n";
    scene += StaticMotion;
    scene += "ParentObject 1\nUnseenByCamera 1\nShadowOptions 7\n"
             "LoadObject Unused\\local.lwo\n";
    scene += StaticMotion;
    scene += "ParentObject 1\nShadowOptions 7\nAddNullObject SFX,FixtureSignal,3,7\n";
    scene += StaticMotion;
    scene += "ParentObject 1\nShadowOptions 7\n";
    // Eleven ordinary nodes exceed the parser's initial ten-node block.
    for (unsigned i = 0; i < 6; ++i) {
        scene += "AddNullObject Marker" + std::to_string(i) + "\n";
        scene += StaticMotion;
        scene += "ParentObject 1\nShadowOptions 7\n";
    }
    return CHECK(write_text("Data/Scene/neutral.lws", scene.c_str()), "write original scene");
}

IDirect3DRMFrame3* child(IDirect3DRMFrame3* parent, unsigned index, unsigned expected)
{
    IDirect3DRMFrameArray* list = nullptr;
    IDirect3DRMFrame3* frame = nullptr;
    if (!CHECK_EQ(parent->GetChildren(&list), DD_OK)) return nullptr;
    if (CHECK_EQ(list->GetSize(), expected)) {
        IDirect3DRMFrame* old = nullptr;
        if (CHECK_EQ(list->GetElement(index, &old), DD_OK)) {
            CHECK_EQ(old->QueryInterface(IID_IDirect3DRMFrame3, reinterpret_cast<void**>(&frame)), DD_OK);
            old->Release();
        }
    }
    list->Release();
    return frame;
}
Mesh* frame_mesh(IDirect3DRMFrame3* frame)
{
    DWORD count = 0;
    if (!CHECK_EQ(frame->GetVisuals(&count, nullptr), DD_OK) ||
        !CHECK_EQ(count, 1u)) return nullptr;
    IUnknown* visual = nullptr;
    IDirect3DRMUserVisual* user = nullptr;
    CHECK_EQ(frame->GetVisuals(&count, &visual), DD_OK);
    if (!CHECK(visual != nullptr, "primitive visual")) return nullptr;
    CHECK_EQ(visual->QueryInterface(IID_IDirect3DRMUserVisual, reinterpret_cast<void**>(&user)), DD_OK);
    visual->Release();
    if (!CHECK(user != nullptr, "immediate mesh visual")) return nullptr;
    auto* mesh = static_cast<Mesh*>(user->GetAppData());
    user->Release();
    return mesh;
}
void check_key(IDirect3DRMFrame3* frame, IDirect3DRMFrame3* parent,
               float position, float scale, float angle, float alpha)
{
    D3DRMMATRIX4D m{};
    if (!CHECK_EQ(frame->GetTransform(parent, m), DD_OK)) return;
    CHECK_NEAR(m[3][0], position, 0.0001f);
    CHECK_NEAR(m[0][0], scale * std::cos(angle), 0.0001f);
    CHECK_NEAR(m[0][2], -scale * std::sin(angle), 0.0001f);
    CHECK_NEAR(m[1][1], 1.0f, 0.0001f);
    auto* mesh = frame_mesh(frame);
    if (CHECK(mesh != nullptr, "parsed geometry") &&
        CHECK(mesh->groupCount > 0, "mesh owns at least one group"))
        CHECK_NEAR(mesh_get_group_material(mesh, 0)->diffuse.a, alpha, 0.0001f);
}

void texture_cache()
{
    unsigned long w = 0, h = 0;
    auto* first = mesh_load_texture("Scene\\", "local.bmp", &w, &h);
    auto* same = mesh_load_texture("Scene\\", "LOCAL.BMP", nullptr, nullptr);
    CHECK_EQ(w, 8ul);
    CHECK_EQ(h, 8ul);
    w = h = 0;
    auto* shared = mesh_load_texture("Scene\\", "shared.bmp", &w, &h);
    auto* sharedAgain = mesh_load_texture("Elsewhere\\", "SHARED.BMP", nullptr, nullptr);
    CHECK_EQ(w, 4ul);
    CHECK_EQ(h, 8ul);
    CHECK_EQ(mesh_load_texture("Scene\\", "absent.bmp", nullptr, nullptr), nullptr);
    if (CHECK(first && same && shared && sharedAgain, "all actual texture requests resolved")) {
        CHECK(first != same && shared != sharedAgain, "independent handle records");
        CHECK_EQ(first->surface, same->surface);
        CHECK_EQ(shared->surface, sharedAgain->surface);
        CHECK(first->surface != shared->surface, "local and shared inputs differ");
        CHECK_EQ(first->colourKey, same->colourKey);
        CHECK_EQ(shared->colourKey, sharedAgain->colourKey);
    }
    for (auto* texture : {same, sharedAgain, first, shared}) {
        if (!texture) continue;
        if (texture->surface) texture->surface->Release();
        std::free(texture);
    }
}

void run(Container* root)
{
    if (!CHECK(mesh_initialise("Shared"), "generic mesh pool")) return;
    lws_initialise("Shared", find_signal, play_signal, sound_enabled);
    CHECK_EQ(lws_parse("Scene\\absent", 0), nullptr);
    auto* scene = lws_parse("Scene\\neutral", 0);
    if (!CHECK(scene != nullptr, "parse resource graph")) return;
    CHECK_EQ(resource_entry_id_get(scene), 12ul);
    CHECK_EQ(lookups, 1u);
    auto* originalRoot = Container_Create(root);
    auto* cloneRoot = Container_Create(root);
    if (!CHECK(originalRoot && cloneRoot, "scene parents")) return;
    lws_meshes_load(scene, originalRoot->activityFrame);
    auto* parent = child(originalRoot->activityFrame, 0, 1);
    if (!CHECK(parent != nullptr, "parent node")) return;
    auto* animated = child(parent, 0, 11);
    auto* repeated = child(parent, 1, 11);
    auto* local = child(parent, 2, 11);
    auto* repeatedLocal = child(parent, 3, 11);
    if (!CHECK(animated && repeated && local && repeatedLocal, "child nodes")) return;
    CHECK(frame_mesh(animated) != frame_mesh(repeated), "shared lookup owns distinct mesh instances");
    CHECK(frame_mesh(local) != frame_mesh(repeatedLocal), "local lookup owns distinct mesh instances");
    CHECK_EQ(signals, 0u);
    check_key(animated, parent, 0, 1, 0, 0.75f);
    lws_time_set(scene, 4);
    CHECK_EQ(signals, 1u);
    lws_time_set(scene, 5);
    check_key(animated, parent, 5, 1.5f, 3.14159265f/4, 0.5f);
    enabled = false;
    lws_time_set(scene, 8);
    CHECK_EQ(signals, 1u);
    enabled = true;
    lws_time_set(scene, 12);
    check_key(animated, parent, 10, 2, 3.14159265f/2, 0.25f);

    auto* clone = lws_clone(scene, cloneRoot->activityFrame);
    if (CHECK(clone != nullptr, "clone resource graph")) {
        auto* cloneParent = child(cloneRoot->activityFrame, 0, 1);
        auto* cloneAnimated = cloneParent ? child(cloneParent, 0, 11) : nullptr;
        if (CHECK(cloneParent && cloneAnimated, "independent clone frames")) {
            enabled = false;
            lws_time_set(clone, 2.5f);
            check_key(cloneAnimated, cloneParent, 2.5f, 1.25f, 3.14159265f/8, 0.625f);
            check_key(animated, parent, 10, 2, 3.14159265f/2, 0.25f);
        }
        lws_free(clone);
        if (cloneAnimated) cloneAnimated->Release();
        if (cloneParent) cloneParent->Release();
    }
    lws_free(scene);
    animated->Release(); repeated->Release(); local->Release(); repeatedLocal->Release();
    parent->Release();
    Container_Remove(cloneRoot);
    Container_Remove(originalRoot);
    // No later mesh or texture request can consult these test-owned
    // handles after their final release.
    texture_cache();
    lws_shutdown();
}
}

int main()
{
    if (!files() || !initialise_headless("ResourceGraph")) return lrr_test_result();
    auto* root = Container_Initialise("ResourceGraph");
    if (CHECK(root != nullptr, "scene root")) {
        run(root);
        Container_Shutdown();
    }
    directdraw_shutdown();
    return lrr_test_result("lws_resource_graph");
}
