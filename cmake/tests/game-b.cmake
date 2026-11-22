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
