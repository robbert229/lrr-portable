# Reconstruction source tree.
#
# Today: one compiling stub translation unit per planned subsystem so `meson
# compile` proves the toolchain, backend seam, static-lib split, resources
# and executable shell end to end. Later the other agent's decompiled modules
# land inside the existing subdirectories (docs/adr/0003).
#
# Subsystems mirror the gods98 codeline:
#   gods98/    engine core (Main, Viewports, Images, Input, Sounds, Movies…)
#   game/      game layer (entities, missions, commands, selection…)
#   platform/  the portability seam — backend entry points + platform.h

proj_inc = include_directories('.')

# Order matters: platform defines miniwin_inc / platsub_deps (miniwin headers +
# SDL3 include paths) consumed by the gods98 lib below.
subdir('platform')   # defines platform_entry, platsub_deps, miniwin_inc
subdir('gods98')
subdir('game')

# Resource-loader validation (finding 01). Runs a synthetic WWAD round trip
# always; point test_wads_dir at the shipped wads to add real-wad checks:
#   meson setup build -Dtest_wads_dir=/path/to/shipped/game
#   meson test -C build --suite resource
if get_option('platform') == 'native'
  test_exes = []
  foreach exe : [['resource', 'gods98/test_resource.cpp'], ['texture', 'gods98/test_texture.cpp'],
                 ['sound', 'gods98/test_sound.cpp'], ['input', 'gods98/test_input.cpp'],
                 ['audio', 'gods98/test_audio.cpp'], ['camera', 'gods98/test_camera.cpp'],
                 ['view', 'gods98/test_view.cpp'],
                 ['message', 'gods98/test_message.cpp'],
                 ['airbeat', 'gods98/test_airbeat.cpp'],
                 ['mainsim', 'gods98/test_mainsim.cpp'],
                 ['simtick', 'gods98/test_simtick.cpp'],
                 ['enginebind', 'gods98/test_engine_bind.cpp'],
                 ['block', 'gods98/test_block.cpp'], ['model', 'gods98/test_model.cpp'],
                 ['entity_ae', 'gods98/test_entity.cpp'], ['movie', 'gods98/test_movie.cpp'],
                 ['flic', 'gods98/test_flic.cpp'], ['media', 'gods98/test_media.cpp'],
                 ['crt', 'crt/test_crt.cpp'],
                 ['draw', 'gods98/test_draw.cpp']]
    test_name = exe[0]
    t = executable(test_name + '_test', exe[1],
      include_directories : gods98_incs,
      dependencies : [platsub_deps, movie_dl_dep],
      link_with : [gods98_lib, game_lib, gods98_lib, game_lib],
      install : false)
    test_exes += [[test_name, t]]
  endforeach

  # These header-boundary checks mirror the standalone CMake targets.
  registry_unit = configure_file(input : 'gods98/g98_029.c',
    output : 'registry_g98_029.c.cpp', copy : true)
  registry_test = executable('registry_test', 'gods98/test_registry.cpp', registry_unit,
    include_directories : [proj_inc, include_directories('gods98'), miniwin_inc],
    dependencies : [platsub_deps])
  test('registry', registry_test)
  c_boundaries_test = executable('c_boundaries_test', 'game/test_c_boundaries.cpp',
    include_directories : [proj_inc, miniwin_inc], dependencies : [platsub_deps])
  test('c_boundaries', c_boundaries_test)

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


platform = get_option('platform')
if platform == 'native'
  exe_kwargs = {}
  extra_sources = []
elif platform == 'winapi'
  windows = import('windows')
  exe_kwargs = {'win_subsystem' : 'windows'}
  extra_sources = [windows.compile_resources('version.rc')]
else
  error('unknown platform: ' + platform)
endif

executable('LegoRR',
  sources : ['main.cpp', 'game/dependency_grid.cpp',
             'game/screen_pick.cpp',
             platform_entry, extra_sources],
  include_directories : [proj_inc, miniwin_inc],
  dependencies : [platsub_deps, gods98_dep, game_dep],
  install : true,
  kwargs : exe_kwargs)

# Output name stays `LegoRR` (becomes LegoRR.exe on the winapi reference
# build) so the A.2 harness can launch it from the staged game dir by the
# exact case-sensitive name it expects (tools/compat/a2.py, docs/adr/0002).
# The native build produces the same-named ELF for drop-in A/B runs.
