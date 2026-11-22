// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_building_model — power smoothing and its dirty flag, footprint
 * access, and attachment points that are absent or already cached. */
#include "game/building_model_link_names.h"
#include "game/building_model.h"
#include "game/building_model_record.h"
#include "test/lrr_test.h"

int main(void)
{
    BuildingModel model = {};
    model.flags = BUILDING_FLAG_SOURCE;
    building_change_power_level(&model, 0);
    CHECK_EQ(model.powerLevel, 0.0f);
    CHECK_EQ(model.flags, BUILDING_FLAG_SOURCE | BUILDING_FLAG_POWERLEVELCHANGED);
    model.powerLevel = 0.5f;
    building_change_power_level(&model, 1);
    CHECK_NEAR(model.powerLevel, 0.55f, 1e-6);
    building_change_power_level(&model, 0);
    CHECK_NEAR(model.powerLevel, 0.5f, 1e-6);

    Building_Point2I footprint[] = { { 0, 0 }, { 1, -1 } };
    model.shapePoints = footprint;
    model.shapeCount = 2;
    unsigned int count = 0;
    CHECK_EQ(building_get_shape_points(&model, &count), &footprint[0]);
    CHECK_EQ(count, 2u);
    CHECK_EQ(building_get_shape_points(&model, nullptr), &footprint[0]);

    CHECK_EQ(building_get_deposit_null(&model), nullptr);
    CHECK_EQ(building_get_camera_null(&model, 1u), nullptr);
    char carry[] = "Carry", camera[] = "Camera", deposit[] = "Deposit";
    int attachments[3] = {};
    model.carryNullName = carry;
    model.cameraNullName = camera;
    model.depositNullName = deposit;
    model.carryNullFrames = 2;
    model.carryNulls[1] = &attachments[0];
    model.cameraNulls[1] = &attachments[1];
    model.depositNull = &attachments[2];

    CHECK_EQ(building_get_carry_null_frames(&model), 2u);
    CHECK_EQ(building_get_carry_null(&model, 1u), &attachments[0]);
    CHECK_EQ(building_get_camera_null(&model, 1u), &attachments[1]);
    CHECK_EQ(building_get_deposit_null(&model), &attachments[2]);
    return lrr_test_result("building_model");
}
