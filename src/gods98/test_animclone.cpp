// SPDX-License-Identifier: LGPL-3.0-or-later
/* animclone.c — copy a retained-mode frame hierarchy and its local
 * transforms, and stop a depth-first walk at a descendant. */
#include <vector>

#include "gods98/animclone.h"
#include "gods98/main_globs.h"
#include "test/lrr_test.h"

struct Walk {
    std::vector<IDirect3DRMFrame3*> seen;
    unsigned int stopAfter = 0;
};

static int visit(IDirect3DRMFrame3* frame, void* data)
{
    auto* walk = static_cast<Walk*>(data);
    walk->seen.push_back(frame);
    return walk->seen.size() == walk->stopAfter;
}

static void check_frames(IDirect3DRM3* rm)
{
    IDirect3DRMFrame3* root = nullptr;
    IDirect3DRMFrame3* original = nullptr;
    IDirect3DRMFrame3* child = nullptr;
    IDirect3DRMFrame3* second = nullptr;
    IDirect3DRMFrame3* copy = nullptr;
    if (CHECK_EQ(rm->CreateFrame(nullptr, &root), DD_OK) &&
        CHECK_EQ(rm->CreateFrame(root, &original), DD_OK) &&
        CHECK_EQ(rm->CreateFrame(original, &child), DD_OK) &&
        CHECK_EQ(rm->CreateFrame(original, &second), DD_OK) &&
        CHECK_EQ(rm->CreateFrame(root, &copy), DD_OK)) {
        original->SetName("source");
        child->SetName("pivot");
        second->SetName("next");
        original->SetPosition(root, 2.0f, 3.0f, 4.0f);
        child->SetPosition(original, -1.0f, 5.0f, 0.5f);

        Walk stopped;
        stopped.stopAfter = 2;
        CHECK_EQ(anim_clone_walk_tree(original, 0, visit, &stopped), 1);
        CHECK_EQ(stopped.seen, (std::vector<IDirect3DRMFrame3*>{original, child}));

        anim_clone_create_copy(original, copy, 0);
        Walk walk;
        CHECK_EQ(anim_clone_walk_tree(copy, 0, visit, &walk), 0);
        if (CHECK_EQ(walk.seen.size(), std::size_t{3})) {
            CHECK_EQ(walk.seen[0], copy);
            CHECK(walk.seen[1] != child, "the copy owns a distinct child frame");
            CHECK(walk.seen[2] != second, "the copy owns a distinct sibling frame");

            char name[32] = {};
            DWORD size = sizeof(name);
            CHECK_EQ(walk.seen[1]->GetName(&size, name), DD_OK);
            CHECK_STREQ(name, "pivot");
            D3DVECTOR pos = {};
            CHECK_EQ(copy->GetPosition(root, &pos), DD_OK);
            CHECK_NEAR(pos.x, 2.0f, 1e-6);
            CHECK_NEAR(pos.y, 3.0f, 1e-6);
            CHECK_NEAR(pos.z, 4.0f, 1e-6);
            CHECK_EQ(walk.seen[1]->GetPosition(copy, &pos), DD_OK);
            CHECK_NEAR(pos.x, -1.0f, 1e-6);
            CHECK_NEAR(pos.y, 5.0f, 1e-6);
            CHECK_NEAR(pos.z, 0.5f, 1e-6);
        }
    }
    if (copy) copy->Release();
    if (second) second->Release();
    if (child) child->Release();
    if (original) original->Release();
    if (root) root->Release();
}

int main()
{
    IDirect3DRM* rm = nullptr;
    IDirect3DRM3* rm3 = nullptr;
    if (!CHECK_EQ(Direct3DRMCreate(&rm), DD_OK)) return lrr_test_result();
    CHECK_EQ(rm->QueryInterface(IID_IDirect3DRM3, reinterpret_cast<void**>(&rm3)), DD_OK);
    rm->Release();
    if (rm3) {
        s_main_d3drm = rm3;
        check_frames(rm3);
        s_main_d3drm = nullptr;
        rm3->Release();
    }
    return lrr_test_result();
}
