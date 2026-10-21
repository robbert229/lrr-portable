game_lib = static_library('game',
  'game.cpp',
  include_directories : include_directories('.'),
  install : false)

game_dep = declare_dependency(link_with : game_lib,
  include_directories : include_directories('.'))