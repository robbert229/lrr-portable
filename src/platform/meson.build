# The platform seam (docs/adr/0004).
#
# One of two backends is compiled depending on the `platform` option:
#   native : src/platform/native/ — miniwin over SDL3 (primary)
#   winapi : src/platform/winapi/ — real DirectX via MinGW (reference)
# The window/events/present/audio glue (miniwin is a framebuffer seam; it
# never creates the window, pumps events, presents, or plays audio — app's
# job) is its own tiny static lib so both the game exe and the per-subsystem
# tests link it through gods98.

platform = get_option('platform')

if platform == 'winapi'
  platform_entry = files('winapi/entry.cpp')
  platform_glue = static_library('platform_glue',
    'winapi/frameloop.cpp',
    include_directories : include_directories('..'),
    install : false)
  platsub_deps = []
  miniwin_inc = []
elif platform == 'native'
  platform_entry = files('native/entry.cpp')
  # Build miniwin directly so Meson owns its compile and link graph. Renderer
  # selection lives at the platform seam, where other backends can be added.
  fs = import('fs')
  miniwin_root_rel = '3rdparty/isle-portable/miniwin/include'
  if not fs.exists(meson.project_source_root() / miniwin_root_rel / 'miniwin' / 'windows.h')
    error('miniwin not bootstrapped — run `tools/setup-miniwin.sh` '
          + '(native backend needs the sparse isle-portable submodule)')
  endif
  miniwin_inc = include_directories('../../' + miniwin_root_rel)
  miniwin_src = '../../3rdparty/isle-portable/miniwin'
  miniwin_sdl = dependency('sdl3', required : true)
  miniwin_generated = miniwin_src / 'src/d3drm/backends/sdl3gpu/shaders/generated'
  miniwin_inc_dirs = [
    miniwin_inc,
    include_directories(miniwin_src / 'src/internal'),
    include_directories(miniwin_generated),
  ]
  miniwin_cpp_args = ['-DMINIWIN', '-DUSE_SOFTWARE_RENDER', '-DUSE_PALETTE_SW_RENDER']
  if get_option('buildtype') == 'debug'
    miniwin_cpp_args += '-DDEBUG'
  endif
  miniwin_lib = static_library('miniwin',
    miniwin_src / 'src/windows/windows.cpp',
    miniwin_src / 'src/ddraw/ddpalette.cpp',
    miniwin_src / 'src/ddraw/ddraw.cpp',
    miniwin_src / 'src/ddraw/ddsurface.cpp',
    miniwin_src / 'src/ddraw/framebuffer.cpp',
    miniwin_src / 'src/d3drm/d3drm.cpp',
    miniwin_src / 'src/d3drm/d3drmdevice.cpp',
    miniwin_src / 'src/d3drm/d3drmframe.cpp',
    miniwin_src / 'src/d3drm/d3drmlight.cpp',
    miniwin_src / 'src/d3drm/d3drmmesh.cpp',
    miniwin_src / 'src/d3drm/d3drmtexture.cpp',
    miniwin_src / 'src/d3drm/d3drmviewport.cpp',
    miniwin_src / 'src/d3drm/d3drmrenderer.cpp',
    miniwin_src / 'src/internal/meshutils.cpp',
    miniwin_src / 'src/d3drm/backends/software/renderer.cpp',
    miniwin_src / 'src/d3drm/backends/palettesw/renderer.cpp',
    include_directories : miniwin_inc_dirs,
    dependencies : miniwin_sdl,
    cpp_args : miniwin_cpp_args,
    install : false)
  miniwin_dep = declare_dependency(
    link_with : miniwin_lib,
    include_directories : miniwin_inc,
    dependencies : miniwin_sdl,
    compile_args : ['-DMINIWIN'])
  platsub_deps = [miniwin_dep]

  platform_glue = static_library('platform_glue',
    'native/frameloop.cpp',
    include_directories : [include_directories('..'), miniwin_inc],
    dependencies : [platsub_deps],
    install : false)
else
  error('unknown platform: ' + platform)
endif

platform_glue_dep = declare_dependency(link_with : platform_glue,
  include_directories : include_directories('..'))
