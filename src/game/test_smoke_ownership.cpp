// SPDX-License-Identifier: LGPL-3.0-or-later
/* An emitter that owns its frame releases it; a borrowed frame survives.
 * Use actual retained-mode frames and meshes with a flat synthetic terrain
 * and no audio device or textures. */
#include <algorithm>
#include <vector>

#include <miniwin/d3drm.h>

#include "game/sfx_link_names.h"
#include "game/sfx.h"
#include "game/terrain_map.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/main_globs.h"
#include "test/lrr_test.h"

struct SFX_Vector3F { float x, y, z; };
struct Mesh_Texture;
static TerrainMap map = {};
static unsigned int soundAttempts;

extern "C" void* __wrap_level_current_terrain_get() { return &map; }
extern "C" float __wrap_terrain_map_world_z_get(TerrainMap*, float, float) { return 0.0f; }
extern "C" Mesh_Texture* __wrap_mesh_load_texture(const char*, const char*,
                                                  unsigned long*, unsigned long*)
{
    return nullptr;
}
extern "C" int __wrap_sound_bank_entry_load(const char*, int, int, int) { return 5; }
extern "C" int __wrap_s_play_seam(int mode, intptr_t, int slot, int loop, const float*)
{
    CHECK_EQ(mode, 1);
    CHECK_EQ(slot, 5);
    CHECK_EQ(loop, 1);
    ++soundAttempts;
    return 0; // The device cannot start this ambient sound yet.
}

static std::vector<Container*> child_containers(Container* root)
{
    std::vector<Container*> result;
    IDirect3DRMFrameArray* children = nullptr;
    if (!CHECK_EQ(Container_GetMasterFrame(root)->GetChildren(&children), DD_OK)) return result;
    for (DWORD i = 0; i < children->GetSize(); ++i) {
        IDirect3DRMFrame* child = nullptr;
        if (CHECK_EQ(children->GetElement(i, &child), DD_OK)) {
            IDirect3DRMFrame3* frame = nullptr;
            if (CHECK_EQ(child->QueryInterface(IID_IDirect3DRMFrame3,
                                              reinterpret_cast<void**>(&frame)), DD_OK)) {
                Container* owner = Container_Frame_GetOwner(frame);
                if (owner && owner != root) result.push_back(owner);
                frame->Release();
            }
            child->Release();
        }
    }
    children->Release();
    return result;
}

static DWORD visuals(Container* cont)
{
    DWORD count = 0;
    CHECK_EQ(cont->activityFrame->GetVisuals(&count, nullptr), DD_OK);
    return count;
}

static void check_ownership(Container* root)
{
    map.gridWidth = map.gridHeight = 2;
    map.blockSize = 8.0f;
    map.worldY = 16.0f;
    smoke_initialise();
    // Texture lookup is stubbed: the mesh is untextured, not rendered, and
    // still exercises the production allocation and removal paths.
    smoke_load_textures("Synthetic", "particle", 1);
    sfx_initialise();
    sfx_set_sample_populate_mode(1);
    int soundID = 0;
    if (!CHECK_EQ(sfx_get_type("SFX_FixtureSmoke", &soundID), 1)) return;
    sfx_set_sample_populate_mode(0);
    char sample[] = "synthetic";
    if (!CHECK_EQ(sfx_load_sample_property(sample, soundID), 1)) return;
    sfx_set_sound_on(1, 0);

    Container* borrowed = Container_Create(root);
    if (!CHECK(borrowed != nullptr, "create borrowed frame")) return;
    const DWORD baseVisuals = visuals(borrowed);
    CHECK_EQ(child_containers(root).size(), 1u);
    SFX_Vector3F direction{0, 0, -1};
    Smoke* continuous = smoke_create_smoke_area(borrowed, 1, 1, &direction,
        0.4f, 0.4f, 0.4f, 0.2f, 1.0f, 1, 0, 0);
    Smoke* owned = smoke_create_smoke_area(nullptr, 1, 1, &direction,
        0.4f, 0.4f, 0.4f, 0.2f, 1.0f, 0, 0, soundID);

    if (CHECK(continuous && owned, "create borrowed and owned emitters")) {
        CHECK_EQ(visuals(borrowed), baseVisuals + 1);
        CHECK_EQ(child_containers(root).size(), 2u);

        // The original retries a failed sound start after a new group
        // appears. Retaining ownership must not change that behaviour.
        for (unsigned i = 0; i < 40; ++i) smoke_update(5.0f);
        CHECK(soundAttempts > 1, "ambient sound starts are retried");

        smoke_remove(continuous, borrowed);
        CHECK_EQ(visuals(borrowed), baseVisuals);
        CHECK_EQ(Container_GetParent(borrowed), root);
    }
    smoke_remove_all();
    smoke_remove_all();
    const auto remaining = child_containers(root);
    CHECK_EQ(remaining.size(), 1u);
    CHECK(std::find(remaining.begin(), remaining.end(), borrowed) != remaining.end(),
          "only the borrowed frame remains");
    // On regression, clean up the leaked frame before root teardown.
    for (Container* child : remaining) {
        if (child != borrowed) Container_Remove(child);
    }
    Container_Remove(borrowed);
    sfx_set_sound_on(0, 0);
}

int main()
{
    IDirect3DRM* rm = nullptr;
    IDirect3DRM3* rm3 = nullptr;
    if (!CHECK_EQ(Direct3DRMCreate(&rm), DD_OK)) return lrr_test_result();
    CHECK_EQ(rm->QueryInterface(IID_IDirect3DRM3, reinterpret_cast<void**>(&rm3)), DD_OK);
    rm->Release();
    if (!rm3) return lrr_test_result();
    s_main_d3drm = rm3;
    Container* root = Container_Initialise("SmokeOwnership");
    if (CHECK(root != nullptr, "create root container")) check_ownership(root);
    Container_Shutdown();
    s_main_d3drm = nullptr;
    rm3->Release();
    return lrr_test_result();
}
