# SPDX-License-Identifier: LGPL-3.0-or-later
# The tests of the game units (src/game).
# CMakeLists.txt includes this file inside if(BUILD_TESTING), after
# cmake/LrrTesting.cmake; docs/building.md, "Writing a test".

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

# Help eligibility, tutorial gating, and recognition of button release.
lrr_unit_test(helpwindow SOURCES ${GAME_DIR}/test_helpwindow.cpp
  WRAP text_buffer_reset_target nerpfunc_get_tutorial_flags
  LABELS helpwindow)

# Info message name lookup and bounded instance queue behavior.
lrr_unit_test(info_unit SOURCES ${GAME_DIR}/test_info_unit.cpp LABELS info_unit)

# Interface menu slide transitions and independent icon state.
lrr_unit_test(interface_unit SOURCES ${GAME_DIR}/test_interface_unit.cpp
  WRAP sound_alias_play LABELS interface_unit)

# Lego's level resource accounting and object catalog lookups.
lrr_unit_test(lego_unit SOURCES ${GAME_DIR}/test_lego_unit.cpp
  WRAP ui_overlay_animation_advance LABELS lego_unit)

# Camera zoom range, free movement, and acceleration on a normalized direction.
lrr_unit_test(camera_control SOURCES ${GAME_DIR}/test_camera_control.cpp LABELS camera_control)

# Live object work eligibility and capacity of the teleport request queue.
lrr_unit_test(legoobject_unit SOURCES ${GAME_DIR}/test_legoobject_unit.cpp LABELS legoobject_unit)

# Spotlight dimmer transitions and movement limits relative to the light.
lrr_unit_test(lighteffects SOURCES ${GAME_DIR}/test_lighteffects.cpp
  WRAP render_object_set_packed_rgba_from_float_channels ddiscene_object_position_get
  LABELS lighteffects)

# Loader profile round-trip and progress bar geometry with synthetic files.
lrr_unit_test(loader SOURCES ${GAME_DIR}/test_loader.cpp
  WRAP Image_LoadBMPScaled Image_DisplayScaled Image_Remove main_loop_frame_pump
  LABELS loader)

# The units' own tests: each calls one unit directly.
foreach(name IN ITEMS bezier steering animation_object reward savegame)
  lrr_unit_test(${name} SOURCES ${GAME_DIR}/test_${name}.cpp LABELS ${name})
endforeach()
# FrontEnd.c's random menu overlay roll on a simulated clock: the unit's
# calls to main_time_get reach the test's __wrap_main_time_get. WRAP needs
# GNU ld's --wrap; lrr_unit_test leaves these tests out where the linker
# has none.
lrr_unit_test(overlay_roll SOURCES ${GAME_DIR}/test_overlay_roll.cpp
  WRAP main_time_get
  LABELS frontend)
# The menu edge scroll and the reward screen scroll on a simulated clock.
lrr_unit_test(menu_frame_rate SOURCES ${GAME_DIR}/test_menu_frame_rate.cpp
  WRAP main_time_get
  LABELS frontend)
# LegoObject.c's flock update reads the flock's own hasPosition, not
# the getter the linker folded it into: the unit's calls to the Stats.c
# and Container.c getters reach the test's __wrap_ stubs.
lrr_unit_test(flock_position SOURCES ${GAME_DIR}/test_flock_position.cpp
  WRAP
    entity_stats_word_get
    entity_stats_float_at120
    entity_stats_float_at124
    entity_stats_float_at128
    entity_stats_float_at12c
    entity_stats_float_at130
    entity_stats_float_at134
    entity_stats_float_at138
    ddiscene_object_position_get
  LABELS legoobject)
# AITask.c's job assignment with tweaks:fix_idleness_syndrome off and on:
# a job with a unit on its way keeps no idle unit from the next job, a
# unit that cannot reach a job does not pause it for the others, and an
# idle unit is no job's unit on its way. The unit's calls to the map,
# object, message and config units reach the test's __wrap_ stubs.
lrr_unit_test(idleness_syndrome SOURCES ${GAME_DIR}/test_idleness_syndrome.cpp
  WRAP
    _Z26LRR_GetFixIdlenessSyndromev
    level_current_terrain_get
    selection_contains_entity_active_target
    lego_entity_matches_current_selection_or_owner
    entity_stats_word_get
    legoobject_type_two_target_resolve_and_apply
    legoobject_transform_position_get
    legoobject_position_to_terrain_cell
    legoobject_surface_vector_normalize
    legoobject_adjacent_position_candidate_find
    terrain_map_block_to_world
    terrain_map_world_to_block
    legoobject_grid_route_search_adapter
    cmd_produce
    idle_register
  LABELS aitask)
# LegoObject.c's run away frees only the route lists it allocated: the
# unit's calls to free reach the test's __wrap_free, which records a
# stack address instead of aborting on it.
lrr_unit_test(run_away SOURCES ${GAME_DIR}/test_run_away.cpp
  WRAP
    free
    ddiscene_object_position_get
    entity_stats_word_get
    entity_capability_flags_get
    entity_state_flags_get
    idle_register
  LABELS legoobject)
# camera_control.c's radar camera moves its root by at most maxMove a frame: the
# unit's calls to the scene's position, facing and pose functions reach
# the test's __wrap_ stubs, which move fake frames.
lrr_unit_test(camera_track SOURCES ${GAME_DIR}/test_camera_track.cpp
  WRAP
    ddiscene_object_position_get
    ddiscene_object_position_set
    ddiscene_object_facing_pair_get
    ddiscene_object_pose_method_dispatch
    render_object_rotate_axis
    scene_object_camera_teleport_or_offset
    legoobject_type_component_get
    lego_flags1_bit17_set
  LABELS camera)
# Info.c's $name$ tokens carry the whole image pointer to the panel
# print and to Font's "%b": the units' image and panel calls reach the
# test's __wrap_ stubs.
lrr_unit_test(info_image_token SOURCES ${GAME_DIR}/test_info_image_token.cpp
  WRAP
    Image_LoadBMPScaled
    Image_SetPenZeroTrans
    Image_DisplayScaled
    panel_text_window_print_f
    panel_state_flags_toggle
    ui_text_output_object_release_target
    ui_text_output_position_and_draw
    ui_record_callback_forward
    indexed_text_metrics_pair_set
    indexed_text_metrics_pair_get
    sound_submit_with_global_cooldown
  LABELS info)
# The info panel prints a token's "%b" as the original's VC5 LIBC does,
# through the real panel print, TextWindow and the units' vsprintf.
lrr_unit_test(info_panel_percent_b SOURCES ${GAME_DIR}/test_info_panel_percent_b.cpp
  WRAP
    Image_LoadBMPScaled
    Image_SetPenZeroTrans
    Image_DisplayScaled
    panel_state_flags_toggle
    ui_text_output_object_release_target
    ui_text_output_position_and_draw
    ui_record_callback_forward
    indexed_text_metrics_pair_set
    indexed_text_metrics_pair_get
    sound_submit_with_global_cooldown
  LABELS info)
# Lego.cfg lays the interface out only for 640x480: another mode reads
# that layout ([divergence:layout-fallback]). The units' Panel.c calls
# and the file read reach the test's __wrap_ stubs.
lrr_unit_test(layout_fallback SOURCES ${GAME_DIR}/test_layout_fallback.cpp
  WRAP
    file_load_binary
    legacy_overlay_image_record_load
    panel_category_bounds_set
    panel_texture_slots_allocate
    case_insensitive_name_table_lookup
  LABELS display)
# The block grids' index (game/block_grid.h): a NaN position's INT_MIN
# coordinates resolve to block 0, as the original's 32-bit arithmetic.
# GNU ld's --wrap, as the frontend tests above.
lrr_unit_test(block_grid SOURCES ${GAME_DIR}/test_block_grid.cpp
  WRAP
    ddiscene_object_position_get
    entity_state_flags_get
    container_mesh_hide_group
    scene_mesh_color_and_alpha_set
  LABELS block_grid)
# LegoObject.c keeps a carried object's facing when its carrier stands
# still, instead of the original's 0/0 NaN. The carry null and the
# carried object's container are stand-ins.
lrr_unit_test(carried_facing SOURCES ${GAME_DIR}/test_carried_facing.cpp
  WRAP
    renderer_cached_handle_48_get_or_create
    ddiscene_object_position_get
    ddiscene_object_position_set
    ddiscene_object_pose_method_dispatch
  LABELS carried_facing)

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

# Sound name registration, sample markers, deferred playback and cooldown.
lrr_unit_test(sfx SOURCES ${GAME_DIR}/test_sfx.cpp
  WRAP sound_bank_entry_load s_play_seam sound_source_spatial_refresh_all LABELS sfx)

# Spiderweb damage, teardown and restart, beyond the existing grid tests.
lrr_unit_test(spiderweb SOURCES ${GAME_DIR}/test_spiderweb.cpp
  WRAP legoobject_position_to_terrain_cell object_type_name_index_find weapons_get_damage_for_object
  LABELS spiderweb)

# Stats level changes, counters and tool lookup.
lrr_unit_test(entity_stats SOURCES ${GAME_DIR}/test_entity_stats.cpp LABELS entity_stats)

# Teleporter object types, ring distance and instant completion.
lrr_unit_test(teleporter SOURCES ${GAME_DIR}/test_teleporter.cpp
  WRAP legoobject_run_through_lists_skip_upgrade_parts legoobject_transform_position_get
       level_current_terrain_get camera_get_topdown_world_pos
  LABELS teleporter)

# Message formatting, replacement rules and tutorial text reset.
lrr_unit_test(text SOURCES ${GAME_DIR}/test_text.cpp LABELS text)

# Tooltip hover delay and the laid-out text, box and icon rows.
lrr_unit_test(tooltip SOURCES ${GAME_DIR}/test_tooltip.cpp
  WRAP surface_fill_pixel_spans surface_draw_polyline font_print_f Image_DisplayScaled
  LABELS tooltip)

# Configured upgrade parts and rebuilding the list when a level changes.
lrr_unit_test(upgrade SOURCES ${GAME_DIR}/test_upgrade.cpp
  WRAP lego_upgrade_name_entry_find legoobject_create legoobject_remove LABELS upgrade)

# Vehicle upgrade masks, animation overrun and cached attachment lookup.
lrr_unit_test(vehicle_model SOURCES ${GAME_DIR}/test_vehicle_model.cpp
  WRAP ddiscene_advance_by_time_target ddiscene_object_frame_position_get
       xf_texture_name_format resource_name_register_or_query_target
  LABELS vehicle_model)

# Pool connectivity and opening and sealing an outlet.
lrr_unit_test(water SOURCES ${GAME_DIR}/test_water.cpp
  WRAP scene_render_lock_toggle terrain_map_block_vertices_get LABELS water)

# Weapon configuration defaults and overrides, hit bounds and model timers.
lrr_unit_test(weapons SOURCES ${GAME_DIR}/test_weapons.cpp
  WRAP lego_entity_type_pair_lookup building_upgrade_admission_check LABELS weapons)
# Lego.c's level loader on a synthetic 8x8 level and a headless software
# device, then the simulation for a while: the path a game session takes
# through lego_level_load and the main simulation step.
lrr_unit_test(level_boot SOURCES ${GAME_DIR}/test_level_boot.cpp
  ${PROJECT_SOURCE_DIR}/src/test/synthetic_level_fixture.cpp
  LABELS level_boot ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")

# Existing lifecycle entry points on generated inputs in an isolated process.
lrr_unit_test(synthetic_lifecycle SOURCES ${GAME_DIR}/test_synthetic_lifecycle.cpp
  ${PROJECT_SOURCE_DIR}/src/test/synthetic_level_fixture.cpp
  LABELS level_boot ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")
lrr_unit_test_variant(synthetic_missing_config OF synthetic_lifecycle
  ARGS missing-config LABELS level_boot ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")

# Real setup and cleanup of individual subsystems in independent processes.
lrr_unit_test(synthetic_startup_shutdown SOURCES ${GAME_DIR}/test_synthetic_startup_shutdown.cpp
  ${PROJECT_SOURCE_DIR}/src/test/synthetic_level_fixture.cpp
  ARGS reward LABELS level_boot ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")
foreach(scenario IN ITEMS reward-missing-image interface scene)
  lrr_unit_test_variant(synthetic_startup_${scenario} OF synthetic_startup_shutdown
    ARGS ${scenario} LABELS level_boot ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")
endforeach()
# Synthetic resource classification and carrier admission.
lrr_unit_test(mining_logistics SOURCES ${GAME_DIR}/test_mining_logistics.cpp
  LABELS mining_logistics)

# Real route search across an obstructed synthetic map.
lrr_unit_test(navigation_synthetic SOURCES ${GAME_DIR}/test_navigation_synthetic.cpp
  LABELS navigation)

# Health changes and tool capacity/replacement on a synthetic raider.
lrr_unit_test(raider_synthetic SOURCES ${GAME_DIR}/test_raider_synthetic.cpp
  LABELS raider)

# Pending task reassignment and pool-copy ownership.
lrr_unit_test(task_lifecycle SOURCES ${GAME_DIR}/test_task_lifecycle.cpp
  LABELS aitask)
