# SPDX-License-Identifier: LGPL-3.0-or-later
# This slice is included before the shared test registrations.
include(${CMAKE_CURRENT_LIST_DIR}/../LrrTesting.cmake)

# Advisor vector operations, configuration names and request state.
lrr_unit_test(advisor SOURCES ${GAME_DIR}/test_advisor.cpp
  WRAP ddiscene_light_create ddiscene_object_pose_method_dispatch scene_render_lock_toggle
  LABELS advisor)

# AI priority registration, task matching and timer updates.
lrr_unit_test(aitask SOURCES ${GAME_DIR}/test_aitask.cpp LABELS aitask)

# Animation activity changes invalidate cached attachments and update timing.
lrr_unit_test(animation_object_activity SOURCES ${GAME_DIR}/test_animation_object_activity.cpp
  WRAP scene_action_name_set ddiscene_frame_update_and_callbacks
    ddiscene_advance_by_time_target ddiscene_object_frame_position_get
  LABELS animation_object)

# Bubble names, the visibility setting and priority between status and task icons.
lrr_unit_test(bubbles SOURCES ${GAME_DIR}/test_bubbles.cpp
  WRAP block_subgroup Image_LoadBMPScaled Image_SetupTrans
  LABELS bubbles)

# Building power smoothing, footprint access and cached attachment points.
lrr_unit_test(building_model SOURCES ${GAME_DIR}/test_building_model.cpp LABELS building_model)

# Segment and polyline distances and unclamped line projections.
lrr_unit_test(collision SOURCES ${GAME_DIR}/test_collision.cpp LABELS collision)

# Construction-zone membership, resource counting, and readiness.
lrr_unit_test(construction SOURCES ${GAME_DIR}/test_construction.cpp LABELS construction)

# Building refunds are configurable and each building refunds at most once.
lrr_unit_test(entity_cell_effects SOURCES ${GAME_DIR}/test_entity_cell_effects.cpp
  WRAP legoobject_position_to_terrain_cell entity_indexed_record_word_at_b0_get
    entity_indexed_record_word_at_b4_get stats_table_word_at_b8
    legoobject_build_placement_command_setup legoobject_build_placement_alternate_command_setup
  LABELS entity_cell_effects)

# Damage number slot exhaustion, expiry, and eligibility.
lrr_unit_test(screen_effect_quads SOURCES ${GAME_DIR}/test_screen_effect_quads.cpp
  WRAP scene_render_lock_toggle entity_state_flags_get
  LABELS screen_effect_quads)

# Dependency level thresholds, enable toggle, and sticky prerequisites.
lrr_unit_test(dependencies SOURCES ${GAME_DIR}/test_dependencies.cpp
  WRAP block_subgroup lego_entity_type_pair_lookup building_upgrade_admission_check
    legoobject_run_through_lists_skip_upgrade_parts legoobject_task_kind_filtered_search
  LABELS dependencies)

# Terrain detail mesh transform and packed texture selection.
lrr_unit_test(detail SOURCES ${GAME_DIR}/test_detail.cpp LABELS detail)

# Rockfall style names and the fallback loader for miscellaneous effects.
lrr_unit_test(rock_fall_styles SOURCES ${GAME_DIR}/test_rock_fall_styles.cpp
  WRAP container_load scene_render_lock_toggle
  LABELS rock_fall_styles)

# Fence registration, node reuse and stud placement across a gap.
lrr_unit_test(terrain_cell_objects SOURCES ${GAME_DIR}/test_terrain_cell_objects.cpp
  WRAP lego_cell_init_pass terrain_cell_occupied_or_flag_current
  LABELS terrain_cell_objects)

# Encyclopedia selection, one-shot text, and marker cleanup.
lrr_unit_test(encyclopedia SOURCES ${GAME_DIR}/test_encyclopedia.cpp
  WRAP ui_text_output_object_release_target panel_text_window_print_f
    ui_text_output_position_and_draw lego_selected_entity_ground_marker_render
  LABELS encyclopedia)

# Erosion neighbor filtering and lock expiry on a synthetic level.
lrr_unit_test(erode SOURCES ${GAME_DIR}/test_erode.cpp LABELS erode)

# Landslide eligibility, including reserved cells and safe-cavern edges.
lrr_unit_test(fallin SOURCES ${GAME_DIR}/test_fallin.cpp LABELS fallin)

# Steering group arrival hides only followers that have reached their targets.
lrr_unit_test(steering_arrival SOURCES ${GAME_DIR}/test_steering_arrival.cpp
  WRAP scene_render_lock_toggle LABELS steering)

# Frontend rectangle bounds, menu item types and dynamic menu growth.
lrr_unit_test(frontend_unit SOURCES ${GAME_DIR}/test_frontend_unit.cpp LABELS frontend_unit)
