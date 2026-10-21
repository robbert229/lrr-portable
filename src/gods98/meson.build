# SPDX-License-Identifier: LGPL-3.0-or-later
gods98_lib = static_library('gods98',
  'gods98.cpp',
  'engine.cpp',
  'tickers.cpp',
  'resource.cpp',
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
endif