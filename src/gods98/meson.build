# SPDX-License-Identifier: LGPL-3.0-or-later
gods98_incs = [include_directories('.'), include_directories('..'),
               miniwin_inc]

gods98_lib = static_library('gods98',
  'gods98.cpp',
  'engine.cpp',
  'tickers.cpp',
  'resource.cpp',
  'texture.cpp',
  'sound.cpp',
  'input.cpp',
  'surface.cpp',
  'frameloop.cpp',
  'audio.cpp',
  'block.cpp',
  include_directories : gods98_incs,
  dependencies : [platsub_deps, platform_glue_dep],
  install : false)

gods98_dep = declare_dependency(link_with : gods98_lib,
  include_directories : include_directories('.'))

# Resource-loader validation (finding 01). Runs a synthetic WWAD round trip
# always; point test_wads_dir at the shipped wads to add real-wad checks:
#   meson setup build -Dtest_wads_dir=/path/to/shipped/game
#   meson test -C build --suite resource
if get_option('platform') == 'native'
  test_exes = []
  foreach exe : [['resource', 'test_resource.cpp'], ['texture', 'test_texture.cpp'],
                 ['sound', 'test_sound.cpp'], ['input', 'test_input.cpp'],
                 ['audio', 'test_audio.cpp'], ['block', 'test_block.cpp']]
    test_name = exe[0]
    t = executable(test_name + '_test', exe[1],
      include_directories : gods98_incs,
      dependencies : [platsub_deps],
      link_with : gods98_lib,
      install : false)
    test_exes += [[test_name, t]]
  endforeach

  test_wads = get_option('test_wads_dir')
  foreach tinfo : test_exes
    if ['resource', 'texture', 'sound', 'block'].contains(tinfo[0])
      test(tinfo[0], tinfo[1],
        args : test_wads == '' ? [] : ['--real', test_wads],
        suite : tinfo[0])
    elif tinfo[0] == 'audio'
      # Headless-safe: never depend on a real audio device in CI.
      test(tinfo[0], tinfo[1],
        args : test_wads == '' ? [] : ['--real', test_wads],
        suite : tinfo[0],
        env : ['SDL_AUDIODRIVER=dummy'])
    else
      test(tinfo[0], tinfo[1], suite : tinfo[0])
    endif
  endforeach
endif