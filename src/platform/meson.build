# The platform seam (docs/adr/0004).
#
# One of two entry points is compiled depending on the `platform` option:
#   native : src/platform/native/entry.cpp — miniwin over SDL3 (primary)
#   winapi : src/platform/winapi/entry.cpp — real DirectX via MinGW (reference)
# Both call the backend-neutral `LegorrrMain()` in src/main.cpp.

platform = get_option('platform')

if platform == 'winapi'
  platform_entry = files('winapi/entry.cpp')
  platsub_deps = []
  miniwin_inc = []
elif platform == 'native'
  platform_entry = files('native/entry.cpp')
  # miniwin is a CMake project hosted in the sparse isle-portable submodule
  # (3rdparty/isle-portable/miniwin). A thin wrapper at
  # subprojects/miniwin/CMakeLists.txt bridges meson's cmake.subproject to it
  # (docs/adr/0004, Q-A/Q-F). miniwin's own CMake finds SDL3 — the host needs
  # SDL3 devel headers.
  fs = import('fs')
  miniwin_root_rel = '3rdparty/isle-portable/miniwin/include'
  if not fs.exists(meson.project_source_root() / miniwin_root_rel / 'miniwin' / 'windows.h')
    error('miniwin not bootstrapped — run `tools/setup-miniwin.sh` '
          + '(native backend needs the sparse isle-portable submodule)')
  endif
  # miniwin's public headers live on the separate INTERFACE target
  # `miniwin-headers`; meson's cmake module doesn't propagate its
  # BUILD_INTERFACE include dir, so pin the include path here at the seam.
  # (platform-relative: this meson.build lives in src/platform/.)
  miniwin_inc = include_directories('../../' + miniwin_root_rel)
  cmake = import('cmake')
  miniwin_sub = cmake.subproject('miniwin')
  miniwin_dep = miniwin_sub.dependency('miniwin')
  miniwin_headers_dep = miniwin_sub.dependency('miniwin-headers')
  platsub_deps = [miniwin_dep, miniwin_headers_dep]
else
  error('unknown platform: ' + platform)
endif