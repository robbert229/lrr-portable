# SPDX-License-Identifier: LGPL-3.0-or-later
# Game layer — mission plumbing (findings 07/08/09/22) plus shared helpers.
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
  'util.cpp',
  include_directories : gods98_incs,
  install : false)

game_dep = declare_dependency(link_with : game_lib,
  include_directories : [include_directories('.'), include_directories('..')])

# Game-layer tests: config (block-text), level (.map load), objective
# (finding 22), savegame (.sav/.osf). All run from loose synthetic fixtures
# (suite dir = test workdir); point test_wads_dir at the shipped wads to add
# real-data checks (best-effort, skipped when data is absent).
if get_option('platform') == 'native'
  foreach exe : [['config', 'test_config.cpp'], ['level', 'test_level.cpp'],
                 ['objective', 'test_objective.cpp'], ['savegame', 'test_savegame.cpp']]
    test_name = exe[0]
    t = executable(test_name + '_test', exe[1],
      include_directories : gods98_incs,
      dependencies : [],
      link_with : [gods98_lib, game_lib],
      install : false)
    test_wads = get_option('test_wads_dir')
    test(test_name, t,
      args : test_wads == '' ? [] : ['--real', test_wads],
      suite : test_name,
      workdir : meson.current_build_dir())
  endforeach
endif
