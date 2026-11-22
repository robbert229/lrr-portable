# SPDX-License-Identifier: LGPL-3.0-or-later

# Terrain coordinate transforms, directions and texture fade mappings.
lrr_unit_test(terrain_map SOURCES ${GAME_DIR}/test_terrain_map.cpp LABELS terrain_map)

# Message-name lookup and dispatch of queued move requests.
lrr_unit_test(message_unit SOURCES ${GAME_DIR}/test_message_unit.cpp
  WRAP ptl_translate_event aitask_cmd_create_move LABELS message_unit)

# NERPs registers, script argument decoding and timer updates.
lrr_unit_test(tutorial_script SOURCES ${GAME_DIR}/test_tutorial_script.cpp LABELS tutorial_script)

# Resource quotas, object conditions and timer outcomes.
lrr_unit_test(objective_unit SOURCES ${GAME_DIR}/test_objective_unit.cpp
  WRAP legoobject_run_through_lists_skip_upgrade_parts legoobject_position_to_terrain_cell
  LABELS objective_unit)

# Raider recall order and snapshot/restore, beyond the save-image tests.
lrr_unit_test(objectrecall SOURCES ${GAME_DIR}/test_objectrecall.cpp
  WRAP entity_building_index_set legoobject_rename LABELS objectrecall)

# Health bar configuration and the geometry emitted without a renderer.
lrr_unit_test(objinfo SOURCES ${GAME_DIR}/test_objinfo.cpp
  WRAP entity_quad_mesh_render_record_allowed surface_fill_pixel_spans surface_draw_polyline
  LABELS objinfo)

# Panel lookup, hit bounds, control geometry and slide state.
lrr_unit_test(panel_unit SOURCES ${GAME_DIR}/test_panel_unit.cpp
  WRAP Image_DisplayScaled LABELS panel_unit)

# Cursor name lookup and temporary override expiry.
lrr_unit_test(pointer SOURCES ${GAME_DIR}/test_pointer.cpp LABELS pointer)

# Priority configuration, ordering, toggle state and reset.
lrr_unit_test(priorities SOURCES ${GAME_DIR}/test_priorities.cpp
  WRAP media_action_feedback_sound_dispatch LABELS priorities)

# Per-level event translation preserves the message payload.
lrr_unit_test(ptl SOURCES ${GAME_DIR}/test_ptl.cpp
  WRAP block_open block_close LABELS ptl)

# Radar bounds and the screen/world transforms on a synthetic map.
lrr_unit_test(radarmap SOURCES ${GAME_DIR}/test_radarmap.cpp LABELS radarmap)

# Weighted mission scoring, beyond the existing reward record test.
lrr_unit_test(reward_score SOURCES ${GAME_DIR}/test_reward_score.cpp
  WRAP level_current_owner_get LABELS reward)

# Roof diagonals and normal updates using captured mesh vertices.
lrr_unit_test(roof SOURCES ${GAME_DIR}/test_roof.cpp
  WRAP container_mesh_set_vertices lwo_layer_polygon_vertex_data_read LABELS roof)

# Scrollbar arrow input, thumb dragging and rounded positions.
lrr_unit_test(scrollinfo SOURCES ${GAME_DIR}/test_scrollinfo.cpp
  WRAP Image_LoadBMPScaled Image_DisplayScaled LABELS scrollinfo)

# Placement footprint rotation and translation on the block grid.
lrr_unit_test(selectplace SOURCES ${GAME_DIR}/test_selectplace.cpp LABELS selectplace)
