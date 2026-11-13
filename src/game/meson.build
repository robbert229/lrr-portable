# SPDX-License-Identifier: LGPL-3.0-or-later
# Game layer — mission plumbing (findings 07/08/09/10/22) plus shared helpers.
#
# The game layer links against the gods98 static lib for the resource /
# block-text-config primitives (findings 01/12), so its tests can run the
# full load path from loose fixture files.

gods98_incs = [include_directories('..', '../gods98'), miniwin_inc]  # repo-root + gods98

building_scene_sources = []
if get_option('platform') == 'winapi'
  building_scene_sources += ['building_scene.cpp']
endif

game_sources = [
  'aitask_port.cpp',
  'message_unit.cpp',
  'legoobject_port.cpp',
  'frontend_unit.cpp',
  'interface_port.cpp',
  'weapons_port.cpp',
  'lego_port.cpp',
  'entity_stats_port.cpp',
  'creature_scene.cpp',
  'reward_port.cpp',
  'camera_control_port.cpp',
  'helpwindow_port.cpp',
  'tile_variants_port.cpp',
  'cell_flood_passes_port.cpp',
  'vehicle_model_port.cpp',
  'upgrade_levels_port.cpp',
  'tooltip_port.cpp',
  'text_port.cpp',
  'record_chains_port.cpp',
  'grid_cell_records.cpp',
  'sfx_port.cpp',
  'placement_footprint.cpp',
  'scrollinfo_port.cpp',
  'terrain_overlay_quads.cpp',
  'map_view_port.cpp',
  'level_id_remap.cpp',
  'pointer_port.cpp',
  'object_overlays.cpp',
  'object_state_file.cpp',
  'objective_unit.cpp',
  'tutorial_script_port.cpp',
  'terrain_map_port.cpp',
  'loading_progress.cpp',
  'lighteffects_port.cpp',
  'bubbles_port.cpp',
  'info_port.cpp',
  'terrain_cell_objects_port.cpp',
  'rock_fall_styles_port.cpp',
  'dependencies_port.cpp',
  'entity_cell_effects.cpp',
  'placement_port.cpp',
  'encyclopedia.cpp',
  'lava_erosion_port.cpp',
  'screen_effect_quads_port.cpp',
  'building_model_port.cpp',
  'cmd.cpp',
  'dependency_grid.cpp',
  'options.cpp',
  'savegame.cpp',
  'util.cpp',
  'upgrades.cpp',
  'frontend.cpp',
  'select.cpp',
  'entity_config.cpp',
  'level.cpp',
  'scene_render.cpp',
  'ai.cpp',
  'terrain_grid.cpp',
  'entity.cpp',
  'spawn.cpp',
  'screen_pick.cpp',
  'objective.cpp',
  'tutorial_hud.cpp',
  'ui_list.cpp',
  'mission.cpp',
  'effects.cpp',
  'weapon.cpp',
  'video_graph.cpp',
  'startup_config.cpp',
  'entity_motion_owner.cpp',
  'game.cpp',
  'opgroup.cpp',
  'render.cpp',
  'tutorial_world_arrows.cpp',
]
game_c_sources = [
  'advisor.c',
  'aitask.c',
  'animation_object.c',
  'bezier.c',
  'camera_control.c',
  'entity_stats.c',
  'info_unit.c',
  'interface_unit.c',
  'lego_unit.c',
  'legoobject_unit.c',
  'panel_unit.c',
  'priorities.c',
  'reward.c',
  'steering.c',
  'terrain_cell_objects.c',
  'terrain_map.c',
  'tutorial_script.c',
  'weapons.c',
  'sfx.c',
  'rock_fall_styles.c',
  'vehicle_model.c',
  'building_model.c',
  'lighteffects.c',
  'helpwindow.c',
  'tooltip.c',
  'bubbles.c',
  'erode.c',
  'fallin.c',
  'text.c',
  'dependencies.c',
  'pointer.c',
  'radarmap.c',
  'collision.c',
  'construction.c',
  'scrollinfo.c',
  'screen_effect_quads.c',
  'upgrade.c',
  'detail.c',
  'water.c',
  'teleporter.c',
]
# Native/reference builds compile original-shape C units as C++ (ADR 0005).
game_c_as_cpp = []
foreach source : game_c_sources
  game_c_as_cpp += configure_file(input : source, output : '@PLAINNAME@.cpp', copy : true)
endforeach

game_lib = static_library('game',
  game_sources, game_c_as_cpp, building_scene_sources,
  include_directories : gods98_incs + [miniwin_inc],
  dependencies : [platsub_deps],
  install : false)

game_dep = declare_dependency(link_with : game_lib,
  include_directories : [include_directories('.'), include_directories('..')])

# Game-layer tests: level (.map load), objective (finding 22), savegame
# (.sav/.osf), mission (task/reward, finding 09), entity (.ol + tables,
# finding 10). All run from loose synthetic fixtures (suite = test workdir);
# point test_wads_dir at the shipped wads to add a real-data probe to level
# (best-effort, skipped when data is absent). The block-text parser itself
# is covered by gods98's `block` suite (finding 12, src/gods98/test_block.cpp).
if get_option('platform') == 'native'
  foreach exe : [['level', 'test_level.cpp'],
                 ['objective', 'test_objective.cpp'],
                 ['savegame', 'test_savegame.cpp'],
                 ['mission', 'test_mission.cpp'],
                 ['entity', 'test_entity.cpp'],
                 ['cmd', 'test_cmd.cpp'],
                 ['select', 'test_select.cpp'],
                 ['ai', 'test_ai.cpp'],
                 ['render', 'test_render.cpp'],
['bind', 'test_bind.cpp'],
                 ['frontend', 'test_frontend.cpp'],
                 ['options', 'test_options.cpp'],
                 ['bezier', 'test_bezier.cpp'],
                 ['steering', 'test_steering.cpp'],
                 ['animation_object', 'test_animation_object.cpp']]
    test_name = exe[0]
    t = executable(test_name + '_test', exe[1],
      include_directories : gods98_incs + [miniwin_inc],
      dependencies : [platsub_deps],
      link_with : [gods98_lib, game_lib],
      install : false)
    test_wads = get_option('test_wads_dir')
    test(test_name, t,
      args : test_wads != '' and test_name == 'level' ? ['--real', test_wads] : [],
      suite : test_name,
      workdir : meson.current_build_dir())
  endforeach
endif
