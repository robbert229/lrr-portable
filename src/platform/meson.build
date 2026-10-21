# The platform seam (docs/adr/0004).
#
# One of two entry points is compiled depending on the `platform` option:
#   native : src/platform/native/entry.cpp — miniwin over SDL3 (primary)
#   winapi : src/platform/winapi/entry.cpp — real DirectX via MinGW (reference)
# Both call the backend-neutral `LegorrrMain()` in src/main.cpp.

platform = get_option('platform')
platform_inc = include_directories('../..')   # resolve src/platform/platform.h

if platform == 'winapi'
  platform_entry = files('winapi/entry.cpp')
  platsub_deps = []
elif platform == 'native'
  platform_entry = files('native/entry.cpp')
  # miniwin is a CMake project hosted in the sparse isle-portable submodule
  # (3rdparty/isle-portable/miniwin). A thin wrapper at
  # subprojects/miniwin/CMakeLists.txt bridges meson's cmake.subproject to it
  # (docs/adr/0004, Q-A/Q-F). miniwin's own CMake finds SDL3 — the host needs
  # SDL3 devel headers.
  cmake = import('cmake')
  miniwin_sub = cmake.subproject('miniwin')
  miniwin_dep = miniwin_sub.dependency('miniwin')
  # miniwin's public headers live on the separate INTERFACE target
  # `miniwin-headers`; make sure the include dir reaches our entries.
  miniwin_headers_dep = miniwin_sub.dependency('miniwin-headers')
  platsub_deps = [miniwin_dep, miniwin_headers_dep]
else
  error('unknown platform: ' + platform)
endif