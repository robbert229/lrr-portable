# SPDX-License-Identifier: LGPL-3.0-or-later
gods98_lib = static_library('gods98',
  'gods98.cpp',
  'engine.cpp',
  'tickers.cpp',
  'resource.cpp',
  'texture.cpp',
  'sound.cpp',
  include_directories : include_directories('.'),
  install : false)

gods98_dep = declare_dependency(link_with : gods98_lib,
  include_directories : include_directories('.'))

# Resource-loader validation (finding 01). Runs a synthetic WWAD round trip
# always; point test_wads_dir at the shipped wads to add real-wad checks:
#   meson setup build -Dtest_wads_dir=/path/to/shipped/game
#   meson test -C build --suite resource
if get_option('platform') == 'native'
  resource_test = executable('resource_test',
    'test_resource.cpp',
    include_directories : include_directories('.'),
    link_with : gods98_lib,
    install : false)
  test_wads = get_option('test_wads_dir')
  test('resource', resource_test,
    args : test_wads == '' ? [] : ['--real', test_wads],
    suite : 'resource')

  # Texture loader validation (finding 13): synthetic BMP round trips always;
  # real-wad checks compare pixel hashes against PIL-derived references.
  texture_test = executable('texture_test',
    'test_texture.cpp',
    include_directories : include_directories('.'),
    link_with : gods98_lib,
    install : false)
  test('texture', texture_test,
    args : test_wads == '' ? [] : ['--real', test_wads],
    suite : 'texture')

  # Sound loader validation (finding 14): synthetic RIFF/WAVE round trips
  # always; real-wad checks parse all 387 shipped .wav and match the five
  # reference entries against wave-module-derived PCM hashes.
  sound_test = executable('sound_test',
    'test_sound.cpp',
    include_directories : include_directories('.'),
    link_with : gods98_lib,
    install : false)
  test('sound', sound_test,
    args : test_wads == '' ? [] : ['--real', test_wads],
    suite : 'sound')
endif