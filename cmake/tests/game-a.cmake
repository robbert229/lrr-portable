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
