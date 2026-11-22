// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_bubbles — case-insensitive configuration and the precedence of
 * call-to-arms, fleeing, and the first non-wait task when choosing an icon. */
#include "game/aitask_record.h"
#include "game/bubbles_link_names.h"
#include "game/bubbles.h"
#include "game/lego_globs.h"
#include "game/legoobject_record.h"
#include "gods98/config.h"
#include "gods98/images.h"
#include "test/lrr_test.h"

static Image armsImage, fleeImage, drillImage;

extern "C" BlockRec* __wrap_block_subgroup(BlockFile* root, const char*)
{
    return root;
}

extern "C" Image* __wrap_Image_LoadBMPScaled(const char* name, unsigned int, unsigned int)
{
    if (std::strcmp(name, "arms") == 0) return &armsImage;
    if (std::strcmp(name, "flee") == 0) return &fleeImage;
    if (std::strcmp(name, "drill") == 0) return &drillImage;
    return nullptr;
}

extern "C" void __wrap_Image_SetupTrans(struct TextureSurface*, float, float, float,
                                       float, float, float)
{
}

int main(void)
{
    s_engine_config_root = "Game";
    bubble_initialise();
    CHECK_EQ(bubble_get_bubble_type("bubble_cantdo"), 0);
    CHECK_EQ(bubble_get_bubble_type("BUBBLE_REQUEST"), 38);
    CHECK_EQ(bubble_get_object_uis_always_visible(), 0);
    bubble_toggle_object_uis_always_visible();
    CHECK_EQ(bubble_get_object_uis_always_visible(), 1);
    bubble_toggle_object_uis_always_visible();
    CHECK_EQ(bubble_get_object_uis_always_visible(), 0);

    char armsKey[] = "Bubble_CallToArms", armsName[] = "arms";
    char fleeKey[] = "Bubble_Flee", fleeName[] = "flee";
    char drillKey[] = "Bubble_Drill", drillName[] = "drill";
    BlockRec end = {};
    BlockRec drill = { nullptr, drillKey, drillName, 1u, 0u, &end, nullptr, nullptr };
    BlockRec flee = { nullptr, fleeKey, fleeName, 1u, 0u, &drill, nullptr, nullptr };
    BlockRec arms = { nullptr, armsKey, armsName, 1u, 0u, &flee, nullptr, nullptr };
    bubble_load_bubbles(&arms);

    AITask drillTask = {};
    drillTask.type = 0x08; // Dig.
    AITask waitTask = {};
    waitTask.type = 0x0e; // Wait.
    waitTask.next = &drillTask;
    Lego_Object raider = {};
    raider.aiTask = &waitTask;
    raider.flags4 = 0x20; // Call to arms.
    raider.flags1 = 0x800000; // Running away.
    Image* image = nullptr;

    bubble_live_object_get_bubble_image(&raider, &image);
    CHECK_EQ(image, &armsImage);
    raider.flags4 = 0;
    bubble_live_object_get_bubble_image(&raider, &image);
    CHECK_EQ(image, &fleeImage);
    raider.flags1 = 0;
    bubble_live_object_get_bubble_image(&raider, &image);
    CHECK_EQ(image, &drillImage);
    return lrr_test_result("bubbles");
}
