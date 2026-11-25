// SPDX-License-Identifier: LGPL-3.0-or-later
/* Real engine ticks on the generated grid: simulation timing and oxygen
 * accounting, plus view ownership using the existing neutral scene. */
#include <string>
#include "gods98/directdraw.h"
#include "gods98/input_globs.h"
#include "gods98/input_key_names.h"
#include "gods98/viewports_link_names.h"
#include "gods98/viewports.h"
#include "game/camera_control.h"
#include "game/entity_stats.h"
#include "game/lego_unit.h"
#include "game/interface_unit.h"
#include "game/legoobject_unit.h"
#include "game/message_unit_link_names.h"
#include "game/message_unit.h"
#include "game/objective_unit.h"
#include "game/panel_unit.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"

namespace {
constexpr char Stats[] =
    " Figure { Levels 1 CollRadius 4 CollHeight 6 PickSphere 9 CrossLand TRUE RouteSpeed 6 OxygenCoef -100 } ";
constexpr unsigned Paused = 0x100000;

Lego_Object* Figure()
{
    float x, y;
    terrain_map_block_to_world(legoGlobs.currLevel->map, 2, 3, &x, &y);
    auto* obj = legoobject_create_in_world(legoGlobs.miniFigureData, 2, 0, 0, x, y, 0);
    if (CHECK(obj != nullptr, "create neutral scene object")) {
        obj->flags3 |= 0x20400170;
    }
    return obj;
}

void TimingAndAir()
{
    auto* figure = Figure();
    if (!figure) return;
    auto* level = legoGlobs.currLevel;
    objective_stop_showing_objective();
    CHECK_EQ(interface_is_item_available(1), 0); // no configured admission facility
    CHECK(lego_engine_tick(1), "initial simulation tick");
    level->oxygenLevel = level->f_204 = 100;
    level->oxygenRate = 0.1f;
    CHECK_EQ(figure->stats->OxygenCoef, -100.0f);
    lego_game_speed_set(1.0f);
    const auto initialCounter = figure->field_3d4;
    CHECK(lego_engine_tick(25), "one real second at normal simulation speed");
    CHECK_NEAR(level->oxygenLevel, 90.0f, 0.001f);
    CHECK_NEAR(figure->field_3d4, initialCounter + 25.0f, 0.001f);
    CHECK_NEAR(level->f_204, 90.0f, 0.001f);
    CHECK_EQ(panelGlobs.airMeterOxygenLow, 0);

    lego_game_speed_set(2.0f);
    CHECK(lego_engine_tick(25), "one real second at doubled simulation speed");
    CHECK_NEAR(level->oxygenLevel, 70.0f, 0.001f);
    CHECK_NEAR(figure->field_3d4, initialCounter + 75.0f, 0.001f);
    legoGlobs.flags1 |= Paused;
    CHECK(lego_engine_tick(25), "paused simulation still processes a frame");
    CHECK_NEAR(level->oxygenLevel, 70.0f, 0.001f);
    CHECK_NEAR(figure->field_3d4, initialCounter + 75.0f, 0.001f);
    legoGlobs.flags1 &= ~Paused;
    lego_game_speed_set(1.0f);
    level->oxygenLevel = level->f_204 = 13;
    CHECK(lego_engine_tick(25), "air falls below low threshold");
    CHECK_NEAR(level->oxygenLevel, 3.0f, 0.001f);
    CHECK_NEAR(level->f_204, 3.0f, 0.001f);
    CHECK_EQ(panelGlobs.airMeterOxygenLow, 1);

    figure->stats->OxygenCoef = 100.0f;
    CHECK(lego_engine_tick(25), "positive supply crosses threshold");
    CHECK_NEAR(level->oxygenLevel, 13.0f, 0.001f);
    CHECK_EQ(panelGlobs.airMeterOxygenLow, 0);
    level->oxygenLevel = 98;
    CHECK(lego_engine_tick(25), "air supply saturates");
    CHECK_EQ(level->oxygenLevel, 100.0f);
    CHECK_EQ(level->f_204, 100.0f);
    figure->stats->OxygenCoef = -100.0f;
    level->oxygenLevel = 1;
    CHECK(lego_engine_tick(25), "empty air supply ends the mission");
    CHECK_EQ(level->oxygenLevel, 0.0f);
    CHECK_EQ(level->status, 2);
}

void ViewOwnership()
{
    auto* figure = Figure();
    if (!figure) return;
    auto* view = static_cast<Viewport*>(legoGlobs.viewMain);
    auto* topCamera = static_cast<Container*>(legoGlobs.cameraMain->contCam);
    auto* detailCamera = static_cast<Container*>(legoGlobs.cameraFP->contCam);
    CHECK_EQ(legoGlobs.viewMode, 1);
    CHECK_EQ(viewport_get_camera(view), topCamera);
    const float topClip = viewport_get_back_clip(view);
    const float topField = viewport_get_field(view);
    CHECK_EQ(message_enter_first_person_view(0), 0);
    message_select_object2(figure, 1, 0);
    CHECK_EQ(message_get_primary_selected_unit(), figure);
    if (!CHECK_EQ(message_enter_first_person_view(0), 1)) return;
    CHECK_EQ(legoGlobs.viewMode, 0);
    CHECK_EQ(legoGlobs.objectFP, figure);
    CHECK_EQ(legoGlobs.cameraFP->trackObj, figure);
    CHECK_EQ(legoGlobs.cameraFP->trackFPCameraFrame, 0);
    CHECK_EQ(viewport_get_camera(view), detailCamera);
    CHECK_NEAR(viewport_get_back_clip(view), legoGlobs.currLevel->blockSize * legoGlobs.FPClipBlocks, 0.01f);
    legoGlobs.flags1 |= Paused;
    CHECK(lego_engine_tick(1), "paused frame preserves the pending field adjustment");
    CHECK_NEAR(viewport_get_field(view), topField, 0.001f);
    objective_stop_showing_objective();
    CHECK(lego_engine_tick(1), "render an actual frame with the attached camera");
    const float firstField = (4.0f * topField + 0.9f) / 5.0f;
    CHECK_NEAR(viewport_get_field(view), firstField, 0.001f);
    CHECK_EQ(message_enter_first_person_view(1), 1);
    CHECK_EQ(legoGlobs.cameraFP->trackFPCameraFrame, 1);
    CHECK_NEAR(viewport_get_field(view), firstField, 0.001f);
    CHECK(lego_engine_tick(1), "render second attachment frame");
    CHECK_NEAR(viewport_get_field(view), (4.0f * firstField + 0.6f) / 5.0f, 0.001f);
    lego_sound_minimum_distance_update_mode(1, nullptr, 0);
    CHECK_EQ(legoGlobs.viewMode, 1);
    CHECK_EQ(legoGlobs.objectFP, nullptr);
    CHECK_EQ(legoGlobs.cameraFP->trackObj, nullptr);
    CHECK_EQ(viewport_get_camera(view), topCamera);
    CHECK_NEAR(viewport_get_back_clip(view), topClip, 0.01f);
    CHECK(lego_engine_tick(1), "render after releasing attachment");
    message_clear_selected_units();
}
}

int main(int argc, char** argv)
{
    const std::string mode = argc == 2 ? argv[1] : "air";
    if (!CHECK(mode == "air" || mode == "view", "known scenario") ||
        !synthetic_fixture::initialise_full_game(("simulation_modes_" + mode).c_str(),
                                                 "SimulationModes", Stats)) return lrr_test_result();
    if (mode == "air") TimingAndAir();
    else ViewOwnership();
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("simulation_modes");
}
