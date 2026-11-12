# SPDX-License-Identifier: LGPL-3.0-or-later
gods98_incs = [include_directories('.'), include_directories('..'),
               miniwin_inc]

gods98_backend_sources = []
if get_option('platform') == 'winapi'
  gods98_backend_sources += files('lwsc_winapi.cpp')
endif

gods98_sources = [
  'mesh_unit.cpp',
  'container_unit.cpp',
  'directdraw.cpp',
  'avi_player.cpp',
  'images_unit.cpp',
  'files.cpp',
  'lwo.cpp',
  'wad.cpp',
  'lws.cpp',
  'viewports.cpp',
  'com_object_tree.cpp',
  'file_list_streams.cpp',
  'text_window.cpp',
  'flic_unit.cpp',
  'input_unit.cpp',
  'bitmap_font.cpp',
  'ptrstack.cpp',
  'message.cpp',
  'input.cpp',
  'mainsim.cpp',
  'engine.cpp',
  'simtick.cpp',
  'camera.cpp',
  'media.cpp',
  'record.cpp',
  'sound.cpp',
  'scene_pool.cpp',
  'entity.cpp',
  'view.cpp',
  'texture.cpp',
  'resource.cpp',
  'scene_resources.cpp',
  'frameloop.cpp',
  'audio.cpp',
  'surface.cpp',
  'movie.cpp',
  'flic.cpp',
  'heap.cpp',
  'string.cpp',
  'model.cpp',
  'block.cpp',
  'draw_port.cpp',
  'gods98.cpp',
  'lwsc_scene.cpp',
  'lwsc_miniwin.cpp',
  'container_native.cpp',
  'container_route.cpp',
  'registry_port.cpp',
  'sound_wave.cpp',
  'tickers.cpp',
  'winmm_port.cpp',
  '../crt/crt_runtime_aliases.cpp',
  '../crt/crt_runtime_calls.cpp',
  '../crt/crt.cpp',
  '../crt/crt_heap.cpp',
]
gods98_c_sources = [
  'utils.c',
  'config.c',
  'maths.c',
  'keys.c',
  'draw.c',
  'soundsys.c',
  'sound3d.c',
  'g98_024.c',
  'g98_026.c',
  'dxerror.c',
  'g98_029.c',
]
# Native/reference builds compile original-shape C units as C++ (ADR 0005).
gods98_c_as_cpp = []
foreach source : gods98_c_sources
  gods98_c_as_cpp += configure_file(input : source, output : '@PLAINNAME@.cpp', copy : true)
endforeach

gods98_lib = static_library('gods98',
  gods98_sources, gods98_c_as_cpp, gods98_backend_sources,
  include_directories : gods98_incs,
  dependencies : [platsub_deps, platform_glue_dep],
  install : false)

gods98_dep = declare_dependency(link_with : gods98_lib,
  include_directories : include_directories('.'),
  dependencies : [movie_dl_dep, platsub_deps])
