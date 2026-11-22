# SPDX-License-Identifier: LGPL-3.0-or-later

# Terrain coordinate transforms, directions and texture fade mappings.
lrr_unit_test(terrain_map SOURCES ${GAME_DIR}/test_terrain_map.cpp LABELS terrain_map)

# Message-name lookup and dispatch of queued move requests.
lrr_unit_test(message_unit SOURCES ${GAME_DIR}/test_message_unit.cpp
  WRAP ptl_translate_event aitask_cmd_create_move LABELS message_unit)

# NERPs registers, script argument decoding and timer updates.
lrr_unit_test(tutorial_script SOURCES ${GAME_DIR}/test_tutorial_script.cpp LABELS tutorial_script)
