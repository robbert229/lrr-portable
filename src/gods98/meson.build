# SPDX-License-Identifier: LGPL-3.0-or-later
gods98_lib = static_library('gods98',
  'gods98.cpp',
  'engine.cpp',
  'tickers.cpp',
  include_directories : include_directories('.'),
  install : false)

gods98_dep = declare_dependency(link_with : gods98_lib,
  include_directories : include_directories('.'))