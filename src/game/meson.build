# SPDX-License-Identifier: LGPL-3.0-or-later
# Game layer — mission plumbing (findings 07/08/09/10/22) plus shared helpers.
#
# The game layer links against the gods98 static lib for the resource /
# block-text-config primitives (findings 01/12), so its tests can run the
# full load path from loose fixture files.

gods98_incs = [include_directories('..', '../gods98')]  # repo-root + gods98

game_lib = static_library('game',
  'game.cpp',
  'level.cpp',
  'objective.cpp',
  'savegame.cpp',
  'mission.cpp',
  'entity.cpp',
  'cmd.cpp',
  'select.cpp',
  'ai.cpp',
  'render.cpp',
  'frontend.cpp',
  'util.cpp',
  include_directories : gods98_incs,
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
                 ['frontend', 'test_frontend.cpp']]
    test_name = exe[0]
    t = executable(test_name + '_test', exe[1],
      include_directories : gods98_incs + [miniwin_inc],
      dependencies : [platsub_deps],
      link_with : [gods98_lib, game_lib],
      install : false)
    test_wads = get_option('test_wads_dir')
    test(test_name, t,
      args : test_wads == '' ? [] : ['--real', test_wads],
      suite : test_name,
      workdir : meson.current_build_dir())
  endforeach
endif
