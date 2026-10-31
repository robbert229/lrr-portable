# SPDX-License-Identifier: LGPL-3.0-or-later
gods98_incs = [include_directories('.'), include_directories('..'),
               miniwin_inc]

gods98_lib = static_library('gods98',
  'gods98.cpp',
  'string.cpp',
  '../crt/crt.cpp',
  '../crt/crt_heap.cpp',
  '../crt/crt_runtime_calls.cpp',
  '../crt/crt_runtime_aliases.cpp',
  'heap.cpp',
  'ptrstack.cpp',
  'record.cpp',
  'engine.cpp',
  'tickers.cpp',
  'resource.cpp',
  'texture.cpp',
  'sound.cpp',
  'input.cpp',
  'surface.cpp',
  'frameloop.cpp',
  'audio.cpp',
  'camera.cpp',
  'view.cpp',
  'message.cpp',
  'mainsim.cpp',
  'simtick.cpp',
  'block.cpp',
  'model.cpp',
  'entity.cpp',
  'movie.cpp',
  'flic.cpp',
  'media.cpp',
  'scene_pool.cpp',
  'scene_resources.cpp',
  include_directories : gods98_incs,
  dependencies : [platsub_deps, platform_glue_dep],
  install : false)

gods98_dep = declare_dependency(link_with : gods98_lib,
  include_directories : include_directories('.'),
  dependencies : [movie_dl_dep, platsub_deps])

# Resource-loader validation (finding 01). Runs a synthetic WWAD round trip
# always; point test_wads_dir at the shipped wads to add real-wad checks:
#   meson setup build -Dtest_wads_dir=/path/to/shipped/game
#   meson test -C build --suite resource
if get_option('platform') == 'native'
  test_exes = []
  foreach exe : [['resource', 'test_resource.cpp'], ['texture', 'test_texture.cpp'],
                 ['sound', 'test_sound.cpp'], ['input', 'test_input.cpp'],
                 ['audio', 'test_audio.cpp'], ['camera', 'test_camera.cpp'],
                 ['view', 'test_view.cpp'],
                 ['message', 'test_message.cpp'],
                 ['airbeat', 'test_airbeat.cpp'],
                 ['mainsim', 'test_mainsim.cpp'],
                 ['simtick', 'test_simtick.cpp'],
                 ['enginebind', 'test_engine_bind.cpp'],
                 ['block', 'test_block.cpp'], ['model', 'test_model.cpp'],
                 ['entity_ae', 'test_entity.cpp'], ['movie', 'test_movie.cpp'],
                 ['flic', 'test_flic.cpp'], ['media', 'test_media.cpp'],
                 ['string', 'test_string.cpp'],
                 ['crt', '../crt/test_crt.cpp'],
                 ['heap', 'test_heap.cpp'],
                 ['ptrstack', 'test_ptrstack.cpp'],
                 ['record', 'test_record.cpp'],
                 ['surface', 'test_surface.cpp']]
    test_name = exe[0]
    t = executable(test_name + '_test', exe[1],
      include_directories : gods98_incs,
      dependencies : [platsub_deps, movie_dl_dep],
      link_with : gods98_lib,
      install : false)
    test_exes += [[test_name, t]]
  endforeach

  test_wads = get_option('test_wads_dir')
  foreach tinfo : test_exes
    # `-Dmovies=disabled` compiles the movie module out (stub); drop its test
    # from the suite so the test summary reflects the configuration.
    if get_option('movies').disabled() and tinfo[0] == 'movie'
      continue
    endif
    if ['resource', 'texture', 'sound', 'block', 'model', 'entity_ae',
        'movie', 'flic'].contains(tinfo[0])
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
